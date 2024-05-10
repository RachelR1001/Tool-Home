#include <Adafruit_GFX.h>
#include <FastLED.h>
#include <FastLED_NeoMatrix.h>
#include <Fonts/TomThumb.h>
#include <Servo.h>

String wenben;
volatile int changdu;

CRGB leds[256];
FastLED_NeoMatrix *matrix;
uint8_t colorW = 80;
const int irSensorPin = 3; // 红外传感器连接到数字引脚3
bool isPersonDetected = false; // 检测到人的状态
bool actionPerformed = false; // 是否已执行动作
Servo myServo; 

void initMatrix() {
  matrix = new FastLED_NeoMatrix(leds, 32, 8, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG);
  FastLED.addLeds<NEOPIXEL, 2>(leds, 256).setCorrection(TypicalLEDStrip);
  matrix->begin();
  matrix->setTextWrap(false);
  matrix->setBrightness(colorW);
  matrix->setFont(&TomThumb);
}

void setup() {
  initMatrix();
  wenben = "Welcome to Tool Home";
  changdu = String(wenben).length();
  Serial.begin(9600);
  
  pinMode(irSensorPin, INPUT); // 设置红外传感器引脚为输入
  myServo.attach(9); // 舵机连接到数字引脚9
}

void loop() {
  isPersonDetected = digitalRead(irSensorPin); // 读取红外传感器状态

  if (isPersonDetected && !actionPerformed) {
    // 如果检测到人且动作尚未执行
    actionPerformed = true; // 标记动作已执行

    // 执行舵机动作
    unsigned long startTime = millis();
    while (millis() - startTime < 3000) { // 持续3秒
    for (int pos = 0; pos <= 180; pos += 5) {
      myServo.write(pos);
      delay(15);
    }
    for (int pos = 180; pos >= 0; pos -= 5) {
      myServo.write(pos);
      delay(15);
    }
    }

    // 显示滚动文本
    for (int i = 32; i >= (-52 - changdu); i--) {
      matrix->clear();
      matrix->setTextColor(matrix->Color(254,132,1));
      matrix->setCursor(i, 6);
      matrix->print(wenben);
      matrix->show();
      delay(100);
    }

  } else if (!isPersonDetected) {
    // 如果没有检测到人，重置动作标志并显示固定文本
    actionPerformed = false; // 重置动作标志
    matrix->clear();
    matrix->setTextColor(matrix->Color(254,132,1));
    matrix->setCursor(0, 6);
    matrix->print("ToolHome");
    matrix->show();
    delay(1000); 
  }
}

