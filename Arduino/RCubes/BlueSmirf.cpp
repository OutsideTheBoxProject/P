/*-------------------------------------------------------------------------

  Adaptation for BlueSmirf, based on the below
  Written by Chris Frauenberger (christopher.frauenberger@tuwien.ac.at)
  
  Arduino library for the Teensy3Bluetooth Serial shield available at the hackerspaceshop.com

  Check it out here:
  http://www.hackerspaceshop.com/teensy/teensy-3-1-bluetooth-module.html


  Written by Amir Hassan and Florian Bittner for hackerspaceshop.com,
  contributions by members of the open source community.


  This is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of
  the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library.  If not, see
  <http://www.gnu.org/licenses/>.
  -------------------------------------------------------------------------*/


#include "BlueSmirf.h"

#define INTERRUPT_PIN 3
#define RX 0
#define TX 1

BlueSmirf BSSerial(RX,TX);


void BlueSmirf::sendCommand(const char* cmd, bool cr) {
  
  Serial.print("Cmd:");
  if (cr) {
    Serial.println(cmd);
    mySerial.println(cmd);
  }
  else {
    Serial.print(cmd);
    mySerial.print(cmd);
  }
  Serial.print(" start response|");
  delay(100);
  receiveMessage();
  Serial.print(response);
  Serial.println("| end response");
}

void BlueSmirf::receiveMessage(void){
  bool keepReading = true;
  int index = 0;
  char message[256];
  char incomingByte;
  
  message[0] = '\0';
  
  while(keepReading){
    keepReading = false;
    
    if (mySerial.available() > 0) {
      // read the incoming byte:
      incomingByte = Serial.read();
      if(incomingByte != 13){
        message[index++] = incomingByte;
        keepReading = true;
      }
    }
  }
  message[index] = '\0';
  response = String(message);
}


void BlueSmirf::begin(unsigned long speed) {

   mySerial.begin(speed);
   enterCmdMode();
   sendCommand("ST,253"); // Continous configuration, local only.
   leaveCmdMode();
}




void BlueSmirf::enterCmdMode() {
  Serial.println("Enter command mode");
  sendCommand("$$$",false);
}

void BlueSmirf::leaveCmdMode() {
  Serial.println("Leave command mode");
  sendCommand("---");
}

bool BlueSmirf::checkConnected() {
  Serial.println("Check if connected");
  enterCmdMode();
  sendCommand("GK");
  if (response.startsWith("1")) isconnected = true;
  else isconnected = false;
  leaveCmdMode();
  return isconnected;
}


void BlueSmirf::setBaudrate(unsigned long speed) {


}

void BlueSmirf::setPinCode(const char* pin_code) {


}

void BlueSmirf::setDeviceName(const char* device_name) {

   // DEBUG
   Serial.println("");
   Serial.print("Setting DEVICENAME on device to: ");
   Serial.println(device_name);

   String cmd = "S-," + String(device_name);
   enterCmdMode();
   sendCommand(cmd.c_str());
   leaveCmdMode();

   

}

void BlueSmirf::makeMaster() {

  // DEBUG
  Serial.println("");
  Serial.println("Making device Master");
  enterCmdMode();
  sendCommand("SM,1");
  leaveCmdMode();
  myRole = 1;

}

void BlueSmirf::makeSlave() {

   // DEBUG
  Serial.println("");
  Serial.println("Making device Slave");
  enterCmdMode();
  sendCommand("SM,0");
  leaveCmdMode();
  myRole = 0;

}

void BlueSmirf::resetDevice() {
   // DEBUG
   Serial.println("");
   Serial.println("Resetting device");

}


int BlueSmirf::searchDevices() {

   // DEBUG
   Serial.println("");
   Serial.println("Searching for remote devices");

  int devices = 0;
  bool searching = true;

  for (int j=0; j<20; j++) {
    availableDevicesNames[j] = "";
    availableDevicesMacs[j] = "";
  }

  enterCmdMode();
  sendCommand("I,20");
  leaveCmdMode();

  while (searching) {
    delay(200);
    receiveMessage();
    if (response.length() > 0) {

      if (response.compareTo("Inquiry Done")) {
        searching = false;
      }
      else {
        availableDevicesMacs[devices] = response.substring(0,12);
        availableDevicesNames[devices] = response.substring(12,response.indexOf(",",12));        
        devices++;
      }
      
   }
 }
 return devices;
}
  
void BlueSmirf::connectTo(String device_mac) {
   // DEBUG
   Serial.println("");
   Serial.println("Connect to a mac");

  isconnected = false;
  enterCmdMode();
  String cmd = "C," + device_mac;
  sendCommand(cmd.c_str());
  leaveCmdMode();
  while (!isconnected) {
    delay(1000);
    checkConnected();
  }
}


void BlueSmirf::end() {
  mySerial.end();
}

int BlueSmirf::available() {
  return mySerial.available();
}

int BlueSmirf::read() {
  return mySerial.read();
}

int BlueSmirf::peek() {
  return mySerial.peek();
}

void BlueSmirf::flush() {
  mySerial.flush();
}

size_t BlueSmirf::write(uint8_t c) {
  return mySerial.write(c);
}

