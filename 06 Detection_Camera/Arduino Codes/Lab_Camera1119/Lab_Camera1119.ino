#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
// #include <ESPAsyncWebServer.h>
#include <ESPAsyncWebSrv.h>
#include <Base64.h>
#include <ArduinoJson.h> // 包含ArduinoJson库

//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//
//            You must select partition scheme from the board menu that has at least 3MB APP space.
//            Face Recognition is DISABLED for ESP32 and ESP32-S2, because it takes up from 15 
//            seconds to process single frame. Face Detection is ENABLED if PSRAM is enabled as well

// ===================
// Select camera model
// ===================
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
//#define CAMERA_MODEL_ESP32S3_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
//#define CAMERA_MODEL_M5STACK_UNITCAM // No PSRAM
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM
//#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
// ** Espressif Internal Boards **
//#define CAMERA_MODEL_ESP32_CAM_BOARD
//#define CAMERA_MODEL_ESP32S2_CAM_BOARD
//#define CAMERA_MODEL_ESP32S3_CAM_LCD
//#define CAMERA_MODEL_DFRobot_FireBeetle2_ESP32S3 // Has PSRAM
//#define CAMERA_MODEL_DFRobot_Romeo_ESP32S3 // Has PSRAM
#include "camera_pins.h"

// ===========================
// Enter your WiFi credentials
// ===========================

const char* ssid = "SSID";
const char* password = "PASSWORD";
String camResponse = "";

AsyncWebServer server(8012);
// WebServer server(8012); // HTTP服务器在端口自己定义，不要都写 80！！！

//shelf物品编码
const char* url1 = "http://IPAddress/Service_and_Code";
//shelf是否已经转好
const char* url2 = "http://IPAddress/shelf_ready";
//发送图片和&level给python进行处理
const char* url3 = "http://IPAddress/lock_close";
const char* postUrl = "http://IPAddress/compare-images";


void startCameraServer();
void setupLedFlash(int pin);

String takePhoto() {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return "";
  }

  String base64Image = base64::encode(fb->buf, fb->len);
  esp_camera_fb_return(fb);
  return base64Image;
}

// void handlePostRequest(AsyncWebServerRequest *request) {
//   if (request->hasParam("body", true)) {
//     AsyncWebParameter* p = request->getParam("body", true);
//     Serial.println(p->value());
//     request->send(200, "text/plain", camResponse);
//   } else {
//     request->send(400, "text/plain", "POST request missing body.");
//   }
// }

// void setupServer() {
//   server.on("/cam_compare", HTTP_GET, [](AsyncWebServerRequest *request){
//     request->send(200, "text/plain", "Hello from ESP32 CAM!");
//   });

//   // server.on("/cam_compare", HTTP_POST, handlePostRequest);
//   server.begin();
// }

void sendPostRequest(String photo1, String photo2, String shelfLevel) {
  HTTPClient http;
  http.begin(postUrl);
  http.addHeader("Content-Type", "application/json");

  String requestBody = "{\"photo1\":\"" + photo1 + "\", \"photo2\":\"" + photo2 + "\", \"shelfLevel\":\"" + shelfLevel + "\"}";
  int httpCode = http.POST(requestBody);

  if (httpCode > 0) {
    // String response = http.getString();
    camResponse = http.getString();
    Serial.println(camResponse);
    server.on("/cam_compare", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", camResponse);
  });

  // server.on("/cam_compare", HTTP_POST, handlePostRequest);
  server.begin();
  }

  http.end();
}
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.frame_size = FRAMESIZE_SVGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(config.pixel_format == PIXFORMAT_JPEG){
    if(psramFound()){
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
      config.frame_size = FRAMESIZE_SVGA;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if(config.pixel_format == PIXFORMAT_JPEG){
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash(LED_GPIO_NUM);
#endif

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

 // setupServer();
  // 定义你部署的服务路由
  // server.on("/", HTTP_GET, handleRoot); //Get的方法
  // server.on("/post", HTTP_POST, handlePost); //Post的方法

  // 启动服务器
  // server.begin();
}


void loop() {
  Serial.println("In loop");
  HTTPClient http;
  http.begin(url1);
  int httpCode = http.GET();
  Serial.println(httpCode);

  if (httpCode > 0) {
    Serial.println(httpCode);
    String itemCode = http.getString();
    Serial.println(itemCode);
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, itemCode);

    String value = doc["obj_code"].as<String>(); // 替换 "key" 为您想提取的字段名
    Serial.println(value);
    // Serial.println(itemCode.obj_code);
    if(value!=""){
       String shelfLevel = value.substring(2, 3);
        Serial.println(shelfLevel);
        http.end();

        http.begin(url2);
        httpCode = http.GET();
        Serial.print("url2");
        Serial.println(httpCode);
        if (httpCode > 0 && http.getString() == "1") {
          String photo1 = takePhoto();
          Serial.print("photo1");
          Serial.println(photo1);
          // Serial.println("First request sent at: " + String(millis()));
          delay(3000);
          http.begin(url3);
          httpCode = http.GET();
          Serial.print("url3");
          Serial.println(httpCode);
          if (httpCode > 0 && http.getString()== "1") {
            String photo2 = takePhoto();
            Serial.print("photo2");
            Serial.println(photo2);
            // Serial.println("First request sent at: " + String(millis()));
            sendPostRequest(photo1, photo2, shelfLevel);
          }
        }
    }
   
  }

  http.end();
  delay(1000);
}
