#include <Wire.h>
#include <Stepper.h>
#include <DS1302.h>

#define UNO1_ADDR 0x08
#define RST_PIN  5
#define DAT_PIN  7
#define CLK_PIN  6
#define cambien A2
#define den 2  

const int stepsPerRevolution = 2048;
Stepper myStepper(stepsPerRevolution, 8, 10, 9, 11);
DS1302 rtc(RST_PIN, DAT_PIN, CLK_PIN);

struct Alarm {
    int hour;
    int minute;
};

Alarm alarms[3] = {
    {8, 56}, 
    {12, 45}, 
    {23, 19}
};

bool dongCoDangQuay = false;  
String output = "";
String command = "";  
bool manualOverride = false;  
unsigned long lastCommandTime = 0;  
const unsigned long COMMAND_TIMEOUT = 10000;  
bool feeding = false;  

void setup() {
    Serial.begin(9600);
    Wire.begin(UNO1_ADDR);
    Wire.onRequest(sendDataToESP32);
    Wire.onReceive(receiveCommand);  
    rtc.halt(false);
    rtc.writeProtect(false);
    myStepper.setSpeed(15);  // Giữ nguyên 15 như code cũ
    pinMode(den, OUTPUT);
    digitalWrite(den, LOW);  
    pinMode(cambien, INPUT);
    
    Serial.println("🟢 Hệ thống sẵn sàng!");
}

void loop() {
    Time now = rtc.getTime();
    output = "Giờ hiện tại: " + String(now.hour) + ":" + String(now.min) + ":" + String(now.sec);

    // Xử lý chế độ tự động theo lịch
    for (int i = 0; i < 3; i++) {
        if (now.hour == alarms[i].hour && now.min == alarms[i].minute && now.sec == 0) {
            output += " 🔔 Đang quay";
            dongCoDangQuay = true;
            quayDongCo();
            dongCoDangQuay = false;
            output += " ✅ Hoàn thành phiên quay";
        }
    }

    executeCommand();  
    handleFeeding();  // Xử lý cho ăn với tốc độ nhanh
    capNhatCamBien();  
    Serial.println(output);
    delay(500);  // Giữ delay như code cũ
}

void quayDongCo() {
    unsigned long startTime = millis();
    while (millis() - startTime < 60000) {
        myStepper.step(-19);  // Quay liên tục như code cũ
        capNhatCamBien();    // Giữ logic cũ
    }
}

void startFeeding() {
    feeding = true;
    dongCoDangQuay = true;
    output += " 🍽 Bắt đầu cho ăn thủ công";
}

void stopFeeding() {
    feeding = false;
    dongCoDangQuay = false;
    output += " ⏹ Ngừng cho ăn thủ công";
}

void handleFeeding() {
    if (feeding) {
        // Quay liên tục trong 1 giây mỗi lần gọi để giữ tốc độ cao
        unsigned long startTime = millis();
        while (millis() - startTime < 1000) {  // Quay trong 1 giây rồi kiểm tra lại lệnh
            myStepper.step(-19);
            capNhatCamBien();  // Giữ logic cũ
        }
        output += " | Động cơ quay (Cho ăn)";
    }
}

void capNhatCamBien() {
    int giatri = analogRead(cambien);

    // Nếu đã quá thời gian timeout, quay lại chế độ tự động
    if (millis() - lastCommandTime > COMMAND_TIMEOUT && manualOverride) {
        manualOverride = false;  
        command = "";  
        Serial.println("🔄 Hết thời gian thủ công, quay lại chế độ tự động");
    }

    // Nếu đang ở chế độ thủ công, cảm biến LDR không kiểm soát đèn
    if (manualOverride) {
        output += " 🔦 Cảm biến: " + String(giatri) + " | LED: " + (digitalRead(den) ? "ON (Thủ công)" : "OFF (Thủ công)");
    } else {
        // Chế độ tự động, đèn bật/tắt theo giá trị cảm biến
        digitalWrite(den, giatri > 500 ? HIGH : LOW);
        output += " 🔦 Cảm biến: " + String(giatri) + " | LED: " + (digitalRead(den) ? "ON (Tự động)" : "OFF (Tự động)");
    }
}


void sendDataToESP32() {
    Time now = rtc.getTime();
    int giatri = analogRead(cambien);
    bool ledStatus = digitalRead(den);  

    char buffer[32];  
    snprintf(buffer, sizeof(buffer), 
             "%02d:%02d:%02d|LDR:%d|LED:%d|M:%d", 
             now.hour, now.min, now.sec, 
             giatri, ledStatus, dongCoDangQuay);

    Serial.print("📤 Dữ liệu gửi ESP32: ");
    Serial.println(buffer);

    Wire.write(buffer);
    delay(10);
}

void receiveCommand(int byteCount) {
    command = "";
    while (Wire.available()) {
        char c = Wire.read();
        command += c;
    }
    lastCommandTime = millis();  
    Serial.print("📡 Nhận lệnh từ ESP32: ");
    Serial.println(command);
}

void executeCommand() {
    if (command == "LED_ON") {
        manualOverride = true;
        digitalWrite(den, HIGH);
        output += " | Đèn bật thủ công";
    } else if (command == "LED_OFF") {
        manualOverride = true;
        digitalWrite(den, LOW);
        output += " | Đèn tắt thủ công";
    } else if (command == "FEED_START") {
        manualOverride = true;
        startFeeding();
    } else if (command == "FEED_STOP") {
        manualOverride = true;
        stopFeeding();
    }
}