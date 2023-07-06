#include<Arduino.h>
#include<WiFi.h>
#include<FirebaseESP32.h>
#include<ESP32Servo.h>
#include<ArduinoJson.h>
#include<LiquidCrystal_I2C.h>

StaticJsonDocument<250> docIn;
// lcd info
LiquidCrystal_I2C lcd(0x27, 16, 2);
// servo info
Servo barrierIn;
Servo barrierOut;
#define BARRIER_IN_PIN  23
#define BARRIER_OUT_PIN 4
//wifi info
#define WIFI_SSID "Duong"
#define WIFI_PASSWORD "00000000"
//FB INFO
#define API_KEY "AIzaSyB13ZLG-6PA9d6qrghp46ctHNnQzuwMJqY"
#define DATABASE_URL "https://smart-parking-b4df2-default-rtdb.firebaseio.com/"
// #define USER_EMAIL ""
// #define USER_PASSWORD ""
#define STREAM_PATH "/647f467ad3f1cad87ec87dc8"
#define TOTAL_SLOT_PATH "/total_slot"
// cycle read fb
#define FirebaseReadInterval 500
// cycle on servo
#define barrierInterval 20
// barrier state type
typedef enum{
  BARRIER_OPEN  =0,
  BARRIER_CLOSE =1,
} barrier_state_t;
// var
unsigned long recMillis = 0, barrierInMillis=0, barrierOutMillis=0;
FirebaseData fbData;
barrier_state_t barrierInState=BARRIER_CLOSE;
barrier_state_t barrierOutState=BARRIER_CLOSE;
uint8_t totalSlot;
uint8_t useSlot;
uint8_t barrierInPos=0;
uint8_t barrierOutPos=0;
bool flag1=false, flag2=false;
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
void setup() {
 Serial.begin(115200);
 WiFiInit();
 FirebaseInit();
 barrierInit();
 lcdInit();
 // get total slot
// while(Firebase.ready()&& totalSlot==0){
//   Firebase.getInt(fbData,TOTAL_SLOT_PATH);
//   totalSlot=fbData.intData();
//   Serial.printf("tong so vi tri co the gui xe la:%d",totalSlot); 
//   Serial.println();
//  }
}


void loop() {
  // 1 check wifi connection
      if(WiFi.status()==WL_CONNECTED){
  // 2 get data from firebase database
        if(Firebase.ready()&&(millis() - recMillis > FirebaseReadInterval||recMillis==0)&&flag1==false&&flag2==false){
          Firebase.getString(fbData,STREAM_PATH);
          String strData=fbData.stringData();
          Serial.println(strData);
          //2.1 parse json string
          docIn.clear();
          DeserializationError err =deserializeJson(docIn,strData);
          if(err){
            Serial.println("false");
            Serial.println(err.f_str());
            return;
          }
          String barrierInStateTmp =docIn["barrier_in"];
          String barrierOutStateTmp =docIn["barrier_out"];
          useSlot =docIn["use_slot"];
          //2.2 check data change
          // barrier in
          barrier_state_t dtbState;
          // convert the barrier_in state
          if(cmpStr(barrierInStateTmp,"close")) {dtbState=BARRIER_CLOSE; Serial.println("close");}
          else if(cmpStr(barrierInStateTmp,"open")) {dtbState=BARRIER_OPEN; Serial.println("open");}
          else {  Serial.println("erro"); return; }
          if(dtbState!= barrierInState) flag1=true;
          // convert the barrier_out state
          if(cmpStr(barrierOutStateTmp,"close")) {dtbState=BARRIER_CLOSE; Serial.println("close");}
          else if(cmpStr(barrierOutStateTmp,"open")) {dtbState=BARRIER_OPEN; Serial.println("open");}
          else {  Serial.println("erro"); return; }
          if(dtbState!= barrierOutState) flag2=true;
          recMillis=millis();
        }
    //3 action
    //3.1 barier_in action
         if(((millis() - barrierInMillis) > barrierInterval||barrierInMillis==0)&&flag1){
          if(barrierInState==BARRIER_CLOSE){
            //open barrier
            if(barrierInPos<=90){
              Serial.printf("barier in open pos=  %d",barrierInPos);
              Serial.println();
              barrierInPos+=2;
              barrierIn.write(barrierInPos);
              //barrierInMillis= millis();
            }else{
              barrierInState=BARRIER_OPEN;
              flag1=false;
            }
          }else{
            // close barier
            if(barrierInPos>0){
              Serial.printf("barier in close pos=%d", barrierInPos);
              Serial.println();
              barrierInPos-=2;
              barrierIn.write(barrierInPos);
             // barrierInMillis= millis();
            }else{
              lcdPrint(useSlot);
              barrierInState=BARRIER_CLOSE;
              flag1=false;
            }
          }
          barrierInMillis= millis();
         }
    //3.2 barier_out action
        if((millis() - barrierOutMillis > barrierInterval||barrierOutMillis==0)&&flag2){
          if(barrierOutState==BARRIER_CLOSE){
            //open barrier
            if(barrierOutPos<=90){
              Serial.printf("barier out open pos=  %d",barrierOutPos);
              Serial.println();
              barrierOutPos+=2;
              barrierOut.write(barrierOutPos);
              //barrierInMillis= millis();
            }else{
              barrierOutState=BARRIER_OPEN;
              flag2=false;
            }
          }else{
            // close barier
            if(barrierOutPos>0){
              Serial.printf("barier iout close pos=%d", barrierOutPos);
              Serial.println();
              barrierOutPos-=2;
              barrierOut.write(barrierOutPos);
             // barrierInMillis= millis();
            }else{
              lcdPrint(useSlot);
              barrierOutState=BARRIER_CLOSE;
              flag2=false;
            }
          }
          barrierOutMillis= millis();
         }
      }else{
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        Serial.println("reconnect wifi: ");
        delay(3000);
      }
}


void WiFiInit(){
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
void FirebaseInit(){
  Firebase.begin(DATABASE_URL, API_KEY);
  Firebase.reconnectWiFi(true);
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  Serial.println();
}

void barrierInit(){
  barrierIn.setPeriodHertz(50); // PWM frequency for SG90
  barrierIn.attach(BARRIER_IN_PIN, 500, 2400); // Minimum and maximum pulse width (in µs) to go from 0° to 180
  barrierOut.setPeriodHertz(50); // PWM frequency for SG90
  barrierOut.attach(BARRIER_OUT_PIN, 500, 2400); // Minimum and maximum pulse width (in µs) to go from 0° to 180
  // close barrier
  barrierIn.write(0);
  barrierOut.write(0);
}

// void barrierOpen(){
//   for (uint8_t i = 0; i <=90; i+=5){
//     Serial.println(i);
//     barrier.write(i);
//     delay(15);
//   }
// }

// void barrierClose(){
//    for (uint8_t i = 90; i >0; i-=5){
//     Serial.println(i);
//     barrier.write(i);
//     delay(15);
//     }
// }

void lcdInit(){
  lcd.init();                 
  lcd.backlight();
  lcd.setCursor(0, 0);
}
void lcdPrint(uint8_t useSlot){
  /*
          WELCOME
    SL : USESLOT/TOTALSLOT
  */
  lcd.clear();
  lcd.setCursor(5,0);
  lcd.print("WELCOME");
  lcd.setCursor(0,1);
  lcd.print("SL: ");
  lcd.print(useSlot);
  lcd.print("/");
  lcd.print(totalSlot);
  Serial.printf("sl :%d/%d",useSlot,totalSlot);
  Serial.println();
}

bool cmpStr(String data1, String data2){
  uint8_t dt1Leng = data1.length();
  uint8_t dt2Leng = data2.length();
  if(dt1Leng!=dt2Leng) return false;
  for(uint8_t i=0; i< dt1Leng;i++){
    if(data1[i]!=data2[i]) return false;
  }
  return true;
}

bool onDataChange(barrier_state_t data){
  static barrier_state_t stateTmp= BARRIER_CLOSE;
  if(data== stateTmp) return false;
  stateTmp= data;
  return true;
}