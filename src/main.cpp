#include<Arduino.h>
#include<WiFi.h>
#include<FirebaseESP32.h>
#include<ESP32Servo.h>
// servo info
Servo barrier;
#define BARRIER_PIN 22
//wifi info
#define WIFI_SSID "Win10"
#define WIFI_PASSWORD "00000000"
//FB INFO
#define API_KEY "AIzaSyBb01NrdKpcT9Nj5cbq8ttb45c8MTFJmCg"
#define DATABASE_URL "https://demo2-77b78-default-rtdb.asia-southeast1.firebasedatabase.app/"
// #define USER_EMAIL ""
// #define USER_PASSWORD ""
#define STREAM_PATH "/barrier_in"
// cycle read fb
#define FirebaseReadInterval 1500
// barrier state type
typedef enum{
  BARRIER_OPEN  =0,
  BARRIER_CLOSE =1,
} barrier_state_t;
// var
unsigned long recMillis = 0;
FirebaseData fbData;
String strData;
barrier_state_t barrierState=BARRIER_CLOSE;
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


void setup() {
 Serial.begin(115200);
 WiFiInit();
 FirebaseInit();
 barrierInit();
}


void loop() {
      if(WiFi.status()==WL_CONNECTED){
        if(Firebase.ready()&&(millis() - recMillis > FirebaseReadInterval||recMillis==0)){
          Firebase.getString(fbData,STREAM_PATH);
          strData=fbData.stringData();
          barrier_state_t dtbState;
          if(cmpStr(strData,"close")) {dtbState=BARRIER_CLOSE; Serial.println("close");}
          else if(cmpStr(strData,"open")) {dtbState=BARRIER_OPEN; Serial.println("open");}
          else {
            Serial.println("erro");
            goto SKIP;
            }
          if(dtbState!= barrierState){
            if(dtbState==BARRIER_OPEN) barrierOpen();
            else  barrierClose();
            barrierState=dtbState;
          }
         
          SKIP:recMillis=millis();
        }
        // if(onDataChange(barrierState)){
        //   if(barrierState== BARRIER_CLOSE) Serial.println("close");
        //   else Serial.println("open");
        // }
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
  // barrier.attach(BARRIER_PIN);
  barrier.setPeriodHertz(50); // PWM frequency for SG90
  barrier.attach(BARRIER_PIN, 500, 2400); // Minimum and maximum pulse width (in µs) to go from 0° to 180
  // close barrier
  barrier.write(0);
}

void barrierOpen(){
  for (uint8_t i = 0; i <=90; i+=5){
    Serial.println(i);
    barrier.write(i);
    delay(15);
  }
}

void barrierClose(){
   for (uint8_t i = 90; i >0; i-=5){
    Serial.println(i);
    barrier.write(i);
    delay(15);
    }
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
