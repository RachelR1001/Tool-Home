
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h> // 包含ArduinoJson库
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <ESPAsyncWebSrv.h>
#include <AsyncTCP.h>

#define SS 18
#define RST 19
#define BUZZER 10
#define RELAY 1

const int magsensor = 2;  // KY-024连接的模拟输入引脚
const char* ssid = "YourSSID";
const char* password = "YourPassword";

int shelfStatus = 0;
String lock_check = "";
String lock_close = "1";
String serviceType = "";
int shelfFlag = 0;

AsyncWebServer server(8013);//HTTP 服务器端口定义
const char* get_shelfurl = "http://IPAddress/shelf_ready"; // GET请求的URL

MFRC522 rfid(SS, RST);
byte managerKeyUID[4] = { 0x3A, 0xC9, 0x6A, 0xCB };

int initialCode = 0;

int magValue = analogRead(magsensor); //读取磁力传感器的状态
const int lockClosedValue = 3000; // 电磁锁关闭时传感器的读数

//基本设置
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  SPI.begin();  //RFID reader 引脚输出设置 
  pinMode(RELAY, OUTPUT);
  pinMode(magsensor, INPUT);
  digitalWrite(RELAY, HIGH); 
  pinMode(BUZZER, OUTPUT);
  noTone(BUZZER);
  Serial.println("Put your card to the reader");
  Serial.println();

  //连接到 WIFI
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi: ");
  Serial.println(WiFi.localIP());

  //启动部署的服务，让其他请求的板子能收到验证信号
  server.on("/lock_check", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(200, "text/plain", lock_check);
  });

  server.on("/lock_close", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(200, "text/plain", lock_close);
  });

  server.begin();
}

//定义GET 请求
void sendGetRequest() {
  HTTPClient http;
  http.begin(get_shelfurl); // 指定GET请求的URL
  int httpCode = http.GET(); // 发送GET请求

  if (httpCode > 0) { //一般请求成功的状态码是 200
    String payload1 = http.getString(); // 获取响应内容
    shelfStatus = payload1.toInt();
    Serial.print("payload1:");
    Serial.println(payload1);
    Serial.print("shelfStatus:");
    Serial.println(shelfStatus);
    Serial.print("httpCode:");
    Serial.println(httpCode);
  } else {
    Serial.println("Error on HTTP request");
    Serial.println("httpCode:");
    Serial.println(httpCode);
  }
  http.end(); // 关闭连接
}
void serviceCodeRequest(){
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
    serviceType = doc["serviceType"].as<String>();
      
    String objCode = doc["obj_code"].as<String>();
    delay(1000);
  }
    http.end(); // 关闭连接
}

void loop() {
  if(initialCode == 0){
    rfid.PCD_Init();  //RFID 重置
  }
  shelfStatus = 0;
  
  
  //RFID 检测 找卡
  if (!rfid.PICC_IsNewCardPresent()) //有新的卡片出现时 if 判断为 false，不执行 if 函数，执行 if 之后的函数。没有新卡片出现时 if 判断为 true 执行 if 函数，return 会中断 loop 循环。
   {
     return;
   }
   if (!rfid.PICC_ReadCardSerial())  
   {
     return;
   }
  //设定空字符串用来存储UID 信息
  Serial.print("UID tag:");
  String content = ""; 
  for (byte i = 0; i < rfid.uid.size; i++) {
   content.concat(String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " "));
   content.concat(String(rfid.uid.uidByte[i], HEX));
  }
  content.toUpperCase();//将读取到的 UID 信息转换成大写
  Serial.println(content);

  //验证 RFID 卡信息
  if (content.substring(1) == "16 ED EE AD")  //从索引位置 1 开始提取，可以加多个卡片信息：content == "xx xx xx xx"||content == "xx xx xx xx"
  {
    Serial.println("Message : Authorized access");
    Serial.println();
    initialCode =1;
    lock_check = "1";   
    tone(BUZZER, 1000, 100); //声音大小：500HZ 持续时间：0.3 秒
 
    while(serviceType == ""){
      serviceCodeRequest();
      // if(seviceType != ""){
      //   return;
      // }
    }
      // if (serviceType == "Borrow" && objCode != "") {
    if (serviceType == "Borrow") {
     Serial.println(serviceType);
     //调用Shelf get请求
     sendGetRequest();   
      
    if (shelfStatus == 1) {
     Serial.print("shelfStatus:");
     Serial.println(shelfStatus);
     digitalWrite(RELAY, LOW);  //开门
     lock_close = "0";
     Serial.println("The door is open");
     delay(3000);
     Serial.print("magnetic Value Pre: ");
     Serial.println(magValue);
    
     //检测门是否关好
     while (magValue < 2800){
      magValue = analogRead(magsensor); 
      //Serial.print("magnetic Value Test: ");
      //Serial.println(magValue); 
     }

     digitalWrite(RELAY, HIGH);  //电磁锁通电
     Serial.println("Door is closed");
     Serial.print("magnetic Value After: ");
     Serial.println(magValue); 
     lock_close = "1";
     initialCode = 0;
     magValue = 0;
     shelfStatus = 0; 
     serviceType = "";
    } 
    }

    if (serviceType == "Return") {
     Serial.println(serviceType);
     Serial.print("shelfStatus:");
     Serial.println(shelfStatus);
     digitalWrite(RELAY, LOW);  //开门
     lock_close = "0";
     Serial.println("The door is open");
     delay(3000);
     Serial.print("magnetic Value Pre: ");
     Serial.println(magValue);
    
     //检测门是否关好
     while (magValue < 2800){
      magValue = analogRead(magsensor); 
      //Serial.print("magnetic Value Test: ");
      //Serial.println(magValue); 
     }

     digitalWrite(RELAY, HIGH);  //电磁锁通电
     Serial.println("Door is closed");
     Serial.print("magnetic Value After: ");
     Serial.println(magValue); 
     lock_close = "1";
     initialCode = 0;
     magValue = 0;
     shelfStatus = 0; 
     serviceType = "";
    }
    

  } else {
     Serial.println("Access denied");
     lock_check = "0";
     tone(BUZZER, 600);
     delay(100); //持续 0.1 秒
     noTone(BUZZER);
     tone(BUZZER, 600);
     delay(100);
     noTone(BUZZER);
  }
  delay(2000);
  lock_check = ""; 
  //serviceType = "";
  }

