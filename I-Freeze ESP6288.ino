#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <DHT.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>

// digital pin we're connected to (temperature sensor)
#define DHTPIN 4
// digital pin we're connected to (Door sensor)
#define REED_SWITCH 2

#define DHTTYPE DHT11   
DHT dht(DHTPIN, DHTTYPE);

//Wi-Fi
const char* ssid     = "developer";
const char* password = "gogowawaZ!2020";

String tempDoorStatus = "OPEN";
int tempTemperature = 1000;
int tempHumidity = 1000;

const long utcOffsetInSeconds = 3600;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

//URL
const char* updateFridgeURL = "http://us-central1-ifreeze-d1941.cloudfunctions.net/fridgeApp/updateFridge/WwHAYf90KI06As9wR0N1";
const char* createHistoryURL = "http://us-central1-ifreeze-d1941.cloudfunctions.net/fridgeApp/createHistory";

//----------------------------------------------------------------// 
//By Static ssid and password
void setupWifi1(){
   WiFi.begin(ssid, password);    
   Serial.print("Attempting to connect to SSID: ");
   Serial.println(ssid);  

   // Wait for connection
   while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
   }
   Serial.println("Connecte...");
   timeClient.begin();
}

//By Hotspot
void setupWifi2(){
	WiFiManager wifiManager;
	wifiManager.autoConnect("i-freeze");
	if (WiFi.status() != WL_CONNECTED){
   	 Serial.println("Disconnect");
     	 WiFiManager wifiManager;
   	 wifiManager.startConfigPortal("i-freeze","12345678");
     	 Serial.println("Connected to wifi");
   }	

}

//----------------------------------------------------------------// 

void setup() {
  
  pinMode(REED_SWITCH, INPUT_PULLUP);
  
  dht.begin();
  
  Serial.begin(9600);

  setupWifi();

}

//----------------------------------------------------------------// 

String getDoorSensor(){
 
  String doorStatus;
  
  if ((digitalRead(REED_SWITCH) == HIGH))
  {
     Serial.println("DOOR OPEN!!");
    
     doorStatus = "OPEN";

  } 
  else if ((digitalRead(REED_SWITCH) == LOW))
  {
     Serial.println("DOOR CLOSED!!");
     
     doorStatus = "CLOSED";

  }
   return doorStatus;
  
}

//----------------------------------------------------------------// 

int getTemperature(){
  
  int temperature = dht.readTemperature();// Read temperature as Celsius (the default)
//  float h = dht.readHumidity();// Reading humidity 
//  float f = dht.readTemperature(true);// Read temperature as Fahrenheit (isFahrenheit = true)
//  tValue =c;
  return temperature;
}

//----------------------------------------------------------------// 

int getHumidity(){
  
  int humidity = dht.readHumidity();// Reading humidity 

  return humidity;
}

//----------------------------------------------------------------//
//Update data if change
void updateData(String doorStatus, float temperature, float humidity, String timeNow) {
 
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(updateFridgeURL);

  String updateData = "temperature=" + String(temperature) + "&humidity=" + String(humidity) + "&doorStatus=" + doorStatus + "&timeNow=" + timeNow; 
  http.addHeader("Content-Type", "application/x-www-form-urlencoded"); 

  int httpResponseCode = http.PUT(updateData);
  
    // Send HTTP PUT request
  if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String result = http.getString();
      Serial.println(result);
  }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }

    // Free resources
    http.end();
  }
  else {
    Serial.println("WiFi Disconnected");
  }
}

//----------------------------------------------------------------//
//Send data to history link 
void createHistory(String doorStatus, String timeNow) {
 
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(createHistoryURL);

  String createHistoryData = "doorStatus=" + doorStatus + "&time=" + timeNow; 
  http.addHeader("Content-Type", "application/x-www-form-urlencoded"); 

  int httpResponseCode = http.POST(createHistoryData);
    
    // Send HTTP PUT request
  if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String result = http.getString();
      Serial.println(result);
  }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
      }

    // Free resources
    http.end();
  }
  else {
    Serial.println("WiFi Disconnected");
  }
}

//----------------------------------------------------------------// 
bool compare(int temperature, int humidity, String doorStatus){
  
  bool flag = false;
  
  if(temperature != tempTemperature){
    tempTemperature = temperature;
    flag = true;
  }

  if(humidity != tempHumidity){
    tempHumidity = humidity;
    flag = true;
  }
  
  if(doorStatus != tempDoorStatus){
    String timeNow = daysOfTheWeek[timeClient.getDay()] + String(", ") + String(timeClient.getHours()+1) + ":" + String(timeClient.getMinutes()) + ":" + String(timeClient.getSeconds());
    createHistory(doorStatus,timeNow);
    tempDoorStatus = doorStatus;
    flag = true;
  }
   
  return flag;
}
//----------------------------------------------------------------// 

void loop() {
  
   if (WiFi.status() != WL_CONNECTED)
   {
     setupWifi();
   }
   String doorStatus = getDoorSensor();
   timeClient.update();
   int temperature = getTemperature();
   int humidity    = getHumidity();
   String timeNow = daysOfTheWeek[timeClient.getDay()] + String(", ") + String(timeClient.getHours()+1) + ":" + String(timeClient.getMinutes()) + ":" + String(timeClient.getSeconds());
   if(compare(temperature, humidity, doorStatus)){
     updateData(doorStatus, temperature, humidity, timeNow);
     Serial.println("changed");
   }
   else{
    Serial.println("not changed");
   }
   delay(1800000); //kol 6 sa3at
   
        
}
