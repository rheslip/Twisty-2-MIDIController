
// Custom MIDI callback that handles (or prints) incoming messages.
// not sure why author of Control Surface chose to put callback functions in a structure but it seems to work
// most of this cut from the Control Surface "MIDI-Input-Fine-Grained-All-Callbacks" example
//
// there is some risk in processing MIDI start/stop etc on on core 0 since core 1 could also be using MIDI
// core 0 syncing sequencers while core 1 is doing clocks is a bit dicey
// idling core 1 when using these resources should work

struct MyMIDI_Callbacks : FineGrainedMIDI_Callbacks<MyMIDI_Callbacks> {
  // Note how this ^ name is identical to the argument used here ^

  void onNoteOff(Channel channel, uint8_t note, uint8_t velocity, Cable cable) {
//    Serial << "Note Off: " << channel << ", note " << note << ", velocity "
//           << velocity << ", " << cable << endl;
  }
  // 
  void onNoteOn(Channel channel, uint8_t note, uint8_t velocity, Cable cable) {
  //  Serial.printf("ch %d noteon %d\n",channel.getRaw(),note);
    for (int16_t i=0; i< NTRACKS;++i) {  // control surface "Channel" is a real pain in the ass to deal with
      if (channel.getRaw()+1 == MIDIinputchannel[i])  {
        notes[i].offset=(int8_t)note-MIDDLE_C; // incoming midi notes are used as a signed offset from middle C
  //      Serial.printf("offset %d\n",(int8_t)note-MIDDLE_C);
      }
    }
  }

  void onControlChange(Channel channel, uint8_t controller, uint8_t value,
                       Cable cable) {
    Serial << "Control Change: " << channel << ", controller " << controller
           << ", value " << value << ", " << cable << endl;
  }

  void onSystemExclusive(SysExMessage se) {
    Serial << F("System Exclusive: [") << se.length << "] "
           << AH::HexDump(se.data, se.length) << ", " << se.cable << endl;
  }
  void onTimeCodeQuarterFrame(uint8_t data, Cable cable) {
    Serial << "MTC Quarter Frame: " << data << ", " << cable << endl;
  }
  void onSongPosition(uint16_t beats, Cable cable) {
    Serial << "Song Position Pointer: " << beats << ", " << cable << endl;
  }
  void onSongSelect(uint8_t songnumber, Cable cable) {
    Serial << "Song Select: " << songnumber << ", " << cable << endl;
  }

  // process MIDI clock messages
// count MIDI clocks for a while to get a decent average and then compute BPM from it
// if external MIDI clock is enabled use it as the master clock

  void onClock(Cable cable) { 
    long qn,clockperiod;
    clockperiod= (long)(((60.0/(float)bpm)/PPQN)*1000); // for call to clocktick(). use calculated BPM which is more stable - MIDI clock has a lot of jitter
    --MIDIclocks;
    if (MIDIclocks ==0 ) {
      MIDIclocks=PPQN*2;
      qn=millis()-lastMIDIclock;
      lastMIDIclock=millis();
      if (MIDIsync >0) --MIDIsync;
      if (MIDIsync ==0) {
        if ((qn < 6200) && (qn > 480))  bpm=2*60.0*1000/qn; // check that clock value is between 20 and 240BPM so we don't trash the current bpm
      }
//      Serial.printf("%d %d\n",qn,bpm);
    }
    // if (useMIDIclock) clocktick(clockperiod); 
  }

/*
  void onClock(Cable cable) { 
    int32_t qn;
    static int32_t lastMIDIclock;
    static float qn_acc,fbpm;
    qn=millis()-lastMIDIclock;
    if ((qn < 125) && (qn > 10)) qn_acc+=qn; // handles case of clock startup - clock measurement will not be valid
//    else qn_acc=23; // assume approx 120 bpm if last measurement was out of whack  
    float qnaverage=qn_acc/(PPQN*4); // do a running average of clock period over a few seconds to smooth things out
    qn_acc-=qnaverage;
    lastMIDIclock=millis();
// this will automatically sync BPM to external 24 PPQN MIDI clock
    if ((qnaverage < 125.0) && (qnaverage > 10.0))  fbpm=(60.0*1000/(qnaverage*PPQN)); // check that clock value is between 20 and 240BPM so we don't trash the current bpm
    else fbpm=120.0;
    Serial.printf("%d %f %f\n",qn,qnaverage,fbpm);
  }
*/

  void onStart(Cable cable) { 
  //  Serial.printf("Start\n");
    all_notes_off();  // in case notes are already playing
    sync_sequencers(); // sync all sequencers 
    MIDIsync=16; // sync BPM again
    showLEDs(); // update LEDs and display
    shownotes();
    showrhythms();
    rp2040.idleOtherCore();  // so core 1 doesn't also modify state machine
    controlstate=RUNNING; // force core 1 to playing state
    rp2040.resumeOtherCore();
  }

  void onContinue(Cable cable) { // process MIDI continue message - continue playing
 // Serial.printf("Continue\n");
    rp2040.idleOtherCore();  // so core 1 doesn't also modify state machine
    controlstate=RUNNING; // put core 1 in playing state
    rp2040.resumeOtherCore(); 
  }

  void onStop(Cable cable) { 
 //   Serial.printf("Stop\n");
    all_notes_off();  // so notes don't hang
    rp2040.idleOtherCore();  // so core 1 doesn't also modify state machine
    controlstate=IDLE; // put core 1 in idle state machine state
    rp2040.resumeOtherCore();
  }

  void onActiveSensing(Cable cable) {
    Serial << "Active Sensing: " << cable << endl;
  }
  void onSystemReset(Cable cable) {
    Serial << "System Reset: " << cable << endl;
  }

} callback;
