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

#define RX 0
#define TX 1

BlueSmirf BSSerial(RX,TX);


void BlueSmirf::sendCommand(const char* cmd, bool cr, bool vb) {
  
  if (vb) Serial.print("Cmd:");
  if (cr) {
    if (vb) Serial.println(cmd);
    mySerial.println(cmd);
  }
  else {
    if (vb) Serial.print(cmd);
    mySerial.print(cmd);
  }
  if (vb) Serial.print(" start response|");
  delay(100);
  receiveMessage();
  if (vb) Serial.print(response);
  if (vb) Serial.println("| end response");
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
      incomingByte = mySerial.read();
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
   sendCommand("SM,0");
   sendCommand("SO,%");
   leaveCmdMode();
   
}




void BlueSmirf::enterCmdMode() {
//  Serial.println("Enter command mode");
  sendCommand("$$$",false,false);
}

void BlueSmirf::leaveCmdMode() {
//  Serial.println("Leave command mode");
  sendCommand("---",true,false);
}

void BlueSmirf::killConnection() {
  enterCmdMode();
  sendCommand("K,");
  leaveCmdMode();  
  while (response.indexOf("DISCONNECT") != -1) {
    Serial.println(response);
    receiveMessage();
    delay(500);
  }
  Serial.println("Connection killed");
}

void BlueSmirf::reboot() {
//  Serial.println("Leave command mode");
  enterCmdMode();
  sendCommand("R,1");
  leaveCmdMode();
}

bool BlueSmirf::checkConnected() {
//  Serial.println("Check if connected");
  String conStr;
  enterCmdMode();
  sendCommand("GK", true, false);
  if (myRole == 1) conStr = "4,0,0";
  else conStr = "1,0,0";
  if (response.indexOf(conStr) != -1) isconnected = true;
  else isconnected = false;
  leaveCmdMode();
  return isconnected;
}

void BlueSmirf::waitForAck() {
  char a[3];
  for (int j=0;j<3;j++) a[j]='-';
  bool ackReceived = false;

  while (!ackReceived) {
    while (BSSerial.available()) {
      a[0] = a[1];
      a[1] = a[2];
      a[2] = BSSerial.read();
    }
    if (a[0] == 'A' && a[1] == 'C' && a[2] == 'K') ackReceived = true;
    delay(10);
  }
}

void BlueSmirf::sendAck() {

  mySerial.write("ACK",3);
  
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
  String result = "";
  int index;
  
  for (int j=0; j<10; j++) {
    availableDevicesNames[j] = "";
    availableDevicesMacs[j] = "";
  }

  enterCmdMode();
  sendCommand("I,4");
  leaveCmdMode();

  while ((result.indexOf("No Devices Found") == -1) && (result.indexOf("Inquiry Done") == -1)) {
    Serial.print(".");
    delay(200);
    receiveMessage();
    result += response;
  }
  Serial.println("");
  Serial.println(result);
  if (result.indexOf("No Devices Found") != -1) return 0;

  result = result.substring(8);
  while (result.indexOf(",") != -1) {
    availableDevicesMacs[devices] = result.substring(0,12);
    availableDevicesNames[devices] = result.substring(13,result.indexOf(",",13));   
    Serial.print(availableDevicesMacs[devices]);
    Serial.print(" / ");
    Serial.println(availableDevicesNames[devices]);
    result = result.substring(14 + availableDevicesNames[devices].length());
    index = result.indexOf(",");
    if (index >12) result = result.substring(index-12);
    devices++;
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
    delay(200);
    receiveMessage(); // get connected message out of the way
    if (response.length() > 0) Serial.println(response);
    if (response.indexOf("CONNECT") !=-1) isconnected = true;
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

size_t BlueSmirf::write(const uint8_t *buffer, size_t size) {
  return mySerial.write(buffer,size);
}

