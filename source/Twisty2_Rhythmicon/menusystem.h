


// Copyright 2020 Rich Heslip
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
//
// this is adapted from my XVA1 menu system. this one is bitmapped so its a mix of character and pixel addressing
// Feb 2022 - adapted again as a single encoder menu system - very similar to the Arduino Neu-rah menu system but only 2 levels
// menu items are displayed top to bottom of screen with a title bar
// encoder scrolls menu selector, click to select submenu
// encoder scrolls submenu selector, click to edit parameter
// last submenu item is treated as "back to top menu" so make sure its set up that way
// parameters can be:
// text strings which return ordinals
// integers in range -9999 to +9999, range and increment set in the submenu table
// floats in range -9.99 to +9.99 - floats are displayed but the parameter behind them is an int in the range -999 to +999 so your code has to convert the int to float
// the parameter field in the submenu initializer must point to an integer variable - when you edit the on screen value its this value you are changing
// the handler field in the submenu initializer must be either null or point to a function which is called when you edit the parameter
// Dec 2025 - minor mods for the Twisty 2 controller. ripped out all the file handling stuff because its not needed for this app

// these defs for 128x32 display with 6x8 font
#define SCREENWIDTH 128
#define SCREENHEIGHT 32
#define DISPLAY_X 21  // 21 char display
#define DISPLAY_Y 4   // 4 lines max
#define DISPLAY_CHAR_HEIGHT 8 // character height in pixels - for bitmap displays
#define DISPLAY_CHAR_WIDTH 6 // character width in pixels - for bitmap displays
#define DISPLAY_X_MENUPAD 2   // extra space between menu items in pixels
#define DISPLAY_Y_MENUPAD 3   // extra vertical space between menu items in pixels
#define DISPLAY_Y_OFFSET 0   // offset from top of screen in pixels - TFT is a bit wonky
#define TOPMENU_LINE 0    // line to start menus on
#define TOPMENU_Y (TOPMENU_LINE*(DISPLAY_CHAR_HEIGHT+DISPLAY_Y_MENUPAD)+DISPLAY_Y_OFFSET)   // pixel y position to display top menus
#define TOPMENU_X (1 * DISPLAY_CHAR_WIDTH)   // x pos to display top menus - first character reserved for selector character
#define TOPMENU_LINES 3 // number of menu text lines to display
#define SUBMENU_LINE 0 // line to start sub menus on
#define SUBMENU_Y (SUBMENU_LINE*(DISPLAY_CHAR_HEIGHT+DISPLAY_Y_MENUPAD)+DISPLAY_Y_OFFSET)   // line to display sub menus
#define SUBMENU_X (1 * DISPLAY_CHAR_WIDTH)   // x pos to display sub menus name field
#define SUBMENU_VALUE_X (15 * DISPLAY_CHAR_WIDTH)  // x pos to display submenu values
#define SUBMENU_LINES 3 // number of menu text lines to display
#define FILEMENU_LINES 3 // number of files to show 
#define FILEMENU_X (1 * DISPLAY_CHAR_WIDTH)   // x pos to display file menus - first character reserved for selector character
#define FILEMENU_Y (TOPMENU_LINE*(DISPLAY_CHAR_HEIGHT+DISPLAY_Y_MENUPAD)+DISPLAY_Y_OFFSET)   // pixel y position to display file menus


enum menumodes{TOPSELECT,SUBSELECT,PARAM_INPUT}; // UI state machine states
static int16_t menustate=SUBSELECT; // start out at sub menu

enum paramtype{TYPE_NONE,TYPE_INTEGER,TYPE_FLOAT, TYPE_TEXT}; // parameter display types

// submenus 
struct submenu {
  const char *name; // display name
  int16_t min;  // min value of parameter
  int16_t max;  // max value of parameter
  int16_t step; // step size. if 0, don't print ie spacer
  enum paramtype ptype; // how its displayed
  const char ** ptext;   // points to array of text for text display
  int16_t *parameter; // value to modify
  void (*handler)(void);  // function to call on value change
//  void (*exithandler)(void);  // function to call on exiting value change
};

// top menus
struct menu {
   const char *name; // menu text
   struct submenu * submenus; // points to submenus for this menu
   int8_t submenuindex;   // stores the index of the submenu we are currently using
   int8_t numsubmenus; // number of submenus - not sure why this has to be int but it crashes otherwise. compiler bug?
};

// timer and flag for managing temporary messages
#define MESSAGE_TIMEOUT 1500
long messagetimer;
bool message_displayed;

int16_t enab1 =1;
int16_t enab2 =2;
int16_t level1,pan1;

int16_t nul;    // dummy parameter and function for testing
void dummy( void) {}

// ********** menu structs that build the menu system below *********

// text arrays used for submenu TYPE_TEXT fields
const char * textoffon[] = {"   Off", "    On"};
const char * textstepmode[] = {" Fwd", " Rev","Pong","Walk","Rand"};
//{CHROMATIC,MAJOR,MINOR,HARMONIC_MINOR,MAJOR_PENTATONIC,MINOR_PENTATONIC,DORIAN,PHRYGIAN,LYDIAN,MIXOLYDIAN};
const char * scalenames[] = {"Chroma"," Major", " Minor","HarMin","MajPen","MinPen","Dorian","Phrygi","Lydian","Mixoly"};
//const char * textrates[] = {" 8x"," 6x"," 4x"," 3x", " 2x","1.5x"," 1x","/1.5"," /2"," /3"," /4"," /5"," /6"," /7"," /8"," /9"," /10"," /11"," /12"," /13"," /14"," /15"," /16"," /32"," /64","/128"};

// NOTE that the order and number of the text menus much match the graphical UI pages
// ie we keep the graphic display and its associated text menus in sync - uses variable UIpage for main menus - notes, gates etc
// current_track is the index of the sequence we are editing which indexes into the submenus ie note 1, note 2
// menus are created at compile time so we have to point to each sequencer array parameter individually
struct submenu note1params[] = {
  // name,longname,min,max,step,type,*textfield,*parameter,*handler
//  "RATE",0,25,-1,TYPE_TEXT,textrates,&notes[0].divider,0,

  "Root 1",1,115,1,TYPE_INTEGER,0,&notes[0].root,0,
  "Scale 1",0,9,1,TYPE_TEXT,scalenames,&notes[0].scale,0,
  "Step Mode",0,4,1,TYPE_TEXT,textstepmode,&notes[0].stepmode,0,
  "MIDI Out 1",1,16,1,TYPE_INTEGER,0,&MIDIoutputchannel[0],0,
  "MIDI In 1",1,16,1,TYPE_INTEGER,0,&MIDIinputchannel[0],0,
  "Root 2",1,115,1,TYPE_INTEGER,0,&notes[1].root,0,
  "Scale 2",0,9,1,TYPE_TEXT,scalenames,&notes[1].scale,0,
  "Step Mode",0,4,1,TYPE_TEXT,textstepmode,&notes[1].stepmode,0,
  "MIDI Out 2",1,16,1,TYPE_INTEGER,0,&MIDIoutputchannel[1],0,
  "MIDI In 2",1,16,1,TYPE_INTEGER,0,&MIDIinputchannel[1],0,
  "Root 3",1,115,1,TYPE_INTEGER,0,&notes[2].root,0,
  "Scale 3",0,9,1,TYPE_TEXT,scalenames,&notes[2].scale,0,
  "Step Mode",0,4,1,TYPE_TEXT,textstepmode,&notes[2].stepmode,0,
  "MIDI Out 3",1,16,1,TYPE_INTEGER,0,&MIDIoutputchannel[2],0,
  "MIDI In 3",1,16,1,TYPE_INTEGER,0,&MIDIinputchannel[2],0,
  " BPM",20,240,1,TYPE_INTEGER,0,&bpm,0,
  "Bat Voltage",0,0,1,TYPE_FLOAT,0,&batteryvoltage,0,  // battery voltage displayed in menu - no screen real estate left on main screen


};



// top level menu structure - each top level menu contains one submenu
struct menu mainmenu[] = {
  // name,submenu *,initial submenu index,number of submenus
  "Note 1",note1params,0,sizeof(note1params)/sizeof(submenu),

};

#define NUM_MAIN_MENUS sizeof(mainmenu)/ sizeof(menu)
menu * topmenu=mainmenu;  // points at current menu
int16_t topmenuindex=0;  // keeps track of which top menu item we are displaying

// highlight the currently selected menu item
void drawselector( int8_t index) {
  int line = index % TOPMENU_LINES;
  display.setCursor (0, TOPMENU_Y+DISPLAY_Y_MENUPAD+line*(DISPLAY_CHAR_HEIGHT+DISPLAY_Y_MENUPAD));
  display.print(">"); 
  updatedisplay();
}

// highlight the currently selected menu item as being edited
void draweditselector( int8_t index) {
  int line = index % TOPMENU_LINES;
  display.setCursor (0, TOPMENU_Y+DISPLAY_Y_MENUPAD+line*(DISPLAY_CHAR_HEIGHT+DISPLAY_Y_MENUPAD) );
 // display.setTextColor(RED,BLACK);
  display.print("*"); 
  display.setTextColor(WHITE,BLACK);
  updatedisplay();
}

// dehighlight the currently selected menu item
void undrawselector( int8_t index) {
  int line = index % TOPMENU_LINES;
  display.setCursor (0, TOPMENU_Y+DISPLAY_Y_MENUPAD+line*(DISPLAY_CHAR_HEIGHT+DISPLAY_Y_MENUPAD) );
  display.print(" "); 
  updatedisplay();
}

// display the top menu
// index - currently selected top menu
void drawtopmenu( int8_t index) {
    display.fillScreen(BLACK);
    display.setCursor(TOPMENU_X,TOPMENU_Y);
    int i = (index/TOPMENU_LINES)*TOPMENU_LINES; // which group of menu items to display
    int last = i+NUM_MAIN_MENUS % TOPMENU_LINES; // show only up to the last menu item
    if ((i + TOPMENU_LINES) <= NUM_MAIN_MENUS) last = i+TOPMENU_LINES; // handles case like 2nd of 3 menu pages
    int y=DISPLAY_Y_OFFSET;

    for (i; i< last ; ++i) {
      display.setCursor ( TOPMENU_X, y ); 
      display.print(topmenu[i].name);
      y+=DISPLAY_CHAR_HEIGHT+DISPLAY_Y_MENUPAD;
    }
    updatedisplay();
} 

// display a sub menu item and its value
// index is the index into the current top menu's submenu array

void drawsubmenu( int8_t index) {
    submenu * sub;
    sub=topmenu[topmenuindex].submenus; //get pointer to the submenu array
    // print the name text
    int y= SUBMENU_Y+DISPLAY_Y_MENUPAD+(index % SUBMENU_LINES)*(DISPLAY_CHAR_HEIGHT+DISPLAY_Y_MENUPAD); // Y position of this menu index
    display.setCursor (SUBMENU_X,y) ; // set cursor to parameter name field
    display.print(sub[index].name); 
    
    // print the value
    display.setCursor (SUBMENU_VALUE_X, y ); // set cursor to parameter value field
    display.print("      "); // erase old value
    display.setCursor (SUBMENU_VALUE_X, y ); // set cursor to parameter value field
    if (sub[index].step !=0) { // don't print dummy parameter 
      int16_t val=*sub[index].parameter;  // fetch the parameter value
      char temp[7];
      switch (sub[index].ptype) {
        case TYPE_INTEGER:   // print the value as an unsigned integer    
          sprintf(temp,"%6d",val); // lcd.print doesn't seem to print uint8 properly
          display.print(temp);  
          display.print(" ");  // blank out any garbage
          break;
        case TYPE_FLOAT:   // print the int value as a float  
          sprintf(temp,"%1.2f",(float)val/1000); // menu should have int value between -1000 to +1000 so float is -1 to +1
          display.print(temp);  
          display.print(" ");  // blank out any garbage
          break;
        case TYPE_TEXT:  // use the value to look up a string
          if (val > sub[index].max) val=sub[index].max; // sanity check
          if (val < 0) val=0; // min index is 0 for text fields
          display.print(sub[index].ptext[val]); // parameter value indexes into the string array
          display.print(" ");  // blank out any garbage
          break;
        default:
        case TYPE_NONE:  // blank out the field
          display.print("     ");
          break;
      } 
    }
    updatedisplay(); 
}

// display sub menus of the current topmenu

void drawsubmenus() {
    int8_t index,len;
    index= topmenu[topmenuindex].submenuindex; // submenu field index
    len= topmenu[topmenuindex].numsubmenus; // number of submenu items
    submenu * sub=topmenu[topmenuindex].submenus; //get pointer to the current submenu array
    display.fillScreen(BLACK);
    display.setCursor(0,DISPLAY_Y_OFFSET);
//    display.printf("%s",topmenu[topmenuindex].name); // show the menu we came from at top of screen
    int i = (index/SUBMENU_LINES)*SUBMENU_LINES; // which group of menu items to display
    int last = i+len % SUBMENU_LINES; // show only up to the last menu item
    if ((i + SUBMENU_LINES) <= len) last = i+SUBMENU_LINES; // handles case like 2nd of 3 menu pages
    int y=SUBMENU_Y+DISPLAY_Y_MENUPAD;

    for (i; i< last ; ++i) {
       drawsubmenu(i);
    }
    updatedisplay();
} 


// menu handler
// a run to completion state machine - it never blocks except while waiting for encoder button release
// allows the rest of the application to run while parameters are adjusted
// Dec 2025 - modded for Twisty 2 menus - only submenu level is used

void domenus(void) {
  int16_t enc;
  int8_t index; 

  
  enc=rmenuenc.getValue();

  switch (menustate) {

    case SUBSELECT:  // 
      if (enc !=0 ) { // move selector
        int submenupage = topmenu[topmenuindex].submenuindex / SUBMENU_LINES;  
        undrawselector(topmenu[topmenuindex].submenuindex);
        topmenu[topmenuindex].submenuindex+=enc;
        if (topmenu[topmenuindex].submenuindex <0) topmenu[topmenuindex].submenuindex=0;  // we don't wrap menus around, just stop at the ends
        if (topmenu[topmenuindex].submenuindex >=(topmenu[topmenuindex].numsubmenus -1) ) topmenu[topmenuindex].submenuindex=topmenu[topmenuindex].numsubmenus -1; 
        if ((topmenu[topmenuindex].submenuindex / SUBMENU_LINES) != submenupage) {
          drawsubmenus();  // redraw if we scrolled beyond the menu page
        }
        drawselector(topmenu[topmenuindex].submenuindex);   
      } 
      if (!digitalRead(RMENU_ENCSW_IN)) { // submenu item has been selected so either go back to top or go to change parameter state
	      submenu * sub;
		    sub=topmenu[topmenuindex].submenus; //get pointer to the submenu array
        undrawselector(topmenu[topmenuindex].submenuindex);
        draweditselector(topmenu[topmenuindex].submenuindex); // show we are editing
        menustate=PARAM_INPUT;  // change the submenu parameter
        while(!digitalRead(RMENU_ENCSW_IN)) delay(10); // loop here till button released 
      }  
      break;
    case PARAM_INPUT:  // changing value of a parameter
      submenu * sub=topmenu[topmenuindex].submenus; //get pointer to the current submenu array
      index= topmenu[topmenuindex].submenuindex; // submenu field index
      if (enc !=0 ) { // change value      
        int16_t temp=*sub[index].parameter + enc*sub[index].step; // menu code uses ints - convert to floats when needed
        if (temp < (int16_t)sub[index].min) temp=sub[index].min;
        if (temp > (int16_t)sub[index].max) temp=sub[index].max;
        *sub[index].parameter=temp;
        if (sub[index].handler != 0) (*sub[index].handler)();  // call the handler function
        drawsubmenu(index);
      }
      if (!digitalRead(RMENU_ENCSW_IN)) { // stop changing parameter
        undrawselector(topmenu[topmenuindex].submenuindex);
        drawselector(topmenu[topmenuindex].submenuindex); // show we are selecting again
        menustate=SUBSELECT;
        //if (sub[index].exithandler != 0) (*sub[index].exithandler)();  // call the exit handler function
        while(!digitalRead(RMENU_ENCSW_IN)) delay(10); // loop till button released
        
      }   
      break;
  } // end of case statement
}


