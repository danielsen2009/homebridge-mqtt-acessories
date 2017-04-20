#include <Adafruit_NeoPixel.h>
#include "rdm630.h"

#define numLeds 1
#define ledPin 15
#define relayPin 13

int card1 = 5177109;
int card2 = 5185118;

rdm630 rfid(14, 12);  //TX-pin of RDM630 connected to Arduino pin 6

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(numLeds, ledPin, NEO_GRB + NEO_KHZ800);

unsigned long result;

void setup()
{
    Serial.begin(9600);  // start serial to PC
    rfid.begin();
    pixels.begin();
    Serial.println("Ready");
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, HIGH);
    pixels.setPixelColor(0, pixels.Color(0,100,0));
    pixels.show();
    delay(5000);
    pixels.setPixelColor(0, pixels.Color(100,100,100));
    pixels.show();
}

void loop()
{
    byte data[6];
    byte length;

    if(rfid.available()){
        rfid.getData(data,length);
        Serial.println("Data valid");
        for(int i=0;i<length;i++){
            Serial.print(data[i],HEX);
            Serial.print(" ");
        }
        Serial.println();
        //concatenate the bytes in the data array to one long which can be 
        //rendered as a decimal number
        result = 
          ((unsigned long int)data[1]<<24) + 
          ((unsigned long int)data[2]<<16) + 
          ((unsigned long int)data[3]<<8) + 
          data[4];              
        Serial.print("decimal CardID: ");
        Serial.println(result);
        if(result == card1){
        pixels.setPixelColor(0, pixels.Color(0,100,0));
        pixels.show();
        delay(2000);
        pixels.setPixelColor(0, pixels.Color(100,100,100));
        pixels.show();
        }
        if(result == card2){
        pixels.setPixelColor(0, pixels.Color(100,0,0));
        pixels.show();
        delay(2000);
        pixels.setPixelColor(0, pixels.Color(100,100,100));
        pixels.show();
        }
    }
}
