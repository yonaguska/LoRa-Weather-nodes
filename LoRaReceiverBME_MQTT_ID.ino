/*
   Receiver which receives messages through a Hope RFM95 LoRa
   Module.
   It sends a feedback signal (checksum) to the receiver

   Copyright <2017> <Andreas Spiess>

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.

   Based on the Example of the library
*/

// USE ME

#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>

#define ESP8266  // comment if you want to use the sketch on a Arduino board
#define MQTT     // uncomment if you want to foreward the message via MQTT
#define OLED        // comment if you do not have a OLED display

//const long freq = 868E6;
const long freq = 915E6;
const int SF = 9;
const long bw = 125E3;

#ifdef OLED
#include "SSD1306.h"
SSD1306  display(0x3d, 4, 5);
#endif

#ifdef MQTT
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
//#include <credentials.h>

IPAddress server(192, 168, 1, 169); // My Mac
#define TOPIC "sensor"
#endif

// MQTT broker
const char* mqtt_server = "192.168.1.169"; // My Mac

int counter, lastCounter;

// my global variables
char* myIP;
char sensor_ip[30] = "";
char sensor_id[35] = "";

// JSON buffer
StaticJsonBuffer<200> jsonBuffer;
boolean goodParse;
const char* thisID;

// my functions
// convert our IP to a char array
char* ip2CharArray(IPAddress ip) {
    static char a[16];
    sprintf(a, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return a;
}

#ifdef MQTT
WiFiClient wifiClient;
PubSubClient client(wifiClient);
#endif

void setup() {
    Serial.begin(115200);  // Matched to Wemos D1 mini
    Serial.println("=========================== ==========================");
    Serial.println("=========================== ==========================");
    Serial.println("LoRa Receiver");
#ifdef ESP8266
    LoRa.setPins(16, 17, 15); // set CS, reset, IRQ pin
#else
    LoRa.setPins(10, A0, 2); // set CS, reset, IRQ pin
#endif
#ifdef OLED
    display.init();
    display.flipScreenVertically();
    display.setFont(ArialMT_Plain_10);

    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 10, "Mailbox");
    display.display();
#endif

    if (!LoRa.begin(freq)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }

    LoRa.setSpreadingFactor(SF);
    // LoRa.setSignalBandwidth(bw);
    Serial.println("LoRa Started");

#ifdef MQTT
    WiFi.begin("Gitli", "Barabajagal1950");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");

    // Printing the ESP IP address
    Serial.print("WiFi connected as: ");
    Serial.println(WiFi.localIP());
    myIP = ip2CharArray(WiFi.localIP());

    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

    if (client.connect("Mailbox", "admin", "admin")) {
        client.publish(TOPIC"/STAT", "Mailbox live");
        // client.subscribe("inTopic");
    }
    Serial.print("MQTT Started, connected to: ");
    Serial.println(mqtt_server);
#endif

    Serial.print("Frequency ");
    Serial.print(freq);
    Serial.print(" Bandwidth ");
    Serial.print(bw);
    Serial.print(" SF ");
    Serial.println(SF);

    sprintf(sensor_ip,"%s/%s", TOPIC, myIP);
    Serial.print("sensor_ip: ");
    Serial.println(sensor_ip);
}

void loop() {
    // try to parse packet
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        // received a packet
        String message = "";
        while (LoRa.available()) {
            message = message + ((char)LoRa.read());
        }
        String rssi = "\"RSSI\":\"" + String(LoRa.packetRssi()) + "\"";
        String jsonString = message;
        jsonString.replace("xxx", rssi);

        int ii = jsonString.indexOf("Count", 1);
        String count = jsonString.substring(ii + 8, ii + 11);
        counter = count.toInt();
        if (counter - lastCounter == 0) Serial.println("Repetition");
        lastCounter = counter;

        sendAck(message);
        String value1 = jsonString.substring(8, 11);  // Vcc or height
        String value2 = jsonString.substring(23, 26); // counter
        parseMyJSON(jsonString);
        Serial.print("loop: thisID => "); Serial.println(thisID);
        sprintf(sensor_id,"%s.%s", sensor_ip, thisID);
        //Serial.print("loop: sensor_id => "); Serial.println(sensor_id);
#ifdef SMURF // was OLED
        //display.clear();
        display.setTextAlignment(TEXT_ALIGN_RIGHT);
        displayText(40, 0, value1, 3);
        display.setTextAlignment(TEXT_ALIGN_RIGHT);
        displayText(120, 0, String(LoRa.packetRssi()), 3);
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        displayText(60, 35, count, 3);
        display.display();
#endif

#ifdef MQTT
        if (!client.connected()) {
            reconnect();
        }
        //client.publish(TOPIC"/MESSAGE", jsonString.c_str());
        //client.publish(sensor_ip, jsonString.c_str());
        client.publish(sensor_id, jsonString.c_str());
#endif
    }
#ifdef MQTT
    client.loop();
#endif
} // end of loop

void parseMyJSON(String jsonString) {
    Serial.print("parseMyJSON: jsonString => "); Serial.println(jsonString);
    jsonBuffer.clear();
    JsonObject& root = jsonBuffer.parseObject(jsonString);
    if(!root.success()) {
        Serial.println("parseObject() failed");
        goodParse = false;
    } else {
        goodParse = true;
    }
    thisID = root["ID"];
    //Serial.print("             thisID => "); Serial.println(thisID);
}

#ifdef MQTT
void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection to server @ ");
        Serial.print(mqtt_server);
        Serial.print("...");
        // Attempt to connect
        if (client.connect("Mailbox")) {
            Serial.println("connected");
            // Once connected, publish an announcement...
            client.publish(TOPIC"/sensor", "I am alive again");
            // ... and resubscribe
            //  client.subscribe("inTopic");
        } else {
            Serial.print("failed, rc = ");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}
#endif

void sendAck(String message) {
    int check = 0;
    for (int i = 0; i < message.length(); i++) {
        check += message[i];
    }
    // Serial.print("/// ");
    LoRa.beginPacket();
    LoRa.print(String(check));
    LoRa.endPacket();
    Serial.print(message);
    Serial.print(" ");
    Serial.print("Ack Sent: ");
    Serial.print(check);
    Serial.print("  sendAck: >>>> Publishing to MQTT at "); Serial.print(mqtt_server); Serial.print(" from "); Serial.println(myIP);

}

#ifdef OLED
void displayText(int x, int y, String tex, byte font ) {
    switch (font) {
        case 1:
               display.setFont(ArialMT_Plain_10);
               break;
        case 2:
               display.setFont(ArialMT_Plain_16);
               break;
        case 3:
               display.setFont(ArialMT_Plain_24);
               break;
        default:
               display.setFont(ArialMT_Plain_16);
               break;
    }
    display.drawString(x, y, tex);
}
#endif

