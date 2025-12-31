
// Copyright 2023 Rich Heslip
//
// Author: Rich Heslip 
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
/*
 * Twisty 2 MIDI controller using RP2040 or RP2350
 * uses 16 multiplexed encoders for encoder and button input with RGB LEDs
 * two encoders for menu inputs
 * I2C OLED display
 * R Heslip Dec 2025 
 * based on my Twisty MIDI controller code and other stuff
 * 
 */


#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include "Clickencoder.h"
//#include "StepSeq.h"
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include "LittleFS.h"

// #define BLUETOOTH  // define for bluetooth MIDI - not working yet on Pico 2W
#ifdef BLUETOOTH
#include <Control_Surface.h>
#include <MIDI_Interfaces/BluetoothMIDI_Interface.hpp>
#endif

#define TRUE 1
#define FALSE 0

#define PIN_WIRE_SDA 2
#define PIN_WIRE_SCL 3
#define LEDPIN 4

#define NUMPIXELS 16 // 
#define LED_RED 0x1f0000  // only using 5 bits to keep LEDs from getting too bright
#define LED_GREEN 0x001f00
#define LED_BLUE 0x00001f
#define LED_ORANGE (LED_RED|LED_GREEN)
#define LED_VIOLET (LED_RED|LED_BLUE)
#define LED_AQUA (LED_GREEN|LED_BLUE)
#define LED_BLACK 0
#define LED_WHITE (LED_RED|LED_GREEN|LED_BLUE)

int32_t colorpalette[8]={LED_RED,LED_ORANGE,LED_GREEN,LED_AQUA,LED_BLUE,LED_VIOLET,LED_WHITE,LED_BLACK}; // limited color palette makes things easier

int8_t brightness_table[32] ={  // more or less exponential table to compensate for perceived brightness
  1,1,2,2,
  3,3,4,4,
  6,6,8,8,
  12,12,16,16,
  25,25,34,34,
  44,44,58,58,
  72,72,96,96,
  110,110,127,127
};

Adafruit_NeoPixel LEDS(NUMPIXELS, LEDPIN, NEO_GRB + NEO_KHZ800);

// I used mostly int16 types - its what the menu requires and the compiler seems to deal with basic conversions
#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 32  // OLED display height, in pixels
#define SCREEN_BUFFER_SIZE (SCREEN_WIDTH * ((SCREEN_HEIGHT + 7) / 8))
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET);

#define DISPLAY_BLANK_MS 120*1000  // display blanking time
#define LEDFLASH_EDIT  250   // LED flash while editing
int32_t displaytimer ; // display blanking timer
int32_t LEDtimer; // LED flash timer
bool LEDstate;   // for LED flash
#define OLED_DISPLAY   // for graphics conditionals

#define NUMENCODERS 16 // number of encoders

// I/O definitions
#define A_MUX_0 9   // HC4067 address lines
#define A_MUX_1 8
#define A_MUX_2 7
#define A_MUX_3 6

#define ENCA_IN 17 // ports we read values from MUX
#define ENCB_IN 11 // A & B swapped to get correct rotation
#define ENCSW_IN 10

#define LMENU_ENCA_IN 19 // left menu encoder
#define LMENU_ENCB_IN 18 // A & B swapped to get correct rotation
#define LMENU_ENCSW_IN 16

#define RMENU_ENCA_IN 22 // left menu encoder
#define RMENU_ENCB_IN 21 // A & B swapped to get correct rotation
#define RMENU_ENCSW_IN 20

// encoders - use an array of clickencoder objects for the 16 multiplexed encoders
#define ENCDIVIDE 4  // divide by 4 works best with my encoders
ClickEncoder enc[NUMENCODERS] = {
  ClickEncoder(ENCA_IN,ENCB_IN,ENCSW_IN,ENCDIVIDE), 
  ClickEncoder(ENCA_IN,ENCB_IN,ENCSW_IN,ENCDIVIDE), 
  ClickEncoder(ENCA_IN,ENCB_IN,ENCSW_IN,ENCDIVIDE), 
  ClickEncoder(ENCA_IN,ENCB_IN,ENCSW_IN,ENCDIVIDE),
  ClickEncoder(ENCA_IN,ENCB_IN,ENCSW_IN,ENCDIVIDE), 
  ClickEncoder(ENCA_IN,ENCB_IN,ENCSW_IN,ENCDIVIDE), 
  ClickEncoder(ENCA_IN,ENCB_IN,ENCSW_IN,ENCDIVIDE), 
  ClickEncoder(ENCA_IN,ENCB_IN,ENCSW_IN,ENCDIVIDE),
  ClickEncoder(ENCA_IN,ENCB_IN,ENCSW_IN,ENCDIVIDE), 
  ClickEncoder(ENCA_IN,ENCB_IN,ENCSW_IN,ENCDIVIDE), 
  ClickEncoder(ENCA_IN,ENCB_IN,ENCSW_IN,ENCDIVIDE), 
  ClickEncoder(ENCA_IN,ENCB_IN,ENCSW_IN,ENCDIVIDE),
  ClickEncoder(ENCA_IN,ENCB_IN,ENCSW_IN,ENCDIVIDE), 
  ClickEncoder(ENCA_IN,ENCB_IN,ENCSW_IN,ENCDIVIDE), 
  ClickEncoder(ENCA_IN,ENCB_IN,ENCSW_IN,ENCDIVIDE), 
  ClickEncoder(ENCA_IN,ENCB_IN,ENCSW_IN,ENCDIVIDE)
};

ClickEncoder lmenuenc(LMENU_ENCA_IN,LMENU_ENCB_IN,LMENU_ENCSW_IN,ENCDIVIDE); // left menu encoder object
ClickEncoder rmenuenc(RMENU_ENCA_IN,RMENU_ENCB_IN,RMENU_ENCSW_IN,ENCDIVIDE); // right menu encoder object

#ifdef BLUETOOTH
BluetoothMIDI_Interface midi_ble;
#endif
// USB MIDI object
Adafruit_USBD_MIDI usb_midi;
// serial MIDI object
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, serialMIDI);
// Instantiate a MIDI over BLE interface


// Create a new instance of the Arduino MIDI Library,
// and attach usb_midi as the transport.
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MidiUSB);

#define BASE_CC 16  // lowest CC number to use
#define CONTROLLER_PAGES 4  // number of pages
#define DEFAULT_CHANNEL 1  // default MIDI channel
uint16_t page=0; // CC page 0-3
enum encodertypes {CCTYPE};
enum switchmodes {MOMENTARY,TOGGLE};

struct controllerencoder {
  int16_t type;   // midi message type
  int16_t channel;
  int16_t ccnumber;
  int16_t minvalue;
  int16_t maxvalue;
  int16_t value;
  int16_t color;  // leds are 8 bits R, G and B
  int16_t label; // index of label
};

struct controllerswitch {
  int16_t mode;
  int16_t type;
  int16_t channel;
  int16_t ccnumber;
  int16_t minvalue;
  int16_t maxvalue;
  int16_t value;
  int16_t color;
  int16_t label; // index of label
};

struct controllerpage {
  struct controllerencoder encoder[NUMENCODERS];
  struct controllerswitch encswitch[NUMENCODERS];
} controls[CONTROLLER_PAGES];

// edit buffer
struct controller {
  struct controllerencoder encoder;
  struct controllerswitch encswitch;
} editbuffer;

// initialize all the controller settings
void initcontrols(void) {
  int16_t ccnumber=BASE_CC;
  for (int16_t p=0;p<CONTROLLER_PAGES;++p) {
    for (int16_t i=0; i<NUMENCODERS;++i) {
      controls[p].encoder[i].type=CCTYPE;
      controls[p].encoder[i].channel=DEFAULT_CHANNEL;
      controls[p].encoder[i].ccnumber=ccnumber;
      controls[p].encoder[i].minvalue=0;
      controls[p].encoder[i].maxvalue=127;
      controls[p].encoder[i].value=64;
      controls[p].encoder[i].color=p;
      controls[p].encoder[i].label=0;  // label index 0 is "CC"
      controls[p].encswitch[i].mode=TOGGLE;
      controls[p].encswitch[i].type=CCTYPE;
      controls[p].encswitch[i].channel=DEFAULT_CHANNEL+1;
      controls[p].encswitch[i].ccnumber=ccnumber++;
      controls[p].encswitch[i].minvalue=0;
      controls[p].encswitch[i].maxvalue=127;
      controls[p].encswitch[i].value=0;
      controls[p].encswitch[i].color=p;  // not used for now
      controls[p].encswitch[i].label=0;  // label index 0 is "CC"
    }  
  }
}

// values use in save/retore menu
int16_t saverestore_slot=1;
int16_t saverestore_action=0; // initial setting is restore
int16_t saverestore_confirm=0; // take/don't take action

// Allocate the JSON document for save/restore
JsonDocument doc;

enum control {ENCODER,BUTTON};
int16_t lastcontrol=0; // keeps track of last used control

// copy encoder parameters to temporary parameters for editing
// this allows one menu for all encoders vs 64 almost identical menus
void copy_to_editbuffer(int16_t page,int16_t index) {
  editbuffer.encoder.type=controls[page].encoder[index].type;
  editbuffer.encoder.channel=controls[page].encoder[index].channel;
  editbuffer.encoder.ccnumber=controls[page].encoder[index].ccnumber;
  editbuffer.encoder.minvalue=controls[page].encoder[index].minvalue;
  editbuffer.encoder.maxvalue=controls[page].encoder[index].maxvalue;
  editbuffer.encoder.color=controls[page].encoder[index].color;
  editbuffer.encoder.label=controls[page].encoder[index].label;
  editbuffer.encswitch.mode=controls[page].encswitch[index].mode;
  editbuffer.encswitch.type=controls[page].encswitch[index].type;
  editbuffer.encswitch.channel=controls[page].encswitch[index].channel;
  editbuffer.encswitch.ccnumber=controls[page].encswitch[index].ccnumber;
  editbuffer.encswitch.minvalue=controls[page].encswitch[index].minvalue;
  editbuffer.encswitch.maxvalue=controls[page].encswitch[index].maxvalue;
  editbuffer.encswitch.color=controls[page].encswitch[index].color;
  editbuffer.encswitch.label=controls[page].encswitch[index].label;
}

// copy edited temporary parameters to encoder parameters
void restore_from_editbuffer(int16_t page,int16_t index) {
  controls[page].encoder[index].type=editbuffer.encoder.type;
  controls[page].encoder[index].channel=editbuffer.encoder.channel;
  controls[page].encoder[index].ccnumber=editbuffer.encoder.ccnumber;
  controls[page].encoder[index].minvalue=editbuffer.encoder.minvalue;
  controls[page].encoder[index].maxvalue=editbuffer.encoder.maxvalue;
  controls[page].encoder[index].color=editbuffer.encoder.color;
  controls[page].encoder[index].label=editbuffer.encoder.label;
  controls[page].encswitch[index].mode=editbuffer.encswitch.mode;
  controls[page].encswitch[index].type=editbuffer.encswitch.type;
  controls[page].encswitch[index].channel=editbuffer.encswitch.channel;
  controls[page].encswitch[index].ccnumber=editbuffer.encswitch.ccnumber;
  controls[page].encswitch[index].minvalue=editbuffer.encswitch.minvalue;
  controls[page].encswitch[index].maxvalue=editbuffer.encswitch.maxvalue;
  controls[page].encswitch[index].color=editbuffer.encswitch.color;
  controls[page].encswitch[index].label=editbuffer.encswitch.label;
}

enum ui_states {UI_SEND_MIDI,UI_EDIT,UI_LOADSAVE};
int16_t UI_state=UI_SEND_MIDI;

#define TIMER_MICROS 1000 // interrupt period

// RP2040 timer code from https://github.com/raspberrypi/pico-examples/blob/master/timer/timer_lowlevel/timer_lowlevel.c
// Use alarm 0
#define ALARM_NUM 0
#define ALARM_IRQ timer_hardware_alarm_get_irq_num(timer_hw, ALARM_NUM)

static void alarm_in_us(uint32_t delay_us) {
  hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
  irq_set_exclusive_handler(ALARM_IRQ, alarm_irq);
  irq_set_enabled(ALARM_IRQ, true);
  alarm_in_us_arm(delay_us);
}

static void alarm_in_us_arm(uint32_t delay_us) {
  uint64_t target = timer_hw->timerawl + delay_us;
  timer_hw->alarm[ALARM_NUM] = (uint32_t) target;
}

// timer interrupt handler
// scans thru the multiplexed encoders and handles the menu encoders

static void alarm_irq(void) {
  for (int addr=0; addr< NUMENCODERS;++addr) {
    digitalWrite(A_MUX_0, addr & 1);
    digitalWrite(A_MUX_1, addr & 2);
    digitalWrite(A_MUX_2, addr & 4);
    digitalWrite(A_MUX_3, addr & 8); 
    delayMicroseconds(4);         // address settling time 
    enc[addr].service();    // check the encoder inputs
  } 
  lmenuenc.service(); // handle the menu encoders which are on different port pins
  rmenuenc.service(); // 
  hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM); // clear IRQ flag
  alarm_in_us_arm(TIMER_MICROS);  // reschedule interrupt
}

// set up as include files because I'm too lazy to create proper header and .cpp files
#include "menusystem.h"  // has to come after display and encoder objects creation
#include "fileio.h"

 // midi related stuff
// Channel is 0-15. Typically reported to the user as 1-16.

void sendnoteOn(byte channel, byte pitch, byte velocity) {
  MidiUSB.sendNoteOn(pitch,velocity,channel);
  serialMIDI.sendNoteOn(pitch,velocity,channel);
}

void sendnoteOff(byte channel, byte pitch, byte velocity) {
  MidiUSB.sendNoteOff(pitch,velocity,channel);
  serialMIDI.sendNoteOff(pitch,velocity,channel);
}

// message 0x0B control change.
// 2nd parameter is the control number number (0-119).
// 3rd parameter is the control value (0-127).

void sendcontrolChange(byte channel, byte control, byte value) {
  MidiUSB.sendControlChange(control,value,channel);
  serialMIDI.sendControlChange(control,value,channel);
#ifdef BLUETOOTH
  MIDIAddress midiaddress= {control,Channel_1+channel};  // confusing way of sending MIDI messages IMO
  midi_ble.sendControlChange(midiaddress, value); 
#endif
}

// turn off the display to avoid burn in but keep the display buffer intact
// the next call to display.display() will automagically restore what was there plus whatver has been drawn since

void blankdisplay(void) {
  uint8_t tempbuf[SCREEN_BUFFER_SIZE];
  uint8_t * buf, * tbuf;
  buf=display.getBuffer();
  tbuf= tempbuf;
  memcpy(tbuf,buf,SCREEN_BUFFER_SIZE); // copy frame buffer to temp storage
  display.clearDisplay();
  display.display();  // turn off the OLEDs
  memcpy(buf,tbuf,SCREEN_BUFFER_SIZE); // restore the old frame buffer
}

// update the display and reset the display blanking timer
void updatedisplay(){
  display.display();
  displaytimer=millis();
}

// show CC value of kob on display
void showencodercc(int16_t page, int16_t encoder){
  display.setTextSize(2);
  display.setCursor(0,16);
  display.print("              ");
  display.setCursor(0,16);
  // the default label 0 "CC" is a special case where we show the CC number in large font, otherwise we show a custom label
  if (controls[page].encoder[encoder].label ==0) display.printf("%s %d %d","CC",controls[page].encoder[encoder].ccnumber,controls[page].encoder[encoder].value);
  else display.printf("%s %d",labels[controls[page].encoder[encoder].label],controls[page].encoder[encoder].value);
  display.setTextSize(1);
  updatedisplay();  
}

// show CC value of button on display
void showswitchcc(int16_t page, int16_t button){
  display.setTextSize(2);
  display.setCursor(0,16);
  display.print("              ");
  display.setCursor(0,16);
  // the default label 0 "CC" is a special case where we show the CC number in large font, otherwise we show a custom label
  if (controls[page].encswitch[button].label ==0) display.printf("%s %d %d","CC",controls[page].encswitch[button].ccnumber,controls[page].encswitch[button].value);
  else display.printf("%s %d",labels[controls[page].encswitch[button].label],controls[page].encswitch[button].value);
  display.setTextSize(1);
  updatedisplay();  
}

// show page number on display
void showpage(int16_t page){
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("                     "); // erase the whole line
  display.setCursor(0,0);
  display.printf("Pg %d",page);
  display.setTextSize(1);
//  updatedisplay();  
}

// show CC number on display
void showcc(int16_t cc){
  display.setTextSize(1);
  display.setCursor(32,0);
  display.printf("CC %d",cc);
  display.setTextSize(1);
//  updatedisplay();  
}
// show page number on display
void showchannel(int16_t channel){
  display.setTextSize(1);
  display.setCursor(80,0);
  display.printf("Ch %d",channel);
  display.setTextSize(1);
  updatedisplay(); 
}

// show encoder encoder value
void showencoder(int16_t page,int16_t controlindex) {
    showpage(page+1);  // for display use 1 based indices
    showcc(controls[page].encoder[controlindex].ccnumber);
    showchannel(controls[page].encoder[controlindex].channel); // update the display
    showencodercc(page,controlindex);
}

// show encoder switch value
void showswitch(int16_t page,int16_t controlindex) {
    showpage(page+1);  // for display use 1 based indices
    showcc(controls[page].encswitch[controlindex].ccnumber);
    showchannel(controls[page].encswitch[controlindex].channel); // update the display
    showswitchcc(page,controlindex);
}

// show the LED associated with a encoder
void showencoderLED(int16_t page,int16_t encoder) {
  int32_t color=colorpalette[controls[page].encoder[encoder].color];
  color=color/31; // scale color down to 1 bit **** this isn't going to work with custom colors
  color=color*brightness_table[(controls[page].encoder[encoder].value>>2 & 0x1f)]; // scale the brightness by the CC value
  LEDS.setPixelColor(encoder,color);
}

// show the LED associated with a switch
void showswitchLED(int16_t page,int16_t button) {
  if (controls[page].encswitch[button].value == controls[page].encswitch[button].maxvalue) LEDS.setPixelColor(button,LED_WHITE);
  else LEDS.setPixelColor(button,LED_BLACK);
//  Serial.printf("val %d \n",controls[page].sw[button].value);
}

// show all the encoder LEDs on a page
void showencoderLEDs(int16_t page) {
  for (int16_t i=0;i< NUMPIXELS;++i) showencoderLED(page,i);
}

// flush all encoder messages 
// pending messages cause race conditions in the menus sometimes 
void flush_encoders(void) {
  ClickEncoder::Button button;
  ClickEncoder::ButtonEvent event;  
  for (int i=0;i<NUMENCODERS;++i) {  
    enc[i].getButton();
    enc[i].getButtonEvent();
    enc[i].getValue();
  }
  lmenuenc.getButton();
  lmenuenc.getButtonEvent();
  lmenuenc.getValue();
  rmenuenc.getButton();
  rmenuenc.getButtonEvent();
  rmenuenc.getValue();
}

void fatalerror(const char * errorstring){
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.printf("FATAL ERROR\n\n%s", errorstring);
  updatedisplay();

  while (1) {
    for (int16_t i=0;i<NUMENCODERS;++i) LEDS.setPixelColor(i,LED_RED); // flashing red
    LEDS.show();
    delay(75);
    for (int16_t i=0;i<NUMENCODERS;++i) LEDS.setPixelColor(i,LED_BLACK); // 
    LEDS.show();
    delay(75);
  } 
}

// ***** Menu handler functions *****

// menu function to handle save/restore menus
void save_restore(void) {
  display.clearDisplay();
  display.setCursor(0,12);
  if ((saverestore_action == 1) && (saverestore_confirm ==1)) {
    if (saveconfig(saverestore_slot)) display.printf("Saved to Slot %d", saverestore_slot);
    else display.printf("File Write Error");
  } 
  if ((saverestore_action == 0) && (saverestore_confirm ==1)) {
    if (loadconfig(saverestore_slot)) display.printf("Restored from Slot %d", saverestore_slot);
    else display.printf("File Read Error");     
  }
  if ((saverestore_action == 2) && (saverestore_confirm ==1)) {
    LittleFS.format();
    display.printf("FFS ReFormatted");       
  }
  if (saverestore_confirm == 0) display.printf("Aborted Save/Restore"); 
  display.display();
  rmenuenc.getButton(); // eat the last click so we don't jump back into the save menu
  saverestore_action=saverestore_confirm=0; // reset the menu
  UI_state=UI_SEND_MIDI;  // put the UI back to default state
  page=lastcontrol=0;
  showencoderLEDs(page); // update the LEDs
  LEDS.show();
  delay(3000);     // delay here to show above fail/success message
  display.clearDisplay();
  showencoder(page,lastcontrol);   // restore the display
  updatedisplay();
  flush_encoders();   // toss any encoder messages
}


void setup() {
  Serial.begin(115200);
 // while (!Serial);  // wait for USB connection

// init IO ports
  pinMode(A_MUX_0, OUTPUT);    // encoder mux addresses
  pinMode(A_MUX_1, OUTPUT);  
  pinMode(A_MUX_2, OUTPUT);  
  pinMode(A_MUX_3, OUTPUT);
  pinMode(LMENU_ENCA_IN, INPUT_PULLUP);  // menu encoder and switches
  pinMode(LMENU_ENCB_IN, INPUT_PULLUP);    
  pinMode(LMENU_ENCSW_IN, INPUT_PULLUP); 
  pinMode(RMENU_ENCA_IN, INPUT_PULLUP);  // menu encoder and switches
  pinMode(RMENU_ENCB_IN, INPUT_PULLUP);    
  pinMode(RMENU_ENCSW_IN, INPUT_PULLUP); 

// set up I2C pins - meant to use I2C0 controller but messed up the schematic pins so have to use I2C1
  Wire1.setSDA(PIN_WIRE_SDA);
  Wire1.setSCL(PIN_WIRE_SCL);
  Wire1.begin();

// set up timer interrupt 
  alarm_in_us(TIMER_MICROS);
 
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
   // fatalerror(); // Don't proceed, loop forever
  }
  display.setRotation(2);
  display.clearDisplay();
// text display tests
  display.setTextSize(2);

	display.setTextColor(WHITE,BLACK); // foreground, background  
  display.setCursor(20,0);
  display.println("Twisty 2");
  display.display();
  delay(3000);

  initcontrols(); // set up default encoder and switch values 

 // for (int16_t i=0; i< NUMENCODERS;++i) enc[i].setDoubleClickEnabled(false); // turn off the doubleclick since it introduces a delay

  LEDS.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  showencoderLEDs(0); // show page 0 encoder LED colors
  LEDS.show();

 if (!LittleFS.begin()) fatalerror("Can't mount FS"); // start up filesystem

 serialMIDI.begin(MIDI_CHANNEL_OMNI);  // hardware serial port

// Manual begin() is required on core without built-in support e.g. mbed rp2040
  if (!TinyUSBDevice.isInitialized()) {
    TinyUSBDevice.begin(0);
  }
  usb_midi.setStringDescriptor("Twisty 2 MIDI");
  // Initialize USB MIDI, and listen to all MIDI channels
  MidiUSB.begin(MIDI_CHANNEL_OMNI);

#ifdef BLUETOOTH
    // Change the name of the BLE device (must be done before initializing it)
  midi_ble.setName("Twisty2");
  // Initialize the BT MIDI interface
  MIDI_Interface::beginAll();
#endif

  // If already enumerated, additional class driver begin() e.g msc, hid, midi won't take effect until re-enumeration
  if (TinyUSBDevice.mounted()) {
    TinyUSBDevice.detach();
    delay(10);
    TinyUSBDevice.attach();
  }

  display.clearDisplay();
  display.setTextSize(1);  
  showencoder(0,0);   // put something on the display
  updatedisplay();

  flush_encoders();  // clear any initial junk from encoders

  displaytimer=millis(); // reset display blanking timer
}

void loop() {
  ClickEncoder::Button button;
  ClickEncoder::ButtonEvent event;
  int16_t t,n;

  // read any new MIDI messages
  MidiUSB.read(); 
  
  if ((millis()-displaytimer) > DISPLAY_BLANK_MS) blankdisplay(); // protect the OLED from burnin

  switch (UI_state) {
    case UI_SEND_MIDI: 
      for (int i=0;i<NUMENCODERS;++i) {
        if ((t=enc[i].getValue()) !=0) { // if encoder has moved, update the value
          if (controls[page].encoder[i].minvalue < controls[page].encoder[i].maxvalue ) { // normal direction
            controls[page].encoder[i].value=controls[page].encoder[i].value + t;
            if (controls[page].encoder[i].value > controls[page].encoder[i].maxvalue) controls[page].encoder[i].value = controls[page].encoder[i].maxvalue;
            if (controls[page].encoder[i].value < controls[page].encoder[i].minvalue) controls[page].encoder[i].value = controls[page].encoder[i].minvalue;
          } else {
            controls[page].encoder[i].value=controls[page].encoder[i].value - t; // encoder direction is reversed
            if (controls[page].encoder[i].value > controls[page].encoder[i].minvalue) controls[page].encoder[i].value = controls[page].encoder[i].minvalue;
            if (controls[page].encoder[i].value < controls[page].encoder[i].maxvalue) controls[page].encoder[i].value = controls[page].encoder[i].maxvalue;            
          }
         // Serial.printf("t=%d value %d\n",t,controls[page].encoder[i].value);
          sendcontrolChange(controls[page].encoder[i].channel, controls[page].encoder[i].ccnumber,controls[page].encoder[i].value);
          lastcontrol=i;  // save index of the last used encoder for menus
          showencoderLED(page,lastcontrol);
          showencoder(page,lastcontrol);
          updatedisplay();       
        }
      }

      for (int i=0;i<NUMENCODERS;++i) {
        event=enc[i].getButtonEvent();
        if (event == ClickEncoder::ActiveEdge) { // button just pressed, send message and update LEDs
         
          switch (controls[page].encswitch[i].mode) {
            case MOMENTARY:
              controls[page].encswitch[i].value=controls[page].encswitch[i].maxvalue; 
              sendcontrolChange(controls[page].encswitch[i].channel, controls[page].encswitch[i].ccnumber,controls[page].encswitch[i].value); //  send a MIDI message  
              break;
            case TOGGLE:
              if (controls[page].encswitch[i].value == controls[page].encswitch[i].minvalue) {
                controls[page].encswitch[i].value = controls[page].encswitch[i].maxvalue;
                sendcontrolChange(controls[page].encswitch[i].channel, controls[page].encswitch[i].ccnumber,controls[page].encswitch[i].value); //  send a MIDI message  
              }
              else {
                controls[page].encswitch[i].value = controls[page].encswitch[i].minvalue;
                sendcontrolChange(controls[page].encswitch[i].channel, controls[page].encswitch[i].ccnumber,controls[page].encswitch[i].value); //  send a MIDI message  
              }
            // Serial.printf("mode %d val %d min %d max %d\n",controls[page].encswitch[i].mode,controls[page].encswitch[i].value,controls[page].encswitch[i].minvalue,controls[page].encswitch[i].maxvalue);
              break;
            default:
              break;
          }  // end switch
          lastcontrol=i;  // save index of the last used encoder for menus
          showswitchLED(page,lastcontrol);
          showswitch(page,lastcontrol);
          updatedisplay();       
        }
        if ((event == ClickEncoder::InActiveEdge) && (controls[page].encswitch[i].mode==MOMENTARY)) { // button just released, send message and update LEDs 
          controls[page].encswitch[i].value=controls[page].encswitch[i].minvalue;
          sendcontrolChange(controls[page].encswitch[i].channel, controls[page].encswitch[i].ccnumber,controls[page].encswitch[i].value); //  send a MIDI message  
          lastcontrol=i;  // save index of the last used encoder for menus
          showswitchLED(page,lastcontrol);
          showswitch(page,lastcontrol);
          updatedisplay();    
        }
      }

      if ((t=lmenuenc.getValue()) !=0) { // left encoder changes controls page
        page=constrain(page+t,0,CONTROLLER_PAGES-1);
        showencoder(page,0);
        showencoderLEDs(page);
      }

      if (lmenuenc.getButton() == ClickEncoder::Clicked) { // entering edit mode
        display.clearDisplay();
        topmenuindex=0;  // not using top menu, just submenus
        menustate=SUBSELECT; // do submenu when button is released
        copy_to_editbuffer(page,lastcontrol); //copy encoder parameters for editing
        topmenu[topmenuindex].submenuindex=0;  // start from the first item
        drawsubmenus();
        drawselector(topmenu[topmenuindex].submenuindex);
        updatedisplay();
        UI_state=UI_EDIT;
      }
      if (rmenuenc.getButton() == ClickEncoder::Clicked) { // entering save and restore mode
        display.clearDisplay();
        topmenuindex=1;  // not using top menu, just submenus
        menustate=SUBSELECT; // do submenu when button is released
        copy_to_editbuffer(page,lastcontrol); //copy encoder parameters for editing
        topmenu[topmenuindex].submenuindex=0;  // start from the first item
        drawsubmenus();
        drawselector(topmenu[topmenuindex].submenuindex);
        updatedisplay();
        UI_state=UI_LOADSAVE;
      }
      break;

    case UI_EDIT:  // handing edit menu state
      for (n=0; n< NUMENCODERS;++n) {   // see if a button is pressed
        if ((button=enc[n].getButton()) == ClickEncoder::Clicked) break;
      }
      if ((n< NUMENCODERS) || ((t=lmenuenc.getValue()) !=0)) { // or we can scroll thru controls with encoder
        restore_from_editbuffer(page,lastcontrol); // copy edited values back to the encoder parameters
        showencoderLED(page,lastcontrol); // restore control color  
        if (n<NUMENCODERS) lastcontrol=n;    
        else lastcontrol=constrain(lastcontrol+t,0,NUMENCODERS-1);
        copy_to_editbuffer(page,lastcontrol); //copy encoder parameters for editing
        drawsubmenus();
        drawselector(topmenu[topmenuindex].submenuindex);
        updatedisplay();
      }
      if (lmenuenc.getButton() == ClickEncoder::Clicked) { // click to exit menus
        display.clearDisplay();
        restore_from_editbuffer(page,lastcontrol); // copy edited values back to the encoder parameters
        showencoder(page,lastcontrol); // redraw the encoder display
        showencoderLED(page,lastcontrol); // update the LED too
        updatedisplay();
        UI_state=UI_SEND_MIDI;
        while(!digitalRead(LMENU_ENCSW_IN)) delay(10); // loop here till button released 
        flush_encoders();   // toss any encoder messages
      }
      else {
        domenus();
        if ((millis()-LEDtimer) > LEDFLASH_EDIT) {
          LEDtimer=millis();
          if (LEDstate) {
            LEDstate=0;
            LEDS.setPixelColor(lastcontrol,colorpalette[editbuffer.encoder.color]); // flash LED in the current editing color at full brightness
          }
          else {
            LEDstate=1;
            LEDS.setPixelColor(lastcontrol,LED_BLACK);
          }
        }
      }
      break;
    case UI_LOADSAVE:  // handing save/load menu state
      if (lmenuenc.getButton() == ClickEncoder::Clicked) { // click to exit menus
        display.clearDisplay();
        showencoder(page,lastcontrol); // redraw the encoder display
        showencoderLED(page,lastcontrol); // update the LED too
        updatedisplay();
        UI_state=UI_SEND_MIDI;
        while(!digitalRead(LMENU_ENCSW_IN)) delay(10); // loop here till button released 
        flush_encoders();   // toss any encoder messages
      }
      else domenus();
      break;
    default:
      break;
  }  // end switch

 // Serial.printf("enc %d button %d\n",rmenuenc.getValue(),rmenuenc.getButton());
 // delay(100);
  LEDS.show();
}
