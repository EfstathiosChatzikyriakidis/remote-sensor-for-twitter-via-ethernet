/*
 *  Remote Sensor For Twitter Via Ethernet, DHCP, DNS And Button.
 *
 *  Copyright (C) 2010 Efstathios Chatzikyriakidis (stathis.chatzikyriakidis@gmail.com)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// include related libraries for ethernet, networking and twitter.
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetDHCP.h>
#include <EthernetDNS.h>
#include <Twitter.h>

// function prototypes.
const char * const buildTwitterMsg (const long sensorValue, const char * const msgFormat);
const long getSensorValue (const int pin, const int N, const long time);
const char * const ipV4ToDottedStyle (const uint8_t * const ipAddr);
void blinkLed (const int ledPin, const int times, const long time);
const boolean sendTwitterMsg (const char * const twitterMsg);

// sketch global variables.
const int buttonPin = 2;   // the pin number of the button element.
const int sensorPin = 0;   // the pin number for the input sensor.
const int ledGreenPin = 4; // the pin number of the green led.
const int ledRedPin = 7;   // the pin number of the red led.

const int SAMPLES_LENGTH = 30; // number of sensor samples.
const long SAMPLE_DELAY = 20;  // delay time (ms) for next sensor sample.

const int BLINK_TIMES = 10; // number of times a led will blink.
const long LED_DELAY = 30;  // delay time (ms) for a led to wait in light or in dark.

const long BOUNCE_DURATION = 200; // define a bounce time (ms) for the button.
long bounceTime = 0;              // variable to hold ms count to debounce a pressed switch.

// the format of the twitter message (end always with '\0' null-character).
const char * const twitterMsgFormat = "Sensor Value: %ld.\0";

// enable this if you want to get messages on serial line.
const boolean debugMode = true;

// mac address for the arduino ethernet shield.
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// twitter communication object (pass the account token).
Twitter twitter("INSERT-ACCOUNT-TOKEN-HERE");

// startup point entry (runs once).
void setup()
{
  // set button and sensor as inputs.
  pinMode(buttonPin, INPUT);
  pinMode(sensorPin, INPUT);

  // set leds as outputs.
  pinMode(ledGreenPin, OUTPUT);
  pinMode(ledRedPin, OUTPUT);

  // code for debug.
  if(debugMode) {
    // start the serial line to print verbose messages.
    Serial.begin(9600);
    Serial.println("Attempting to obtain a DHCP lease ...");
  }

  // initiate a DHCP session. the argument is the MAC (hardware) address that
  // you want your Ethernet shield to use. this call will block until a DHCP
  // lease has been obtained. the request will be periodically resent until
  // a lease is granted, but if there is no DHCP server on the network or if
  // the server fails to respond, this call will block forever.
  EthernetDHCP.begin(mac);

  // code for debug.
  if(debugMode) {
    // both collect and show network connection
    // information (if there is an DHCP lease).

    const byte * ipAddr = EthernetDHCP.ipAddress();
    const byte * gatewayAddr = EthernetDHCP.gatewayIpAddress();
    const byte * dnsAddr = EthernetDHCP.dnsIpAddress();

    Serial.println("A DHCP lease has been obtained.");
    Serial.print("My IP address is: ");
    Serial.println(ipV4ToDottedStyle(ipAddr));
    Serial.print("Gateway IP address is: ");
    Serial.println(ipV4ToDottedStyle(gatewayAddr));
    Serial.print("DNS IP address is: ");
    Serial.println(ipV4ToDottedStyle(dnsAddr));
  }
}

// loop the main sketch.
void loop()
{
  // you should periodically call this method in your loop(): it will allow
  // the DHCP library to maintain your DHCP lease, which means that it will
  // periodically renew the lease and rebind if the lease cannot be renewed.
  EthernetDHCP.maintain();

  // dark both leds (green, red).
  digitalWrite(ledRedPin, LOW);
  digitalWrite(ledGreenPin, LOW);

  // if the button is pressed down.
  if (digitalRead(buttonPin) == HIGH) {
    // ignore presses intervals less than the bounce time.
    if (abs(millis() - bounceTime) > BOUNCE_DURATION) {

      // light the red led.
      digitalWrite(ledRedPin, HIGH);

      // build the message and try to send it to twitter.
      if (sendTwitterMsg(
            buildTwitterMsg(
              getSensorValue(sensorPin, SAMPLES_LENGTH, SAMPLE_DELAY), twitterMsgFormat))) {
        // if the message was sent, dark the red led and blink the green.
        digitalWrite(ledRedPin, LOW);
        blinkLed(ledGreenPin, BLINK_TIMES, LED_DELAY);
      } else {
        // if the message was not sent blink the red led.
        blinkLed(ledRedPin, BLINK_TIMES, LED_DELAY);
      }

      // set whatever bounce time in ms is appropriate.
      bounceTime = millis(); 
    }
  }
}

// just a utility function to nicely format an IPV4 address in dotted-style.
const char * const ipV4ToDottedStyle(const uint8_t * const ipAddr)
{
  // length of characters for a dotted-style IPV4 address.
  static const int IPV4_DOTTED_LENGTH = 16;

  // the IPV4 address in dotted style.
  static char buf[IPV4_DOTTED_LENGTH];

  // format the IPV4 and store it to the buffer.
  sprintf(buf, "%d.%d.%d.%d\0", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);

  // return IPV4 address in dotted-style.
  return buf;
}

// build and return a formatted twitter message with a variable sensor value.
const char * const buildTwitterMsg(const long sensorValue, const char * const msgFormat)
{
  static const int TWITTER_MSG_LENGTH = 256;  // the maximum size of the twitter message.
  static char twitterMsg[TWITTER_MSG_LENGTH]; // the message that arduino sends to twitter.

  // build the twitter message.
  sprintf(twitterMsg, msgFormat, sensorValue);

  // return the twitter message.
  return twitterMsg;
}

// blink the led pin 'ledPin' 'times' times with 'time' delay time.
void blinkLed(const int ledPin, const int times, const long time)
{
  // blink the led given.
  for (int i = 0; i < times; i++) {
    // light the led and wait.
    digitalWrite(ledPin, HIGH);
    delay(time);

    // dark the led and wait.
    digitalWrite(ledPin, LOW);
    delay(time);
  }
}

// get an average value from the input sensor.
const long getSensorValue(const int pin, const int N, const long time)
{
  static int curSample; // current sensor sample.
  static long curValue; // current sensor value.

  // current value variable first works as a sum counter.
  curValue = 0;

  // get sensor samples first with delay to calculate the sum of them.
  for (int i = 0; i < N; i++) {
    // get sensor sample.
    curSample = analogRead(pin);

    // add sample to the sum counter.
    curValue += curSample;
    
    // delay some time for the next sample.
    delay(time);
  }  

  // get the average sensor value.
  return (curValue / N);
}

// try to send a message to twitter.
const boolean sendTwitterMsg(const char * const twitterMsg)
{
  // code for debug.
  if(debugMode) {
    Serial.println(twitterMsg);
    Serial.println("Try connecting to Twitter...");
  }

  // try to send the message to twitter.
  if (twitter.post(twitterMsg)) {
    // wait until an answer returns from twitter.
    int status = twitter.wait();

    // if the message was sent without problem.
    if (status == 200) {
      // code for debug.
      if(debugMode)
        Serial.println("Message was sent.");

      // the message was sent.
      return true;
    }
    else {
      // code for debug.
      if(debugMode) {
        Serial.print("Message was not sent (failed code: ");
        Serial.print(status);
        Serial.println(").");
      }

      // the message was not sent.
      return false;
    }
  } 
  else {
    // code for debug.
    if(debugMode)
      Serial.println("Twitter connection failed.");

    // the message was not sent.
    return false;
  }
}
