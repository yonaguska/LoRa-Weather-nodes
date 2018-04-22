/*
 * LoRa Sender which transmits the altitude measured by a BMP085 via the Hope RFM95 LoRa
 * Module. 
 * It expects a feedback signal from the receiver containing a checksum.
 * 
 * Copyright <2017> <Andreas Spiess>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.
 * 
 * Based on the Example of the library
 
 Extended to use BME280 and to return floating point values, as necessary
 Extended to include an ID read from DIP switch
 Changed counter and messageLostCounter from int to long
 
*/



// #define ESP8266 // Uncomment if you want to use the sketch for the Wemos shield
// #define BMP     // Uncomment if you want to use a BMP085 sensor
#define BME      // Uncomment if you want to use BME280 sensor
// #define INTER  //uncomment if you want to use interrupt 1 insteaad of a timer interrupt

#include <SPI.h>
#include <LoRa.h>

//#include <Adafruit_BMP085.h>  // Uncomment if you want to use a BMP085 sensor
#include <Adafruit_Sensor.h>  // Uncomment if you want to use BME280 sensor
#include <Adafruit_BME280.h>  // Uncomment if you want to use BME280 sensor

#ifndef ESP8266
#include <LowPower.h>
#endif

//const long freq = 868E6;  // for Europe?
const long freq = 915E6;  // for USA
const int SF = 9;
const long bw = 125E3;
long zeroAltitude;

long counter = 1L, messageLostCounter = 0L;
//#define BMP
// #define INTER

#ifdef BMP
Adafruit_BMP085 bmp;
#endif
#ifdef BME
Adafruit_BME280 bme; // I2C
#endif

// My known globals
float h, t, dp, p, pin;
double vcc;
char humidityString[6];
char temperatureFString[6];
char dpString[6];
char pressureString[7];
char pressureInchString[6];
char *retString = "";
char message[90];
char localMsg[90];
char convertBuff[40];
char messageAndID[120];

int blueLED = 5;
int greenLED = 6;
int redLED = 9;
int ledOFF = 255;
int ledON = 0;

const double highGuard = 3.5;
const double lowGuard = 3.25;

boolean sw0 = false;
boolean sw1 = false;
boolean sw2 = false;
boolean sw3 = false;
int id;


void setup() {
    Serial.begin(57600);  // Arduino pro mini can't go any faster than this
    while (!Serial);

    // define A1, A2 and A3 as inputs; we'll use them to read a DIP switch for an ID for the unit
    pinMode(A1, INPUT);
    digitalWrite(A1, INPUT_PULLUP);  // set pullup on analog pin 1
    pinMode(A2, INPUT);
    digitalWrite(A2, INPUT_PULLUP);  // set pullup on analog pin 2
    pinMode(A3, INPUT);
    digitalWrite(A3, INPUT_PULLUP);  // set pullup on analog pin 3
    // and define 4 as an input for the ID
    pinMode(4, INPUT);
    digitalWrite(4, INPUT_PULLUP); // set pullup on digital pin 4
    readID();  // read the DIP switch

#ifdef ESP8266
    LoRa.setPins(16, 17, 15); // set CS, reset, IRQ pin
#else
    LoRa.setPins(10, A0, 2); // set CS, reset, IRQ pin
#endif

    Serial.println("LoRa Sender");

    if (!LoRa.begin(freq)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }
    LoRa.setSpreadingFactor(SF);
    //  LoRa.setSignalBandwidth(bw);

#ifdef BMP
    if (!bmp.begin()) {
        Serial.println("Could not find a valid BMP085 sensor, check wiring!");
        while (1) {}
    }
    Serial.print("Pressure = ");
    zeroAltitude = bmp.readPressure();
    Serial.println(zeroAltitude);
    Serial.println(" Pa");
#endif
#ifdef BME
    //BME init
    Serial.println(F("BME280 test"));
    if (!bme.begin()) {
        Serial.println("Could not find a valid BME280 sensor, check wiring or power cycle!");
        while (1); {}
    }
#endif

#ifdef INTER
    pinMode(3, INPUT);
    Serial.println("Interrupt driven");
#endif

    Serial.print("Frequency ");
    Serial.print(freq);
    Serial.print(" Bandwidth ");
    Serial.print(bw);
    Serial.print(" SF ");
    Serial.println(SF);

    // cycle the RGB LED to show it's operable
    pulseLED(greenLED, ledON, 200);
    delay(100); pulseLED(blueLED, ledON, 200);
    delay(100); pulseLED(redLED, ledON, 200);

} // end of setup

void loop() {

    readID(); // read the DIP switch

#ifdef INTER
    attachInterrupt(1, wakeUp, RISING);
#endif

#ifdef BMP
    //delay(100);
    int alti = (int)bmp.readAltitude(zeroAltitude);
    sprintf(message, "{\"Alt\":\"%3d\",\"Count\":\"%03d\",\"Lost\":\"%3d\",BMP}", alti, counter, messageLostCounter);
    sendLoRaMessage();
#endif

#ifdef BME
    //delay(100);
    // read our local Vcc
    vcc = (double)(readVcc()/10)/100.0;
    //retString = ftoa(convertBuff, (double)(readVcc()/10)/100.0, 3);
    retString = ftoa(convertBuff, vcc, 3);
    sprintf(message, "{\"ID\":\"%02d\",\"analogpin\":%s}", id, retString);
    sendLoRaMessage();
    pinMode(blueLED, OUTPUT); analogWrite(blueLED, ledOFF);
    pinMode(greenLED, OUTPUT);  analogWrite(greenLED, ledOFF);
    pinMode(redLED, OUTPUT); analogWrite(redLED, ledOFF);
    if (vcc >= highGuard) {
        analogWrite(greenLED, ledON); delay(100); analogWrite(greenLED, ledOFF); // GREEN
    }
    if (vcc < highGuard && vcc >= lowGuard) {
        analogWrite(greenLED, ledON); analogWrite(redLED, ledON); delay(100); analogWrite(greenLED, ledOFF); analogWrite(redLED, ledOFF); // YELLOW
    }
    if (vcc < lowGuard) {
        analogWrite(redLED, ledON); delay(100); analogWrite(redLED, ledOFF); // RED
    }
    
    // read our sensor
    readBME();
    
    retString = ftoa(convertBuff, (double)t, 3); // TEMPERATURE
    sprintf(message, "{\"ID\":\"%02d\",\"temperature\":%s}", id, retString);
    sendLoRaMessage();
    
    retString = ftoa(convertBuff, (double)h, 3); // HUMIDITY
    sprintf(message, "{\"ID\":\"%02d\",\"humidity\":%s}", id, retString);
    sendLoRaMessage();
    
    retString = ftoa(convertBuff, (double)p, 3); // PRESSURE
    sprintf(message, "{\"ID\":\"%02d\",\"rawpressure\":%s}", id, retString);
    sendLoRaMessage();
    
    retString = ftoa(convertBuff, (double)pin, 3); // PRESSURE IN INCHES
    sprintf(message, "{\"ID\":\"%02d\",\"pressure\":%s}", id, retString);
    sendLoRaMessage();
    
    retString = ftoa(convertBuff, (double)dp, 3); // DEW POINT
    sprintf(message, "{\"ID\":\"%02d\",\"dewpoint\":%s}", id, retString);
    sendLoRaMessage();

    //sprintf(message, "{\"ID\":\"%02d\",\"ID\":%d}", id, id);  // ID
    //sendLoRaMessage();

    sprintf(message, "{\"ID\":\"%02d\",\"Count\":\"%03d\",\"Lost\":\"%03d\"}", id, counter, messageLostCounter); // Lost messages?
    sendLoRaMessage();
#endif

#if !defined (BMP) && !defined (BME)
    sprintf(message, "{\"Vcc\":\"%s\",\"Count\":\"%03d\",\"Lost\":\"%03d\",no BMP}", retString, counter, messageLostCounter);
    sendLoRaMessage();
#endif
 
#ifdef ESP8266
    delay(8000);
#else
    Serial.println("Falling asleep");  // for Arduino pro mini
    delay(100);
#ifdef INTER
        LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);  // use interrupt
        detachInterrupt(1);
#else
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);       // use sleep
        delay(10000);
#endif
#endif
} // end of loop

void pulseLED(int pin, int value, int duration) {
    //return;
    // set up LED pins
    pinMode(blueLED, OUTPUT); analogWrite(blueLED, ledOFF);
    pinMode(greenLED, OUTPUT);  analogWrite(greenLED, ledOFF);
    pinMode(redLED, OUTPUT); analogWrite(redLED, ledOFF);
    analogWrite(pin, value); delay(duration); analogWrite(pin, ledOFF);
}

void readID() {
    //return;
    // at this point, without a DIP switch connected, A1, A2 and A3 will read HIGH
    if (digitalRead(A1)) {
        sw0 = true;
    } else {
        sw0 = false;
    }
    if (digitalRead(A2)) {
        sw1 = true;
    } else {
        sw1 = false;
    }
    if (digitalRead(A3)) {
        sw2 = true;
    } else {
        sw2 = false;
    }
    // and define 4 as an input for the ID
    pinMode(4, INPUT);
    digitalWrite(4, INPUT_PULLUP); // set pullup on digital pin 4
    if (digitalRead(4)) {
        sw3 = true;
    } else {
        sw3 = false;
    }
    id = (sw3 * 8) + (sw2 * 4) + (sw1 * 2) + (sw0 * 1);
    return;
    Serial.print("ID is: ");
    Serial.print(sw3);
    Serial.print(" ");
    Serial.print(sw2);
    Serial.print(" ");
    Serial.print(sw1);
    Serial.print(" ");
    Serial.println(sw0);
}

void sendLoRaMessage(){
    sendMessage(message);
    int nackCounter = 0;
    while (!receiveAck(message) && nackCounter <= 5) {
        Serial.println(" refused ");
        Serial.print(nackCounter);
        LoRa.sleep();
        delay(1000);
        sendMessage(message);
        nackCounter++;
    }

    if (nackCounter >= 5) {
        Serial.println("");
        Serial.println("--------------- MESSAGE LOST ---------------------");
        messageLostCounter++;
        delay(100);
    } else {
        Serial.println("Acknowledged ");
    }
    counter++;
    delay(1000);
    LoRa.sleep();
}

long readVcc() {  // read our local Vcc ... Arduino pro mini
    long result;
    // Read 1.1V reference against AVcc
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
    delay(2); // Wait for Vref to settle
    ADCSRA |= _BV(ADSC); // Convert
    while (bit_is_set(ADCSRA, ADSC));
    result = ADCL;
    result |= ADCH << 8;
    result = 1126400L / result; // Back-calculate AVcc in mV
    return result;
}

bool receiveAck(String message) {
    String ack;
    Serial.print(" Waiting for Ack ");
    bool stat = false;
    unsigned long entry = millis();
    while (stat == false && millis() - entry < 2000) {
        if (LoRa.parsePacket()) {
            ack = "";
            while (LoRa.available()) {
                ack = ack + ((char)LoRa.read());
            }
            int check = 0;
            // Serial.print("///");
            for (int i = 0; i < message.length(); i++) {
                check += message[i];
                //Serial.print(message[i]);
            }
            /*    Serial.print("/// ");
            Serial.println(check);
            Serial.print(message);
            */
            Serial.print(" Ack ");
            Serial.print(ack);
            Serial.print(" Check ");
            Serial.print(check);
            if (ack.toInt() == check) {
                Serial.print(" Checksum OK ");
                stat = true;
            }
        }
    }
    return stat;
}

void readBME() {
    // get data from BME
    h = bme.readHumidity();
    t = bme.readTemperature();
    t = t*1.8+32.0;
    dp = t-0.36*(100.0-h);
    p = bme.readPressure()/100.0F; // pressure in Pascals, 100 Pascals is 1 hPa (aka millibar)
    pin = 0.02953*p;               // pressure in inches of Hg
}


void sendMessage(String message) {
    Serial.print(message);
    // add our ID
    //sprintf(messageAndID,"
    // send packet
    LoRa.beginPacket();
    LoRa.print(message);
    LoRa.endPacket();
}

void wakeUp()
{
    delay(100);
    Serial.println("wakeup");
    delay(100);
}

char *ftoa(char *a, double f, int precision) { // http://forum.arduino.cc/index.php?topic=44262.0
   long p[] = {0,10,100,1000,10000,100000,1000000,10000000,100000000};
 
   char *ret = a;
   long heiltal = (long)f;
   itoa(heiltal, a, 10);
   while (*a != '\0') a++;
   *a++ = '.';
   long desimal = abs((long)((f - heiltal) * p[precision]));
   itoa(desimal, a, 10);
   return ret;
}

