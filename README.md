# 🅿️ Smart IoT Based Parking System

**Automatic gate control and live occupancy tracking for a 3-spot parking lot.**

*ESP32 + IR sensors + servo gate + OLED display, synced to the Blynk IoT app.*

![License](https://img.shields.io/badge/license-MIT-lightgrey)
![Platform](https://img.shields.io/badge/platform-ESP32-blue)
![Framework](https://img.shields.io/badge/framework-Arduino-00979D)
![IoT](https://img.shields.io/badge/IoT-Blynk-green)

[Wiring](#-servo-power--decoupling) • [Setup](#️-setup) • [How it works](#-how-it-works)

<!-- Add a photo or short clip of the built gate here, e.g. docs/demo.gif -->

---

## Why

Small parking lots — a housing society, a small office, a college block — don't need a barcode-and-camera ANPR system. They need something that knows when a spot is free and doesn't make people get out of the car to lift a gate.

This project is that: a self-contained ESP32 unit that watches the lot with IR sensors, opens and closes a boom gate on its own, and reports live status to a phone app — with no cloud service beyond Blynk and no dependency on the internet to keep working locally.

## What it is

- A self-contained gate controller — no phone app is required for the gate itself to function
- Real-time occupancy tracking for 3 spots, shown on a local OLED and in the Blynk app
- A finite-state-machine-driven gate, not a simple "IR sensor → relay" toggle

## What it is not

- Not a license-plate recognition or camera-based system
- Not cloud-dependent — Blynk is for remote monitoring/override only, the gate logic runs entirely on-device
- Not a payment or ticketing system

---

## Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                        ESP32 Dev Board                        │
│                                                                 │
│   IR sensors (x5) ──▶  Gate state   ──▶  OLED display          │
│   in/out + 3 spots     machine           (spot + status view)  │
│                            │                                   │
│                            ▼                                   │
│                      Servo (boom gate) ◀── External 5V         │
│                                              + decoupling caps  │
└───────────────────────────┬────────────────────────────────────┘
                             │ WiFi
                             ▼
                   ┌───────────────────┐
                   │   Blynk IoT app    │
                   │  live status,       │
                   │  manual override    │
                   └───────────────────┘
```

---

## Quick Start

1. Wire it up per the [pin table](#-pin-configuration) and [power section](#-servo-power--decoupling) below
2. Install `Blynk`, `ESP32Servo`, and `U8g2` via the Arduino Library Manager
3. Fill in your WiFi + Blynk credentials in `SmartIoTParkingSystem.ino`
4. Upload via Arduino IDE, board: **ESP32 Dev Module**

Full setup, including the Blynk template, is below.

---

## Features

- 🚗 Real-time 3-spot occupancy tracking (debounced IR reads)
- 🚦 Automatic entry/exit gate control via a finite state machine
- 📟 Live OLED dashboard (spot status + system status)
- 📱 Blynk app integration — live spot status, gate state, free-spot count, manual override toggle
- 📶 Multi-network WiFi fallback with auto-reconnect
- ⏱️ Timeout handling for stalled/stuck vehicles at the gate

---

## 🔧 Hardware Required

| Component                     | Qty | Notes                          |
|--------------------------------|-----|---------------------------------|
| ESP32 Dev Board                | 1   | Any ESP32 DevKit variant        |
| IR Obstacle Sensor Module      | 5   | 2 for gate (in/out), 3 for spots|
| SG90 / MG90S Servo Motor       | 1   | Drives the boom gate            |
| SH1106 128x64 OLED Display     | 1   | I2C interface                   |
| External 5V power supply       | 1   | Powers the servo — see below    |
| Electrolytic capacitor         | 1   | 1000µF, 16V — servo power rail  |
| Ceramic capacitor               | 1   | 100nF (0.1µF) — servo power rail|
| Jumper wires, breadboard/PCB   | –   |                                  |

## 🔌 Pin Configuration

| Signal          | ESP32 GPIO |
|------------------|-----------|
| IR – Outside gate | 14        |
| IR – Inside gate  | 27        |
| IR – Spot 1       | 26        |
| IR – Spot 2       | 25        |
| IR – Spot 3       | 33        |
| Servo signal      | 12        |
| OLED SDA          | 21        |
| OLED SCL          | 22        |

---

## ⚡ Servo Power & Decoupling

The ESP32 outputs logic at **3.3V** and its onboard regulator can only supply a small amount of current — nowhere near enough to drive a servo, especially at the stall current it draws the instant the gate starts or stops moving. Trying to power the servo from the ESP32's 5V/VIN pin is a common source of random resets and browned-out WiFi, so this build powers the servo from a **separate external 5V supply**, with only the signal wire coming from the ESP32.

**Wiring rule:** ESP32 and servo do **not** share a power rail — but they **do** share a common ground. The ESP32 GND, the servo GND, and the external supply's GND must all be tied together, or the PWM signal has no reference and the servo will behave erratically.

**Capacitors on the servo power rail** (placed as close to the servo's V+/GND leads as possible):
- **1000µF, 16V electrolytic** — acts as a local energy reservoir ("tank cap"). When the servo motor moves, it draws a short, sharp current spike; without a bulk cap nearby, that spike drags the whole 5V rail down, which is what causes ESP32 brownouts and glitching.
- **100nF (0.1µF) ceramic**, in parallel with the electrolytic — the electrolytic is too slow to respond to the very fast switching noise the motor's brushes generate, so the ceramic cap handles that high-frequency ripple.

Both capacitors go across the same two rail wires, right at the servo connector — not back at the power supply.

---

## ⚙️ Setup

1. **Clone the repo**
   ```bash
   git clone https://github.com/<your-username>/<your-repo>.git
   ```

2. **Create a Blynk template**
   - Sign up / log in at [blynk.cloud](https://blynk.cloud)
   - Create a new template and add these datastreams:

     | Virtual Pin | Type    | Purpose                        |
     |-------------|---------|----------------------------------|
     | V0          | LED     | Spot 1 occupied                 |
     | V1          | LED     | Spot 2 occupied                 |
     | V2          | LED     | Spot 3 occupied                 |
     | V3          | Label   | Gate state (OPEN/CLOSED/LOCKED) |
     | V4          | Label   | System status message           |
     | V5          | Label   | Gate activity log                |
     | V6          | Label   | Free spot count                  |
     | V7          | Switch  | Manual gate override             |

   - Copy your **Template ID**, **Template Name**, and device **Auth Token**

3. **Fill in your credentials** in `SmartIoTParkingSystem.ino`:
   ```cpp
   #define BLYNK_TEMPLATE_ID   "YOUR_BLYNK_TEMPLATE_ID"
   #define BLYNK_TEMPLATE_NAME "YOUR_BLYNK_TEMPLATE_NAME"
   #define BLYNK_AUTH_TOKEN    "YOUR_BLYNK_AUTH_TOKEN"

   #define WIFI_SSID   "YOUR_WIFI_SSID_1"
   #define WIFI_PASS   "YOUR_WIFI_PASSWORD_1"
   // ...and networks 2 & 3
   ```

4. **Wire the hardware** per the pin table and power/decoupling section above.

5. **Upload** to the ESP32 via Arduino IDE.

> ⚠️ Never commit real WiFi passwords or Blynk auth tokens. Keep your filled-in `.ino` out of version control, or use a gitignored `secrets.h` if you split credentials out.

---

## 🧠 How It Works

The gate is driven by a finite state machine:

```
IDLE → (vehicle outside + spot free) → ENTERING_WAIT_INSIDE → ENTERING_WAIT_CLEAR → IDLE
IDLE → (vehicle inside)              → EXITING_WAIT_OUTSIDE  → EXITING_WAIT_CLEAR  → IDLE
```

- Each state has a timeout (5s to reach the gate, 30s hard timeout while clearing) to prevent the gate getting stuck open.
- Parking spot state is polled and debounced every loop, and only pushed to Blynk/OLED when it changes.
- Manual override (V7) bypasses the state machine entirely and directly opens/closes the gate.

---

## Status

Working prototype — gate automation, spot tracking, OLED display, and Blynk sync are all functional. Enclosure/mounting and OTA firmware updates are in progress.

If you build one, I'd like to hear what you changed.

---

## 📁 Project Structure

```
smart-iot-parking-system/
├── SmartIoTParkingSystem.ino   # Main firmware
├── README.md
└── LICENSE
```

---

## License

[MIT](LICENSE)
