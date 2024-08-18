/*********************************************************************
This sketch is intended to run on a D1 Mini. It reads pressure 
from a transducer and sends the readings to tago.io.

This sketch uses the 1.3" OLED display, SH1106 128x64, I2C comms.

x1Val and x2Val must be calibrated to the voltage readings at
0 and 100 PSI respectively for the output to be accurate.

*********************************************************************/


#include <SPI.h>
#include <Wire.h>


/*for 1.3 display*/
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
/* Uncomment the initialize the I2C address , uncomment only one, If you get a totally blank screen try the other*/
#define i2c_Address 0x3c //initialize with the I2C addr 0x3C Typically eBay OLED's
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//This ProgramID is the name of the sketch and identifies what code is running on the D1 Mini
const char* ProgramID = "LMWA_d1_002";

//Wifi Stuff
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
//const char *ssid =	"LMWA-PumpHouse";		// cannot be longer than 32 characters!
//const char *pass =	"ds42396xcr5";		//
const char *ssid =	"WiFiFoFum";		// cannot be longer than 32 characters!
const char *pass =	"6316EarlyGlow";		//
WiFiClient client;
String wifistatustoprint;

//Tago.io server address and device token
char server[] = "api.tago.io";
int lastConnectionTime = 0;            // last time you connected to the server, in milliseconds
int currentMillis = 0;
int postingInterval = 10 * 1000; // delay between updates, in milliseconds
String Device_Token = "1c275a1c-27ee-42b0-b59b-d82e40de269d"; //d1_002_pressure_sensor Default token
String pressure_string = "";

//Data payload variables
int counter = 1;
char pressureout[32];
String pressuretoprint;


//Sensor setup & payload variables
const int SensorPin = A0;
const float alpha = 0.95; // Low Pass Filter alpha (0 - 1 ). .95 was way too slow
const float aRef = 5; // analog reference
float filteredVal = 512; // midway starting point
//float x1Val = .78; //voltage at 0 psi
//float y1Val = 0;
//float x2Val = 2.9; //voltage at 100 psi
//float y2Val = 60;
//float mVal = 0;
//float bVal = 0;
float sensorVal;
float psiVal;
float voltage;
char psichar[32];
String psistring;
float psi;
char filteredValchar[32];
String filteredValstring;
char sensorValchar[32];
String sensorValstring;

//binary sensor stuff
#define BINARYPIN 13

void setup() {


  Serial.begin(115200);
  while (!Serial) {
    ;                     // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("SETUP");
  Serial.println();

  pinMode(BINARYPIN, INPUT_PULLUP);

  //1.3" OLED Setup
  delay(250); // wait for the OLED to power up
  display.begin(i2c_Address, true); // Address 0x3C default
 //display.setContrast (0); // dim display
 
  display.display();
  delay(2000);

  // Clear the buffer.
  display.clearDisplay();

  // draw a single pixel
  display.drawPixel(10, 10, SH110X_WHITE);
  // Show the display buffer on the hardware.
  // NOTE: You _must_ call display after making any drawing commands
  // to make them visible on the display hardware!
  display.display();
  delay(2000);
  display.clearDisplay();

}

void loop() {
  currentMillis = millis();

  //Wifi Stuff
  if (WiFi.status() != WL_CONNECTED) {
    
    //Write wifi connection to display
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println("Booting Program ID:");
    display.println(ProgramID);
    display.setTextSize(1);
    display.println(" ");
    display.println("Connecting To WiFi:");
    display.println(ssid);
    display.println(" ");
    display.println("Wait for it......");
    display.display();

    //write wifi connection to serial
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.println("...");
    WiFi.setHostname(ProgramID);
    WiFi.begin(ssid, pass);

    //delay 8 seconds for effect
    delay(8000);

    if (WiFi.waitForConnectResult() != WL_CONNECTED){
      return;
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println("Water Pressure Sensor\nDevice ID: d1_002");
    display.setTextSize(1);
    display.println(" ");
    display.println("Connected To WiFi:");
    display.println(ssid);
    display.println(" ");
    display.display();

    Serial.println("\n\nWiFi Connected! ");
    printWifiStatus();

  }

  if (WiFi.status() == WL_CONNECTED) {
    wifistatustoprint="Wifi Connected!";
  }else{
    wifistatustoprint="Womp, No Wifi!";
  }

  //Initialize y=mx+b output calibration
//  mVal = (y2Val-y1Val)/(x2Val-x1Val);
//  bVal = mVal * x1Val;

  //perform reading and filter the value.
  sensorVal = (float)analogRead(SensorPin);  //raw Analog To Digital Converted (ADC) value from sensor
  filteredVal = (alpha * filteredVal) + ((1.0 - alpha) * sensorVal); // Low Pass Filter, smoothes out readings.
  voltage = (filteredVal * aRef) / 1023; //calculate voltage using smoothed sensor reading... 5.0 is system voltage, 1023 is resolution of the ADC...
//  psi = (mVal * voltage) - bVal; // calibrate output using y=mx+b
  psi = (23.608 * voltage) - 20.446; // generated by Excel Scatterplot for transducer S/N 2405202308207
  psiVal = roundoff(psi, 1); //rounded psi value
/*
  //print values to serial console for inspection
  Serial.print("mVal :"); Serial.println(mVal, 0);
  Serial.print("bVal :"); Serial.println(bVal, 0);
  Serial.print("raw_adc "); Serial.println(sensorVal, 0);
  Serial.print("smoothed_adc "); Serial.println(filteredVal, 0);
  Serial.print("voltage= "); Serial.println(voltage, 0);
  Serial.print("psi= "); Serial.println(psi, 1);
  Serial.println("  ");
*/
  //convert data to string types  
  dtostrf(sensorVal, 8, 2, sensorValchar);
  sensorValstring = "Raw: " + String (sensorValchar);
 
  dtostrf(filteredVal, 8, 2, filteredValchar);
  filteredValstring = "Filtered: " + String (filteredValchar);

  dtostrf(psi, 8, 2, psichar);
  psistring = "Voltage: " + String(voltage) + "\nPSI: " + String(psichar);
  
  // clear the display
  display.clearDisplay();

  //buffer next display payload
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("Water Pressure Sensor");
  display.print("Prog. ID: "); display.println(ProgramID);
  display.setTextSize(1);
  display.println(sensorValstring);
  display.println(filteredValstring);
  display.println(psistring);
  display.println(wifistatustoprint);
  display.print("SSID:");
  display.println(ssid);
  display.print("IP:");
  display.println(WiFi.localIP());
  // write the buffer to the display
  display.display();

  //BINARY SENSOR
  int status;
  status = digitalRead(BINARYPIN);
  Serial.print("BINARYPIN Status: ");
  Serial.println(status);
  if (status == HIGH)
  {
    Serial.println("BINARY SENSOR OPEN");
  }
  else
  {
    Serial.println("BINARY SENSOR CLOSED");
  }

  // if upload interval has passed since your last connection,
  // then connect again and send data to tago.io
/*  Serial.print("currentMillis: "); Serial.println(currentMillis, 0);
  Serial.print("lastConnectionTime: "); Serial.println(lastConnectionTime, 0);
  Serial.print("PostingInterval: "); Serial.println(postingInterval, 0);*/
  if (currentMillis - lastConnectionTime > postingInterval) {
    Serial.println("Time to post to tago.io!");
    // then, send data to Tago
    httpRequest();
  }



  counter++;
  delay (.01);
}

float roundoff(float value, unsigned char prec){
  float pow_10 = pow(10.0f, (float)prec);
  return round(value * pow_10) / pow_10;
}

// this method makes a HTTP connection to tago.io
void httpRequest() {

  Serial.println("Sending this Pressure:");
  Serial.println(psi);

    // close any connection before send a new request.
    // This will free the socket on the WiFi shield
    client.stop();

    Serial.println("Starting connection to server for Pressure...");
    // if you get a connection, report back via serial:
    String PostPressure = String("{\"variable\":\"pressure\", \"value\":") + String(psi)+ String(",\"unit\":\"PSI\"}");
    String Dev_token = String("Device-Token: ")+ String(Device_Token);
    if (client.connect(server,80)) {                      // we will use non-secured connnection (HTTP) for tests
    Serial.println("Connected to server");
    // Make a HTTP request:
    client.println("POST /data? HTTP/1.1");
    client.println("Host: api.tago.io");
    client.println("_ssl: false");                        // for non-secured connection, use this option "_ssl: false"
    client.println(Dev_token);
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(PostPressure.length());
    client.println();
    client.println(PostPressure);
    Serial.println("Pressure sent!\n");
    }  else {
      // if you couldn't make a connection:
      Serial.println("Server connection failed.");
    }

    client.stop();

    // note the time that the connection was made:
    lastConnectionTime = currentMillis;
}

//this method prints wifi network details
void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  Serial.print("Hostname: ");
  Serial.println(WiFi.getHostname());

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  Serial.println("");
}

//comment to remove