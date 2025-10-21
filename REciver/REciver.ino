#include <WiFi.h>
#include <esp_now.h>
#include <WebServer.h>
#include <map>
#include <vector>

const char* AP_SSID = "ESP32-AP";
const char* AP_PASS = "esp32pass";

// sender MAC
uint8_t senderMac[6] = {0x8C, 0x4F, 0x00, 0xD0, 0x4F, 0x20};

struct ChunkHeader {
  uint32_t frame_id;
  uint16_t total_packets;
  uint16_t packet_index;
  uint16_t payload_len;
} __attribute__((packed));

struct IncomingFrame {
  uint32_t frame_id;
  uint16_t total_packets;
  uint16_t received_packets;
  uint32_t last_activity;
  std::vector<std::vector<uint8_t>> parts;
};

std::map<uint32_t, IncomingFrame> frames;
std::vector<uint8_t> latestImage;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
WebServer server(80);

void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len < (int)sizeof(ChunkHeader)) return;
  const ChunkHeader* h = (const ChunkHeader*)data;
  uint32_t fid = h->frame_id;

  portENTER_CRITICAL(&mux);
  auto &frame = frames[fid];
  frame.frame_id = fid;
  frame.total_packets = h->total_packets;
  frame.last_activity = millis();

  if (frame.parts.empty()) frame.parts.resize(h->total_packets);

  if (frame.parts[h->packet_index].empty()) {
    frame.parts[h->packet_index].assign(data + sizeof(ChunkHeader), data + sizeof(ChunkHeader) + h->payload_len);
    frame.received_packets++;
  }

  if (frame.received_packets == frame.total_packets) {
    latestImage.clear();
    for (auto &part : frame.parts) {
      latestImage.insert(latestImage.end(), part.begin(), part.end());
    }
    frames.erase(fid);
  }
  portEXIT_CRITICAL(&mux);
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32-CAM Stream</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
      padding: 20px;
    }
    .container {
      background: rgba(255, 255, 255, 0.95);
      border-radius: 20px;
      box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
      padding: 30px;
      max-width: 900px;
      width: 100%;
    }
    h1 {
      color: #333;
      text-align: center;
      margin-bottom: 10px;
      font-size: 2em;
      font-weight: 600;
    }
    .subtitle {
      text-align: center;
      color: #666;
      margin-bottom: 25px;
      font-size: 0.9em;
    }
    .status {
      display: flex;
      align-items: center;
      justify-content: center;
      margin-bottom: 20px;
      gap: 8px;
    }
    .status-indicator {
      width: 12px;
      height: 12px;
      background: #4CAF50;
      border-radius: 50%;
      animation: pulse 2s infinite;
    }
    @keyframes pulse {
      0%, 100% { opacity: 1; }
      50% { opacity: 0.5; }
    }
    .status-text {
      color: #4CAF50;
      font-weight: 500;
    }
    .image-container {
      position: relative;
      background: #000;
      border-radius: 15px;
      overflow: hidden;
      box-shadow: 0 10px 30px rgba(0, 0, 0, 0.2);
      aspect-ratio: 4/3;
    }
    .image-container img {
      width: 100%;
      height: 100%;
      object-fit: contain;
      display: block;
    }
    .loading {
      position: absolute;
      top: 50%;
      left: 50%;
      transform: translate(-50%, -50%);
      color: #fff;
      font-size: 1.2em;
    }
    .controls {
      display: flex;
      gap: 15px;
      margin-top: 20px;
      justify-content: center;
      flex-wrap: wrap;
    }
    .btn {
      padding: 12px 24px;
      border: none;
      border-radius: 10px;
      font-size: 1em;
      font-weight: 500;
      cursor: pointer;
      transition: all 0.3s ease;
      box-shadow: 0 4px 15px rgba(0, 0, 0, 0.1);
    }
    .btn-primary {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
    }
    .btn-primary:hover {
      transform: translateY(-2px);
      box-shadow: 0 6px 20px rgba(102, 126, 234, 0.4);
    }
    .btn-secondary {
      background: #fff;
      color: #667eea;
      border: 2px solid #667eea;
    }
    .btn-secondary:hover {
      background: #667eea;
      color: white;
    }
    .stats {
      display: flex;
      justify-content: space-around;
      margin-top: 20px;
      padding: 15px;
      background: #f8f9fa;
      border-radius: 10px;
    }
    .stat-item {
      text-align: center;
    }
    .stat-value {
      font-size: 1.5em;
      font-weight: 600;
      color: #667eea;
    }
    .stat-label {
      color: #888;
      margin-top: 5px;
      font-size: 0.85em;
    }
    .info-box {
      background: #f8f9fa;
      border-radius: 10px;
      padding: 15px;
      margin-top: 15px;
      font-size: 0.9em;
      color: #555;
      text-align: center;
    }
    @media (max-width: 600px) {
      .container { padding: 20px; }
      h1 { font-size: 1.5em; }
      .btn { padding: 10px 20px; font-size: 0.9em; }
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>ESP32-CAM Live Stream</h1>
    <p class="subtitle">ESP-NOW Wireless Camera Feed</p>
    
    <div class="status">
      <div class="status-indicator"></div>
      <span class="status-text">Live</span>
    </div>
    
    <div class="image-container">
      <img id="stream" src="/image.jpg" alt="Camera Stream">
      <div class="loading" id="loading">Waiting for image...</div>
    </div>
    
    <div class="controls">
      <button class="btn btn-primary" onclick="toggleRefresh()">
        <span id="refreshBtn"> Pause</span>
      </button>
      <button class="btn btn-secondary" onclick="downloadImage()">
         Download
      </button>
      <button class="btn btn-secondary" onclick="refreshNow()">
         Refresh Now
      </button>
    </div>

    <div class="stats">
      <div class="stat-item">
        <div class="stat-value" id="fps">0.0</div>
        <div class="stat-label">FPS</div>
      </div>
      <div class="stat-item">
        <div class="stat-value" id="frames">0</div>
        <div class="stat-label">Frames</div>
      </div>
      <div class="stat-item">
        <div class="stat-value" id="latency">0</div>
        <div class="stat-label">Latency (ms)</div>
      </div>
    </div>
    
    <div class="info-box">
      <strong>Maximum Speed Mode</strong><br>
      ESP32 IP: <strong>192.168.4.1</strong> • SSID: <strong>ESP32-AP</strong>
    </div>
  </div>

  <script>
    let autoRefresh = true;
    let frameCount = 0;
    let lastFrameTime = Date.now();
    let fpsHistory = [];
    const img = document.getElementById('stream');
    const loading = document.getElementById('loading');

    img.onload = () => {
      loading.style.display = 'none';
      frameCount++;
      
      const now = Date.now();
      const latency = now - lastFrameTime;
      const fps = latency > 0 ? 1000 / latency : 0;
      
      fpsHistory.push(fps);
      if (fpsHistory.length > 10) fpsHistory.shift();
      
      const avgFps = fpsHistory.reduce((a, b) => a + b, 0) / fpsHistory.length;
      
      document.getElementById('fps').textContent = avgFps.toFixed(1);
      document.getElementById('frames').textContent = frameCount;
      document.getElementById('latency').textContent = latency.toFixed(0);
      
      lastFrameTime = now;
      
      if (autoRefresh) {
        refreshImage();
      }
    };

    img.onerror = () => {
      loading.style.display = 'block';
      loading.textContent = 'No image available';
      setTimeout(() => {
        if (autoRefresh) refreshImage();
      }, 100);
    };

    function refreshImage() {
      img.src = '/image.jpg?' + Date.now();
    }

    function toggleRefresh() {
      autoRefresh = !autoRefresh;
      const btn = document.getElementById('refreshBtn');
      if (autoRefresh) {
        btn.textContent = '⏸ Pause';
        refreshImage();
      } else {
        btn.textContent = '▶ Play';
      }
    }

    function refreshNow() {
      img.src = '/image.jpg?' + Date.now();
    }

    function downloadImage() {
      const link = document.createElement('a');
      link.href = '/image.jpg?' + Date.now();
      link.download = 'esp32cam_' + Date.now() + '.jpg';
      link.click();
    }

    refreshImage();
  </script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

void handleImage() {
  if (latestImage.empty()) {
    server.send(404, "text/plain", "No image received");
    return;
  }
  server.sendHeader("Content-Type", "image/jpeg");
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  WiFiClient client = server.client();
  client.write(latestImage.data(), latestImage.size());
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.println("AP IP: " + WiFi.softAPIP().toString());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    while (true);
  }

  esp_now_register_recv_cb(onReceive);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, senderMac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  server.on("/", handleRoot);
  server.on("/image.jpg", handleImage);
  server.begin();
  Serial.println("HTTP server started at http://192.168.4.1");
}

void loop() {
  server.handleClient();
}