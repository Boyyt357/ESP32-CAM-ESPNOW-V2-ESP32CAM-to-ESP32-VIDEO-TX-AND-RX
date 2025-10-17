#include <esp_now.h>
#include <WiFi.h>
#include "esp_camera.h"

// Receiver MAC
uint8_t receiverMac[6] = {0x00, 0x4B, 0x12, 0x4A, 0x81, 0xF9};

// Fragmentation settings
const size_t CHUNK_PAYLOAD = 1400;

struct ChunkHeader {
  uint32_t frame_id;
  uint16_t total_packets;
  uint16_t packet_index;
  uint16_t payload_len;
} __attribute__((packed));

// ✅ New-style callback (for ESP-IDF 5.x / Arduino-ESP32 v3.x)
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.printf("Send status: %s\n", status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

bool setupCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 25;
  config.fb_count = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed");
    return false;
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (!setupCamera()) {
    Serial.println("Camera setup failed!");
    while (true) delay(1000);
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    while (true) delay(1000);
  }

  esp_now_register_send_cb(OnDataSent);

  // ✅ new name for function
  if (!esp_now_is_peer_exist(receiverMac)) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiverMac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
  }

  Serial.println("ESP32-CAM sender ready");
}

uint32_t nextFrameId() {
  static uint32_t id = 0;
  return ++id;
}

void loop() {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Frame capture failed");
    delay(1000);
    return;
  }

  size_t img_len = fb->len;
  uint8_t * img_buf = fb->buf;
  uint16_t total_packets = (img_len + CHUNK_PAYLOAD - 1) / CHUNK_PAYLOAD;
  uint32_t frameId = nextFrameId();

  for (uint16_t i = 0; i < total_packets; i++) {
    size_t offset = i * CHUNK_PAYLOAD;
    uint16_t len = (img_len - offset > CHUNK_PAYLOAD) ? CHUNK_PAYLOAD : img_len - offset;

    uint8_t packet[sizeof(ChunkHeader) + len];
    ChunkHeader h = {frameId, total_packets, i, len};
    memcpy(packet, &h, sizeof(h));
    memcpy(packet + sizeof(h), img_buf + offset, len);

    esp_now_send(receiverMac, packet, sizeof(h) + len);
    delay(5);
  }

  esp_camera_fb_return(fb);
  delay(20);
}