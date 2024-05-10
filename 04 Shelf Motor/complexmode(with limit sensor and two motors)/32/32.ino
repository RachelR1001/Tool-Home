#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
// #include <AccelStepper.h>
// #include <WiFiClient.h>
// #include <ESPAsyncWebSrv.h>
#include <HTTPClient.h>


const int stepPin = 14; // A4988 step 引脚连接到 Arduino 14号引脚
const int dirPin = 12;  // A4988 dir 引脚连接到 Arduino 12号引脚
const int crashSensorPin = 18; // 碰撞传感器连接到 Arduino 18号引脚
// int isReady = 0;
WebServer server(1650);

const char* ssid = "YourSSID";
const char* password = "PASSWORD";
void setup() {
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(crashSensorPin, INPUT_PULLUP); // 启用内部上拉电阻
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("");
  Serial.print("Connected to WiFi: ");
  Serial.println(WiFi.localIP());
  server.on("/shelf_ready", HTTP_GET, handleRoot);

  server.begin();
}

void handleRoot() {
  // server.send(200, "text/plain", "1");
//  if(isReady == 1){
    server.send(200, "text/plain", "1");//Get的方法
    Serial.println("serveron");
  // 启动服务器
//  }else{
//   server.send(503, "text/plain", "");//Get的方法
//  }
  
  
}
void loop() {
  Serial.println("in loop");
    HTTPClient http;
    http.begin("http://IPAddress/moveMotor");
//Specify the URL
    int httpCode = http.GET();
    //
    if (httpCode > 0) {
      String payload = http.getString();
      // Serial.println("payload:");
      Serial.println(payload);
      String secondFigure = payload;
      delay(1000);
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);
      delay(1000);
      
        if(secondFigure == "1") {
          digitalWrite(dirPin, HIGH); // 设置电机方向
          Serial.println("move");

          // 如果碰撞传感器未被触发，继续运行电机
          while (digitalRead(crashSensorPin) == HIGH) {
            Serial.println("move2");
            digitalWrite(stepPin, HIGH);
            delayMicroseconds(2000); // 调整这个值以改变速度
            digitalWrite(stepPin, LOW);
            delayMicroseconds(2000);
          }
                // isReady = 1;
                server.handleClient();
        }
    }
}

