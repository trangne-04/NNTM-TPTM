#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define UNO1_ADDR 0x08  // Uno 1
#define UNO2_ADDR 0x09  // Uno 2
#define UNO3_ADDR 0x0A  // Uno 3 (MQ-135)

const char* ssid = "NGUYEN TRANG T1";
const char* password = "nguyentrang";
const char* server_url = "http://192.168.1.6:5000/update";
const char* control_url = "http://192.168.1.6:5000/control";

String cleanString(const char* input);

void setup() {
    Serial.begin(115200);
    Wire.begin(); // ESP32 làm master I2C
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n🟢 Đã kết nối WiFi!");
}

void loop() {
    char buffer1[32] = {0};
    char buffer2[32] = {0};
    char buffer3[32] = {0};

    Wire.requestFrom(UNO1_ADDR, 31);
    int i = 0;
    while (Wire.available() && i < 31) {
        buffer1[i++] = Wire.read();
    }
    buffer1[i] = '\0';
    String uno1_clean = cleanString(buffer1);

    Wire.requestFrom(UNO2_ADDR, 31);
    i = 0;
    while (Wire.available() && i < 31) {
        buffer2[i++] = Wire.read();
    }
    buffer2[i] = '\0';
    String uno2_clean = cleanString(buffer2);

    Wire.requestFrom(UNO3_ADDR, 31);
    i = 0;
    while (Wire.available() && i < 31) {
        buffer3[i++] = Wire.read();
    }
    buffer3[i] = '\0';
    String uno3_clean = cleanString(buffer3);

    Serial.print("📥 UNO 1 (Clean): ");
    Serial.println(uno1_clean);
    Serial.print("📥 UNO 2 (Clean): ");
    Serial.println(uno2_clean);
    Serial.print("📥 UNO 3 (MQ-135, Clean): ");
    Serial.println(uno3_clean);

    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(server_url);
        http.addHeader("Content-Type", "application/json");

        String jsonPayload = "{";
        jsonPayload += "\"uno1\":\"" + uno1_clean + "\",";
        jsonPayload += "\"uno2\":\"" + uno2_clean + "\",";
        jsonPayload += "\"uno3\":\"" + uno3_clean + "\"";
        jsonPayload += "}";

        Serial.print("📤 JSON gửi đi: ");
        Serial.println(jsonPayload);

        int httpResponseCode = http.POST(jsonPayload);
        Serial.print("📤 HTTP Response Code: ");
        Serial.println(httpResponseCode);
        http.end();

        checkControlCommand();
    }

    delay(1000);
}

void checkControlCommand() {
    HTTPClient http;
    http.begin(control_url);
    int httpCode = http.GET();
    if (httpCode == 200) {
        String payload = http.getString();
        Serial.print("📡 Nhận lệnh từ Flask: ");
        Serial.println(payload);

        if (payload == "LED_ON") {
            Wire.beginTransmission(UNO1_ADDR);
            Wire.write((const uint8_t*)"LED_ON", strlen("LED_ON"));
            Wire.endTransmission();
            Serial.println("📤 Gửi LED_ON đến Uno 1");
        } else if (payload == "LED_OFF") {
            Wire.beginTransmission(UNO1_ADDR);
            Wire.write((const uint8_t*)"LED_OFF", strlen("LED_OFF"));
            Wire.endTransmission();
            Serial.println("📤 Gửi LED_OFF đến Uno 1");
        } else if (payload == "FEED_START") {
            Wire.beginTransmission(UNO1_ADDR);
            Wire.write((const uint8_t*)"FEED_START", strlen("FEED_START"));
            Wire.endTransmission();
            Serial.println("📤 Gửi FEED_START đến Uno 1");
        } else if (payload == "FEED_STOP") {
            Wire.beginTransmission(UNO1_ADDR);
            Wire.write((const uint8_t*)"FEED_STOP", strlen("FEED_STOP"));
            Wire.endTransmission();
            Serial.println("📤 Gửi FEED_STOP đến Uno 1");
        } else if (payload == "PUMP_ON") {
            Wire.beginTransmission(UNO2_ADDR);
            Wire.write((const uint8_t*)"PUMP_ON", strlen("PUMP_ON"));
            Wire.endTransmission();
            Serial.println("📤 Gửi PUMP_ON đến Uno 2");
        } else if (payload == "PUMP_OFF") {
            Wire.beginTransmission(UNO2_ADDR);
            Wire.write((const uint8_t*)"PUMP_OFF", strlen("PUMP_OFF"));
            Wire.endTransmission();
            Serial.println("📤 Gửi PUMP_OFF đến Uno 2");
        } else if (payload == "AUTO") {
            Wire.beginTransmission(UNO2_ADDR);
            Wire.write((const uint8_t*)"AUTO", strlen("AUTO"));
            Wire.endTransmission();
            Serial.println("📤 Gửi AUTO đến Uno 2");
        }
    }
    http.end();
}

String cleanString(const char* input) {
    String output = "";
    for (int i = 0; input[i] != '\0'; i++) {
        char c = input[i];
        if (c >= 32 && c <= 126) {
            output += c;
        }
    }
    output.trim();
    return output;
}