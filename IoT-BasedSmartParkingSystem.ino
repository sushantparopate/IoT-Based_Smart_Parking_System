/*
  Smart IoT Based Parking System
  ESP32 + Blynk IoT + IR Sensors + Servo Gate + OLED Display

  Features:
    - Automatic gate control on vehicle entry/exit (IR sensor based)
    - Real-time occupancy tracking for 3 parking spots
    - OLED status display (spot availability + system status)
    - Blynk app integration (live status, manual gate override)
    - Auto-reconnect if WiFi drops

  Libraries required (Arduino Library Manager):
    - Blynk (Blynk IoT)
    - ESP32Servo
    - U8g2

  Board: ESP32 Dev Module
*/

// ── Credentials ───────────────────────────────────────────
// Blynk template info + WiFi networks live in secrets.h, which is
// gitignored so it never gets pushed. Copy secrets.h.example to
// secrets.h in this same folder and fill in your real values.
#include "secrets.h"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <U8g2lib.h>

// ── Pin definitions ──────────────────────────────────────
#define IR_OUTSIDE  14   // Detects vehicle approaching gate from outside
#define IR_INSIDE   27   // Detects vehicle approaching gate from inside
#define IR_SPOT1    26
#define IR_SPOT2    25
#define IR_SPOT3    33
#define SERVO_PIN   12
#define OLED_SDA    21
#define OLED_SCK    22

// ── Timing ────────────────────────────────────────────────
#define GATE_TIMEOUT    5000   // ms to wait for vehicle to reach the gate
#define BLYNK_INTERVAL  500    // ms between Blynk sync updates

// ── OLED ──────────────────────────────────────────────────
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, OLED_SCK, OLED_SDA);

Servo gateServo;
BlynkTimer timer;

// ── State ─────────────────────────────────────────────────
bool spot1Occupied = false;
bool spot2Occupied = false;
bool spot3Occupied = false;
String statusMsg    = "System Ready";
String gateActivity = "Gate: Idle";
bool manualOverride = false;

enum GateState {
  IDLE,
  ENTERING_WAIT_INSIDE,
  ENTERING_WAIT_CLEAR,
  EXITING_WAIT_OUTSIDE,
  EXITING_WAIT_CLEAR
};

GateState gateState = IDLE;
unsigned long gateTimer = 0;


// ─── DEBOUNCED IR READ ───────────────────────────────────
bool stableRead(int pin) {
  int count = 0;
  for (int i = 0; i < 5; i++) {
    if (digitalRead(pin) == LOW) count++;
    delay(2);
  }
  return count >= 3;
}


// ─── DISPLAY ────────────────────────────────────────────
void drawSpot(int x, bool occupied, const char* label) {
  display.drawRFrame(x, 14, 38, 32, 3);
  display.setFont(u8g2_font_6x10_tf);
  int lx = x + (38 - strlen(label) * 6) / 2;
  display.drawStr(lx, 26, label);
  if (occupied) {
    display.drawBox(x + 4, 29, 30, 11);
    display.setDrawColor(0);
    display.setFont(u8g2_font_5x7_tf);
    display.drawStr(x + 7, 37, "FULL");
    display.setDrawColor(1);
  } else {
    display.drawFrame(x + 4, 29, 30, 11);
    display.setFont(u8g2_font_5x7_tf);
    display.drawStr(x + 7, 37, "OPEN");
  }
}

void updateDisplay() {
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  const char* title = "SMART IOT PARKING";
  int titleW = strlen(title) * 6;
  int titleX = (128 - titleW) / 2;
  display.drawStr(titleX, 10, title);
  display.drawHLine(0, 12, 128);
  drawSpot(2,  spot1Occupied, "P1");
  drawSpot(45, spot2Occupied, "P2");
  drawSpot(88, spot3Occupied, "P3");
  display.drawHLine(0, 50, 128);
  display.setFont(u8g2_font_5x7_tf);
  String s = statusMsg;
  if (s.length() > 21) s = s.substring(0, 21);
  int statusW = s.length() * 5;
  int statusX = (128 - statusW) / 2;
  display.drawStr(statusX, 60, s.c_str());
  display.sendBuffer();
}


// ─── STATUS ─────────────────────────────────────────────
void setStatus(String msg) {
  statusMsg = msg;
  Serial.println(msg);
  updateDisplay();
  Blynk.virtualWrite(V4, msg);
}

void setGateActivity(String msg) {
  gateActivity = msg;
  Blynk.virtualWrite(V5, msg);
}


// ─── GATE ────────────────────────────────────────────────
void openGate() {
  gateServo.write(135);
  setStatus("Gate Open");
}

void closeGate() {
  gateServo.write(45);
  setStatus("Gate Closed");
}


// ─── MANUAL OVERRIDE (V7) ───────────────────────────────
BLYNK_WRITE(V7) {
  int val = param.asInt();
  if (val == 1) {
    manualOverride = true;
    openGate();
    setGateActivity("Manual Override - Gate OPENED");
    Blynk.virtualWrite(V3, "OPEN");
    Serial.println("Manual Override: Gate OPENED");
  } else {
    manualOverride = false;
    closeGate();
    setGateActivity("Manual Override OFF - Gate CLOSED");
    Blynk.virtualWrite(V3, "CLOSED");
    Serial.println("Manual Override: Gate CLOSED");
  }
}


// ─── PARKING SPOTS ──────────────────────────────────────
bool allSpotsFull() {
  return spot1Occupied && spot2Occupied && spot3Occupied;
}

bool anySpotFree() {
  return !allSpotsFull();
}

void checkParkingSpots() {
  bool s1 = stableRead(IR_SPOT1);
  bool s2 = stableRead(IR_SPOT2);
  bool s3 = stableRead(IR_SPOT3);

  bool changed = (s1 != spot1Occupied ||
                  s2 != spot2Occupied ||
                  s3 != spot3Occupied);

  spot1Occupied = s1;
  spot2Occupied = s2;
  spot3Occupied = s3;

  if (changed) {
    updateDisplay();

    // LED widgets: 1 = occupied (FULL), 0 = free
    Blynk.virtualWrite(V0, spot1Occupied ? 1 : 0);
    Blynk.virtualWrite(V1, spot2Occupied ? 1 : 0);
    Blynk.virtualWrite(V2, spot3Occupied ? 1 : 0);

    int freeSpots = (!spot1Occupied ? 1 : 0) +
                    (!spot2Occupied ? 1 : 0) +
                    (!spot3Occupied ? 1 : 0);
    Blynk.virtualWrite(V6, String(freeSpots) + "/3 spots available");

    if (allSpotsFull()) {
      setStatus("Lot Full");
      setGateActivity("Lot FULL - Entry DENIED");
    }
  }
}


// ─── GATE STATE MACHINE ──────────────────────────────────
void runGateStateMachine() {

  if (manualOverride) return;

  bool outside = stableRead(IR_OUTSIDE);
  bool inside  = stableRead(IR_INSIDE);
  unsigned long now = millis();

  switch (gateState) {

    case IDLE:
      if (outside) {
        if (anySpotFree()) {
          openGate();
          setGateActivity("Vehicle detected - Gate OPENED");
          Blynk.virtualWrite(V3, "OPEN");
          gateTimer = now;
          gateState = ENTERING_WAIT_INSIDE;
          setStatus("Vehicle Entering");
        } else {
          setStatus("Full - Denied");
          setGateActivity("Lot FULL - Entry DENIED");
          Blynk.virtualWrite(V3, "LOCKED");
          delay(2000);
          setStatus("Lot Full");
        }
      } else if (inside) {
        openGate();
        setGateActivity("Vehicle exiting - Gate OPENED");
        Blynk.virtualWrite(V3, "OPEN");
        gateTimer = now;
        gateState = EXITING_WAIT_OUTSIDE;
        setStatus("Vehicle Exiting");
      } else {
        setGateActivity("Gate Idle - Monitoring...");
      }
      break;

    case ENTERING_WAIT_INSIDE:
      if (inside) {
        gateTimer = now;
        gateState = ENTERING_WAIT_CLEAR;
        setStatus("Car Passing...");
        setGateActivity("Car passing through gate...");
      } else if (now - gateTimer >= GATE_TIMEOUT) {
        closeGate();
        Blynk.virtualWrite(V3, "CLOSED");
        setGateActivity("Timeout - Gate CLOSED");
        gateState = IDLE;
        setStatus("Timeout");
      }
      break;

    case ENTERING_WAIT_CLEAR:
      if (inside || outside) {
        gateTimer = now;
        setStatus("Car Stuck - Waiting...");
        setGateActivity("Car in gate - Holding OPEN");
      } else {
        closeGate();
        Blynk.virtualWrite(V3, "CLOSED");
        setGateActivity("Car Entered - Gate CLOSED");
        gateState = IDLE;
        setStatus("Car Entered");
      }
      if (now - gateTimer >= 30000) {
        closeGate();
        Blynk.virtualWrite(V3, "CLOSED");
        setGateActivity("Hard Timeout - Gate CLOSED");
        gateState = IDLE;
        setStatus("Hard Timeout");
      }
      break;

    case EXITING_WAIT_OUTSIDE:
      if (outside) {
        gateTimer = now;
        gateState = EXITING_WAIT_CLEAR;
        setStatus("Car Passing...");
        setGateActivity("Car passing through gate...");
      } else if (now - gateTimer >= GATE_TIMEOUT) {
        closeGate();
        Blynk.virtualWrite(V3, "CLOSED");
        setGateActivity("Timeout - Gate CLOSED");
        gateState = IDLE;
        setStatus("Timeout");
      }
      break;

    case EXITING_WAIT_CLEAR:
      if (inside || outside) {
        gateTimer = now;
        setStatus("Car Stuck - Waiting...");
        setGateActivity("Car in gate - Holding OPEN");
      } else {
        closeGate();
        Blynk.virtualWrite(V3, "CLOSED");
        setGateActivity("Car Exited - Gate CLOSED");
        gateState = IDLE;
        setStatus("Car Exited");
      }
      if (now - gateTimer >= 30000) {
        closeGate();
        Blynk.virtualWrite(V3, "CLOSED");
        setGateActivity("Hard Timeout - Gate CLOSED");
        gateState = IDLE;
        setStatus("Hard Timeout");
      }
      break;
  }
}


// ─── BLYNK SYNC ─────────────────────────────────────────
void syncBlynk() {
  Blynk.virtualWrite(V0, spot1Occupied ? 1 : 0);
  Blynk.virtualWrite(V1, spot2Occupied ? 1 : 0);
  Blynk.virtualWrite(V2, spot3Occupied ? 1 : 0);
  Blynk.virtualWrite(V3, gateState == IDLE ? "CLOSED" : "OPEN");
  Blynk.virtualWrite(V4, statusMsg);
  Blynk.virtualWrite(V5, gateActivity);
  Blynk.virtualWrite(V7, manualOverride ? 1 : 0);

  int freeSpots = (!spot1Occupied ? 1 : 0) +
                  (!spot2Occupied ? 1 : 0) +
                  (!spot3Occupied ? 1 : 0);
  Blynk.virtualWrite(V6, String(freeSpots) + "/3 spots available");
}


// ─── DISPLAY WiFi STATUS ─────────────────────────────────
void showWiFiStatus(bool connected) {
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  const char* title = "SMART IOT PARKING";
  int titleW = strlen(title) * 6;
  int titleX = (128 - titleW) / 2;
  display.drawStr(titleX, 10, title);
  display.drawHLine(0, 12, 128);
  if (connected) {
    display.drawStr(16, 35, "WiFi Connected");
    display.drawStr(28, 50, "Starting...");
  } else {
    display.drawStr(8, 30, "WiFi Disconnected");
    display.drawStr(12, 45, "Reconnecting...");
  }
  display.sendBuffer();
  delay(2000);
}


// ─── WIFI CONNECT ────────────────────────────────────────
bool connectWiFi() {
  while (true) {
    Serial.print("Trying WiFi: ");
    Serial.println(WIFI_SSID);

    display.clearBuffer();
    display.setFont(u8g2_font_6x10_tf);
    const char* title = "SMART IOT PARKING";
    int titleW = strlen(title) * 6;
    int titleX = (128 - titleW) / 2;
    display.drawStr(titleX, 10, title);
    display.drawHLine(0, 12, 128);
    display.setFont(u8g2_font_5x7_tf);
    display.drawStr(10, 35, "Connecting to WiFi...");
    String ssidShort = String(WIFI_SSID);
    if (ssidShort.length() > 21) ssidShort = ssidShort.substring(0, 21);
    display.drawStr(10, 50, ssidShort.c_str());
    display.sendBuffer();

    WiFi.disconnect(true);
    delay(200);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    for (int i = 0; i < 20; i++) {
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to WiFi");
        return true;
      }
      delay(500);
      Serial.print(".");
    }
    Serial.println(" Failed. Retrying...");

    display.clearBuffer();
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(titleX, 10, title);
    display.drawHLine(0, 12, 128);
    display.setFont(u8g2_font_5x7_tf);
    display.drawStr(10, 35, "WiFi failed.");
    display.drawStr(10, 50, "Retrying...");
    display.sendBuffer();
    delay(3000);
  }
}


// ─── SETUP & LOOP ────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  pinMode(IR_OUTSIDE, INPUT);
  pinMode(IR_INSIDE,  INPUT);
  pinMode(IR_SPOT1,   INPUT);
  pinMode(IR_SPOT2,   INPUT);
  pinMode(IR_SPOT3,   INPUT);

  gateServo.attach(SERVO_PIN);

  Wire.begin(OLED_SDA, OLED_SCK);
  display.begin();

  closeGate();

  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  const char* title = "SMART IOT PARKING";
  int titleW = strlen(title) * 6;
  int titleX = (128 - titleW) / 2;
  display.drawStr(titleX, 10, title);
  display.drawHLine(0, 12, 128);
  display.setFont(u8g2_font_5x7_tf);
  display.drawStr(30, 35, "Connecting...");
  display.sendBuffer();

  connectWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    showWiFiStatus(true);
    Blynk.config(BLYNK_AUTH_TOKEN);
    Blynk.connect();
  } else {
    showWiFiStatus(false);
  }

  timer.setInterval(BLYNK_INTERVAL, syncBlynk);

  timer.setInterval(5000L, []() {
    if (WiFi.status() != WL_CONNECTED) {
      showWiFiStatus(false);
      if (connectWiFi()) {
        showWiFiStatus(true);
        Blynk.config(BLYNK_AUTH_TOKEN);
        Blynk.connect();
      }
    }
  });

  checkParkingSpots();
  setStatus("System Ready");
  setGateActivity("Gate Idle - Monitoring...");
}

void loop() {
  Blynk.run();
  timer.run();
  checkParkingSpots();
  runGateStateMachine();
  delay(50);
}
