// RCubes
//
// http://outsidethebox.at
//
//
// HARDWARE
// 
// Teensy 3.2
// Audio Shield http://www.pjrc.com/store/teensy3_audio.html
//
// Three pushbuttons need:
//   Record Button: pin 4 to GND
//   Play Button:   pin 5 to GND
//   Connect Button: pin 6 to GND
//
// Status LEDs
//   Connection pin 7   
//
//
// Bluetooth Module
//  Serial: 
//    RX 0
//    TX 1
//  Interrupt: 3


#include <Bounce.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include "BluetoothSerial.h"


// GUItool: begin automatically generated code
AudioInputI2S            i2s2;           //xy=105,63
AudioAnalyzePeak         peak1;          //xy=278,108
AudioRecordQueue         queue1;         //xy=281,63
AudioPlaySdRaw           playRaw1;       //xy=302,157
AudioOutputI2S           i2s1;           //xy=470,120
AudioConnection          patchCord1(i2s2, 0, queue1, 0);
AudioConnection          patchCord2(i2s2, 0, peak1, 0);
AudioConnection          patchCord3(playRaw1, 0, i2s1, 0);
AudioConnection          patchCord4(playRaw1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=265,212
// GUItool: end automatically generated code

#define RECBUTTON 4
#define PLAYBUTTON 5
#define CONNECTBUTTON 6
#define CONNECTLED 7

// Bounce objects to easily and reliably read the buttons
Bounce buttonRecord = Bounce(RECBUTTON, 8); // 8 = 8 ms debounce time
Bounce buttonPlay =   Bounce(PLAYBUTTON, 8);  
Bounce buttonConnect =   Bounce(CONNECTBUTTON, 8);  


// which input on the audio shield will be used?
//const int myInput = AUDIO_INPUT_LINEIN;
const int myInput = AUDIO_INPUT_MIC;

// Unique RCube name
const char* myId = "RCube1";

// Remember which mode we're doing
#define STOPPED 0
#define RECORDING 1
#define PLAYING 2
#define MASTER 4
#define SLAVE 5
int mode = STOPPED;  // 0=stopped, 1=recording, 2=playing

// Remember BT connection status
#define CONNECTED 1
#define NOT_CONNECTED 0
#define TRYING_TO_CONNECT 2
int btstatus = NOT_CONNECTED;

// The file where data is recorded
File frec;

void setup() {
  // Configure the pushbutton pins
  pinMode(RECBUTTON, INPUT_PULLUP);
  pinMode(PLAYBUTTON, INPUT_PULLUP);
  pinMode(CONNECTBUTTON, INPUT_PULLUP);
  pinMode(CONNECTLED, OUTPUT);

  digitalWrite(CONNECTLED, LOW);

  // Audio connections require memory, and the record queue
  // uses this memory to buffer incoming audio.
  AudioMemory(60);

  // Enable the audio shield, select input, and enable output
  sgtl5000_1.enable();  
  sgtl5000_1.inputSelect(myInput);
  sgtl5000_1.unmuteHeadphone();
  sgtl5000_1.volume(0.5);

  // Initialize the SD card
  SPI.setMOSI(7);
  SPI.setSCK(14);
  if (!(SD.begin(10))) {
    // stop here if no SD card, but print a message
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }

  // setup Bluetooth
  BTSerial.begin(9600);  
  BTSerial.sendCommand("AT+DEFAULT\r\n");
//  BTSerial.reset();
  BTSerial.setDeviceName(myId);
//  BTSerial.setBaudRate(9600);
  BTSerial.sendCommand("AT+ROLE1\r\n");
  BTSerial.sendCommand("AT+ROLE1\r\n");
  BTSerial.sendCommand("AT+ROLE1\r\n");
  BTSerial.sendCommand("AT+ROLE\r\n");
//  BTSerial.onSerialConnectionChange(connectionChange);
}


void loop() {
//  // First, read the buttons
//  buttonRecord.update();
//  buttonPlay.update();
//  buttonConnect.update();
//
//  if (mode != MASTER && mode != SLAVE) {
//    // Respond to button presses
//    if (buttonRecord.fallingEdge()) {
//      Serial.println("Record Button Press");
//      if (mode == PLAYING) stopPlaying();
//      if (mode == STOPPED) startRecording();
//    }
//    if (buttonRecord.risingEdge()) {
//      Serial.println("Record Button Release");
//      if (mode == RECORDING) stopRecording();
//      if (mode == PLAYING) stopPlaying();
//    }
//    if (buttonPlay.fallingEdge()) {
//      Serial.println("Play Button Press");
//      if (mode == RECORDING) stopRecording();
//      if (mode == STOPPED) startPlaying();
//    }
//    if (buttonConnect.fallingEdge()) {
//      Serial.println("Connected!");
//      if (mode == RECORDING) stopRecording();
//      if (mode == PLAYING) stopPlaying();
//      startTransmission();
//    }
//  }
//
//  if (mode == MASTER && btstatus == CONNECTED) {
//    transferRecording();
//  }
//  if (mode == RECORDING) {
//    continueRecording();
//  }
//  if (mode == PLAYING) {
//    continuePlaying();
//  }
//
//  // when using a microphone, continuously adjust gain
//  if (myInput == AUDIO_INPUT_MIC) adjustMicLevel();

delay(100);
}


void startRecording() {
  Serial.println("startRecording");
  if (SD.exists("RECORD.RAW")) {
    // The SD library writes new data to the end of the
    // file, so to start a new recording, the old file
    // must be deleted before new data is written.
    SD.remove("RECORD.RAW");
  }
  frec = SD.open("RECORD.RAW", FILE_WRITE);
  if (frec) {
    queue1.begin();
    mode = RECORDING;
  }
}

void continueRecording() {
  if (queue1.available() >= 2) {
    byte buffer[512];
    // Fetch 2 blocks from the audio library and copy
    // into a 512 byte buffer.  The Arduino SD library
    // is most efficient when full 512 byte sector size
    // writes are used.
    memcpy(buffer, queue1.readBuffer(), 256);
    queue1.freeBuffer();
    memcpy(buffer+256, queue1.readBuffer(), 256);
    queue1.freeBuffer();
    // write all 512 bytes to the SD card
    elapsedMicros usec = 0;
    frec.write(buffer, 512);
    // Uncomment these lines to see how long SD writes
    // are taking.  A pair of audio blocks arrives every
    // 5802 microseconds, so hopefully most of the writes
    // take well under 5802 us.  Some will take more, as
    // the SD library also must write to the FAT tables
    // and the SD card controller manages media erase and
    // wear leveling.  The queue1 object can buffer
    // approximately 301700 us of audio, to allow time
    // for occasional high SD card latency, as long as
    // the average write time is under 5802 us.
    Serial.print("SD write, us=");
    Serial.println(usec);
  }
}

void stopRecording() {
  Serial.println("stopRecording");
  queue1.end();
  if (mode == RECORDING) {
    while (queue1.available() > 0) {
      frec.write((byte*)queue1.readBuffer(), 256);
      queue1.freeBuffer();
    }
    frec.close();
  }
  mode = STOPPED;
}

void startPlaying() {
  Serial.println("startPlaying");
  playRaw1.play("RECORD.RAW");
  mode = PLAYING;
}

void continuePlaying() {
  if (!playRaw1.isPlaying()) {
    playRaw1.stop();
    mode = STOPPED;
  }
}

void stopPlaying() {
  Serial.println("stopPlaying");
  if (mode == PLAYING) playRaw1.stop();
  mode = STOPPED;
}

void startTransmission() {
  Serial.println("Connect button pressed, initiating connection as master");
  int devices;
  char mac[32];

  BTSerial.makeMaster();
  devices = BTSerial.searchDevices();
  if (devices > 0) {
    for (int i=0;i<devices;i++) {
      BTSerial.availableDevices[i].toCharArray(mac,32);
      String rname = String(BTSerial.getRemoteName(mac));
      if (rname.indexOf("RCube") > 0) {
        BTSerial.connectTo(mac);
        i = devices;
      }
    }
    mode = MASTER;
    Serial.println("Found and connected to other cube");
  }
  else {
    Serial.println("No remote devices found");
  }
}

void transferRecording() {
  Serial.println("Transferring the recording to slave");

  // tbd

  // disconnect?? maybe make slave??
  mode = STOPPED;
}

void connectionChange() {

  Serial.println("Connection status changed");

  if (btstatus == CONNECTED) {
    if (mode == MASTER) BTSerial.makeSlave(); // thats the default state of the BT module, make sure it gets back
    mode = STOPPED;
    btstatus = NOT_CONNECTED;
  }
  if (btstatus == NOT_CONNECTED) {
    if (mode != MASTER) {
      if (mode == RECORDING) stopRecording();
      if (mode == PLAYING) stopPlaying();
      mode = SLAVE;
    }
    btstatus = CONNECTED;
  }
  
}

void adjustMicLevel() {
  // TODO: read the peak1 object and adjust sgtl5000_1.micGain()
  // if anyone gets this working, please submit a github pull request :-)
}

