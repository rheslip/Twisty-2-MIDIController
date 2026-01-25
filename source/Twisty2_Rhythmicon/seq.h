// sequencer related definitions and structures
// Jan 2025 stripped dramtically and modified for Subharmonicon like behaviour 


uint32_t clocktimer = 0; // clock rate in ms

enum STEPMODE {FORWARD,BACKWARD,PINGPONG,RANDOMWALK,RANDOM};

// clocks are setup for 24ppqn MIDI clock
struct seqclock {
  int16_t divider; // clock divider 1-15
  int16_t counter; // clock countdown counter
  int16_t ppqn_counter; // PPQN counter - 24 clocks
};

// rhythm generators - divide down the PPQN clock
seqclock rhythm[NUM_CLOCKS] = {
	1,		// divider
	1,		// counter
	PPQN_DIV,		// ppqn counter = 16th notes
	
	1,		// divider
	1,		// counter
	PPQN_DIV,		// ppqn counter

	1,		// divider
	1,		// counter
	PPQN_DIV,		// ppqn counter

	1,		// divider
	1,		// counter
	PPQN_DIV,		// ppqn counter
}	;
	
struct sequencer { // anything modified by a menu must be int16
  int8_t val[SEQ_STEPS];  // values of note offsets from root 
  int8_t index;    // index of step we are on
  int8_t lastnotesent; // for doing note offs
  int16_t stepmode;    // step mode - fwd, backward etc
  int16_t state;    // state - used for step modes  
  int16_t root;   // "root" note - note offsets are relative to this. also used for euclidean offset and CC number
  int16_t offset; // offset value from external MIDI
  int16_t scale;  // index of scale to apply
  int16_t gatecounter; // for gate on/off timing
  int16_t gateduration; // duration in PPQN clocks
};

// notes are stored as offsets from the root 
sequencer notes[NTRACKS] = {
  0,0,0,0,  // initial data
  0,   // step index
  0,  // last note sent
  FORWARD, // step mode
  0,     // state - used for step modes
  60,   // root note
  0,    // offset
  0,    // chromatic scale
  0,   // gate counter
  GATE_DIV,   // gate duration derived from PPQN clock

  0,0,0,0,  // initial data
  0,   // step index
  0,  // last note sent
  FORWARD, // step mode
  0,     // state - used for step modes
  60,   // root note
  0,    // offset
  0,    // chromatic scale
  0,   // gate counter
  GATE_DIV,   // gate duration derived from PPQN clock

    0,0,0,0,  // initial data
  0,   // step index
  0,  // last note sent
  FORWARD, // step mode
  0,     // state - used for step modes
  60,   // root note
  0,    // offset
  0,    // chromatic scale
  0,   // gate counter
  GATE_DIV,   // gate duration derived from PPQN clock
};


// clock all the clock dividers
// called at PPQN rate
void clocktick () {
  bool clked[NTRACKS];
  int16_t smallestdivider;

  for (uint8_t track=0; track<NTRACKS;++track) { // gate counter is in PPQN clocks
    if (notes[track].gatecounter > 0) {
      --notes[track].gatecounter;
      if (notes[track].gatecounter ==0) {
        sendnoteOff(MIDIoutputchannel[track],notes[track].lastnotesent,0);
      }     
    }
  }

  clked[0]=clked[1]=clked[2]=false;
  for (uint8_t i=0; i<NUM_CLOCKS;++i) { // clock the sequencers from the rhythm generators
	  if ((--rhythm[i].ppqn_counter) ==0) {
		  rhythm[i].ppqn_counter=PPQN_DIV;
		  if ((--rhythm[i].counter) == 0) {
			  rhythm[i].counter=rhythm[i].divider;
        smallestdivider=MAX_DIVIDER;
        for (int8_t track=0;track<NTRACKS;++track) {
          if (rhythmclks[track][i]) {  // if this is a clock source for this track
            for (uint8_t clk=0; clk<NUM_CLOCKS;++clk) { 
              if ((rhythm[clk].divider < smallestdivider) && (rhythmclks[track][clk])) smallestdivider=rhythm[clk].divider; // find the fastest clock - to calculate gate time
            }
            if (!clked[track]) {
              clked[track]=true;  // flag seq0 as clocked - we don't want to clock it multiple times if rhythm clocks happen at the same time
              // Serial.printf("track 0 clock %d tick index %d\n",i,notes[0].index);
              showLED(notes[track].index+track*SEQ_STEPS); // restore old color
            switch (notes[track].stepmode) {
                  case FORWARD:
                    ++notes[track].index;
                    if (notes[track].index >= SEQ_STEPS) notes[track].index=0;
                    break;
                  case BACKWARD:
                    --notes[track].index;
                    if (notes[track].index < 0) notes[track].index=SEQ_STEPS-1;
                    break;
                  case PINGPONG:
                    if (notes[track].state == FORWARD) {
                      ++notes[track].index;
                      if (notes[track].index >= (SEQ_STEPS-1)) {
                        notes[track].index=SEQ_STEPS-1;
                        notes[track].state=BACKWARD;
                      }
                    }
                    else {
                      --notes[track].index;
                      if (notes[track].index < 0) {
                        notes[track].index=1;
                        notes[track].state=FORWARD;
                      }
                    }
                    break;
                    case RANDOMWALK:
                      notes[track].index+=random(-1,2); // range of -1 to +1
                      notes[track].index=constrain(notes[track].index,0,SEQ_STEPS-1);
                    break;
                    case RANDOM:
                      notes[track].index=random(0,SEQ_STEPS);
                    break;        
                  default:
                    break;
                }             
           //   ++notes[track].index;
           //   if (notes[track].index >= SEQ_STEPS) notes[track].index=0;
              if (notes[track].gatecounter !=0) sendnoteOff(MIDIoutputchannel[track],notes[track].lastnotesent,0); // don't leave notes on - could happen with multiple clock sources
              notes[track].lastnotesent=constrain(quantize(notes[track].val[notes[track].index]+notes[track].root+notes[track].offset,scales[notes[track].scale],notes[track].root),0,127);
              sendnoteOn(MIDIoutputchannel[track],notes[track].lastnotesent,DEFAULT_VELOCITY);
              notes[track].gatecounter=smallestdivider*PPQN/PPQN_DIV; // initialize gate timer from fastest clock period *** thought this would be half this value for 50% gate - something I'm not getting here 
              LEDS.setPixelColor(notes[track].index+track*SEQ_STEPS,LED_WHITE);  // turn on seq led
            //  Serial.printf("div 0 %d smallest %d %d\n",rhythm[0].divider,smallestdivider,notes[track].gatecounter);
            }
          }
        }
		  }
    }
	}
}
 

// must be called regularly for sequencer to run
void do_clocks(void) {
  uint32_t clockperiod= (uint32_t)(((60.0/(float)bpm)/PPQN)*1000);
  if ((millis() - clocktimer) > clockperiod) {
    clocktimer=millis(); 
    clocktick();
  }
}

// send noteoff for all notes
void all_notes_off(void) {
  for (uint8_t track=0; track<NTRACKS;++track) {
    sendnoteOff(MIDIoutputchannel[track],notes[track].lastnotesent,0); // turn the note off
  }
}

// resets all clock counters and indices to get everything back in sync
void sync_sequencers(void){
  for (int track=0; track<NTRACKS;++track) {
    notes[track].index=0;
  }
  for (int i=0; i<NUM_CLOCKS;++i) {
    rhythm[i].ppqn_counter=PPQN;
    rhythm[i].counter=rhythm[i].divider;
  }
}

