#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

// --- BẠN CHỈ CẦN SỬA 2 DÒNG NÀY ---
const char* ssid = "phuc";          // Điền tên WiFi không có pass
const char* mqtt_server = "192.168.98.89 2402:9d80:410:bf3f:887:a522:187d:8459";  // Ví dụ: "172.28.224.38"

// Cấu hình chân cảm biến DHT11
#define DHTPIN 4      // Nối chân DATA của DHT11 vào GPIO4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  // Lệnh kết nối WiFi không yêu cầu mật khẩu
  WiFi.begin(ssid); 
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" WiFi connected");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  dht.begin();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Đọc dữ liệu mỗi 5 giây
  static unsigned long lastMsg = 0;
  unsigned long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    // Kiểm tra xem cảm biến có bị lỏng dây không
    if (isnan(h) || isnan(t)) {
      Serial.println("Lỗi đọc cảm biến DHT! Kiem tra lai day cam.");
      return;
    }

    // Ép kiểu số thực sang chuỗi để gửi đi
    char tempString[8];
    dtostrf(t, 1, 2, tempString);
    char humString[8];
    dtostrf(h, 1, 2, humString);

    Serial.print("Nhiệt độ: "); Serial.print(tempString);
    Serial.print(" | Độ ẩm: "); Serial.println(humString);

    // Bắn dữ liệu lên trạm MQTT ở 2 kênh (topic)
    client.publish("esp32/nhietdo", tempString);
    client.publish("esp32/doam", humString);
  }
}
