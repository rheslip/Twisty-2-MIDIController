# Twisty 2 MIDI Controller
2nd Generation of my Twisty MIDI controller using Raspberry Pi Pico or Pico 2.

The Twisty 2 MIDI controller has 16 parameter encoders with switches, 16 RGB LEDs, 2 menu encoders, 128x32 pixel OLED display, USB MIDI, TRS MIDI Out, and optional Bluetooth MIDI with Pico W or Pico 2W.
It provides four pages of MIDI controls and fully customizable MIDI messages, encoder ranges, switch functions and LED colors for each control. Text labels can be attached to each control as a visual aid to what the control does. You can save and recall up to 16 controller setups in the Pico's flash memory. STL files for a 3D printed case are provided as well.

Twisty 2 is a completely open source project - source code, KICad schematics and layout, gerber files and enclosure 3D files are in this archive.

![](https://github.com/rheslip/Twisty-2-MIDIController/blob/main/images/IMG_0609.JPG)


![](https://github.com/rheslip/Twisty-2-MIDIController/blob/main/images/IMG_0610.JPG)


![](https://github.com/rheslip/Twisty-2-MIDIController/blob/main/images/IMG_0611.JPG)

**Build Guide**

Solder the .1u caps, resistors and the diode. Solder the 16 SK6812 RGB leds making sure the orientation is correct and that the LEDs face the top side of the PCB. Solder the TRS MIDI out jack.

Solder the TSSOP 74HC4067s. I apply a very thin layer of flux to the pads and place the chip so its aligned on the pads. Using a fine tipped soldering iron tack one pin down with solder, adjust as needed and when the part is correctly aligned tack another pin down. Make sure the pins are sitting flat on the pads before you solder the rest. Solder each pin with very fine solder and don't worry about shorts between pins. Once all the pins are soldered examine your work with a magnifier and remove any shorts beween pins with fine solder wick.

Clip the support lugs off all the encoders - there was not enough room in the layout to accomodate them. The encoders have good mechanical support from the enclosure top anyway. The encoders mount from the TOP side of the board and are soldered on the bottom side. You **MUST** solder S2, S6, D2 and D6 **BEFORE** mounting the Pico board. Soldering S2, S6 and then the Pico makes soldering the Pico somewhat easier than soldering all the encoders first.

After all the encoders are soldered, mount the OLED display - temporarily mount the enclosure top as a guide to get the spacing from the display to the top correct.

Hardest part of the build is soldering the three TSSOP 74HC4067 analog MUXes. If encoders aren't working, recheck the soldering on these chips - most faults will be here! Rest of the build is straightforward.


![](https://github.com/rheslip/Twisty-2-MIDIController/blob/main/images/IMG_0612.JPG)


Pico can be mounted with header strips. I used header pins in the corners and short wires to connect the Pico to the PCB which makes the Pico board a lot easier to remove if needed.


![](https://github.com/rheslip/Twisty-2-MIDIController/blob/main/images/IMG_0613.JPG)


Side view of OLED mounting


![](https://github.com/rheslip/Twisty-2-MIDIController/blob/main/images/IMG_0614.JPG)


**Enclosure**

The prototype enclosure was printed on a Bambu P1S with black PLA+ filament. For the most part the stock filament settings were used. Slowing down the first layer speeds by 50% helps with adhesion and finish if you are using a textured build plate. The light pipe holes in the top had to be drilled a bit to fit 1.75mm transparent PETG which was lightly glued to hold it in place. Use small screws to hold the PCB to the bottom part standoffs and attach the top with the encoder nuts.


![](https://github.com/rheslip/Twisty-2-MIDIController/blob/main/images/IMG_0615.JPG)



**User Guide**

The top 16 controls (control=encoder+switch) are used to send MIDI CC messages, program change messages or or note on/off messages depending on how the controls are configured. The two extra controls at the bottom are used in conjunction with the OLED display to configure controls, save configurations and recall configurations.

The RGB LEDs associated with each control normally show the state of whatever was last used - encoder as a color and switch states as off or bright white. Encoder LED brightness changes as the control is moved to give a relative idea of the encoder setting. Double click the bottom left control to toggle between showing encoder states and switch states.

Rotate bottom left encoder to move between the 4 pages of controls. Default colors for each page are Red (page 1), Orange (page 2), Green (page ) and Aqua (page 4).

The top line of the OLED display shows the current page, the last used control, its message type and MIDI channel. The larger text below that shows a label if one has been configured and the last value that was sent for that control.


**The Configuration Menu**

Clicking the bottom left control will bring up a configuration menu to edit the last control that was used. The LED for that control will flash to indicate it is selected for editing. To select a different control when in edit mode either press a control switch to choose it or use bottom left encoder to scroll to a different control on the current page.

When in edit mode use the bottom right encoder the scroll through the edit menu items. Click the bottom right control switch to select an item - a "*" character will appear beside the item to indicate it is being edited. Use the bottom right encoder to change the value for the item and click the bottom right encoder switch to select a value.

Click the bottom left encoder again to exit the configuration edit menu.

**Configuration Menu Items:**

**Enc MIDI Ch.** - selects the MIDI channel that the selected encoder will send CC messages on

**Enc CC No.**  - selects the CC number for this encoder. The default CC assignments are 16-31 for page 1, 32-47 for page 2, 48-63 for page 3 and 64-79 for page 4

**Enc Label** - default is "CC". There are approximately 100 common audio terms (e.g. "Level", "Filter", "Track"...) that can be selected as an aid for remembering the function of the control. The label will be displayed instead of the CC number whenever this encoder is used.

**Enc Color** - selects the LED color for this encoder - Red, Orange, Green, Aqua, Blue or Violet. This is useful for grouping control functions by color.

**Enc Min** - minimum CC value that will be sent when the encoder is rotated - defaults to 0

**Enc Max** - maximum CC value that will be sent when the encoder is rotated - defaults to 127. 
**Note** If the minimum setting is higher than the maximum the encoder will reverse ie rotating clockwise sends a lower CC value.

**Switch MIDI Ch.** - selects the MIDI channel that the selected switch will send CC messages on

**Switch Mode** - selects either momentary mode (press for max value, release for min value) or toggle mode (select min or max value on alternate presses)

**Switch Type** - selects switch function:

     **CC** = send CC messages 
 
     **PC** = send program change messages
 
     **Note** = send Note on/off messages
 
     **SetEnc** = when the switch is activated, set the associated encoder on this control to Maxvalue. This provides a quick way to set an encoder to a specific value e.g. set a filter to flat response.
 
**Switch CC No.** - selects the CC number for this switch. This selection only applies when the switch type is send CC messages. The default switch CC assignments are the same as the associated encoder but on MIDI channel 2. 

**Switch Label** - same as encoder label - a memory aid for what the switch does

**Switch Min** - CC, PC or note value sent when the switch changes to its inactive state. defaults to 0

**Switch Max** - CC, PC or note value sent when the switch changes to its active state. defaults to 127. When switch type is SetEnc, sets associated encoder to this value.


**The Save/Load Menu**

Clicking the bottom right encoder enters the save/load menu. Twisty 2 supports saving and loading up to 16 different configurations in the Pico's flash memory.

**Note** You must select an FS size in the Arduino Pico Tools menu - 128K or larger is suggested. The application will automatically format a new FS if it has not been used before. Internally the application uses the LittleFS filesystem and stores configurations as JSON files.

**Save/Load Menu Items:**

**Slot** - selects the slot number 1-16 to save the current configuration to.

**Action** - select Load to load an already saved configuration from the selected slot. Save will save the current configuration to the selected slot. Format will reformat the LittleFS partition - this will ERASE ALL previously saved configurations.

**Confirm** - select Yes to confirm the selected action. The app will **NOT** warn you if you are about to overwrite an existing configuration. If Yes is selected the requested action will be performed, a success/fail message will appear and the Save/load Menu will exit.

**Note** You will get an error message if you attempt to load a configuration slot that hasn't previously been saved to.

To exit the Save/Load menu without changing anything click the bottom left control switch.


**Building the Firmware**

The app was written in Arduino 2.3.7 with the Pico Arduino board support package installed. Select the board type as Pico, Pico 2, Pico 2 or Pico 2W depending on the board you used. The prototype was tested with both a Pico W and a Pico 2 W. In the tools menu select the FS size you want (128k or more is recommended), IP/Bluetooth stack as IPV4 + Bluetooth if you want BLE MIDI, and select the USB stack as Adafruit TinyUSB. This app should not need overclocking since the CPU demands are light.

If you want BLE MIDI, use a Pico W or Pico 2W and uncomment the #define BLUETOOTH directive near the top of the main source file.

Note: I used the Control Surface library because it was the only one I could find that supports BLE MIDI for Arduino Pico. It will give you a "platform not supported" warning when you compile with Bluetooth support. This library has a bug which messes up BLE MIDI Note On/Off messages so that code is commented out until I find a solution. Switch type Note works fine for USB or TRS MIDI.


**Library Dependencies**

Adafruit Graphics

Adafruit SSD1306 driver

Adafruit TinyUSB

Adafruit NeoPixel

Arduino MIDI

Arduino JSON  https://arduinojson.org/

Control Surface  https://github.com/tttapa/Control-Surface


