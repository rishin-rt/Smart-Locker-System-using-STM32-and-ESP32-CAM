# Smart Locker System — Dependencies

## STM32 (STM32CubeIDE)

### HAL Libraries (included in STM32CubeMX generated code)
- STM32F1xx HAL Driver
- HAL_SPI   — RC522 RFID communication
- HAL_I2C   — OLED display (SSD1306)
- HAL_UART  — ESP32-CAM communication
- HAL_TIM   — PWM servo motor control
- HAL_GPIO  — Keypad scanning, buzzer, LEDs

### External Libraries
- SSD1306 OLED Driver (I2C)
  Source: https://github.com/afiskon/stm32-ssd1306

## ESP32-CAM (Arduino IDE)

### Board Package
- ESP32 by Espressif Systems v2.0+
  Install via: Arduino IDE → Boards Manager → Search "esp32"

### Built-in Libraries (included with ESP32 board package)
- esp_camera.h       — Camera control
- WiFi.h             — WiFi connectivity
- WiFiClientSecure.h — HTTPS for Telegram API
- fd_forward.h       — Face detection
- fr_forward.h       — Face recognition
- freertos/FreeRTOS.h — Task scheduling

### Arduino IDE Settings for ESP32-CAM
- Board: AI Thinker ESP32-CAM
- Upload Speed: 115200
- Flash Frequency: 80MHz
- Flash Mode: QIO
- Partition Scheme: Huge APP (3MB No OTA)

## Software Tools

| Tool | Version | Purpose |
|------|---------|---------|
| STM32CubeIDE | 1.12+ | STM32 firmware development |
| STM32CubeMX | 6.8+ | Peripheral configuration |
| Arduino IDE | 2.0+ | ESP32-CAM programming |
| KiCad | 7.0+ | Circuit schematic design |
| ST-Link V2 | — | STM32 programming/debugging |
| ESP32 Programming Module | — | ESP32-CAM flashing |
