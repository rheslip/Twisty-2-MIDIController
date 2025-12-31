


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

const char *menutitle;  // points to title of current menu/submenu
const char *maintitle=" Twisty 2   ";

const char *filesroot="/Samples";  // root of file tree

char filepath[200];  // working file path
#define ROOTLEN  8  // length of root filename
#define MAXFILES 500 // max number of files per directory we can read

int8_t topmenuindex=0;  // keeps track of which top menu item we are displaying
//int8_t fileindex=0;  // keeps track of which file we are displaying
int numfiles=0; // number of files in current directory

enum menumodes{TOPSELECT,SUBSELECT,PARAM_INPUT,FILEBROWSER,WAITFORBUTTONUP}; // UI state machine states
//static int16_t uistate=TOPSELECT; // start out at top menu
static int16_t menustate=SUBSELECT; // start out at sub menu

enum paramtype{TYPE_NONE,TYPE_INTEGER,TYPE_FLOAT, TYPE_TEXT,TYPE_FILENAME}; // parameter display types

// holds file and directory info
struct fileinfo {
	char name[80];
	bool isdir;   // true if directory
  uint32_t size; // not using at the moment 
} files[MAXFILES];

// for sorting filenames in alpha order
int comp(const void *a,const void *b) {
return (strcmp((char *)a,(char *)b));
}
 
	
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
  void (*exithandler)(void);  // function to call on exiting value change
};

// top menus
struct menu {
   const char *name; // menu text
   struct submenu * submenus; // points to submenus for this menu
   int8_t submenuindex;   // stores the index of the submenu we are currently using
   int8_t numsubmenus; // number of submenus - not sure why this has to be int but it crashes otherwise. compiler bug?
};

// dummy variable for menu testing
int16_t dummy;


void testfunc(void) {
  printf("test function %d\n",dummy);
}; // 

 

// ********** menu structs that build the menu system below *********


// text arrays used for submenu TYPE_TEXT fields
// making everything 6 letters justifies text to right side of display
const char * onoff[] = {"   Off","    On"};
const char * ledcolors[] = {"   Red","Orange"," Green","  Aqua","  Blue","Violet"," White"};
const char * actions[] ={"  Load","  Save","Format"};
const char * no_yes[] ={"    No","   Yes"};
const char * enctypes[] ={"    CC"};
const char * switchmodes[] ={"Moment","Toggle"};
const char * switchtypes[] ={"    CC","    PC","  Note"," Reset","Encodr"};
// knob labels that can be assigned via the menus. NOTE label 0 must be "CC" because its the only one that shows the CC number - special case
// labels can be no more than 6 characters or they will interfere with the value display
const char * labels[] ={"    CC","Attack"," Decay","Sustai","Releas","  Freq","  Reso"," Depth","Volume"," Start","  Stop","Resume"};
#define NUM_LABELS 12  // not sure how to calculate this automatically


struct submenu controlparams[] = {
  // name,min,max,step,type,*textfield,*parameter,*handler,*exithandler
  "Enc MIDI Chan.",1,16,1,TYPE_INTEGER,0,&editbuffer.encoder.channel,0,0,
//  "Enc Type",0,0,1,TYPE_TEXT,enctypes,&editbuffer.encoder.type,0,0,  // encoder supports just CC messages for now
  "Enc CC No.",0,127,1,TYPE_INTEGER,0,&editbuffer.encoder.ccnumber,0,0,
  "Enc Label",0,NUM_LABELS-1,1,TYPE_TEXT,labels,&editbuffer.encoder.label,0,0, 
  "Enc Color",0,5,1,TYPE_TEXT,ledcolors,&editbuffer.encoder.color,0,0,
  "Enc Min",0,127,1,TYPE_INTEGER,0,&editbuffer.encoder.minvalue,0,0, 
  "Enc Max",0,127,1,TYPE_INTEGER,0,&editbuffer.encoder.maxvalue,0,0,    
  "Switch MIDI Chan.",1,16,1,TYPE_INTEGER,0,&editbuffer.encswitch.channel,0,0,
  "Switch Mode",0,1,1,TYPE_TEXT,switchmodes,&editbuffer.encswitch.mode,0,0,  // 
  "Switch Type",0,0,1,TYPE_TEXT,switchtypes,&editbuffer.encswitch.type,0,0,  // 
  "Switch CC No.",0,127,1,TYPE_INTEGER,0,&editbuffer.encswitch.ccnumber,0,0,
  "Switch Label",0,NUM_LABELS-1,1,TYPE_TEXT,labels,&editbuffer.encswitch.label,0,0, 
//  "Switch Color",0,5,1,TYPE_TEXT,ledcolors,&editbuffer.encswitch.color,0,0,  // switches are always white
  "Switch Min",0,127,1,TYPE_INTEGER,0,&editbuffer.encswitch.minvalue,0,0, 
  "Switch Max",0,127,1,TYPE_INTEGER,0,&editbuffer.encswitch.maxvalue,0,0,       
};

struct submenu loadsave[] = {
  // name,min,max,step,type,*textfield,*parameter,*handler
  "Slot",1,16,1,TYPE_INTEGER,0,&saverestore_slot,0,0,
  "Action",0,2,1,TYPE_TEXT,actions,&saverestore_action,0,0,
  "Confirm?",0,1,1,TYPE_TEXT,no_yes,&saverestore_confirm,0,save_restore,
};

// top level menu structure - each top level menu contains one submenu
struct menu mainmenu[] = {
  // name,submenu *,initial submenu index,number of submenus
  "",controlparams,0,sizeof(controlparams)/sizeof(submenu),
  "",loadsave,0,sizeof(loadsave)/sizeof(submenu),
 };

#define NUM_MAIN_MENUS sizeof(mainmenu)/ sizeof(menu)
menu * topmenu=mainmenu;  // points at current menu

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
//    display.fillRect(0,TOPMENU_Y,SCREENWIDTH,SCREENHEIGHT-TOPMENU_Y,BLACK); // erase old 
    display.setCursor(TOPMENU_X,TOPMENU_Y);
    display.printf("%s",menutitle);
    int i = (index/TOPMENU_LINES)*TOPMENU_LINES; // which group of menu items to display
    int last = i+NUM_MAIN_MENUS % TOPMENU_LINES; // show only up to the last menu item
    if ((i + TOPMENU_LINES) <= NUM_MAIN_MENUS) last = i+TOPMENU_LINES; // handles case like 2nd of 3 menu pages
 //   int y=TOPMENU_Y+DISPLAY_Y_MENUPAD+DISPLAY_Y_OFFSET;
    int y=DISPLAY_Y_OFFSET;

    for (i; i< last ; ++i) {
      display.setCursor ( TOPMENU_X, y ); 
      display.print(topmenu[i].name);
/*    
	  if (i < NUM_VOICES) {			// first N items are always samples - show the sample filename
      display.printf("%-.22s",sample[voice[topmenuindex].sample].sname);
		 // strncpy(temp,sample[voice[i].sample].sname,DISPLAY_X-3); // 3 columns are used: selector, sample#, space
	  }
*/    
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
/*
		    case TYPE_FILENAME:  // print filename of sample using index in min
		      display.setCursor (SUBMENU_X, y ); // leave room for selector
          display.printf("%-22s",sample[voice[topmenuindex].sample].sname);
        break;
  */
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
//    display.fillRect(0,TOPMENU_Y,SCREENWIDTH,SCREENHEIGHT-TOPMENU_Y,BLACK); // erase old 
    display.fillScreen(BLACK);
    display.setCursor(0,DISPLAY_Y_OFFSET);
//    display.printf("%s",topmenu[topmenuindex].name); // show the menu we came from at top of screen
    int i = (index/SUBMENU_LINES)*SUBMENU_LINES; // which group of menu items to display
    int last = i+len % SUBMENU_LINES; // show only up to the last menu item
    if ((i + SUBMENU_LINES) <= len) last = i+SUBMENU_LINES; // handles case like 2nd of 3 menu pages
    int y=SUBMENU_Y+DISPLAY_Y_MENUPAD;

    for (i; i< last ; ++i) {
      //display.setCursor ( SUBMENU_X, y ); 
      //display.print(sub[i].name);
      //y+=DISPLAY_CHAR_HEIGHT+DISPLAY_Y_MENUPAD;
      drawsubmenu(i);
    }
    updatedisplay();
} 

/*
// get file size 
int16_t get_filesize(char * path) {
  File f, entry;
  f=SD.open(path); // open the path
  entry =  f.openNextFile(); // open the file - seems to be necessary
  return f.size(); // size in bytes
}


// function to get the content of a given folder 
int16_t get_dir_content(char * path) {
  File dir; 
  int16_t nfiles;
  dir = SD.open(path); // open the path
  nfiles=0;
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) break;
 //   Serial.print(entry.name());
    if (entry.isDirectory()) {
 //     Serial.println("/");
	    strcpy(files[nfiles].name,entry.name());
      files[nfiles].name[0]=toupper(files[nfiles].name[0]); // so they sort properly
		  files[nfiles].isdir=true;
      files[nfiles].size=0;
		  ++ nfiles;
    //  printDirectory(entry, numTabs + 1); //recursive call from SD example

    } else {
 //     Serial.print("\t\t");
//      Serial.println(entry.size(), DEC); // files have sizes, directories do not
	    strcpy(files[nfiles].name,entry.name());
      files[nfiles].name[0]=toupper(files[nfiles].name[0]); // so they sort properly
		  files[nfiles].isdir=false;
      files[nfiles].size=entry.size();
		  ++ nfiles;
    }
    entry.close();
    if (nfiles >= (MAXFILES-1)) break; // hit the limit of our directory structure
  }
	qsort (files, nfiles, sizeof(fileinfo), comp);  // put them in alpha order
	strcpy(files[nfiles].name,".."); // last entry is to go up
	files[nfiles].isdir=0; // special case - don't treat it as a directory
	++nfiles;
	return nfiles;  
}

// display a list of files
// index - currently selected file
void drawfilelist( int16_t index) {

  //display.clearDisplay();
  display.fillScreen(BLACK);
//  display.setCursor(0,0);
//  display.printf("S%d /%s",topmenuindex,directory); // show sample # and current directory on top line
  int16_t i = (index/FILEMENU_LINES)*FILEMENU_LINES; // which group of menu items to display
  int last = i+numfiles % TOPMENU_LINES; // show only up to the last menu item
  if ((i + FILEMENU_LINES) <= numfiles) last = i+FILEMENU_LINES; // handles case like 2nd of 3 menu pages
  int y=FILEMENU_Y+DISPLAY_Y_MENUPAD;

  for (i; i< last ; ++i) {
    display.setCursor ( FILEMENU_X, y ); 
	  if (files[i].isdir) display.print("/"); // show its a directory  	
    display.printf("%-.24s",files[i].name);	
    y+=DISPLAY_CHAR_HEIGHT+DISPLAY_Y_MENUPAD;
  }
   // display.display();
} 

// popdir - go up one level in the directory tree
// returns 1 if at top level
bool popdir(void) {
  int16_t p;
  p=strlen(filepath);
  if (p <= ROOTLEN) return true;
  --p; // index of last character in directory string
  while (filepath[p] != '/') --p;
  filepath[p]=0; // null terminate directory string
  return 0;
}
*/

// menu handler
// a run to completion state machine - it never blocks except while waiting for encoder button release
// allows the rest of the application to run while parameters are adjusted
// modded July 2024 for the Pico groovebox - doesn't display top menus, scrolls thru submenus when button1 is pressed with encoder rotation

void domenus(void) {
  int16_t enc;
  int8_t index; 
  char temp[80]; // for file paths
  static int16_t fileindex=0;  // index of selected file/directory   
  static int16_t lastdir=0;  // index of last directory we looked at
  static int16_t lastfile=0;  // index of last file we looked at  
  bool exitflag;


//  ClickEncoder::Button button; 
  
  enc=rmenuenc.getValue();
 // enc_L=Encoder2.getValue();
//  button= Encoder2.getButton();

  // process the menu encoder 
//  enc=P4Encoder.getValue();


  switch (menustate) {
  /*  case TOPSELECT:  // scrolling thru top menu
      if (enc_L !=0) { // scroll thru submenus
        int topmenupage = (topmenuindex) / TOPMENU_LINES;  
        topmenuindex+=enc_L;
        if (topmenuindex <0) topmenuindex=0;  // we don't wrap menus around, just stop at the ends
        if (topmenuindex >=(NUM_MAIN_MENUS -1) ) topmenuindex=NUM_MAIN_MENUS-1; 
        topmenu[topmenuindex].submenuindex=0;  // start from the first item
        drawsubmenus();
        drawselector(topmenu[topmenuindex].submenuindex);  
        if (topmenuindex < NTRACKS) showpattern(topmenuindex); // show the piano roll if this is a track menu
        uistate=SUBSELECT; // do that submenu
      }
      if (!(currtouched & TRACK_BUTTON)) {
        uistate=SUBSELECT; // do submenu when button is released
        topmenu[topmenuindex].submenuindex=0;  // start from the first item
        drawsubmenus();
        drawselector(topmenu[topmenuindex].submenuindex);
        if (topmenuindex < NTRACKS) showpattern(topmenuindex); // show the piano roll if this is a track menu
      } 
       
      break;
      */
    case SUBSELECT:  // 
    /*
      if (enc_L !=0) { // scroll thru submenus
        int topmenupage = (topmenuindex) / TOPMENU_LINES;  
        topmenuindex+=enc_L;
        topmenuindex=constrain(topmenuindex,0,NUM_MAIN_MENUS -1);
        topmenu[topmenuindex].submenuindex=0;  // start from the first item
        drawsubmenus();
        drawselector(topmenu[topmenuindex].submenuindex);  
        if (topmenuindex < NTRACKS) showpattern(topmenuindex); // show the piano roll if this is a track menu  
      }
*/
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
      //if ((rmenuenc.getButton() == ClickEncoder::Clicked)) { // submenu item has been selected so either go back to top or go to change parameter state

	      submenu * sub;
		    sub=topmenu[topmenuindex].submenus; //get pointer to the submenu array
 /*       if (sub[topmenu[topmenuindex].submenuindex].ptype ==TYPE_FILENAME ) { // if filebrowser menu type so we go into file browser
          
          //Serial.printf("path=%s\n",filepath);
          numfiles=get_dir_content(filepath);
			    fileindex=0;
          if (files[0].isdir) drawfilelist(fileindex=lastdir);
			    else drawfilelist(fileindex=lastfile);
          drawselector(fileindex); 
          uistate=FILEBROWSER;
          while(!digitalRead(RMENU_ENCSW_IN));// dosequencers(); // keep sequencers running till button release
        }
        else { */
          undrawselector(topmenu[topmenuindex].submenuindex);
          draweditselector(topmenu[topmenuindex].submenuindex); // show we are editing
          menustate=PARAM_INPUT;  // change the submenu parameter
          while(!digitalRead(RMENU_ENCSW_IN)) delay(10); // loop here till button released 
         //while(rmenuenc.getButton() != ClickEncoder::Open); // loop here till button released
      //  }
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
      //if ((Encoder1.getButton() == ClickEncoder::Clicked)) { // stop changing parameter
        undrawselector(topmenu[topmenuindex].submenuindex);
        drawselector(topmenu[topmenuindex].submenuindex); // show we are selecting again
        menustate=SUBSELECT;
        if (sub[index].exithandler != 0) (*sub[index].exithandler)();  // call the exit handler function
        while(!digitalRead(RMENU_ENCSW_IN)) delay(10); // loop till button released
        
      }   
      break;
/*
	  case FILEBROWSER:  // browse files - file structure is ./<filesroot>/<directory>/<file> ie all files must be in a directory 
      if (enc !=0 ) { // move selector
        int filespage = (fileindex) / FILEMENU_LINES;  
        undrawselector(fileindex);
        fileindex+=enc;
        if (fileindex <0) fileindex=0;  // we don't wrap menus around, just stop at the ends
        if (fileindex >=(numfiles -1) ) fileindex=numfiles-1; 
        if ((fileindex / FILEMENU_LINES) != filespage) {
          drawfilelist(fileindex);  // redraw if we scrolled beyond the menu page
        }
        drawselector(fileindex);    
      }
      if (!digitalRead(R_ENC_SW)) { // file item has been selected 
		    if (files[fileindex].isdir) {  // show nested directory	
          strcat(filepath,"/"); 
          strcat(filepath,files[fileindex].name); // save the current directory
          numfiles=get_dir_content(filepath);
			    fileindex=lastfile;
          drawfilelist(fileindex);
          drawselector(fileindex);
			    while(!digitalRead(R_ENC_SW));// loop here till encoder released, otherwise we go right back into select
		    }
		    else if (fileindex == (numfiles -1)) { // last file is always ".." so go up to directories
          popdir();  // go up one level
          numfiles=get_dir_content(filepath);
			    lastfile=lastdir=0;        // new directory so start at beginning
          drawfilelist(fileindex=lastdir);
          drawselector(lastdir);
			    while(!digitalRead(R_ENC_SW));// 
		    }
		    else {  // we have selected a file so load it
			    exitflag=0;
          while (!digitalRead(R_ENC_SW)) {   // long press to exit - way to get out of a very long file list
            if (Encoder1.getButton() == ClickEncoder::Held) {
					    exitflag=1;
					    break;
				    } 
          }      
			    if (exitflag) {  // don't load file, go back to root dir
				    lastfile=0;
				    strcpy(filepath,filesroot);
			    }
			    else {    
                 
				    lastfile=fileindex;  // remember where we were
				    char temp2[200];
				    strcpy(temp2,filepath);  // build the full file path
				    strcat(temp2,"/");
            strcat(temp2,files[fileindex].name);

// at this point you load file etc

            else strcpy(sample[track].sname,"**File load error**");
          }
			    topmenu[topmenuindex].submenuindex=0;  // restore submenu from the first item
			    drawsubmenus();
			    drawselector(topmenu[topmenuindex].submenuindex); 
          if (topmenuindex < NTRACKS) showpattern(topmenuindex); // show the piano roll if this is a track menu 
			    uistate=SUBSELECT;	
			    while(!digitalRead(R_ENC_SW)); // 
          
		  }
    } // end of case FILEBROWSER
    */
  } // end of case statement
}


