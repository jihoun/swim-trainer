// Swim power meter: two band load cells -> propulsive watts over BLE.
//
// Left/right HX711 -> SidePipeline (filter, band model, stroke detection)
//   -> PowerCombiner (3 s power, L/R balance, stroke cycles)
//   -> BLE Cycling Power Service, readable by Garmin / Zwift / any power app.

#include <Arduino.h>
#include <HX711_ADC.h>
#include <NimBLEDevice.h>
#include <Preferences.h>

#include "BandModel.h"
#include "CpsPacket.h"
#include "PowerCombiner.h"
#include "SidePipeline.h"
#include "bands.h"
#include "config.h"

using swimpower::BandModel;
using swimpower::PowerCombiner;
using swimpower::Side;
using swimpower::SidePipeline;
using swimpower::SideResult;

namespace {

HX711_ADC cellL(kPinLeftDout, kPinLeftSck);
HX711_ADC cellR(kPinRightDout, kPinRightSck);

SidePipeline pipeL(kFilterAlpha);
SidePipeline pipeR(kFilterAlpha);
PowerCombiner combiner;
Preferences prefs;

size_t activeSet = bands::kDefaultSet;
bool streaming = false;
float lastForceL = 0.0f, lastForceR = 0.0f;

// Sample-rate measurement
uint32_t rateWindowStart = 0;
uint32_t samplesL = 0, samplesR = 0;
float rateL = 0.0f, rateR = 0.0f;

// BLE
NimBLECharacteristic* cpsMeasurement = nullptr;
uint32_t lastNotifyMs = 0;

// ---------------------------------------------------------------------------
void applyBandSet(size_t idx) {
  if (idx >= bands::kSetCount) return;
  activeSet = idx;
  const bands::BandSet& s = bands::kSets[idx];
  pipeL.setBand(BandModel(s.left, s.left_count));
  pipeR.setBand(BandModel(s.right, s.right_count));
  Serial.printf("band set: %s\n", s.name);
}

void loadCalibration() {
  prefs.begin("swim", true);
  cellL.setCalFactor(prefs.getFloat("calL", kDefaultCalLeft));
  cellR.setCalFactor(prefs.getFloat("calR", kDefaultCalRight));
  if (prefs.isKey("tareL")) cellL.setTareOffset(prefs.getLong("tareL"));
  if (prefs.isKey("tareR")) cellR.setTareOffset(prefs.getLong("tareR"));
  activeSet = prefs.getUInt("band", bands::kDefaultSet);
  prefs.end();
}

void saveCalibration() {
  prefs.begin("swim", false);
  prefs.putFloat("calL", cellL.getCalFactor());
  prefs.putFloat("calR", cellR.getCalFactor());
  prefs.putLong("tareL", cellL.getTareOffset());
  prefs.putLong("tareR", cellR.getTareOffset());
  prefs.putUInt("band", activeSet);
  prefs.end();
  Serial.println("saved");
}

// ---------------------------------------------------------------------------
void setupBle() {
  NimBLEDevice::init(kDeviceName);
  NimBLEServer* server = NimBLEDevice::createServer();

  NimBLEService* cps = server->createService(NimBLEUUID(static_cast<uint16_t>(0x1818)));
  cpsMeasurement = cps->createCharacteristic(NimBLEUUID(static_cast<uint16_t>(0x2A63)),
                                             NIMBLE_PROPERTY::NOTIFY);
  NimBLECharacteristic* feature =
      cps->createCharacteristic(NimBLEUUID(static_cast<uint16_t>(0x2A65)), NIMBLE_PROPERTY::READ);
  const uint32_t featureValue = swimpower::kCpsFeature;
  feature->setValue(reinterpret_cast<const uint8_t*>(&featureValue), sizeof(featureValue));
  NimBLECharacteristic* location =
      cps->createCharacteristic(NimBLEUUID(static_cast<uint16_t>(0x2A5D)), NIMBLE_PROPERTY::READ);
  const uint8_t locationValue = 0;  // Other
  location->setValue(&locationValue, 1);
  cps->start();

  NimBLEService* dis = server->createService(NimBLEUUID(static_cast<uint16_t>(0x180A)));
  dis->createCharacteristic(NimBLEUUID(static_cast<uint16_t>(0x2A29)), NIMBLE_PROPERTY::READ)
      ->setValue("DIY");
  dis->createCharacteristic(NimBLEUUID(static_cast<uint16_t>(0x2A24)), NIMBLE_PROPERTY::READ)
      ->setValue("swim-trainer");
  dis->start();

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(NimBLEUUID(static_cast<uint16_t>(0x1818)));
  adv->setAppearance(0x0484);  // Cycling: Power Sensor
  adv->setName(kDeviceName);
  adv->enableScanResponse(true);
  adv->start();
  Serial.println("BLE advertising as SwimPower (Cycling Power Service)");
}

void notifyPower(uint32_t now) {
  if (!cpsMeasurement || now - lastNotifyMs < kNotifyIntervalMs) return;
  lastNotifyMs = now;
  uint8_t buf[swimpower::kCpsPacketSize];
  const size_t len = swimpower::encodeCpsMeasurement(
      buf, combiner.powerW(now), combiner.leftBalancePct(now), combiner.cycles(),
      combiner.lastCycleMs());
  cpsMeasurement->setValue(buf, len);
  cpsMeasurement->notify();
}

// ---------------------------------------------------------------------------
void handleSample(Side side, HX711_ADC& cell, SidePipeline& pipe, float sign, uint32_t now) {
  const float forceN = sign * cell.getData() * kGravity;
  const SideResult r = pipe.update(now, forceN);
  combiner.addWork(side, now, r.work_delta_j);
  if (side == Side::Left) {
    lastForceL = r.force_n;
    ++samplesL;
  } else {
    lastForceR = r.force_n;
    ++samplesR;
  }
  if (r.stroke_done) {
    combiner.onStroke(side, now);
    if (!streaming) {
      Serial.printf("stroke %c  peak %.0f N  work %.1f J  dur %lu ms  rise %lu ms\n",
                    side == Side::Left ? 'L' : 'R', r.stroke.peak_n, r.stroke.work_j,
                    static_cast<unsigned long>(r.stroke.duration_ms),
                    static_cast<unsigned long>(r.stroke.rise_ms));
    }
  }
}

void printStatus(uint32_t now) {
  const swimpower::StrokeConfig& c = pipeL.detector().config();
  Serial.printf("band %s | cal L %.1f R %.1f | thr on %.0f off %.0f N\n",
                bands::kSets[activeSet].name, cellL.getCalFactor(), cellR.getCalFactor(),
                c.on_threshold_n, c.off_threshold_n);
  Serial.printf("force L %.1f R %.1f N | baseline L %.1f R %.1f N\n", lastForceL, lastForceR,
                pipeL.detector().baseline(), pipeR.detector().baseline());
  Serial.printf("power %.0f W | balance L %.0f%% | strokes %lu | cycles %u\n",
                combiner.powerW(now), combiner.leftBalancePct(now),
                static_cast<unsigned long>(combiner.armStrokes()), combiner.cycles());
}

void handleCommand(String line) {
  line.trim();
  if (line.isEmpty()) return;
  const uint32_t now = millis();

  if (line == "tare") {
    Serial.println("taring, keep load cells unloaded...");
    cellL.tare();
    cellR.tare();
    Serial.println("tared (use `save` to keep)");
  } else if (line.startsWith("cal ")) {
    // cal L 5.0  |  cal R 5.0   (known hanging mass in kg)
    const char sideChar = toupper(line.charAt(4));
    const float kg = line.substring(6).toFloat();
    if (kg <= 0.0f || (sideChar != 'L' && sideChar != 'R')) {
      Serial.println("usage: cal L|R <kg>");
      return;
    }
    HX711_ADC& cell = sideChar == 'L' ? cellL : cellR;
    cell.refreshDataSet();
    const float factor = cell.getNewCalibration(kg);
    Serial.printf("cal %c factor %.2f (use `save` to keep)\n", sideChar, factor);
  } else if (line == "stream on") {
    streaming = true;
    Serial.println("t_ms,F_L,F_R");
  } else if (line == "stream off") {
    streaming = false;
  } else if (line == "rate") {
    Serial.printf("samples/s L %.1f R %.1f\n", rateL, rateR);
  } else if (line.startsWith("band ")) {
    const int n = line.substring(5).toInt();
    applyBandSet(static_cast<size_t>(n - 1));
  } else if (line.startsWith("thr ")) {
    // thr <on_N> <off_N>
    const int sp = line.indexOf(' ', 4);
    swimpower::StrokeConfig c = pipeL.detector().config();
    c.on_threshold_n = line.substring(4, sp).toFloat();
    if (sp > 0) c.off_threshold_n = line.substring(sp + 1).toFloat();
    pipeL.setStrokeConfig(c);
    pipeR.setStrokeConfig(c);
    Serial.printf("thresholds on %.0f off %.0f N\n", c.on_threshold_n, c.off_threshold_n);
  } else if (line == "status") {
    printStatus(now);
  } else if (line == "save") {
    saveCalibration();
  } else {
    Serial.println("commands: tare | cal L|R <kg> | stream on|off | rate | band 1|2|3 | "
                   "thr <on> <off> | status | save");
  }
}

}  // namespace

// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nswim-trainer power meter");

  cellL.begin();
  cellR.begin();
  // Start both cells; skip tare here, the stored offset is used instead.
  byte readyL = 0, readyR = 0;
  while (!readyL || !readyR) {
    if (!readyL) readyL = cellL.startMultiple(2000, false);
    if (!readyR) readyR = cellR.startMultiple(2000, false);
  }
  if (cellL.getTareTimeoutFlag()) Serial.println("WARNING: left HX711 not responding, check wiring");
  if (cellR.getTareTimeoutFlag()) Serial.println("WARNING: right HX711 not responding, check wiring");

  loadCalibration();
  pipeL.setStrokeConfig(defaultStrokeConfig());
  pipeR.setStrokeConfig(defaultStrokeConfig());
  applyBandSet(activeSet);

  setupBle();
  rateWindowStart = millis();
}

void loop() {
  const uint32_t now = millis();

  bool fresh = false;
  if (cellL.update()) {
    handleSample(Side::Left, cellL, pipeL, kSignLeft, now);
    fresh = true;
  }
  if (cellR.update()) {
    handleSample(Side::Right, cellR, pipeR, kSignRight, now);
    fresh = true;
  }
  if (fresh && streaming) {
    Serial.printf("%lu,%.2f,%.2f\n", static_cast<unsigned long>(now), lastForceL, lastForceR);
  }

  if (now - rateWindowStart >= 1000) {
    const float secs = (now - rateWindowStart) / 1000.0f;
    rateL = samplesL / secs;
    rateR = samplesR / secs;
    samplesL = samplesR = 0;
    rateWindowStart = now;
  }

  notifyPower(now);

  static String line;
  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      handleCommand(line);
      line = "";
    } else {
      line += c;
    }
  }
}
