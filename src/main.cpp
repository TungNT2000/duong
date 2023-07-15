#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <OneButton.h>
StaticJsonDocument<250> docIn;
// lcd info
LiquidCrystal_I2C lcd(0x27, 16, 2);
// servo info
Servo barrierIn;
Servo barrierOut;
#define BARRIER_IN_PIN 23
#define BARRIER_OUT_PIN 4
// button info
#define BTN_IN 12
#define BTN_OUT 13
OneButton btnIn(BTN_IN, true);
OneButton btnOut(BTN_OUT, true);
// wifi info
#define WIFI_SSID "Win10"
#define WIFI_PASSWORD "00000000"
// FB INFO
#define API_KEY "AIzaSyB13ZLG-6PA9d6qrghp46ctHNnQzuwMJqY"
#define DATABASE_URL "https://smart-parking-b4df2-default-rtdb.firebaseio.com/"
// #define USER_EMAIL ""
// #define USER_PASSWORD ""
#define STREAM_PATH "/647f467ad3f1cad87ec87dc8"
// test info
//  #define TOTAL_SLOT_PATH "/total_slot"
// #define API_KEY "AIzaSyBb01NrdKpcT9Nj5cbq8ttb45c8MTFJmCg"
// #define DATABASE_URL "https://demo2-77b78-default-rtdb.asia-southeast1.firebasedatabase.app/"
// // #define USER_EMAIL ""
// // #define USER_PASSWORD ""
// #define STREAM_PATH "/"
// #define TOTAL_SLOT_PATH "/total_slot"
// cycle to read fb
#define FirebaseReadInterval 500
// cycle to on servo
#define barrierInterval 20
// cycle to reconnect wifi
#define reconnectWiFiInterval 5000
// barrier state type
typedef enum
{
  BARRIER_OPEN = 0,
  BARRIER_CLOSE = 1,
} barrier_state_t;
// var
unsigned long recMillis = 0, barrierInMillis = 0, barrierOutMillis = 0;
FirebaseData fbData;
barrier_state_t barrierInState = BARRIER_CLOSE;
barrier_state_t barrierOutState = BARRIER_CLOSE;
uint8_t totalSlot;
uint8_t useSlot;
uint8_t barrierInPos = 0;
uint8_t barrierOutPos = 0;
bool flag1 = false, flag2 = false;
TaskHandle_t taskHandle_1;
TaskHandle_t taskHandle_2;
void taskGetCommand(void *parameter);
void taskAction(void *parameter);
// put function declarations here:
// init wifi
void WiFiInit();
// init firebase
void FirebaseInit();
//  init servo
void barrierInit();
void barrierOpen();
void barrierClose();
bool cmpStr(String data1, String data2);
bool onDataChange(barrier_state_t data);
// init lcd
void lcdInit();
void lcdPrint(uint8_t useSlot);
// init button
void btnInit();
void btnInOnClk();
void btnOutOnClk();
void btnLoop();
void setup()
{
  Serial.begin(115200);
  WiFiInit();
  FirebaseInit();
  barrierInit();
  lcdInit();
  btnInit();
  // get total slot
  // while (Firebase.ready() && totalSlot == 0)
  // {
  //   Firebase.getInt(fbData, TOTAL_SLOT_PATH);
  //   totalSlot = fbData.intData();
  //   Serial.printf("tong so vi tri co the gui xe la:%d", totalSlot);
  //   Serial.println();
  // }
  // get data from fb
  xTaskCreatePinnedToCore(taskGetCommand, "TaskGetCmd", 1024 * 15, NULL, 1, &taskHandle_1, 0);
  // barrier action
  xTaskCreatePinnedToCore(taskAction, "TaskAction", 1024 * 15, NULL, 1, &taskHandle_2, 1);
}
void taskGetCommand(void *parameter)
{
  (void)parameter;
  while (true)
  {
    // 1 check wifi connection
    if (WiFi.status() == WL_CONNECTED)
    {
      // 2 get data from firebase database
      if (Firebase.ready() && (millis() - recMillis > FirebaseReadInterval || recMillis == 0))
      {
        Firebase.getString(fbData, STREAM_PATH);
        String strData = fbData.stringData();
        Serial.println(strData);
        // 2.1 parse json string
        docIn.clear();
        DeserializationError err = deserializeJson(docIn, strData);
        if (err)
        {
          Serial.println("false");
          Serial.println(err.f_str());
          return;
        }
        String barrierInStateTmp = docIn["barrier_in"];
        String barrierOutStateTmp = docIn["barrier_out"];
        useSlot = docIn["use_slot"];
        // 2.2 check data change
        //  barrier in
        barrier_state_t dtbState;
        // convert the barrier_in state
        if (flag1 == false)
        {
          if (cmpStr(barrierInStateTmp, "close"))
          {
            dtbState = BARRIER_CLOSE;
            Serial.println("close");
          }
          else if (cmpStr(barrierInStateTmp, "open"))
          {
            dtbState = BARRIER_OPEN;
            Serial.println("open");
          }
          else
          {
            Serial.println("erro");
            return;
          }
          if (dtbState != barrierInState)
            flag1 = true;
        }
        // convert the barrier_out state
        if (flag2 == false)
        {
          if (cmpStr(barrierOutStateTmp, "close"))
          {
            dtbState = BARRIER_CLOSE;
            Serial.println("close");
          }
          else if (cmpStr(barrierOutStateTmp, "open"))
          {
            dtbState = BARRIER_OPEN;
            Serial.println("open");
          }
          else
          {
            Serial.println("erro");
            return;
          }
          if (dtbState != barrierOutState)
            flag2 = true;
        }
        recMillis = millis();
      }
    }
    else
    {
      // check button update
      btnLoop();
      // reconnect wifi
      if (millis() - recMillis > reconnectWiFiInterval)
      {
        Serial.println("Reconnecting to WiFi...");
        WiFi.disconnect();
        WiFi.reconnect();
        recMillis = millis();
      }
    }
  }
}
void taskAction(void *parameter)
{
  (void)parameter;
  while (true)
  {
    if (((millis() - barrierInMillis) > barrierInterval || barrierInMillis == 0) && flag1)
    {
      if (barrierInState == BARRIER_CLOSE)
      {
        // open barrier
        if (barrierInPos <= 90)
        {
          Serial.printf("barier in open pos=  %d", barrierInPos);
          Serial.println();
          barrierInPos += 2;
          barrierIn.write(barrierInPos);
          // barrierInMillis= millis();
        }
        else
        {
          barrierInState = BARRIER_OPEN;
          flag1 = false;
        }
      }
      else
      {
        // close barier
        if (barrierInPos > 0)
        {
          Serial.printf("barier in close pos=%d", barrierInPos);
          Serial.println();
          barrierInPos -= 2;
          barrierIn.write(barrierInPos);
          // barrierInMillis= millis();
        }
        else
        {
          lcdPrint(useSlot);
          barrierInState = BARRIER_CLOSE;
          flag1 = false;
        }
      }
      barrierInMillis = millis();
    }
    // 3.2 barier_out action
    if ((millis() - barrierOutMillis > barrierInterval || barrierOutMillis == 0) && flag2)
    {
      if (barrierOutState == BARRIER_CLOSE)
      {
        // open barrier
        if (barrierOutPos <= 90)
        {
          Serial.printf("barier out open pos=  %d", barrierOutPos);
          Serial.println();
          barrierOutPos += 2;
          barrierOut.write(barrierOutPos);
          // barrierInMillis= millis();
        }
        else
        {
          barrierOutState = BARRIER_OPEN;
          flag2 = false;
        }
      }
      else
      {
        // close barier
        if (barrierOutPos > 0)
        {
          Serial.printf("barier iout close pos=%d", barrierOutPos);
          Serial.println();
          barrierOutPos -= 2;
          barrierOut.write(barrierOutPos);
          // barrierInMillis= millis();
        }
        else
        {
          lcdPrint(useSlot);
          barrierOutState = BARRIER_CLOSE;
          flag2 = false;
        }
      }
      barrierOutMillis = millis();
    }
  }
}
void loop() {}

void WiFiInit()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}
void FirebaseInit()
{
  Firebase.begin(DATABASE_URL, API_KEY);
  Firebase.reconnectWiFi(true);
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  Serial.println();
}

void barrierInit()
{
  barrierIn.setPeriodHertz(50);
  barrierIn.attach(BARRIER_IN_PIN, 500, 2400);
  barrierOut.setPeriodHertz(50);
  barrierOut.attach(BARRIER_OUT_PIN, 500, 2400);
  // close barrier
  barrierIn.write(0);
  barrierOut.write(0);
}

void lcdInit()
{
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
}
void lcdPrint(uint8_t useSlot)
{
  /*
          WELCOME
    SL : USESLOT/TOTALSLOT
  */
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("WELCOME");
  lcd.setCursor(0, 1);
  lcd.print("SL: ");
  lcd.print(useSlot);
  lcd.print("/");
  lcd.print(totalSlot);
  Serial.printf("sl :%d/%d", useSlot, totalSlot);
  Serial.println();
}

bool cmpStr(String data1, String data2)
{
  uint8_t dt1Leng = data1.length();
  uint8_t dt2Leng = data2.length();
  if (dt1Leng != dt2Leng)
    return false;
  for (uint8_t i = 0; i < dt1Leng; i++)
  {
    if (data1[i] != data2[i])
      return false;
  }
  return true;
}

bool onDataChange(barrier_state_t data)
{
  static barrier_state_t stateTmp = BARRIER_CLOSE;
  if (data == stateTmp)
    return false;
  stateTmp = data;
  return true;
}
void btnInit()
{
  btnIn.attachClick(btnInOnClk);
  btnOut.attachClick(btnOutOnClk);
}
void btnInOnClk()
{
  Serial.println("button in click");
  if (flag1 == false)
    flag1 = true;
}
void btnOutOnClk()
{
  Serial.println("button out click");
  if (flag2 == false)
    flag2 = true;
}
void btnLoop()
{
  btnIn.tick();
  btnOut.tick();
}