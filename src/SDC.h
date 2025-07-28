/*
 * ======================================================================================================================
 * SDC.h - SD Card
 * ======================================================================================================================
 */

/*
  SdFat library automatically calls SPI.beginTransaction() before it starts communicating with 
  the SD card and SPI.endTransaction() when finished. This behavior ensures exclusive access 
  to the SPI bus and correct configuration for the SD card, so you don’t need to wrap your 
  calls to SdFat functions with these SPI commands yourself.
*/

#define CF_NAME           "CONFIG.TXT"
#define KEY_MAX_LENGTH    30                // Config File Key Length
#define VALUE_MAX_LENGTH  30                // Config File Value Length
#define LINE_MAX_LENGTH   VALUE_MAX_LENGTH+KEY_MAX_LENGTH+3   // =, CR, LF 

// Prototyping functions to aviod compile function unknown issue.
bool Particle_Publish(char *EventName); 
bool OBS_Full();
void OBS_Do();

/* 
 *=======================================================================================================================
 * SD_initialize()
 *=======================================================================================================================
 */
void SD_initialize() {
#if (PLATFORM_ID == PLATFORM_MSOM)
  // Configure SPI using SdSpiConfig:
  // Parameters:
  //  - CS pin (SD_ChipSelect)
  //  - SPI mode: SHARED_SPI lets you share the SPI bus with other devices
  //  - SPI clock speed (here 10 MHz)
  //  - SPI instance pointer (&SPI) for SPI0
  SdSpiConfig spiConfig(SD_ChipSelect, SHARED_SPI, SD_SCK_MHZ(10), &SPI);
  if (!SD.begin(spiConfig)) {
#else
  if (!SD.begin(SD_ChipSelect)) {
#endif
    Output ("SD:NF");
    SystemStatusBits |= SSB_SD;
    delay (5000);
  }
  else {
    if (!SD.exists(SD_obsdir)) {
      if (SD.mkdir(SD_obsdir)) {
        Output ("SD:MKDIR OBS OK");
        Output ("SD:Online");
        SD_exists = true;
      }
      else {
        Output ("SD:MKDIR OBS ERR");
        Output ("SD:Offline");
        SystemStatusBits |= SSB_SD;  // Turn On Bit     
      } 
    }
    else {
      Output ("SD:Online");
      Output ("SD:OBS DIR Exists");
      SD_exists = true;
    }
  }
}

/* 
 *=======================================================================================================================
 * SD_LogObservation()
 *=======================================================================================================================
 */
void SD_LogObservation(char *observations) {
  char SD_logfile[24];
  File fp;

  if (!SD_exists) {
    return;
  }

  if (!Time.isValid()) {
    return;
  }
  
  sprintf (SD_logfile, "%s/%4d%02d%02d.log", SD_obsdir, Time.year(), Time.month(), Time.day());
  
  fp = SD.open(SD_logfile, FILE_WRITE); 
  if (fp) {
    fp.println(observations);
    fp.close();
    SystemStatusBits &= ~SSB_SD;  // Turn Off Bit
    // Output ("OBS Logged to SD");
  }
  else {
    SystemStatusBits |= SSB_SD;  // Turn On Bit - Note this will be reported on next observation
    Output ("SD:Open(Log)ERR");
    // At thins point we could set SD_exists to false and/or set a status bit to report it
    // sd_initialize();  // Reports SD NOT Found. Library bug with SD
  }
}

/* 
 *=======================================================================================================================
 * SD_N2S_Delete()
 *=======================================================================================================================
 */
bool SD_N2S_Delete() {
  bool result;

  if (SD_exists && SD.exists(SD_n2s_file)) {
    if (SD.remove (SD_n2s_file)) {
      SystemStatusBits &= ~SSB_N2S; // Turn Off Bit
      Output ("N2S->DEL:OK");
      result = true;
    }
    else {
      Output ("N2S->DEL:ERR");
      SystemStatusBits |= SSB_SD; // Turn On Bit
      result = false;
    }
  }
  else {
    SystemStatusBits &= ~SSB_N2S; // Turn Off Bit
    Output ("N2S->DEL:NF");
    result = true;
  }
  eeprom.n2sfp = 0;
  EEPROM_Update();
  return (result);
}

/* 
 *=======================================================================================================================
 * SD_NeedToSend_Add()
 *=======================================================================================================================
 */
void SD_NeedToSend_Add(char *observation) {
  File fp;

  if (!SD_exists) {
    return;
  }
  
  fp = SD.open(SD_n2s_file, FILE_WRITE); // Open the file for reading and writing, starting at the end of the file.
                                         // It will be created if it doesn't already exist.
  if (fp) {  
    if (fp.size() > SD_n2s_max_filesz) {
      fp.close();
      Output ("N2S:Full");
      if (SD_N2S_Delete()) {
        // Only call ourself again if we truely deleted the file. Otherwise infinate loop.
        SD_NeedToSend_Add(observation); // Now go and log the data
      }
    }
    else {
      fp.println(observation); //Print data, followed by a carriage return and newline, to the File
      fp.close();
      SystemStatusBits &= ~SSB_SD;  // Turn Off Bit
      SystemStatusBits |= SSB_N2S; // Turn on Bit that says there are entries in the N2S File
      Output ("N2S:OBS Added");
    }
  }
  else {
    SystemStatusBits |= SSB_SD;  // Turn On Bit - Note this will be reported on next observation
    Output ("N2S:Open Error");
    // At thins point we could set SD_exists to false and/or set a status bit to report it
    // sd_initialize();  // Reports SD NOT Found. Library bug with SD
  }
}

/* 
 *=======================================================================================================================
 * SD_N2S_Publish()
 *=======================================================================================================================
 */
void SD_N2S_Publish() {
  File fp;
  char ch;
  int i;
  int sent=0;
  char *EventType;

  if (SD_exists && SD.exists(SD_n2s_file)) {
    Output ("N2S:Publish");

    fp = SD.open(SD_n2s_file, FILE_READ); // Open the file for reading, starting at the beginning of the file.

    if (fp) {
      // Delete Empty File or too small of file to be valid
      if (fp.size()<=20) {
        fp.close();
        Output ("N2S:Empty");
        SD_N2S_Delete();
      }
      else {
        if (eeprom.n2sfp) {
          if (fp.size()<=eeprom.n2sfp) {
            // Something wrong. Can not have a file position that is larger than the file
            eeprom.n2sfp = 0; 
          }
          else {
            fp.seek(eeprom.n2sfp);  // Seek to where we left off last time. 
          }
        }

        // Loop through each line / obs and transmit
        i = 0;
        while (fp.available() && (i < MAX_MSGBUF_SIZE )) {
          ch = fp.read();

          if (ch == 0x0A) {  // newline
            if (Particle_Publish(EventType)) {
              sprintf (Buffer32Bytes, "N2S[%d]%s->PUB:OK", sent++, EventType);
              Output (Buffer32Bytes);
              Serial_write (msgbuf);

              // setup for next line in file
              i = 0;

              // file position is at the start of the next observation or at eof
              eeprom.n2sfp = fp.position();        
            }
            else { // Delay then retry
              sprintf (Buffer32Bytes, "N2S[%d]%s->PUB:RETRY", sent, EventType);
              Output (Buffer32Bytes);
              Serial_write (msgbuf);

              // Do some Background work to create a 5 second delay
              // 4 Seconds is the Particle Burst recovery period
              for (int d=0; d<5; d++) {
                BackGroundWork(); 
              }

              if (Particle_Publish(EventType)) {
                sprintf (Buffer32Bytes, "N2S[%d]%s->PUB:OK", sent++, EventType);
                Output (Buffer32Bytes);
                // setup for next line in file
                i = 0;

                // file position is at the start of the next observation or at eof
                eeprom.n2sfp = fp.position();
              }
              else {
                sprintf (Buffer32Bytes, "N2S[%d]%s->PUB:ERR", sent, EventType);
                Output (Buffer32Bytes);
                // On transmit failure, stop processing file.
                break;
              }
            } // RETRY
            
            // At this point file pointer's position is at the first character of the next line or at eof

            // We could be in this loop for a while. We don't want to miss our observation window.
            // So make the observation and stay in the loop if we have space in the OBS array.
            // We need to avoid a full array that would cause all observations to be saved to N2S file 
            // we currently have open, a bad thing.
            if ( (System.millis() - lastOBS) > OBSERVATION_INTERVAL) {
              Output ("N2S:OBS Needed");
              if (OBS_Full()) {
                // need to get out of this loop and let the main loop make the needed observation
                Output ("N2S:OBS FULL");
                break;
              }
              else {
                I2C_Check_Sensors(); // Make sure Sensors are online
                OBS_Do();                  
              }
            }
          } // Newline
          else if (ch == 0x0D) { // CR, LF follows and will trigger the line to be processed       
            msgbuf[i] = 0; // null terminate then wait for newline to be read to process OBS

            // After the JSON structure is a comma and Particle Event Type ("FS" or "RS")
            msgbuf[i-3] = 0; // Set the comma to Null so we don't transmit to Particle what follows
            EventType = &msgbuf[i-2];      
          }
          else {
            msgbuf[i++] = ch;
          }

          // Check for buffer overrun
          if (i >= MAX_MSGBUF_SIZE) {
            sprintf (Buffer32Bytes, "N2S[%d]->BOR:ERR", sent);
            Output (Buffer32Bytes);
            fp.close();
            SD_N2S_Delete(); // Bad data in the file so delete the file           
            return;
          }
        } // end while 

        if (fp.available() <= 20) {
          // If at EOF or some invalid amount left then delete the file
          fp.close();
          SD_N2S_Delete();
        }
        else {
          // At this point we sent 0 or more observations but there was a problem.
          // eeprom.n2sfp was maintained in the above read loop. So we will close the
          // file and next time this function is called we will seek to eeprom.n2sfp
          // and start processing from there forward. 
          fp.close();
          EEPROM_Update(); // Update file postion in the eeprom.
        }
      }
    }
    else {
        Output ("N2S->OPEN:ERR");
    }
  }
}

/* 
 * =======================================================================================================================
 * Support functions for Config file
 * 
 *  https://arduinogetstarted.com/tutorials/arduino-read-config-from-sd-card
 *  
 *  myInt_1    = SD_findInt(F("myInt_1"));
 *  myFloat_1  = SD_findFloat(F("myFloat_1"));
 *  myString_1 = SD_findString(F("myString_1"));
 *  
 *  CONFIG.TXT content example
 *  myString_1=Hello
 *  myInt_1=2
 *  myFloat_1=0.74
 * =======================================================================================================================
 */

int SD_findKey(const __FlashStringHelper * key, char * value) {
  
  // Disable LoRA SPI0 Chip Select
  pinMode(LORA_SS, OUTPUT);
  digitalWrite(LORA_SS, HIGH);
  
  File configFile = SD.open(CF_NAME);

  if (!configFile) {
    Serial.print(F("SD Card: error on opening file "));
    Serial.println(CF_NAME);
    return(0);
  }

  char key_string[KEY_MAX_LENGTH];
  char SD_buffer[KEY_MAX_LENGTH + VALUE_MAX_LENGTH + 1]; // 1 is = character
  int key_length = 0;
  int value_length = 0;

  // Flash string to string
  PGM_P keyPoiter;
  keyPoiter = reinterpret_cast<PGM_P>(key);
  byte ch;
  do {
    ch = pgm_read_byte(keyPoiter++);
    if (ch != 0)
      key_string[key_length++] = ch;
  } while (ch != 0);

  // check line by line
  while (configFile.available()) {
    // UNIX uses LF = \n
    // WINDOWS uses CFLF = \r\n
    int buffer_length = configFile.readBytesUntil('\n', SD_buffer, LINE_MAX_LENGTH);
    if (SD_buffer[buffer_length - 1] == '\r')
      buffer_length--; // trim the \r

    if (buffer_length > (key_length + 1)) { // 1 is = character
      if (memcmp(SD_buffer, key_string, key_length) == 0) { // equal
        if (SD_buffer[key_length] == '=') {
          value_length = buffer_length - key_length - 1;
          memcpy(value, SD_buffer + key_length + 1, value_length);
          break;
        }
      }
    }
  }

  configFile.close();  // close the file
  return value_length;
}

int HELPER_ascii2Int(char *ascii, int length) {
  int sign = 1;
  int number = 0;

  for (int i = 0; i < length; i++) {
    char c = *(ascii + i);
    if (i == 0 && c == '-')
      sign = -1;
    else {
      if (c >= '0' && c <= '9')
        number = number * 10 + (c - '0');
    }
  }

  return number * sign;
}

long HELPER_ascii2Long(char *ascii, int length) {
  int sign = 1;
  long number = 0;

  for (int i = 0; i < length; i++) {
    char c = *(ascii + i);
    if (i == 0 && c == '-')
      sign = -1;
    else {
      if (c >= '0' && c <= '9')
        number = number * 10 + (c - '0');
    }
  }

  return number * sign;
}

float HELPER_ascii2Float(char *ascii, int length) {
  int sign = 1;
  int decimalPlace = 0;
  float number  = 0;
  float decimal = 0;

  for (int i = 0; i < length; i++) {
    char c = *(ascii + i);
    if (i == 0 && c == '-')
      sign = -1;
    else {
      if (c == '.')
        decimalPlace = 1;
      else if (c >= '0' && c <= '9') {
        if (!decimalPlace)
          number = number * 10 + (c - '0');
        else {
          decimal += ((float)(c - '0') / pow(10.0, decimalPlace));
          decimalPlace++;
        }
      }
    }
  }

  return (number + decimal) * sign;
}

String HELPER_ascii2String(char *ascii, int length) {
  String str;
  str.reserve(length);
  str = "";

  for (int i = 0; i < length; i++) {
    char c = *(ascii + i);
    str += String(c);
  }
  return str;
}

char* HELPER_ascii2CharStr(char *ascii, int length) {
  char *str;
  str = (char *) malloc (length+1);
  str[0] = 0;
  for (int i = 0; i < length; i++) {
    char c = *(ascii + i);
    str[i] = c;
    str[i+1] = 0;
  }
  return str;
}

bool SD_available(const __FlashStringHelper * key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = SD_findKey(key, value_string);
  return value_length > 0;
}

int SD_findInt(const __FlashStringHelper * key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = SD_findKey(key, value_string);
  return HELPER_ascii2Int(value_string, value_length);
}

float SD_findFloat(const __FlashStringHelper * key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = SD_findKey(key, value_string);
  return HELPER_ascii2Float(value_string, value_length);
}

String SD_findString(const __FlashStringHelper * key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = SD_findKey(key, value_string);
  return HELPER_ascii2String(value_string, value_length);
}

char* SD_findCharStr(const __FlashStringHelper * key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = SD_findKey(key, value_string);
  return HELPER_ascii2CharStr(value_string, value_length);
}

long SD_findLong(const __FlashStringHelper * key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = SD_findKey(key, value_string);
  return HELPER_ascii2Long(value_string, value_length);
}


/* 
 * =======================================================================================================================
 * SD_ReadConfigFile()
 * =======================================================================================================================
 */
void SD_ReadConfigFile() {
  cf_aes_pkey     = SD_findCharStr(F("aes_pkey"));
  sprintf(msgbuf, "CF:aes_pkey=[%s]", cf_aes_pkey); Output (msgbuf);

  cf_aes_myiv     = SD_findLong(F("aes_myiv"));
  sprintf(msgbuf, "CF:aes_myiv=[%lu]", cf_aes_myiv);   Output (msgbuf);

  cf_lora_unitid  = SD_findInt(F("lora_unitid"));
  sprintf(msgbuf, "CF:lora_unitid=[%d]", cf_lora_unitid); Output (msgbuf);

  cf_lora_txpower = SD_findInt(F("lora_txpower"));
  sprintf(msgbuf, "CF:lora_txpower=[%d]", cf_lora_txpower); Output (msgbuf);

  cf_lora_freq   = SD_findInt(F("lora_freq"));
  sprintf(msgbuf, "CF:lora_freq=[%d]", cf_lora_freq); Output (msgbuf);
}