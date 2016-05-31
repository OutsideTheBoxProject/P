/*-------------------------------------------------------------------------
  
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


#include "BluetoothSerial.h"

#define INTERRUPT_PIN 3
#define RX 0
#define TX 1

BluetoothSerial BTSerial(RX,TX);


void BluetoothSerial::begin(unsigned long speed) {


   //debug
   Serial.println("GO");
   pinMode(INTERRUPT_PIN,INPUT_PULLUP); 
   mySerial.begin(speed);

}

void BluetoothSerial::sendCommand(const char* cmd) {
  Serial.print("Cmd:");
  Serial.print(cmd);
  Serial.print(" start response|");
  mySerial.write(cmd); 
  delay(50);
  while (mySerial.available()) {
    char c = mySerial.read();
    Serial.print(c); 
  }
  Serial.println("|end response");
}



void BluetoothSerial::setBaudrate(unsigned long speed) {

  //  BAUDRATE TABLE
  //  1---1200
  //  2---2400
  //  3---4800
  //  4---9600 
  //  5---19200 
  //  6---38400 
  //  7---57600 
  //  8---115200 
  //  9---230400 
  //  A---460800 
  //  B---921600 
  //  C---1382400 
  //  default:4---9600

   if(speed == 0) speed=9600;
   char baudRateCode[1];

   switch (speed){

   case 1200:     baudRateCode[0]='1'; break;
   case 2400:     baudRateCode[0]='2'; break;
   case 4800:     baudRateCode[0]='3'; break;
   case 19200:    baudRateCode[0]='5'; break;
   case 38400:    baudRateCode[0]='6'; break;
   case 57600:    baudRateCode[0]='7'; break;
   case 115200:   baudRateCode[0]='8'; break;
   case 230400:   baudRateCode[0]='9'; break;
   case 460800:   baudRateCode[0]='A'; break;
   case 921600:   baudRateCode[0]='B'; break;
   case 1382400:  baudRateCode[0]='C'; break;

  default:
    // 9600
    baudRateCode[0]='4';
  break;

   }


   // DEBUG
   Serial.println("");
   Serial.print("Setting BAUDRATE on device to: ");
   Serial.print(speed);
   Serial.print("  (");
   Serial.print(baudRateCode[0]);
   Serial.println(")");


    mySerial.write("AT+BAUD");
    mySerial.write(baudRateCode[0]);
    mySerial.write("\r\n");
    delay(50);
    while (mySerial.available()) {
      char c = mySerial.read();
      Serial.print(c); // assuming OK
    }
    Serial.println("|");


  // should we close the connection and reopen with set baudrate here?

  // TODO return 1 on succcess / -1 on fail


}

void BluetoothSerial::setPinCode(const char* pin_code) {

   // DEBUG
   Serial.println("");
   Serial.print("Setting PINCODE on device to: '");
   Serial.print(pin_code);
   Serial.println("'");

   // set pin code
   mySerial.write("AT+PIN");
   mySerial.write(pin_code);
   mySerial.write("\r\n");
   delay(50);
   while (mySerial.available()) {
     char c = mySerial.read();
     Serial.print(c); // assuming OK
   }
   Serial.println("|");


  // TODO return 1 on succcess / -1 on fail






}

void BluetoothSerial::setDeviceName(const char* device_name) {

   // DEBUG
   Serial.println("");
   Serial.print("Setting DEVICENAME on device to: ");
   Serial.println(device_name);


  // set name of bluetooth device 
  mySerial.write("AT+NAME");
  mySerial.write(device_name);
  mySerial.write("\r\n");
  delay(50);
  while (mySerial.available()) {
    char c = mySerial.read();
    Serial.print(c); // assuming OK
  }
  Serial.println("|");


  // TODO return 1 on succcess / -1 on fail


}

void BluetoothSerial::makeMaster() {

   // DEBUG
   Serial.println("");
   Serial.println("Making device Master");

  sendCommand("AT+ROLE1\r\n");
  
  myRole = 1;

}

void BluetoothSerial::makeSlave() {

   // DEBUG
   Serial.println("");
   Serial.println("Making device Slave");

  sendCommand("AT+ROLE0\r\n");

  myRole = 0;

}

void BluetoothSerial::resetDevice() {
   // DEBUG
   Serial.println("");
   Serial.println("Resetting device");

  sendCommand("AT+RESET\r\n"); // Set search the remote Bluetooth device automatically

}


int BluetoothSerial::searchDevices() {

   // DEBUG
   Serial.println("");
   Serial.println("Searching for remote devices");

  String response = "";
  char buf[512];
  int devices = 0;
  int i = 0;
  int j = 0;

  for (int j=0; j<20; j++) availableDevices[j] = "";

  sendCommand("AT+ROLE\r\n");
  sendCommand("AT+IAC9e8b33\r\n"); 
  sendCommand("AT+COD001f00\r\n"); 
  sendCommand("AT+INQM1,9,15\r\n"); 
  sendCommand("AT+INQ\r\n"); // Start search
  sendCommand("AT+STATE\r\n");
  delay(10000);
  while (mySerial.available()) {
    buf[j] = mySerial.read(); 
    Serial.print(buf[j]);
    j++;
  }
  buf[j] = '\0';
  Serial.println("|");
  response = String(buf);
  Serial.println(response);
  // Sample response: 
  // +INQS
  // +INQ: 11:22:33:44:55:66,001f00,-90
  // ..
  // +INQE
  i=response.indexOf("+INQ:");
  while (i > -1) {
    availableDevices[devices] = response.substring(i,response.indexOf(",",i));
    response = response.substring(i+6);
    devices++;
    i=response.indexOf("+INQ:");
  }
  return devices;
}

  
char* BluetoothSerial::getRemoteName(const char* device_mac) {
   // DEBUG
   Serial.println("");
   Serial.println("Getting a remote name from mac");

  String response = "";
  char rname[64];
  int i;

  mySerial.write("AT+RNAME");
  mySerial.write(device_mac);
  mySerial.write("\r\n");
  delay(50);
  while (mySerial.available()) response += (char)mySerial.read(); 
  // +RNAME=BC04-B
  i=response.indexOf("+RNAME:");
  if (i > -1) {
    response = response.substring(i);
    response.toCharArray(rname, 64);
    return rname;
  }
  return "";
  
}
  
void BluetoothSerial::connectTo(const char* device_mac) {
   // DEBUG
   Serial.println("");
   Serial.println("Connect to a mac");

  mySerial.write("AT+CONNECT");
  mySerial.write(device_mac);
  mySerial.write("\r\n");
  delay(50);
  while (mySerial.available()) {
    char c = mySerial.read();
    Serial.print(c); // assuming OK
  }
  Serial.println("");
  
}


void BluetoothSerial::onSerialConnectionChange(void (*userFunc)(void)) {
  attachInterrupt(INTERRUPT_PIN,userFunc,CHANGE);
}

void BluetoothSerial::end() {
  mySerial.end();
}

int BluetoothSerial::available() {
  return mySerial.available();
}

int BluetoothSerial::read() {
  return mySerial.read();
}

int BluetoothSerial::peek() {
  return mySerial.peek();
}

void BluetoothSerial::flush() {
  mySerial.flush();
}

size_t BluetoothSerial::write(uint8_t c) {
  mySerial.write(c);
}

