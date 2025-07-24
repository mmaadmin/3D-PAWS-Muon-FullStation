PRODUCT_VERSION(1);
#define COPYRIGHT "Copyright [2025] [University Corporation for Atmospheric Research]"
#define VERSION_INFO "FSM-250721v1"

/*
 *======================================================================================================================
 * FullStation (FS) - Muon (M)
 *   Board Type : Particle Muon https://docs.particle.io/reference/datasheets/m-series/muon-datasheet/
 *
 *   Description: Monitor 3D-PAWS Full Station and transmit data to Particle Cloud
 *   Author: Robert Bubon
 *   Date:   2025-03-17 RJB Initial Development Based on Full Station Boron Version 38
 *
 *  Porting Notes:
 *     PMIC needs #if (PLATFORM_ID == PLATFORM_BORON) || (PLATFORM_ID == PLATFORM_MSOM)
 *     https://github.com/particle-iot/device-os/blob/develop/hal/shared/platforms.h
 * 
 *     LoRa Needs a Pin Change.
 *     #if (PLATFORM_ID == PLATFORM_MSOM)
 *     #define LORA_IRQ_PIN  D20    // G0 on LoRa board
 *     #define LORA_SS       D3     // Slave Select Pin
 *     #define LORA_RESET    D21    // Used by lora_initialize()
 *     #else
 *  
 *                        
 * NOTES:
 * When there is a successful transmission of an observation any need to send obersavations will be sent. 
 * On transmit a failure of these need to send observations, processing is stopped and the file is deleted.
 * 
 * Calibration
 * Adding serial console jumper after boot will cause sensors to be read every 1 second and values printed.
 * Removing serial console jumper will resume normal operation
 * 
 * Required Libraries:
 *  adafruit-HTU21DF        https://github.com/adafruit/Adafruit_HTU21DF_Library - 1.1.0 - I2C ADDRESS 0x40
 *  adafruit-BusIO          https://github.com/adafruit/Adafruit_BusIO - 1.8.2
 *  Adafruit_MCP9808        https://github.com/adafruit/Adafruit_MCP9808_Library - 2.0.0 - I2C ADDRESS 0x18
 *  Adafruit_BME280         https://github.com/adafruit/Adafruit_BME280_Library - 2.1.4 - I2C ADDRESS 0x77  (SD0 to GND = 0x76)
 *  Adafruit_BMP280         https://github.com/adafruit/Adafruit_BMP280_Library - 2.3.0 -I2C ADDRESS 0x77  (SD0 to GND = 0x76)
 *  Adafruit_BMP3XX         https://github.com/adafruit/Adafruit_BMP3XX - 2.1.0 I2C ADDRESS 0x77 and (SD0 to GND = 0x76)
 *  Adafruit_GFX            https://github.com/adafruit/Adafruit-GFX-Library - 1.10.10
 *  Adafruit_Sensor         https://github.com/adafruit/Adafruit_Sensor - 1.1.4
 *  Adafruit_SHT31          https://github.com/adafruit/Adafruit_SHT31 - 2.2.0 I2C ADDRESS 0x44 and 0x45 when ADR Pin High
 *  Adafruit_VEML7700       https://github.com/adafruit/Adafruit_VEML7700/ - 2.1.2 I2C ADDRESS 0x10
 *  Adafruit_SI1145         https://github.com/adafruit/Adafruit_SI1145_Library - 1.1.1 - I2C ADDRESS 0x60
 *  Adafruit_SSD1306        https://github.com/adafruit/Adafruit_SSD1306 - 2.4.6 - I2C ADDRESS 0x3C  
 *  Adafruit_PM25AQI        https://github.com/adafruit/Adafruit_PM25AQI - 1.0.6 I2C ADDRESS 0x12 - Modified to Compile, Adafruit_PM25AQI.cpp" line 104
 *  Adafruit_HDC302x        https://github.com/adafruit/Adafruit_HDC302x - 1.0.2 I2C ADDRESS 0x46 and 0x47 ( SHT uses 0x44 and x045)
 *  Adafruit_LPS35HW        https://github.com/adafruit/Adafruit_LPS35HW - 1.0.6 I2C ADDRESS 0x5D and 0x5C
 *  DFRobot_B_LUX_V30B      https://github.com/DFRobot/DFRobot_B_LUX_V30B - 1.0.1 I2C ADDRESS 0x4A (Not Used Reference Only) SEN0390
 *                          https://wiki.dfrobot.com/Ambient_Light_Sensor_0_200klx_SKU_SEN0390
 *  RTCLibrary              https://github.com/adafruit/RTClib - 1.13.0
 *  SdFat                   https://github.com/greiman/SdFat.git - 1.0.16 by Bill Greiman
 *  RF9X-RK-SPI1            https://github.com/rickkas7/AdafruitDataLoggerRK - 0.2.0 - Modified RadioHead LoRa for SPI1
 *  AES-master              https://github.com/spaniakos/AES - 0.0.1 - Modified to make it compile
 *  CryptoLW-RK             https://github.com/rickkas7/CryptoLW-RK - 0.2.0
 *  HIH8000                 No Library, Local functions hih8_initialize(), hih8_getTempHumid() - rjb
 *  SENS0390                https://wiki.dfrobot.com/Ambient_Light_Sensor_0_200klx_SKU_SEN0390 - DFRobot_B_LUX_V30B - 1.0.1 I2C ADDRESS 0x94
 *  EEPROM                  https://docs.particle.io/reference/device-os/api/eeprom/eeprom/
 *                          On Gen 3 devices (Argon, Boron, B Series SoM, Tracker SoM, and E404X) 
 *                          the EEPROM emulation is stored as a file on the flash file system. 
 *                          Since the data is spread across a large number of flash sectors, 
 *                          flash erase-write cycle limits should not be an issue in general.
 * 
 *  Dallas OneWire          https://github.com/particle-iot/OneWireLibrary - Not supported on Boron/Argon
 *                          WARNING:This library has not been updated for the Argon, Boron, Tracker One, Photon 2/P2.
 *                          https://docs.particle.io/reference/device-os/libraries/o/OneWire/
 *
 *  LeafSens                https://github.com/tinovi/LeafArduino   I2C ADDRESS 0x61
 *  i2cArduino              https://github.com/tinovi/i2cArduinoI2c I2C ADDRESS 0x63
 *  i2cMultiSm              https://github.com/tinovi/i2cMultiSoilArduino/tree/master/lib ADDRESS 0x65
 * 
 * Distance Sensors
 * The 5-meter sensors (MB7360, MB7369, MB7380, and MB7389) use a scale factor of (Vcc/5120) per 1-mm.
 * Particle 12bit resolution (0-4095),  Sensor has a resolution of 0 - 5119mm,  Each unit of the 0-4095 resolution is 1.25mm
 * 
 * Tinovi Moisture Sensors
 * Non-Contact Capacitive leaf wetness, Temperature sensor
 *   https://tinovi.com/shop/i2c-non-contact-capacitive-leaf-wetness-temperature/
 *   https://tinovi.com/wp-content/uploads/2021/10/Leaf-Wetness-i2c-2021-10-11.pdf
 * 
 * PM-WCS-3-I2C I2C Non-Contact Capacitive Soil Moisture, Temperature sensor
 *   https://tinovi.com/shop/i2c-capacitive-soil-moisture-temperature-and-ec-sensor-variation-cable/
 *   https://tinovi.com/wp-content/uploads/2022/08/PM-WCS-3-I2C.pdf
 * 
 * SOIL-MULTI-5-I2C I2C Capacitive multi level soil moisture, temperature sensor
 *   https://tinovi.com/shop/soil-multi-5-i2c-i2c-capacitive-soil-moisture-temperature-sensor/
 *   https://tinovi.com/wp-content/uploads/2024/07/SOIL-MULTI-5-I2C.pdf
 *
 * Battery Charger Status from System.batteryState()
 *  0 = BATTERY_STATE_UNKNOWN
 *  1 = BATTERY_STATE_NOT_CHARGING
 *  2 = BATTERY_STATE_CHARGING
 *  3 = BATTERY_STATE_CHARGED
 *  4 = BATTERY_STATE_DISCHARGING
 *  5 = BATTERY_STATE_FAULT
 *  6 = BATTERY_STATE_DISCONNECTED
 *
 * Publish to Particle
 *  Event Name: FS
 *  Event Variables:
 *  at      timestamp
 *  rg      rain gauge 1
 *  rgt     rain gauge 1 total today
 *  rgp     rain gauge 1 total prior
 *  rg2     rain gauge 2
 *  rgt2    rain gauge 2 total today
 *  rgp2    rain gauge 2 total prior
 *  ws      wind speed
 *  wd      wind direction
 *  wg      wind gust
 *  wgd     wind gust direction
 *  bp1     bmx_pressure
 *  bt1     bmx_temperature
 *  bh1     bmx_humidity
 *  bp2     bmx_pressure
 *  bt2     bmx_temperature
 *  bh2     bmx_humidity
 *  mt1     mcp_temp MCP1
 *  mt2     mcp_temp MCP2
 *  gt1     globe_temp MCP3
 *  gt2     globe_temp MCP4
 *  hh1     htu_humidity
 *  ht1     htu_temperature
 *  st1     sht_temperature
 *  sh1     sht_humidity
 *  st2     sht_temperature
 *  sh2     sht_humidity
 *  hdt1    hdc_temperature
 *  hdh1    hdc_humidity
 *  hdt2    hdc_temperature
 *  hdh2    hdc_humidity
 *  ht2     hih_temperature
 *  hh2     hih_humidity
 *  lpp1    lps35hw_humidity
 *  lpt1    lps35hw_temperature
 *  lpp2    lps35hw_humidity
 *  lpt2    lps35hw_temperature
 *  sv1     si_visible
 *  si1     si_infrared
 *  su1     su_ultraviolet
 *  vlx     veml_lux
 *  blx     blux30_lux
 *  sg      snow / stream gauge
 *  pm1s10  Standard Particle PM1.0
 *  pm1s25  Standard Particle PM2.5
 *  pm1s100 Standard Particle PM10.0 
 *  pm1e10  Atmospheric Environmental PM1.0
 *  pm1e25  Atmospheric Environmental PM2.5
 *  pm1e100 Atmospheric Environmental PM10.0
 *  hi      heat index
 *  wbt     wet bulb temperature
 *  wbgt    wet bub globe temperature
 *  tlwt    Tinovi Leaf Wetness temperature    
 *  tlww    Tinovi Leaf Wetness
 *  tsmt    Tinovi Soil Moisture temperature
 *  tsme25  Tinovi Soil Moisture e25
 *  tsmec   Tinovi Soil Moisture ec
 *  tsmvwc  Tinovi Soil Moisture vwc
 *  tmsmt   Tinovi Multi Level Soil Moisture
 *  tmsms1  Tinovi Multi Level Soil Moisture Soil Sensor 1 vwc
 *  tmsms2  Tinovi Multi Level Soil Moisture Soil Sensor 2 vwc
 *  tmsms3  Tinovi Multi Level Soil Moisture Soil Sensor 3 vwc
 *  tmsms4  Tinovi Multi Level Soil Moisture Soil Sensor 4 vwc
 * 
 * State of Health - Variables included with transmitted sensor readings
 *  bcs  = Battery Charger Status
 *  bpc  = Battery Percent Charge
 *  cfr    Charger Fault Register
 *  css  = Cell Signal Strength - percentage (0.0 - 100.0)
 *  hlth = Health 32bits - See System Status Bits in below define statements
 * 
 * AN002 Device Powerdown
 * https://support.particle.io/hc/en-us/articles/360044252554?input_string=how+to+handle+low+battery+and+recovery
 * 
 * NOTE: Compile Issues
 * If you have compile issues like multiple definations of functions then you need to clean the compile directory out
 *    ~/.particle/toolchains/deviceOS/2.0.1/build/target/user/...
 * 
 * Modified Libraries
 * In Adafruit_BMP280/src/Adafruit_BMP280.h Lines 39,40  Comment out the below 2 lines
 *   extern TwoWire Wire;
 *   extern SPIClass SPI;
 *   Uncommented code for takeForcedMeasurement()
 * In Adafruit_Sensor/src
 *   cp /dev/null Adafruit_Sensor.cpp
 * 
 * In Adafruit_HDC302x/src/Adafruit_HDC302x.cpp
 *   Read modified so while does not block
 *   int retries = 0;
 *   const int MAX_RETRIES = 10;
 *   while (!i2c_dev->read(buffer, 6)) {
 *     if (++retries >= MAX_RETRIES) {
 *       return false; // CRC check failed
 *     }
 *   }
 * 
 * DFRobot_B_LUX_V30B Library Not used it. It's bit banging with possible infinate loops - RJB
 * 
 * PIN Assignments
 * D8   = Serial Console (Ground Pin to Enable) - Not on Grove Shield
 * D7   = On Board LED - Lit when rain gauge tips, blinks when console connection needed
 * D6   = Reserved for Lora IRQ - Not on Grove Shield
 * D5   = SD Card Chip Select
 * D4   = SPI1 MSIO - Reserved for LoRa
 * D3   = SPI1 MOSI - Reserved for LoRa
 * D2   = SPI1 SCK  - Reserved for LoRa
 * D1   = I2C SCL
 * D0   = I2C SDA
 * 
 * A0   = WatchDog Monitor/Relay Reset Trigger
 * A1   = WatchDog Monitor Heartbeat
 * A2   = Wind Speed IRQ
 * A3   = Rain Gauge IRQ
 * A4   = 2nd Rain Gauge or Distance Gauge based on SD card file existing
 * A5   = Optional read/report of average on analog pin 
 * D13  = SPIO CLK   SD Card
 * D12  = SPI0 MOSI  SD Card
 * D11  = SPI0 MISO  SD Card
 * D10  = UART1 RX - Reserved for LoRa CS
 * D9   = UART1 TX - Reserved for LoRa RESET
 * 
 * Connection code based on below
 * https://community.particle.io/t/calling-particle-disconnect-after-a-failed-particle-connect-does-not-stop-led-from-blinking-green/19723/6
 * 
 * Max Particle message size 622 characters 
 * 
 * ========================================================
 * Support for 3rd Party Sim 
 * ========================================================
 *   SEE https://support.particle.io/hc/en-us/articles/360039741113-Using-3rd-party-SIM-cards
 *   SEE https://docs.particle.io/cards/firmware/cellular/setcredentials/
 *   Logic
 *     Output how sim is configured (internal or external)
 *     If console is enabled and SD found and SIM.TXT exists at the top level of SD card
 *       Read 1st line from SIM.TXT. Parse line for one of the below patterns
 *        INTERNAL
 *        AUP epc.tmobile.com username passwd
 *        UP username password
 *        APN epc.tmobile.com
 *      Perform appropriate actions to set sim
 *      Rename file to SIMOLD.TXT, so we don't do this on next boot
 *      Output notification to user to reboot then flash board led forever
 *
 * ========================================================
 * Support for Argon WiFi Boards
 * https://docs.particle.io/reference/device-os/api/wifi/wifi/
 * ========================================================
 * At the top level of the SD card make a file called WIFI.TXT
 * Add one line to the file
 * This line has 3 items that are comma separated Example
 * 
 * AuthType,ssid,password
 * 
 * Where AuthType is one of these keywords (WEP WPA WPA2 UNSEC)
 * Blank password is supported for UNSEC
 * ======================================================================================================================
 * 
 * Collecting Wind Data
 * Wind_SampleSpeed() - Return a wind speed based on how many interrupts and duration between calls to this function
 * Wind_SampleDirection() - Talk i2c to the AS5600 sensor and get direction
 * Wind_TakeReading() - Call this function every second. It calls wind direction and wind speed functions. Then saves samples in a circular buffer of 60 buckets.
 *
 * Creating the Wind Oobservations
 * Wind_DirectionVector() - Uses the 60 sample buckets of wind direction where speed is greater than zero to compute and return a wind vector.
 * Wind_SpeedAverage() - Use the 60 sample buckets of wind speed to return a wind speed average.
 * Wind_GustUpdate() - Uses the most current highest 3 consecutive samples of wind speed from the 60 samples. The 3 samples are then averaged for wind gust. 
 *   Variables wind.gust and wind.gust_directionare set by this function. 
 *   Call this function before calling Wind_Gust() and Wind_GustDirection()
 * Wind_Gust() - Returns wind.gust
 * Wind_GustDirection() - Returns wind.gust_direction
 * ======================================================================================================================
 */

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP3XX.h>
#include <Adafruit_HTU21DF.h>
#include <Adafruit_MCP9808.h>
#include <Adafruit_SI1145.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_VEML7700.h>
#include <Adafruit_PM25AQI.h>
#include <Adafruit_HDC302x.h>
#include <Adafruit_LPS35HW.h>
#include <RTClib.h>
#include <SdFat.h>
#include <RH_RF95.h>
#include <AES.h>
#include <i2cArduino.h>
#include <LeafSens.h>
#include <i2cMultiSm.h>

/*
 * ======================================================================================================================
 *  Loop Timers
 * ======================================================================================================================
 */
#define DELAY_NO_RTC               60000    // Loop delay when we have no valided RTC
#define OBSERVATION_INTERVAL       60000    // 60000 = 1 minute
#define DEFAULT_OBS_TRANSMIT_INTERVAL 15    // Transmit observations every N minutes Set to 15 for 15min Transmits

/*
 * ======================================================================================================================
 *  Relay Power Control Pin
 * ======================================================================================================================
 */
#define REBOOT_PIN            A0  // Trigger Watchdog or external relay to cycle power
#define HEARTBEAT_PIN         A1  // Watchdog Heartbeat Keep Alive

/*
 * ======================================================================================================================
 * System Status Bits used for report health of systems - 0 = OK
 * An unsigned long is 32 bits
 * 
 * OFF =   SSB &= ~SSB_PWRON
 * ON =    SSB |= SSB_PWROFF
 * 
 * ======================================================================================================================
 */
#define SSB_PWRON           0x1       // Set at power on, but cleared after first observation
#define SSB_SD              0x2       // Set if SD missing at boot or other SD related issues
#define SSB_RTC             0x4       // Set if RTC missing at boot
#define SSB_OLED            0x8       // Set if OLED missing at boot, but cleared after first observation
#define SSB_N2S             0x10      // Set when Need to Send observations exist
#define SSB_FROM_N2S        0x20      // Set in transmitted N2S observation when finally transmitted
#define SSB_AS5600          0x40      // Set if wind direction sensor AS5600 has issues                                                        
#define SSB_BMX_1           0x80      // Set if Barometric Pressure & Altitude Sensor missing
#define SSB_BMX_2           0x100     // Set if Barometric Pressure & Altitude Sensor missing
#define SSB_HTU21DF         0x200     // Set if Humidity & Temp Sensor missing
#define SSB_SI1145          0x400     // Set if UV index & IR & Visible Sensor missing
#define SSB_MCP_1           0x800     // Set if MCP9808 I2C Temperature Sensor missing
#define SSB_MCP_2           0x1000    // Set if MCP9808 I2C Temperature Sensor missing
#define SSB_MCP_3           0x2000    // Set if MCP9808 I2C Temperature Sensor missing
#define SSB_LORA            0x4000    // Set if LoRa Radio missing at startup
#define SSB_SHT_1           0x8000    // Set if SHTX1 Sensor missing
#define SSB_SHT_2           0x10000   // Set if SHTX2 Sensor missing
#define SSB_HIH8            0x20000   // Set if HIH8000 Sensor missing
#define SSB_VLX             0x40000   // Set if VEML7700 Sensor missing
#define SSB_PM25AQI         0x80000   // Set if PM25AQI Sensor missing
#define SSB_HDC_1           0x100000  // Set if HDC302x I2C Temperature Sensor missing
#define SSB_HDC_2           0x200000  // Set if HDC302x I2C Temperature Sensor missing
#define SSB_BLX             0x400000  // Set if BLUX30 I2C Sensor missing
#define SSB_LPS_1           0x800000  // Set if LPS35HW I2C Sensor missing
#define SSB_LPS_2           0x1000000 // Set if LPS35HW I2C Sensor missing
#define SSB_TLW             0x2000000 // Set if Tinovi Leaf Wetness I2C Sensor missing
#define SSB_TSM             0x4000000 // Set if Tinovi Soil Moisture I2C Sensor missing
#define SSB_TMSM            0x8000000 // Set if Tinovi MultiLevel Soil Moisture I2C Sensor missing


/*
  0  = All is well, no data needing to be sent, this observation is not from the N2S file
  16 = There is N2S data, This observation is not from the N2S file
  32 = This observation is from the N2S file. And when it was saved to the N2S file, the N2S file did not exist. So it is the first observation from the file.
  48 = This observation is from the N2S file. And when it was saved to the N2S file, the N2S file existed and this observation was appended.
*/ 

#define MAX_MSGBUF_SIZE 1024

/*
 * ======================================================================================================================
 *  Globals
 * ======================================================================================================================
 */

char msgbuf[MAX_MSGBUF_SIZE]; // Used to hold messages
char *msgp;                   // Pointer to message text
char Buffer32Bytes[32];       // General storage
int  LED_PIN = D7;            // Built in LED
bool TurnLedOff = false;      // Set true in rain gauge interrupt
unsigned long SystemStatusBits = SSB_PWRON; // Set bit 1 to 1 for initial value power on. Is set to 0 after first obs
bool JustPoweredOn = true;    // Used to clear SystemStatusBits set during power on device discovery
bool SendSystemInformation = true; // Send System Information to Particle Cloud. True means we will send at boot.

uint64_t lastOBS = 0;         // time of next observation
int countdown = 600;          // Exit station monitor/mode - when countdown reaches 0
                              // Protects against burnt out pin or forgotten jumper

uint64_t LastTimeUpdate = 0;
uint64_t LastTransmitTime = 0;

int  cf_reboot_countdown_timer = 79200; // There is overhead transmitting data so take off 2 hours from 86400s
                                        // Set to 0 to disable feature
int DailyRebootCountDownTimer;

uint64_t obs_tx_interval = DEFAULT_OBS_TRANSMIT_INTERVAL;  // Default OBS Transmit interval 15 Minutes

char imsi[16] = "";  // International Mobile Subscriber Identity

/*
 * ======================================================================================================================
 *  SD Card Stuff
 * ======================================================================================================================
 */
#define SD_ChipSelect D5                // GPIO 10 is Pin 10 on Feather and D5 on Particle Boron Board
SdFat SD;                               // File system object.
File SD_fp;
char SD_obsdir[] = "/OBS";              // Store our observations in this directory. At Poewer on it is created if not exist
bool SD_exists = false;                 // Set to true if SD card found at boot
char SD_n2s_file[] = "N2SOBS.TXT";      // Need To Send Observation file
uint32_t SD_n2s_max_filesz = 512 * 60 * 48;  // Keep a little over 2 days. When it fills, it is deleted and we start over.

char SD_sim_file[] = "SIM.TXT";         // File used to set Ineternal or External sim configuration
char SD_simold_file[] = "SIMOLD.TXT";   // SIM.TXT renamed to this after sim configuration set

char SD_wifi_file[] = "WIFI.TXT";       // File used to set WiFi configuration

char SD_TX5M_FILE[]  = "TXI5M.TXT";     // Transmit every 5 Minutes 
char SD_TX10M_FILE[] = "TXI10M.TXT";    // Transmit every 10 Minutes

char SD_INFO_FILE[] = "INFO.TXT";       // Store INFO information in this file. Every INFO call will overwrite content


#if (PLATFORM_ID == PLATFORM_BORON) || (PLATFORM_ID == PLATFORM_MSOM)
/*
 * ======================================================================================================================
 *  Power Management IC (bq24195)
 * ======================================================================================================================
 */
PMIC pmic;
#endif

/*
 * ======================================================================================================================
 *  Local Code Includes - Do not change the order of the below 
 * ======================================================================================================================
 */
#include "QC.h"                   // Quality Control Min and Max Sensor Values on Surface of the Earth
#include "SF.h"                   // Support Functions
#include "OP.h"                   // OutPut support for OLED and Serial Console
#include "CF.h"                   // Configuration File Variables
#include "TM.h"                   // Time Management
#include "LoRa.h"                 // LoRa
#include "Sensors.h"              // I2C Based Sensors
#include "WRD.h"                  // Wind Rain Distance
#include "EP.h"                   // EEPROM
#include "SDC.h"                  // SD Card
#include "OBS.h"                  // Do Observation Processing
#include "SM.h"                   // Station Monitor
#include "PS.h"                   // Particle Support Functions
#include "INFO.h"                 // Station Information

/*
 * ======================================================================================================================
 * HeartBeat() - Burns 250 ms 
 * ======================================================================================================================
 */
void HeartBeat() {
  digitalWrite(HEARTBEAT_PIN, HIGH);
  delay(250);
  digitalWrite(HEARTBEAT_PIN, LOW);
}

/*
 * ======================================================================================================================
 * BackGroundWork() - Take Sensor Reading, Check LoRa for Messages, Delay 1 Second for use as timming delay            
 * ======================================================================================================================
 */
void BackGroundWork() {
  // Anything that needs sampling every second add below. Example Wind Speed and Direction, StreamGauge

  uint64_t OneSecondFromNow = System.millis() + 1000;

  Wind_TakeReading();

  if (A4_State == A4_STATE_DISTANCE) {
    DistanceGauge_TakeReading();
  }

  if (PM25AQI_exists) {
    pm25aqi_TakeReading();
  }

  HeartBeat();  // Burns 250ms

  if (LORA_exists) {
    lora_msg_poll(); // Provides a 750ms delay
  }

  int64_t TimeRemaining = (OneSecondFromNow - System.millis());
  if ((TimeRemaining > 0) && (TimeRemaining < 1000)) {
    delay (TimeRemaining);
  }

  if (TurnLedOff) {   // Turned on by rain gauge interrupt handler
    digitalWrite(LED_PIN, LOW);  
    TurnLedOff = false;
  }
}

// You must use SEMI_AUTOMATIC or MANUAL mode so the battery is properly reconnected on
// power-up. If you use AUTOMATIC, you may be unable to connect to the cloud, especially
// on a 2G/3G device without the battery.
SYSTEM_MODE(SEMI_AUTOMATIC);

// https://docs.particle.io/cards/firmware/system-thread/system-threading-behavior/
SYSTEM_THREAD(ENABLED);

/*
 * ======================================================================================================================
 * setup() - runs once, when the device is first turned on.
 * ======================================================================================================================
 */
void setup() {
  // The device has booted, reconnect the battery.
#if (PLATFORM_ID == PLATFORM_BORON) || (PLATFORM_ID == PLATFORM_MSOM)
	pmic.enableBATFET();
#endif

  // Set Default Time Format
  Time.setFormat(TIME_FORMAT_ISO8601_FULL);

  // WatchDog 
  pinMode (REBOOT_PIN, OUTPUT);
  pinMode (HEARTBEAT_PIN, OUTPUT);

  pinMode (LED_PIN, OUTPUT);
  
  Output_Initialize();
  delay(2000); // Prevents usb driver crash on startup, Arduino needed this so keeping for Particle

  Serial_write(COPYRIGHT);
  Output (VERSION_INFO);
  delay(4000);

  // Set Daily Reboot Timer
  DailyRebootCountDownTimer = cf_reboot_countdown_timer;

  // Initialize SD card if we have one.
  SD_initialize();

  // Report if we have Need to Send Observations
  if (SD_exists && SD.exists(SD_n2s_file)) {
    SystemStatusBits |= SSB_N2S; // Turn on Bit
    Output("N2S:Exists");
  }
  else {
    SystemStatusBits &= ~SSB_N2S; // Turn Off Bit
    Output("N2S:None");
  }

  if (SD_exists && SD.exists(CF_NAME)) {
    SD_ReadConfigFile();
  }
  else {
    sprintf(msgbuf, "CF:NO %s", CF_NAME); Output (msgbuf);
    Output(msgbuf);
  }

  // Display EEPROM Information 
  EEPROM_Dump();

  // Check if correct time has been maintained by RTC
  // Uninitialized clock would be 2000-01-00T00:00:00
  stc_timestamp();
  sprintf (msgbuf, "%s+", timestamp);
  Output(msgbuf);

  // Read RTC and set system clock if RTC clock valid
  rtc_initialize();

  if (Time.isValid()) {
    Output("STC: Valid");
  }
  else {
    Output("STC: Not Valid");
  }

  stc_timestamp();
  sprintf (msgbuf, "%s=", timestamp);
  Output(msgbuf);

#if PLATFORM_ID == PLATFORM_ARGON
	pinMode(PWR, INPUT);
	pinMode(CHG, INPUT);
  //==================================================
  // Check if we need to program for WiFi change
  //==================================================
  WiFiPrintCredentials();
  WiFiChangeCheck();
  WiFiPrintCredentials();
#else
  //==================================================
  // Check if we need to program for Sim change
  //==================================================
  SimChangeCheck();
#endif

  //==================================================
  // Wind Speed and Rain Gauge Interrupt Based Sensors
  //==================================================

  // Optipolar Hall Effect Sensor SS451A - Wind Speed
  anemometer_interrupt_count = 0;
  anemometer_interrupt_stime = System.millis();
  attachInterrupt(ANEMOMETER_IRQ_PIN, anemometer_interrupt_handler, FALLING);

  // Optipolar Hall Effect Sensor SS451A - Rain Gauge
  raingauge1_interrupt_count = 0;
  raingauge1_interrupt_stime = System.millis();
  raingauge1_interrupt_ltime = 0;  // used to debounce the tip
  attachInterrupt(RAINGAUGE1_IRQ_PIN, raingauge1_interrupt_handler, FALLING);

  // Check SD Card for files to determine Transmit Interval for OBS 5,10 or 15 minutes
  TXI_Initialize();
  
  // Check SD Card for files to determine if pin A4 has a DIST, 2nd Rain Gauge or Raw file
  A4_Initialize();

  // Check SD Card for files to determine if pin A5 Raw file
  A5_Initialize();

  // Adafruit i2c Sensors
  bmx_initialize();
  htu21d_initialize();
  mcp9808_initialize();
  sht_initialize();
  hih8_initialize();
  si1145_initialize();
  vlx_initialize();
  blx_initialize();
  as5600_initialize();
  pm25aqi_initialize();
  hdc_initialize();
  lps_initialize();

  // Tinovi Mositure Sensors
  tlw_initialize();
  tsm_initialize();
  tmsm_initialize();
  
  // Derived Observations
  wbt_initialize();
  hi_initialize();
  wbgt_initialize();
  
  // Initialize RH_RF95 LoRa Module
  lora_initialize();
  lora_device_initialize();
  lora_msg_check();

  // Connect the device to the Cloud. 
  // This will automatically activate the cellular connection and attempt to connect 
  // to the Particle cloud if the device is not already connected to the cloud.
  // Upon connection to cloud, time is synced, aka Particle.syncTime()

  // Note if we call Particle.connect() and are not truely connected to the Cell network, Code blocks in particle call
  Particle.setDisconnectOptions(CloudDisconnectOptions().graceful(true).timeout(5s));
  Particle.connect();
  
  // Setup Remote Function to DoAction, Expects a parameter to be passed from Particle to control what action
  if (Particle.function("DoAction", Function_DoAction)) {
    Output ("DoAction:OK");
  }
  else {
    Output ("DoAction:ERR");
  }
  Wind_Distance_Air_Initialize(); // Will call HeartBeat()

#if (PLATFORM_ID == PLATFORM_BORON) || (PLATFORM_ID == PLATFORM_SOM)
  // Get International Mobile Subscriber Identity
  if ((RESP_OK == Cellular.command(callback_imsi, imsi, 10000, "AT+CIMI\r\n")) && (strcmp(imsi,"") != 0)) {
    sprintf (msgbuf, "IMSI:%s", imsi);
    Output (msgbuf);
  }
  else {
    Output("IMSI:NF");
  }
#endif

  if (Time.isValid()) {
    // We now a a valid clock so we can initialize the EEPROM and make an observation
    EEPROM_Initialize();
    OBS_Do();   
  }
}

/*
 * ======================================================================================================================
 * loop() runs over and over again, as quickly as it can execute.
 * ======================================================================================================================
 */
void loop() {
  // If Serial Console Pin LOW then Display Station Information
  if (0 && countdown && digitalRead(SCE_PIN) == LOW) {
    StationMonitor();
    BackGroundWork();
    countdown--;
  }
  else { // Normal Operation - Main Work

    // This will be invalid if the RTC was bad at poweron and we have not connected to Cell network
    // Upon connection to cell network system Time is set and this becomes valid
    if (Time.isValid()) {  
 
      // Set RTC from Cell network time.
      RTC_UpdateCheck();

      if (!eeprom_valid) {
        // We now a a valid clock so we can initialize the EEPROM
        EEPROM_Initialize();
      }

      // Perform an Observation, save in OBS structure, Write to SD
      if ( (System.millis() - lastOBS) > OBSERVATION_INTERVAL) {  // 1 minute
        I2C_Check_Sensors(); // Make sure Sensors are online
        OBS_Do(); 
      }

      if (SendSystemInformation && Particle.connected()) {
        INFO_Do(); // Function sets SendSystemInformation back to false.
      }

      // Time to Enable Network and Send Observations we have collected
      if ( (LastTransmitTime == 0) || ((System.millis() - LastTransmitTime) > (obs_tx_interval * 60 * 1000)) ) {
        if (Particle.connected()) {
          Output ("Connected");
          LastTransmitTime = System.millis();

          // Incase we are staying connected to the Cell network
          // request time synchronization from the Cell network - Every 4 Hours
          if ((System.millis() - LastTimeUpdate) > (4*3600*1000)) {
            // Note that this function sends a request message to the Cloud and then returns. 
            // The time on the device will not be synchronized until some milliseconds later when 
            // the Cloud responds with the current time between calls to your loop.

            // !!! What if we drop the Cell connection before we get a time update for the Cloud?
            //     If we have 15 observations to send, that 15s + 5s graceful particle disconnect
            Particle.syncTime();
            LastTimeUpdate = System.millis();
          }

          OBS_PublishAll();

          // If we waited too long for acks while publishing and this threw off our wind observations.
          // In that code ws_refresh was set to true for us to reinit wind data.
          if (ws_refresh) {
            Output ("WS Refresh Required");
            Wind_Distance_Air_Initialize();
          }

          // Update OLED and Console
          stc_timestamp();
          Output(timestamp);
          Output_CellBatteryInfo();

          // Shutoff System Status Bits related to initialization after we have logged first observation 
          JPO_ClearBits();
        }
      }

#if PLATFORM_ID == PLATFORM_ARGON
      // See if it's been an hour without a network connection and transmission of data
      // With Argon WiFi we have seen it stuck in Breathing Green - Trying to connect
      if (System.millis() - LastTransmitTime > (3600 * 1000)) {  
        // Been too long with out a network connection, lets reboot
        Output("1HR W/O NW: Rebooting");
        delay(5000);
        System.reset();
      }
#endif
      // Do background work, delays 1 Second
      BackGroundWork();
    }
    else {

      stc_timestamp();
      Output(timestamp);
      Output("ERR: No Clock");
      delay (DELAY_NO_RTC);
    }

    // ========================================================================================
    // Reboot Boot Every 22+ hours - Not using time but a loop counter.
    // ========================================================================================
    if ((cf_reboot_countdown_timer>0) && (--DailyRebootCountDownTimer<=0)) {
      Output ("Daily Reboot");

      if (Particle.connected()) {
        OBS_PublishAll();
      }

      EEPROM_SaveUnreportedRain();
      delay(1000);

      // Lets not rip the rug out from the modem. Do a graceful shutdown.
      Particle.disconnect();
      waitFor(Particle.disconnected, 1000);  // Returns true when disconnected from the Cloud.

#if (PLATFORM_ID == PLATFORM_BORON) || (PLATFORM_ID == PLATFORM_MSOM)
      // Be kind to the cell modem and try to shut it down
      Cellular.disconnect();
      delay(1000);
      Cellular.off();
#endif

      Output("Rebooting");  
      delay(1000);
   
      DeviceReset();

      // We should never get here, but just incase 
      Output("I'm Alive! Why?");  

#if (PLATFORM_ID == PLATFORM_BORON) || (PLATFORM_ID == PLATFORM_MSOM)
		  Cellular.on();
      delay(1000);
#endif

		  Particle.connect();

      DailyRebootCountDownTimer = cf_reboot_countdown_timer; // Reset count incase reboot fails

      // We need to reinitialize our wind readings before we can move on.
      Wind_Distance_Air_Initialize();
    }   

#if (PLATFORM_ID == PLATFORM_BORON) || (PLATFORM_ID == PLATFORM_MSOM)
    // ========================================================================================
    // Low Power Check and Power Off
    // ========================================================================================

    // Before we go do an observation and transmit, check our power status.
    // If we are not connected to a charging source and our battery is at a low level
    // then power down the display and board. Wait for power to return.
    // Do this at a high enough battery level to avoid the board from powering
    // itself down out of our control. Also when power returns to be able to charge
    // the battery and transmit with out current drops causing the board to reset or 
    // power down out of our control.

    // Could change the below to...
    // SEE: https://docs.particle.io/reference/device-os/api/system-calls/system-uptime/
    // int powerSource = System.powerSource();
    // if ((powerSource == POWER_SOURCE_BATTERY) && (System.batteryCharge() <= 10.0) {

    if (!pmic.isPowerGood() && (System.batteryCharge() <= 15.0)) {

      Output("Low Power!");

      if (Particle.connected()) {
        OBS_PublishAll(); 
        INFO_Do();
      }

      // While this function will disconnect from the Cloud, it will keep the connection to the network.
      Particle.disconnect();
      waitFor(Particle.disconnected, 1000);  // Returns true when disconnected from the Cloud.
      
      Cellular.disconnect();
      delay(1000);
      Cellular.off();
       
      if (LORA_exists) {
        rf95.sleep(); // Power Down LoRa. Note: it turn on when accessed
      }

      Output("Powering Down");

      OLED_sleepDisplay();
      delay(5000);

      // Disabling the BATFET disconnects the battery from the PMIC. Since there
		  // is no longer external power, this will turn off the device.
		  pmic.disableBATFET();

		  // This line should not be reached. When power is applied again, the device
		  // will cold boot starting with setup().

		  // However, there is a potential for power to be re-applied while we were in
		  // the process of shutting down so if we're still running, enable the BATFET
		  // again and reconnect to the cloud. Wait a bit before doing this so the
		  // device has time to actually power off.
		  delay(2000);

      OLED_wakeDisplay();   // May need to toggle the Display reset pin.
		  delay(2000);
		  Output("Power Re-applied");

      // WakeUp LoRa
      // May need to toggle LoRa Reset

		  pmic.enableBATFET();

		  Cellular.on();

		  Particle.connect();

      // We need to reinitialize our wind readings before we can move on.
      Wind_Distance_Air_Initialize();
    }
#endif
  }
}
