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
// Two pushbuttons need:
//   Record Button: pin 0 to GND
//   Play Button:   pin 1 to GND
//
// Master Side:
//    SDA  o  o SCL
//    GND  x  x Connected
//
//    Connected pin: 2 to GND (if connected, initiate transfer)
//    LED for transmission on pin 3
// 
// Slave Side:
//    SDA  x  x SCL
//    GND  o  o GND
//
//
// Teensy: 
//    SCL 19 
//    SDA 18
//    GND 
//
//



#include <Bounce.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

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

// Bounce objects to easily and reliably read the buttons
Bounce buttonRecord = Bounce(0, 8); // 8 = 8 ms debounce time
Bounce buttonPlay =   Bounce(1, 8);  
Bounce buttonConnected =   Bounce(2, 8);  


// which input on the audio shield will be used?
//const int myInput = AUDIO_INPUT_LINEIN;
const int myInput = AUDIO_INPUT_MIC;

// unique cube i2c adresses  
// http://www.pjrc.com/teensy/td_libs_Wire.html
#define MY_ADDRESS 0x10
#define OTHER_ADDRESS 0x11

// Remember which mode we're doing
#define STOPPED 0
#define RECORDING 1
#define PLAYING 2
#define MASTER 4
#define SLAVE 5
int mode = 0;  // 0=stopped, 1=recording, 2=playing

// The file where data is recorded
File frec;

void setup() {
  // Configure the pushbutton pins
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, OUTPUT);

  digitalWrite(3, LOW);

  // Audio connections require memory, and the record queue
  // uses this memory to buffer incoming audio.
  AudioMemory(60);

  // Enable the audio shield, select input, and enable output
  sgtl5000_1.enable();
  // PROBLEM HERE 
  // enable() will always start the teensy as master without address, we need every cube to have an adress however
  // see http://forum.arduino.cc/index.php?topic=13579.0
  Wire.begin(MY_ADDRESS);
  Wire.onRequest(requestEvent);
  
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
}


void loop() {
  // First, read the buttons
  buttonRecord.update();
  buttonPlay.update();
  buttonConnected.update();

  if (mode != MASTER && mode != SLAVE) {
    // Respond to button presses
    if (buttonRecord.fallingEdge()) {
      Serial.println("Record Button Press");
      if (mode == PLAYING) stopPlaying();
      if (mode == STOPPED) startRecording();
    }
    if (buttonRecord.risingEdge()) {
      Serial.println("Record Button Release");
      if (mode == RECORDING) stopRecording();
      if (mode == PLAYING) stopPlaying();
    }
    if (buttonPlay.fallingEdge()) {
      Serial.println("Play Button Press");
      if (mode == RECORDING) stopRecording();
      if (mode == STOPPED) startPlaying();
    }
    if (buttonConnected.fallingEdge()) {
      Serial.println("Connected!");
      if (mode == RECORDING) stopRecording();
      if (mode == PLAYING) stopPlaying();
      startTransmission();
    }
  }

  // If we're playing or recording or transmitting carry on...
  if (mode == MASTER) {
    transmit();
  }
  if (mode == RECORDING) {
    continueRecording();
  }
  if (mode == PLAYING) {
    continuePlaying();
  }

  // when using a microphone, continuously adjust gain
  if (myInput == AUDIO_INPUT_MIC) adjustMicLevel();

  // this is not pretty, but necessary for teensy to switch between slave / master modes once it has read stuff...
  delay(100);
  Wire.begin(MY_ADDRESS);
  Wire.onRequest(requestEvent);

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
  if (mode == 2) playRaw1.stop();
  mode = STOPPED;
}

void startTransmission() {
  Serial.print("my address is ");
  Serial.print(MY_ADDRESS);
  Serial.print(" trying other address ");
  Serial.println(OTHER_ADDRESS);
  mode = MASTER;
  if (SD.exists("RECORD.RAW")) {
    SD.remove("RECORD.RAW");  
    Serial.println("Removed own recording");
  }
  frec = SD.open("RECORD.RAW", FILE_WRITE);
  if (frec) {
    digitalWrite(3, HIGH); // switch on the transmission LED
  }
  else {
    Serial.println("Cannot open file, aborting transmission");
    mode = STOPPED; // abort if file cannot be opened
  }
}

void transmit() {
  char buffer[512];
  int i = 0;
  Serial.println("Requesting to transmit recording as MASTER");

//  Wire.requestFrom(OTHER_ADDRESS, 512); 
//  while(Wire.available()) {
//    buffer[i] = Wire.read();
//    i++;
//  }
//  frec.write(buffer, i);

  Serial.print("Reading:");
  Wire.requestFrom(OTHER_ADDRESS, 6);
  while (Wire.available() > 0){
      char c = Wire.read();
      Serial.print(c);
  }
  Serial.println(":end");

  if (i < 512) { // all file data is sent or transmission is interrupted
    frec.close();
    mode = STOPPED;
    Serial.println("MASTER: File data transmission completed");
    digitalWrite(3, LOW); // switch off the transmission LED
  }
}  

// receive a connection as slave from another cube
void requestEvent() {
  char buffer[512];
  int i = 0;
  if (mode != SLAVE) {
    Serial.println("New connection request");
    Serial.print("my address is ");
    Serial.print(MY_ADDRESS);
    Serial.print(" request is from ");
    Serial.println(OTHER_ADDRESS);
    if (mode == RECORDING) stopRecording();
    if (mode == PLAYING) stopPlaying();
    frec = SD.open("RECORD.RAW", FILE_READ);
    if(frec) {
      digitalWrite(3, HIGH); // switch on the transmission LED
      mode = SLAVE;
    }
  }
  if (mode == SLAVE) {
    Wire.write("Hello ");
//    while (frec.available()>0 && i < 512) {
//      buffer[i] = frec.read();
//      i++;
//    }
//    Wire.write(buffer,i);
    Serial.println("Data Sent");
    if (i < 512) { // all file data is sent
      frec.close();
      mode = STOPPED;
      Serial.println("SLAVE: File data transmission completed");
      digitalWrite(3, LOW); // switch off the transmission LED    
    }
  }
}

void adjustMicLevel() {
  // TODO: read the peak1 object and adjust sgtl5000_1.micGain()
  // if anyone gets this working, please submit a github pull request :-)
}

