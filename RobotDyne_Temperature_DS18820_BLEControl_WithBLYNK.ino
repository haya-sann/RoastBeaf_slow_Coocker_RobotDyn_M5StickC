//M5StickCを使った低温調理器！
//Detect Temperature with DS18820 to control RoboDyne ZeroCross PWM Controller 
/*
この本体プログラムと同じ階層に
blynkToken.h
というファイルを作り、以下のように記述して下さい。
//// You should get Auth Token in the Blynk App.
//// Go to the Project Settings (nut icon).
//// This is a secret key for BLYNK
char auth[]="Your Blink Auth Token"; //Auth Token:低温調理器用
*/

#define BLYNK_PRINT Serial

#define BLYNK_USE_DIRECT_CONNECT

#include <M5StickC.h>  //追加部分
#include <BlynkSimpleEsp32_BLE.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TimeLib.h>
#include <WidgetRTC.h> //To use the Time library in an Arduino sketch, include TimeLib.h.
#include "blynkToken.h"

// DS1818B20 Sense  wire is plugged into  pin G0 on the M5StickC - GPIO 0
#define ONE_WIRE_BUS 0
#define SENSER_BIT  9   // 精度の設定bit。9 or 12

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);


// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature DS18B20(&oneWire);
char temperatureCString[6];
char temperatureFString[6];

BlynkTimer timer;
WidgetRTC rtc;

#include <RBDdimmerESP32.h>//

#define PWM_PIN  26 // GPIO for dimming, don't use pins 28(GPIO9), 29(GPIO10), 30(GPIO11), 31(GPIO6), 33(GPIO8), 5(GPIO36), 8(GPIO39), 10(GPIO34), 11(GPIO35), 32(GPIO7) 
#define ZCPin   36 // GPIO for Zero-Cross, don't use pins 28(GPIO9), 29(GPIO10), 30(GPIO11), 31(GPIO6), 33(GPIO8)
#define heaterMAXVALUE 60

dimmerLampESP32 dimmer(PWM_PIN, ZCPin); //initialase port for dimmer  for ESP8266, ESP32, Arduino due boards
int heater = 57; //default cooking temperature
bool emergencyStop = false;

// Digital clock display of the time
void clockDisplay()
{
  // You can call hour(), minute(), ... at any time
  // Please see Time library examples for details

  String currentTime = String(hour()) + ":" + minute() + ":" + second();
  String currentDate = String(day()) + " " + month() + " " + year();

  // Send time to the App
  Blynk.virtualWrite(V10, currentTime);
  // Send date to the App
  Blynk.virtualWrite(V11, currentDate);
  M5.Lcd.setTextColor(RED);
  M5.Lcd.fillRect(0, 15, 180, 21, BLACK);
  M5.Lcd.setCursor(0, 15, 4);
  M5.Lcd.print(currentTime);    

  
}

BLYNK_CONNECTED() {
  // Synchronize time on connection
  rtc.begin();
}


void getTemperature() {
float tempC;
float tempF;
float threshold = 1.0;
int magnification =15; //設定温度近くでのパワー設定に影響がある。
int powerRatio;

do {
    DS18B20.requestTemperatures(); 
    tempC = DS18B20.getTempCByIndex(0) - 3.7 ;//氷の温度を校正測って
    dtostrf(tempC, 2, 2, temperatureCString);
    tempF = DS18B20.getTempFByIndex(0);
    dtostrf(tempF, 3, 2, temperatureFString);
    
    // You can send any value at any time.
    // Please don't send more that 10 values per second.

    Serial.print("In the Do Loop. Temp : "); Serial.print(tempC); Serial.print(" / ");
    Serial.print("Target temperature : "); Serial.print(heater); Serial.print(" / ");
    Blynk.virtualWrite(V5, tempC);
    Blynk.virtualWrite(V2, heater); //set initial value
    M5.Lcd.setTextColor(OLIVE);
    M5.Lcd.setCursor(26, 35, 2);
    M5.Lcd.print("TEMP /");
    M5.Lcd.setCursor(86, 35);
    M5.Lcd.print("Target");    

    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.fillRect(0, 55, 180, 24, BLACK);
    M5.Lcd.setCursor(0, 55, 4);
    M5.Lcd.printf("%4.2f / %d", tempC, heater);    


//    if (tempC <=threshold) {analogWrite(PWMPin, 0.0);}
//    else {analogWrite(PWMPin, (tempC-threshold) * magnification);}
//   

    if (emergencyStop == false) {
        powerRatio = (heater+threshold-tempC)*magnification;
        if (powerRatio > 100) powerRatio = 100;
        if (powerRatio < 0) powerRatio = 0;
    } 
    else powerRatio = 0;    
    
    Serial.print("Heating Value:");
    Serial.println(powerRatio);
    dimmer.setPower(powerRatio); // setPower(設定温度-現在温度)*magnification( default = 30)
    Blynk.virtualWrite(V1,powerRatio);
    clockDisplay();
  } while (tempC == 85.0 || tempC == (-127.0));
}

void setup()
{
  M5.begin();  
  Serial.begin(115200);
  dimmer.begin(NORMAL_MODE, ON); //dimmer initialisation: name.begin(MODE, STATE) 
  M5.Lcd.setRotation(1);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextFont(2);
  
  pinMode(GPIO_NUM_10, OUTPUT);    //追加部分
  digitalWrite(GPIO_NUM_10, HIGH);    //追加部分
  pinMode(ONE_WIRE_BUS, INPUT);    //追加部分
  
  // Debug console

  Serial.println("Waiting for connections...");
  Blynk.setDeviceName("M5StickC_Blynk"); //Device Name for BT 
  M5.Lcd.print("BLE : M5StickC_Blynk");
  Blynk.begin(auth); //auth token is written in blynkToken.h file to prevent public release.

  
  DS18B20.begin(); 
// IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
  DS18B20.setResolution(SENSER_BIT);

  // Setup a function to be called every 3 seconds
  timer.setInterval(3000L, getTemperature); //これでプロセスは終わる。この後にコードを書いても実行されないからそのつもりで。
}

void loop() {

    Blynk.run();
    timer.run();
    M5.update();
    delay(1000);
}

//追加部分

BLYNK_WRITE(V2) { //温度設定用値
  heater = param.asInt();
  Serial.print("Detect V2 change : heater Value:");
  Serial.println(heater);
}


BLYNK_WRITE(V3) { //
    int emergencyFlag = param.asInt();
      if ( emergencyFlag == 1 ){
        emergencyStop = true;
      }
      else emergencyStop = false;
    Serial.print("Emergency Stop Button is pressed : ");
    Serial.println(emergencyStop);
}

BLYNK_WRITE(V4) { //
    int PowerSwitch = param.asInt();
      if ( PowerSwitch == 100 ){
    Serial.print("Poweroff Button is pressed : ");
    for(int i=0; i<5; i++){
    M5.Lcd.fillScreen(RED);
    delay(100);
    M5.Lcd.fillScreen(BLACK);
    delay(100);
      }
    M5.Lcd.setCursor(0, 30, 4);
    M5.Lcd.printf("Now Power");    
    M5.Lcd.setCursor(0, 55, 4);
    M5.Lcd.printf("Going Down");    
    delay(5000);
    M5.Axp.PowerOff();
  } else {
    Serial.println(PowerSwitch);
    Blynk.virtualWrite(V4,0);
  }
}
