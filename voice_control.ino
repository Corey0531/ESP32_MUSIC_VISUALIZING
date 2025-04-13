/// @file    Blink.ino
/// @brief   Blink the first LED of an LED strip
/// @example Blink.ino

//為了丟github所以有加入英文註解
//Program author: Corey
//2025-4-13

#include <FastLED.h>
#define NUM_LEDS 60  //how many LED you have
#define DATA_PIN 23  //use this
#define CLOCK_PIN 13
CRGB leds[NUM_LEDS];

//模式判別
//Mode identification
const int buttonPin = 0;
//esp32上的按鈕
//Buttons on esp32
int mode = 0;
bool lastButtonState = HIGH;
bool currentButtonState;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
//防止機械彈跳
//Prevent mechanical bounce

bool modeChanged = false;  // 用來判斷是否剛切換模式
//Used to determine whether the mode has just been switched
volatile float micValue = 0.0;  // volatile 是關鍵字，確保變數不被快取，及時更新
//Volatile is a keyword that ensures that variables are not cached and updated in a timely manner

#define NUM_SAMPLES 10
float voltageBuffer[NUM_SAMPLES];
int bufferIndex = 0;


//micphone Voltage read
//using MAX4466
//有平滑處理，防止跳動
//Smooth treatment to prevent jumping
void Task1(void *pvParameters) {
  while (1) {
    uint16_t adc = analogRead(A13);
    float voltage = adc / 4095.0 * 3.3;

    voltageBuffer[bufferIndex] = voltage;
    bufferIndex = (bufferIndex + 1) % NUM_SAMPLES;

    float sum = 0.0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
      sum += voltageBuffer[i];
    }
    micValue = sum / NUM_SAMPLES;

    Serial.println(micValue);
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

//ledcontrol
//WS2815 or WS2812
void Task2(void *pvParameters) {
  while (1) {
    int reading = digitalRead(buttonPin);

    if (reading != lastButtonState) {
      lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
      if (reading == LOW && currentButtonState == HIGH) {
        mode = (mode + 1) % 2;
        //如果要增加模式可以改這
        //If you want to add a mode, you can change this

        modeChanged = true;
        //表示這一輪剛切換模式
        //Indicates that this round has just switched modes
        Serial.print("Mode changed to: ");
        Serial.println(mode);
      }
      currentButtonState = reading;
    }

    lastButtonState = reading;

    // 初始化變數
    //Initialize variables
    float adjusted = 0.0;
    int numToLight = 0;


    // ---------- 根據 mode 顯示 LED ----------
    if (modeChanged) {
      Serial.print("Mode changed to: ");
      Serial.println(mode);
      modeChanged = false;
    }

    float minVoltage = 1.10;
    //量到的最低值是1.1左右
    //The lowest value measured is about 1.1
    float maxVoltage = 2.10;
    //最高值是2.1左右
    //The highest value is around 2.1

    //推測是靠音壓去改變電壓，因為住在宿舍所以不敢開太大聲
    //I guess it's the sound pressure that changes the voltage. I don't dare turn it up too loud because I live in a dormitory.
    //理論上極限是3.3
    //Theoretically the limit is 3.3
    adjusted = constrain(micValue, minVoltage, maxVoltage);
    numToLight = map((adjusted - minVoltage) * 1000, 0, (maxVoltage - minVoltage) * 1000, 0, NUM_LEDS);
    numToLight = constrain(numToLight, 0, NUM_LEDS);

    fill_solid(leds, NUM_LEDS, CRGB::Black);

    switch (mode) {
      case 0:
        for (int i = 0; i < numToLight; i++) {
          leds[i] = CRGB::White;
        }
        break;
        //All white
      case 1:
        for (int i = 0; i < numToLight; i++) {
          int j = i % 3;
          leds[i] = (j == 0) ? CRGB::Green : (j == 1) ? CRGB::Red
                                                      : CRGB::Blue;
        }
        break;
        //Like GRBGRBGRB...
    }


    FastLED.show();
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}




void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);
  currentButtonState = digitalRead(buttonPin);
  //初始化避免第一次誤判
  //Initialization to avoid misjudgment at the first time

  xTaskCreatePinnedToCore(Task1, "Task1", 10000, NULL, 1, NULL, 0);  // Core 0
  xTaskCreatePinnedToCore(Task2, "Task2", 10000, NULL, 1, NULL, 1);  // Core 1

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);     // GRB ordering is assumed
  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);  // GRB ordering is typical

  analogSetAttenuation(ADC_11db);
  analogSetWidth(12);
  //麥克風的設定
  //Microphone settings
}

void loop() {
  //不需要寫任何東西
  //No need to write anything
}
