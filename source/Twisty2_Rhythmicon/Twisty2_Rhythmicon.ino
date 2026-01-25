
// Copyright 2026 Rich Heslip
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
 * MIDI sequencer using RP2040
 * uses 16 multiplexed encoders for step data input
 * a separate encoder for menu inputs
 * Graphics and text UI is processed on core 0, sequencer clocking and MIDI processed by core 1
 * Jan 2026 - implemented a Subharmonicon like sequencer from the Twisty 2 code and the one I wrote for the Teensy 4
 */



#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include "Clickencoder.h"
#include <Adafruit_NeoPixel.h>
#include <Control_Surface.h>

// BT MIDI works (with the exception of note messages) on the Twisty2 app but does not work here. maybe because I'm using both cores?
#define BLUETOOTH  // define for bluetooth MIDI 
#ifdef BLUETOOTH
#include <MIDI_Interfaces/BluetoothMIDI_Interface.hpp>
#endif


#define TRUE 1
#define FALSE 0

// I used mostly int16 types - its what the menu requires and the compiler seems to deal with basic conversions

//int16_t steps = 8; // initial number of steps


// graphical editing UI states - UI logic assumes an init state and an edit state for each
// text parameter editing system has its own state machine for historical reasons
// the text menu system requires parameters to be 16 bit integers which is why most of the data types are int16

enum UISTATES {RUN,DISPLAYOFF,DISPLAYON};
int16_t UI_state=RUN; // initial UI state
bool menumode=0;  // when true we are in the text menu system

#define BATTERY_SCALE 3300*2/1024  // 3.3v reference, 10 bit A/D with external 2:1 divider. * 1000 so menus can display as a float
#define BATVOLTAGE 28  // battery voltage input 
int16_t batteryvoltage; // integer but displayed as a float in menus

#define DEFAULT_VELOCITY 120
#define NTRACKS 3 // number of sequencer tracks - each track has notes, gates etc
#define NUM_CLOCKS 4
#define SEQ_STEPS 4 // 4 step sequencer
int16_t current_track=0; // track we are editing
int16_t MIDIinputchannel[NTRACKS] = {1,2,3}; // midi channel to use for sequencer notes
int16_t MIDIoutputchannel[NTRACKS] = {1,2,3}; // midi channel to use for sequencer notes
int16_t trackenabled[NTRACKS] = {1,1,1}; // 1 if track on is 1, 0 if off

#define MAX_DIVIDER 128  // maximum clock divider
bool rhythmclks[NTRACKS+1][NUM_CLOCKS]= {1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1}; // 1 to enable clock source to track. last 4 are always set to show divider LEDs

const char * notenames[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B","C"};

#define DISPLAY_BLANK_MS 120*1000  // display blanking time
int32_t displaytimer ; // display blanking timer

#define TEMPO    120  // startup tempo
#define PPQN 24  // clocks per quarter note
#define PPQN_DIV 6 // divider for 16th notes
#define GATE_DIV 3  // gate time is half the note duration
#define MIDDLE_C 60 // MIDI note used as default sequencer value and as a base for incoming MIDI offsets

int16_t bpm = TEMPO;
int32_t lastMIDIclock; // timestamp of last MIDI clock
int16_t MIDIclocks=PPQN*2; // midi clock counter
int16_t MIDIsync = 16;  // number of clocks required to sync BPM
int16_t useMIDIclock = 0; // true if we are using MIDI clock

enum CONTROLSTATES {IDLE,STARTUP,RUNNING,RUNJUSTSYNCED,SHUTDOWN}; // control state machine states
int16_t controlstate=RUNNING; // state machine state

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

int32_t divcolors[NUM_CLOCKS]={LED_RED,LED_GREEN,LED_AQUA,LED_VIOLET}; // colors indicate which clock divider is in use

Adafruit_NeoPixel LEDS(NUMPIXELS, LEDPIN, NEO_GRB + NEO_KHZ800);

// sequencer object
//StepSeq seq = StepSeq(128);

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

//#define START_STOP_BUTTON 5  // start/stop button
//#define SHIFT_BUTTON 28      // UI function shift button

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

#define OLED_DISPLAY   // for graphics conditionals

#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 32  // OLED display height, in pixels
#define SCREEN_BUFFER_SIZE (SCREEN_WIDTH * ((SCREEN_HEIGHT + 7) / 8))

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET);


// use Control Surface MIDI
USBMIDI_Interface usbMIDI;

HardwareSerialMIDI_Interface serialMIDI {Serial1, MIDI_BAUD};

#ifdef BLUETOOTH
// Instantiate a MIDI over BLE interface
BluetoothMIDI_Interface bleMIDI;
#endif

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


// midi related stuff - after initialization all MIDI stuff runs on core1 for timing accuracy
// splitting it across both cores causes MidiUSB to hang eventually

void sendnoteOn(uint8_t channel,uint8_t pitch, uint8_t velocity) {
  MIDIAddress midiaddress ={pitch,Channel_1 + (channel-1)}; // control surface library requires this form of MIDI addressing -I'm not a fan of the design but its the only Arduino BLE MIDI library I could find
  usbMIDI.sendNoteOn(midiaddress, velocity);
  serialMIDI.sendNoteOn(midiaddress, velocity);
#ifdef BLUETOOTH
  bleMIDI.sendNoteOn(midiaddress, velocity);
#endif
}

void sendnoteOff(uint8_t channel, uint8_t pitch,uint8_t velocity) {
  MIDIAddress midiaddress= {pitch,Channel_1 + (channel-1)};
  usbMIDI.sendNoteOff(midiaddress, velocity);
  serialMIDI.sendNoteOff(midiaddress, velocity);
#ifdef BLUETOOTH
  bleMIDI.sendNoteOff(midiaddress, velocity);
#endif
}

// message 0x0B control change.
// 2nd parameter is the control number number (0-119).
// 3rd parameter is the control value (0-127).

void sendcontrolChange(uint8_t channel, uint8_t control, uint8_t value) {
  MIDIAddress midiaddress= {control,Channel_1 + (channel-1)};  
  usbMIDI.sendControlChange(midiaddress, value); 
  serialMIDI.sendControlChange(midiaddress, value); 
#ifdef BLUETOOTH
  bleMIDI.sendControlChange(midiaddress, value); 
#endif
}

// message program change.
// 2nd parameter is the PC value (0-127).

void sendprogramChange(uint8_t channel, uint8_t value) {
  MIDIAddress midiaddress= {value,Channel_1 + (channel-1)};  // confusing way of sending MIDI messages
  usbMIDI.sendProgramChange(midiaddress); 
  serialMIDI.sendProgramChange(midiaddress); 
#ifdef BLUETOOTH
  bleMIDI.sendProgramChange(midiaddress); 
#endif
}


// set up as include files because I'm too lazy to create proper header and .cpp files
#include "scales.h"   //
#include "seq.h"   // has to come after midi note on/of
#include "menusystem.h"  // has to come after display and encoder objects creation
#include "MIDIcallbacks.h"

// these functions are here to avoid forward references. should really do proper include files!



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

// display rhythm divider value on OLED
void showrhythm(int16_t r){
  display.setCursor(SCREENWIDTH/NUM_CLOCKS*r,24);
  display.print("     ");
  display.setCursor(SCREENWIDTH/NUM_CLOCKS*r,24);
  display.printf("/%d",rhythm[r].divider);
  updatedisplay();  
}

// show all rhythm dividers
void showrhythms(void) {
  for (int16_t i=0; i< NUM_CLOCKS;++i) showrhythm(i);
}

// show note value on OLED
void shownote(int16_t track, int16_t index){
  display.setCursor(SCREENWIDTH/SEQ_STEPS*index,track*8); 
  display.print("     ");
  display.setCursor(SCREENWIDTH/SEQ_STEPS*index,track*8); 
  int16_t notenumber=constrain(quantize(notes[track].val[index]+notes[track].root,scales[notes[track].scale],notes[track].root),0,127);
 // Serial.printf("track %d index %d notenumber %d note %d octave %d\n",track,index, notenumber,notenumber%12,notenumber/12-2);
  display.printf("%s%d",notenames[notenumber%12],notenumber/12-2); 
  updatedisplay();   
}

// show all note values on OLED
void shownotes(void){
  for (int16_t track=0; track< NTRACKS;++track)
    for (int16_t step=0; step< SEQ_STEPS;++step) shownote(track,step);
}

// show LED color for encoder - this depends on what's clocking it
// last 4 encoders are the clock dividers so their LEDs are always on

void showLED(int16_t enc) {
  if (rhythmclks[enc/NUM_CLOCKS][enc%NUM_CLOCKS]) LEDS.setPixelColor(enc,divcolors[enc%NUM_CLOCKS]);
  else LEDS.setPixelColor(enc,LED_BLACK);
}

// update all LEDs
void showLEDs(void) {
  for (int16_t i=0; i< NUMENCODERS;++i) showLED(i);
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

void setup() {
  Serial.begin(115200);

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
 
  LEDS.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  LEDS.show();

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    fatalerror(""); // Don't proceed, loop forever
  }
  display.setRotation(2);
  display.clearDisplay();
  display.setTextSize(1);

	display.setTextColor(WHITE,BLACK); // foreground, background  
  display.setCursor(16,10);
  display.printf("PicoRhythmicon\n"); // power on message
  display.display();
  delay(3000);

 // display.setTextSize(1);

#ifdef BLUETOOTH
  bleMIDI.setName("PicoRythmicon");
#endif

  MIDI_Interface::beginAll();


  displaytimer=millis(); // reset display blanking timer

  showLEDs(); // show startup LED state
  shownotes();
  showrhythms();

  // attach MIDI message handler functions
  usbMIDI.setCallbacks(callback); // Attach the custom callbacks
  bleMIDI.setCallbacks(callback); // Attach the custom callbacks

}

// first Pico core does UI etc - not super time critical
void loop() {
  
  ClickEncoder::Button button;
  int16_t encvalue,edited_step,edited_val;

  if ((millis()-displaytimer) > DISPLAY_BLANK_MS) {
    UI_state=DISPLAYOFF;  // 
  } 

  if (menumode) {  // in menu mode we just loop here doing menus - a bit kludgy

    domenus();  // call the text menu state machine
    batteryvoltage=analogRead(BATVOLTAGE)*BATTERY_SCALE; // update the battery voltage
    if ((lmenuenc.getButton() == ClickEncoder::Clicked) || (rmenuenc.getButton() == ClickEncoder::DoubleClicked) ) { // exit menu mode
      UI_state=RUN; // exiting text menus so force a redraw
      menumode=FALSE;
      display.clearDisplay();
      shownotes();
      showrhythms();
      flush_encoders();   // toss any encoder messages to avoid race conditions
    }
//    displaytimer=millis(); // reset display blanking timer so menus don't blank
  }
  
 if (!menumode) {  // do the UI state machine

    if (rmenuenc.getButton() == ClickEncoder::Clicked) { // enter menu mode
      display.fillScreen(BLACK); // erase screen
      topmenuindex=0; // 
      topmenu[topmenuindex].submenuindex=0;  // start from the first item
      drawsubmenus();
      drawselector(topmenu[topmenuindex].submenuindex);
      updatedisplay();
      flush_encoders();   // toss any encoder messages
      menumode=TRUE; // shift button toggles onscreen menus
      UI_state=DISPLAYON; // because we will fall into the UI state machine below
    }

// UI state machine
    switch (UI_state) {
      case RUN:  // core 0 running UI, display on
        break; 
      case DISPLAYON: // display was blanked, redraw
        display.display(); 
        UI_state=RUN;
        break;
      case DISPLAYOFF: // display timed out, blank display
        blankdisplay(); 
        UI_state=RUN;
        break;
      

      default:
        UI_state = RUN;
    }
/*
    if (encvalue=lmenuenc.getValue()) { // scroll thru UI pages
      displaytimer=millis(); // reset display blanking timer
      if (!digitalRead(LMENU_ENCSW_IN)) { // button value not working for some reason
        current_track+=encvalue;
        current_track=constrain(current_track,0,NTRACKS-1); // handle wrap around
        UI_state=UIpages[UIpage]; // forces redraw
      }
      else {
        UIpage+=encvalue;  // next page
        UIpage=constrain(UIpage,0,NUMUIPAGES-1); // handle wrap around
        UI_state=UIpages[UIpage];
      //Serial.printf("encvalue= %d UIpage=%d UI_state=%d\n",encvalue,UIpage,UI_state);
      }
    }
*/
    button = lmenuenc.getButton();
    if (button == ClickEncoder::Clicked) { // toggle play/stop
      if (controlstate == RUNNING) controlstate=IDLE;  // this core writes, other core only reads - multicore safe
      else controlstate= RUNNING;
      UI_state=DISPLAYON; // redraw screen if it was blanked
    }

    if (button == ClickEncoder::DoubleClicked) { // sync sequencers
      rp2040.fifo.push(0);  // send a sync message to other core which runs sequencers
      UI_state=DISPLAYON; // redraw screen if it was blanked
    }

    for (int16_t trk=0; trk< NTRACKS; ++trk) { // encoder buttons enable and disable clock sources
      for (int16_t clk=0; clk< NUM_CLOCKS; ++clk) { 
        int16_t index=trk*NUM_CLOCKS +clk;
        button = enc[index].getButton();
        if (button == ClickEncoder::Clicked) { // toggle clock source on/off
          if (rhythmclks[trk][clk]) rhythmclks[trk][clk]=0; 
          else rhythmclks[trk][clk]=1;
          showLED(index);  // update LED state too
          UI_state=DISPLAYON; // redraw screen if it was blanked
        }
        int16_t val;
        if ((val=enc[index].getValue()) !=0) { // change note values if encoders changed
          notes[trk].val[clk]=constrain(notes[trk].val[clk]+val,-24,24); // allow 2 octave range
          shownote(trk,clk);
          UI_state=DISPLAYON; // redraw screen if it was blanked
          
        }
      }
    }
    for (int16_t i=NTRACKS*NUM_CLOCKS; i< NUMENCODERS;++i) { // last 4 encoders set clock dividers
      int16_t val;
      if ((val=enc[i].getValue()) !=0) { // change clock dividers if encoder changed
        rhythm[i%NUM_CLOCKS].divider=constrain(rhythm[i%NUM_CLOCKS].divider-val,1,MAX_DIVIDER); // divider range is 1-16, CCW increases divider as on SubHarmonicon
        showrhythm(i%NUM_CLOCKS);
        UI_state=DISPLAYON; // redraw screen if it was blanked
      }
    }
  }

}

// second core setup
// second core dedicated to clock and MIDI processing
void setup1() {
  delay (1000); // wait for main core to start up peripherals

}

// second core dedicated to clocks and note on/off for timing accuracy - graphical UI causes redraw delays etc
// implemented as a simple state machine
// start button toggles sequencers on and off
// shift + start button resyncs sequencers
void loop1(){

  LEDS.show(); // update LED display

  MIDI_Interface::updateAll(); // Update the Control Surface MIDI interfaces

// multicore safe messages from core1 to core2 via the fifo

  while (rp2040.fifo.available()) { // get MIDI command, channel# = voice#
    rp2040.fifo.pop(); // only one command - sync
    sync_sequencers();
    showLEDs();  // update LEDs which probably changed
  }

  if (controlstate==RUNNING) do_clocks();

}

