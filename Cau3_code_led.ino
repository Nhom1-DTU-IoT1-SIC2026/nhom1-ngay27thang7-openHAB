#include <WiFi.h>
#include <PubSubClient.h>

// ================= THÔNG TIN CẤU HÌNH =================
const char* ssid        = "phuc";       // Tên WiFi
const char* mqtt_server = "192.168.98.89"; // IP Raspberry Pi (OpenHAB/Mosquitto)
const int   mqtt_port   = 1883;                   // Cổng MQTT

// Cấu hình chân phần cứng
const int LED_PIN    = 2; // Chân kết nối LED
const int BUTTON_PIN = 4; // Chân kết nối Nút nhấn 4 chân

// MQTT Topics
const char* TOPIC_LED_SET   = "home/led/set";   // Lắng nghe lệnh từ OpenHAB
const char* TOPIC_LED_STATE = "home/led/state"; // Gửi trạng thái hiện tại về OpenHAB

// Biến quản lý trạng thái LED & Nút nhấn
bool ledState = false;             // Trạng thái LED (false = OFF, true = ON)
int lastButtonState = HIGH;        // Trạng thái nút bấm trước đó
unsigned long lastDebounceTime = 0; // Thời gian chống rung nút
const unsigned long debounceDelay = 50; // Khoảng thời gian chống rung (ms)

WiFiClient espClient;
PubSubClient client(espClient);

// Hàm cập nhật trạng thái LED và đồng bộ lên OpenHAB
void setLedState(bool newState) {
  ledState = newState;
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  
  // Gửi trạng thái mới lên MQTT để OpenHAB cập nhật giao diện
  if (client.connected()) {
    client.publish(TOPIC_LED_STATE, ledState ? "ON" : "OFF");
  }
  Serial.print("Da doi trang thai LED -> ");
  Serial.println(ledState ? "ON" : "OFF");
}

// Hàm nhận lệnh từ OpenHAB gửi xuống
void callback(char* topic, byte* payload, unsigned int length) {
  String messageBuffer;
  for (int i = 0; i < length; i++) {
    messageBuffer += (char)payload[i];
  }
  
  Serial.print("Nhan lenh tu OpenHAB [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(messageBuffer);

  if (messageBuffer == "ON" && !ledState) {
    setLedState(true);
  } else if (messageBuffer == "OFF" && ledState) {
    setLedState(false);
  }
}

// Kết nối lại MQTT khi mất mạng
void reconnect() {
  while (!client.connected()) {
    Serial.print("Dang ket noi MQTT...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("Da ket noi!");
      client.subscribe(TOPIC_LED_SET);
    } else {
      Serial.print("Loi ket noi, rc=");
      Serial.print(client.state());
      Serial.println(" Thu lai sau 5 giay...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Dùng điện trở kéo lên nội bộ
  
  digitalWrite(LED_PIN, LOW); // Mặc định ban đầu tắt LED

  // Kết nối WiFi
  WiFi.begin(ssid);
  Serial.print("Dang ket noi WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi da ket noi!");

  // Cấu hình MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  // 1. Duy trì kết nối MQTT
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // 2. Xử lý đọc nút nhấn (có chống rung / debounce)
  int reading = digitalRead(BUTTON_PIN);

  // Nếu trạng thái nút bị thay đổi (do nhiễu hoặc do bấm)
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Nếu nút thực sự bị nhấn xuống (mức LOW do dùng INPUT_PULLUP)
    static int buttonState = HIGH;
    if (reading != buttonState) {
      buttonState = reading;
      
      // Khi nhấn xuống (LOW), đảo trạng thái LED
      if (buttonState == LOW) {
        setLedState(!ledState);
      }
    }
  }

  lastButtonState = reading;
}
