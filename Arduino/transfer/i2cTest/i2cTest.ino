
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
//#include <Audio.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>


//AudioControlSGTL5000     sgtl5000_1; 

#define BUTTON 0
Bounce b = Bounce(0, 8); // 8 = 8 ms debounce time

#define THIS_ADDRESS 0x11
#define OTHER_ADDRESS 0x10

boolean last_state = HIGH;

void setup() {
 
 pinMode(BUTTON, INPUT_PULLUP);
 
// sgtl5000_1.enable();

 Wire.begin(THIS_ADDRESS);
 Wire.onReceive(receiveEvent);
}

void loop() {
  b.update();
  if (b.fallingEdge()) {
    Serial.print(THIS_ADDRESS);
    Serial.print(" send to ");
    Serial.println(OTHER_ADDRESS);
    Wire.beginTransmission(OTHER_ADDRESS);
    Wire.send(1);
    Wire.endTransmission();
    
  }
  delay(200);

    // this is not pretty, but necessary for teensy to switch between slave / master modes once it has read stuff...
    Wire.begin(THIS_ADDRESS);
    Wire.onReceive(receiveEvent);

}

void receiveEvent(int howMany){
  Serial.print(THIS_ADDRESS);
  Serial.print(" received a request from ");
  Serial.print(OTHER_ADDRESS);
  Serial.print(":");
  while (Wire.available() > 0){
    boolean c = Wire.receive();
    Serial.print(c, DEC);
  }
  Serial.println(":end");
}
