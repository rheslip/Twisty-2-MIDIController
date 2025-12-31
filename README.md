# Twisty 2 MIDIController
2nd Generation of my Twisty MIDI controller using Raspberry Pi Pico or Pico 2.

16 parameter encoders with switches and RGB LEDs, 2 menu encoders, OLED display, USB MIDI, TRS MIDI Out, Bluetooth MIDI with Pico W.
Four pages of parameters, encoder ranges, switch function and colors fully customizable. Save and recall up to 16 controller setups. STL files for 3D printed case.

![](https://github.com/rheslip/Twisty-2-MIDIController/blob/main/images/IMG_0609.JPG)


![](https://github.com/rheslip/Twisty-2-MIDIController/blob/main/images/IMG_0610.JPG)


![](https://github.com/rheslip/Twisty-2-MIDIController/blob/main/images/IMG_0611.JPG)


Hardest part of the build is soldering the three TSSOP 74HC4067 analog MUXes. If encoders aren't working, check your soldering carefully! Rest of the build is straightforward.


![](https://github.com/rheslip/Twisty-2-MIDIController/blob/main/images/IMG_0612.JPG)


Pico can be mounted with header strips. I used headers in the corners and short wires to connect the Pico to the PCB which makes the Pico board a lot easier to remove if needed.


![](https://github.com/rheslip/Twisty-2-MIDIController/blob/main/images/IMG_0613.JPG)


Side view of OLED mounting


![](https://github.com/rheslip/Twisty-2-MIDIController/blob/main/images/IMG_0614.JPG)


Light pipes are short sections of transparent 3D printer filament.


![](https://github.com/rheslip/Twisty-2-MIDIController/blob/main/images/IMG_0615.JPG)


**Build Guide**

Solder the .1u caps, resistors and the diode. Solder the 16 SK6812 RGB leds making sure the orientation is correct and that the LEDs face the top side of the PCB. Solder the TRS MIDI out jack.

Clip the support lugs off all the encoders - there was not enough room in the layout to accomodate them. The encoders have good mechanical support from the enclosure top anyway. The encoders mount from the TOP side of the board and are soldered on the bottom side. You **MUST** solder S2, S6, D2 and D6 **BEFORE** mounting the Pico board. Soldering S2, S6 and then the Pico makes soldering the Pico somewhat easier than soldering all the encoders first.

After all the encoders are soldered, mount the OLED display - temporarily mount the enclosure top as a guide to get the spacing from the display to the top correct.

