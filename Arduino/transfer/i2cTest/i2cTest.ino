
/**
*
* Sample Multi Master I2C implementation.  Sends a button state over I2C to another
* Arduino, which flashes an LED correspinding to button state.
* 
* Connections: Arduino analog pins 4 and 5 are connected between the two Arduinos, 
* with a 1k pullup resistor connected to each line.  Connect a push button between 
* digital pin 10 and ground, and an LED (with a resistor) to digital pin 9.
* 
*/

#include <Wire.h>
#include <Audio.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>


AudioControlSGTL5000     sgtl5000_1; 

#define BUTTON 0
Bounce b = Bounce(0, 8); // 8 = 8 ms debounce time

#define THIS_ADDRESS 0x11
#define OTHER_ADDRESS 0x10

boolean last_state = HIGH;

void setup() {
 
 pinMode(BUTTON, INPUT_PULLUP);
 
 sgtl5000_1.enable();

 Wire.begin(THIS_ADDRESS);
 Wire.onRequest(requestEvent);
}

void loop() {
  int rbyte;
  b.update();
  if (b.fallingEdge()) {
    Serial.print(THIS_ADDRESS);
    Serial.print(" requests 30 bytes from ");
    Serial.print(OTHER_ADDRESS);
    rbyte = Wire.requestFrom(OTHER_ADDRESS, 30);
    Serial.print(" (receiving ");
    Serial.print(rbyte);
    Serial.print("):");
    while (Wire.available() > 0){
      char c = Wire.read();
      Serial.print(c);
    }
    Serial.println(":end");
  }
  delay(100);
}

void requestEvent(){
  Serial.print(THIS_ADDRESS);
  Serial.print(" received a request from ");
  Serial.println(OTHER_ADDRESS);

  Wire.write("Hello ");
}
