# Smart-Locker-System-using-STM32-and-ESP32-CAM
Multi-factor authentication smart locker using STM32 and ESP32-CAM
# 🔐 Smart Locker System

A secure embedded smart locker system with **multi-factor authentication** using RFID, PIN keypad, and face recognition. Built with STM32F103C8T6 (Blue Pill) as the main controller and ESP32-CAM for vision and IoT functionality. Features real-time intruder detection with **Telegram Bot API** alerts.

---

## 🧠 Overview

Traditional lockers rely on a single authentication method which can be easily compromised. This system integrates **three authentication layers** — RFID card, PIN entry, and face recognition — to provide robust, multi-factor security. Unauthorized access attempts are detected, photographed, and immediately reported to the owner's mobile device.

---

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Authentication Inputs                  │
│   RFID Reader (RC522)  +  4x4 Keypad  +  Gyroscope      │
└──────────────────────┬──────────────────────────────────┘
                       │ SPI / GPIO / I2C
                       ▼
┌─────────────────────────────────────────────────────────┐
│            STM32F103C8T6 (Central Controller)            │
│  • Reads RFID + PIN authentication                       │
│  • Reads gyroscope tamper data                           │
│  • Controls Servo Motor (lock/unlock)                    │
│  • Drives OLED display                                   │
│  • Sends UART commands to ESP32-CAM                      │
└──────────────────────┬──────────────────────────────────┘
                       │ UART
                       ▼
┌─────────────────────────────────────────────────────────┐
│                  ESP32-CAM Module                        │
│  • Face recognition processing                           │
│  • Captures intruder images                              │
│  • Sends alerts via Telegram Bot API (Wi-Fi)            │
└──────────────────────┬──────────────────────────────────┘
                       │ Wi-Fi (HTTPS)
                       ▼
              Telegram App (User Mobile)
```

---

## ✨ Features

- ✅ **Multi-Factor Authentication** — RFID + PIN + Face Recognition
- ✅ **Real-Time Intruder Detection** — captures image on unauthorized access
- ✅ **Telegram Bot Alerts** — instant photo notification to owner's phone
- ✅ **Tamper Detection** — MPU6050 gyroscope detects locker movement/tilt
- ✅ **OLED Display** — real-time status feedback
- ✅ **FreeRTOS** — non-blocking Telegram task for smooth operation
- ✅ **Low Cost** — total hardware cost ~₹2,551
- ✅ **Modular Design** — easily expandable with additional sensors

---

## 🛠️ Hardware Components

| Component | Purpose |
|-----------|---------|
| STM32F103C8T6 (Blue Pill) | Main microcontroller |
| ESP32-CAM | Face recognition + Wi-Fi + Camera |
| RFID RC522 | Card-based authentication |
| 4×4 Keypad | PIN entry |
| Servo Motor | Locker lock/unlock mechanism |
| MPU6050 Gyroscope | Tamper detection |
| SSD1306 OLED | Status display |
| Buzzer | Audio alerts |
| Li-ion Battery (5200mAh) | Power supply |
| Buck Converter + TP4056 | Power management |

---

## 💻 Software Stack

| Tool | Purpose |
|------|---------|
| STM32CubeIDE | STM32 firmware development |
| STM32CubeMX | Peripheral configuration |
| Arduino IDE | ESP32-CAM programming |
| KiCad | Circuit schematic design |
| Embedded C | STM32 firmware |
| Arduino C++ | ESP32-CAM firmware |
| Telegram Bot API | Real-time mobile alerts |
| FreeRTOS | Task scheduling on ESP32 |

---

## 📡 Communication Interfaces

| Interface | Used For |
|-----------|---------|
| SPI | STM32 ↔ RC522 RFID Reader |
| I2C | STM32 ↔ OLED Display + MPU6050 |
| UART | STM32 ↔ ESP32-CAM |
| GPIO/PWM | Keypad scanning + Servo control |
| Wi-Fi (HTTPS) | ESP32-CAM ↔ Telegram API |

---

## 📁 Project Structure

```
smart-locker-system/
│
├── STM32_Firmware/
│   ├── Core/Src/
│   │   ├── main.c              # Main authentication logic
│   │   └── rc522.c             # Custom RFID SPI driver
│   ├── Core/Inc/
│   │   └── rc522.h             # RFID driver header
│   └── STM32F103C8T6.ioc       # CubeMX configuration
│
├── ESP32CAM_Firmware/
│   ├── esp32cam_main.ino       # Face detection + Telegram alerts
│   └── camera_config.h        # Camera configuration
│
├── Schematics/
│   └── smart_locker_kicad/     # KiCad schematic files
│
└── README.md
```

---

## 🔧 Firmware Logic

### STM32 Authentication Flow

```
System Start → OLED: "SMART LOCKER READY"
       ↓
┌──────────────────────┐
│   Scan for RFID Card │ ──── Card Detected ────→ Unlock (90°) → Lock (0°)
└──────────────────────┘
       ↓ No card
┌──────────────────────┐
│   Keypad PIN Entry   │ ──── PIN Correct ──────→ Unlock (90°) → Lock (0°)
│   Press A to submit  │ ──── PIN Wrong ────────→ Buzzer + "DENIED"
└──────────────────────┘
       ↓ UART from ESP32
┌──────────────────────┐
│   Face Recognition   │ ──── Face OK ──────────→ Unlock (90°) → Lock (0°)
│   via ESP32-CAM      │ ──── Intruder ─────────→ Capture + Telegram Alert
└──────────────────────┘
```

### ESP32-CAM Flow

```
Continuous Camera Feed
       ↓
Face Detection (MTMN model)
       ↓
Face Recognized? ──── YES ──→ Send "ID:x\n" via UART to STM32
       ↓ NO
Intruder Detected? ─── YES ──→ Capture JPEG → Send Telegram Photo
```

---

## 📲 Telegram Alert Setup

1. Create a Telegram Bot via **@BotFather**
2. Get your **Bot Token** and **Chat ID**
3. Update in `esp32cam_main.ino`:
```cpp
const char* botToken = "YOUR_BOT_TOKEN";
const char* chatId = "YOUR_CHAT_ID";
```
4. Alerts are rate-limited to once every **15 seconds** to prevent spam

---

## 💰 Cost Breakdown

| Component | Cost (₹) |
|-----------|----------|
| STM32 Blue Pill | 229 |
| ESP32-CAM | 600 |
| RFID RC522 | 60 |
| Servo Motor | 85 |
| MPU6050 | 175 |
| OLED Display | 160 |
| Li-ion Battery | 360 |
| Other components | 882 |
| **Total** | **₹2,551** |

---

## 📈 Test Results

| Test Case | Result |
|-----------|--------|
| RFID Authentication | ✅ Successful |
| PIN Authentication | ✅ Successful |
| Face Recognition | ✅ Accurate |
| Unauthorized Attempt | ✅ Alert Sent |
| Image Capture | ✅ Successful |
| Tamper Detection | ✅ Working |

---

## 🔮 Future Scope

- Cloud storage for access logs
- Mobile application integration
- AI-based facial recognition upgrade
- Fingerprint authentication support
- Multiple user management

---

## 📚 Standards Used

- **IEEE 802.11** — Wi-Fi communication (ESP32-CAM)
- **ISO/IEC 14443** — RFID contactless card standard (RC522)
- **Telegram Bot API** — Real-time notification protocol

---

## 📄 License

This project is for academic purposes under KIIT University.
