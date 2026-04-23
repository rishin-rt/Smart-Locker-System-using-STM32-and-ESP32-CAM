# Setup Guide — Smart Locker System

## Step 1 — Hardware Connections

### STM32 Blue Pill Pin Connections

| Component | STM32 Pin | Notes |
|-----------|-----------|-------|
| RC522 RFID SDA/CS | PA4 | SPI Chip Select |
| RC522 RFID SCK | PA5 | SPI Clock |
| RC522 RFID MOSI | PA7 | SPI MOSI |
| RC522 RFID MISO | PA6 | SPI MISO |
| RC522 RFID RST | PA2 | Reset pin |
| OLED SDA | PB7 | I2C Data |
| OLED SCL | PB6 | I2C Clock |
| Servo Motor | PA1 | PWM TIM2 CH2 |
| Buzzer | PB0 | GPIO Output |
| ESP32-CAM TX | PA10 | UART1 RX |
| ESP32-CAM RX | PA9 | UART1 TX |
| Keypad Row 1 | PA8 | GPIO Output |
| Keypad Row 2 | PB15 | GPIO Output |
| Keypad Row 3 | PB14 | GPIO Output |
| Keypad Row 4 | PB13 | GPIO Output |
| Keypad Col 1 | PB12 | GPIO Input |
| Keypad Col 2 | PB11 | GPIO Input |
| Keypad Col 3 | PB10 | GPIO Input |
| Keypad Col 4 | PB1 | GPIO Input |
| MPU6050 SDA | PB7 | Shared I2C |
| MPU6050 SCL | PB6 | Shared I2C |

---

## Step 2 — Configure WiFi and Telegram

Open `esp32cam_main.ino` and update:

```cpp
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* botToken = "YOUR_TELEGRAM_BOT_TOKEN";
const char* chatId   = "YOUR_TELEGRAM_CHAT_ID";
```

### How to get Telegram Bot Token:
1. Open Telegram, search **@BotFather**
2. Send `/newbot`
3. Follow instructions, copy the token

### How to get Chat ID:
1. Search **@userinfobot** on Telegram
2. Send any message, it replies with your Chat ID

---

## Step 3 — Flash STM32 Firmware

1. Open **STM32CubeIDE**
2. Import project from `STM32_Firmware/` folder
3. Connect ST-Link V2 to STM32 Blue Pill
4. Build and Flash (Run → Debug or Run)

---

## Step 4 — Flash ESP32-CAM

1. Open **Arduino IDE**
2. Open `esp32cam_main.ino`
3. Select Board: **AI Thinker ESP32-CAM**
4. Connect ESP32 Programming Module
5. Press Upload

---

## Step 5 — Power Up

1. Connect Li-ion battery through TP4056 and Buck Converter
2. STM32 OLED should display **"SMART LOCKER READY"**
3. ESP32-CAM connects to WiFi automatically

---

## Step 6 — Test Authentication

| Method | How to test |
|--------|------------|
| PIN | Press keys on keypad, press A to submit (default: 1234) |
| RFID | Tap RFID card on RC522 reader |
| Face | Stand in front of ESP32-CAM |

---

## Changing the PIN

In `main.c`, find and update:
```c
char password[5] = "1234";  // Change to your PIN
```

---

## Troubleshooting

| Problem | Solution |
|---------|---------|
| OLED not displaying | Check I2C address (0x3C or 0x3D) |
| RFID not reading | Check SPI connections and RC522 power (3.3V) |
| Servo not moving | Check PWM timer configuration |
| Telegram not sending | Check WiFi credentials and Bot Token |
| Face not detected | Ensure good lighting conditions |
