#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <IPAddress.h>
#include <PubSubClient.h>

// Khai báo cấu hình mạng wifi, bạn sửa theo thông tin wifi của bạn nhé
const char* ssid = "your_wifi_network";
const char* password = "your_wifi_password";

// Khai báo cấu hình giao thức MQTT và địa chỉ MQTT server
const char* clientId = "SmartPlug1";
// use public MQTT broker
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
const char* mqttUsername = "<MQTT_BROKER_USERNAME>";
const char* mqttPassword = "<MQTT_BROKER_PASSWORD>";

// Topic dùng để gửi trạng thái bật tắt của đèn về MQTT server
const char* relay1StatusTopic = "/SmartPlug1/Relay1/Status";
const char* relay2StatusTopic = "/SmartPlug1/Relay2/Status";

// Topic dùng để nhận lệnh bật tắt từ MQTT server
const char* relay1CommandTopic = "/SmartPlug1/Relay1/Command";
const char* relay2CommandTopic = "/SmartPlug1/Relay2/Command";

// Thời gian kết nối lại wifi và MQTT server nếu bị ngắt kết nối, ~ 3 phút
const unsigned long reconnectInterval = 120000;
// Lưu lại thời gian lần cuối kết nối lại
unsigned long lastReconnectTime = 0;

// Cấu hình các chân nối với 2 relay
const byte relay1Pin = 12;
const byte relay2Pin = 13;

// Lưu trạng thái bật tắt của 2 relay, false = tắt, true = bật
boolean relay1Status = false;
boolean relay2Status = false;

// Khởi tạo thư viện kết nối wifi và MQTT
WiFiClient espClient;
PubSubClient client(espClient);


void setup() {
  Serial.begin(115200);

  // Khởi tạo chế độ hoạt động của các chân
  pinMode(relay1Pin, OUTPUT);
  pinMode(relay2Pin, OUTPUT);
  digitalWrite(relay1Pin, HIGH);
  digitalWrite(relay2Pin, HIGH);
  relay1Status = true;
  relay2Status = true;

  connectWifiMQTTServer();
}

void loop() {
  if (!client.connected()) {
    // Nếu phát hiện bị ngắt kết nối, cần phải kết nối lại
    unsigned long current = millis();
    if (current - lastReconnectTime > reconnectInterval) {
      lastReconnectTime = millis();
      connectWifiMQTTServer();
    }
  } else {
    // kiểm tra xem có dữ liệu gửi đi từ server hay không
    client.loop();
  } 
  delay(50);
}

void connectWifiMQTTServer() {
  Serial.print("Dang thuc hien ket noi Wifi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Ket noi Wifi thanh cong. Dia chi IP la: ");
  Serial.println(WiFi.localIP());

  // Kết nối tới MQTT server cho đến khi thành công
  while (!client.connected()) {
    Serial.println("Dang ket noi voi MQTT server...");
    if (client.connect(clientId, mqttUsername, mqttPassword)) {
      Serial.println("Da ket noi thanh cong toi MQTT server");
      delay(100);
    } else {
      Serial.print("Ket noi that bai, error code =");
      Serial.println(client.state());
      delay(1000);
    }
  }
}

void onMQTTMessageReceived(char* topic, byte* payload, unsigned int length) {
  Serial.print("Nhan duoc du lieu tu MQTT server [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Đoạn code dưới đây kiểm tra xem lệnh nhận được là làm việc gì
  // và thực hiện bật tắt relay tương ứng
  if (strcmp(topic, relay1CommandTopic) == 0) {
    if ((char)payload[0] == '1') {
      Serial.println("Nhan duoc lenh bat relay 1");
      digitalWrite(relay1Pin, HIGH);
      relay1Status = true;
    } else if ((char)payload[0] == '0') {
      Serial.println("Nhan duoc lenh tat relay 1");
      digitalWrite(relay1Pin, LOW);
      relay1Status = false;
    }
  } else if (strcmp(topic, relay2CommandTopic) == 0) {
    if ((char)payload[0] == '1') {
      Serial.println("Nhan duoc lenh bat relay 2");
      digitalWrite(relay2Pin, HIGH);
      relay2Status = true;
    } else if ((char)payload[0] == '0') {
      Serial.println("Nhan duoc lenh tat relay 2");
      digitalWrite(relay2Pin, LOW);
      relay2Status = false;
    }
  } else {
    Serial.println("Lenh nhan duoc khong hop le: ");
  }
}
