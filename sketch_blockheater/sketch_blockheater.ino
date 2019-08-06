/*
  ReadPrivateChannel

  Reads the latest voltage value from a private channel on ThingSpeak every 30 seconds
  and prints to the serial port debug window.

  For an example of how to read from a public channel, see ReadChannel example

  ThingSpeak ( https://www.thingspeak.com ) is an analytic IoT platform service that allows you to aggregate, visualize and
  analyze live data streams in the cloud.

  Copyright 2017, The MathWorks, Inc.

  Documentation for the ThingSpeak Communication Library for Arduino is in the extras/documentation folder where the library was installed.
  See the accompaning licence file for licensing information.
*/

#include "ThingSpeak.h"
#include <Wire.h>
#include "DS3231.h"

// ***********************************************************************************************************
// This example selects the correct library to use based on the board selected under the Tools menu in the IDE.
// Yun, Ethernet shield, WiFi101 shield, esp8266, ESP32 and MKR1000 are all supported.
// With Yun, the default is that you're using the Ethernet connection.
// If you're using a wi-fi 101 or ethernet shield (http://www.arduino.cc/en/Main/ArduinoWiFiShield), uncomment the corresponding line below
// ***********************************************************************************************************
//#define USE_WIFI101_SHIELD
#define USE_ETHERNET_SHIELD

#if !defined(USE_WIFI101_SHIELD) && !defined(USE_ETHERNET_SHIELD) && !defined(ARDUINO_SAMD_MKR1000) && !defined(ARDUINO_AVR_YUN) && !defined(ARDUINO_ARCH_ESP8266) && !defined(ARDUINO_ARCH_ESP32)
#error "Uncomment the #define for either USE_WIFI101_SHIELD or USE_ETHERNET_SHIELD"
#endif

#if defined(ARDUINO_AVR_YUN)
#include "YunClient.h"
YunClient client;
#else
#if defined(USE_WIFI101_SHIELD) || defined(ARDUINO_SAMD_MKR1000) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
// Use WiFi
#ifdef ARDUINO_ARCH_ESP8266
#include <ESP8266WiFi.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#else
#include <SPI.h>
#include <WiFi101.h>
#endif
char ssid[] = "<YOURNETWORK>";    //  your network SSID (name)
char pass[] = "<YOURPASSWORD>";   // your network password
int status = WL_IDLE_STATUS;
WiFiClient  client;
#elif defined(USE_ETHERNET_SHIELD)
// Use wired ethernet shield
#include <SPI.h>
#include <Ethernet.h>
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
EthernetClient client;
#endif
#endif


/*
  *****************************************************************************************
  **** Visit https://www.thingspeak.com to sign up for a free account and create
  **** a channel.  The video tutorial http://community.thingspeak.com/tutorials/thingspeak-channels/
  **** has more information. You need to change this to your channel, and your read API key
  **** IF YOU SHARE YOUR CODE WITH OTHERS, MAKE SURE YOU REMOVE YOUR READ API KEY!!
  *****************************************************************************************

  This is the ThingSpeak channel used in the write examples (31461).  It is private, and requires a
  read API key to access it.  We'll read from the first field.
*/
unsigned long myChannelNumber = 31461;
const char * myReadAPIKey = "";
const char * myWriteAPIKey = "";

RTClib RTC;

int hour_alarm, minute_alarm, hour_now, minute_now, hour_start, minute_start ;
unsigned long timestamp, delay_time = 30000;
DateTime now;
boolean heater = false;
int const TEMP = 0;
const int LED = 9;
const int MANUAL_TIMER = 8;

void setup() {
  pinMode(LED, OUTPUT);  
  pinMode(MANUAL_TIMER, OUTPUT);
  Wire.begin();

  Serial.begin(9600);
#ifdef ARDUINO_AVR_YUN
  Bridge.begin();
#else
#if defined(ARDUINO_ARCH_ESP8266) || defined(USE_WIFI101_SHIELD) || defined(ARDUINO_SAMD_MKR1000) || defined(ARDUINO_ARCH_ESP32)
  WiFi.begin(ssid, pass);
#else
  Ethernet.begin(mac);
#endif
#endif

  ThingSpeak.begin(client);
}

void loop() {

  // Read the latest value from field 1 of channel 31461
  hour_alarm = ThingSpeak.readIntField(myChannelNumber, 1, myReadAPIKey);
  minute_minute = ThingSpeak.readIntField(myChannelNumber, 2, myReadAPIKey);

  // Write the heating status
  ThingSpeak.writeField(myChannelNumber, 3, status, myWriteAPIKey);

  timestamp = millis();

  while (millis() < (timestamp + delay_time)) {
    // If there is an alarm time
    if (hour_alarm != -1) {
      
      digitalWrite(MANUAL_TIMER, HIGH);     //close manual_timer  
            
      // Get the current time
      now = RTC.now();
      hour_now = (now.hour(), DEC);
      minute_now = (now.minute(), DEC);

      // Get temperature
      int tempReading = analogRead(TEMP);
      float tempVolts = tempReading * 5.0 / 1024.0;
      float tempC = (tempVolts - 0.5) * 100.0;

      // Adjusting alarm time based on temperature
      if (tempC <= 5 && tempC > -5) {
        if (minute_alarm > 29) {
          minute_start = minute_alarm - 30;
        } else {
          hour_alarm = hour_alarm - 1;
          minute_start = 60 - (30 - minute_alarm);
        }
      } else if (tempC <= -5 && tempC > -10) {
        if (hour_alarm > 0) {
          hour_start = hour_alarm - 1;
        } else {
          hour_start = 23;
        }
      } else if (tempC <= -10) {
        if (hour_alarm > 1) {
          hour_start = hour_alarm - 1;
        } else if (hour_alarm == 1) {
          hour_start = 23;
        } else if (hour_alarm == 0) {
          hour_start = 22;
        }
      }

      // Heating
      if (hour_now <= hour_alarm && hour_now >= hour_start ) {
        if ((minute_alarm >= minute_now && minute_now <= minute_start) || (minute_alarm >= minute_now && minute_now >= minute_start)) {
               
          digitalWrite(LED, HIGH);          //Heating on             
          heater = true;                    //heating status ON
        } else {
                       
          digitalWrite(LED, LOW);           //Heating off
          heater = false;                   //heating status OFF
        }
      }

      //If there is no alarm time
    } else {
      digitalWrite(MANUAL_TIMER, HIGH);     //open manual_timer     
      digitalWrite(LED, LOW);               //heating status OFF
      heater = false;                       //heating status OFF
    }
  }
  
}
