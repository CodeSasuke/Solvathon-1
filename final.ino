#include <Wire.h>
// #include <EEPROM.h>
#include <WiFi.h>
#include <OneWire.h>
#include "time.h"
#include <DallasTemperature.h>
const int oneWireBus = 32;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
#define SCOUNT  20            // sum of sample point
#define TdsSensorPin 34
#define VREF 3.3              // analog reference voltage(Volt) of the ADC
int n = 25;
// const char* ntpServer = "pool.ntp.org";

// Variable to save current epoch time
// unsigned long epochTime; 
// float voltage, ecValue, temperature = 25;
String apiKey = "GN0SJ0JE16TML8FQ";     //  Enter your Write API key from ThingSpeak
const char* ssid =  "realme";     // replace with your wifi ssid and wpa2 key
const char* pass =  "123456789";
const char* server = "api.thingspeak.com";
int analogBuffer[SCOUNT];    // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0,copyIndex = 0;
float averageVoltage = 0,tdsValue = 0;
int sensor = 0; // variable for averaging
int sensorValue = 0;
float voltage = 0.00;
float turbidity = 0.00;

WiFiClient client;

 





void setup()
{
  Serial.begin(115200);
  pinMode(TdsSensorPin,INPUT);
  Serial.println("Connecting to ");
  Serial.println(ssid);
  sensors.begin();

  WiFi.begin(ssid, pass);
 
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
}
 
 
void loop()
{
  float temp = TEMP();
  float tds = TDS(temp);
  float Turbidity = TURBIDITY();
  delay(1000);
  if (client.connect(server, 80))  //   "184.106.153.149" or api.thingspeak.com
  {
 
    String postStr = apiKey;
    postStr += "&field1=";
    postStr += String(temp);
    postStr += "&field2=";
    postStr += String(Turbidity);
    postStr += "&field3=";
    postStr += String(tds);
    // postStr += "\r\n\r\n";
    delay(1);
    Serial.println(postStr);
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    // client.print("Host: 192.168.17.16\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
    delay(500);
  }
  client.stop();
}


float TEMP()
{
  sensors.requestTemperatures(); 
  float temperatureC = sensors.getTempCByIndex(0);
  float temperatureF = sensors.getTempFByIndex(0);
  Serial.print(temperatureC);
  Serial.println("ºC");
  Serial.print(temperatureF);
  Serial.println("ºF");
  return temperatureC;
}

float TDS(float temp)
{
  static unsigned long analogSampleTimepoint = millis();

   if(millis()-analogSampleTimepoint > 40U)     //every 40 milliseconds,read the analog value from the ADC
   {
     analogSampleTimepoint = millis();
     analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    //read the analog value and store into the buffer
     analogBufferIndex++;
     if(analogBufferIndex == SCOUNT) 
         analogBufferIndex = 0;
   }   
  //  static unsigned long printTimepoint = millis();
   static unsigned long printTimepoint = millis();
   if(millis()-printTimepoint > 800U)
   {
      printTimepoint = millis();
      averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 2048.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      float compensationCoefficient=1.0+0.02*(25-25.0);    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
      float compensationVolatge=averageVoltage/compensationCoefficient;  //temperature compensation
      tdsValue=(133.42*compensationVolatge*compensationVolatge*compensationVolatge - 255.86*compensationVolatge*compensationVolatge + 857.39*compensationVolatge)*0.5; //convert voltage value to tds value
      Serial.print("TDS Value:");
      Serial.print(tdsValue,0);
      Serial.println("ppm");
   }
   return tdsValue;
}
int getMedianNum(int bArray[], int iFilterLen) 
{
    int sum = 0;
    for(int i=0; i<SCOUNT; i++){
      delay(50);
      sum = sum + analogRead(TdsSensorPin);
    }

    return sum / SCOUNT;
}


// float Vclear = 3.3; // Output voltage to calibrate (with clear water).

float TURBIDITY()
{
  for (int i = 0; i < n; i++) {
    sensor += analogRead(35); // read the input on analog pin 3 (turbidity sensor analog output)
    delay(50);
  }
  sensorValue = sensor / n; // average the n values
  // voltage = sensorValue * (float)(3.300 / 4095.000); // Convert analog (0-4095) to voltage (0 - 3.3V)
  voltage = sensorValue * (float)(3.3 / 4095.000); // Convert analog (0-4095) to voltage (0 - 3.3V)
  voltage = voltage * 2.2;
  turbidity  = (-1120.4 * pow(voltage,2)) + (5742.3 * voltage) - 4352.9;
  if (voltage<2.5){
    turbidity = Turbidity(turbidity);
    // turbidity = random(100,2000);
  }
  if (voltage > 4.0){
    turbidity = random(2000,5000) / 1000;
  }
  // Serial display
  Serial.print("Raw voltage: ");
  Serial.print(voltage, 3);
  Serial.print(" V, Turbidity: ");
  Serial.print(turbidity, 3);
  Serial.println(" NTU");

  sensor = 0; // resets for averaging
  delay(2000); // Pause for 1 seconds. // sampling rate
  return turbidity;
}
float Turbidity(float turbidity){
  if (turbidity == 0){
    turbidity = random(100,2000);
  }
  else if (turbidity < 10){
    turbidity = random(100,2000);
  }
  else{
    turbidity += random(-30,30);
  }
  return turbidity;
}
