#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "fd_forward.h"
#include "fr_forward.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ==================== CONFIGURATION ====================
// Replace with your WiFi credentials
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Replace with your Telegram Bot Token and Chat ID
const char* botToken = "YOUR_BOT_TOKEN";
const char* chatId   = "YOUR_CHAT_ID";

// Camera pin config for ESP32-CAM (AI-Thinker module)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ==================== GLOBALS ====================
WiFiClientSecure client;

volatile bool intruder_detected = false;
volatile bool telegramRequest   = false;
uint8_t *telegramImage          = NULL;
size_t   telegramImageLen       = 0;
unsigned long lastTelegramTime  = 0;

// UART receive buffer (from STM32)
uint8_t uart_rx;
uint8_t rx_buffer[20];
uint8_t rx_index  = 0;

// Face recognition config
static mtmn_config_t mtmn_config = {0};

// ==================== CAMERA INIT ====================
void initCamera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;
    config.pin_d0       = Y2_GPIO_NUM;
    config.pin_d1       = Y3_GPIO_NUM;
    config.pin_d2       = Y4_GPIO_NUM;
    config.pin_d3       = Y5_GPIO_NUM;
    config.pin_d4       = Y6_GPIO_NUM;
    config.pin_d5       = Y7_GPIO_NUM;
    config.pin_d6       = Y8_GPIO_NUM;
    config.pin_d7       = Y9_GPIO_NUM;
    config.pin_xclk     = XCLK_GPIO_NUM;
    config.pin_pclk     = PCLK_GPIO_NUM;
    config.pin_vsync    = VSYNC_GPIO_NUM;
    config.pin_href     = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn     = PWDN_GPIO_NUM;
    config.pin_reset    = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size   = FRAMESIZE_QVGA;
    config.jpeg_quality = 10;
    config.fb_count     = 1;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed: 0x%x\n", err);
        return;
    }
    Serial.println("Camera initialized!");
}

// ==================== WIFI INIT ====================
void initWiFi() {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

// ==================== TELEGRAM PHOTO ====================
void sendTelegramPhoto(uint8_t *image, size_t len) {
    Serial.println("Sending photo to Telegram...");
    client.setInsecure();

    if (!client.connect("api.telegram.org", 443)) {
        Serial.println("Telegram connection failed!");
        return;
    }

    String head = "--ESP32CAM\r\n"
                  "Content-Disposition: form-data; name=\"chat_id\";\r\n\r\n" +
                  String(chatId) +
                  "\r\n--ESP32CAM\r\n"
                  "Content-Disposition: form-data; name=\"photo\"; filename=\"intruder.jpg\"\r\n"
                  "Content-Type: image/jpeg\r\n\r\n";

    String tail = "\r\n--ESP32CAM--\r\n";

    uint32_t totalLen = head.length() + len + tail.length();

    client.println("POST /bot" + String(botToken) + "/sendPhoto HTTP/1.1");
    client.println("Host: api.telegram.org");
    client.println("Content-Type: multipart/form-data; boundary=ESP32CAM");
    client.println("Content-Length: " + String(totalLen));
    client.println();
    client.print(head);
    client.write(image, len);
    client.print(tail);

    unsigned long timeout = millis();
    while (client.connected() && millis() - timeout < 5000) {
        while (client.available()) {
            String line = client.readStringUntil('\n');
            Serial.println(line);
            timeout = millis();
        }
    }
    client.stop();
    Serial.println("Telegram photo sent!");
}

// ==================== TELEGRAM TASK (FreeRTOS) ====================
void telegramTask(void *pvParameters) {
    while (true) {
        if (telegramRequest) {
            telegramRequest = false;
            Serial.println("Telegram task: sending...");
            sendTelegramPhoto(telegramImage, telegramImageLen);
            if (telegramImage) {
                free(telegramImage);
                telegramImage = NULL;
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

// ==================== FACE DETECTION LOOP ====================
void faceDetectionLoop() {
    mtmn_config.type       = FAST;
    mtmn_config.min_face   = 80;
    mtmn_config.pyramid    = 0.707;
    mtmn_config.pyramid_times = 4;

    while (true) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            continue;
        }

        // Convert to RGB for face detection
        dl_matrix3du_t *image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
        if (!image_matrix) {
            esp_camera_fb_return(fb);
            continue;
        }

        if (fmt2rgb888(fb->buf, fb->len, fb->format, image_matrix->item)) {
            box_array_t *net_boxes = face_detect(image_matrix, &mtmn_config);

            if (net_boxes) {
                Serial.println("Face detected!");

                // Send UART signal to STM32
                Serial.println("ID:1");   // UART1 → STM32

                // Capture intruder image if triggered
                if (intruder_detected && millis() - lastTelegramTime > 15000) {
                    if (!telegramRequest) {
                        uint8_t *jpg_buf = NULL;
                        size_t   jpg_len = 0;

                        if (fmt2jpg(image_matrix->item,
                                    fb->width * fb->height * 3,
                                    fb->width, fb->height,
                                    PIXFORMAT_RGB888, 90,
                                    &jpg_buf, &jpg_len)) {
                            telegramImage    = (uint8_t*)malloc(jpg_len);
                            memcpy(telegramImage, jpg_buf, jpg_len);
                            telegramImageLen = jpg_len;
                            telegramRequest  = true;
                        }
                    }
                    lastTelegramTime  = millis();
                    intruder_detected = false;
                }

                dl_lib_free(net_boxes->score);
                dl_lib_free(net_boxes->box);
                if (net_boxes->landmark) dl_lib_free(net_boxes->landmark);
                dl_lib_free(net_boxes);
            }
        }

        dl_matrix3du_free(image_matrix);
        esp_camera_fb_return(fb);

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// ==================== SETUP ====================
void setup() {
    Serial.begin(115200);  // UART to STM32
    delay(100);

    initCamera();
    initWiFi();

    // Start Telegram task on Core 0
    xTaskCreatePinnedToCore(
        telegramTask,
        "TelegramTask",
        8192,
        NULL,
        1,
        NULL,
        0
    );

    Serial.println("Smart Locker ESP32-CAM Ready!");
}

// ==================== LOOP ====================
void loop() {
    // Check UART from STM32 for intruder trigger
    if (Serial.available()) {
        char c = Serial.read();
        if (c == '\n') {
            rx_buffer[rx_index] = '\0';
            String msg = String((char*)rx_buffer);

            if (msg == "CAPTURE_INTRUDER") {
                intruder_detected = true;
                Serial.println("Intruder trigger received from STM32!");
            }
            rx_index = 0;
        } else {
            if (rx_index < 19) rx_buffer[rx_index++] = c;
        }
    }

    // Run face detection
    faceDetectionLoop();
}
