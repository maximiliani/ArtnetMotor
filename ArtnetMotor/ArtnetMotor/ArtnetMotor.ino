//
// ArtnetMotor
//
// This project controls a motor with an ESP8266.
// Developed with [embedXcode](https://embedXcode.weebly.com)
//
// Author 		maximiliani
// 				RaHoni
//
// Date			16.04.20 14:31
// Version		<#version#>
//
// Copyright	© maximiliani, 2020
// Licence		licence
//
// See         ReadMe.txt for references
//


// Core library for code-sense - IDE-based
// !!! Help: http://bit.ly/2AdU7cu
#if defined(ENERGIA) // LaunchPad specific
#include "Energia.h"
#elif defined(TEENSYDUINO) // Teensy specific
#include "Arduino.h"
#elif defined(ESP8266) // ESP8266 specific
#include "Arduino.h"
#elif defined(ARDUINO) // Arduino 1.8 specific
#include "Arduino.h"
#else // error
#error Platform not defined
#endif // end IDE

//*
 * This is a example for a Artnet-based drape control system with an ESP 8266.
 * Developed by: RaHoni and maximiliani
 * Version: 1.0.0
 * Date: 03-03-2020
 */

// First we have to include the necessary libraries to use Artnet, SPIFFS, WIFI and the webserver.
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <Artnet.h>
#include "FS.h"

/* Here are variables you have to change, if you choosed another wiring.
 * relaisAuf is the port for the relay, which opens the drape.
 * relaisZU is the port for the relay, which closes the drape.
 * timeForAction is the time the drape needs to open/close.
 */
#define relaisAuf 12
#define relaisZu 14
#define timeForAction 13

// This are the WiFi-Settings
#ifndef STASSID
#define STASSID "SSID"
#define STAPSK  "Password"
#endif
const char* ssid = STASSID;
const char* password = STAPSK;

// This creates objects from the libraries to use them.
ESP8266WebServer server(80);
WiFiUDP UdpSend;
ArtnetReceiver artnet;

// This is the basic web config.
const char* www_username = "admin"; // Username for the webpage
const char* www_password = "Mauritz2020!"; // Password for the webpage
const char* www_realm = "Custom Auth Realm";
const char* authFailResponse = "Authentication Failed";

// These are some variables, which you can change with Artnet or the web interface.
uint32_t universe = 16;
uint32_t address = 500;
int actStatus;
int target;
int lastVal;

// This methods reacts on a change request by a POST on the weburl http://<IP>/change.
void change(){
  Serial.println("Enter Change");

  // This opens SPIFFS to save the changed data permanently.
  SPIFFS.begin();
  File a = SPIFFS.open("/address.txt", "w+");
  File u = SPIFFS.open("/universe.txt", "w+");
  if (!a || !u) Serial.println("No Data file!");
  String header, content;
  bool hasCont = false;

  // This saves the address and universe if the user entered a value to the web interface and give a feedback to the user.
  if (server.hasArg("ADDRESS") && server.hasArg("UNIVERSE")){
    hasCont = true;
    address = String(server.arg("ADDRESS")).toInt();
    universe = String(server.arg("UNIVERSE")).toInt();
    content = "<html><body><h2>Successful</h2><br>";
    File c = SPIFFS.open("/data.txt", "w+");
    c.close();
    Serial.println(address);
    Serial.println(universe);
    a.print(String(address));
    u.print(String(universe));
    a.close();
    u.close();
  }else content = "<html><body><h2>Error</h2><br></body></html>";
  server.send(200, "text/html", content);
  SPIFFS.end();
}

// This is the setup, which runs at every reboot.
void setup() {
  Serial.begin(115200);
  SPIFFS.begin();
  pinMode(relaisAuf,OUTPUT);
  pinMode(relaisZu,OUTPUT);
  pinMode(BUILTIN_LED, OUTPUT);

  // This connects to WiFi.
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { Serial.print("."); delay(500);}
  //ArduinoOTA.begin();
  artnet.begin();

  // This reads the saved data from SPIFFS if they exist. This is necessary, because some idiots pull plugs ...
  if(SPIFFS.exists("/data.txt")){
    Serial.println("Data found! Reading last data.");
    File a = SPIFFS.open("/address.txt", "r");
    File u = SPIFFS.open("/universe.txt", "r");
    universe = int(u.parseInt());
    address = int(a.parseInt());
    Serial.println(www_username);
    Serial.println(www_password);
    delay(100);
    a.close();
    u.close();
  }
  SPIFFS.end();

  artnet.subscribe(universe, [&](uint8_t* data, uint16_t size)
  {
    if(actStatus >= 255) actStatus = 255;
    else if(actStatus <=0) actStatus = 0;
    Serial.println(data[address-1]);
    target = int(data[address-1]);
  });
  
  // This configures the webserver actions.
  server.on ("/change", change);
  server.on("/", []() {
    Serial.println(www_username);
    if (!server.authenticate(www_username, www_password)) return server.requestAuthentication(DIGEST_AUTH, www_realm, authFailResponse);
    //if (!server.authenticate("admin", "password")) return server.requestAuthentication(DIGEST_AUTH, www_realm, authFailResponse);
    String content = "<html><body><form action='/change' method='POST'><h2>Hello, now you can change the values!</h2><br>";
    content += "Universe:<input type='text' name='UNIVERSE' placeholder='"+ String(universe) +"'><br>";
    content += "Address:<input type='text' name='ADDRESS' placeholder='"+ String(address) +"'><br>";
    content += "<input type='submit' name='SUBMIT' value='Submit'></form><br></body></html>";
    server.send(200, "text/html", content);
  });
  server.begin();
  Serial.print("Open http://");
  Serial.print(WiFi.localIP());
  Serial.println("/ in your browser to see it working");
  digitalWrite(BUILTIN_LED, LOW);
}

// This is the loop, which is runned in a loop while the chip has power.
void loop() {
  digitalWrite(BUILTIN_LED, LOW);
  artnet.parse(); // check if artnet packet has come and execute callback
  server.handleClient();  // This handles the webserver.
  if(actStatus != target){
    if(actStatus > target){
      digitalWrite(relaisZu, LOW);
      digitalWrite(relaisAuf, HIGH);
      delay(50);
      digitalWrite(relaisAuf, LOW);
      Serial.println(target + " " + actStatus);
      actStatus --;
    }else {
      digitalWrite(relaisAuf, LOW);
      digitalWrite(relaisZu, HIGH);
      delay(50);
      digitalWrite(relaisZu, LOW);
      Serial.println(target + " " + actStatus);
      actStatus ++;
    }
  }
}
