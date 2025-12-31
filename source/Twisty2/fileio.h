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


// file operations for Twisty 2
// configurations are saved and loaded in JSON format

#define VERSION 100 // JSON format is versioned in case it changes at some point

// save current config to filesystem
int16_t saveconfig(int16_t slot) {
  char filename[20];
  sprintf(filename,"slot%d.json",slot);

  if (LittleFS.exists(filename)) LittleFS.remove(filename); // open for write doesn't seem to overwrite so delete exiting file

  File file = LittleFS.open(filename, "w");
  if (!file) {
    Serial.println("file open failed");
    return 0;
  }

  file.printf("{ \"Version\" : %d ,\n",VERSION);
  file.printf("\"page\" :  [\n");

  for (int16_t p=0;p<CONTROLLER_PAGES;++p) {
    file.printf("  { \"control\" : [ \n");
    for (int16_t c=0; c< NUMENCODERS;c++) {
    //  file.printf("  { \"control\" : [ {");
      file.printf("    { \"EncoderType\":%d,",controls[p].encoder[c].type);
      file.printf("\"EncoderChannel\":%d,",controls[p].encoder[c].channel);
      file.printf("\"EncoderCCNumber\":%d,",controls[p].encoder[c].ccnumber);
      file.printf("\"EncoderMinValue\":%d,",controls[p].encoder[c].minvalue);
      file.printf("\"EncoderMaxValue\":%d,",controls[p].encoder[c].maxvalue);
      file.printf("\"EncoderValue\":%d,",controls[p].encoder[c].value);
      file.printf("\"EncoderColor\":%d,",controls[p].encoder[c].color);
      file.printf("\"EncoderLabel\":%d,",controls[p].encoder[c].label);
      file.printf("\"SwitchMode\":%d,",controls[p].encswitch[c].mode);
      file.printf("\"SwitchType\":%d,",controls[p].encswitch[c].type);
      file.printf("\"SwitchChannel\":%d,",controls[p].encswitch[c].channel);
      file.printf("\"SwitchCC\":%d,",controls[p].encswitch[c].ccnumber);
      file.printf("\"SwitchMinValue\":%d,",controls[p].encswitch[c].minvalue);
      file.printf("\"SwitchMaxValue\":%d,",controls[p].encswitch[c].maxvalue);
      file.printf("\"SwitchValue\":%d,",controls[p].encswitch[c].value);
      file.printf("\"SwitchColor\":%d,",controls[p].encswitch[c].color);
      file.printf("\"SwitchLabel\":%d",controls[p].encswitch[c].label);
      if (c == (NUMENCODERS-1)) file.printf("}\n");
      else file.printf("},\n");
    }
    if (p == (CONTROLLER_PAGES-1)) file.printf("    ]\n  }\n");
    else file.printf("    ]\n  },\n");
  }
  file.printf("]\n}\n");
  file.close();
  return 1;
}


// read and deserialize JSON file
int16_t loadconfig(int16_t slot) {
  char filename[20];
  sprintf(filename,"slot%d.json",slot);

  File file = LittleFS.open(filename, "r");
  if (!file) {
    Serial.println("file open failed");
    return 0;
  }
  if (!file) {
    Serial.println("Failed to open file for reading");
    return 0;
  }

  // Allocate the JSON document
  JsonDocument doc;

  // Deserialize the JSON document from the file
  DeserializationError error = deserializeJson(doc, file);

  // Close the file
  file.close();

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return 0;
  } else {
    // Data has been deserialized successfully, access the values
    int16_t mode,channel,ccnumber,minvalue,maxvalue,value,color,label; 

    for (int16_t p=0;p<CONTROLLER_PAGES;++p) {
      for (int16_t c=0; c< NUMENCODERS;c++) {
        controls[p].encoder[c].type=doc["page"][p]["control"][c]["EncoderMode"];
        controls[p].encoder[c].channel=doc["page"][p]["control"][c]["EncoderChannel"];
        controls[p].encoder[c].ccnumber=doc["page"][p]["control"][c]["EncoderCCNumber"];
        controls[p].encoder[c].minvalue=doc["page"][p]["control"][c]["EncoderMinValue"];
        controls[p].encoder[c].maxvalue=doc["page"][p]["control"][c]["EncoderMaxValue"];
        controls[p].encoder[c].value=doc["page"][p]["control"][c]["EncoderValue"];
        controls[p].encoder[c].color=doc["page"][p]["control"][c]["EncoderColor"];
        controls[p].encoder[c].label=doc["page"][p]["control"][c]["EncoderLabel"];
        controls[p].encswitch[c].mode=doc["page"][p]["control"][c]["SwitchMode"];
        controls[p].encswitch[c].type=doc["page"][p]["control"][c]["SwitchType"];
        controls[p].encswitch[c].channel=doc["page"][p]["control"][c]["SwitchChannel"];
        controls[p].encswitch[c].ccnumber=doc["page"][p]["control"][c]["SwitchCCNumber"];
        controls[p].encswitch[c].minvalue=doc["page"][p]["control"][c]["SwitchMinValue"];
        controls[p].encswitch[c].maxvalue=doc["page"][p]["control"][c]["SwitchMaxValue"];
        controls[p].encswitch[c].value=doc["page"][p]["control"][c]["SwitchValue"];
        controls[p].encswitch[c].color=doc["page"][p]["control"][c]["SwitchColor"];
        controls[p].encswitch[c].label=doc["page"][p]["control"][c]["SwitchLabel"];
      }
    }
  }
  return 1;
}

/*
// debug code - show files and contents
int16_t loadconfig(int16_t slot) {
  Dir dir = LittleFS.openDir("/");
  while (dir.next()) {
      Serial.println(dir.fileName());
      if(dir.fileSize()) {
          File file = dir.openFile("r");
          while(file.available()){
        // Read one byte at a time and print to Serial Monitor
            Serial.write(file.read());
          }
          file.close();
      }
      
  }
  return 1;
}
*/
