# PicoRhythmicon - a Subharmonicon-like sequencer using the Twisty 2 MIDI Controller hardware

The Twisty 2 was designed to be a MIDI controller but also as a platform for creating other MIDI devices. This one is a sequencer similar to the Moog Subharmonicon. The Subharmonicon sequencer is based on a very old polyrhythmic instrument called the Rhythmicon.

Demo and tutorial video: 

I suggest you also watch a tutorial video on the Moog Subharmonicon to get a better understanding of how this sequencer works.


**User Guide**

The top 16 encoders are set up as three rows of (four step) step sequencers with the last row being clock dividers. Notes are set by rotating the encoders. The top three rows of the display show the MIDI notes that will be sent for each step of the three sequencers. The display shows the clock divider values on the botton row.

The 4 clock divider LED colors are (L-R) Red, Green, Blue and Purple. Rotate the encoders counterclockwise to increase the divider i.e. slow down the clock. To connect a clock divider to a sequencer, press the button on the sequencer's row that corresponds to the clock divder's column. The sequencer LED will change to the same color as the clock divider to indicate that sequencer is connected to that divider.

The internal master clock is 24 pulses per quarter note/6 = sixteenth notes when the clock divider is set to 1. Changing the clock divider to 2 results in eighth notes, 3 is dotted eighths, 4 is quarter note etc etc. The maximum clock divider is 128 which is 8 bars. If a 24 PPQN MIDI clock is sent to the PicoRhythmicon via it's USB or BLE MIDI interface it will automatically sync to it. It also responds to MIDI transport messages - Start, Stop and rewind.

**The Setup Menu**

Clicking the bottom right encoder will bring up a setup menu. When in edit mode use the bottom right encoder the scroll through the edit menu items. Click the bottom right control switch to select an item - a "*" character will appear beside the item to indicate it is being edited. Use the bottom right encoder to change the value for the item and click the bottom right encoder switch to select a value. Click the bottom left encoder or double clock the bottom right encoder to exit the setup menu.

Click the bottom left encoder again to exit the configuration edit menu.

**Setup Menu Items:**

Each of the three sequencers has setup options:

**Root** - root note for the scale quantizer (default C3 MIDI nite 60). The note for any given step is the root note plus an offset dialled in by that step's encoder.

**Scale**  - selects the scale for the sequencer from CHROMATIC, MAJOR, MINOR, HARMONIC_MINOR, MAJOR_PENTATONIC, MINOR_PENTATONIC, DORIAN, PHRYGIAN, LYDIAN, or MIXOLYDIAN. The note shown on the display is the quantized value. Because of this you may have to rotate the encoder a few steps to get to the next note in the scale.

**Step mode** - changes the way the sequencer steps from one note to the next: 

Fwd - steps from left to right

Rev - steps from right to left

Pong - reverses direction on the first and last steps resulting in an 8 step sequence

Walk - random walk - the sequencer randomly steps to the right or the left 

Rand - a random step is chosen

**MIDI Out** - MIDI channel that this sequencer will send notes on

**MIDI In** - MIDI channel that this sequencer will respond to incoming transpose notes. Transpose is calculated as an offset from MIDI note 60, which is added to the root note plus the offset dialled in by a step's encoder. ie the note sent on any given step is calculated as root + step offset + (transpose note - 60)

**BPM** - sets the internal clock BPM. As noted above, the unit will sync to an external MIDI clock sent via the BLE or USB interfaces.

**Bat Voltage** - shows the battery voltage if the hardware supports it. See the enclosure README file for the hardware mods needed for battery operation.



**Building the Firmware**

The app was written in Arduino 2.3.7 with the Pico Arduino board support package installed. Select the board type as Pico, Pico 2, Pico 2 or Pico 2W depending on the board you used. The prototype was tested with both a Pico W and a Pico 2 W. In the tools menu select IP/Bluetooth stack as IPV4 + Bluetooth if you want BLE MIDI, and select the USB stack as Adafruit TinyUSB. This app should not need overclocking since the CPU demands are light.

If you want BLE MIDI, use a Pico W or Pico 2W and uncomment the #define BLUETOOTH directive near the top of the main source file.

Note: I used the Control Surface library because it was the only one I could find that supports BLE MIDI for Arduino Pico. It will give you a warning about supported platforms when you compile but the library does work on the RP2040 and RP2350.


**Library Dependencies**

Adafruit Graphics

Adafruit SSD1306 driver

Adafruit TinyUSB

Adafruit NeoPixel

Control Surface  https://github.com/tttapa/Control-Surface


