// [G]eneral-purpose [W]iFi-connected [I]ndicator [D]evice (GWID)

// SPDX-License-Identifier: MIT
// Copyright (c) 2025, 2026 Timothy Sakulich

//These definitions are used to embed sketch information in the code itself
#define BUILDVERSION "gwid-v1-00"
#define SKETCHNAME __FILE__
#define BUILDDATE __DATE__
#define BUILDTIME __TIME__

#define DEBUG 1 // If set to 1, the compiler will include code for verbose debugging
#if DEBUG==1
#define outputDebug(x); Serial.print(x);
#define outputDebugln(x); Serial.println(x);
#else
#define outputDebug(x); 
#define outputDebugln(x); 
#endif

//#define ALLOW_REMOTE_RESET // For debugging purposes ONLY--comment out

/*
OVERVIEW: This sketch is the firmware for a WiFi-connected indicator device 
comprised of a NodeMCU 1.0 Development Board connected to a NeoPixel Ring, 
(12 Pixel) and piezo buzzer. The device is through HTTP resource and query 
requests. When properly configured with a home automation hub IP address and port
on the same local network, the device can serve a remote alert or indicator 
triggered by home automation applications and rules. This sketch was designed and
tested in combination with a Hubitat Elevation C-8 hub running a custom device 
driver written in Groovy.  

A complete description of GWID's feature set, as well instructions for device 
hardware build, setup, and usage are available at GitHub repository
https://github.com/tdspublic/GWID


SELECT FEATURES:
- Display effects include flash, rotor, pulse, sos, strobe, trail and rainbow 
  with capability to tailor color, speed, brightness, and other settings
- User-supplied WiFi SSID and password credentials through an Access Portal 
  created with <WiFiManager.h> library functionality. This avoids hard coding 
  credentials into the sketch. The access portal is named GWID_AP with local IP 
  address 192.168.4.1. WiFi credentials are saved to flash memory and persist 
  across reboots and network disconnects until manually reset.
- "Factory Reset" - A SPST-NO tactile switch connected between D1 and GND on 
  the NodeMCU board provides capability to manually clear WiFi credentials 
  and other saved settings from flash memory, and reboots the device 
  to the GWID_AP access portal.
- Over-the-air updating capability using the <ArduinoOTA.h> library. 
- Ability to restrict GWID control to a specific client IP Address supplied
  by the user.

KEY HARDWARE COMPONENTS: 
- ESP8266/NodeMCU 1.0 Development Board - connect to 5VDC/2A USB Power Supply
- NeoPixel Ring (12 pixel) - PWR to NodeMCU VIN, GND to GND, IN to D7 (GPIO13) 
  via 330 ohn resister. Include a 1000uF Capacitor across NeoPixel GND and PWR
- Piezo Buzzer - connect positive to D2 (GPIO4) and negative to GND
- Tactile Switch - Connect one terminal of the microswitch to D2 and the
  other to GND. Connect 10k ohm pull up resister between D2 and NodeMCU VIN

USER GUIDE: 
Refer to GitHub repository https://github.com/tdspublic/GWID
*/


//******************************************************************************
//******************************* DECLARATIONS *********************************
//******************************************************************************

//Multiple libraries are required for this sketch
//Standard linbraries built into Arduino IDE:
#include <string.h>
#include <WiFiClient.h>
// Bundled with ESP8266 board core:
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h> 
#include <ESP8266WiFi.h>
// Third party libraries to install using Arduino IDE library manager:
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h> // "ArduinoJson by Benoit"
#include <Preferences.h> // "Preferences by Volodymyr Shymanskyy"
#include <WiFiManager.h> // "WiFi Manager by tzapu"


//NeoPixel device --  pin connections, global constants and global variables
#define NEOPIXEL_DATA_PIN D7              // NodeMCU GPIO pin connected to the NeoPixel
#define NUM_PIXELS 12                     // number of NeoPixels in the ring
const uint16_t RING_MIDPOINT = 6;         // defined for convenience in coding
const uint16_t STROBE_INTERVAL_MODULUS = 3;// defines the pixel interval used in strobe mode
                                          // e.g., 3 strobes every 3rd pixel
const uint16_t ROTOR_INTERVAL_MODULUS = 4;// defines the pixel interval used in rotor
                                          // e.g., 4 strobes every 4th pixel
Adafruit_NeoPixel strip(NUM_PIXELS, NEOPIXEL_DATA_PIN, NEO_GRB + NEO_KHZ800);

//Reset button -- pin connection, global constants and global variables
#define BUTTON_PIN D1          // NodeMCU pin connected to the button
const long LONG_PRESS_DURATION = 3000; // 3 seconds in milliseconds
long buttonPressTime = 0;

//Piezo buzzer -- pin connection, global constants and global variables
#define PIEZO_BUZZER D2       // used for piezo buzzer
bool piezoActive = false;    // piezo active default to off

//Onboard LED
#define LED_ONBOARD D4        // NodeMCU Built in LED 

//Declare WiFi and HTTP capabilities
ESP8266WebServer server(80); //Server on port 80
// Global variables for WiFi event handlers

WiFiEventHandler onDisconnectedHandler;
WiFiEventHandler onConnectedHandler;

// Initialize WiFiManager
WiFiManager wifiManager;
WiFiClient wificlient;
HTTPClient httpclient;

// Declare a preferences instance for saving data to flash memory that 
// persists across device reboots.
Preferences preferences;


//Declare variables for Saved IP/Port and to restrict client resource requests
IPAddress savedIP = (0,0,0,0);    // set through the /saveurl resource
unsigned int savedPort = 0;       // set through the /saveurl resource
bool hasSavedIP = false;          // set to true when user supplies valid IP/Port
char savedURL[80];                // created from saved IP and Port values
bool restrictClientFlag = false;  // set through the /saveurl resource
char gwidPin[7];                  // optional PIN set through GWID Access Portal
bool gwidPinIsSet = false;        // set to true if a PIN was provided 


//Declare variables that serve as parameters for device settings. Values are set 
//and reported hrough HTTP resource/query requests
char nameParam[16];              // Updated after WiFi connection is established
char modeParam[40];              // Current dsiplay mode for the device
char rgbParam[7];                // Current RGB color (HEX)
char patternParam[NUM_PIXELS+1]; // Stores pixel color settings (r,g,b,y,c,p,w,o)
uint16_t levelParam = 50;        // Current brightness level in the range 0-100
uint16_t speedParam = 2;         // Current speed setting (integer value for lookup tables)
char toneParam[4];               // "on" or "off" -- set in combination with piezoActive

// Misc global variables for controlling display effects in loop()
uint16_t stripBrightness;         // actual NeoPixel brightness in range 0-255 
uint16_t timingPattern = 0;       // Selected based on display mode
uint16_t currentLeadPixel = 0;   // Required for trail and rotor modes
uint16_t trailFade = 30;          // Value to decrement each successive pixel
bool     pixelMotionCw = true;    // true = Clockwise, false = Counter Clockwise
uint16_t pulseStripBrightness = 0; // Required for pulse mode
uint16_t pulseRange = 0;          // Required for pulse mode 
uint16_t pulseDirection = 1;      // Required for pulse mode
uint16_t rainbowTimer = 0;        // Required for rainbow mode

// Timing Patterns - this is a series of numbers defining the relative interval between 
// toggling pixels/piezo buzzer. A zero before the end of the pattern 
// indicates a short pattern that restarts when it detects a zero. 
// Undeclared global array elements automatically initialize to zeros
const byte NUM_TIMING_SEQUENCES = 7;  // number timing sequences in the array
const byte MAX_SEQUENCE_VALUES = 18;  // max number of values in a timing sequence
const uint16_t pattern[NUM_TIMING_SEQUENCES][MAX_SEQUENCE_VALUES] =
{ {1,0},               // constant for pattern, rainbow, solid
  {1,1,0},             // even tempo for flash, pulse, rotor, trail
  {10,1,0},            // strobe pattern 1 
  {10,1,1,1,0},        // strobe pattern 2 (default)
  {10,1,1,1,1,1,0},    // strobe pattern 3 
  {7, 1,1,1,1,1, 2, 3,2,3,2,3, 2, 1,1,1,1,1},  //for sos pattern
  {10,1,1,1,1,1,1,1,0} // strobe pattern for police
};

// The speed of the effect for a given mode is determined by the values in the 
// following lookup tables. The values are msptu = milliseconds per timing unit.  
// These values have been tested visually to ensure the effects (e.g., flash) 
// are neither too fast nor too slow to be useful or pleasing
const uint16_t NUM_SPEEDS = 5;  // Number of speeds contained in the following lookup tables
const uint16_t MSPTU_FOR_FLASH[]   = {1200,800,400,200,100};
const uint16_t MSPTU_FOR_POLICE[]  = {200,100,75,50,25};
const uint16_t MSPTU_FOR_PULSE[]   = {120,90,60,30,15};
const uint16_t MSPTU_FOR_RAINBOW[] = {30,20,10,5,2};
const uint16_t MSPTU_FOR_ROTOR[]   = {110,90,70,40,20};
const uint16_t MSPTU_FOR_SOS[]     = {250,200,150,80,60};
const uint16_t MSPTU_FOR_STROBE[]  = {300,200,100,50,25};
const uint16_t MSPTU_FOR_TRAIL[]   = {110,90,70,40,20};

bool flagPatternRestart = true;

// Declare global variables for tracking time in the main loop
byte step = 0;               // current step in pattern default to start
bool toggle = false;         // toggles each time step == 0
unsigned long currentTime;   // current time for test
unsigned long startTime;     // start time of this step

uint16_t msPerUnit = 0;          // initialize the timing per unit



const char * HELP_TEXT = "GWID Help ***************************************************** \r\n\
Full user manual available at https://github.com/tdspublic/GWID \r\n\
\r\n\
URL PATHS FOR STANDALONE GWID COMMANDS \r\n\
Basic syntax is http://<GWID_IPAddress>/<command> \r\n\
\r\n\
/<command>   Description \r\n\
-----------  ------------------------------------- \r\n\
/beep        Briefly sounds the piezo buzzer \r\n\
/getstate    Returns JSON format message with current device settings \r\n\
/getversion  Returns JSON format message device hardware ID, and build version, date & time \r\n\
/help        Provides basic info on controlling the GWID through HTTP \r\n\
/report      Returns HTML format report of device information. \r\n\
/saveurl     Sets saved IP and port, with optional argument for restricting control\r\n\
             http://<GWID_IPAddress>/saveurl?ip=<IPAddress>&port=<Port>&restrict \r\n\
             To clear previously saved values, use \r\n\
             http://<GWID_IPAddress>/saveurl?clear \r\n\
\r\n\
\r\n\
SET THE DISPLAY MODE \r\n\
Basic syntax is http://<GWID_IPAddress>/?<mode> \r\n\
\r\n\
?<mode>           Description \r\n\
----------------  ---------------------------- \r\n\
?pattern=<value>  Sets color of individual pixels (r,g,b,y,c,p,w,o) \r\n\
?flash            Flashing effect \r\n\
?police           Alternates red and blue strobe side to side \r\n\
?pulse            Pulsing effect \r\n\
?rotor            Three evenly-spaced pixels rotating around the ring \r\n\
?solid            Solid color \r\n\
?sos              Flashes S.O.S pattern \r\n\
?strobe           Four evenly-spaced, white pixels strobing \r\n\
?trail            One pixel moving with a trail of fading pixels behind it \r\n\
?rainbow          Rotating color rainbow \r\n\
\r\n\
\r\n\
MODIFY DISPLAY MODE PARAMETERS \r\n\
Basic syntax is http://<GWID_IPAddress>/?<parameter>=<value> \r\n\
\r\n\
?<parameter>=<value>  Description \r\n\
--------------------  -------------------------- \r\n\
?level=<value>        Sets overall brightness of the NeoPixel ring (0-100) \r\n\
?rgb=<value>          Sets primary pixel color to 6-character RGB hex string \r\n\
                      (omit the hashmark # prefix) \r\n\
?speed=<value>        Sets the speed of the visual and tone effect (0 - 4) \r\n\
?tone=<value>         Set piezo buzzer to active (on) or inactive (off) \r\n\
\r\n\
Set multiple <parameter>=<value> pairs using the ampersand (\"&\") character as a separator. \r\n\
Parameters can be set in combination with setting the display mode as well. \r\n\
http://<GWID_IPAddress>/?<mode>&<parameter1>=<value1>&<parameter2>=<value2>... \r\n\
";


// HTML content with placeholders
const char* REPORT_TEMPLATE PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<title>GWID Report Template</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  body {font-family: Arial, Helvetica, sans-serif;}
</style>
</head>
<body>
<h1>General-purpose WiFi-connected Indicator Device (GWID)</h1>
<h2>DEVICE INFORMATION</h2>
<p>
Currently Running Firmware Version %BUILDVERSION%<br>
Source Code %SKETCHNAME% (%BUILDDATE% %BUILDTIME%)
</p>
<table>
	<tbody>
    <tr><td>SSID:</td>             <td>%SSID%</td></tr>
    <tr><td>Board IP:</td>         <td>%IPADDRESS%</td></tr>
    <tr><td>Device Host Name:</td> <td>%HOSTNAME%</td></tr>
    <tr><td>MAC Address:</td>      <td>%MAC%</td></tr>
  </tbody>
</table>
<h2>SAVED CLIENT CONFIGURATION</h2>
<table>
  <tbody>
    <tr><td>Saved Client IP:</td>   <td>%SAVEDIP%</td></tr>
    <tr><td>Saved Client Port:</td> <td>%SAVEDPORT%</td></tr>
  </tbody>
</table>
<p>
%RESTRICTIONMESSAGE%
<br>
<strong>%PINFLAGMESSAGE%</strong>
</p>
<h2>CURRENT DEVICE STATE</h2>
<table>
  <tbody>
    <tr><td>Name:</td>     <td>%NAMEPARAM%</td></tr>
    <tr><td>Mode:</td>     <td>%MODEPARAM%</td></tr>
    <tr><td>RGB</td>       <td>%RGBPARAM%</td></tr>
    <tr><td>Pattern:</td>  <td>%PATTERNPARAM%</td></tr>
    <tr><td>Level:</td>    <td>%LEVELPARAM%</td></tr>
    <tr><td>Speed:</td>    <td>%SPEEDPARAM%</td></tr>
    <tr><td>Tone:</td>     <td>%TONEPARAM%</td></tr>
  </tbody>
</table>
</body>
</html>
)=====";

//******************************************************************************
//*************** FUNCTION PROTOTYPES (FORWARD DECLARATIONS) *******************
//******************************************************************************

// The following function prototypes (forward declarations) are not strictly 
// necessary under the Arduino IDE, which automatically generates them during 
// sketch compilation. However, some other development environments do not have 
// that feature. They are explicit in this sketch to support future portability.


//Functions to handle HTTP
void setCORS(void);
void syncToSavedIP(void);
void handleCaseInsensitiveRequest(void);
void handleBeep(void);
void handleGetState(void);
void handleGetVersion(void);
void handleHelp(void);
void handleModes(void);
void handleReport(void);
void handleReset(void);
void handleSaveURL(void);
//Miscellaneous Functions
bool isValidHexRgbColor(const char *data);
bool isValidInteger(String str);
bool setRingToHexColor(const char *data);
int  scale100to255 (int inputVal);
int  scale255to100 (int inputVal);
uint32_t hexToUint32(const char* hexString);
void colorWipe(uint32_t color, int wait);
void movePixelRotor(void);
void movePixelTrail(void);
void resetRingForNewMode(void);
void rgbToHex(int red, int green, int blue, char* hexColor);\
void setRingToLevel(void);
void setRingToPattern(void);
//Functions to Manage Access Portal and WiFi 
void ConfigurationPortal(void);
void onStationConnected(const WiFiEventStationModeConnected& evt);
void onStationDisconnected(const WiFiEventStationModeDisconnected& evt);


//******************************************************************************
//********************************** SETUP *************************************
//******************************************************************************

void setup() {
  Serial.begin(115200);
  delay(500); 
  outputDebugln("Serial connection established.");

  outputDebugln("============ Running setup()");

  //Onboard LED port Direction output
  pinMode(LED_ONBOARD, OUTPUT);
  digitalWrite(LED_ONBOARD, HIGH); //LED off

  //Piezo Direction output
  pinMode(PIEZO_BUZZER, OUTPUT);
  digitalWrite(PIEZO_BUZZER, LOW); //Piezo Buzzer off

  // Blink the onboard LED to indicate startup
  digitalWrite(LED_ONBOARD, LOW);
  delay(200);
  digitalWrite(LED_ONBOARD, HIGH);

  //NEOPIXEL initialize strip
  strip.begin();

  // Configure the button pin (requires external pull-up resistor)
  pinMode(BUTTON_PIN, INPUT);
  // Check if button is held down at boot to perform a factory reset
  // This is a robust method to catch a factory reset on startup.
  if (digitalRead(BUTTON_PIN) == LOW) {
    outputDebugln("Button held on startup. Restarting to GWID_AP access portal...");

    // Turn off all pixels
    strip.clear();
    strip.show(); 

    //Turn off wifi event handlers 
    onConnectedHandler = nullptr;
    onDisconnectedHandler = nullptr;
    // Erase saved WiFi credentials and restart the GWID device.
    wifiManager.resetSettings();        
    ESP.restart(); 
  }
  

  // NEOPIXEL Indicate Initializing WiFi (yellow at pixel 0)
  strcpy(modeParam, "booting");
  levelParam = 25;
  setRingToLevel();
  strcpy(patternParam,"yooooooooooo");
  setRingToPattern();
  strip.show();

  // If there no saved SSID and password then call the configuration portal
  // to obtain new credentials and reboot
  if (!wifiManager.getWiFiIsSaved()) ConfigurationPortal();

  // If execution gets to this point, then there are saved SSID and password credentials.
  // So attempt to connect. 

  // Do NOT start access portal on failure to connect.  
  // If connection keeps failing, the user may have to disconnect power to the GWID device  
  // and force a manual reset. (Press the reset button while reconnecting power).

  wifiManager.setEnableConfigPortal(false);  
  wifiManager.setConnectTimeout(60); 
  wifiManager.autoConnect("GWID_AP");
  
  WiFi.hostname().toCharArray(nameParam, sizeof(nameParam));
  //nameParam = WiFi.hostname();
    
  outputDebug("Device is connected to ");
  outputDebug(WiFi.SSID());
  outputDebug(" with IP address ");
  outputDebug(WiFi.localIP());
  outputDebug(" as ");
  outputDebug(WiFi.hostname());
  outputDebug(" with MAC Address: ");
  outputDebugln(WiFi.macAddress()); 
  
  //NEOPIXEL Indicate WiFi Connected (green at pixel 0)
  strcpy(modeParam, "connected");
  levelParam = 25;
  setRingToLevel();
  strcpy(patternParam,"gooooooooooo");
  setRingToPattern();
  strip.show();

  // Do a loop for one second
  unsigned long loopStart = millis();
  while ((millis() - loopStart) < 1000); {}


  //
  //  ****** THE FOLLOWING CODE MUST BE INCLUDED FOR OTA UPDATES ******
  // The following code enables over-the-air flashing of the ESP8266 NodeMCU sketch 
  // (without connection to serial port)

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA Ready");

  // Declare the handler for HTTP calls to the GWID device IP
  // to provide case-insensitive processing of the incoming URL
  server.onNotFound(handleCaseInsensitiveRequest);
  server.enableCORS(true);
  server.begin(); //Start server
  outputDebugln("HTTP server started");

  //Retrieve Saved IP and Port and GWID PIN from flash and set up the Saved URL
  //open in read only mode (second parameter = True)
  preferences.begin("savedURL_config", true);

  hasSavedIP = preferences.getBool ("hasSavedIP", false);
  if (hasSavedIP) {

    uint32_t IPAddressValue = preferences.getUInt("savedIP", 0); // Default to 0 if not found
    savedIP = IPAddressValue; 
    savedPort = preferences.getUInt("savedPort",0);

    String tempString = "http://" + savedIP.toString();
    if (savedPort > 0) tempString += ":" + String(savedPort);  // omit if port is 0
    tempString.toCharArray(savedURL,sizeof(savedURL));
    
    restrictClientFlag = preferences.getBool ("restrictClientFlag", false);

    outputDebug("Saved IP: ");
    outputDebugln(savedIP);
    outputDebug("Saved Port: ");
    outputDebugln(savedPort);
    outputDebug("Saved URL: ");
    outputDebugln(savedURL);
    outputDebug("Restrict_client: ");
    outputDebugln(restrictClientFlag);
  }
  else {
    outputDebugln("No Saved URL ");
    savedIP = IPAddress(0, 0, 0, 0);
    savedPort = 0; 
    strcpy(savedURL, "<< Not Set >>");
  }

  gwidPinIsSet = preferences.getBool ("gwidPinIsSet", false);
  if (gwidPinIsSet) {
    String tempString = preferences.getString("gwidPin", "Not Found");
    tempString.toCharArray(gwidPin,sizeof(gwidPin));
  }

  preferences.end();

  speedParam = 2;
  strcpy(toneParam, "off");
  piezoActive = false;

  //NEOPIXEL Indicate completed setup() and ready to accept commands (blue at pixel 0)
  //Initialize remaining global parameters used to control the GWID device as well
  strcpy(modeParam, "ready");
  strcpy(patternParam,"booooooooooo");
  setRingToPattern();
  strip.show();

  if (hasSavedIP) { 
    // if restrictClientFlag is not set, then add blue at pixel 6 
    // otherwise add red at pixel 6
    if (!restrictClientFlag) {
      strcpy(patternParam,"booooobooooo");
      setRingToPattern();
      strip.show();
    }
    else {
      strcpy(patternParam,"booooorooooo");
      setRingToPattern();
      strip.show();
    }
  }

  // Do a loop for one second
  loopStart = millis();
  while ((millis() - loopStart) < 1000); {}

  // If the saved IP & port are validated, Make an initial call to savedURL;
  if (hasSavedIP) syncToSavedIP();
}


//******************************************************************************
//********************************** LOOP **************************************
//******************************************************************************

void loop() {

  ArduinoOTA.handle();   //Handle OTA request
  
  server.handleClient();   // Listen for incoming HTTP traffic

  // Check for button press
  if (digitalRead(BUTTON_PIN) == LOW) {
    if (buttonPressTime == 0) {
      buttonPressTime = millis();
      outputDebugln("Button pressed. Hold for 3 seconds to reset WiFi and saved preferences.");
    }
    
    // Check for a long press
    if (millis() - buttonPressTime >= LONG_PRESS_DURATION) {
      outputDebugln("Long press detected! Calling Reset Configuration Portal...");
        // Turn off all pixels
        strip.clear();
        strip.show(); 
        //Turn off wifi event handlers 
        onConnectedHandler = nullptr;
        onDisconnectedHandler = nullptr;
        // Erase saved WiFi credentials and restart the GWID device.   
        wifiManager.resetSettings();   
        ESP.restart(); 
    }
  } else {
    // Reset the press timer when the button is released
    buttonPressTime = 0;
  }


  currentTime = millis();      // get current time for compare

  // The TIMER variables are counters used to create various effects in the lighting/beeper
  // as the code loops, dependending the state of the indicator
  if ( ((currentTime - startTime) > pattern[timingPattern][step]*msPerUnit) || flagPatternRestart ) {

    flagPatternRestart = false;

    if (strcmp(modeParam,"flash") == 0) { 
      strip.setBrightness(stripBrightness);
      if (step % 2 == 0) setRingToHexColor(rgbParam);
      else setRingToHexColor ("000000");
      strip.show();
    }

    else if (strcmp(modeParam,"pattern") == 0) {
      strip.setBrightness(stripBrightness);
      setRingToPattern();  
      strip.show();
    }

  if (strcmp(modeParam,"police") == 0) { 
      strip.setBrightness(stripBrightness);
      strip.setPixelColor (0,strip.Color( 0, 0, 0)); 
      strip.setPixelColor (RING_MIDPOINT,strip.Color( 0, 0, 0)); 
      
      if (step % 2 == 0) { 
          for (int i = 1; i < RING_MIDPOINT; i++) {
            if (toggle) { //strobe blue side
              strip.setPixelColor(i, strip.Color( 0, 0, 255));
              strip.setPixelColor(i+RING_MIDPOINT, strip.Color( 50, 0, 0));
            }
            else { // strobe red side
              strip.setPixelColor(i, strip.Color( 0, 0, 50));
              strip.setPixelColor(i+RING_MIDPOINT, strip.Color( 255, 0, 0));
            }
        }
      }
      else {
        for (int i = 1; i < RING_MIDPOINT; i++) {
          strip.setPixelColor(i, strip.Color( 0, 0, 50));
          strip.setPixelColor(i+RING_MIDPOINT, strip.Color( 50, 0, 0));
        }
      } 
      strip.show();
    }

    else if (strcmp(modeParam,"pulse") == 0) { 
      pulseStripBrightness = pulseStripBrightness + pulseDirection;
      // have to recalculate each iteration for whatever the current stripBrightness is set to
      pulseRange = 2*stripBrightness/3;

      // If reached the max upper level (i.e., the value of stripBrightness), start dimming
      if (pulseStripBrightness > stripBrightness) {
        pulseStripBrightness = stripBrightness;
        pulseDirection = -1;
      }

      // If reached the max lower level, start brightening
      else if (pulseStripBrightness < stripBrightness - pulseRange) {
        pulseStripBrightness = stripBrightness - pulseRange;
        pulseDirection = 1;
      }
      
      strip.setBrightness(pulseStripBrightness);
      colorWipe(hexToUint32(rgbParam),0);
      strip.show();
    }

    else if (strcmp(modeParam,"rainbow") == 0) {
      strip.setBrightness(stripBrightness);
      rainbowTimer = (rainbowTimer + 256) % 65536;
      strip.rainbow(rainbowTimer);
      strip.show();
    }

    else if (strcmp(modeParam,"rotor") == 0) {
      strip.setBrightness(stripBrightness); 
      if (step % 2 == 0) movePixelRotor();
      strip.show();
    }

    else if (strcmp(modeParam,"solid") == 0) {
      strip.setBrightness(stripBrightness);
      setRingToHexColor (rgbParam);
      strip.show();
    }

    else if (strcmp(modeParam,"sos") == 0) { 
      strip.setBrightness(stripBrightness);
      if (step % 2 == 0) setRingToHexColor (rgbParam);
      else setRingToHexColor ("000000");
      strip.show();
    }

    else if (strcmp(modeParam,"strobe") == 0) {
      strip.setBrightness(stripBrightness);
      setRingToHexColor (rgbParam);
      if (step % 2 == 0) {
      for (int i = 0; i < NUM_PIXELS; i++) {
          if ((i % STROBE_INTERVAL_MODULUS) == 0) {
            strip.setPixelColor(i, strip.Color( 255, 255, 255));
          }
        }
      }
      strip.show();
    }

    else if (strcmp(modeParam,"trail") == 0) {
      strip.setBrightness(stripBrightness); 
      if (step % 2 == 0) movePixelTrail();
      strip.show();
    }

    else { // any other modeParam
      strip.setBrightness(stripBrightness); 
      strip.show();
    }

    if (piezoActive) {
      digitalWrite(LED_ONBOARD, (step % 2 == 1)); // change led state
      digitalWrite(PIEZO_BUZZER, (step % 2 == 0)); //change piezo buzzer state
    }

    step++;  // next step in timing sequence

    startTime = currentTime;  // start next led pattern step time
    if ((step >= MAX_SEQUENCE_VALUES)                    // end of pattern
        || (pattern[timingPattern][step] == 0)) { // or short pattern
      // end of pattern processing
      step = 0;   //restart pattern
      if (step == 0) toggle = !toggle; // toggle for police mode
    }
  }

} // End of loop 






//******************************************************************************
//************************ Functions to handle HTTP ****************************
//******************************************************************************

void setCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");     // or specific origin
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");
}

void syncToSavedIP() {
  outputDebugln("============ Called syncToSavedIP()");
  
  // if there is not a validated saved URL then simply return
  if (!hasSavedIP) {
    outputDebugln("No validated saved URL to sync with.");
  }

  digitalWrite(LED_ONBOARD, HIGH); //LED off
  digitalWrite(PIEZO_BUZZER, LOW); //Piezo Buzzer off

  levelParam = 25;
  stripBrightness = scale100to255 (levelParam);
  strip.setBrightness(stripBrightness);

  // NEOPIXEL Indicate with blue pixels at fixed intervals around the NEOPIXEL
  strcpy(modeParam, "sync");
  levelParam = 25;
  setRingToLevel();
  strcpy(patternParam,"b00b00b00b00");
  setRingToPattern();  
  strip.show();

  // Call handleGetState(), which will send JSON notification to the saved URL and also 
  // reply to the requesting IP address (if applicable)
  handleGetState();
}



void handleCaseInsensitiveRequest () {
  outputDebugln("============ Called handleCaseInsensitiveRequest()");

  IPAddress remoteIP = server.client().remoteIP(); // Get the IP address of the client

  outputDebug("IP of Requesting Client: ");
  outputDebugln(remoteIP);

  String pathOnly = server.uri();
  pathOnly.toLowerCase();

  outputDebug("Incoming URI Path: ");
  outputDebugln(pathOnly);

  // Check if client is requesting an unrestricted resource
  // Call the apropriate handle, which will also respond to the remote client
  if (pathOnly.equals("/beep")) { 
    handleBeep();
    return;
  }
  if (pathOnly.equals("/help")) { 
    handleHelp();
    return;
  }  
  if (pathOnly.equals("/getstate")) { 
    handleGetState();
    return;
  }
  if (pathOnly.equals("/report")) { 
    handleReport();
    return;
  }
  if (pathOnly.equals("/getversion")) { 
    handleGetVersion();
    return; 
  }
  
  // Only process "/" if the resource is not restricted or if
  // the remoteIP is the same as the savedIP
  if (pathOnly.equals("/")) {
    if (!restrictClientFlag || (remoteIP == savedIP)) {
      if (server.args() > 0) handleModes();
      else {
        String message = "Invalid request at root -- no query arguments";
        outputDebugln(message);
        server.send(400, "text/plain", "400: " + message);
      }
    }
    else { // restricted resource called and remoteIP is not the savedIP
        String message = "Restricted resource -- requesting IP address does not match saved IP address";
        outputDebugln(message);
        server.send(401, "text/plain", "401: " + message);
    }
    return; 
  }

 
  // Only process "/saveurl" if the resource is not restricted or if
  // the remoteIP is the same as the savedIP
  if (pathOnly.equals("/saveurl")) { 
    if (!restrictClientFlag || (remoteIP == savedIP)) handleSaveURL();
    else { // restricted resource called and remoteIP is not the savedIP
      String message = "Restricted resource -- requesting IP address does not match saved IP address";
      outputDebugln(message); 
      server.send(401, "text/plain", "401: " + message);
    }      
    return;
  } 

  // optional functionality (see comments above)
  // Only process "/reset" if the resource is not restricted or if 
  // the remoteIP is the same as the savedIP
  #ifdef ALLOW_REMOTE_RESET
  if (pathOnly.equals("/reset")) { 
    if (!restrictClientFlag || (remoteIP == savedIP))handleReset();
    else { // restricted resource called and remoteIP is not the savedIP
      message = "Restricted resource -- requesting IP address does not match saved IP address";
      outputDebugln(message);
      server.send(401, "text/plain", "401: " + message);
    }      
    return; 
  }
  #endif

  // Otherwise no custom handler matches. Send a 404 Not Found response
  String message = pathOnly + " not found"; 
  outputDebugln(message);
  server.send(404, "text/plain", "404: " + message);
}


void handleBeep() {
  outputDebugln("============ Called handleBeep()");
  digitalWrite(LED_ONBOARD, LOW); //LED off
  digitalWrite(PIEZO_BUZZER, HIGH); //Piezo Buzzer off
  delay (40);
  digitalWrite(LED_ONBOARD, HIGH); //LED off
  digitalWrite(PIEZO_BUZZER, LOW); //Piezo Buzzer off
  
  // Send HTTP reply to requesting IP address
  server.send(200, "text/plain", "Device beep.");
}



void handleGetState() {
  outputDebugln("============ Called handleGetState()");

  StaticJsonDocument<200> doc;
  doc["Name"]       = nameParam;
  doc["Mode"]       = modeParam;
  doc["RGB"]        = rgbParam;
  doc["Pattern"]    = patternParam;
  doc["Level"]      = levelParam;
  doc["Speed"]      = speedParam;
  doc["Tone"]       = toneParam;

  String jsonString;
  serializeJson(doc, jsonString);


  // Send response to requesting remote IP (if set)

  IPAddress remoteIP = server.client().remoteIP();
  if (remoteIP) server.send(200, "application/json", jsonString);

  // If remote IP is not set, then this is GWID bootup condition
  // so see if there is a savedURL to post to
  else if (hasSavedIP) {
    outputDebug("Now Sending http json message: ");
    outputDebugln(jsonString);
    
    httpclient.begin(wificlient,savedURL);
    httpclient.POST (jsonString);
    httpclient.end();

    //NOTE: Future firmware upgrade could add error handling here 
    
    outputDebug("Message sent to ");
    outputDebugln(savedURL);   
    }

  // No remote IP and no savedURL
  else { 
    outputDebugln("No remoteIP and no savedURL -- no message sent.");
  }
}



void handleReport() {
  outputDebugln("============ Called handleReport()");

  // Uses PROGMEM and s.replace() to build an HTML response from the 
  // template named REPORT_TEMPLATE. This approach is supposed to be 
  // more efficient with memory and processing than other methods 
  // while still relatively easy to decipher the code logic.
  String s = FPSTR(REPORT_TEMPLATE); // Use FPSTR to access PROGMEM string
  s.replace("%BUILDVERSION%",BUILDVERSION);
  s.replace("%SKETCHNAME%",SKETCHNAME);
  s.replace("%BUILDDATE%",BUILDDATE);
  s.replace("%BUILDTIME%",BUILDTIME); 
  s.replace("%SSID%", WiFi.SSID());
  s.replace("%IPADDRESS%", WiFi.localIP().toString());
  s.replace("%HOSTNAME%",WiFi.getHostname());
  s.replace("%MAC%",WiFi.macAddress());
  if (hasSavedIP) {
    s.replace("%SAVEDIP%",savedIP.toString());
    if (savedPort > 0) {
      s.replace("%SAVEDPORT%",String(savedPort));
    }
    else{
      s.replace("%SAVEDPORT%","&ltNot Set&gt");
    }
  }
  else {
    s.replace("%SAVEDIP%","&ltNot Set&gt");
    s.replace("%SAVEDPORT%","&ltNot Set&gt");
  }
  if (restrictClientFlag) {
    s.replace("%RESTRICTIONMESSAGE%","<strong>Control RESTRICTED to the Saved Client IP Address</strong>");
  }
  else {
    s.replace("%RESTRICTIONMESSAGE%","Control Not Restricted to a specific IP Address");
  }
  if (gwidPinIsSet) {
    s.replace("%PINFLAGMESSAGE%","GWID PIN is Set");
  }
  else
  {
    s.replace("%PINFLAGMESSAGE%"," ");
  }
  s.replace("%NAMEPARAM%",nameParam);
  s.replace("%MODEPARAM%",modeParam);
  s.replace("%RGBPARAM%",rgbParam);
  s.replace("%PATTERNPARAM%",patternParam);
  s.replace("%LEVELPARAM%",String(levelParam));
  s.replace("%SPEEDPARAM%",String(speedParam));
  s.replace("%TONEPARAM%",toneParam);
  
  server.send(200, "text/html", s);

}



void handleSaveURL (){ 
  outputDebugln("============ Called handleSaveURL()");
  
  IPAddress remoteIP = server.client().remoteIP(); 
  IPAddress localIP = WiFi.localIP();
  IPAddress subnetMask = WiFi.subnetMask();
  // Explicitly cast to uint32_t to avoid "ambiguous overload" 
  // when using bitwise operators like '&' or '~'
  uint32_t localNet = (uint32_t)localIP & (uint32_t)subnetMask;
  uint32_t remoteNet = (uint32_t)remoteIP & (uint32_t)subnetMask;

  // Only accept from a device on the same local net
  if (remoteNet == localNet) {  
    outputDebugln("IP is on same local subnet.");
  
    bool valid_IP_value = false;
    bool valid_Port_value = false;
    IPAddress validatedIP;
    uint32_t validatedPort;

    if (server.hasArg("clear")) {
      // clear all previously stored values
      hasSavedIP = false;
      savedIP = IPAddress(0, 0, 0, 0);
      savedPort = 0; 
      restrictClientFlag = false;
      strcpy(savedURL, "<< Not Set >>");

      // Save to flash memory
      preferences.begin("savedURL_config", false);
      preferences.putBool("hasSavedIP", hasSavedIP);

      uint32_t ipAddressValue = (uint32_t)savedIP;
      preferences.putUInt("savedIP", ipAddressValue);
      preferences.putUInt("savedPort", savedPort);
      preferences.end();

      String message = ("Cleared saved IP and Port values.");
      // Send HTTP reply to requesting IP address
      server.send(200, "text/plain", message);

      //NEOPIXEL Indicate connected but cleared saved URL (blue at pixel 0)
      strcpy(modeParam, "ready");
      levelParam = 25;
      setRingToLevel();
      strcpy(patternParam,"booooooooooo");
      setRingToPattern();   
      return;  
    }

    else {
      if (server.hasArg("ip")) {
        String temp_string = server.arg("ip");
        int temp_string_length = temp_string.length();
        if (temp_string_length <= 15) {
          // IP Address Validation (basic check for format)
          IPAddress parsedIP;
          valid_IP_value = parsedIP.fromString(temp_string);
          if (valid_IP_value) validatedIP = parsedIP;
          else {
            outputDebugln("Invalid IP address!");         
          }
        }
      }
      else { // if IP not specified, use the IP from the remote request
        valid_IP_value = true;
        validatedIP = remoteIP;
      }

      if (server.hasArg("port")) {
        String temp_string = server.arg("port");
        if (isValidInteger(temp_string)) {
          int parsedPort = temp_string.toInt();
          // check for valid range -- setting to 0 is essentially a "null" value
          valid_Port_value = ((parsedPort >= 0) && (parsedPort <= 65535));
          if (valid_Port_value) validatedPort = parsedPort;
          else {          
            outputDebugln("Invalid port number!");        
          }
        }
      }
       else { // otherwise set port to 0
        valid_Port_value = true;
        validatedPort = 0;
      }

      hasSavedIP = valid_IP_value && valid_Port_value;

      // If validation passes, update saved IP & port and save to flash memory
      // otherwise make no changes to current values or flash memory
      if (hasSavedIP) {
        restrictClientFlag = server.hasArg("restrict");

        // Save to flash memory
        preferences.begin("savedURL_config", false);

        preferences.putBool("hasSavedIP", hasSavedIP);
        preferences.putBool("restrictClientFlag", restrictClientFlag);

        savedIP = validatedIP;
        savedPort = validatedPort;

        uint32_t ipAddressValue = (uint32_t)savedIP;
        preferences.putUInt("savedIP", ipAddressValue);
        preferences.putUInt("savedPort", savedPort);
        preferences.end();

        preferences.end();

        String tempString = "http://" + savedIP.toString();
        if (savedPort > 0) tempString += ":" + String(savedPort);  // omit if port is 0
        tempString.toCharArray(savedURL,sizeof(savedURL));

        String message = "Saved URL set to: " + String(savedURL);
        if (restrictClientFlag) message += " (Restrict client flag is set)";      
        outputDebugln(message);

        // Send HTTP reply to requesting IP address
        server.send(200, "text/plain", message);

        //NEOPIXEL Indicate connected and configured to saved URL
        strcpy(modeParam, "ready");
        levelParam = 25;
        setRingToLevel();
        if (!restrictClientFlag) { // blue at pixels 0 and 6
          strcpy(modeParam, "ready");
          strcpy(patternParam,"booooobooooo");
          setRingToPattern();
        }
        else { // blue at pixel 0 and red at pixel 6
          strcpy(modeParam, "ready");
          strcpy(patternParam,"booooorooooo");
          setRingToPattern();
        }
        strip.show(); 
        }
      else { 
        // Do not save savedIP or savedPort to flash memory
        String message = ("Saved IP and Port values failed validation! No changes made.");       
        outputDebugln(message);

        // Send HTTP reply to requesting IP address
        server.send(400, "text/plain", "400: " + message);
      }
    }
  } // end of validated local network
  else {
    String message = ("Requesting IP is NOT on same local network -- No changes made.");  
    outputDebugln(message);
    
    // Send HTTP reply to requesting IP address
    server.send(403, "text/plain", "403: " + message);
  }
}




void handleGetVersion() {
  outputDebugln("============ Called handleGetVersion()");
  

  StaticJsonDocument<200> doc;
  doc["Hardware_ID"]     = nameParam;
  doc["Build_Version"]   = BUILDVERSION;
  doc["Build_Date"]      = BUILDDATE;
  doc["Build_Time"]      = BUILDTIME;

  String jsonString;
  serializeJson(doc, jsonString);
  
  // Send HTTP reply to requesting IP address
  server.send(200, "application/json", jsonString);
}




#ifdef ALLOW_REMOTE_RESET
void handleReset() {
  String message = "Received reset.<br>";
  message += "Once the indicator light turns red, remove power from GWID device, then plug it back in.<br>";
  message += "The device will start up and create a local access portal named GWID_AP.<br>";
  message += "Connect to that portal in order to supply the new WiFi credentials.<br>";
  outputDebugln(message);
  
  // Send HTTP reply to requesting IP address
  server.send(200, "text/plain", message);

  // Turn off all pixels
  strip.clear();
  strip.show(); 

  //Turn off wifi event handlers 
  onConnectedHandler = nullptr;
  onDisconnectedHandler = nullptr;
  // Erase saved WiFi credentials and restart the GWID device.  
  wifiManager.resetSettings();      
  ESP.restart(); 
}
#endif




void handleModes() {   
  outputDebugln("============ Called handleModes()");

  int numargs = server.args();

  String message = "Found " + String(numargs) + " arguments.";
  outputDebugln(message);
    
  //////// START OF LOOP TO PROCESS QUERY ARGUMENTS ////////

  for (int i = 0; i < numargs; i++) {
    String argument_name = server.argName(i);
    argument_name.toLowerCase();
    String argument_value = server.arg(i);
    argument_value.toLowerCase();

    String message = "Setting GWID display to argument " + String(i) + ": " + argument_name + " = " + argument_value;
    outputDebugln(message);

    // PROCESS ACTIONS TO SET DISPLAY MODE TO pattern
    if (argument_name =="pattern") {
      // Set to a pattern of colors using letter labels for each pixel
      // r (red) g (green) b (blue) y (yellow) c (cyan) p (purple) w (white) o (off)
      
      String message = "Setting GWID display to pattern mode";
      outputDebugln(message);
      
      //Ensure argument length is same as NUM_PIXELS, otherwise it is invalid input
      if (argument_value.length() == NUM_PIXELS) {
        for(int i=0; i< NUM_PIXELS; i++) {patternParam[i] = argument_value.charAt(i);}
        patternParam[NUM_PIXELS] = '\0';  // terminate with null char

        setRingToPattern();

        strcpy(modeParam, "pattern");
        timingPattern = 0;
        msPerUnit = 0;

        resetRingForNewMode();      
        
        outputDebug("Set to ");
        outputDebug(modeParam);
        outputDebug(" -- computed average rgbParam is ");
        outputDebugln(rgbParam);       
      }
    }


    // PROCESS ACTIONS TO SET DISPLAY MODE TO flash
    else if (argument_name == "flash") {
      // Flash with a single color (hexidecimal value) 
      String message = "Setting GWID display to flash mode";   
      outputDebugln(message);    

      // Set the pattern string to all 'X's
      for(int i=0; i< NUM_PIXELS; i++) {patternParam[i]='*';}
      patternParam[NUM_PIXELS] = '\0'; // terminate with null char

      strcpy(modeParam, "flash");
      timingPattern = 1;  // use the timing pattern for flash
      msPerUnit =  MSPTU_FOR_FLASH[speedParam];

      // Check to see if "default" was specified for this mode
      // if so, set the mode to a white flash at medium speed without piezo
      if (argument_value == "default") {
        strcpy(rgbParam,"ffffff");
        levelParam = 50;
        stripBrightness = scale100to255(levelParam);
        speedParam = 2; // medium speed
        msPerUnit = MSPTU_FOR_FLASH[speedParam]; 
        strcpy(toneParam, "off");
        piezoActive = false;
      }
      resetRingForNewMode();
      
      outputDebug("Set to ");
      outputDebugln(modeParam);   
    } 

// PROCESS ACTIONS TO SET DISPLAY MODE TO police
    else if (argument_name == "police") {
      // Flash with a single color (hexidecimal value) 
      String message = "Setting GWID display to police mode";   
      outputDebugln(message);    

      // Set the pattern string
      patternParam[0] = 'o';
      patternParam[NUM_PIXELS-1] = 'o';
      for(int i=1; i<RING_MIDPOINT; i++) {
        patternParam[i]='b';
        patternParam[i+RING_MIDPOINT]='r';
        }
      patternParam[NUM_PIXELS] = '\0'; // terminate with null char

      strcpy(modeParam, "police");
      timingPattern = 6;  // use the timing pattern for strobe;
      msPerUnit =  MSPTU_FOR_POLICE[speedParam];

      strcpy(rgbParam,"ff00ff");
      // Check to see if "default" was specified for this mode
      // if so, set the mode to a white flash at medium speed without piezo
      if (argument_value == "default") {
        levelParam = 50;
        stripBrightness = scale100to255(levelParam);
        speedParam = 2; // medium speed
        msPerUnit = MSPTU_FOR_POLICE[speedParam]; 
        strcpy(toneParam, "off");
        piezoActive = false;
      }

      resetRingForNewMode();

      outputDebug("Set to ");
      outputDebugln(modeParam);     
    } 

    // PROCESS ACTIONS TO SET DISPLAY MODE TO pulse
    else if (argument_name == "pulse") {
      // Breathing effect with a single color (hexidecimal value)   
      String message = "Setting GWID display to pulse mode";     
      outputDebugln(message);     

      // Set the pattern string to all 'X's
      for(int i=0; i< NUM_PIXELS; i++) {patternParam[i]='*';}
      patternParam[NUM_PIXELS] = '\0'; // terminate with null char

      strcpy(modeParam, "pulse");
      timingPattern=1; // use the timing pattern for pulse
      msPerUnit =  MSPTU_FOR_PULSE[speedParam];

      // Check to see if "default" was specified for this mode
      // if so, set the mode to a white pulse at medium speed without piezo
      if (argument_value == "default") {
        strcpy(rgbParam,"ffffff");
        levelParam = 50;
        stripBrightness = scale100to255(levelParam);
        speedParam = 2; // medium speed
        msPerUnit = MSPTU_FOR_PULSE[speedParam]; 
        strcpy(toneParam, "off");
        piezoActive = false;
      }

      resetRingForNewMode();
     
      outputDebug("Set to ");
      outputDebugln(modeParam);    
    }
    
    // PROCESS ACTIONS TO SET DISPLAY MODE TO rotor
    else if (argument_name == "rotor") {
      // Flash with a single color (hexidecimal value) 
      String message = "Setting GWID display to rotor mode";
           
      outputDebugln(message);
      
      // Set the pattern string to all 'X's
      for(int i=0; i< NUM_PIXELS; i++) {patternParam[i]='*';}
      patternParam[NUM_PIXELS] = '\0'; // terminate with null char

      strcpy(modeParam, "rotor");
      timingPattern = 1;  // Use timing pattern for rotor
      msPerUnit =  MSPTU_FOR_ROTOR[speedParam];

      // Check to see if "default" was specified for this mode
      // if so, set the mode to a white dot trail at medium speed without piezo
      if (argument_value == "default") {
        strcpy(rgbParam,"ffffff");
        levelParam = 50;
        stripBrightness = scale100to255(levelParam);
        speedParam = 2; // medium speed
        pixelMotionCw = true; // clockwise direction
        msPerUnit = MSPTU_FOR_ROTOR[speedParam]; // medium speed
        strcpy(toneParam, "off");
        piezoActive = false;
      }

      // Check to see if a direction was specified for this mode
      else if (argument_value == "cw") pixelMotionCw = true;
      else if (argument_value == "ccw") pixelMotionCw = false;

      resetRingForNewMode();
     
      outputDebug("Set to ");
      outputDebugln(modeParam);    
    }

    // PROCESS ACTIONS TO SET DISPLAY MODE TO solid
    else if (argument_name == "solid") {
      // Set to a single color (hexidecimal value)

      String message = "Setting GWID display to solid mode";         
      outputDebugln(message);
          
      // Set the pattern string to all 'X's
      for(int i=0; i< NUM_PIXELS; i++) {patternParam[i]='*';}
      patternParam[NUM_PIXELS] = '\0'; // terminate with null char

      strcpy(modeParam, "solid");
      timingPattern=0;
      msPerUnit = 0;

      // Check to see if "default" was specified for this mode
      // if so, set the mode to white without piezo
      if (argument_value == "default") {
        strcpy(rgbParam,"ffffff");
        levelParam = 50;
        stripBrightness = scale100to255(levelParam);
        strcpy(toneParam, "off");
        piezoActive = false;
      }

      resetRingForNewMode();

      outputDebug("Set to ");
      outputDebugln(modeParam);      
    }


    // PROCESS ACTIONS TO SET DISPLAY MODE TO sos
    else if (argument_name == "sos") {
      // Flash with a single color (hexidecimal value) 
      String message = "Setting GWID display to s.o.s. mode";
           
      outputDebugln(message);      

      // Set the pattern string to all 'X's
      for(int i=0; i< NUM_PIXELS; i++) {patternParam[i]='*';}
      patternParam[NUM_PIXELS] = '\0'; // terminate with null char

      strcpy(modeParam, "sos");
      timingPattern = 5;  // use the timing pattern for sos
      msPerUnit =  MSPTU_FOR_SOS[speedParam];

      // Check to see if "default" was specified for this mode
      // if so, set the mode to a white flash at medium speed without piezo
      if (argument_value == "default") {
        strcpy(rgbParam,"ffffff");
        levelParam = 50;
        stripBrightness = scale100to255(levelParam);
        speedParam = 2; // medium speed
        msPerUnit = MSPTU_FOR_SOS[speedParam];
        strcpy(toneParam, "off");
        piezoActive = false;
      }

      resetRingForNewMode();
     
      outputDebug("Set to ");
      outputDebugln(modeParam);     
    } 


    // PROCESS ACTIONS TO SET DISPLAY MODE TO strobe
    else if (argument_name == "strobe") {
      // Strobe wtih a single color (hexidecimal value)
      String message = "Setting GWID display to strobe mode";
      
      outputDebugln(message);
      
      // Set the pattern string to all '*'s, but with 'W's at the strobing pixels
      for (int i = 0; i < NUM_PIXELS; i++) {
        if ((i % STROBE_INTERVAL_MODULUS) == 0) {
          //strip.setPixelColor(i, 16, 16, 0);
          patternParam[i]='w';
          }
        else {
          //strip.setPixelColor(i, 0, 0, 0);
          patternParam[i]='*';
        }
      }
      patternParam[NUM_PIXELS] = '\0'; // terminate with null char

      strcpy(modeParam, "strobe");
      timingPattern = 3;  // Use the timing pattern for double strobe unless changed
                    // by a passed argument (see below)
      msPerUnit =  MSPTU_FOR_STROBE[speedParam];

      // Check to see if "default" was specified for this mode
      // if so, set the mode to a white strobe at medium speed without piezo
      if (argument_value == "default") {
        strcpy(rgbParam,"222222");
        levelParam = 50;
        stripBrightness = scale100to255(levelParam);
        speedParam = 2; // medium speed
        msPerUnit = MSPTU_FOR_STROBE[speedParam]; 
        strcpy(toneParam, "off");
        piezoActive = false;
      }
      // Check to see if a strobe pattern was specified for
      // this mode 
      else if (argument_value == "1") timingPattern=2;
      else if (argument_value == "2") timingPattern=3;
      else if (argument_value == "3") timingPattern=4;

      resetRingForNewMode();
     
      outputDebug("Set to ");
      outputDebugln(modeParam);     
    }

    // PROCESS ACTIONS TO SET DISPLAY MODE TO trail
    else if (argument_name == "trail") {
      // Flash with a single color (hexidecimal value) 
      String message = "Setting GWID display to trail mode";
           
      outputDebugln(message);     

      // Set the pattern string to all 'X's
      for(int i=0; i< NUM_PIXELS; i++) {patternParam[i]='*';}
      patternParam[NUM_PIXELS] = '\0'; // terminate with null char

      strcpy(modeParam, "trail");
      timingPattern = 1;  // Use timing pattern for trail
      msPerUnit =  MSPTU_FOR_TRAIL[speedParam];

      // Check to see if "default" was specified for this mode
      // if so, set the mode to a white dot trail at medium speed without piezo
      if (argument_value == "default") {
        strcpy(rgbParam,"ffffff");
        levelParam = 50;
        stripBrightness = scale100to255(levelParam);
        speedParam = 2; // medium speed
        pixelMotionCw = true; // clockwise direction
        msPerUnit = MSPTU_FOR_TRAIL[speedParam]; // medium speed
        strcpy(toneParam, "off");
        piezoActive = false;
      }

      // Check to see if a direction was specified for this mode
      else if (argument_value == "cw") pixelMotionCw = true;
      else if (argument_value == "ccw") pixelMotionCw = false;

      resetRingForNewMode();
     
      outputDebug("Set to ");
      outputDebugln(modeParam);     
    } 

    // PROCESS ACTIONS TO SET DISPLAY MODE TO rainbow
    else if (argument_name == "rainbow") {
      // Flash with a single color (hexidecimal value) 
      String message = "Setting GWID display to rainbow mode";
           
      outputDebugln(message);     

      strcpy(modeParam, "rainbow");
      strcpy(rgbParam,"ffffff");
      timingPattern = 0; 
      msPerUnit =  MSPTU_FOR_RAINBOW[speedParam];

      for(int i=0; i< NUM_PIXELS; i++) {patternParam[i]='*';}
      patternParam[NUM_PIXELS] = '\0';  // terminate with null char

      // Check to see if "default" was specified for this mode
      if (argument_value == "default") {
        levelParam = 50;
        stripBrightness = scale100to255(levelParam);
        speedParam = 2; // medium speed
        msPerUnit =  MSPTU_FOR_RAINBOW[speedParam];
        strcpy(toneParam, "off");
        piezoActive = false;
      }

    resetRingForNewMode();
    
      outputDebug("Set to ");
      outputDebugln(modeParam);     
    }

    // PROCESS ACTIONS TO SET level PARAMETER
    else if (argument_name == "level") {
      // Set the overall brightness level

      String message = "Setting GWID display to level = " + argument_value;    
      outputDebugln(message);
    
      // Validate that the input is an integer in the right range
      // then set the level and computed brightness accordingly
      String temp_str = argument_value;
      if (isValidInteger (temp_str)) {
        int temp_int = temp_str.toInt();
        if (temp_int >= 0 && temp_int <= 100) {
          // input is valid, so set the level and brightness variables
          levelParam = temp_int;
          stripBrightness = scale100to255 (levelParam);  //Scale to NeoPixel range 0 - 255
          strip.setBrightness(stripBrightness); 
        }
        else {         
          outputDebugln("Value out of range -- ignored.");
          
        }
      }
      else { // Not a valid input, ignore the input       
        outputDebugln("Invalid argument value -- ignored.");        
      } 
    }

    // PROCESS ACTIONS TO SET speed PARAMETER
    else if (argument_name == "speed") {
      //Set the speed of the display mode

      String message = "Setting GWID display to speed = " + argument_value;      
      outputDebugln(message);
      
      // Validate that the input is an integer in right range
      // then use the lookup table corresponding to the current 
      // display mode to set the timing msPerUnit variable
      String temp_str = argument_value;
      if (isValidInteger (temp_str)) {
        int temp_int = temp_str.toInt();
        if (temp_int >= 0 && temp_int <= (NUM_SPEEDS-1)) {
          speedParam = temp_int; 
          // input is valid, so set the actual timing based on current mode
          if      (strcmp(modeParam,"flash") == 0) msPerUnit = MSPTU_FOR_FLASH[speedParam]; 
          else if (strcmp(modeParam,"police") == 0) msPerUnit = MSPTU_FOR_POLICE[speedParam]; 
          else if (strcmp(modeParam,"pulse") == 0) msPerUnit = MSPTU_FOR_PULSE[speedParam]; 
          else if (strcmp(modeParam,"rainbow") == 0) msPerUnit = MSPTU_FOR_RAINBOW[speedParam];
          else if (strcmp(modeParam,"rotor") == 0) msPerUnit = MSPTU_FOR_ROTOR[speedParam]; 
          else if (strcmp(modeParam,"sos") == 0) msPerUnit = MSPTU_FOR_SOS[speedParam]; 
          else if (strcmp(modeParam,"strobe") == 0) msPerUnit = MSPTU_FOR_STROBE[speedParam]; 
          else if (strcmp(modeParam,"trail") == 0) msPerUnit = MSPTU_FOR_TRAIL[speedParam]; 
          else {} // do nothing -- timing not used in to pattern or solid modes
        }
        else { // Not a valid input, ignore the input
           
          outputDebugln("Value out of range -- ignored.");
         
        }
      }
      else { // Not a valid input, ignore the input       
        outputDebugln("Invalid argument value -- ignored.");        
      } 

    }

    // PROCESS ACTIONS TO SET rgb PARAMETER
    else if (argument_name == "rgb") {
      //Set the flash pattern

      String message = "Setting GWID display to rgb = " + argument_value;     
      outputDebugln(message);
      
      //Ensure argument was 6 characters, otherwise it is invalid input
      if (argument_value.length() == 6) {
        argument_value.toLowerCase();

        //Attempt to set the color -- if valid RGB hex value, then update indicator state variables
        if (isValidHexRgbColor(argument_value.c_str())) {
          argument_value.toCharArray(rgbParam, sizeof(rgbParam)); 
          
          outputDebug("Set to rgbParam ");
          outputDebugln(rgbParam);          
        }
        else {// ignore the input and do nothing         
          outputDebugln("Invalid argument value -- ignored.");         
        }
      }
      else {// ignore the input and do nothing       
        outputDebugln("Invalid argument value -- ignored.");       
      }
    }

    // PROCESS ACTIONS TO SET tone PARAMETER
    else if (argument_name == "tone") {
      //Set the piezo on or off
      String message = "Setting GWID to tone = " + argument_value;
     
      outputDebugln(message);
      
      if (argument_value == "on") {
        strcpy(toneParam, "on");
        piezoActive = true;
      }
      else if (argument_value == "off"){
        strcpy(toneParam, "off");
        piezoActive = false;
        //immediately turn off
        digitalWrite(LED_ONBOARD, 1) ; // turn off light
        digitalWrite(PIEZO_BUZZER, 0); //turn off piezo
      }
      else {// ignore the input and do nothing       
        outputDebugln("Invalid argument value -- ignored.");       
      }
    }
    else {
      // ignore unrecognized parameter name    
      outputDebugln("Invalid argument name -- ignored.");     
    } 

  }

  // Call handleGetState() to generate JSON response to the remote client
  handleGetState();
} 


void handleHelp() {  
  outputDebugln("============ Called handleHelp()"); 
  // Send HTTP reply to requesting IP address
  server.send(200, "text/plain", HELP_TEXT);
}




//******************************************************************************
//************************* Miscellaneous Functions ****************************
//******************************************************************************

int scale100to255 (int inputVal) {
  // Convert the input from 0-100 range (used by the level= parameter) to the 
  // 0-255 range (needed by NeoPixel strip.brightness()). On the NeoPixel, 
  // small incremental changes in the numerical brightness are perceptually 
  // more dramatic at the lower end of the range (closer to 0) than at the 
  // higher end (closer to 255). In recognition of that, this sketch uses a 
  // nonlinear quadratic formula for the conversion. At the low end of the 
  // range, small changes in level result in small changes in the calculated
  // brightness value. At the high end of the scale, small changes in level 
  // result in much larger changes to the calculated brightness value.

  // The quadratic curve passes through (x,y) coordinates (0,0) and (100,255) 
  // with slope=1 at (0,0). The equation is y = 0.0155*x^2 + x. 

  // Using float for the calculations prevents integer truncation.
  float y_float = 0.0155 * (float)inputVal * (float)inputVal + (float)inputVal;

  //Now round to nearest integer
  int y_integer = round(y_float);

  return y_integer;
}

/*
int scale255to100 (int inputVal) {
  // This is the inverse function of scale100to255(). It converts the 0-255 range 
  // (used for NeoPixel strip.brightness()) back to the 0-100 range (used by the 
  // level= parameter). While not used by GWID functionality as currently 
  // designed, the funciton is provided here for future reference in the event 
  // that new device functionality can make use of it.

  // Using float for the calculations prevents integer truncation. 

  float coefficient = 0.0155;
  float result = (float)inputVal/coefficient + (0.5/coefficient)*(0.5/coefficient);
  result = sqrtf(result) - (0.5/coefficient);
  int x_integer = round(result);
  return x_integer;
}
*/

// Helper function to check if a String contains only digits
bool isValidInteger(String str) {
  if (str.length() == 0) return false;
  // Check for an optional sign at the beginning
  int startIndex = 0;
  if (str.charAt(0) == '+' || str.charAt(0) == '-') {
    startIndex = 1;
  }
  // Check that all subsequent characters are digits
  for (int i = startIndex; i < str.length(); i++) {
    if (!isDigit(str.charAt(i))) return false;
  }
  return true;
}

// Helper function to check if a char array is a valid hexadecimal
// RGB color value
bool isValidHexRgbColor(const char *data) {
  int red, green, blue;
  int result = sscanf(data, "%02x%02x%02x", &red, &green, &blue);
  if (result == 3) {
    return true;
  }
  else return false;
}


uint32_t hexToUint32(const char* hexString) {
  // strtoul(string, endptr, base)
  // string: The C-style string containing the hexadecimal number.
  // endptr: A pointer to a char pointer that will point to the first character
  //         in the string that is not part of the number. Can be NULL if not needed.
  // base: The base of the number (16 for hexadecimal).
  return strtoul(hexString, NULL, 16);
}


void rgbToHex(int red, int green, int blue, char* hexColor) {
  // Ensure values are within the 0-255 range
  red = constrain(red, 0, 255);
  green = constrain(green, 0, 255);
  blue = constrain(blue, 0, 255);

  // Use sprintf to format the hexadecimal string into the char array
  // %02x ensures two lowercase hexadecimal digits are used for each component
  sprintf(hexColor, "%02x%02x%02x", red, green, blue);
}


bool setRingToHexColor(const char *data) {
  int red, green, blue;
  int result = sscanf(data, "%02x%02x%02x", &red, &green, &blue);
  if (result == 3) {
    colorWipe(strip.Color(red,   green,   blue), 0); // Red
    return true;
  }
  else return false;
}


// Fill strip pixels one after another with a color. Strip is NOT cleared
// first; anything there will be covered pixel by pixel. Pass in color
// (as a single 'packed' 32-bit value, which you can get by calling
// strip.Color(red, green, blue) as shown in the loop() function above),
// and a delay time (in milliseconds) between pixels.
void colorWipe(uint32_t color, int wait) {
  for (int i = 0; i < strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
  }
}

// This method converts levelParam (0-100) to a NeoPixel brightness (0-255)
// and sets the strip brightness accordingly
void setRingToLevel() {
  stripBrightness = scale100to255 (levelParam);
  strip.setBrightness(stripBrightness);
}

//This method sets the strip to the patternParam and computes an "average" color 
//across the pixels to update rgbParam
void setRingToPattern() {
  uint32_t red = 0;
  uint32_t green = 0;
  uint32_t blue = 0;

  for(int i=0; i< NUM_PIXELS; i++){
    if      (patternParam[i] == 'o') {strip.setPixelColor(i,0,0,0);}
    else if (patternParam[i] == 'r') {strip.setPixelColor(i,255,0,0); red += 255;}
    else if (patternParam[i] == 'g') {strip.setPixelColor(i,0,255,0); green += 255;}
    else if (patternParam[i] == 'b') {strip.setPixelColor(i,0,0,255); blue += 255;}
    else if (patternParam[i] == 'y') {strip.setPixelColor(i,255,255,0); red += 255; green += 255;}
    else if (patternParam[i] == 'c') {strip.setPixelColor(i,0,255,255); green += 255; blue += 255;}
    else if (patternParam[i] == 'p') {strip.setPixelColor(i,255,0,255); red += 255; blue += 255;}
    else if (patternParam[i] == 'w') {strip.setPixelColor(i,255,255,255); red += 255; green += 255; blue += 255;}
    //simply ignore other values    
  }

  red = round(red/NUM_PIXELS);
  green = round(green/NUM_PIXELS);
  blue = round(blue/NUM_PIXELS);

  // Udpdate the values of global parameters
  rgbToHex(red, green, blue, rgbParam);
}


// Fades a trail of pixels
void movePixelTrail() {
    if (pixelMotionCw) {
      // Move the dot Clockwise
      // Advance to the next pixel, wrapping around the ring cw
      currentLeadPixel = (currentLeadPixel + 1) % NUM_PIXELS;
    }
    else {
      // Move the dot Counter Clockwise
      // Reverse to the prior pixel, wrapping around the ring ccw
      if (currentLeadPixel == 0) currentLeadPixel = NUM_PIXELS-1;
      else currentLeadPixel = (currentLeadPixel - 1);
    }

    // Set the new pixel to the trail color
    strip.setPixelColor(currentLeadPixel, hexToUint32(rgbParam));

    // Fade the trail
    for (int i = 0; i < NUM_PIXELS; i++) {
    // Get the current color of the pixel
    uint32_t color = strip.getPixelColor(i);
    // Extract individual R, G, B components
    byte red = (color >> 16) & 0xFF;
    byte green = (color >> 8) & 0xFF;
    byte blue = color & 0xFF;
    
    // Reduce brightness of each color component
    red = (red > trailFade) ? red - trailFade : 0;
    green = (green > trailFade) ? green - trailFade : 0;
    blue = (blue > trailFade) ? blue - trailFade : 0;
    
    // Set the new, faded color
    strip.setPixelColor(i, strip.Color(red, green, blue));
    }
}

// Rotates a pattern of pixels
void movePixelRotor() {   
    if (pixelMotionCw) {
      // Move the dot Clockwise
      // Advance to the next pixel, wrapping around the ring cw
      currentLeadPixel = (currentLeadPixel + 1) % NUM_PIXELS;
    }
    else {
      // Move the dot Counter Clockwise
      // Reverse to the prior pixel, wrapping around the ring ccw
      if (currentLeadPixel == 0) currentLeadPixel = NUM_PIXELS-1;
      else currentLeadPixel = (currentLeadPixel - 1);
    }

    // Set the new pixel to the trail color

    for (int i = 0; i < NUM_PIXELS; i++) {
      int temp_pixel = (i + currentLeadPixel) % NUM_PIXELS;
      if ((i % ROTOR_INTERVAL_MODULUS) == 0) strip.setPixelColor(temp_pixel, hexToUint32(rgbParam));
      else strip.setPixelColor(temp_pixel, strip.Color(0,0,0));
    }
}


void resetRingForNewMode() { // used to reset when starting new timing or pixel modality     
      outputDebugln("Resetting for new pattern");     

      flagPatternRestart = true;
      startTime = millis();
      step = 0;                   // start at beginning of timing pattern 
      toggle = false;
      currentLeadPixel = 0;        // start at the top
      
      // Clear other pixel states 
      // This is mainly to clear the "trail" of pixels if trail was previous mode
      strip.clear();
}    



//******************************************************************************
//***************** Functions to Manage Access Portal and WiFi *****************
//******************************************************************************


void ConfigurationPortal() { 
  outputDebugln("Resetting WiFi and Preferences...");
  
  //Turn off wifi event handlers 
  onConnectedHandler = nullptr;
  onDisconnectedHandler = nullptr;
  //First, double check to erase any saved WiFi credentials
  wifiManager.resetSettings();  

  preferences.begin("savedURL_config", false);
  preferences.clear();
  preferences.end();

  digitalWrite(LED_ONBOARD, LOW);
  //NEOPIXEL Indicate ConfigurationPortal (red at pixel 0)
  const int temp_LED_level = 255;
  const int pixel_brightness = 16;
  strip.setBrightness(temp_LED_level);
  strip.clear();
  strip.setPixelColor(0, pixel_brightness, 0, 0);
  strip.show(); 

  // Clear out all old preferences
 
  outputDebugln("Clearing preferences...");
  
  // Set up On Demand Access Portal to obtain SSDI, PW and preferences to save
  preferences.begin("savedURL_config", false);

  wifiManager.setConfigPortalTimeout(180);  //180 seconds
  WiFiManagerParameter custom_gwidPin_param("gwidPin", "GWID PIN", gwidPin, 6);
  wifiManager.addParameter(&custom_gwidPin_param);

  //wifiManager.setCustomHeadElement("<style>body{max-width:500px;margin:auto}</style>");
  wifiManager.setConnectTimeout(60);
  if (!wifiManager.startConfigPortal("GWID_AP")) {     
      outputDebugln("Unable to connect to SSID.  Robooting...");
      //flash red LED here?
      ESP.restart(); 
  }
  
  outputDebugln("Control Returned from GWID_AP access portal.");
  
  // Retrieve values from custom parameters
  strcpy(gwidPin, custom_gwidPin_param.getValue());

  preferences.putBool("hasSavedIP", false);

  // Check if user set a PIN, then update saved preferences according;y
  gwidPinIsSet = (strcmp(gwidPin,"") != 0);
  preferences.putBool("gwidPinIsSet", gwidPinIsSet);
  if (gwidPinIsSet) {
    preferences.putString("gwidPin", gwidPin);    
      outputDebug("GWID PIN was set to ");
      outputDebugln(gwidPin);   
  }
  else {   
      outputDebugln("GWID PIN was not set.");   
  }

  preferences.end();
 
  outputDebugln("Exiting Config Portal. Rebooting in 3 seconds...");
  
  //Clear the NeoPixel pixel of any other indications (e.g., from WiFi events) 
  strip.clear();

  //NEOPIXEL indicate succesful return from GWID_AP access portal and ready to reboot.
  //Written as a nonblocking loop, without using delay() statement, so as not to interfere 
  //with stability of newly established WiFi connection

  int blinkCount = 0; // Reset blink counter
  int pixelnumber = 0;
  bool temptoggle = false;
  unsigned long temppreviousMillis = millis();
  while (blinkCount < 8) { // blink pixel 0 four times
    unsigned long tempcurrentMillis = millis();
    if (tempcurrentMillis - temppreviousMillis >= 500) {
      temppreviousMillis = tempcurrentMillis;
      temptoggle = !temptoggle; // Toggle the state
      digitalWrite(LED_ONBOARD, temptoggle); // toggle the onboard LED
      strip.setPixelColor(0, 0, pixel_brightness * temptoggle, 0);
      strip.show(); 
      blinkCount++; 
    }
    yield(); // Allow other tasks to run so as not to interfere 
             // with stability of newly established WiFi connection
  }
  
  // Restart the ESP8266 to apply changes
  ESP.restart(); 
}


void onStationConnected(const WiFiEventStationModeConnected& evt) {
  const int temp_stripBrightness = 255;
  outputDebug("Connected to WiFi, IP address: ");
  outputDebugln(WiFi.localIP());
  //NEOPIXEL Indicate Reconnect (green pixels at positions 0 and 6)
  strip.clear();
  strip.setBrightness(temp_stripBrightness);
  strcpy(patternParam,"gooooogooooo");
  setRingToPattern();
  strip.show(); 
}


void onStationDisconnected(const WiFiEventStationModeDisconnected& evt) {
  const int temp_stripBrightness = 35;
  outputDebugln("WiFi Disconnected!");
  //NEOPIXEL Indicate Disconnect (yellow pixels positions 0 and 6)
  strip.clear();
  strip.setBrightness(temp_stripBrightness);
  strcpy(patternParam,"yoooooyooooo");
  setRingToPattern();
  strip.show(); 
}
