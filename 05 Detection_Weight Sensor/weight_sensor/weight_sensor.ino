//
//  https://github.com/dvarrel/ESPAsyncWebSrv
//  https://github.com/me-no-dev/ESPAsyncWebServer/blob/master/examples/simple_server/simple_server.ino
//
//  have to change AsyncWebSocket.cpp
//  line: 832
//    from
//      return IPAddress(0U);
//    to
//      return IPAddress((uint32_t) 0U);

//
// A simple server implementation showing how to:
//  * serve static messages
//  * read GET and POST parameters
//  * handle missing pages / 404s
//

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
//#include <vector>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebSrv.h>

AsyncWebServer server(80);

// 定义你要请求别人的 URL
const char* post_url = "http://.com/get"; // GET请求的URL

// 给屏幕用的：
const char* get_url = "http://IPAddress/Service_and_Code"; // GET请求的URL
String service_type = "Borrow"; //borrow 是初始状态

const char* ssid = "SSID";         /*Enter Your SSID*/
const char* password = "PASSWORD";

const char* PARAM_MESSAGE = "message";

//HAVE 
bool haveObject[6];

int measures[6];

char changedNum[6];
char nochange[6];

/*整体逻辑：
1) setup 之后会read第一波weight。

2) 正式read weight。

*/



void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void setup() {
  // put your setup code here, to run once:
  pinMode(1, INPUT);
  pinMode(0, INPUT);
  
  for (int i = 0; i < 6; i ++) {
    haveObject[i] = false;
    measures[i] = 0;

    changedNum[i] = '0';
    nochange[i] = '0';
  }
    Serial.begin(115200);
    Serial.println("weight sensor module starts working");

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("WiFi Failed!\n");
        return;
    }

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Send a GET request to <IP>/get?message=<message>
    server.on("/weight_setup", HTTP_GET, [] (AsyncWebServerRequest *request) {
        Serial.println("GET method: weight set up");
        int params = request->params();
        setup_weights();
        String msg = "set up!!";
        //msg += millis();
        request->send(200, "text/plain", msg.c_str());
    });
    

    server.on("/weight_check", HTTP_GET, [] (AsyncWebServerRequest *request) {
        /*Serial.println("GET method: weight set up");
        int params = request->params();
        int isDetected = 0;
        String msg = "0";
        if (!checkArrays(changedList,nochange)) {
          isDetected = 1;
          msg = "1";
        }*/
        //std::vector<int> sendlist = obj_list;//这里要怎么把msg和objlist一起送出去？
        //定义你要发送post请求的时候需要传的对象内容
        StaticJsonDocument<200> doc; // 创建JSON文档对象
        if (!checkArrays(changedNum,nochange)) {
          doc["is_detected"] = 1; // 添加键值对
          doc["detectedLevel"] = changedNum;//添加obj list似乎不行
        }else {
          doc["is_detected"] = 0; // 添加键值对
          doc["detectedLevel"] = changedNum;}

        String jsonBody;
        serializeJson(doc, jsonBody); // 将JSON对象序列化为字符串
        //esp32作为服务响应http get请求时，如何发送内容为对象 发送http response 如何响应的内容为对象


        //msg += millis();
        request->send(200, "application/json",jsonBody);
    });

    server.onNotFound(notFound);

    server.begin();
}


void sendGetRequest() {
  HTTPClient http;
  http.begin(get_url); // 指定GET请求的URL
  int httpCode = http.GET(); // 发送GET请求

  if (httpCode > 0) {
    String payload = http.getString(); // 获取响应内容
    Serial.println(httpCode);
    Serial.println(payload);
  } else {
    Serial.println("Error on HTTP request");
  }

  http.end(); // 关闭连接
}


void readWeights(){
  char prevNum[6];
  copyArrays(prevNum, changedNum);
  for (int i = 0; i < 6; i++) {
    //if (i != 4 && i != 0) {break;}
    if (i == 1 || i == 0) {
    int prev = measures[i];
    int n = analogRead(i);
    measures[i] = n;
    Serial.println("the weight of ");
    Serial.println(i);
    Serial.println(" is: ");
    Serial.println(n);
    if (abs(n - prev) > 80) {
      Serial.println("Changed occured!");
      if (n < prev) {
        haveObject[0] = false;
        Serial.print("The object on");
        Serial.print(i); 
        Serial.println(" is Taken away.");
      } else {
        haveObject[0] = true;
        Serial.print("Object is put on ");
        Serial.print(i);
        Serial.println(" location.");
      }
      changedNum[i] ='1';
      //objectchangedList[i] =1;
      //obj_list.push_back(i);
    }
    
  }
  }
    if (!checkArrays(changedNum,prevNum)) {
      Serial.println("isDetected.");
    }
    //obj_list.clear();
    delay(2000);
    return;
      

}

void resetCharList (char arr[]) {
  for (int i = 0; i < 6; i ++) {
    arr[i] = '0';
  }
}

void setup_weights() {
  //detect everything and set the original state.
  for (int i = 0; i < 6; i ++) {
    haveObject[i] = false;
    measures[i] = 0;
  }
  Serial.println("Setup Weight:");

   for (int i = 0; i < 6; i++) {
    int n = analogRead(i);
    measures[i] = n;
    if (n > 100) {
      haveObject[i] = true;
    }
    Serial.println(n);
  }

  resetCharList(changedNum);
  return;

}

void loop() {
  readWeights();
  Serial.println(WiFi.localIP());
  delay(2000);
}

bool checkArrays(char arr1[], char arr2[]){
  for (int i = 0; i < 6; i++){
 
        if (arr1[i] != arr2[i]) {return false;}
  }
  return true;
}

void copyArrays(char arr1[], char arr2[]){
  for (int i = 0; i < 6; i++){
 
        arr1[i] = arr2[i];
  }
  return;
}

