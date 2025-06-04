#include <Wire.h>
#include "DHT.h"

#define RELAY_PIN 7
#define WATER_SENSOR A0
#define WATER_THRESHOLD 500
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

bool isPumpOn = false;
bool isManualMode = false;

void setup() {
    Wire.begin(0x09);
    Wire.onRequest(sendData);
    Wire.onReceive(receiveEvent);
    Serial.begin(9600);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);  // Khởi tạo relay tắt (LOW là tắt)
    dht.begin();
}

void loop() {
    int waterRaw = analogRead(WATER_SENSOR);
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    // Chỉ điều khiển tự động khi không ở chế độ thủ công
    if (!isManualMode) {
        if (waterRaw < WATER_THRESHOLD) {
            digitalWrite(RELAY_PIN, HIGH);  // Bật máy bơm khi nước thấp
            isPumpOn = true;
        } else {
            digitalWrite(RELAY_PIN, LOW);   // Tắt máy bơm khi nước cao
            isPumpOn = false;
        }
    }

    Serial.print("Nhiệt Độ: ");
    Serial.print(t);
    Serial.print(", Độ Ẩm: ");
    Serial.print(h);
    Serial.print(", Nước: ");
    Serial.println(waterRaw);
    Serial.print("Chế độ: ");
    Serial.println(isManualMode ? "Thủ công" : "Tự động");
    Serial.print("Trạng thái máy bơm: ");
    Serial.println(isPumpOn ? "Bật" : "Tắt");

    delay(1000);
}

void sendData() {
    int waterRaw = analogRead(WATER_SENSOR);
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    char buffer[32];
    if (isnan(h) || isnan(t)) {
        Serial.println("DHT11 error!");
        strcpy(buffer, "0.0,0.0,0,0");
    } else {
        dtostrf(t, 4, 1, buffer);
        strcat(buffer, ",");
        dtostrf(h, 4, 1, buffer + strlen(buffer));
        strcat(buffer, ",");
        itoa(waterRaw, buffer + strlen(buffer), 10);
        strcat(buffer, ",");
        itoa(isPumpOn ? 1 : 0, buffer + strlen(buffer), 10);
    }
    Wire.write(buffer, strlen(buffer));
}

void receiveEvent(int howMany) {
    char buffer[32];
    int i = 0;
    while (Wire.available() && i < 31) {
        buffer[i++] = Wire.read();
    }
    buffer[i] = '\0';

    if (strcmp(buffer, "PUMP_ON") == 0) {
        digitalWrite(RELAY_PIN, HIGH);  // Bật máy bơm (HIGH là bật)
        isPumpOn = true;
        isManualMode = true;
        Serial.println("Máy bơm bật - Chế độ thủ công");
    } else if (strcmp(buffer, "PUMP_OFF") == 0) {
        digitalWrite(RELAY_PIN, LOW);   // Tắt máy bơm (LOW là tắt)
        isPumpOn = false;
        isManualMode = true;
        Serial.println("Máy bơm tắt - Chế độ thủ công");
    } else if (strcmp(buffer, "AUTO") == 0) {
        isManualMode = false;
        Serial.println("Chuyển sang chế độ tự động");
    }
}