#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>

#define numLeds 1
#define ledPin 15
#define relayPin 13

int data1 = 0;
int ok = -1;

SoftwareSerial RFID(14, 12); // RX and TX
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(numLeds, ledPin, NEO_GRB + NEO_KHZ800);

// use first sketch in http://wp.me/p3LK05-3Gk to get your tag numbers
int tag1[14] = {2,49,51,48,48,52,70,51,52,57,49,70,57,3};
int tag2[14] = {2,31,33,30,30,34,46,32,46,41,41,44,39,3};
int newtag[14] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0}; // used for read comparisons

void setup()
{
  RFID.begin(9600);    // start serial to RFID reader
  Serial.begin(9600);  // start serial to PC 
  pixels.begin();
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);
  pinMode(ledPin, OUTPUT);
  pixels.setPixelColor(0, pixels.Color(100,100,100)); // Moderately bright green color.
  pixels.show();
}

boolean comparetag(int aa[14], int bb[14])
{
  boolean ff = false;
  int fg = 0;
  for (int cc = 0 ; cc < 14 ; cc++)
  {
    if (aa[cc] == bb[cc])
    {
      fg++;
    }
  }
  if (fg == 14)
  {
    ff = true;
  }
  return ff;
}

void checkmytags() // compares each tag against the tag just read
{
  ok = 0; // this variable helps decision-making,
  // if it is 1 we have a match, zero is a read but no match,
  // -1 is no read attempt made
  if (comparetag(newtag, tag1) == true)
  {
    ok++;
  }
  if (comparetag(newtag, tag2) == true)
  {
    ok++;
  }
}

void readTags()
{
  ok = -1;

  if (RFID.available() > 0) 
  {
    // read tag numbers
    delay(100); // needed to allow time for the data to come in from the serial buffer.

    for (int z = 0 ; z < 14 ; z++) // read the rest of the tag
    {
      data1 = RFID.read();
      newtag[z] = data1;
      Serial.println(data1);
    }
    RFID.flush(); // stops multiple reads

    // do the tags match up?
    checkmytags();
  }

  // now do something based on tag type
  if (ok > 0) // if we had a match
  {
    Serial.println("Accepted");
    digitalWrite(relayPin, LOW);
    pixels.setPixelColor(0, pixels.Color(0,100,0)); // Moderately bright green color.
    pixels.show();
    delay(2000);
    digitalWrite(relayPin, HIGH);
    pixels.setPixelColor(0, pixels.Color(100,100,100)); // Moderately bright green color.
    pixels.show();

    ok = -1;
  }
  else if (ok == 0) // if we didn't have a match
  {
    Serial.println("Rejected");
    pixels.setPixelColor(0, pixels.Color(100,0,0)); // Moderately bright green color.
    pixels.show();
    delay(2000);
    pixels.setPixelColor(0, pixels.Color(100,100,100)); // Moderately bright green color.
    pixels.show();

    ok = -1;
  }
}

void loop()
{
  readTags();
}
