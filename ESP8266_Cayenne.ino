// This example shows how to connect to Cayenne using an ESP8266 and send/receive sample data.
// Make sure you install the ESP8266 Board Package via the Arduino IDE Board Manager and select the correct ESP8266 board before compiling. 

//#define CAYENNE_DEBUG
#define CAYENNE_PRINT Serial
#include <CayenneMQTTESP8266.h>
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>

bool debug = false; // if (debug) { 
bool debug2 = false; // if (debug2) { 

// Stuff for LoRa setup
//const long freq = 868E6;
const long freq = 915E6;
const int SF = 9;
const long bw = 125E3;
int counter, lastCounter;
IPAddress local_ip = WiFi.localIP();
float myTemp = 10.0;

// WiFi network info.
char ssid[] = "Gitli";
char wifiPassword[] = "Barabajagal1950";

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = "ba447b90-4584-11e8-9c3e-8331599aadae";
char password[] = "08f17aa3674d5f6d36026ca0f3bb7e63e61b29fe";
char clientID[] = "b7af3fd0-4586-11e8-b4ef-898f2f5b9050";

unsigned long lastMillis = 0;

void sendAck(String message) {
    int check = 0;
    for (int i = 0; i < message.length(); i++) {
        check += message[i];
    }
    if (debug) { Serial.print("  sendAck: LoRa send"); }
    LoRa.beginPacket();
    LoRa.print(String(check));
    LoRa.endPacket();
    if (debug) { 
        Serial.print("  sendAck: message => ");
        Serial.print(message);
        Serial.print(" ");
        Serial.print("Ack Sent: ");
        Serial.println(check);
    }
}

void setup() {
    
	Serial.begin(115200);
    Serial.println("");
    Serial.println("=========================== ==========================");
    Serial.println("=========================== ==========================");

    // Connect to WiFi, then connect to Cayenne
    Serial.println("Initiating connection to Cayenne MQTT");
	Cayenne.begin(username, password, clientID, ssid, wifiPassword);

    // Set up LoRa
    LoRa.setPins(16, 17, 15); // set CS, reset, IRQ pin
    Serial.println("Initiating LoRa receiver");
    if (!LoRa.begin(freq)) {
        Serial.println("  Starting LoRa failed!");
        while (1);
    }
    // Configuring LoRa
    LoRa.setSpreadingFactor(SF);
    // LoRa.setSignalBandwidth(bw);
    Serial.print("LoRa Started: ");
    Serial.print("Frequency ");
    Serial.print(freq);
    Serial.print(" Bandwidth ");
    Serial.print(bw);
    Serial.print(" SF ");
    Serial.println(SF);
    Serial.println("");
    
}

void loop() {

    StaticJsonBuffer<200> jsonBuffer;

	Cayenne.loop();
    if (debug2) { Serial.println("In main loop, past cayenne loop"); }

    // try to parse packet
    int packetSize = LoRa.parsePacket();
    if (packetSize > 20) {
        if (debug) { Serial.print("Trying to preocess non-zero LoRa packet: "); Serial.println(packetSize); }
        // received a packet
        String message = "";
        while (LoRa.available()) {
            message = message + ((char)LoRa.read());
        }
        Serial.print("  message => "); Serial.println(message);
        String rssi = "\"RSSI\":\"" + String(LoRa.packetRssi()) + "\"";
        String jsonString = message;
        jsonString.replace("xxx", rssi);
        Serial.print("  Signal strength: "); Serial.println(rssi);

        int ii = jsonString.indexOf("Count", 1);
        String count = jsonString.substring(ii + 8, ii + 11);
        counter = count.toInt();
        if (debug) { if (counter - lastCounter == 0) Serial.println("  Repetition"); }
        lastCounter = counter;

        sendAck(message);
        if (debug) { Serial.print("  jsonString => "); Serial.println(jsonString); }
        if (debug2) { Serial.print("  substring 2,4 => "); Serial.println(jsonString.substring(2,4)); }
        if (jsonString.substring(2, 4) == "ID") { // starts as {"ID", but we only want to check for ID
            if (debug2) { Serial.println("    preamble was ID"); }
            JsonObject& root = jsonBuffer.parseObject(jsonString);
            if(!root.success()) {
                Serial.println("parseObject() failed\n");
                return;
            } else { 
                if (debug2) { Serial.println("    JSON parsing successful"); }
           }
           // parse key and value pairs using C++11 syntax (preferred):
           String idVal = "";  // string for current ID
           char idChar[5];
           String thisKey;
           String thisValue;
           float floatValue;
           char charKey[20];
           char charValue[10];
           int channelID = 0;
           for (auto kv : root) {
               thisKey = kv.key;
               thisValue = kv.value.as<char*>();
               if (debug) { Serial.print("      "); Serial.print(thisKey); Serial.print("  =>  "); Serial.println(thisValue); }
               if (thisKey == "ID") {
                   idVal = thisValue;
                   idVal.toCharArray(idChar,5);
                   if (debug) { Serial.print("      our ID is "); Serial.println(idChar); }
               } else {
                   thisKey = kv.key;
                   thisKey.toCharArray(charKey, 10);
                   thisValue = kv.value.as<char*>();
                   thisValue.toCharArray(charValue, 10);
                   floatValue = atof(charValue);
                   
                   if (debug2) { Serial.println("      Publishing to Cayenne "); }
                   if (thisKey == "analogpin") {
                        channelID = atoi(idChar)*10 + 1;
                        if (debug) { Serial.print("      writing voltage "); Serial.print(floatValue); Serial.print(" to channel "); Serial.println(channelID); }
                        Cayenne.virtualWrite(channelID, floatValue, "voltage", "v");
                   } else if (thisKey == "temperature") {
                        channelID = atoi(idChar)*10 + 2;
                        if (debug) { Serial.print("      writing temperature "); Serial.print(floatValue); Serial.print(" to channel "); Serial.println(channelID); }
                        Cayenne.virtualWrite(channelID, floatValue, "temp", "f");
                   } else if (thisKey == "humidity") {
                        channelID = atoi(idChar)*10 + 3;
                        if (debug) { Serial.print("      writing humidity "); Serial.print(floatValue); Serial.print(" to channel "); Serial.println(channelID); }
                        Cayenne.virtualWrite(channelID, floatValue, "hvac_hum", "p");
                   } else if (thisKey == "rawpressure") {
                        channelID = atoi(idChar)*10 + 4;
                        if (debug) { Serial.print("      writing rawpressure "); Serial.print(floatValue); Serial.print(" to channel "); Serial.println(channelID); }
                        Cayenne.virtualWrite(channelID, floatValue, "press", "hpa");
                   } else if (thisKey == "pressure") {
                        channelID = atoi(idChar)*10 + 5;
                        if (debug) { Serial.print("      writing pressure "); Serial.print(floatValue); Serial.print(" to channel "); Serial.println(channelID); }
                        Cayenne.virtualWrite(channelID, floatValue, "press", "bar");
                   } else if (thisKey == "dewpoint") {
                        channelID = atoi(idChar)*10 + 6;
                        if (debug) { Serial.print("      writing dewpoint "); Serial.print(floatValue); Serial.print(" to channel "); Serial.println(channelID); }
                        Cayenne.virtualWrite(channelID, floatValue, "temp", "f");
                   } else {
                        Serial.print("      ERROR: unknown sensor stream ==> "); Serial.println(thisKey);
                   }
               }

           } // for loop processing JSON key/values
           
        } else {
            Serial.println("    preamble was not ID\n");
            return;
        }


        if (debug) { Serial.println("End packet processing\n"); }

    } // fi
    
}

// Default function for sending sensor data at intervals to Cayenne.
// You can also use functions for specific channels, e.g CAYENNE_OUT(1) for sending channel 1 data.
CAYENNE_OUT_DEFAULT()
{
    if (debug2) { Serial.print("CAYENNE_OUT_DEFAULT ..."); Serial.println(lastMillis); }
    myTemp += 0.1;
	// Write data to Cayenne here. This example just sends the current uptime in milliseconds on virtual channel 0.
	Cayenne.virtualWrite(0, millis());
	// Some examples of other functions you can use to send data.
	//Cayenne.celsiusWrite(1, myTemp);
	//Cayenne.luxWrite(2, 700);
	//Cayenne.virtualWrite(3, 50, TYPE_PROXIMITY, UNIT_CENTIMETER);
}

// Default function for processing actuator commands from the Cayenne Dashboard.
// You can also use functions for specific channels, e.g CAYENNE_IN(1) for channel 1 commands.
CAYENNE_IN_DEFAULT()
{
	Serial.print("CAYENNE_IN_DEFAULT ..."); Serial.println(lastMillis);
	CAYENNE_LOG("Channel %u, value %s", request.channel, getValue.asString());
	//Process message here. If there is an error set an error message using getValue.setError(), e.g getValue.setError("Error message");
}


