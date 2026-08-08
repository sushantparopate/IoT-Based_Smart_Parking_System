\# 🅿️ IoT-Based\_Smart\_Parking\_System





An automated parking system built on ESP32 that tracks real-time occupancy of 3 parking spots, controls a servo-driven boom gate based on IR sensor input, displays live status on an OLED screen, and syncs everything to the \*\*Blynk IoT\*\* app for remote monitoring and manual override.



!\[Platform](https://img.shields.io/badge/platform-ESP32-blue)

!\[Framework](https://img.shields.io/badge/framework-Arduino-00979D)

!\[IoT](https://img.shields.io/badge/IoT-Blynk-green)

!\[License](https://img.shields.io/badge/license-MIT-lightgrey)



\---



\## 📷 Overview



The system uses 5 IR sensors — one at the outer gate, one at the inner gate, and one per parking spot — to automatically detect vehicle entry/exit and manage a servo gate, while continuously updating spot occupancy on both a local OLED display and the Blynk dashboard.



\*\*Core behavior:\*\*

\- Gate opens automatically when a vehicle is detected outside \*\*and\*\* a spot is free

\- Gate opens automatically for vehicles exiting

\- Entry is denied (gate stays shut) if the lot is full

\- Manual gate override available from the Blynk app

\- Auto-reconnect across up to 3 saved WiFi networks if the connection drops



\---



\## ✨ Features



\- 🚗 Real-time 3-spot occupancy tracking (debounced IR reads)

\- 🚦 Automatic entry/exit gate control via a finite state machine

\- 📟 Live OLED dashboard (spot status + system status)

\- 📱 Blynk app integration — live spot status, gate state, free-spot count, manual override toggle

\- 📶 Multi-network WiFi fallback with auto-reconnect

\- ⏱️ Timeout handling for stalled/stuck vehicles at the gate



\---



\## 🔧 Hardware Required



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



\---



\## 🔌 Pin Configuration



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



\---



\## ⚡ Servo Power \& Decoupling



The ESP32 outputs logic at \*\*3.3V\*\* and its onboard regulator can only supply a small amount of current — nowhere near enough to drive a servo, especially at the stall current it draws the instant the gate starts or stops moving. Trying to power the servo from the ESP32's 5V/VIN pin is a common source of random resets and browned-out WiFi, so this build powers the servo from a \*\*separate external 5V supply\*\*, with only the signal wire coming from the ESP32.



\*\*Wiring rule:\*\* ESP32 and servo do \*\*not\*\* share a power rail — but they \*\*do\*\* share a common ground. The ESP32 GND, the servo GND, and the external supply's GND must all be tied together, or the PWM signal has no reference and the servo will behave erratically.



\*\*Capacitors on the servo power rail\*\* (placed as close to the servo's V+/GND leads as possible):

\- \*\*1000µF, 16V electrolytic\*\* — acts as a local energy reservoir ("tank cap"). When the servo motor moves, it draws a short, sharp current spike; without a bulk cap nearby, that spike drags the whole 5V rail down, which is what causes ESP32 brownouts and glitching.

\- \*\*100nF (0.1µF) ceramic\*\*, in parallel with the electrolytic — the electrolytic is too slow to respond to the very fast switching noise the motor's brushes generate, so the ceramic cap handles that high-frequency ripple.



Both capacitors go across the same two rail wires, right at the servo connector — not back at the power supply.



\---



\## 📚 Software Requirements



Install via Arduino IDE \*\*Library Manager\*\*:



\- \[`Blynk`](https://github.com/blynkkk/blynk-library) (Blynk IoT)

\- `ESP32Servo`

\- `U8g2` (by olikraus)



\*\*Board support:\*\* Install the `esp32` board package (via Boards Manager, using the Espressif package URL) and select \*\*ESP32 Dev Module\*\* as the board.



\---



\## ⚙️ Setup



1\. \*\*Clone the repo\*\*

&#x20;  ```bash

&#x20;  git clone https://github.com/<your-username>/smart-iot-parking-system.git

&#x20;  ```



2\. \*\*Create a Blynk template\*\*

&#x20;  - Sign up / log in at \[blynk.cloud](https://blynk.cloud)

&#x20;  - Create a new template and add these datastreams:



&#x20;    | Virtual Pin | Type    | Purpose                        |

&#x20;    |-------------|---------|----------------------------------|

&#x20;    | V0          | LED     | Spot 1 occupied                 |

&#x20;    | V1          | LED     | Spot 2 occupied                 |

&#x20;    | V2          | LED     | Spot 3 occupied                 |

&#x20;    | V3          | Label   | Gate state (OPEN/CLOSED/LOCKED) |

&#x20;    | V4          | Label   | System status message           |

&#x20;    | V5          | Label   | Gate activity log                |

&#x20;    | V6          | Label   | Free spot count                  |

&#x20;    | V7          | Switch  | Manual gate override             |



&#x20;  - Copy your \*\*Template ID\*\*, \*\*Template Name\*\*, and device \*\*Auth Token\*\*



3\. \*\*Fill in your credentials\*\* in `SmartIoTParkingSystem.ino`:

&#x20;  ```cpp

&#x20;  #define BLYNK\_TEMPLATE\_ID   "YOUR\_BLYNK\_TEMPLATE\_ID"

&#x20;  #define BLYNK\_TEMPLATE\_NAME "YOUR\_BLYNK\_TEMPLATE\_NAME"

&#x20;  #define BLYNK\_AUTH\_TOKEN    "YOUR\_BLYNK\_AUTH\_TOKEN"



&#x20;  #define WIFI\_SSID   "YOUR\_WIFI\_SSID\_1"

&#x20;  #define WIFI\_PASS   "YOUR\_WIFI\_PASSWORD\_1"

&#x20;  // ...and networks 2 \& 3

&#x20;  ```



4\. \*\*Wire the hardware\*\* per the pin table and power/decoupling section above.



5\. \*\*Upload\*\* to the ESP32 via Arduino IDE.



> ⚠️ Never commit real WiFi passwords or Blynk auth tokens. Keep your filled-in `.ino` out of version control, or use a gitignored `secrets.h` if you split credentials out.



\---



\## 🧠 How It Works



The gate is driven by a finite state machine:



```

IDLE → (vehicle outside + spot free) → ENTERING\_WAIT\_INSIDE → ENTERING\_WAIT\_CLEAR → IDLE

IDLE → (vehicle inside)              → EXITING\_WAIT\_OUTSIDE  → EXITING\_WAIT\_CLEAR  → IDLE

```



\- Each state has a timeout (5s to reach the gate, 30s hard timeout while clearing) to prevent the gate getting stuck open.

\- Parking spot state is polled and debounced every loop, and only pushed to Blynk/OLED when it changes.

\- Manual override (V7) bypasses the state machine entirely and directly opens/closes the gate.



\---



\## 📁 Project Structure



```

smart-iot-parking-system/

├── SmartIoTParkingSystem.ino   # Main firmware

├── README.md

└── LICENSE

```



\---



\## 📄 License



MIT — see \[LICENSE](LICENSE).





