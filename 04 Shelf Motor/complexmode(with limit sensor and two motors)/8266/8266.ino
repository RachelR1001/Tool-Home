#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
// #include <AccelStepper.h>
// #include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <ESPAsyncWebSrv.h>


const int stepPin = 14; // A4988 step 引脚连接到 Arduino 2号引脚
const int dirPin = 12;  // A4988 dir 引脚连接到 Arduino 3号引脚
const int crashSensorPin = 13; // 碰撞传感器连接到 Arduino 4号引脚
// const int STEPS_PER_REV = 2048;
AsyncWebServer server(5131);
const char* ssid = "SSID";
const char* password = "PASSWORD";

void setup() {
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(crashSensorPin, INPUT_PULLUP); // 启用内部上拉电阻
  Serial.begin(9600);
  Serial.println("begin");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
    Serial.println("IP address: " + WiFi.localIP().toString());
  }
  Serial.println(WiFi.localIP());
}
void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;
    http.begin(client, "http://IPAddress/Service_and_Code");
//Specify the URL
    int httpCode = http.GET();
    //
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("payload:");
      Serial.println(payload);
      delay(1000);
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);
      String serviceType = doc["serviceType"].as<String>();
      String objCode = doc["obj_code"].as<String>();
      delay(1000);
      if (serviceType == "Borrow" && objCode != "") {
        String firstLetter = objCode.substring(0, 1);
        String secondFigure = objCode.substring(1, 2);
        if(firstLetter == "A") {
  
            digitalWrite(dirPin, HIGH); // 设置电机方向
          Serial.println("move:"); 
            // 如果碰撞传感器未被触发，继续运行电机
            while (digitalRead(crashSensorPin) == HIGH) {
              // Serial.println("pin"); 
              digitalWrite(stepPin, HIGH);
              delayMicroseconds(2000); // 调整这个值以改变速度
              digitalWrite(stepPin, LOW);
              //  Serial.println("low"); 
              delayMicroseconds(2000);
            }
            Serial.println(digitalRead(crashSensorPin));
            delay(500);
            server.on("/moveMotor", HTTP_GET, [](AsyncWebServerRequest *request){
            
            request->send(200, "text/plain", "1");
            Serial.println("serverOn");
            });
          server.begin();

            // // 当碰撞传感器被触发时，停止电机运行
            // delay(1000); // 等待一秒
        }
      }
          if (serviceType == "Return") {
              Serial.println(serviceType);
                if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            WiFiClient client;
            http.begin(client, "http://IPAddress/lock_close");
        //Specify the URL
            int httpCode = http.GET();
            //
            if (httpCode > 0) {
              String payload = http.getString();
              
              if (payload == "1")
              
              delay(1000);
              digitalWrite(dirPin, HIGH); // 设置电机方向
            
              for(int step = 0; step < 200; step++){
                  // 检查碰撞传感器
                  if(digitalRead(crashSensorPin) == LOW){
                    // 如果碰撞传感器被触发，停止电机并退出循环
                    return;
                  }

                  // 移动一步
                  digitalWrite(stepPin, HIGH);
                  delayMicroseconds(2000); // 调整这个值以改变速度
                  digitalWrite(stepPin, LOW);
                  delayMicroseconds(2000);
                }
            }
            http.end();
          }
        }
    }
  }
}