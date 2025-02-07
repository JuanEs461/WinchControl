// Change the zero to the requested value 0.1-0.9
// Also change line 1373 to the correct min value (ctrl-f and searh for minLimitLimitSet)
char minLimitLimit = '0';
// Also also, ctrl-f for minLimitLimitSet in C:\Users\Co-op Student\Documents\Arduino\libraries\HTML_Txt_1_16.cpp

byte mac[] = { 0x12, 0x56, 0x78, 0x9A, 0xBC, 0xEF };  // MAC address for each unit ***Must change every for every instalation*** (Unique identifier)

/*
          AGO PHOENIX PROJECT - ROTARY ENCODER
Winch Control first version by Kailyn Crossman August 2023
              Second version by Juan E. Ochoa July 2024

The goal of this program is to control a winch using the 
P1AM-100 Arduino Compatible CPU from Automation Direct.
The winch is controlled by either an on-board joystick or
wireless computer controls. The joystick interfaces with the 
P1AM-100 using the specific I/O Automation Direct cards listed below. 
The joystick is powered from the 10V pin on the VFD drive.
The wireless computer controls mostly use an ethernet card to control the winch
but it does also utilize the P1AM output card. 
There is also Serial input and output funcionality included in this project.
Using the P1AM serial card a LCD screen is used to display winch and
computer control status. (There may be the possiblilty of upgrading to a 
touch screen interface but that is not addressed in this program)
Also serial is utilized to input the constants required for a sheave encoder. 

----------Parts list----------
5V AC-DC Converter - Powers LCD
24V AC-DC Converter - Powers Joystick switches, HSC, and Analog card
P1AM-ETH - Ethernet Card
P1AM-SERIAL - Serial Card
P1AM-GPIO - LCD button's
P1AM-100 - CPU
P1-4ADL2DAL-2 - Analog card (4 inputs, 2 outputs, requires 24V)
P1-08TRS - Relay Output (8 Relays)
P1-08ND3 - 12-24VDC Input (2 COM, 4 inputs each)
P1-02HSC - High Speed Counter (2 Counters possible)
P1-08SIM - Test Input Card (Not required for actual product)
*/

//***********************************************************************Libraries***********************************************************************
#include <SD.h>               // For SD card
#include <Ethernet.h>         // For Computer Control (W5100.h was edited to have proper address as the P1AM uses the W5500.h internet driver)
#include <P1AM.h>             // For P1AM
#include <P1_HSC.h>           // For high speed counter
#include <P1AM_Functions.h>   // Custom for writing commands to P1AM
#include <P1AM_Serial.h>      // For serial ports
#include <RS232_Functions.h>  // Custom for writing commands to LCD
#include "utility/w5100.h"    // Used for retransmission count and timeout

//***********************************************************************Defines***********************************************************************
//P1-4ADL2DAL-2
#define ANALOG_SLOT 3
//CHANNELS
#define ANALOG_OUTPUT_CHANNEL1 2
#define ANALOG_INPUT_CHANNEL1 1
//P1-08TRS
#define OUTPUT_SLOT 1
//CHANNELS
#define STOP_START_OUTPUT_CHANNEL 1
#define OUT_OUTPUT_CHANNEL 2
#define IN_OUTPUT_CHANNEL 3
#define LIMIT_LIGHT_OUTPUT_CHANNEL 4
//P1-08ND3
#define INPUT_SLOT 2
//CHANNELS
#define OUT_INPUT_CHANNEL 1
#define IN_INPUT_CHANNEL 2
#define ESTOP_INPUT_CHANNEL 3
#define MANUAL_START_INPUT_CHANNEL 4
#define STATE_INPUT_CHANNEL 5
#define LIMITSWITCH_INPUT_CHANNEL 6
//TEST(P1-08SIM)
//#define TEST_INPUT_SLOT 5
//CHANNELS
//#define TEST_STATE_INPUT_CHANNEL 1
//#define TEST_ESTOP_INPUT_CHANNEL 2
//#define TEST_MANUAL_START_INPUT_CHANNEL 3
//#define TEST_LIMITSWITCH_INPUT_CHANNEL 4

// LCD Buttons
#define rockerUpPin 4
#define rockerDownPin 11
#define SelectButton A2
#define BackButton 12
#define MenuButton A5
#define PowerFailure A1

//******************************************************************Function Prototypes***********************************************************************
void GetAjaxData(EthernetClient cl, int Analog_Slot, int Analog_Input_Channel);
float GetCablePayout();
void AgoSelect();
void ClientSelect();
float SetCableDiameter();
float SetCableLength();
float SetDrumDiameter();
float SetDrumWidth();
float SetScaleFactor();
float SetStretchFactor();
int SetEncoderCount();
float SetMaxPayout();
float SetMinPayout();
bool SD_Begin();
int readFileInt();
float readFileFloat();
void MainMenu();
void Rotate();
void DrawCursor();
void SelectPressed();
void LCDToggleOffset(int menuReturn);
void LCDEthernetInfo(int menuReturn);
void LCDCableSettings(int menuReturn);
void LCDDrumSettings(int menuReturn);
void LCDSetUnits(int menuReturn);
float LCDSetDrumDiameter(int row);
float LCDSetDrumWidth(int row);
float LCDSetDrumEncRes(int row);
float LCDSetCableLength(int row);
float LCDSetCableDiameter(int row);
float LCDSetMaxLimit(int row);
float LCDSetMinLimit(int row);
float LCDSetScaleFactor(int row);
float LCDSetStretchFactor(int row);
float LCDSetNMEAPeriod(int row);
/**
*@brief print parameters with units
*@param value Value to print that is in m or ft
*@param row Hexidecimal value corresponding to the LCD row
*/
void LCDPrintUnitFloat(float value, int row);

/**
*@brief print drum values
*@param value Value of 00.000 to print that is unitless
*@param row Hexidecimal value corresponding to the LCD row
*/
void LCDPrintUnitFloatCentimeters(float value, int row);

/**
*@brief print factor values
*@param value Value of 0.0000 to print that is unitless
*@param row Hexidecimal value corresponding to the LCD row
*/
void LCDPrintUnitlessFloat(float value, int row);

/**
*@brief print encoder count
*@param value Value of 0000 to print that is in m or ft
*@param row Hexidecimal value corresponding to the LCD row
*/
void LCDPrintUnitlessInt(int value, int row);

/**
*@brief print NMEA string
*@param value Value of 0.0000 to print in s
*@param row Hexidecimal value corresponding to the LCD row
*/
void LCDPrintSeconds(float value, int row);

//*********************************************************************Initialize Modules*********************************************************************
P1_HSC_Module HSC(4);  // Create HSC class object for slot 4. It also automatically creates 2 P1_HSC_CHANNEL objects for this slot

EthernetServer server(80);     // Server is at http port 80
EthernetServer serverTCP(23);  // Another server at telnet port 23

bool alreadyConnected = false;
//*********************************************************************Global Variables*********************************************************************
// Buttons
bool previousManualState = false;
bool manualState = false;
bool previousManualSwitch = false;
bool manualSwitch = false;
bool prevStartPress = false;
bool startPress = false;
bool eStopState = false;
bool limitSwitch = true;

// Winch States
bool stopState = true;
bool startState = false;
bool outState = false;
bool inState = false;

// Error States
bool errorState = false;
bool virtualJoyErr = false;
bool ethernetStateChange = true;
bool minPayErr = false;
bool maxPayErr = false;
bool inStatePayErr = false;
bool outStatePayErr = false;
bool errorHardware = false;
bool errorCable = false;
bool errorAssignment = false;

// Internet response string
String HTTP_req;

// External readings
int speed = 0;
int browserSpeed = 0;

// Internet
int currentClientIP = 0, allowedClientIP = 0;
unsigned int previousServerIP = 0;
unsigned int serverIP = 1;
bool clientConnected = false;
bool timeout = false;
bool DHCPError = false;
IPAddress error0IP(0, 0, 0, 0);
IPAddress error1IP(255, 255, 255, 255);
bool updateLimits = true;

// Timers
unsigned long currentMillis = 0, cableTimer = 0, computerTimer = 0, NMEATimer = 0;
const unsigned long computerTimeout = 1000;
const unsigned long payoutTimeout = 1000;
unsigned long NMEAPeriod = 1000;

// Encoder (Values are defaults for if there is no SD card inserted)
float cableDiameter = 0.00818;
float cableLength = 0;
float drumDiameter = 0.305;
float drumWidth = 0.324;
float scaleFactor = 1;
float stretchFactor = 1;
int drumEncRes = 1024;
float offset = 0;
float trueDiameter;
int encoderCount = 0;
int encoderCurrentCount;
int encoderPreviousCount;
int retainCount;
int drumCountValue;
int drumTurns;
int drumTurnsPerLayer;
float partialDrumTurn;
float calcCablePayout = 0;
float cablePayout = 0;
float cableSpeed = 0;
float previousCablePayout = 0;
bool drumCountNegative, drumDirectionNegative, resetEncoder, resetQuad1;
float maxCablePayedOut = 5000;  // Default min and max values that are set until the user reprograms them
float minCablePayedOut = 0;

// Serial Menu
byte count = 0;

// LCD Menu flags
int previousMenuPosition = 0;
int menuPosition = 0;
bool ESC = false;
bool updateMenu = false;
bool refreshLCD = false;
bool offsetActive = false;
float tempVal = 0;

// LCD Encoder
int previousPressUp = LOW;    // Previous state for rockerUpPin
int previousPressDown = LOW;  // Previous state for rockerDownPin

// Menu Buttons (Interrupts)
bool encoderCW = false;   // Clockwise rotation flag
bool encoderCCW = false;  // Counterclockwise rotation flag
bool selectPressed = false;

// Menu Buttons (no interrupts)
bool backPress;
bool previousBackPress;

// Units
bool metric = true;
bool imperial = false;
bool seconds = true;
bool minutes = false;

// SD Card
File drumFile;
bool sdCard;
bool wsdCard;  // Wireless sd card file check
const int chipSelect = SDCARD_SS_PIN;

//NMEA
String NMEAString = "";
String StringcablePayout = "";
String StringcableSpeed = "";
int checksum = 0;

unsigned long lastDebounceTimeUp = 0;
unsigned long lastDebounceTimeDown = 0;
const unsigned long debounceDelay = 50;  // 50ms debounce delay

//*********************************************************************Setup (Run Once)*********************************************************************
void setup() {

  // Setup Port1 and 2 of the Serial card for RS232 communication and at 9600 buad
  PORT1_RS232_BEGIN(9600);  // LCD Communication
  PORT2_RS232_BEGIN(9600);  // Serial Port Communication

  HTTP_req.reserve(600);   // Reserve 600 bytes in the heap for Client responses
  NMEAString.reserve(50);  // Reserve 50 bytes in the heap for the String to reduce memory fragmentation.
  StringcablePayout.reserve(10);
  StringcableSpeed.reserve(10);

  while (!P1.init())
    ;  // Wait for Modules to Sign on

  blinkingCursorOFF();  // Turn off the blinking cursor incase it was on during a power failure
  clearScreen();        // Clear the LCD screen
  setBrightness(8);     // Set the brightness of the display, fixes LCD screen flashing
  Serial1.print(" AGO PHOENIX WINCH  ");
  setCursor(0x54);
  Serial1.print("booting...");
  delay(1000);

  // Initialize LCD Menu Buttons
  pinMode(PowerFailure, INPUT);          // PowerFailure
  pinMode(rockerUpPin, INPUT_PULLUP);    // rockerUpPin
  pinMode(rockerDownPin, INPUT_PULLUP);  // rockerDownPin
  pinMode(SelectButton, INPUT_PULLUP);   // SelectButton
  pinMode(BackButton, INPUT_PULLUP);     // BackButton
  pinMode(MenuButton, INPUT_PULLUP);     // MenuButton
  pinMode(30, INPUT);                    // SD card chip detect pin

  // Prepare previous state values for the first run through
  previousPressUp = digitalRead(rockerUpPin);      // Initial state of rockerUpPin
  previousPressDown = digitalRead(rockerDownPin);  // Initial state of rockerDownPin
  backPress = !digitalRead(BackButton);            // Invert value for active low

  // Set Interrupts for the rocker switch and select button
  attachInterrupt(digitalPinToInterrupt(rockerUpPin), Rotate, CHANGE);           // Interrupt for rockerUpPin
  attachInterrupt(digitalPinToInterrupt(rockerDownPin), Rotate, CHANGE);         // Interrupt for rockerDownPin
  attachInterrupt(digitalPinToInterrupt(SelectButton), SelectPressed, FALLING);  // Interrupt for SelectButton
  attachInterrupt(digitalPinToInterrupt(PowerFailure), PowerLoss, FALLING);      // Interrupt for PowerFailure


  clearScreen();  // Clear the LCD screen

  //Serial.begin(115200);  // Initialize serial communication at 115200 bits per second
  // Only required for debug

  //while (!Serial)
  //Wait for Serial Port to be opened

  // Initialize the SD card
  if (!SD_Begin()) {
    delay(2000);
    sdCard = false;
    wsdCard = true;  // Don't overwrite the SD! error with WSD!. SD! is the more prominent error
  } else {
    sdCard = true;
    wsdCard = true;
    // Check if wireless files are on the SD card
    if (!SD.exists("CONTROL.HTM")) {
      wsdCard = false;
    } else if (!SD.exists("MONITOR.HTM")) {
      wsdCard = false;
    }
    // Check if the drum constant file is on the sd card
    if (!SD.exists("DRMCONST.TXT")) {  // If the drum constant file doesn't exist create it so the user can store values into it
      drumFile = SD.open("DRMCONST.TXT", FILE_WRITE);
      drumFile.print("retainCount:0,              drumDirectionNegative:F,              maxCablePayedOut:5000.0,           minCablePayedOut:0.0,              cableDiameter:0.00818,              cableLength:0.0,              drumDiameter:0.305,              drumWidth:0.324,              scaleFactor:1.0,              stretchFactor:1.0,              drumEncRes:1024,              offset:0.0,              units:Ms,              NMEAPeriod:1.0,            ");
      drumFile.close();
    }
    // Load the stored values into memory
    drumFile = SD.open("drmConst.TXT", (O_READ));
    drumFile.find("retainCount:");
    retainCount = readFileInt();
    drumFile.find("drumDirectionNegative:");
    drumDirectionNegative = readFileBool();
    drumFile.find("maxCablePayedOut:");
    maxCablePayedOut = readFileFloat();
    drumFile.find("minCablePayedOut:");
    minCablePayedOut = readFileFloat();
    drumFile.find("cableDiameter:");
    cableDiameter = readFileFloat();
    drumFile.find("cableLength:");
    cableLength = readFileFloat();
    drumFile.find("drumDiameter:");
    drumDiameter = readFileFloat();
    drumFile.find("drumWidth:");
    drumWidth = readFileFloat();
    drumFile.find("scaleFactor:");
    scaleFactor = readFileFloat();
    drumFile.find("stretchFactor:");
    stretchFactor = readFileFloat();
    drumFile.find("drumEncRes:");
    drumEncRes = readFileInt();
    drumFile.find("offset:");
    offset = readFileFloat();
    drumFile.find("units:");
    setUnitsFromFile();
    drumFile.find("NMEAPeriod:");
    tempVal = readFileFloat();
    NMEAPeriod = tempVal * 1000;
    drumFile.close();
  }

  // Check if the offset was active from the file
  if (offset != 0) {
    offsetActive = true;
  }

  // Start timers for computer and cable speed timeout functions
  computerTimer = millis();
  cableTimer = millis();

  manualState = HIGH;                                               //P1.readDiscrete(INPUT_SLOT, STATE_INPUT_CHANNEL);   // Read the manual or computer switch to determine which state to start in
  previousManualState = manualState;                                // Preparing for which state to start the program in
  manualSwitch = P1.readDiscrete(INPUT_SLOT, STATE_INPUT_CHANNEL);  // Read the switch to determine which position it is to ensure proper reloads occur
  if (!manualState) {                                               // If the winch is starting in computer control initialize ethernet connection
    ethernetStateChange = false;
    setCursor(0x14);
    Serial1.print("Waiting for ethernet");
    setCursor(0x54);
    Serial1.print("connection");
    if (Ethernet.begin(mac) == 0) {                           // Set mac address and assign IP with DHCP *******DHCP significantly increases the size of the sketch*******
      if (Ethernet.hardwareStatus() == EthernetNoHardware) {  // If there is an error with the P1AM Ethernet card
        clearScreen();
        delay(70);
        Serial1.print("Ethernet shield was not found.");
        errorHardware = true;
      } else if (Ethernet.linkStatus() == LinkOFF) {  // If there is an error with the Ethernet cable
        clearScreen();
        delay(70);
        Serial1.print("Ethernet cable is   not connected.");
        errorCable = true;
      } else {
        clearScreen();
        delay(70);
        Serial1.print("Automatic IP failed");
        errorAssignment = true;
      }
      while (errorHardware || errorCable || errorAssignment) {            // Wait for a fix                                                         //wait for a fix
        manualSwitch = P1.readDiscrete(INPUT_SLOT, STATE_INPUT_CHANNEL);  // Switching to manual mode with fix connection error
        manualState = manualSwitch;
        if (manualSwitch) {
          errorAssignment = false;
          errorHardware = false;
          errorCable = false;
        }
        if (Ethernet.linkStatus() == LinkON) {  //if ethernet signs on then continue
          errorCable = false;
        }
        if (Ethernet.hardwareStatus() != EthernetNoHardware) {
          errorHardware = false;
        }
      }
    } else {
      // Set how many times and for how long the program will try for an ethernet connection, with current settings will timeout after the ethernet cable is disconnected for 2 seconds
      Ethernet.setRetransmissionCount(3);
      Ethernet.setRetransmissionTimeout(100);
    }
    previousServerIP = serverIP;
    serverIP = Ethernet.localIP();
    if (previousServerIP != serverIP) {
      server.begin();  // Start the html server, IP is assigned with DHCP see ****DHCP line 156****
      serverTCP.begin();
    }
  }
  /*
  Comments in the lines below shows other settings
  -isRotary - If true configures channel as a rotary encoder. 
			  This means it will roll over at the rollover position.
  -enableZReset - If true, resets current position when channel's Z input is high.
  -inhibitOn - When the selected input is high, counting is inhibited. "false" 
			   disables this feature.
  -mode - Select counting mode: Step and Direction, Quadrature 4X, and Quadrature 1X.
  -polarity - Sets directional polarity of counting.
  
  */
  HSC.CNT1.isRotary = false;
  HSC.CNT1.enableZReset = false;
  HSC.CNT1.inhibitOn = false;             // oneZ, threeIn, twoZ, fourIn
  HSC.CNT1.mode = quad1x;                 // quad4x, quad1x
  HSC.CNT1.polarity = positiveDirection;  // negativeDirection
  HSC.configureChannels();                // Load settings into HSC module. Leave argument empty to use default CNT1 and CNT2
  HSC.CNT1.setPosition(0);                // Initialize positions

  encoderPreviousCount = 0;   // Prepares encoder readings
  analogWriteResolution(12);  // Set Dac resolution
}

//*******************************************************************Main Loop(Runs continuously)*******************************************************************
void loop() {
  // Update the current run time
  currentMillis = millis();

  // Read the active low SD card chip detect pin and if the sd card is not seen then throw an error
  if (digitalRead(30)) {
    sdCard = false;
  }

  cablePayout = GetCablePayout();

  if ((currentMillis - cableTimer) > payoutTimeout) {     // If the payout speed timer has expired then calculate speed (1 second timer)
    cableSpeed = abs(cablePayout - previousCablePayout);  // Calculate how far the cable has gone since the last reading 1 second ago
    previousCablePayout = cablePayout;                    // Store the current payout reading
    cableTimer = currentMillis;                           // Update the timer to the current time

    // Equation is (current meters - previous meters)/1 second = m/s
  }

  // Read the Estop switch
  if (!P1.readDiscrete(INPUT_SLOT, ESTOP_INPUT_CHANNEL)) {  // If active low switch is pressed
    eStopState = true;
    stopState = true;
    startState = false;
    inState = false;
    outState = false;
    speed = 0;
  } else if (P1.readDiscrete(INPUT_SLOT, ESTOP_INPUT_CHANNEL)) {  // If released
    eStopState = false;
  }

  // Reading the manual switch
  previousManualSwitch = manualSwitch;
  manualSwitch = P1.readDiscrete(INPUT_SLOT, STATE_INPUT_CHANNEL);
  if (manualSwitch != previousManualSwitch) {  // Checking if the manual switch has been toggled
    previousManualState = manualState;
    manualState = !manualState;  // toggle the state
    ethernetStateChange = true;  // flag to tell PLC to check ethernet connection
  }
  if (serverIP != 0) {              //If a server connection has been made
    switch (Ethernet.maintain()) {  // Re-requests a DHCP lease when needed
      case 1:
        DHCPError = true;
        break;
      case 2:
        break;
      case 3:
        DHCPError = true;
        break;
      case 4:
        break;
      default:
        break;
    }
  }
  // Checking ethernet connection when entering computer mode from manual mode
  // If the winch starts in manual mode it is required to switch to computer mode then back to manual mode to connect to the internet for manual monitoring
  if (!manualState && ethernetStateChange) {
    clearScreen();
    setCursor(0x00);
    // Turn off the winch while checking connection
    stopSignal(OUTPUT_SLOT, STOP_START_OUTPUT_CHANNEL);
    clearOutIn(OUTPUT_SLOT, OUT_OUTPUT_CHANNEL, IN_OUTPUT_CHANNEL);
    P1.writeAnalog(0, ANALOG_SLOT, ANALOG_OUTPUT_CHANNEL1);
    stopState = true;
    startState = false;
    delay(50);
    Serial1.print("Waiting for ethernetconnection");
    //error check ethernet connection, same error checking that occurs at setup
    //Ethernet.begin(mac);
    if (Ethernet.begin(mac) == 0) {
      if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        clearScreen();
        delay(70);
        Serial1.print("Ethernet shield was not found.");
        errorHardware = true;
      } else if (Ethernet.linkStatus() == LinkOFF) {
        clearScreen();
        delay(70);
        Serial1.print("Ethernet cable is   not connected.");
        errorCable = true;
      } else {
        clearScreen();
        setCursor(0x00);
        delay(70);
        Serial1.print("Automatic IP failed");
        errorAssignment = true;
      }
      while (errorHardware || errorCable || errorAssignment) {  //wait for a fix
        if (Ethernet.hardwareStatus() != EthernetNoHardware) {
          errorHardware = false;
        }

        if (Ethernet.linkStatus() == LinkON) {
          errorCable = false;
        }

        manualSwitch = P1.readDiscrete(INPUT_SLOT, STATE_INPUT_CHANNEL);
        manualState = manualSwitch;
        if (manualSwitch) {
          errorHardware = false;
          errorCable = false;
          errorAssignment = false;
        }
      }
    } else {
      Ethernet.setRetransmissionCount(3);
      Ethernet.setRetransmissionTimeout(100);
    }
    previousServerIP = serverIP;
    serverIP = Ethernet.localIP();
    if (previousServerIP != serverIP) {
      server.begin();  //start the html server
      serverTCP.begin();
    }
    ethernetStateChange = false;
  } else if (manualState && ethernetStateChange) {  // If the ethernet connection is lost while in computer control and manual mode is entered 0.0.0.0 will be displayed
    previousServerIP = serverIP;
    serverIP = Ethernet.localIP();
    if (previousServerIP != serverIP) {
      server.begin();  //start the html server
      serverTCP.begin();
    }
    ethernetStateChange = false;
  }

  EthernetClient client = server.available();  //listen for incoming clients
  EthernetClient clientTCP = serverTCP.available();

  if (client) {                           //if a client is found then
    currentClientIP = client.remoteIP();  //get the current user's IP
    if (!clientConnected) {               //if there was no more previous clients then give the first client control and don't allow anyone else to connect
      allowedClientIP = currentClientIP;
      clientConnected = true;
    }
    bool currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available() && currentClientIP == allowedClientIP) {  //if there is data from the client and the IP is correct
        computerTimer = currentMillis;                                 //update the computerTimer so ethernetTimeout doesnt get called
        char c = client.read();
        HTTP_req += c;  //store the data from the client
        // The client is done sending data once it sends a newline chacter
        //this means it is ready to receive data
        if (c == '\n' && currentLineIsBlank) {
          //Serial.println(HTTP_req);
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println();
          // All the HTTP_req.indexOf("") statements are reading requests sent from the client to the server using AJAX techniques
          // Most come from the buttons and slider but some also happen periodically set by timeouts on the webpage
          if (HTTP_req.indexOf("/reload_request") >= 0) {  // A request that is sent from the client to the server
            if (timeout == true) {                         // If the computerTimeout has happened and the client reconnects 0 all controls to correspond with PLC outputs all turning off
              client.print("timeout|");
              if (minPayErr) {
                client.print("minErr");
              } else if (maxPayErr) {
                client.print("maxErr");
              } else {
                client.print("stopped");
              }
              client.stop();
              timeout = false;
            }
            if (manualState != previousManualState) {  // If the state has been toggled
              previousManualState = manualState;
              // Clear all outputs
              stopState = true;
              startState = false;
              inState = false;
              outState = false;
              speed = 0;
              // Reload the webpage to reflect the current state that the winch is in
              client.print("Reload");
              client.stop();
            }
          }
          if (manualState == false) {  // If the winch is in computer control
            if (HTTP_req.indexOf("/stopButtonPressed") >= 0) {
              stopState = true;
              startState = false;
              inState = false;
              outState = false;
              speed = 0;
              client.println("Stopped");
            } else if (HTTP_req.indexOf("GET / HTTP/1.1") >= 0) {  // Prints HTML webpage
              if (sdCard && wsdCard) {
                drumFile = SD.open("control.htm", (O_READ));
                if (drumFile) {
                  while (drumFile.available()) {
                    client.write(drumFile.read());
                  }
                  drumFile.close();
                }
              } else {
                client.println("<!DOCTYPE html>");
                client.println("<html>");
                client.println("<body style=\"background-color:powderblue\">");
                client.println("<h1>SD card has failed. Check that a SD card is in the PLC and that the Card is not corrupted.</h1>");
                client.println("</body>");
                client.println("</html>");
                client.stop();
              }
              updateLimits = true;
            } else if (HTTP_req.indexOf("/output_ajax_switch") >= 0) {  // Periodic call for estop and sheave values
              if (eStopState) {
                client.print("eStop pressed|");  // Client handles setting page values to zero when it receives "eStop pressed" the "|" is used to make an array on the client
                // Check HTMLGetOutputData Myarray = received.split
              } else {
                client.print("eStop released|");
              }
              // The amount of cablePayedOut and speed are always sent to the control page
              if (offsetActive) {
                tempVal = cablePayout - offset;
              } else {
                tempVal = cablePayout;
              }
              if (imperial) {
                tempVal = tempVal * 3.28084;
                client.print(tempVal, 1);
                client.print("ft");
                client.print("|");
                if (seconds) {
                  tempVal = cableSpeed * 3.28084;
                  client.print(tempVal, 2);
                  client.print("ft/s");
                } else if (minutes) {
                  tempVal = cableSpeed * 3.28084 * 60;
                  client.print(tempVal, 1);
                  client.print("ft/m");
                }
                client.print("|");
              } else {
                client.print(tempVal, 1);
                client.print("m");
                client.print("|");
                if (seconds) {
                  client.print(cableSpeed, 2);
                  client.print("m/s");
                } else if (minutes) {
                  tempVal = cableSpeed * 60;
                  client.print(tempVal, 1);
                  client.print("m/m");
                }
                client.print("|");
              }
              if (minPayErr) {     // If the cable has reached its minimum payout value an error message is displayed
                if (startState) {  // Pressing the start button is the first step of clearing the error
                  if (outState) {  // The winch needs to run out to increase the payout value and clear the minimum payout error
                    minPayErr = false;
                  } else if (inStatePayErr || inState) {  // The winch is not allowed to run in
                    // If the winch is currently running in and the minErr is thrown inState will be true and setting stopState to true stops the winch
                    // inStatePayErr is set when the slider is set to In while minPayErr is true
                    minPayErr = true;
                    stopState = true;
                    startState = false;
                    inStatePayErr = false;
                  }
                  client.print("|");
                } else {                       // (startState else) will run if minPayErr isn't on it's first cycle or if the slider is zeroed
                  client.print("minPayErr|");  // Prints message to control page
                  // Turns winch off
                  stopState = true;
                  startState = false;
                  inState = false;
                  outState = false;
                  speed = 0;
                }
              } else if (maxPayErr) {  // If the cable has reached its maximum payout value
                // same logic as minPayErr
                if (startState) {
                  if (inState) {
                    maxPayErr = false;
                  } else if (outStatePayErr || outState) {
                    maxPayErr = true;
                    stopState = true;
                    startState = false;
                    outStatePayErr = false;
                  }
                  client.print("|");
                } else {
                  client.print("maxPayErr|");
                  stopState = true;
                  startState = false;
                  inState = false;
                  outState = false;
                  speed = 0;
                }
              } else {
                client.print("|");
              }
              if (updateLimits) {
                client.print("updateLimits|");
                updateLimits = false;
              } else {
                client.print("|");
              }

              if (imperial) {
                tempVal = minCablePayedOut * 3.28084;
                client.print(tempVal, 1);
                client.print("|");
                tempVal = maxCablePayedOut * 3.28084;
                client.print(tempVal, 1);
                client.print("|");
                client.print("(ft)|");
              } else {
                client.print(minCablePayedOut, 1);
                client.print("|");
                client.print(maxCablePayedOut, 1);
                client.print("|");
                client.print("(m)|");
              }
              if (limitSwitch) {
                client.print("Limits ON|");
                client.print("button buttonRed toggleSwitches|");
              } else {
                client.print("Limits OFF|");
                client.print("button buttonGreen toggleSwitches|");
              }
              if (offsetActive) {
                client.print("Offset ON|");
                client.print("button buttonRed toggleSwitches|");
              } else {
                client.print("Offset OFF|");
                client.print("button buttonGreen toggleSwitches|");
              }
              client.stop();
            } else if (HTTP_req.indexOf("/startButtonPressed") >= 0) {
              if (eStopState) {  // If eStopState has been set in output_ajax call then if the operator tries to start remind them that the estop is pressed
                client.print("E-STOP PRESSED!");
              } else {
                if (speed > 0 && stopState) {  // If speed is set on the slider while the winch is stopped don't allow the winch to start until the speed value is zeroed.
                  // This if statement is slightly redudant because in the webpage javascript speed values shouldn't make it to the server if the winch is stopped
                  // This is mostly still here for edge cases as it has appeared unexpectedly while testing
                  client.println("Speed is set, please press stop to clear");
                } else if ((inState || outState) && stopState) {  // This is here for the same reason as the if statement above.
                  // I did use both of these at the begining of the project but they have become saftey redudancy
                  client.println("Direction is set, please press stop to clear");
                } else if ((inState || outState) && startState) {  // If the winch is already runing don't effect the values
                  client.print("Running ");
                } else {  // If the winch status has passed all the above checks go ahead with the steps to begin running
                  startState = true;
                  stopState = false;
                  client.println("Ready to run ");
                }
              }
              client.stop();
            } else if (HTTP_req.indexOf("/offsetTogglePressed") >= 0) {
              offsetActive = !offsetActive;
              if (offsetActive) {
                offset = cablePayout;
                client.print("Offset ON|");
                client.print("button buttonRed toggleSwitches");
              } else {
                offset = 0;
                client.print("Offset OFF|");
                client.print("button buttonGreen toggleSwitches");
              }
              client.stop();
            } else if (HTTP_req.indexOf("/limitTogglePressed") >= 0) {
              limitSwitch = !limitSwitch;
              if (limitSwitch) {
                client.print("Limits ON|");
                client.print("button buttonRed toggleSwitches");
              } else {
                client.print("Limits OFF|");
                client.print("button buttonGreen toggleSwitches");
              }
              client.stop();
            } else if (HTTP_req.indexOf("/minLimitSet=") >= 0) {
              tempVal = (HTTP_req.substring(17, 23)).toFloat();
              if (imperial) {
                // Convert Metric to Imperial
                minCablePayedOut = tempVal * 0.3048;
              } else {
                minCablePayedOut = tempVal;
              }
              if (sdCard) {
                drumFile = SD.open("DRMCONST.TXT", (O_READ | O_WRITE));
                drumFile.find("minCablePayedOut:");
                drumFile.print(minCablePayedOut, 1);
                drumFile.print(',');
                drumFile.close();
              }
            } else if (HTTP_req.indexOf("/maxLimitSet=") >= 0) {
              tempVal = (HTTP_req.substring(17, 23)).toFloat();
              if (imperial) {
                // Convert Metric to Imperial
                maxCablePayedOut = tempVal * 0.3048;
              } else {
                maxCablePayedOut = tempVal;
              }
              if (sdCard) {
                drumFile = SD.open("DRMCONST.TXT", (O_READ | O_WRITE));
                drumFile.find("maxCablePayedOut:");
                drumFile.print(maxCablePayedOut, 1);
                drumFile.print(',');
                drumFile.close();
              }
            } else if ((HTTP_req.indexOf("/speedSlider=") >= 0)) {  // "Computer Joystick" request
              browserSpeed = (HTTP_req.substring(17, 21)).toInt();  // The request actually looks like GET /speedSlider= ### so the substring reads characters 18 to 21
              // .toInt ensure that browserSpeed is a number and removes trailing letters when the joy values is one(#) or two(##) numbers
              if ((-20 <= browserSpeed) && (browserSpeed <= 20)) {  // Stopped in center, creates the deadzone between OUT and IN on the control page
                browserSpeed = 0;
                outState = false;
                inState = false;
                virtualJoyErr = false;
              } else if (browserSpeed < -20 && !inState && !maxPayErr) {  // Running Out
                browserSpeed += 20;                                       // Adjusts for deadzone
                browserSpeed *= -1;                                       // Adjusts because we are in the negative values of the slider for out
                outState = true;
                inState = false;
                minPayErr = false;
              } else if (browserSpeed > 20 && !outState && !minPayErr) {  // Running In
                browserSpeed -= 20;
                // No need to flip value because we are in the positive area of the slider
                inState = true;
                outState = false;
                maxPayErr = false;
              } else {
                if (minPayErr && startState) {  // Gets called when the start button is pressed and the slider is set to run In.
                  // It doesn't get called when the slider is set to run out because the "Running Out" if statement is true before this one
                  // It also doesn't get called when the slider is in the middle because the top (stoppod in center ) if statement is true
                  // This means that startState can be active for minPayErr logic above
                  browserSpeed = 0;
                  inState = false;
                  stopState = true;
                  inStatePayErr = true;                // Sets the inStatePayErr which turns everything off in minPayErr logic
                } else if (maxPayErr && startState) {  // Gets called when the start button is pressed and the slider is set to run Out
                  // It doesn't run when it seems like it could because of the same reasons in the if above
                  browserSpeed = 0;
                  outState = false;
                  stopState = true;
                  outStatePayErr = true;  // Sets the outStatePayErr which turns everything off in maxPayErr logic
                } else {
                  // If the joystick value is changed to quickly (ie. the user is in InState and then clicks on the slider setting it to OutState)
                  // This direction change is to fast for the VFD
                  browserSpeed = 0;
                  outState = false;
                  inState = false;
                  virtualJoyErr = true;
                }
              }
              speed = map(browserSpeed, 0, 100, 0, 4095);  //converts the 0-100 slider values to 12bit values for the 12bit DAC
              if (startState && !virtualJoyErr) {          // If everything looks okay
                if (!outState && !inState) {               // Message when no speed is selected but the winch is ready to run
                  client.print("Ready to run||");
                  client.stop();
                } else if (outState) {  // Running out
                  client.print("Running |Out at |");
                } else if (inState) {  // Running In
                  client.print("Running |In at |");
                }
                client.print(browserSpeed);  // Print the % of the slider value
                client.print("%");
                client.stop();
              } else if (minPayErr) {  // Err set in the slider value if statement above
                client.print("minErr||");
                client.stop();
              } else if (maxPayErr) {
                client.print("maxErr||");
                client.stop();
              } else if (virtualJoyErr) {  // If the error has been set in the slider value if statement
                client.print("Cant switch motor speed that quickly |Please return joystick to center|");
                client.stop();
              } else {
                client.println("Stopped||");
                client.stop();
              }
            } else if (HTTP_req.indexOf("/beforeClose") >= 0) {  // Is called when the webpage is about to close ('x' button pressed on browser, page reload called, etc...)
              stopState = true;
              startState = false;
              inState = false;
              outState = false;
              speed = 0;
              clientConnected = false;  // Allow new clients to connect
            } else if (HTTP_req.indexOf("/ajax_switch") >= 0) {
              client.print("Stopped|box Red||box Clear||box Clear|0.00|0.00||box Clear||box Clear");  // Catch for when the mode is switched
              client.stop();
            }
          } else if (manualState == true) {                 // Manual State is selected from winch toggle
            if (HTTP_req.indexOf("GET / HTTP/1.1") >= 0) {  // HTTP request for web page
              if (sdCard && wsdCard) {
                drumFile = SD.open("monitor.htm");
                if (drumFile) {
                  while (drumFile.available()) {
                    client.write(drumFile.read());
                  }
                  drumFile.close();
                }
              } else {
                client.println("<!DOCTYPE html>");
                client.println("<html>");
                client.println("<body style=\"background-color:powderblue\">");
                client.println("<h1>SD card has failed. Check that a SD card is in the PLC and that the Card is not corrupted.</h1>");
                client.println("</body>");
                client.println("</html>");
                client.stop();
              }
            } else if (HTTP_req.indexOf("/ajax_switch") >= 0) {  // Gets called every 300ms
              // Read joystick switches and analog data and prints to the html webpage
              GetAjaxData(client, ANALOG_SLOT, ANALOG_INPUT_CHANNEL1);
            } else if (HTTP_req.indexOf("/stopButtonPressed") >= 0) {  //if the server has received "" from the client then do stuff
              client.println("Please Reload, Manual mode is active");
            } else if (HTTP_req.indexOf("/startButtonPressed") >= 0) {
              client.println("Please Reload, Manual mode is active");
            } else if (HTTP_req.indexOf("/speedSlider=") >= 0) {  // Catches for when mode switches from wireless control to manual
              client.println("Stopped| | ");
            } else if (HTTP_req.indexOf("/output_ajax_switch") >= 0) {
              client.print("|0.00m|0.00m/m||||||Limits OFF|button buttonGreen toggleSwitches|Limits OFF|button buttonGreen toggleSwitches");
            } else if (HTTP_req.indexOf("/limitTogglePressed") >= 0) {
              client.print("Please Reload, Manual mode is active|");
            }
          }
          //Serial.println("escapa the packaka");
          HTTP_req = "";  // Clear HTTP_req for next incoming request
          break;
        }
        if (c == '\n') {  // Error checking for complete request messages
          currentLineIsBlank = true;
        } else if (c != '\r') {
          currentLineIsBlank = false;
        }
      } else {  // Handles if a second user tries to connect to the control page
        client.println("<!DOCTYPE html>");
        client.println("<html>");
        client.println("<body style=\"background-color:powderblue\">");
        client.println("<h1>A control session is already open on another computer</h1>");
        client.println("</body>");
        client.println("</html>");
        client.stop();
      }
    }
    //Serial.println("client stop");
    delay(1);
    client.stop();
  }

  //Check for the clients handshake, if it's not received in time, timeout
  if (((currentMillis - computerTimer) >= computerTimeout) && clientConnected) {
    clientConnected = false;     // "Disconnect" the current user and let a new user connect
    if (manualState == false) {  // If in wireless control mode then clear everything, If in manual control mode a user disconnecting wouldn't have any effect so dont do anything
      timeout = true;
      stopState = true;
      startState = false;
      inState = false;
      outState = false;
      speed = 0;
    }
  }
  //???

  if (manualState == true) {
    //Serial1.println("manual state");
    outState = P1.readDiscrete(INPUT_SLOT, OUT_INPUT_CHANNEL);
    inState = P1.readDiscrete(INPUT_SLOT, IN_INPUT_CHANNEL);
    speed = readJoystick(ANALOG_SLOT, ANALOG_INPUT_CHANNEL1);
    limitSwitch = P1.readDiscrete(INPUT_SLOT, LIMITSWITCH_INPUT_CHANNEL);  // Check the limit switch to see if the operator wants limits set
    if (!P1.readDiscrete(INPUT_SLOT, ESTOP_INPUT_CHANNEL)) {
      stopState = true;
      startState = false;
    }
    if (startState == false) {
      if (outState || inState) {  // If any inputs are true while start isn't active then don't allow the winch to start up
        errorState = true;
      } else {
        errorState = false;
      }
    }

    if (manualState == true) {
      startState = true;  // Automatically set startState to true when in manual mode
      stopState = false;  // Ensure the stop state is turned off
    }

    /* ***********ADD MANUAL START BUTTON************
    prevStartPress = startPress;
    startPress = P1.readDiscrete(INPUT_SLOT, MANUAL_START_INPUT_CHANNEL);
    if (startPress && !prevStartPress) {  // Check the press of the momentary switch
      if (!startState) {// && !errorState) {   // Only allow the winch to start if it's off and an error hasn't been thrown from above
        startState = true;
        stopState = false;
      }
    }
    */
  } else if (manualState == false) {
    errorState = false;
    if (!clientConnected) {
      stopState = true;
      startState = false;
    }
  }

  // If the limit switch is set then check for limits
  if (limitSwitch) {  // If the limit switch is set then check for limits
    if (cablePayout <= minCablePayedOut) {
      if (outState) {  // If at MINIMUM payout then we need to run OUT to INCREASE payout
        minPayErr = false;
      } else {
        minPayErr = true;
      }
    } else {
      minPayErr = false;
    }
    if (cablePayout >= maxCablePayedOut) {
      if (inState) {  // If at MAXIMUM payout then we need to run IN to DECREASE payout
        maxPayErr = false;
      } else {
        maxPayErr = true;
      }
    } else {
      maxPayErr = false;
    }
  } else {
    maxPayErr = false;
    minPayErr = false;
  }
  // Toggle the limit light
  if (maxPayErr || minPayErr) {
    P1.writeDiscrete(HIGH, OUTPUT_SLOT, LIMIT_LIGHT_OUTPUT_CHANNEL);
  } else {
    P1.writeDiscrete(LOW, OUTPUT_SLOT, LIMIT_LIGHT_OUTPUT_CHANNEL);
  }
  //Winch control based on states set by client
  if (errorState == true) {
    setCursor(0x54);
    Serial1.print("Return joy to centre");
    // Stop the winch
    stopSignal(OUTPUT_SLOT, STOP_START_OUTPUT_CHANNEL);
    clearOutIn(OUTPUT_SLOT, OUT_OUTPUT_CHANNEL, IN_OUTPUT_CHANNEL);
    P1.writeAnalog(0, ANALOG_SLOT, ANALOG_OUTPUT_CHANNEL1);
  } else if (virtualJoyErr) {
    // Stop the winch
    stopSignal(OUTPUT_SLOT, STOP_START_OUTPUT_CHANNEL);
    clearOutIn(OUTPUT_SLOT, OUT_OUTPUT_CHANNEL, IN_OUTPUT_CHANNEL);
    P1.writeAnalog(0, ANALOG_SLOT, ANALOG_OUTPUT_CHANNEL1);
  } else if (stopState == true) {
    setCursor(0x54);
    Serial1.print("Stopped             ");
    // Stop the winch
    stopSignal(OUTPUT_SLOT, STOP_START_OUTPUT_CHANNEL);
    clearOutIn(OUTPUT_SLOT, OUT_OUTPUT_CHANNEL, IN_OUTPUT_CHANNEL);
    P1.writeAnalog(0, ANALOG_SLOT, ANALOG_OUTPUT_CHANNEL1);
  } else if (stopState == false && startState == true) {               //start is true
    if (outState == true && inState == false && maxPayErr == false) {  // Running Out
      setCursor(0x54);
      Serial1.print("Running Out ");
      runOut(OUTPUT_SLOT, OUT_OUTPUT_CHANNEL, IN_OUTPUT_CHANNEL);
    } else if (inState == true && outState == false && minPayErr == false) {  // Running In
      setCursor(0x54);
      Serial1.print("Running In ");
      runIn(OUTPUT_SLOT, OUT_OUTPUT_CHANNEL, IN_OUTPUT_CHANNEL);
    } else {  // Keep winch ready to run even if out or in isn't pressed
      clearOutIn(OUTPUT_SLOT, OUT_OUTPUT_CHANNEL, IN_OUTPUT_CHANNEL);
      setCursor(0x54);
      Serial1.print("Running          ");
    }
    //Speed Control
    if ((speed <= 4095) && (inState || outState) && (!maxPayErr && !minPayErr)) {
      if (manualState == true) {
        int display = map(speed, 0, 4095, 0, 100);  // Map the 12bit speed value from the joystick to 0-100% for the lcd display
        Serial1.print(display);
        Serial1.print("%    ");
      } else if (manualState == false) {
        Serial1.print(browserSpeed);  // Just print the slider value of 0-100%
        Serial1.print("%   ");
      }
      P1.writeAnalog(speed, ANALOG_SLOT, ANALOG_OUTPUT_CHANNEL1);  // Write the speed to the VFD
      startSignal(OUTPUT_SLOT, STOP_START_OUTPUT_CHANNEL);         // Send the start Signal to the VFD
    } else {                                                       // If speed isn't valid or not all required inputs are set then dont send any values to the VFD
      stopSignal(OUTPUT_SLOT, STOP_START_OUTPUT_CHANNEL);
      speed = 0;
      P1.writeAnalog(speed, ANALOG_SLOT, ANALOG_OUTPUT_CHANNEL1);
    }
  }

  setCursor(0x00);
  if (limitSwitch) {
    if (maxPayErr) {
      Serial1.print("Max Limit!");
    } else if (minPayErr) {
      Serial1.print("Min Limit!");
    } else {
      Serial1.print("Limits ON ");
    }
  } else {
    Serial1.print("Limits OFF");
  }
  if (!sdCard) {
    Serial1.print("  SD!  ");
  } else if (!wsdCard) {
    Serial1.print(" WSD!  ");
  } else {
    Serial1.print("       ");
  }
  if (offsetActive) {
    Serial1.print("OFS");
    tempVal = cablePayout - offset;
  } else {
    Serial1.print("   ");
    tempVal = cablePayout;
  }
  setCursor(0x40);
  Serial1.print("Pay Out        Speed");
  setCursor(0x14);
  if (imperial) {
    tempVal = tempVal * 3.28084;  // convert metric to imperial, tempVal from if offset
    Serial1.print(tempVal, 1);
    if (tempVal < -999) {
      Serial1.print("ft");
    } else if (tempVal < -99) {
      Serial1.print("ft ");
    } else if (tempVal < -9) {
      Serial1.print("ft  ");
    } else if (tempVal < 0) {
      Serial1.print("ft   ");
    } else if (tempVal < 10) {
      Serial1.print("ft    ");
    } else if (tempVal < 100) {
      Serial1.print("ft   ");
    } else if (tempVal < 1000) {
      Serial1.print("ft  ");
    } else {
      Serial1.print("ft ");
    }
    Serial1.print("  ");
  } else {
    // No need to convert metric tempVal from if offset on line 1003
    Serial1.print(tempVal, 1);
    if (tempVal < -999) {
      Serial1.print("m");
    } else if (tempVal < -99) {
      Serial1.print("m ");
    } else if (tempVal < -9) {
      Serial1.print("m  ");
    } else if (tempVal < 0) {
      Serial1.print("m   ");
    } else if (tempVal < 10) {
      Serial1.print("m    ");
    } else if (tempVal < 100) {
      Serial1.print("m   ");
    } else if (tempVal < 1000) {
      Serial1.print("m  ");
    } else {
      Serial1.print("m ");
    }
    Serial1.print("   ");
  }

  if (imperial) {
    tempVal = cableSpeed * 3.28084;  // convert cm to in ask ed
    if (minutes) {
      tempVal *= 60;
    }
    if (tempVal < 10) {
      Serial1.print(" ");
    } else if (tempVal > 999.9) {
      tempVal = 999.9;
    }
    if (seconds) {
      Serial1.print(tempVal, 2);
      Serial1.print("ft/s");
    } else if (minutes) {
      if (tempVal < 100) {
        Serial1.print(" ");
      }
      Serial1.print(tempVal, 1);
      Serial1.print("ft/m");
    }
  } else {
    tempVal = cableSpeed;
    if (minutes) {
      tempVal *= 60;
    }
    if (tempVal < 10) {
      Serial1.print("  ");
    } else if (tempVal < 100) {
      Serial1.print(" ");
    } else if (tempVal > 999.9) {
      tempVal = 999.9;
    }
    if (seconds) {
      Serial1.print(tempVal, 2);
      Serial1.print("m/s");
    } else if (minutes) {
      Serial1.print(" ");  // Make space because there is less characters then for seconds
      Serial1.print(tempVal, 1);
      Serial1.print("m/m");
    }
  }

  if ((currentMillis - NMEATimer) > NMEAPeriod) {
    NMEATimer = currentMillis;
    cableSpeed = fabs(cableSpeed);
    NMEAString = "$YXXDR,D,";
    if (offsetActive) {
      tempVal = cablePayout - offset;
    } else {
      tempVal = cablePayout;
    }
    if (imperial) {
      tempVal = tempVal * 3.28084;
      StringcablePayout = String(tempVal, 1);
      NMEAString.concat(StringcablePayout);
      NMEAString.concat(",F,L,S,");
      tempVal = cableSpeed * 3.28084;
      if (minutes) {
        tempVal *= 60;
      }
      StringcableSpeed = String(tempVal, 1);
      NMEAString.concat(StringcableSpeed);
      NMEAString.concat(",F,R*");
    } else {
      StringcablePayout = String(tempVal, 1);  // tempVal = cablePayout from if above
      NMEAString.concat(StringcablePayout);
      NMEAString.concat(",M,L,S,");
      tempVal = cableSpeed;
      if (minutes) {
        tempVal *= 60;
      }
      StringcableSpeed = String(tempVal, 1);
      NMEAString.concat(StringcableSpeed);
      NMEAString.concat(",M,R*");
    }
    checksum = 0;
    for (int i = 1; i < (NMEAString.length() - 1); i++) {
      checksum = checksum ^ NMEAString[i];
    }
    Serial2.print(NMEAString);
    Serial2.print(checksum, HEX);
    Serial2.print('\r');
    Serial2.print('\n');

    if (clientTCP) {
      if (!alreadyConnected) {
        // clear out the input buffer:
        clientTCP.flush();
        alreadyConnected = true;
      }

      serverTCP.print(NMEAString);
      serverTCP.print(checksum, HEX);
      serverTCP.print('\r');
      serverTCP.print('\n');
    }
  }


  if (Port2.available()) {
    stopSignal(OUTPUT_SLOT, STOP_START_OUTPUT_CHANNEL);  // Stop the winch
    clearOutIn(OUTPUT_SLOT, OUT_OUTPUT_CHANNEL, IN_OUTPUT_CHANNEL);
    P1.writeAnalog(0, ANALOG_SLOT, ANALOG_OUTPUT_CHANNEL1);
    timeout = true;
    stopState = true;
    startState = false;
    inState = false;
    outState = false;
    speed = 0;
    char received = Port2.read();
    Port2.write(received);
    switch (count) {
      case 0:
        if (received == 'c' || received == 'C') {
          count++;
        } else if (received == 'p' || received == 'P') {
          count = 3;
        }
        break;
      case 1:
        if (received == 'm' || received == 'M') {
          count++;
        } else {
          count = 0;
        }
        break;
      case 2:
        if (received == 'd' || received == 'D') {
          ClientSelect();
          count = 0;
        } else {
          count = 0;
        }
        break;
      case 3:
        if (received == 'r' || received == 'R') {
          count++;
        } else {
          count = 0;
        }
        break;
      case 4:
        if (received == 'g' || received == 'G') {
          AgoSelect();
          count = 0;
        } else {
          count = 0;
        }
        break;
      default:
        count = 0;
        break;
    }
  }

  if (!digitalRead(MenuButton)) {
    stopSignal(OUTPUT_SLOT, STOP_START_OUTPUT_CHANNEL);  // Stop the winch
    clearOutIn(OUTPUT_SLOT, OUT_OUTPUT_CHANNEL, IN_OUTPUT_CHANNEL);
    P1.writeAnalog(0, ANALOG_SLOT, ANALOG_OUTPUT_CHANNEL1);
    timeout = true;
    stopState = true;
    startState = false;
    inState = false;
    outState = false;
    speed = 0;
    MainMenu();
    clearScreen();
  }
}
//-----------------------------------------------------------------Serial menuing functions-----------------------------------------------------------------
// AgoSelect *****************************************************************
void AgoSelect() {
  bool ESC = false;
  bool runAgain = false;
  newline();
  newline();
  Serial2.write("--Programming Menu--");
  newline();
  newline();
  Serial2.write("1. Cable Diameter");
  newline();
  Serial2.write("2. Cable Length");
  newline();
  Serial2.write("3. Drum Diameter");
  newline();
  Serial2.write("4. Drum Width");
  newline();
  Serial2.write("5. Scale Factor");
  newline();
  Serial2.write("6. Stretch Factor");
  newline();
  Serial2.write("7. Encoder Resolution");
  newline();
  Serial2.write("8. MAX Cable Payout Limit");
  newline();
  Serial2.write("9. MIN Cable Payout Limit");
  newline();
  Serial2.write("0. Reset Payout Count");
  newline();
  Serial2.write("a. NMEA Period");
  newline();
  newline();
  Serial2.write("ESC to leave any menu");
  newline();
  newline();
  while (!ESC) {  //bookmark1
    if (Serial2.available()) {
      int received = Serial2.read();
      switch (received) {
        case '1':
          Serial2.write("Set Cable Diameter in Centimeters (00.000cm)");
          newline();
          if (sdCard) {
            cableDiameter = SetCableDiameter();
            drumFile = SD.open("drmConst.TXT", (O_READ | O_WRITE));
            drumFile.find("cableDiameter:");
            drumFile.print(cableDiameter, 5);
            drumFile.print(",");
            drumFile.close();
          }
          ESC = true;
          runAgain = true;
          break;
        case '2':
          Serial2.write("Set Cable Length in Meters (0000.0m)");
          newline();
          cableLength = SetCableLength();
          if (sdCard) {
            drumFile = SD.open("drmConst.TXT", (O_READ | O_WRITE));
            drumFile.find("cableLength:");
            drumFile.position();
            drumFile.print(cableLength, 1);
            drumFile.print(",");
            drumFile.close();
          }
          ESC = true;
          runAgain = true;
          break;
        case '3':
          Serial2.write("Set Drum Diameter in Centimeters (00.000cm)");
          newline();
          drumDiameter = SetDrumDiameter();
          if (sdCard) {
            drumFile = SD.open("drmConst.TXT", (O_READ | O_WRITE));
            drumFile.find("drumDiameter:");
            drumFile.position();
            drumFile.print(drumDiameter, 5);
            drumFile.print(",");
            drumFile.close();
          }
          ESC = true;
          runAgain = true;
          break;
        case '4':
          Serial2.write("Set Drum Width in Centimeters (00.000cm)");
          newline();
          drumWidth = SetDrumWidth();
          if (sdCard) {
            drumFile = SD.open("drmConst.TXT", (O_READ | O_WRITE));
            drumFile.find("drumWidth:");
            drumFile.position();
            drumFile.print(drumWidth, 5);
            drumFile.print(",");
            drumFile.close();
          }
          ESC = true;
          runAgain = true;
          break;
        case '5':
          Serial2.write("Set Scale Factor (0.0000)");
          newline();
          scaleFactor = SetScaleFactor();
          if (sdCard) {
            drumFile = SD.open("drmConst.TXT", (O_READ | O_WRITE));
            drumFile.find("scaleFactor:");
            drumFile.position();
            drumFile.print(scaleFactor, 4);
            drumFile.print(",");
            drumFile.close();
          }
          ESC = true;
          runAgain = true;
          break;
        case '6':
          Serial2.write("Set Stretch Factor (0.0000)");
          newline();
          stretchFactor = SetStretchFactor();
          if (sdCard) {
            drumFile = SD.open("drmConst.TXT", (O_READ | O_WRITE));
            drumFile.find("stretchFactor:");
            drumFile.position();
            drumFile.print(stretchFactor, 4);
            drumFile.print(",");
            drumFile.close();
          }
          ESC = true;
          runAgain = true;
          break;
        case '7':
          Serial2.write("Set Encoder Count(0000)");
          newline();
          drumEncRes = SetEncoderCount();
          if (sdCard) {
            drumFile = SD.open("drmConst.TXT", (O_READ | O_WRITE));
            drumFile.find("drumEncRes:");
            drumFile.position();
            drumFile.print(drumEncRes);
            drumFile.print(",");
            drumFile.close();
          }
          ESC = true;
          runAgain = true;
          break;
        case '8':
          Serial2.write("Set MAX Payout Limit(0000.0m)");
          newline();
          maxCablePayedOut = SetMaxPayout();
          if (sdCard) {
            drumFile = SD.open("DRMCONST.TXT", (O_READ | O_WRITE));
            drumFile.find("maxCablePayedOut:");
            drumFile.print(maxCablePayedOut, 1);
            drumFile.print(',');
            drumFile.close();
          }
          updateLimits = true;
          ESC = true;
          runAgain = true;
          break;
        case '9':
          Serial2.write("Set MIN Payout Limit(0000.0m)");  //minLimitLimitSet
          newline();
          minCablePayedOut = SetMinPayout();
          if (sdCard) {
            drumFile = SD.open("DRMCONST.TXT", (O_READ | O_WRITE));
            drumFile.find("minCablePayedOut:");
            drumFile.print(minCablePayedOut, 1);
            drumFile.print(',');
            drumFile.close();
          }
          updateLimits = true;
          ESC = true;
          runAgain = true;
          break;
        case '0':
          offsetActive = false;
          offset = 0.00;
          cableLength = 0.00;
          retainCount = 0.0;
          encoderPreviousCount = 0.0;
          HSC.CNT1.setPosition(0);
          Serial2.write("Encoder Count Reset!");
          ESC = true;
          runAgain = true;
          break;
        case 'a':
          Serial2.write("Set NMEA Period (0000.0s)");
          newline();
          NMEAPeriod = SetNMEAPeriod();
          if (sdCard) {
            tempVal = NMEAPeriod / 1000.0;
            drumFile = SD.open("DRMCONST.TXT", (O_READ | O_WRITE));
            drumFile.find("NMEAPeriod:");
            drumFile.position();
            drumFile.print(tempVal, 1);
            drumFile.print(",");
            drumFile.close();
          }
          ESC = true;
          runAgain = true;
          break;
        case 27:
          ESC = true;
          break;
        default:
          Serial2.write("Select a menu number");
          Serial2.write(13);
          break;
      }
    }
  }
  if (runAgain) {
    AgoSelect();
    runAgain = false;
  } else {
    Serial2.write("Escaped Menu!       ");
    newline();
  }
}

// Client Select *****************************************************************
void ClientSelect() {
  bool ESC = false;
  bool runAgain = false;
  newline();
  newline();
  Serial2.write("--Main Menu--");
  newline();
  newline();
  Serial2.write("1. Cable Diameter");
  newline();
  Serial2.write("2. Cable Length");
  newline();
  Serial2.write("3. Scale Factor");
  newline();
  Serial2.write("4. Stretch Factor");
  newline();
  Serial2.write("5. MAX Cable Payout Limit");
  newline();
  Serial2.write("6. MIN Cable Payout Limit");
  newline();
  Serial2.write("7. Reset Payout Count");
  newline();
  Serial2.write("8. NMEA Period");
  newline();
  newline();
  Serial2.write("ESC to leave any menu");
  newline();
  newline();
  while (!ESC) {
    if (Serial2.available()) {
      int received = Serial2.read();
      switch (received) {
        case '1':
          Serial2.write("Set Cable Diameter in Centimeters (00.000cm)");
          newline();
          cableDiameter = SetCableDiameter();
          if (sdCard) {
            drumFile = SD.open("DRMCONST.TXT", (O_READ | O_WRITE));
            drumFile.find("cableDiameter:");
            drumFile.print(cableDiameter, 5);
            drumFile.print(',');
            drumFile.close();
          }
          ESC = true;
          runAgain = true;
          break;
        case '2':
          Serial2.write("Set Cable Length in Meters (0000.0m)");
          newline();
          cableLength = SetCableLength();
          if (sdCard) {
            drumFile = SD.open("DRMCONST.TXT", (O_READ | O_WRITE));
            drumFile.find("cableLength:");
            drumFile.print(cableLength, 1);
            drumFile.print(',');
            drumFile.close();
          }
          ESC = true;
          runAgain = true;
          break;
        case '3':
          Serial2.write("Set Scale Factor (0.0000)");
          newline();
          scaleFactor = SetScaleFactor();
          cablePayout = GetCablePayout();
          setCursor(0x14);
          Serial1.print(cablePayout, 1);
          if (sdCard) {
            drumFile = SD.open("DRMCONST.TXT", (O_READ | O_WRITE));
            drumFile.find("scaleFactor:");
            drumFile.print(scaleFactor, 4);
            drumFile.print(',');
            drumFile.close();
          }
          ESC = true;
          runAgain = true;
          break;
        case '4':
          Serial2.write("Set Stretch Factor (0.0000)");
          newline();
          stretchFactor = SetStretchFactor();
          if (sdCard) {
            drumFile = SD.open("DRMCONST.TXT", (O_READ | O_WRITE));
            drumFile.find("stretchFactor:");
            drumFile.print(stretchFactor, 4);
            drumFile.print(',');
            drumFile.close();
          }
          ESC = true;
          runAgain = true;
          break;
        case '5':
          Serial2.write("Set MAX Payout Limit(0000.0m)");
          newline();
          maxCablePayedOut = SetMaxPayout();
          if (sdCard) {
            drumFile = SD.open("DRMCONST.TXT", (O_READ | O_WRITE));
            drumFile.find("maxCablePayedOut:");
            drumFile.print(maxCablePayedOut, 1);
            drumFile.print(',');
            drumFile.close();
          }
          updateLimits = true;
          ESC = true;
          runAgain = true;
          break;
        case '6':
          Serial2.write("Set MIN Payout Limit(0000.0m)");  //minLimitLimitSet
          newline();
          minCablePayedOut = SetMinPayout();
          if (sdCard) {
            drumFile = SD.open("DRMCONST.TXT", (O_READ | O_WRITE));
            drumFile.find("minCablePayedOut:");
            drumFile.print(minCablePayedOut, 1);
            drumFile.print(',');
            drumFile.close();
          }
          updateLimits = true;
          ESC = true;
          runAgain = true;
          break;
        case '7':
          offsetActive = false;
          offset = 0;
          cableLength = 0;
          retainCount = 0;
          encoderPreviousCount = 0;
          HSC.CNT1.setPosition(0);
          Serial2.write("Encoder Count Reset!");
          ESC = true;
          runAgain = true;
          break;
        case '8':
          Serial2.write("Set NMEA Period (0000.0s)");
          newline();
          NMEAPeriod = SetNMEAPeriod();
          if (sdCard) {
            tempVal = NMEAPeriod / 1000.0;
            drumFile = SD.open("DRMCONST.TXT", (O_READ | O_WRITE));
            drumFile.find("NMEAPeriod:");
            drumFile.position();
            drumFile.print(tempVal, 1);
            drumFile.print(",");
            drumFile.close();
          }
          ESC = true;
          runAgain = true;
          break;
        case 27:
          ESC = true;
          break;
        default:
          Serial2.write("Select a menu number");
          Serial2.write(13);
          break;
      }
    }
  }
  if (runAgain) {
    ClientSelect();
    runAgain = false;
  } else {
    Serial2.write("Escaped Menu!       ");
    newline();
  }
}

//cable diameter *****************************************************************
float SetCableDiameter() {
  byte buff[] = { 0, 0, 0, 0, 0 };
  byte buffLength = 5;
  byte position = 0;
  byte decimalLocation = 1;
  byte decimalShiftCount = 0;
  byte decimalPrint;
  bool step = false;
  bool inputERR = false;
  bool dot = false;
  bool confirm = false;
  bool ESC = false;
  bool dotManualEntry;
  float diameter = 0;
  do {
    if (Port2.available()) {
      if (position == 0) {
        Port2.write(13);
        Port2.write("                                           ");
        Port2.write(13);
      }
      char received = Port2.read();
      if (received == 8 && position > 0) {                            // Backspace
        if (((position - 1) == decimalLocation) && dotManualEntry) {  // If the dot has been manully entered(array shifted to decimal) then we need to shift the array back to highest place value
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          for (int i = 0; i < decimalShiftCount;) {
            //Shifts the 5 element array one place right-->
            for (byte shift = 0; shift < buffLength; shift++) {
              buff[shift] = buff[shift + 1];
              if (shift == (buffLength - 1)) {
                buff[buffLength] = 0;
              }
            }
            decimalShiftCount--;
            position--;
          }
          dotManualEntry = false;
          dot = false;
        } else if (((position - 1) == decimalLocation) && dot) {  // If deleting the dot that is automatically placed
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          dot = false;
        } else {
          if (inputERR) {  // If the error message is displayed delete it
            Port2.write("                             ");
            for (int i = 0; i < 29; i++) {
              Port2.write(8);
            }
            inputERR = false;
          }
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          position--;
          buff[position] = 0;
        }
      } else if (received == '.' && !dot) {  // Dot is entered before formatting calls for
        dotManualEntry = true;
        dot = true;
        Port2.write(received);
        for (int i = 0; i <= (decimalLocation - position);) {
          //Shifts the 5 element array one place right-->
          for (byte shift = buffLength; shift > 0; shift--) {
            buff[shift] = buff[shift - 1];
            if (shift == 1) {
              buff[0] = 0;
            }
          }
          decimalShiftCount++;
          position++;
        }
      } else if (received == 27) {  //ESC received
        ESC = true;
        break;
      } else if (('0' <= received && received <= '9') && (position < buffLength)) {  //Receiving proper characters
        if ((position == (decimalLocation + 1)) && !dot) {                           // If the user didn't manually put in the decimal point place it for them
          Port2.write('.');
          dot = true;
        }
        Port2.write(received);
        buff[position] = received - '0';
        position++;
      } else if (received == 13) {  // Receiving Enter
        inputERR = false;
        if (position == 0) {  // Do nothing with no input
          ;
        } else if (dot) {
          confirm = true;
        } else if (position == buffLength) {
          Port2.write("                             ");
          confirm = true;
        } else {  // Correct sigfigs if enter is pressed early and no decimal point was entered
          for (int i = 0; i <= (decimalLocation - position); i++) {
            //Shifts the 5 element array one place right-->
            for (byte shift = buffLength; shift > 0; shift--) {
              buff[shift] = buff[shift - 1];
              if (shift == 1) {
                buff[0] = 0;
              }
            }
          }
          confirm = true;
        }
      } else if (position >= buffLength) {  // If the array has been filled do not accept more values, then promt the user to submit entry
        Port2.write(" Press enter to confirm value");
        for (int i = 0; i < 29; i++) {
          Port2.write(8);
        }
        inputERR = true;
      } else {  // Deal with any characters that arn't accepted
        Port2.write(13);
        Port2.write("Please input a number between 00.000-99.999");
        for (int i = 0; i < buffLength; i++) {
          buff[i] = 0;
        }
        inputERR = true;
        break;
      }
    }
  } while (!confirm);
  if (inputERR) {
    SetCableDiameter();
  } else if (ESC) {
    newline();
    Port2.write("Input cancelled, Returning to main menu!");
    newline();
    Port2.write("The current value is: ");
    cableDiameter *= 100;
    Port2.print(cableDiameter, 3);
    cableDiameter *= 0.01;
    return cableDiameter;
  } else {
    for (int i = 0; i < buffLength; i++) {
      diameter += buff[i] * (pow(10, (decimalLocation - i)));
    }
    newline();
    newline();
    Port2.write("Cable Diameter Stored!");
    Port2.write(' ');
    if ((position - 1) > decimalLocation) {
      decimalPrint = ((position - 1) - decimalLocation);
    } else {
      decimalPrint = 0;
    }
    Port2.print(diameter, decimalPrint);
    Port2.write(" cm");
    diameter *= 0.01;
    return diameter;
  }
}
// Cable length *****************************************************************
float SetCableLength() {
  byte buff[] = { 0, 0, 0, 0, 0 };
  byte buffLength = 5;
  byte position = 0;
  byte decimalLocation = 3;
  byte decimalShiftCount = 0;
  byte decimalPrint;
  bool step = false;
  bool inputERR = false;
  bool dot = false;
  bool confirm = false;
  bool ESC = false;
  bool dotManualEntry;
  float length = 0;
  if (drumCountNegative == true) {
    drumDirectionNegative = true;
  } else if (drumCountNegative == false) {
    drumDirectionNegative = false;
  }
  do {
    if (Port2.available()) {
      if (position == 0) {
        Port2.write(13);
        Port2.write("                                           ");
        Port2.write(13);
      }
      char received = Port2.read();
      if (received == 8 && position > 0) {                            // Backspace
        if (((position - 1) == decimalLocation) && dotManualEntry) {  // If the dot has been manully entered(array shifted to decimal) then we need to shift the array back to highest place value
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          for (int i = 0; i < decimalShiftCount;) {
            //Shifts the 5 element array one place right-->
            for (byte shift = 0; shift < buffLength; shift++) {
              buff[shift] = buff[shift + 1];
              if (shift == (buffLength - 1)) {
                buff[buffLength] = 0;
              }
            }
            decimalShiftCount--;
            position--;
          }
          dotManualEntry = false;
          dot = false;
        } else if (((position - 1) == decimalLocation) && dot) {  // If deleting the dot that is automatically placed
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          dot = false;
        } else {
          if (inputERR) {  // If the error message is displayed delete it
            Port2.write("                             ");
            for (int i = 0; i < 29; i++) {
              Port2.write(8);
            }
            inputERR = false;
          }
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          position--;
          buff[position] = 0;
        }
      } else if (received == '.' && !dot) {  // Dot is entered before formatting calls for
        dotManualEntry = true;
        dot = true;
        Port2.write(received);
        for (int i = 0; i <= (decimalLocation - position);) {
          //Shifts the 5 element array one place right-->
          for (byte shift = buffLength; shift > 0; shift--) {
            buff[shift] = buff[shift - 1];
            if (shift == 1) {
              buff[0] = 0;
            }
          }
          decimalShiftCount++;
          position++;
        }
      } else if (received == 27) {  //ESC received
        ESC = true;
        break;
      } else if (('0' <= received && received <= '9') && position < buffLength) {  //Receiving proper characters
        if ((position == (decimalLocation + 1)) && !dot) {                         // If the user didn't manually put in the decimal point place it for them
          Port2.write('.');
          dot = true;
        }
        Port2.write(received);
        buff[position] = received - '0';
        position++;
      } else if (received == 13) {  // Receiving Enter
        inputERR = false;
        if (position == 0) {  // Do nothing with no input
          ;
        } else if (dot) {
          confirm = true;
        } else if (position == buffLength) {
          Port2.write("                             ");
          confirm = true;
        } else {  // Correct sigfigs if enter is pressed early and no decimal point was entered
          for (int i = 0; i <= (decimalLocation - position); i++) {
            //Shifts the 5 element array one place right-->
            for (byte shift = buffLength; shift > 0; shift--) {
              buff[shift] = buff[shift - 1];
              if (shift == 1) {
                buff[0] = 0;
              }
            }
          }
          confirm = true;
        }
      } else if (position >= buffLength) {  // If the array has been filled do not accept more values, then promt the user to submit entry
        Port2.write(" Press enter to confirm value");
        for (int i = 0; i < 29; i++) {
          Port2.write(8);
        }
        inputERR = true;
      } else {  // Deal with any characters that arn't accepted
        Port2.write(13);
        Port2.write("Please input a number between 0000.0-9999.9");
        for (int i = 0; i < buffLength; i++) {
          buff[i] = 0;
        }
        inputERR = true;
        break;
      }
    }
  } while (!confirm);
  if (inputERR) {
    SetCableLength();
  } else if (ESC) {
    newline();
    Port2.write("Input cancelled, Returning to main menu!");
    newline();
    Port2.write("The current value is: ");
    Port2.print(cableLength, 1);
    return cableLength;
  } else {
    for (int i = 0; i < buffLength; i++) {
      length += buff[i] * (pow(10, (decimalLocation - i)));
    }
    newline();
    newline();
    Port2.write("Cable Length Stored!");
    Port2.write(' ');
    if ((position - 1) > decimalLocation) {
      decimalPrint = ((position - 1) - decimalLocation);
    } else {
      decimalPrint = 0;
    }
    Port2.print(length, decimalPrint);
    Port2.write(" m");
    return length;
  }
}
// Drum diameter*****************************************************************
float SetDrumDiameter() {
  byte buff[] = { 0, 0, 0, 0, 0 };
  byte buffLength = 5;
  byte position = 0;
  byte decimalLocation = 1;
  byte decimalShiftCount = 0;
  byte decimalPrint;
  bool step = false;
  bool inputERR = false;
  bool dot = false;
  bool confirm = false;
  bool ESC = false;
  bool dotManualEntry;
  float diameter = 0;
  do {
    if (Port2.available()) {
      if (position == 0) {
        Port2.write(13);
        Port2.write("                                           ");
        Port2.write(13);
      }
      char received = Port2.read();
      if (received == 8 && position > 0) {                            // Backspace
        if (((position - 1) == decimalLocation) && dotManualEntry) {  // If the dot has been manully entered(array shifted to decimal) then we need to shift the array back to highest place value
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          for (int i = 0; i < decimalShiftCount;) {
            //Shifts the 5 element array one place right-->
            for (byte shift = 0; shift < buffLength; shift++) {
              buff[shift] = buff[shift + 1];
              if (shift == (buffLength - 1)) {
                buff[buffLength] = 0;
              }
            }
            decimalShiftCount--;
            position--;
          }
          dotManualEntry = false;
          dot = false;
        } else if (((position - 1) == decimalLocation) && dot) {  // If deleting the dot that is automatically placed
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          dot = false;
        } else {
          if (inputERR) {  // If the error message is displayed delete it
            Port2.write("                             ");
            for (int i = 0; i < 29; i++) {
              Port2.write(8);
            }
            inputERR = false;
          }
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          position--;
          buff[position] = 0;
        }
      } else if (received == '.' && !dot) {  // Dot is entered before formatting calls for
        dotManualEntry = true;
        dot = true;
        Port2.write(received);
        for (int i = 0; i <= (decimalLocation - position);) {
          //Shifts the 5 element array one place right-->
          for (byte shift = buffLength; shift > 0; shift--) {
            buff[shift] = buff[shift - 1];
            if (shift == 1) {
              buff[0] = 0;
            }
          }
          decimalShiftCount++;
          position++;
        }
      } else if (received == 27) {  //ESC received
        ESC = true;
        break;
      } else if (('0' <= received && received <= '9') && (position < buffLength)) {  //Receiving proper characters
        if ((position == (decimalLocation + 1)) && !dot) {                           // If the user didn't manually put in the decimal point place it for them
          Port2.write('.');
          dot = true;
        }
        Port2.write(received);
        buff[position] = received - '0';
        position++;
      } else if (received == 13) {  // Receiving Enter
        inputERR = false;
        if (position == 0) {  // Do nothing with no input
          ;
        } else if (dot) {
          confirm = true;
        } else if (position == buffLength) {
          Port2.write("                             ");
          confirm = true;
        } else {  // Correct sigfigs if enter is pressed early and no decimal point was entered
          for (int i = 0; i <= (decimalLocation - position); i++) {
            //Shifts the 5 element array one place right-->
            for (byte shift = buffLength; shift > 0; shift--) {
              buff[shift] = buff[shift - 1];
              if (shift == 1) {
                buff[0] = 0;
              }
            }
          }
          confirm = true;
        }
      } else if (position >= buffLength) {  // If the array has been filled do not accept more values, then promt the user to submit entry
        Port2.write(" Press enter to confirm value");
        for (int i = 0; i < 29; i++) {
          Port2.write(8);
        }
        inputERR = true;
      } else {  // Deal with any characters that arn't accepted
        Port2.write(13);
        Port2.write("Please input a number between 00.000-99.999");
        for (int i = 0; i < buffLength; i++) {
          buff[i] = 0;
        }
        inputERR = true;
        break;
      }
    }
  } while (!confirm);
  if (inputERR) {
    SetDrumDiameter();
  } else if (ESC) {
    newline();
    Port2.write("Input cancelled, Returning to main menu!");
    newline();
    Port2.write("The current value is: ");
    drumDiameter *= 100;
    Port2.print(drumDiameter, 3);
    drumDiameter *= 0.01;
    return drumDiameter;
  } else {
    for (int i = 0; i < buffLength; i++) {
      diameter += buff[i] * (pow(10, (decimalLocation - i)));
    }
    newline();
    newline();
    Port2.write("Drum Diameter Stored!");
    Port2.write(' ');
    if ((position - 1) > decimalLocation) {
      decimalPrint = ((position - 1) - decimalLocation);
    } else {
      decimalPrint = 0;
    }
    Port2.print(diameter, decimalPrint);
    Port2.write(" cm");
    diameter *= 0.01;
    return diameter;
  }
}

// Drum Width*****************************************************************
float SetDrumWidth() {
  byte buff[] = { 0, 0, 0, 0, 0 };
  byte buffLength = 5;
  byte position = 0;
  byte decimalLocation = 1;
  byte decimalShiftCount = 0;
  byte decimalPrint;
  bool step = false;
  bool inputERR = false;
  bool dot = false;
  bool confirm = false;
  bool ESC = false;
  bool dotManualEntry;
  float diameter = 0;
  do {
    if (Port2.available()) {
      if (position == 0) {
        Port2.write(13);
        Port2.write("                                           ");
        Port2.write(13);
      }
      char received = Port2.read();
      if (received == 8 && position > 0) {                            // Backspace
        if (((position - 1) == decimalLocation) && dotManualEntry) {  // If the dot has been manully entered(array shifted to decimal) then we need to shift the array back to highest place value
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          for (int i = 0; i < decimalShiftCount;) {
            //Shifts the 5 element array one place right-->
            for (byte shift = 0; shift < buffLength; shift++) {
              buff[shift] = buff[shift + 1];
              if (shift == (buffLength - 1)) {
                buff[buffLength] = 0;
              }
            }
            decimalShiftCount--;
            position--;
          }
          dotManualEntry = false;
          dot = false;
        } else if (((position - 1) == decimalLocation) && dot) {  // If deleting the dot that is automatically placed
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          dot = false;
        } else {
          if (inputERR) {  // If the error message is displayed delete it
            Port2.write("                             ");
            for (int i = 0; i < 29; i++) {
              Port2.write(8);
            }
            inputERR = false;
          }
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          position--;
          buff[position] = 0;
        }
      } else if (received == '.' && !dot) {  // Dot is entered before formatting calls for
        dotManualEntry = true;
        dot = true;
        Port2.write(received);
        for (int i = 0; i <= (decimalLocation - position);) {
          //Shifts the 5 element array one place right-->
          for (byte shift = buffLength; shift > 0; shift--) {
            buff[shift] = buff[shift - 1];
            if (shift == 1) {
              buff[0] = 0;
            }
          }
          decimalShiftCount++;
          position++;
        }
      } else if (received == 27) {  //ESC received
        ESC = true;
        break;
      } else if (('0' <= received && received <= '9') && (position < buffLength)) {  //Receiving proper characters
        if ((position == (decimalLocation + 1)) && !dot) {                           // If the user didn't manually put in the decimal point place it for them
          Port2.write('.');
          dot = true;
        }
        Port2.write(received);
        buff[position] = received - '0';
        position++;
      } else if (received == 13) {  // Receiving Enter
        inputERR = false;
        if (position == 0) {  // Do nothing with no input
          ;
        } else if (dot) {
          confirm = true;
        } else if (position == buffLength) {
          Port2.write("                             ");
          confirm = true;
        } else {  // Correct sigfigs if enter is pressed early and no decimal point was entered
          for (int i = 0; i <= (decimalLocation - position); i++) {
            //Shifts the 5 element array one place right-->
            for (byte shift = buffLength; shift > 0; shift--) {
              buff[shift] = buff[shift - 1];
              if (shift == 1) {
                buff[0] = 0;
              }
            }
          }
          confirm = true;
        }
      } else if (position >= buffLength) {  // If the array has been filled do not accept more values, then promt the user to submit entry
        Port2.write(" Press enter to confirm value");
        for (int i = 0; i < 29; i++) {
          Port2.write(8);
        }
        inputERR = true;
      } else {  // Deal with any characters that arn't accepted
        Port2.write(13);
        Port2.write("Please input a number between 00.000-99.999");
        for (int i = 0; i < buffLength; i++) {
          buff[i] = 0;
        }
        inputERR = true;
        break;
      }
    }
  } while (!confirm);
  if (inputERR) {
    SetDrumWidth();
  } else if (ESC) {
    newline();
    Port2.write("Input cancelled, Returning to main menu!");
    newline();
    Port2.write("The current value is: ");
    drumWidth *= 100;
    Port2.print(drumWidth, 3);
    drumWidth *= 0.01;
    return drumWidth;
  } else {
    for (int i = 0; i < buffLength; i++) {
      diameter += buff[i] * (pow(10, (decimalLocation - i)));
    }
    newline();
    newline();
    Port2.write("Drum Width Stored!");
    Port2.write(' ');
    if ((position - 1) > decimalLocation) {
      decimalPrint = ((position - 1) - decimalLocation);
    } else {
      decimalPrint = 0;
    }
    Port2.print(diameter, decimalPrint);
    Port2.write(" cm");
    diameter *= 0.01;
    return diameter;
  }
}

// Encoder Count*****************************************************************
int SetEncoderCount() {
  byte buff[] = { 0, 0, 0, 0 };
  byte buffLength = 4;
  byte position = 0;
  byte decimalLocation = 3;
  byte decimalShiftCount = 0;
  byte decimalPrint;
  bool step = false;
  bool inputERR = false;
  bool dot = false;
  bool confirm = false;
  bool ESC = false;
  bool dotManualEntry;
  int eCount = 0;
  do {
    if (Port2.available()) {
      if (position == 0) {
        Port2.write(13);
        Port2.write("                                           ");
        Port2.write(13);
      }
      char received = Port2.read();
      if (received == 8 && position > 0) {                            // Backspace
        if (((position - 1) == decimalLocation) && dotManualEntry) {  // If the dot has been manully entered(array shifted to decimal) then we need to shift the array back to highest place value
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          for (int i = 0; i < decimalShiftCount;) {
            //Shifts the 5 element array one place right-->
            for (byte shift = 0; shift < buffLength; shift++) {
              buff[shift] = buff[shift + 1];
              if (shift == (buffLength - 1)) {
                buff[buffLength] = 0;
              }
            }
            decimalShiftCount--;
            position--;
          }
          dotManualEntry = false;
          dot = false;
        } else if (((position - 1) == decimalLocation) && dot) {  // If deleting the dot that is automatically placed
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          dot = false;
        } else {
          if (inputERR) {  // If the error message is displayed delete it
            Port2.write("                             ");
            for (int i = 0; i < 29; i++) {
              Port2.write(8);
            }
            inputERR = false;
          }
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          position--;
          buff[position] = 0;
        }
      } else if (received == '.' && !dot) {  // Dot is entered before formatting calls for
        dotManualEntry = true;
        dot = true;
        Port2.write(received);
        for (int i = 0; i <= (decimalLocation - position);) {
          //Shifts the 5 element array one place right-->
          for (byte shift = buffLength; shift > 0; shift--) {
            buff[shift] = buff[shift - 1];
            if (shift == 1) {
              buff[0] = 0;
            }
          }
          decimalShiftCount++;
          position++;
        }
      } else if (received == 27) {  //ESC received
        ESC = true;
        break;
      } else if (('0' <= received && received <= '9') && position < buffLength) {  //Receiving proper characters
        if ((position == (decimalLocation + 1)) && !dot) {                         // If the user didn't manually put in the decimal point place it for them
          Port2.write('.');
          dot = true;
        }
        Port2.write(received);
        buff[position] = received - '0';
        position++;
      } else if (received == 13) {  // Receiving Enter
        inputERR = false;
        if (position == 0) {  // Do nothing with no input
          ;
        } else if (dot) {
          confirm = true;
        } else if (position == buffLength) {
          Port2.write("                             ");
          confirm = true;
        } else {  // Correct sigfigs if enter is pressed early and no decimal point was entered
          for (int i = 0; i <= (decimalLocation - position); i++) {
            //Shifts the 5 element array one place right-->
            for (byte shift = buffLength; shift > 0; shift--) {
              buff[shift] = buff[shift - 1];
              if (shift == 1) {
                buff[0] = 0;
              }
            }
          }
          confirm = true;
        }
      } else if (position >= buffLength) {  // If the array has been filled do not accept more values, then promt the user to submit entry
        Port2.write(" Press enter to confirm value");
        for (int i = 0; i < 29; i++) {
          Port2.write(8);
        }
        inputERR = true;
      } else {  // Deal with any characters that arn't accepted
        Port2.write(13);
        Port2.write("Please input a number between 0.000-9.999");
        for (int i = 0; i < buffLength; i++) {
          buff[i] = 0;
        }
        inputERR = true;
        break;
      }
    }
  } while (!confirm);
  if (inputERR) {
    SetEncoderCount();
  } else if (ESC) {
    newline();
    Port2.write("Input cancelled, Returning to main menu!");
    newline();
    Port2.write("The current value is: ");
    Port2.print(drumEncRes, 0);
    return drumEncRes;
  } else {
    for (int i = 0; i < buffLength; i++) {
      eCount += buff[i] * (pow(10, (decimalLocation - i)));
    }
    newline();
    newline();
    Port2.write("Encoder Count Stored!");
    Port2.write(' ');
    Port2.print(eCount);
    return eCount;
  }
}

// Stretch factor*****************************************************************
float SetStretchFactor() {
  byte buff[] = { 0, 0, 0, 0, 0 };
  byte buffLength = 5;
  byte position = 0;
  byte decimalLocation = 0;
  byte decimalShiftCount = 0;
  byte decimalPrint;
  bool step = false;
  bool inputERR = false;
  bool dot = false;
  bool confirm = false;
  bool ESC = false;
  bool dotManualEntry;
  float factor = 0;
  do {
    if (Port2.available()) {
      if (position == 0) {
        Port2.write(13);
        Port2.write("                                           ");
        Port2.write(13);
      }
      char received = Port2.read();
      if (received == 8 && position > 0) {                            // Backspace
        if (((position - 1) == decimalLocation) && dotManualEntry) {  // If the dot has been manully entered(array shifted to decimal) then we need to shift the array back to highest place value
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          for (int i = 0; i < decimalShiftCount;) {
            //Shifts the 5 element array one place right-->
            for (byte shift = 0; shift < buffLength; shift++) {
              buff[shift] = buff[shift + 1];
              if (shift == (buffLength - 1)) {
                buff[buffLength] = 0;
              }
            }
            decimalShiftCount--;
            position--;
          }
          dotManualEntry = false;
          dot = false;
        } else if (((position - 1) == decimalLocation) && dot) {  // If deleting the dot that is automatically placed
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          dot = false;
        } else {
          if (inputERR) {  // If the error message is displayed delete it
            Port2.write("                             ");
            for (int i = 0; i < 29; i++) {
              Port2.write(8);
            }
            inputERR = false;
          }
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          position--;
          buff[position] = 0;
        }
      } else if (received == '.' && !dot) {  // Dot is entered before formatting calls for
        dotManualEntry = true;
        dot = true;
        Port2.write(received);
        for (int i = 0; i <= (decimalLocation - position);) {
          //Shifts the 5 element array one place right-->
          for (byte shift = buffLength; shift > 0; shift--) {
            buff[shift] = buff[shift - 1];
            if (shift == 1) {
              buff[0] = 0;
            }
          }
          decimalShiftCount++;
          position++;
        }
      } else if (received == 27) {  //ESC received
        ESC = true;
        break;
      } else if (('0' <= received && received <= '9') && position < buffLength) {  //Receiving proper characters
        if ((position == (decimalLocation + 1)) && !dot) {                         // If the user didn't manually put in the decimal point place it for them
          Port2.write('.');
          dot = true;
        }
        Port2.write(received);
        buff[position] = received - '0';
        position++;
      } else if (received == 13) {  // Receiving Enter
        inputERR = false;
        if (position == 0) {  // Do nothing with no input
          ;
        } else if (dot) {
          confirm = true;
        } else if (position == buffLength) {
          Port2.write("                             ");
          confirm = true;
        } else {  // Correct sigfigs if enter is pressed early and no decimal point was entered
          for (int i = 0; i <= (decimalLocation - position); i++) {
            //Shifts the 5 element array one place right-->
            for (byte shift = buffLength; shift > 0; shift--) {
              buff[shift] = buff[shift - 1];
              if (shift == 1) {
                buff[0] = 0;
              }
            }
          }
          confirm = true;
        }
      } else if (position >= buffLength) {  // If the array has been filled do not accept more values, then promt the user to submit entry
        Port2.write(" Press enter to confirm value");
        for (int i = 0; i < 29; i++) {
          Port2.write(8);
        }
        inputERR = true;
      } else {  // Deal with any characters that arn't accepted
        Port2.write(13);
        Port2.write("Please input a number between 0.0000-9.9999");
        for (int i = 0; i < buffLength; i++) {
          buff[i] = 0;
        }
        inputERR = true;
        break;
      }
    }
  } while (!confirm);
  if (inputERR) {
    SetStretchFactor();
  } else if (ESC) {
    newline();
    Port2.write("Input cancelled, Returning to main menu!");
    newline();
    Port2.write("The current value is: ");
    Port2.print(stretchFactor, 4);
    return stretchFactor;
  } else {
    for (int i = 0; i < buffLength; i++) {
      factor += buff[i] * (pow(10, (decimalLocation - i)));
    }
    newline();
    newline();
    Port2.write("Stretch Factor Stored!");
    Port2.write(' ');
    if ((position - 1) > decimalLocation) {
      decimalPrint = ((position - 1) - decimalLocation);
    } else {
      decimalPrint = 0;
    }
    Port2.print(factor, decimalPrint);
    return factor;
  }
}

// Scale factor*****************************************************************
float SetScaleFactor() {
  byte buff[] = { 0, 0, 0, 0, 0 };
  byte buffLength = 5;
  byte position = 0;
  byte decimalLocation = 0;
  byte decimalShiftCount = 0;
  byte decimalPrint;
  bool step = false;
  bool inputERR = false;
  bool dot = false;
  bool confirm = false;
  bool ESC = false;
  bool dotManualEntry;
  float factor = 0;
  do {
    if (Port2.available()) {
      if (position == 0) {
        Port2.write(13);
        Port2.write("                                           ");
        Port2.write(13);
      }
      char received = Port2.read();
      if (received == 8 && position > 0) {                            // Backspace
        if (((position - 1) == decimalLocation) && dotManualEntry) {  // If the dot has been manully entered(array shifted to decimal) then we need to shift the array back to highest place value
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          for (int i = 0; i < decimalShiftCount;) {
            //Shifts the 5 element array one place right-->
            for (byte shift = 0; shift < buffLength; shift++) {
              buff[shift] = buff[shift + 1];
              if (shift == (buffLength - 1)) {
                buff[buffLength] = 0;
              }
            }
            decimalShiftCount--;
            position--;
          }
          dotManualEntry = false;
          dot = false;
        } else if (((position - 1) == decimalLocation) && dot) {  // If deleting the dot that is automatically placed
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          dot = false;
        } else {
          if (inputERR) {  // If the error message is displayed delete it
            Port2.write("                             ");
            for (int i = 0; i < 29; i++) {
              Port2.write(8);
            }
            inputERR = false;
          }
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          position--;
          buff[position] = 0;
        }
      } else if (received == '.' && !dot) {  // Dot is entered before formatting calls for
        dotManualEntry = true;
        dot = true;
        Port2.write(received);
        for (int i = 0; i <= (decimalLocation - position);) {
          //Shifts the 5 element array one place right-->
          for (byte shift = buffLength; shift > 0; shift--) {
            buff[shift] = buff[shift - 1];
            if (shift == 1) {
              buff[0] = 0;
            }
          }
          decimalShiftCount++;
          position++;
        }
      } else if (received == 27) {  //ESC received
        ESC = true;
        break;
      } else if (('0' <= received && received <= '9') && position < buffLength) {  //Receiving proper characters
        if ((position == (decimalLocation + 1)) && !dot) {                         // If the user didn't manually put in the decimal point place it for them
          Port2.write('.');
          dot = true;
        }
        Port2.write(received);
        buff[position] = received - '0';
        position++;
      } else if (received == 13) {  // Receiving Enter
        inputERR = false;
        if (position == 0) {  // Do nothing with no input
          ;
        } else if (dot) {
          confirm = true;
        } else if (position == buffLength) {
          Port2.write("                             ");
          confirm = true;
        } else {  // Correct sigfigs if enter is pressed early and no decimal point was entered
          for (int i = 0; i <= (decimalLocation - position); i++) {
            //Shifts the 5 element array one place right-->
            for (byte shift = buffLength; shift > 0; shift--) {
              buff[shift] = buff[shift - 1];
              if (shift == 1) {
                buff[0] = 0;
              }
            }
          }
          confirm = true;
        }
      } else if (position >= buffLength) {  // If the array has been filled do not accept more values, then promt the user to submit entry
        Port2.write(" Press enter to confirm value");
        for (int i = 0; i < 29; i++) {
          Port2.write(8);
        }
        inputERR = true;
      } else {  // Deal with any characters that arn't accepted
        Port2.write(13);
        Port2.write("Please input a number between 0.000-9.999");
        for (int i = 0; i < buffLength; i++) {
          buff[i] = 0;
        }
        inputERR = true;
        break;
      }
    }
  } while (!confirm);
  if (inputERR) {
    SetScaleFactor();
  } else if (ESC) {
    newline();
    Port2.write("Input cancelled, Returning to main menu!");
    newline();
    Port2.write("The current value is: ");
    Port2.print(scaleFactor, 4);
    return scaleFactor;
  } else {
    for (int i = 0; i < buffLength; i++) {
      factor += buff[i] * (pow(10, (decimalLocation - i)));
    }
    newline();
    newline();
    Port2.write("Scale Factor Stored!");
    Port2.write(' ');
    if ((position - 1) > decimalLocation) {
      decimalPrint = ((position - 1) - decimalLocation);
    } else {
      decimalPrint = 0;
    }
    Port2.print(factor, decimalPrint);
    return factor;
  }
}

// Max payout*****************************************************************
float SetMaxPayout() {
  byte buff[] = { 0, 0, 0, 0, 0 };  // number array for storing inputted values
  byte buffLength = 5;
  byte position = 0;
  byte decimalLocation = 3;
  byte decimalShiftCount = 0;
  byte decimalPrint;
  bool step = false;
  bool inputERR = false;
  bool dot = false;
  bool confirm = false;
  bool ESC = false;
  bool dotManualEntry;
  float maxLimit = 0;
  do {
    if (Port2.available()) {  // If there is data on the serial port
      if (position == 0) {    // If we're in the first position clear the line
        Port2.write(13);
        Port2.write("                                           ");
        Port2.write(13);
      }
      char received = Port2.read();                                   // Read the data on the port
      if (received == 8 && position > 0) {                            // Backspace
        if (((position - 1) == decimalLocation) && dotManualEntry) {  // If the dot has been manully entered(array shifted to decimal) then we need to shift the array back to highest place value
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          for (int i = 0; i < decimalShiftCount;) {  // Loops as many times as the array was shifted to the right
            //Shifts the 5 element array one place left <--
            for (byte shift = 0; shift < buffLength; shift++) {
              buff[shift] = buff[shift + 1];
              if (shift == (buffLength - 1)) {
                buff[buffLength] = 0;
              }
            }
            decimalShiftCount--;
            position--;
          }
          dotManualEntry = false;  // Clears dot logic because we "removed" the dot with the code above
          dot = false;
        } else if (((position - 1) == decimalLocation) && dot) {  // If deleting the dot that is automatically placed
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          dot = false;
        } else {           // If we're not deleting a dot position
          if (inputERR) {  // If the error message is displayed delete it
            Port2.write("                             ");
            for (int i = 0; i < 29; i++) {
              Port2.write(8);
            }
            inputERR = false;
          }
          // Delete a number user input
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          position--;
          buff[position] = 0;
        }
      } else if (received == '.' && !dot) {  // Dot is entered before formatting calls for
        dotManualEntry = true;
        dot = true;
        Port2.write(received);
        for (int i = 0; i <= (decimalLocation - position);) {  // shift the current position right until it is beside the decimal point
          //Shifts the 5 element array one place right-->
          for (byte shift = buffLength; shift > 0; shift--) {
            buff[shift] = buff[shift - 1];
            if (shift == 1) {
              buff[0] = 0;
            }
          }
          decimalShiftCount++;
          position++;
        }
      } else if (received == 27) {  //ESC received
        ESC = true;
        break;                                                                     // Leave the while
      } else if (('0' <= received && received <= '9') && position < buffLength) {  //Receiving proper characters
        if ((position == (decimalLocation + 1)) && !dot) {                         // If the user didn't manually put in the decimal point place it for them
          Port2.write('.');
          dot = true;
        }
        Port2.write(received);
        buff[position] = received - '0';  // Convert the char number to int number for math later on
        position++;
      } else if (received == 13) {  // Receiving Enter
        inputERR = false;
        if (position == 0) {  // Do nothing with no input
          ;
        } else if (dot) {
          confirm = true;
        } else if (position == buffLength) {
          Port2.write("                             ");  // Clear the error message if it was there before
          confirm = true;
        } else {  // Correct sigfigs if enter is pressed early and no decimal point was entered
          for (int i = 0; i <= (decimalLocation - position); i++) {
            //Shifts the 5 element array one place right-->
            for (byte shift = buffLength; shift > 0; shift--) {
              buff[shift] = buff[shift - 1];
              if (shift == 1) {
                buff[0] = 0;
              }
            }
          }
          confirm = true;  // Leave the while
        }
      } else if (position >= buffLength) {  // If the array has been filled do not accept more values, then promt the user to submit entry
        Port2.write(" Press enter to confirm value");
        for (int i = 0; i < 29; i++) {  // Put the cursor back at the begining of the error message to prepare to write it again
          Port2.write(8);
        }
        inputERR = true;
      } else {  // Deal with any characters that arn't accepted
        Port2.write(13);
        Port2.write("Please input a number between 0000.0-9999.9");
        for (int i = 0; i < buffLength; i++) {
          buff[i] = 0;
        }
        inputERR = true;
        break;  // Leave the while
      }
    }
  } while (!confirm);
  if (inputERR) {  // Run again if issues occured
    SetMaxPayout();
  } else if (ESC) {  // Cancel input if esc is pressed
    newline();
    Port2.write("Input cancelled!");
    newline();
    Port2.write("The current value is: ");  // Let the user know what the current stored value is
    Port2.print(maxCablePayedOut, 1);
    Port2.print("m");
    return maxCablePayedOut;                // Return the stored max limit value
  } else {                                  // If the inputed data was good and ready to be stored
    for (int i = 0; i < buffLength; i++) {  // Set the proper sigfigs for the number
                                            // if the inputted number is 1.45 then the calculation would be
                                            /*            1     *  10^0
                + 4     *  10^-1
                + 5     *  10^-2
                + 0     *  10^-3
                = 1.4500        */
      maxLimit += buff[i] * (pow(10, (decimalLocation - i)));
    }
    newline();
    newline();
    Port2.write("MAX Payout Limit Stored!");
    Port2.write(' ');
    if ((position - 1) > decimalLocation) {               // Because of array increment madness (position - 1) is required to get location/position matching
      decimalPrint = ((position - 1) - decimalLocation);  // Set how many place values past the decimal need to be displayed
    } else {
      decimalPrint = 0;
    }
    Port2.print(maxLimit, decimalPrint);  // Print (value, number of places past the decimal)
    Port2.print("m");
    return maxLimit;
  }
}

// Min payout *****************************************************************
float SetMinPayout() {
  byte buff[] = { 0, 0, 0, 0, 0 };  // number array for storing inputted values
  byte buffLength = 5;
  byte position = 0;
  byte decimalLocation = 3;
  byte decimalShiftCount = 0;
  byte decimalPrint;
  bool minErr = true;
  bool step = false;
  bool inputERR = false;
  bool dot = false;
  bool confirm = false;
  bool ESC = false;
  bool dotManualEntry;
  float minLimit = 0;
  do {
    if (Port2.available()) {  // If there is data on the serial port
      if (position == 0) {    // If we're in the first position clear the line
        Port2.write(13);
        Port2.write("                                           ");
        Port2.write(13);
      }
      char received = Port2.read();                                   // Read the data on the port
      if (received == 8 && position > 0) {                            // Backspace
        if (((position - 1) == decimalLocation) && dotManualEntry) {  // If the dot has been manully entered(array shifted to decimal) then we need to shift the array back to highest place value
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          for (int i = 0; i < decimalShiftCount;) {  // Loops as many times as the array was shifted to the right
            //Shifts the 5 element array one place left <--
            for (byte shift = 0; shift < buffLength; shift++) {
              buff[shift] = buff[shift + 1];
              if (shift == (buffLength - 1)) {
                buff[buffLength] = 0;
              }
            }
            decimalShiftCount--;
            position--;
          }
          dotManualEntry = false;  // Clears dot logic because we "removed" the dot with the code above
          dot = false;
        } else if (((position - 1) == decimalLocation) && dot) {  // If deleting the dot that is automatically placed
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          dot = false;
        } else {           // If we're not deleting a dot position
          if (inputERR) {  // If the error message is displayed delete it
            Port2.write("                             ");
            for (int i = 0; i < 29; i++) {
              Port2.write(8);
            }
            inputERR = false;
          }
          // Delete a number user input
          Port2.write(8);
          Port2.write(' ');
          Port2.write(8);
          position--;
          buff[position] = 0;
        }
        // Limit error checking
        for (int i = 0; i <= decimalLocation; i++) {  // Read the values of the array before the decimal point
          if (buff[i]) {                              // If any are non-zero then the limit is large enough
            minErr = false;
            break;
          } else {  // If all are zero the limit might be to low
            minErr = true;
          }
        }
      } else if (received == '.' && !dot) {  // Dot is entered before formatting calls for
        dotManualEntry = true;
        dot = true;
        Port2.write(received);
        for (int i = 0; i <= (decimalLocation - position);) {  // shift the current position right until it is beside the decimal point
          //Shifts the 5 element array one place right-->
          for (byte shift = buffLength; shift > 0; shift--) {
            buff[shift] = buff[shift - 1];
            if (shift == 1) {
              buff[0] = 0;
            }
          }
          decimalShiftCount++;
          position++;
        }
      } else if (received == 27) {  //ESC received
        ESC = true;
        break;                      // Leave the while
      } else if (received == 13) {  // Receiving Enter
        inputERR = false;
        if (position == 0) {  // Do nothing with no input
          ;
        } else if (dot) {
          confirm = true;
        } else if (position == buffLength) {
          Port2.write("                             ");  // Clear the error message if it was there before
          confirm = true;
        } else {  // Correct sigfigs if enter is pressed early and no decimal point was entered
          for (int i = 0; i <= (decimalLocation - position); i++) {
            //Shifts the 5 element array one place right-->
            for (byte shift = buffLength; shift > 0; shift--) {
              buff[shift] = buff[shift - 1];
              if (shift == 1) {
                buff[0] = 0;
              }
            }
          }
          confirm = true;  // Leave the while
        }
      } else if (position == (decimalLocation + 1) && ('0' <= received && received <= minLimitLimit) && minErr) {  // The value trying to be inputted is to low
        Port2.write(13);
        Port2.write("Please input a number greater than 0.0");  //minLimitLimitSet
        for (int i = 0; i < buffLength; i++) {
          buff[i] = 0;
        }
        inputERR = true;
        break;                                                                     // Leave the while
      } else if (('0' <= received && received <= '9') && position < buffLength) {  //Receiving proper characters
        if ((position == (decimalLocation + 1)) && !dot) {                         // If the user didn't manually put in the decimal point place it for them
          Port2.write('.');
          dot = true;
        }
        Port2.write(received);
        buff[position] = received - '0';  // Convert the char number to int number for math later on
        position++;
        // Limit error checking
        for (int i = 0; i <= decimalLocation; i++) {  // Read the values before the decimal point
          if (buff[i]) {                              // If any are non-zero then everything is all good
            minErr = false;
            break;
          } else {  // If there is no non-zero value then the inputted value might be to low
            minErr = true;
          }
        }
      } else if (position >= buffLength) {  // If the array has been filled do not accept more values, then promt the user to submit entry
        Port2.write(" Press enter to confirm value");
        for (int i = 0; i < 29; i++) {  // Put the cursor back at the begining of the error message to prepare to write it again
          Port2.write(8);
        }
        inputERR = true;
      } else {  // Deal with any characters that arn't accepted
        Port2.write(13);
        Port2.write("Please input a number between 0000.0-9999.9");  //minLimitLimitSet
        for (int i = 0; i < buffLength; i++) {
          buff[i] = 0;
        }
        inputERR = true;
        break;  // Leave the while
      }
    }
  } while (!confirm);
  if (inputERR) {  // Run again if issues occured
    SetMinPayout();
  } else if (ESC) {  // Cancel input if esc is pressed
    newline();
    Port2.write("Input cancelled!");
    newline();
    Port2.write("The current value is: ");  // Let the user know what the current stored value is
    Port2.print(minCablePayedOut, 1);
    Port2.print("m");
    return minCablePayedOut;                // Return the stored max limit value
  } else {                                  // If the inputed data was good and ready to be stored
    for (int i = 0; i < buffLength; i++) {  // Set the proper sigfigs for the number
      minLimit += buff[i] * (pow(10, (decimalLocation - i)));
    }
    newline();
    newline();
    Port2.write("MIN Payout Limit Stored!");
    Port2.write(' ');
    if ((position - 1) > decimalLocation) {               // Because of array increment madness (position - 1) is required to get location/position matching
      decimalPrint = ((position - 1) - decimalLocation);  // Set how many place values past the decimal need to be displayed
    } else {
      decimalPrint = 0;
    }
    Port2.print(minLimit, decimalPrint);  // Print (value, number of places past the decimal)
    Port2.print("m");
    return minLimit;
  }
}

float SetNMEAPeriod() {
  byte buff[] = { 0, 0, 0, 0, 0 };
  byte buffLength = 5;
  byte position = 0;
  byte decimalLocation = 3;
  byte decimalShiftCount = 0;
  byte decimalPrint;
  bool step = false;
  bool inputERR = false;
  bool dot = false;
  bool confirm = false;
  bool ESC = false;
  bool dotManualEntry;
  float period = 0;
  do {
    if (Serial2.available()) {
      if (position == 0) {
        Serial2.write(13);
        Serial2.write("                                           ");
        Serial2.write(13);
      }
      char received = Serial2.read();
      if (received == 8 && position > 0) {                            // Backspace
        if (((position - 1) == decimalLocation) && dotManualEntry) {  // If the dot has been manully entered(array shifted to decimal) then we need to shift the array back to highest place value
          Serial2.write(8);
          Serial2.write(' ');
          Serial2.write(8);
          for (int i = 0; i < decimalShiftCount;) {
            //Shifts the 5 element array one place right-->
            for (byte shift = 0; shift < buffLength; shift++) {
              buff[shift] = buff[shift + 1];
              if (shift == (buffLength - 1)) {
                buff[buffLength] = 0;
              }
            }
            decimalShiftCount--;
            position--;
          }
          dotManualEntry = false;
          dot = false;
        } else if (((position - 1) == decimalLocation) && dot) {  // If deleting the dot that is automatically placed
          Serial2.write(8);
          Serial2.write(' ');
          Serial2.write(8);
          dot = false;
        } else {
          if (inputERR) {  // If the error message is displayed delete it
            Serial2.write("                             ");
            for (int i = 0; i < 29; i++) {
              Serial2.write(8);
            }
            inputERR = false;
          }
          Serial2.write(8);
          Serial2.write(' ');
          Serial2.write(8);
          position--;
          buff[position] = 0;
        }
      } else if (received == '.' && !dot) {  // Dot is entered before formatting calls for
        dotManualEntry = true;
        dot = true;
        Serial2.write(received);
        for (int i = 0; i <= (decimalLocation - position);) {
          //Shifts the 5 element array one place right-->
          for (byte shift = buffLength; shift > 0; shift--) {
            buff[shift] = buff[shift - 1];
            if (shift == 1) {
              buff[0] = 0;
            }
          }
          decimalShiftCount++;
          position++;
        }
      } else if (received == 27) {  //ESC received
        ESC = true;
        break;
      } else if (('0' <= received && received <= '9') && position < buffLength) {  //Receiving proper characters
        if ((position == (decimalLocation + 1)) && !dot) {                         // If the user didn't manually put in the decimal point place it for them
          Serial2.write('.');
          dot = true;
        }
        Serial2.write(received);
        buff[position] = received - '0';
        position++;
      } else if (received == 13) {  // Receiving Enter
        inputERR = false;
        if (position == 0) {  // Do nothing with no input
          ;
        } else if (dot) {
          confirm = true;
        } else if (position == buffLength) {
          Serial2.write("                             ");
          confirm = true;
        } else {  // Correct sigfigs if enter is pressed early and no decimal point was entered
          for (int i = 0; i <= (decimalLocation - position); i++) {
            //Shifts the 5 element array one place right-->
            for (byte shift = buffLength; shift > 0; shift--) {
              buff[shift] = buff[shift - 1];
              if (shift == 1) {
                buff[0] = 0;
              }
            }
          }
          confirm = true;
        }
      } else if (position >= buffLength) {  // If the array has been filled do not accept more values, then promt the user to submit entry
        Serial2.write(" Press enter to confirm value");
        for (int i = 0; i < 29; i++) {
          Serial2.write(8);
        }
        inputERR = true;
      } else {  // Deal with any characters that arn't accepted
        Serial2.write(13);
        Serial2.write("Please input a number between 0000.0-9999.9");
        for (int i = 0; i < buffLength; i++) {
          buff[i] = 0;
        }
        inputERR = true;
        break;
      }
    }
  } while (!confirm);
  if (inputERR) {
    SetNMEAPeriod();
  } else if (ESC) {
    newline();
    Serial2.write("Input cancelled, Returning to main menu!");
    newline();
    Serial2.write("The current value is: ");
    tempVal = NMEAPeriod / 1000.0;
    Serial2.print(tempVal, 1);
    return NMEAPeriod;
  } else {
    for (int i = 0; i < buffLength; i++) {
      period += buff[i] * (pow(10, (decimalLocation - i)));
    }
    newline();
    newline();
    Serial2.write("NMEA Period Stored!");
    Serial2.write(' ');
    if ((position - 1) > decimalLocation) {
      decimalPrint = ((position - 1) - decimalLocation);
    } else {
      decimalPrint = 0;
    }
    Serial2.print(period, decimalPrint);
    Serial2.write(" s");
    return period * 1000;
  }
}

//-----------------------------------------------------------------LCD Menu Functions-----------------------------------------------------------------
// Main menu *****************************************************************
void MainMenu() {
  int menuLength = 6;
  menuPosition = 1;
  ESC = false;
  clearScreen();
  updateMenu = true;
  // Debounce variables (local to the function)
  unsigned long lastDebounceTimeUp = 0;
  unsigned long lastDebounceTimeDown = 0;
  const unsigned long debounceDelay = 50;  // Debounce delay in milliseconds

  int previousPressUp = HIGH, previousPressDown = HIGH;  // Assuming buttons are active low (pressed = LOW)
  bool debouncedUp = false, debouncedDown = false;       // Debounced state flags

  while (!ESC) {
    previousMenuPosition = menuPosition;

    // Check if Up button is pressed (goes up the menu)
    int currentPressUp = digitalRead(rockerUpPin);
    if (currentPressUp != previousPressUp) {  // Button state changed
      lastDebounceTimeUp = millis();          // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeUp) > debounceDelay) {
      if (currentPressUp == LOW && !debouncedUp) {                           // Button is pressed and not already debounced
        menuPosition = (menuPosition <= 1) ? menuLength : menuPosition - 1;  // Move up or wrap around
        updateMenu = true;
        debouncedUp = true;  // Mark as debounced
      } else if (currentPressUp == HIGH) {
        debouncedUp = false;  // Reset debounced flag when button is released
      }
    }
    previousPressUp = currentPressUp;  // Store the state

    // Check if Down button is pressed (goes down the menu)
    int currentPressDown = digitalRead(rockerDownPin);
    if (currentPressDown != previousPressDown) {  // Button state changed
      lastDebounceTimeDown = millis();            // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeDown) > debounceDelay) {
      if (currentPressDown == LOW && !debouncedDown) {                       // Button is pressed and not already debounced
        menuPosition = (menuPosition >= menuLength) ? 1 : menuPosition + 1;  // Move down or wrap around
        updateMenu = true;
        debouncedDown = true;  // Mark as debounced
      } else if (currentPressDown == HIGH) {
        debouncedDown = false;  // Reset debounced flag when button is released
      }
    }
    previousPressDown = currentPressDown;  // Store the state

    if ((previousMenuPosition == 4 && menuPosition == 3) || (previousMenuPosition == menuLength && menuPosition == 1) || (previousMenuPosition == 3 && menuPosition == 4) || (previousMenuPosition == 1 && menuPosition == menuLength)) {
      refreshLCD = true;  // Checking to see if the menu has scrolled
    }
    if (updateMenu) {
      if (refreshLCD) {
        clearScreen();
        refreshLCD = false;
      }
      if (menuPosition < 4) {
        setCursor(0x04);
        Serial1.print("-Main Menu-");
        setCursor(0x41);
        Serial1.print("Toggle Offset:");
        setCursor(0x40 + 17);
        if (offsetActive) {
          Serial1.print(" ON");
        } else {
          Serial1.print("OFF");
        }
        setCursor(0x15);
        Serial1.print("Ethernet Info");
        setCursor(0x55);
        Serial1.print("Cable Settings");
      } else if (menuPosition > 3) {
        setCursor(0x01);
        Serial1.print("Drum Settings");
        setCursor(0x41);
        Serial1.print("Set Units:");
        setCursor(0x40 + 16);
        if (metric) {
          Serial1.print(" m");
        } else if (imperial) {
          Serial1.print("ft");
        }
        if (seconds) {
          Serial1.print("/s");
        } else if (minutes) {
          Serial1.print("/m");
        }
        setCursor(0x15);
        Serial1.print("NMEA Period: ");
        LCDPrintSeconds(NMEAPeriod, 0x14);
      }
      DrawCursor();
      updateMenu = false;
    }
    if (selectPressed) {
      switch (menuPosition) {
        case 1:
          LCDToggleOffset(1);
          if (sdCard) {
            drumFile = SD.open("DRMCONST.TXT", (O_READ | O_WRITE));
            drumFile.find("offset:");
            drumFile.print(offset, 1);
            drumFile.print(',');
            drumFile.close();
          }
          break;
        case 2:
          LCDEthernetInfo(2);
          break;
        case 3:
          LCDCableSettings(3);
          break;
        case 4:
          LCDDrumSettings(4);
          break;
        case 5:
          LCDSetUnits(5);
          if (sdCard) {
            drumFile = SD.open("DRMCONST.TXT", (O_READ | O_WRITE));
            drumFile.find("units:");
            drumFile.position();
            if (metric) {
              drumFile.print('M');
            } else if (imperial) {
              drumFile.print('I');
            }
            if (seconds) {
              drumFile.print('s');
            } else if (minutes) {
              drumFile.print('m');
            }
            drumFile.print(",");
            drumFile.close();
          }
          updateLimits = true;
          break;
        case 6:
          NMEAPeriod = LCDSetNMEAPeriod(0x14);
          if (sdCard) {
            tempVal = NMEAPeriod / 1000.0;
            drumFile = SD.open("DRMCONST.TXT", (O_READ | O_WRITE));
            drumFile.find("NMEAPeriod:");
            drumFile.position();
            drumFile.print(tempVal, 1);
            drumFile.print(",");
            drumFile.close();
          }
          break;
      }
      selectPressed = false;
    }
    previousBackPress = backPress;
    backPress = !digitalRead(BackButton);  // Inverting reading because of active low button
    if (backPress && (backPress != previousBackPress)) {
      ESC = true;
      delay(50);
    }
  }
}

// Cursor *****************************************************************
void DrawCursor() {
  //Clear display's ">" parts
  setCursor(0x0);      //1st line, 1st block
  Serial1.print(" ");  //erase by printing a space
  setCursor(0x40);
  Serial1.print(" ");
  setCursor(0x14);
  Serial1.print(" ");
  setCursor(0x54);
  Serial1.print(" ");
  //Place cursor to the new position
  switch (menuPosition)  //this checks the value of the menu
  {
    case 0:
    case 4:
      setCursor(0x0);  //1st line, 1st block
      Serial1.print(">");
      break;
    //-------------------------------
    case 1:
    case 5:
      setCursor(0x40);  //2nd line, 1st block
      Serial1.print(">");
      break;
    //-------------------------------
    case 2:
    case 6:
      setCursor(0x14);  //3rd line, 1st block
      Serial1.print(">");
      break;
    //-------------------------------
    case 3:
    case 7:
      setCursor(0x54);  //4th line, 1st block
      Serial1.print(">");
      break;
  }
}

// Toggle offset *****************************************************************
void LCDToggleOffset(int menuReturn) {
  selectPressed = false;
  offsetActive = !offsetActive;
  if (offsetActive) {
    offset = cablePayout;
  } else {
    offset = 0;
  }
  updateMenu = true;
  menuPosition = menuReturn;
}

// Ethernet afro *****************************************************************
void LCDEthernetInfo(int menuReturn) {
  selectPressed = false;
  ESC = false;
  clearScreen();
  setCursor(0x02);
  Serial1.print("-Ethernet Info-");
  setCursor(0x40);
  if ((Ethernet.localIP() == error0IP) || (Ethernet.localIP() == error1IP)) {
    Serial1.print("No IP Connection");
  } else {
    Serial1.print(Ethernet.localIP());
  }
  setCursor(0x14);
  if (!manualState) {
    Serial1.print("Wireless Control");
  } else {
    Serial1.print("Local Mode");
  }
  setCursor(0x54);
  if (DHCPError) {
    Serial1.print("DHCP Error!");
  }
  while (!ESC) {
    previousBackPress = backPress;
    backPress = !digitalRead(BackButton);
    if (backPress && (backPress != previousBackPress)) {
      ESC = true;
      delay(50);
    }
  }
  encoderCW = false;
  encoderCCW = false;
  ESC = false;
  updateMenu = true;
  refreshLCD = true;
  menuPosition = menuReturn;
}

void LCDCableSettings(int menuReturn) {
  int menuLength = 6;
  menuPosition = 1;
  selectPressed = false;
  clearScreen();
  updateMenu = true;
  ESC = false;
  // Debounce variables (local to the function)
  unsigned long lastDebounceTimeUp = 0;
  unsigned long lastDebounceTimeDown = 0;
  const unsigned long debounceDelay = 50;  // Debounce delay in milliseconds

  int previousPressUp = HIGH, previousPressDown = HIGH;  // Assuming buttons are active low (pressed = LOW)
  bool debouncedUp = false, debouncedDown = false;       // Debounced state flags

  while (!ESC) {
    previousMenuPosition = menuPosition;

    // Check if Up button is pressed (goes up the menu)
    int currentPressUp = digitalRead(rockerUpPin);
    if (currentPressUp != previousPressUp) {  // Button state changed
      lastDebounceTimeUp = millis();          // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeUp) > debounceDelay) {
      if (currentPressUp == LOW && !debouncedUp) {                           // Button is pressed and not already debounced
        menuPosition = (menuPosition <= 1) ? menuLength : menuPosition - 1;  // Move up or wrap around
        updateMenu = true;
        debouncedUp = true;  // Mark as debounced
      } else if (currentPressUp == HIGH) {
        debouncedUp = false;  // Reset debounced flag when button is released
      }
    }
    previousPressUp = currentPressUp;  // Store the state

    // Check if Down button is pressed (goes down the menu)
    int currentPressDown = digitalRead(rockerDownPin);
    if (currentPressDown != previousPressDown) {  // Button state changed
      lastDebounceTimeDown = millis();            // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeDown) > debounceDelay) {
      if (currentPressDown == LOW && !debouncedDown) {                       // Button is pressed and not already debounced
        menuPosition = (menuPosition >= menuLength) ? 1 : menuPosition + 1;  // Move down or wrap around
        updateMenu = true;
        debouncedDown = true;  // Mark as debounced
      } else if (currentPressDown == HIGH) {
        debouncedDown = false;  // Reset debounced flag when button is released
      }
    }
    previousPressDown = currentPressDown;  // Store the state
    if (updateMenu) {
      if ((previousMenuPosition == 4 && menuPosition == 3) || (previousMenuPosition == menuLength && menuPosition == 1) || (previousMenuPosition == 3 && menuPosition == 4) || (previousMenuPosition == 1 && menuPosition == menuLength)) {
        // If the user has scrolled to the next menu
        clearScreen();
      }
      if (menuPosition < 4) {
        setCursor(0x02);
        Serial1.print("-Cable Settings-");
        setCursor(0x41);
        Serial1.print("Max Limit:");
        ConvertMeters(maxCablePayedOut);
        LCDPrintUnitFloat(tempVal, 0x40);
        setCursor(0x15);
        Serial1.print("Min Limit:");
        ConvertMeters(minCablePayedOut);
        LCDPrintUnitFloat(tempVal, 0x14);
        setCursor(0x55);
        Serial1.print("Length:");
        ConvertMeters(cableLength);
        LCDPrintUnitFloat(tempVal, 0x54);
      } else if (menuPosition > 3) {
        setCursor(0x01);
        Serial1.print("Diameter:");
        ConvertCentimeters(cableDiameter);
        LCDPrintUnitFloatCentimeters(tempVal, 0x00);
        setCursor(0x41);
        Serial1.print("Scale:");
        LCDPrintUnitlessFloat(scaleFactor, 0x40);
        setCursor(0x15);
        Serial1.print("Stretch:");
        LCDPrintUnitlessFloat(stretchFactor, 0x14);
      }
      DrawCursor();
      updateMenu = false;
    }
    if (selectPressed) {
      switch (menuPosition) {
        case 1:
          maxCablePayedOut = LCDSetMaxLimit(0x40);
          if (sdCard) {
            drumFile = SD.open("DRMCONST.TXT", (O_READ | O_WRITE));
            drumFile.find("maxCablePayedOut:");
            drumFile.print(maxCablePayedOut, 1);
            drumFile.print(',');
            drumFile.close();
          }
          updateLimits = true;
          break;
        case 2:
          minCablePayedOut = LCDSetMinLimit(0x14);
          if (sdCard) {
            drumFile = SD.open("DRMCONST.TXT", (O_READ | O_WRITE));
            drumFile.find("minCablePayedOut:");
            drumFile.print(minCablePayedOut, 1);
            drumFile.print(',');
            drumFile.close();
          }
          updateLimits = true;
          break;
        case 3:
          cableLength = LCDSetCableLength(0x54);
          if (sdCard) {
            drumFile = SD.open("DRMCONST.TXT", (O_READ | O_WRITE));
            drumFile.find("cableLength:");
            drumFile.position();
            drumFile.print(cableLength, 1);
            drumFile.print(",");
            drumFile.close();
          }
          break;
        case 4:
          cableDiameter = LCDSetCableDiameter(0x00);
          if (sdCard) {
            drumFile = SD.open("drmConst.TXT", (O_READ | O_WRITE));
            drumFile.find("cableDiameter:");
            drumFile.print(cableDiameter, 5);
            drumFile.print(",");
            drumFile.close();
          }
          break;
        case 5:
          scaleFactor = LCDSetScaleFactor(0x40);
          if (sdCard) {
            drumFile = SD.open("drmConst.TXT", (O_READ | O_WRITE));
            drumFile.find("scaleFactor:");
            drumFile.position();
            drumFile.print(scaleFactor, 4);
            drumFile.print(",");
            drumFile.close();
          }
          break;
        case 6:
          stretchFactor = LCDSetStretchFactor(0x14);
          if (sdCard) {
            drumFile = SD.open("drmConst.TXT", (O_READ | O_WRITE));
            drumFile.find("stretchFactor:");
            drumFile.position();
            drumFile.print(stretchFactor, 4);
            drumFile.print(",");
            drumFile.close();
          }
          break;
      }
      selectPressed = false;
    }
    previousBackPress = backPress;
    backPress = !digitalRead(BackButton);
    if (backPress && (backPress != previousBackPress)) {
      ESC = true;
      delay(50);
    }
  }
  ESC = false;
  updateMenu = true;
  refreshLCD = true;
  menuPosition = menuReturn;
}

// Drum settings *****************************************************************
void LCDDrumSettings(int menuReturn) {
  int menuLength = 4;
  selectPressed = false;
  menuPosition = 1;
  clearScreen();
  updateMenu = true;
  ESC = false;
  // Debounce variables (local to the function)
  unsigned long lastDebounceTimeUp = 0;
  unsigned long lastDebounceTimeDown = 0;
  const unsigned long debounceDelay = 50;  // Debounce delay in milliseconds

  int previousPressUp = HIGH, previousPressDown = HIGH;  // Assuming buttons are active low (pressed = LOW)
  bool debouncedUp = false, debouncedDown = false;       // Debounced state flags

  while (!ESC) {
    previousMenuPosition = menuPosition;

    // Check if Up button is pressed (goes up the menu)
    int currentPressUp = digitalRead(rockerUpPin);
    if (currentPressUp != previousPressUp) {  // Button state changed
      lastDebounceTimeUp = millis();          // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeUp) > debounceDelay) {
      if (currentPressUp == LOW && !debouncedUp) {                           // Button is pressed and not already debounced
        menuPosition = (menuPosition <= 1) ? menuLength : menuPosition - 1;  // Move up or wrap around
        updateMenu = true;
        debouncedUp = true;  // Mark as debounced
      } else if (currentPressUp == HIGH) {
        debouncedUp = false;  // Reset debounced flag when button is released
      }
    }
    previousPressUp = currentPressUp;  // Store the state

    // Check if Down button is pressed (goes down the menu)
    int currentPressDown = digitalRead(rockerDownPin);
    if (currentPressDown != previousPressDown) {  // Button state changed
      lastDebounceTimeDown = millis();            // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeDown) > debounceDelay) {
      if (currentPressDown == LOW && !debouncedDown) {                       // Button is pressed and not already debounced
        menuPosition = (menuPosition >= menuLength) ? 1 : menuPosition + 1;  // Move down or wrap around
        updateMenu = true;
        debouncedDown = true;  // Mark as debounced
      } else if (currentPressDown == HIGH) {
        debouncedDown = false;  // Reset debounced flag when button is released
      }
    }
    previousPressDown = currentPressDown;  // Store the state
    if (updateMenu) {
      if ((previousMenuPosition == 4 && menuPosition == 3) || (previousMenuPosition == menuLength && menuPosition == 1) || (previousMenuPosition == 3 && menuPosition == 4) || (previousMenuPosition == 1 && menuPosition == menuLength)) {
        // If the user has scrolled to the next menu
        clearScreen();
      }
      if (menuPosition < 4) {
        setCursor(0x02);
        Serial1.print("-Drum Settings-");
        setCursor(0x41);
        Serial1.print("Diameter:");
        ConvertCentimeters(drumDiameter);
        LCDPrintUnitFloatCentimeters(tempVal, 0x40);
        setCursor(0x15);
        Serial1.print("Width:");
        ConvertCentimeters(drumWidth);
        LCDPrintUnitFloatCentimeters(tempVal, 0x14);
        setCursor(0x55);
        Serial1.print("Encoder Count:");
        LCDPrintUnitlessInt(drumEncRes, 0x54);
      } else if (menuPosition > 3) {
        setCursor(0x01);
        Serial1.print("Reset Encoder:");
      }
      DrawCursor();
      updateMenu = false;
    }
    if (selectPressed) {
      switch (menuPosition) {
        case 1:
          drumDiameter = LCDSetDrumDiameter(0x40);
          if (sdCard) {
            drumFile = SD.open("drmConst.TXT", (O_READ | O_WRITE));
            drumFile.find("drumDiameter:");
            drumFile.position();
            drumFile.print(drumDiameter, 5);
            drumFile.print(",");
            drumFile.close();
          }
          break;
        case 2:
          drumWidth = LCDSetDrumWidth(0x14);
          if (sdCard) {
            drumFile = SD.open("drmConst.TXT", (O_READ | O_WRITE));
            drumFile.find("drumWidth:");
            drumFile.position();
            drumFile.print(drumWidth, 5);
            drumFile.print(",");
            drumFile.close();
          }
          break;
        case 3:
          drumEncRes = LCDSetDrumEncRes(0x54);
          if (sdCard) {
            drumFile = SD.open("drmConst.TXT", (O_READ | O_WRITE));
            drumFile.find("drumEncRes:");
            drumFile.position();
            drumFile.print(drumEncRes);
            drumFile.print(",");
            drumFile.close();
          }
          break;
        case 4:
          offsetActive = false;
          offset = 0;
          cableLength = 0;
          retainCount = 0;
          encoderPreviousCount = 0;
          HSC.CNT1.setPosition(0);
          setCursor(0x01);
          Serial1.print("Reset Encoder:    X");
          break;
      }
      selectPressed = false;
    }
    previousBackPress = backPress;
    backPress = !digitalRead(BackButton);
    if (backPress && (backPress != previousBackPress)) {
      ESC = true;
      delay(50);
    }
  }
  ESC = false;
  updateMenu = true;
  refreshLCD = true;
  menuPosition = menuReturn;
}

// Set units *****************************************************************
void LCDSetUnits(int menuReturn) {
  selectPressed = false;
  if (metric) {
    if (seconds) {
      minutes = true;
      seconds = false;
    } else if (minutes) {
      imperial = true;
      seconds = true;
      metric = false;
      minutes = false;
    }
  } else if (imperial) {
    if (seconds) {
      minutes = true;
      seconds = false;
    } else if (minutes) {
      metric = true;
      seconds = true;
      imperial = false;
      minutes = false;
    }
  } else {
    metric = true;
  }
  updateMenu = true;
  menuPosition = menuReturn;
}

// NMEA Period ******************************************************************
float LCDSetNMEAPeriod(int row) {
  byte buff[] = { 0, 0, 0, 0, 0 };
  byte buffLength = 5;
  int decimalLocation = 4;
  int temp;
  byte valuePosition = 0;
  bool updateCursor = true;  // New variable to handle cursor update
  selectPressed = false;
  bool confirm = false;
  float period = 0;

  // Debounce variables (local to the function)
  unsigned long lastDebounceTimeUp = 0;
  unsigned long lastDebounceTimeDown = 0;
  const unsigned long debounceDelay = 50;  // Debounce delay in milliseconds

  int previousPressUp = HIGH, previousPressDown = HIGH;  // Assuming buttons are active low (pressed = LOW)
  bool debouncedUp = false, debouncedDown = false;       // Debounce flags

  setCursor(row);
  Serial1.print("X");

  row += 13;

  blinkingCursorON();

  temp = NMEAPeriod / 100;  // Convert milliseconds to seconds
  // Only / by 100 instead of 1000 because the following code only works with integers
  // 100ms / 1000 = 0.1 which couldn't by the following code

  // The following code requires the inputted value is x10 higher than the
  // actual value because the buff array requires int's and decimalLocation
  // converts the final value to the proper float value

  for (int i = 4; i >= 0; i--) {
    buff[i] = temp % 10;  // Extract the least significant digit
    temp /= 10;           // Move to the next digit
  }

  setCursor(row);

  for (int i = 0; i <= 4; i++) {
    if (i == decimalLocation) {
      Serial1.print(".");
      Serial1.print(buff[i]);
    } else {
      Serial1.print(buff[i]);
    }
  }

  while (!confirm) {
    if (updateCursor) {  // Check the new variable to decide if we should update the cursor position
      if (valuePosition == decimalLocation) {
        setCursor(row + valuePosition + 1);
      } else {
        setCursor(row + valuePosition);
      }
      updateCursor = false;  // Reset the flag after updating cursor
    }

    // Check if Up button is pressed (increments the value at valuePosition)
    int currentPressUp = digitalRead(rockerUpPin);
    if (currentPressUp != previousPressUp) {  // Button state changed
      lastDebounceTimeUp = millis();          // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeUp) > debounceDelay) {
      if (currentPressUp == LOW && !debouncedUp) {  // Button is pressed and not already debounced
        if (buff[valuePosition] > 8) {
          buff[valuePosition] = 0;
        } else {
          buff[valuePosition]++;
        }
        Serial1.print(buff[valuePosition]);
        updateCursor = true;  // Set the flag to update cursor position
        debouncedUp = true;   // Mark as debounced
      } else if (currentPressUp == HIGH) {
        debouncedUp = false;  // Reset debounced flag when button is released
      }
    }
    previousPressUp = currentPressUp;  // Store the state

    // Check if Down button is pressed (decrements the value at valuePosition)
    int currentPressDown = digitalRead(rockerDownPin);
    if (currentPressDown != previousPressDown) {  // Button state changed
      lastDebounceTimeDown = millis();            // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeDown) > debounceDelay) {
      if (currentPressDown == LOW && !debouncedDown) {  // Button is pressed and not already debounced
        if (buff[valuePosition] < 1) {
          buff[valuePosition] = 9;
        } else {
          buff[valuePosition]--;
        }
        Serial1.print(buff[valuePosition]);
        updateCursor = true;   // Set the flag to update cursor position
        debouncedDown = true;  // Mark as debounced
      } else if (currentPressDown == HIGH) {
        debouncedDown = false;  // Reset debounced flag when button is released
      }
    }
    previousPressDown = currentPressDown;  // Store the state

    if (selectPressed) {
      valuePosition = (valuePosition + 1) % 5;  // This will loop valuePosition from 0 to 3
      selectPressed = false;
      updateCursor = true;  // Set the flag to update cursor position
    }

    previousBackPress = backPress;
    backPress = !digitalRead(BackButton);

    if (backPress && (backPress != previousBackPress)) {
      confirm = true;
      delay(50);
    } else {
      confirm = false;
    }
  }

  for (int i = 0; i < buffLength; i++) {
    period += buff[i] * (pow(10, ((decimalLocation - 1) - i)));
  }

  period = period * 1000;
  blinkingCursorOFF();
  updateMenu = true;

  return period;
}

// Drum diameter *****************************************************************
float LCDSetDrumDiameter(int row) {
  byte buff[] = { 0, 0, 0, 0, 0 };
  byte buffLength = 5;
  int decimalLocation = 2;
  int temp;
  byte valuePosition = 0;
  bool updateCursor = true;  // New variable to handle cursor update
  selectPressed = false;
  bool confirm = false;
  float diameter = 0;

  // Debounce variables (local to the function)
  unsigned long lastDebounceTimeUp = 0;
  unsigned long lastDebounceTimeDown = 0;
  const unsigned long debounceDelay = 50;  // Debounce delay in milliseconds

  int previousPressUp = HIGH, previousPressDown = HIGH;  // Assuming buttons are active low (pressed = LOW)
  bool debouncedUp = false, debouncedDown = false;       // Debounce flags

  setCursor(row);
  Serial1.print("X");

  if (imperial) {
    row += 12;
  } else {
    row += 12;
  }

  blinkingCursorON();

  if (imperial) {
    temp = drumDiameter * 100000 * 0.394;  // convert cm to in ask ed
  } else {
    temp = drumDiameter * 100000;  // Convert cable diameter to an integer
  }

  for (int i = 4; i >= 0; i--) {
    buff[i] = temp % 10;  // Extract the least significant digit
    temp /= 10;           // Move to the next digit
  }

  setCursor(row);

  for (int i = 0; i <= 4; i++) {
    if (i == decimalLocation) {
      Serial1.print(".");
      Serial1.print(buff[i]);
    } else {
      Serial1.print(buff[i]);
    }
  }

  while (!confirm) {
    if (updateCursor) {  // Check the new variable to decide if we should update the cursor position
      if (valuePosition >= decimalLocation) {
        setCursor(row + valuePosition + 1);
      } else {
        setCursor(row + valuePosition);
      }
      updateCursor = false;  // Reset the flag after updating cursor
    }

    // Check if Up button is pressed (increments the value at valuePosition)
    int currentPressUp = digitalRead(rockerUpPin);
    if (currentPressUp != previousPressUp) {  // Button state changed
      lastDebounceTimeUp = millis();          // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeUp) > debounceDelay) {
      if (currentPressUp == LOW && !debouncedUp) {  // Button is pressed and not already debounced
        if (buff[valuePosition] > 8) {
          buff[valuePosition] = 0;
        } else {
          buff[valuePosition]++;
        }
        Serial1.print(buff[valuePosition]);
        updateCursor = true;  // Set the flag to update cursor position
        debouncedUp = true;   // Mark as debounced
      } else if (currentPressUp == HIGH) {
        debouncedUp = false;  // Reset debounced flag when button is released
      }
    }
    previousPressUp = currentPressUp;  // Store the state

    // Check if Down button is pressed (decrements the value at valuePosition)
    int currentPressDown = digitalRead(rockerDownPin);
    if (currentPressDown != previousPressDown) {  // Button state changed
      lastDebounceTimeDown = millis();            // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeDown) > debounceDelay) {
      if (currentPressDown == LOW && !debouncedDown) {  // Button is pressed and not already debounced
        if (buff[valuePosition] < 1) {
          buff[valuePosition] = 9;
        } else {
          buff[valuePosition]--;
        }
        Serial1.print(buff[valuePosition]);
        updateCursor = true;   // Set the flag to update cursor position
        debouncedDown = true;  // Mark as debounced
      } else if (currentPressDown == HIGH) {
        debouncedDown = false;  // Reset debounced flag when button is released
      }
    }
    previousPressDown = currentPressDown;  // Store the state

    if (selectPressed) {
      valuePosition = (valuePosition + 1) % 5;  // This will loop valuePosition from 0 to 3
      selectPressed = false;
      updateCursor = true;
    }

    previousBackPress = backPress;
    backPress = !digitalRead(BackButton);

    if (backPress && (backPress != previousBackPress)) {
      confirm = true;
      delay(50);
    } else {
      confirm = false;
    }
  }

  for (int i = 0; i < buffLength; i++) {
    diameter += buff[i] * (pow(10, ((decimalLocation - 1) - i)));
  }

  blinkingCursorOFF();

  updateMenu = true;
  diameter *= 0.01;

  if (imperial) {
    diameter *= 2.5381;  // convert in back to cm
  }

  return diameter;
}

// Drum width *****************************************************************
float LCDSetDrumWidth(int row) {
  byte buff[] = { 0, 0, 0, 0, 0 };
  byte buffLength = 5;
  int decimalLocation = 2;
  int temp;
  byte valuePosition = 0;
  bool updateCursor = true;  // New variable to handle cursor update
  selectPressed = false;
  bool confirm = false;
  float width = 0;

  // Debounce variables (local to the function)
  unsigned long lastDebounceTimeUp = 0;
  unsigned long lastDebounceTimeDown = 0;
  const unsigned long debounceDelay = 50;  // Debounce delay in milliseconds

  int previousPressUp = HIGH, previousPressDown = HIGH;  // Assuming buttons are active low (pressed = LOW)
  bool debouncedUp = false, debouncedDown = false;       // Debounce flags

  setCursor(row);
  Serial1.print("X");

  if (imperial) {
    row += 12;
  } else {
    row += 12;
  }


  blinkingCursorON();

  if (imperial) {
    temp = drumWidth * 100000 * 0.394;  // convert cm to in ask ed
  } else {
    temp = drumWidth * 100000;  // Convert cable diameter to an integer
  }

  for (int i = 4; i >= 0; i--) {
    buff[i] = temp % 10;  // Extract the least significant digit
    temp /= 10;           // Move to the next digit
  }

  setCursor(row);

  for (int i = 0; i <= 4; i++) {
    if (i == decimalLocation) {
      Serial1.print(".");
      Serial1.print(buff[i]);
    } else {
      Serial1.print(buff[i]);
    }
  }

  while (!confirm) {
    if (updateCursor) {  // Check the new variable to decide if we should update the cursor position
      if (valuePosition >= decimalLocation) {
        setCursor(row + valuePosition + 1);
      } else {
        setCursor(row + valuePosition);
      }
      updateCursor = false;  // Reset the flag after updating cursor
    }

    // Check if Up button is pressed (increments the value at valuePosition)
    int currentPressUp = digitalRead(rockerUpPin);
    if (currentPressUp != previousPressUp) {  // Button state changed
      lastDebounceTimeUp = millis();          // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeUp) > debounceDelay) {
      if (currentPressUp == LOW && !debouncedUp) {  // Button is pressed and not already debounced
        if (buff[valuePosition] > 8) {
          buff[valuePosition] = 0;
        } else {
          buff[valuePosition]++;
        }
        Serial1.print(buff[valuePosition]);
        updateCursor = true;  // Set the flag to update cursor position
        debouncedUp = true;   // Mark as debounced
      } else if (currentPressUp == HIGH) {
        debouncedUp = false;  // Reset debounced flag when button is released
      }
    }
    previousPressUp = currentPressUp;  // Store the state

    // Check if Down button is pressed (decrements the value at valuePosition)
    int currentPressDown = digitalRead(rockerDownPin);
    if (currentPressDown != previousPressDown) {  // Button state changed
      lastDebounceTimeDown = millis();            // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeDown) > debounceDelay) {
      if (currentPressDown == LOW && !debouncedDown) {  // Button is pressed and not already debounced
        if (buff[valuePosition] < 1) {
          buff[valuePosition] = 9;
        } else {
          buff[valuePosition]--;
        }
        Serial1.print(buff[valuePosition]);
        updateCursor = true;   // Set the flag to update cursor position
        debouncedDown = true;  // Mark as debounced
      } else if (currentPressDown == HIGH) {
        debouncedDown = false;  // Reset debounced flag when button is released
      }
    }
    previousPressDown = currentPressDown;  // Store the state

    if (selectPressed) {
      valuePosition = (valuePosition + 1) % 5;  // This will loop valuePosition from 0 to 4
      selectPressed = false;
      updateCursor = true;  // Set the flag to update cursor position
    }

    previousBackPress = backPress;
    backPress = !digitalRead(BackButton);

    if (backPress && (backPress != previousBackPress)) {
      confirm = true;
      delay(50);
    } else {
      confirm = false;
    }
  }

  for (int i = 0; i < buffLength; i++) {
    width += buff[i] * (pow(10, ((decimalLocation - 1) - i)));
  }

  blinkingCursorOFF();

  updateMenu = true;

  width *= 0.01;

  if (imperial) {
    width *= 2.5381;  // convert in back to cm
  }
  return width;
}

// Drum res *****************************************************************
float LCDSetDrumEncRes(int row) {
  byte buff[] = { 0, 0, 0, 0 };
  byte buffLength = 4;
  int temp;
  byte valuePosition = 0;
  bool updateCursor = true;
  selectPressed = false;
  bool confirm = false;
  float count = 0;
  unsigned long lastDebounceTimeUp = 0;
  unsigned long lastDebounceTimeDown = 0;
  const unsigned long debounceDelay = 50;  // Debounce delay in milliseconds

  int previousPressUp = HIGH, previousPressDown = HIGH;  // Assuming buttons are active low (pressed = LOW)
  bool debouncedUp = false, debouncedDown = false;       // Debounce flags


  setCursor(row);
  Serial1.print("X");

  row += 16;
  setCursor(row);

  blinkingCursorON();


  temp = drumEncRes;  // Convert cable diameter to an integer

  for (int i = 3; i >= 0; i--) {
    buff[i] = temp % 10;  // Extract the least significant digit
    temp /= 10;           // Move to the next digit
  }


  for (int i = 0; i <= 3; i++) {
    Serial1.print(buff[i]);
  }

  while (!confirm) {
    if (updateCursor) {  // Check the new variable to decide if we should update the cursor position
      if (valuePosition >= buffLength) {
        setCursor(row + valuePosition + 1);
      } else {
        setCursor(row + valuePosition);
      }
      updateCursor = false;  // Reset the flag after updating cursor
    }

    // Check if Up button is pressed (increments the value at valuePosition)
    int currentPressUp = digitalRead(rockerUpPin);
    if (currentPressUp != previousPressUp) {  // Button state changed
      lastDebounceTimeUp = millis();          // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeUp) > debounceDelay) {
      if (currentPressUp == LOW && !debouncedUp) {  // Button is pressed and not already debounced
        if (buff[valuePosition] > 8) {
          buff[valuePosition] = 0;
        } else {
          buff[valuePosition]++;
        }
        Serial1.print(buff[valuePosition]);
        updateCursor = true;  // Set the flag to update cursor position
        debouncedUp = true;   // Mark as debounced
      } else if (currentPressUp == HIGH) {
        debouncedUp = false;  // Reset debounced flag when button is released
      }
    }
    previousPressUp = currentPressUp;  // Store the state

    // Check if Down button is pressed (decrements the value at valuePosition)
    int currentPressDown = digitalRead(rockerDownPin);
    if (currentPressDown != previousPressDown) {  // Button state changed
      lastDebounceTimeDown = millis();            // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeDown) > debounceDelay) {
      if (currentPressDown == LOW && !debouncedDown) {  // Button is pressed and not already debounced
        if (buff[valuePosition] < 1) {
          buff[valuePosition] = 9;
        } else {
          buff[valuePosition]--;
        }
        Serial1.print(buff[valuePosition]);
        updateCursor = true;   // Set the flag to update cursor position
        debouncedDown = true;  // Mark as debounced
      } else if (currentPressDown == HIGH) {
        debouncedDown = false;  // Reset debounced flag when button is released
      }
    }
    previousPressDown = currentPressDown;  // Store the state

    if (selectPressed) {
      valuePosition = (valuePosition + 1) % 4;  // This will loop valuePosition from 0 to 3
      updateCursor = true;
      selectPressed = false;
    }

    previousBackPress = backPress;
    backPress = !digitalRead(BackButton);

    if (backPress && (backPress != previousBackPress)) {
      confirm = true;
      delay(50);
    } else {
      confirm = false;
    }
  }

  for (int i = 0; i < buffLength; i++) {
    count += buff[i] * (pow(10, ((buffLength - 1) - i)));
  }

  blinkingCursorOFF();
  //underlineCursorOFF();

  updateMenu = true;

  return count;
}

// Cable length*****************************************************************
/**
*@brief Set the cable length through the LCD
*@param row Specify what row the cable length should be written on
*/
float LCDSetCableLength(int row) {
  byte buff[] = { 0, 0, 0, 0, 0 };
  byte buffLength = 5;
  int decimalLocation = 4;
  int temp;
  byte valuePosition = 0;
  bool updateCursor = true;  // New variable to handle cursor update
  selectPressed = false;
  bool confirm = false;
  float length = 0;

  // Debounce variables (local to the function)
  unsigned long lastDebounceTimeUp = 0;
  unsigned long lastDebounceTimeDown = 0;
  const unsigned long debounceDelay = 50;  // Debounce delay in milliseconds

  int previousPressUp = HIGH, previousPressDown = HIGH;  // Assuming buttons are active low (pressed = LOW)
  bool debouncedUp = false, debouncedDown = false;       // Debounce flags

  setCursor(row);
  Serial1.print("X");

  if (drumCountNegative == true) {
    drumDirectionNegative = true;
    if (sdCard) {
      drumFile = SD.open("DRMCONST.TXT", (O_READ | O_WRITE));
      drumFile.find("drumDirectionNegative:");
      drumFile.print("T");
      drumFile.print(',');
      drumFile.close();
    }
  } else if (drumCountNegative == false) {
    drumDirectionNegative = false;
    if (sdCard) {
      drumFile = SD.open("DRMCONST.TXT", (O_READ | O_WRITE));
      drumFile.find("drumDirectionNegative:");
      drumFile.print("F");
      drumFile.print(',');
      drumFile.close();
    }
  }
  if (imperial) {
    row += 12;
  } else {  // Metric
    row += 13;
  }

  blinkingCursorON();

  if (imperial) {
    temp = cableLength * 10 * 3.28084;  // convert cm to in ask ed
  } else {                              // Metric
    temp = cableLength * 10;            // Convert cable diameter to an integer
  }
  for (int i = 4; i >= 0; i--) {
    buff[i] = temp % 10;  // Extract the least significant digit
    temp /= 10;           // Move to the next digit
  }

  setCursor(row);

  for (int i = 0; i <= 4; i++) {
    if (i == decimalLocation) {
      Serial1.print(".");
      Serial1.print(buff[i]);
    } else {
      Serial1.print(buff[i]);
    }
  }

  while (!confirm) {
    if (updateCursor) {  // Check the new variable to decide if we should update the cursor position
      if (valuePosition >= decimalLocation) {
        setCursor(row + valuePosition + 1);
      } else {
        setCursor(row + valuePosition);
      }
      updateCursor = false;  // Reset the flag after updating cursor
    }

    // Check if Up button is pressed (increments the value at valuePosition)
    int currentPressUp = digitalRead(rockerUpPin);
    if (currentPressUp != previousPressUp) {  // Button state changed
      lastDebounceTimeUp = millis();          // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeUp) > debounceDelay) {
      if (currentPressUp == LOW && !debouncedUp) {  // Button is pressed and not already debounced
        if (buff[valuePosition] > 8) {
          buff[valuePosition] = 0;
        } else {
          buff[valuePosition]++;
        }
        Serial1.print(buff[valuePosition]);
        updateCursor = true;  // Set the flag to update cursor position
        debouncedUp = true;   // Mark as debounced
      } else if (currentPressUp == HIGH) {
        debouncedUp = false;  // Reset debounced flag when button is released
      }
    }
    previousPressUp = currentPressUp;  // Store the state

    // Check if Down button is pressed (decrements the value at valuePosition)
    int currentPressDown = digitalRead(rockerDownPin);
    if (currentPressDown != previousPressDown) {  // Button state changed
      lastDebounceTimeDown = millis();            // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeDown) > debounceDelay) {
      if (currentPressDown == LOW && !debouncedDown) {  // Button is pressed and not already debounced
        if (buff[valuePosition] < 1) {
          buff[valuePosition] = 9;
        } else {
          buff[valuePosition]--;
        }
        Serial1.print(buff[valuePosition]);
        updateCursor = true;   // Set the flag to update cursor position
        debouncedDown = true;  // Mark as debounced
      } else if (currentPressDown == HIGH) {
        debouncedDown = false;  // Reset debounced flag when button is released
      }
    }
    previousPressDown = currentPressDown;  // Store the state

    if (selectPressed) {
      valuePosition = (valuePosition + 1) % 5;  // This will loop valuePosition from 0 to 3
      selectPressed = false;
      updateCursor = true;  // Set the flag to update cursor position
    }

    previousBackPress = backPress;
    backPress = !digitalRead(BackButton);

    if (backPress && (backPress != previousBackPress)) {
      confirm = true;
      delay(50);
    } else {
      confirm = false;
    }
  }

  for (int i = 0; i < buffLength; i++) {
    length += buff[i] * (pow(10, ((decimalLocation - 1) - i)));
  }

  blinkingCursorOFF();
  updateMenu = true;

  if (imperial) {
    length *= 0.3048;  // convert ft back to meters
  }
  return length;
}

// Cable diameter *****************************************************************
float LCDSetCableDiameter(int row) {
  byte buff[] = { 0, 0, 0, 0, 0 };
  byte buffLength = 5;
  int decimalLocation = 2;
  int temp;
  byte valuePosition = 0;
  bool updateCursor = true;  // New variable to handle cursor update
  selectPressed = false;
  bool confirm = false;
  float diameter = 0;

  // Debounce variables (local to the function)
  unsigned long lastDebounceTimeUp = 0;
  unsigned long lastDebounceTimeDown = 0;
  const unsigned long debounceDelay = 50;  // Debounce delay in milliseconds

  int previousPressUp = HIGH, previousPressDown = HIGH;  // Assuming buttons are active low (pressed = LOW)
  bool debouncedUp = false, debouncedDown = false;       // Debounce flags


  setCursor(row);
  Serial1.print("X");

  if (imperial) {
    row += 12;
  } else {
    row += 12;
  }

  blinkingCursorON();

  if (imperial) {
    temp = cableDiameter * 100000 * 0.394;  // convert cm to in ask ed
  } else {
    temp = cableDiameter * 100000;  // Convert cable diameter to an integer
  }
  for (int i = 4; i >= 0; i--) {
    buff[i] = temp % 10;  // Extract the least significant digit
    temp /= 10;           // Move to the next digit
  }

  setCursor(row);

  for (int i = 0; i <= 4; i++) {
    if (i == decimalLocation) {
      Serial1.print(".");
      Serial1.print(buff[i]);
    } else {
      Serial1.print(buff[i]);
    }
  }

  while (!confirm) {
    if (updateCursor) {  // Check the new variable to decide if we should update the cursor position
      if (valuePosition >= decimalLocation) {
        setCursor(row + valuePosition + 1);
      } else {
        setCursor(row + valuePosition);
      }
      updateCursor = false;  // Reset the flag after updating cursor
    }

    // Check if Up button is pressed (increments the value at valuePosition)
    int currentPressUp = digitalRead(rockerUpPin);
    if (currentPressUp != previousPressUp) {  // Button state changed
      lastDebounceTimeUp = millis();          // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeUp) > debounceDelay) {
      if (currentPressUp == LOW && !debouncedUp) {  // Button is pressed and not already debounced
        if (buff[valuePosition] > 8) {
          buff[valuePosition] = 0;
        } else {
          buff[valuePosition]++;
        }
        Serial1.print(buff[valuePosition]);
        updateCursor = true;  // Set the flag to update cursor position
        debouncedUp = true;   // Mark as debounced
      } else if (currentPressUp == HIGH) {
        debouncedUp = false;  // Reset debounced flag when button is released
      }
    }
    previousPressUp = currentPressUp;  // Store the state

    // Check if Down button is pressed (decrements the value at valuePosition)
    int currentPressDown = digitalRead(rockerDownPin);
    if (currentPressDown != previousPressDown) {  // Button state changed
      lastDebounceTimeDown = millis();            // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeDown) > debounceDelay) {
      if (currentPressDown == LOW && !debouncedDown) {  // Button is pressed and not already debounced
        if (buff[valuePosition] < 1) {
          buff[valuePosition] = 9;
        } else {
          buff[valuePosition]--;
        }
        Serial1.print(buff[valuePosition]);
        updateCursor = true;   // Set the flag to update cursor position
        debouncedDown = true;  // Mark as debounced
      } else if (currentPressDown == HIGH) {
        debouncedDown = false;  // Reset debounced flag when button is released
      }
    }
    previousPressDown = currentPressDown;  // Store the state

    if (selectPressed) {
      valuePosition = (valuePosition + 1) % 5;  // This will loop valuePosition from 0 to 3
      selectPressed = false;
      updateCursor = true;  // Set the flag to update cursor position
    }

    previousBackPress = backPress;
    backPress = !digitalRead(BackButton);

    if (backPress && (backPress != previousBackPress)) {
      confirm = true;
      delay(50);
    } else {
      confirm = false;
    }
  }

  for (int i = 0; i < buffLength; i++) {
    diameter += buff[i] * (pow(10, ((decimalLocation - 1) - i)));
  }

  blinkingCursorOFF();

  updateMenu = true;
  diameter *= 0.01;

  if (imperial) {
    diameter *= 2.5381;  // convert in back to cm
  }

  return diameter;
}

// Max limit *****************************************************************
float LCDSetMaxLimit(int row) {
  byte buff[] = { 0, 0, 0, 0, 0 };
  byte buffLength = 5;
  int decimalLocation = 4;
  int temp;
  byte valuePosition = 0;
  bool updateCursor = true;  // New variable to handle cursor update
  selectPressed = false;
  bool confirm = false;
  float maxLimit = 0;

  // Debounce variables (local to the function)
  unsigned long lastDebounceTimeUp = 0;
  unsigned long lastDebounceTimeDown = 0;
  const unsigned long debounceDelay = 50;  // Debounce delay in milliseconds

  int previousPressUp = HIGH, previousPressDown = HIGH;  // Assuming buttons are active low (pressed = LOW)
  bool debouncedUp = false, debouncedDown = false;       // Debounce flags

  setCursor(row);
  Serial1.print("X");

  if (imperial) {
    row += 12;
  } else {
    row += 13;
  }

  blinkingCursorON();

  if (imperial) {
    temp = maxCablePayedOut * 10 * 3.28084;  // convert cm to in ask ed
  } else {
    temp = maxCablePayedOut * 10;  // Convert cable diameter to an integer
  }

  for (int i = 4; i >= 0; i--) {
    buff[i] = temp % 10;  // Extract the least significant digit
    temp /= 10;           // Move to the next digit
  }

  setCursor(row);

  for (int i = 0; i <= 4; i++) {
    if (i == decimalLocation) {
      Serial1.print(".");
      Serial1.print(buff[i]);
    } else {
      Serial1.print(buff[i]);
    }
  }

  while (!confirm) {
    if (updateCursor) {  // Check the new variable to decide if we should update the cursor position
      if (valuePosition >= decimalLocation) {
        setCursor(row + valuePosition + 1);
      } else {
        setCursor(row + valuePosition);
      }
      updateCursor = false;  // Reset the flag after updating cursor
    }

    // Check if Up button is pressed (increments the value at valuePosition)
    int currentPressUp = digitalRead(rockerUpPin);
    if (currentPressUp != previousPressUp) {  // Button state changed
      lastDebounceTimeUp = millis();          // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeUp) > debounceDelay) {
      if (currentPressUp == LOW && !debouncedUp) {  // Button is pressed and not already debounced
        if (buff[valuePosition] > 8) {
          buff[valuePosition] = 0;
        } else {
          buff[valuePosition]++;
        }
        Serial1.print(buff[valuePosition]);
        updateCursor = true;  // Set the flag to update cursor position
        debouncedUp = true;   // Mark as debounced
      } else if (currentPressUp == HIGH) {
        debouncedUp = false;  // Reset debounced flag when button is released
      }
    }
    previousPressUp = currentPressUp;  // Store the state

    // Check if Down button is pressed (decrements the value at valuePosition)
    int currentPressDown = digitalRead(rockerDownPin);
    if (currentPressDown != previousPressDown) {  // Button state changed
      lastDebounceTimeDown = millis();            // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeDown) > debounceDelay) {
      if (currentPressDown == LOW && !debouncedDown) {  // Button is pressed and not already debounced
        if (buff[valuePosition] < 1) {
          buff[valuePosition] = 9;
        } else {
          buff[valuePosition]--;
        }
        Serial1.print(buff[valuePosition]);
        updateCursor = true;   // Set the flag to update cursor position
        debouncedDown = true;  // Mark as debounced
      } else if (currentPressDown == HIGH) {
        debouncedDown = false;  // Reset debounced flag when button is released
      }
    }
    previousPressDown = currentPressDown;  // Store the state

    if (selectPressed) {
      valuePosition = (valuePosition + 1) % 5;  // This will loop valuePosition from 0 to 3
      selectPressed = false;
      updateCursor = true;  // Set the flag to update cursor position
    }

    previousBackPress = backPress;
    backPress = !digitalRead(BackButton);

    if (backPress && (backPress != previousBackPress)) {
      confirm = true;
      delay(50);
    } else {
      confirm = false;
    }
  }

  for (int i = 0; i < buffLength; i++) {
    maxLimit += buff[i] * (pow(10, ((decimalLocation - 1) - i)));
  }

  blinkingCursorOFF();
  //underlineCursorOFF();

  updateMenu = true;

  if (imperial) {
    maxLimit *= 0.3048;  // convert ft back to meters
  }

  return maxLimit;
}

// Min limit*****************************************************************
float LCDSetMinLimit(int row) {
  byte buff[] = { 0, 0, 0, 0, 0 };
  byte buffLength = 5;
  int decimalLocation = 4;
  int temp;
  byte valuePosition = 0;
  bool updateCursor = true;  // New variable to handle cursor update
  selectPressed = false;
  bool confirm = false;
  float minLimit = 0;

  // Debounce variables (local to the function)
  unsigned long lastDebounceTimeUp = 0;
  unsigned long lastDebounceTimeDown = 0;
  const unsigned long debounceDelay = 50;  // Debounce delay in milliseconds

  int previousPressUp = HIGH, previousPressDown = HIGH;  // Assuming buttons are active low (pressed = LOW)
  bool debouncedUp = false, debouncedDown = false;       // Debounce flags

  setCursor(row);
  Serial1.print("X");

  if (imperial) {
    row += 12;
  } else {
    row += 13;
  }

  blinkingCursorON();

  if (imperial) {
    temp = minCablePayedOut * 10 * 3.28084;  // convert cm to in ask ed
  } else {
    temp = minCablePayedOut * 10;  // Convert cable diameter to an integer
  }

  for (int i = 4; i >= 0; i--) {
    buff[i] = temp % 10;  // Extract the least significant digit
    temp /= 10;           // Move to the next digit
  }

  setCursor(row);

  for (int i = 0; i <= 4; i++) {
    if (i == decimalLocation) {
      Serial1.print(".");
      Serial1.print(buff[i]);
    } else {
      Serial1.print(buff[i]);
    }
  }

  while (!confirm) {
    if (updateCursor) {  // Check the new variable to decide if we should update the cursor position
      if (valuePosition >= decimalLocation) {
        setCursor(row + valuePosition + 1);
      } else {
        setCursor(row + valuePosition);
      }
      updateCursor = false;  // Reset the flag after updating cursor
    }

    // Check if Up button is pressed (increments the value at valuePosition)
    int currentPressUp = digitalRead(rockerUpPin);
    if (currentPressUp != previousPressUp) {  // Button state changed
      lastDebounceTimeUp = millis();          // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeUp) > debounceDelay) {
      if (currentPressUp == LOW && !debouncedUp) {  // Button is pressed and not already debounced
        if (buff[valuePosition] > 8) {
          buff[valuePosition] = 0;
        } else {
          buff[valuePosition]++;
        }
        Serial1.print(buff[valuePosition]);
        updateCursor = true;  // Set the flag to update cursor position
        debouncedUp = true;   // Mark as debounced
      } else if (currentPressUp == HIGH) {
        debouncedUp = false;  // Reset debounced flag when button is released
      }
    }
    previousPressUp = currentPressUp;  // Store the state

    // Check if Down button is pressed (decrements the value at valuePosition)
    int currentPressDown = digitalRead(rockerDownPin);
    if (currentPressDown != previousPressDown) {  // Button state changed
      lastDebounceTimeDown = millis();            // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeDown) > debounceDelay) {
      if (currentPressDown == LOW && !debouncedDown) {  // Button is pressed and not already debounced
        if (buff[valuePosition] < 1) {
          buff[valuePosition] = 9;
        } else {
          buff[valuePosition]--;
        }
        Serial1.print(buff[valuePosition]);
        updateCursor = true;   // Set the flag to update cursor position
        debouncedDown = true;  // Mark as debounced
      } else if (currentPressDown == HIGH) {
        debouncedDown = false;  // Reset debounced flag when button is released
      }
    }
    previousPressDown = currentPressDown;  // Store the state

    if (selectPressed) {
      valuePosition = (valuePosition + 1) % 5;  // This will loop valuePosition from 0 to 3
      selectPressed = false;
      updateCursor = true;  // Set the flag to update cursor position
    }

    previousBackPress = backPress;
    backPress = !digitalRead(BackButton);

    if (backPress && (backPress != previousBackPress)) {
      confirm = true;
      delay(50);
    } else {
      confirm = false;
    }
  }

  for (int i = 0; i < buffLength; i++) {
    minLimit += buff[i] * (pow(10, ((decimalLocation - 1) - i)));
  }

  blinkingCursorOFF();
  //underlineCursorOFF();

  updateMenu = true;

  if (imperial) {
    minLimit *= 0.3048;  // convert ft back to meters
  }

  return minLimit;
}

// Scale factor *****************************************************************
/**
*@brief Set which row of the LCD the value will be displayed on
*@param row A value from 0 - 3
*/
// Scale factor *****************************************************************
float LCDSetScaleFactor(int row) {
  byte buff[] = { 0, 0, 0, 0, 0 };  // Array to store individual digits of the scale factor
  byte buffLength = 5;              // Length of the buffer array
  int decimalLocation = 1;          // Position of the decimal point
  int temp;
  byte valuePosition = 0;    // Current position being edited
  bool updateCursor = true;  // New variable to handle cursor update
  selectPressed = false;
  bool confirm = false;
  float factor = 0;  // Final scale factor value

  // Debounce variables (local to the function)
  unsigned long lastDebounceTimeUp = 0;
  unsigned long lastDebounceTimeDown = 0;
  const unsigned long debounceDelay = 50;  // Debounce delay in milliseconds

  int previousPressUp = HIGH, previousPressDown = HIGH;  // Assuming buttons are active low (pressed = LOW)
  bool debouncedUp = false, debouncedDown = false;       // Debounce flags

  setCursor(row);
  Serial1.print("X");
  row += 14;  // Move the cursor to the desired row position

  blinkingCursorON();

  temp = scaleFactor * 10000;  // Convert scale factor to an integer

  for (int i = 4; i >= 0; i--) {
    buff[i] = temp % 10;  // Extract the least significant digit
    temp /= 10;           // Move to the next digit
  }

  setCursor(row);

  // Print the scale factor with the decimal point to the LCD
  for (int i = 0; i <= 4; i++) {
    if (i == decimalLocation) {
      Serial1.print(".");
      Serial1.print(buff[i]);
    } else {
      Serial1.print(buff[i]);
    }
  }

  while (!confirm) {
    if (updateCursor) {  // Check the new variable to decide if we should update the cursor position
      if (valuePosition >= decimalLocation) {
        setCursor(row + valuePosition + 1);
      } else {
        setCursor(row + valuePosition);
      }
      updateCursor = false;  // Reset the flag after updating cursor
    }

    // Check if Up button is pressed (increments the value at valuePosition)
    int currentPressUp = digitalRead(rockerUpPin);
    if (currentPressUp != previousPressUp) {  // Button state changed
      lastDebounceTimeUp = millis();          // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeUp) > debounceDelay) {
      if (currentPressUp == LOW && !debouncedUp) {  // Button is pressed and not already debounced
        if (buff[valuePosition] > 8) {
          buff[valuePosition] = 0;
        } else {
          buff[valuePosition]++;
        }
        Serial1.print(buff[valuePosition]);
        updateCursor = true;  // Set the flag to update cursor position
        debouncedUp = true;   // Mark as debounced
      } else if (currentPressUp == HIGH) {
        debouncedUp = false;  // Reset debounced flag when button is released
      }
    }
    previousPressUp = currentPressUp;  // Store the state

    // Check if Down button is pressed (decrements the value at valuePosition)
    int currentPressDown = digitalRead(rockerDownPin);
    if (currentPressDown != previousPressDown) {  // Button state changed
      lastDebounceTimeDown = millis();            // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeDown) > debounceDelay) {
      if (currentPressDown == LOW && !debouncedDown) {  // Button is pressed and not already debounced
        if (buff[valuePosition] < 1) {
          buff[valuePosition] = 9;
        } else {
          buff[valuePosition]--;
        }
        Serial1.print(buff[valuePosition]);
        updateCursor = true;   // Set the flag to update cursor position
        debouncedDown = true;  // Mark as debounced
      } else if (currentPressDown == HIGH) {
        debouncedDown = false;  // Reset debounced flag when button is released
      }
    }
    previousPressDown = currentPressDown;  // Store the state

    // Move to the next digit when select button is pressed
    if (selectPressed) {
      valuePosition = (valuePosition + 1) % 5;  // This will loop valuePosition from 0 to 3
      selectPressed = false;
      updateCursor = true;  // Set the flag to update cursor position
    }

    // Check for back button press to confirm and exit the editing loop
    previousBackPress = backPress;
    backPress = !digitalRead(BackButton);

    if (backPress && (backPress != previousBackPress)) {
      confirm = true;
      delay(50);
    } else {
      confirm = false;
    }
  }

  // Calculate the final scale factor value from the buffer array
  for (int i = 0; i < buffLength; i++) {
    factor += buff[i] * (pow(10, ((decimalLocation - 1) - i)));
  }

  blinkingCursorOFF();

  updateMenu = true;

  return factor;  // Return the final scale factor
}

// Stretch factor *****************************************************************
float LCDSetStretchFactor(int row) {
  byte buff[] = { 0, 0, 0, 0, 0 };  // Array to store individual digits of the stretch factor
  byte buffLength = 5;              // Length of the buffer array
  int decimalLocation = 1;          // Position of the decimal point
  int temp;
  byte valuePosition = 0;    // Current position being edited
  bool updateCursor = true;  // New variable to handle cursor update
  selectPressed = false;
  bool confirm = false;
  float factor = 0;  // Final stretch factor value

  // Debounce variables (local to the function)
  unsigned long lastDebounceTimeUp = 0;
  unsigned long lastDebounceTimeDown = 0;
  const unsigned long debounceDelay = 50;  // Debounce delay in milliseconds

  int previousPressUp = HIGH, previousPressDown = HIGH;  // Assuming buttons are active low (pressed = LOW)
  bool debouncedUp = false, debouncedDown = false;       // Debounce flags

  setCursor(row);
  Serial1.print("X");
  row += 14;
  blinkingCursorON();

  temp = stretchFactor * 10000;  // Convert stretch factor to an integer

  for (int i = 4; i >= 0; i--) {
    buff[i] = temp % 10;  // Extract the least significant digit
    temp /= 10;           // Move to the next digit
  }

  setCursor(row);

  // Print the stretch factor with the decimal point to the LCD
  for (int i = 0; i <= 4; i++) {
    if (i == decimalLocation) {
      Serial1.print(".");
      Serial1.print(buff[i]);
    } else {
      Serial1.print(buff[i]);
    }
  }

  while (!confirm) {
    if (updateCursor) {  // Check the new variable to decide if we should update the cursor position
      if (valuePosition >= decimalLocation) {
        setCursor(row + valuePosition + 1);
      } else {
        setCursor(row + valuePosition);
      }
      updateCursor = false;  // Reset the flag after updating cursor
    }

    // Check if Up button is pressed (increments the value at valuePosition)
    int currentPressUp = digitalRead(rockerUpPin);
    if (currentPressUp != previousPressUp) {  // Button state changed
      lastDebounceTimeUp = millis();          // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeUp) > debounceDelay) {
      if (currentPressUp == LOW && !debouncedUp) {  // Button is pressed and not already debounced
        if (buff[valuePosition] > 8) {
          buff[valuePosition] = 0;
        } else {
          buff[valuePosition]++;
        }
        Serial1.print(buff[valuePosition]);
        updateCursor = true;  // Set the flag to update cursor position
        debouncedUp = true;   // Mark as debounced
      } else if (currentPressUp == HIGH) {
        debouncedUp = false;  // Reset debounced flag when button is released
      }
    }
    previousPressUp = currentPressUp;  // Store the state

    // Check if Down button is pressed (decrements the value at valuePosition)
    int currentPressDown = digitalRead(rockerDownPin);
    if (currentPressDown != previousPressDown) {  // Button state changed
      lastDebounceTimeDown = millis();            // Reset debounce timer
    }

    if ((millis() - lastDebounceTimeDown) > debounceDelay) {
      if (currentPressDown == LOW && !debouncedDown) {  // Button is pressed and not already debounced
        if (buff[valuePosition] < 1) {
          buff[valuePosition] = 9;
        } else {
          buff[valuePosition]--;
        }
        Serial1.print(buff[valuePosition]);
        updateCursor = true;   // Set the flag to update cursor position
        debouncedDown = true;  // Mark as debounced
      } else if (currentPressDown == HIGH) {
        debouncedDown = false;  // Reset debounced flag when button is released
      }
    }
    previousPressDown = currentPressDown;  // Store the state

    // Move to the next digit when select button is pressed
    if (selectPressed) {
      valuePosition = (valuePosition + 1) % 5;  // This will loop valuePosition from 0 to 3
      selectPressed = false;
      updateCursor = true;  // Set the flag to update cursor position
    }

    // Check for back button press to confirm and exit the editing loop
    previousBackPress = backPress;
    backPress = !digitalRead(BackButton);

    if (backPress && (backPress != previousBackPress)) {
      confirm = true;
      delay(50);
    } else {
      confirm = false;
    }
  }

  // Calculate the final stretch factor value from the buffer array
  for (int i = 0; i < buffLength; i++) {
    factor += buff[i] * (pow(10, ((decimalLocation - 1) - i)));
  }

  blinkingCursorOFF();

  updateMenu = true;

  return factor;  // Return the final stretch factor
}
// Unit float *****************************************************************
void LCDPrintUnitFloat(float value, int row) {
  if (metric) {
    setCursor(row + 13);
  } else if (imperial) {
    setCursor(row + 12);
  }
  if (value < 10) {
    Serial1.print("   ");
  } else if (value < 100) {
    Serial1.print("  ");
  } else if (value < 1000) {
    Serial1.print(" ");
  }
  Serial1.print(value, 1);
  if (metric) {
    Serial1.print("m");
  } else if (imperial) {
    Serial1.print("ft");
  }
}

// Print Seconds **************************************************************
void LCDPrintSeconds(float value, int row) {
  setCursor(row + 13);
  value = value / 1000;
  if (value < 10) {
    Serial1.print("   ");
  } else if (value < 100) {
    Serial1.print("  ");
  } else if (value < 1000) {
    Serial1.print(" ");
  }
  Serial1.print(value, 1);
  Serial1.print("s");
}

// Cable diameter ****************************************************************************
void LCDPrintUnitFloatCentimeters(float value, int row) {
  row += 12;
  byte buff[5];
  int temp = value * 100000;
  int pastDecimal = 3;

  for (int i = 4; i >= 0; i--) {
    buff[i] = temp % 10;  // Extract the least significant digit
    temp /= 10;           // Move to the next digit
  }
  setCursor(row);
  if (buff[0] == 0) {
    Serial1.print(" ");
  }
  if (buff[4] == 0) {
    Serial1.print(" ");
    pastDecimal -= 1;
    if (buff[3] == 0) {
      Serial1.print(" ");
      pastDecimal -= 1;
    }
  }
  value *= 100;  //Convert from meters to cm
  Serial1.print(value, pastDecimal);
  if (metric) {
    Serial1.print("cm");
  } else if (imperial) {
    Serial1.print("in");
  }
}

// Drum values *******************************************************************************
void LCDPrintUnitDrumValues(float value, int row) {
  byte buff[5];
  int temp = value * 1000;
  int pastDecimal = 3;

  if (metric) {
    setCursor(row + 12);
  } else if (imperial) {
    setCursor(row + 12);
  }

  for (int i = 4; i >= 0; i--) {
    buff[i] = temp % 10;  // Extract the least significant digit
    temp /= 10;           // Move to the next digit
  }
  if (buff[0] == 0) {
    Serial1.print(" ");
  }
  if (buff[4] == 0) {
    Serial1.print(" ");
    pastDecimal -= 1;
    if (buff[3] == 0) {
      Serial1.print(" ");
      pastDecimal -= 1;
    }
  }
  Serial1.print(value, pastDecimal);
  if (metric) {
    Serial1.print("cm");
  } else if (imperial) {
    Serial1.print("in");
  }
}

// Unitless float*****************************************************************
void LCDPrintUnitlessFloat(float value, int row) {
  row += 14;
  byte buff[5];
  int temp = value * 10000;
  int pastDecimal = 4;

  for (int i = 4; i >= 0; i--) {
    buff[i] = temp % 10;  // Extract the least significant digit
    temp /= 10;           // Move to the next digit
  }
  setCursor(row);
  if (buff[4] == 0) {
    Serial1.print(" ");
    pastDecimal -= 1;
    if (buff[3] == 0) {
      Serial1.print(" ");
      pastDecimal -= 1;
      if (buff[2] == 0) {
        Serial1.print(" ");
        pastDecimal -= 1;
      }
    }
  }
  Serial1.print(value, pastDecimal);
}

// Unitless int*****************************************************************
void LCDPrintUnitlessInt(int value, int row) {
  row += 16;
  setCursor(row);
  if (value < 10) {
    Serial1.print("   ");
  } else if (value < 100) {
    Serial1.print("  ");
  } else if (value < 1000) {
    Serial1.print(" ");
  }
  Serial1.print(value);
}

void ConvertMeters(float value) {
  if (imperial) {
    tempVal = value * 3.28084;
  } else if (metric) {
    tempVal = value;
  }
}

void ConvertCentimeters(float value) {
  if (imperial) {
    tempVal = value * 0.394;
  } else if (metric) {
    tempVal = value;
  }
}

//-----------------------------------------------------------------SD Card Functions-----------------------------------------------------------------
bool SD_Begin(void) {
  if (!SD.begin(chipSelect)) {
    setCursor(0x00);
    Serial1.print("SD Card failed  ");
    return (0);
  } else {
    setCursor(0x00);
    Serial1.print("SD Card initialized.");
    return (1);
  }
}

int readFileInt() {
  int printArray[8];
  int output = 0;
  int i = 0;
  bool isNegative = false;
  while (drumFile.available()) {
    char received = drumFile.read();
    if (received == ',') {
      break;
    }
    if (received == '-') {
      isNegative = true;
    } else {
      printArray[i] = received - '0';
      i++;
    }
  }
  for (int k = 0; k < i; k++) {
    output += printArray[k] * (pow(10, ((i - 1) - k)));
  }
  if (isNegative) {
    output *= -1;
  }
  return output;
}

float readFileFloat() {
  int printArray[] = { 0, 0, 0, 0, 0, 0 };
  float output = 0;
  int LOD = 0;
  int i = 0;
  while (drumFile.available()) {
    char received = drumFile.read();
    if (received == ',') {
      break;
    }
    if (received == '.') {
      LOD = i;
    } else {
      printArray[i] = received - '0';
      i++;
    }
  }
  for (int k = 0; k < i; k++) {
    output += printArray[k] * (pow(10, ((LOD - 1) - k)));
  }
  return output;
}

bool readFileBool() {
  bool output;
  while (drumFile.available()) {
    char received = drumFile.read();
    if (received == ',') {
      break;
    }
    if (received == 'T') {
      output = true;
    } else if (received == 'F') {
      output = false;
    }
  }
  return output;
}

void setUnitsFromFile() {
  while (drumFile.available()) {
    char received = drumFile.read();
    if (received == ',') {
      break;
    }
    if (received == 'M') {
      metric = true;
      imperial = false;
    } else if (received == 'I') {
      imperial = true;
      metric = false;
    }
    if (received == 's') {
      seconds = true;
      minutes = false;
    } else if (received == 'm') {
      minutes = true;
      seconds = false;
    }
  }
}

//-----------------------------------------------------------------HTML read functions-----------------------------------------------------------------
void GetAjaxData(EthernetClient cl, int Analog_Slot, int Analog_Input_Channel) {
  int analog_val;
  int analog_percent;
  if (startState) {
    cl.print("Running|");
    cl.print("box Green|");
    if (outState) {
      cl.print("Runing Out at |");
    } else if (inState) {
      cl.print("Running In at |");
    } else {
      cl.print("Waiting for input |");
    }
    cl.print("box Basic|");
    analog_val = P1.readAnalog(Analog_Slot, Analog_Input_Channel);
    analog_percent = map(analog_val, 0, 8190, 0, 100);
    cl.print(analog_percent);
    cl.print("%|box Basic Right Percent|");
  } else {
    cl.print("Stopped|box Red||box Clear||box Clear|");
  }
  if (offsetActive) {
    tempVal = cablePayout - offset;
  } else {
    tempVal = cablePayout;
  }
  if (imperial) {
    tempVal = tempVal * 3.28084;
    cl.print(tempVal, 1);
    cl.print("ft");
    cl.print("|");
    if (seconds) {
      tempVal = cableSpeed * 3.28084;
      cl.print(tempVal, 2);
      cl.print("ft/s");
    } else if (minutes) {
      tempVal = cableSpeed * 3.28084 * 60;
      cl.print(tempVal, 1);
      cl.print("ft/m");
    }
    cl.print("|");
  } else {
    cl.print(tempVal, 1);
    cl.print("m");
    cl.print("|");
    if (seconds) {
      cl.print(cableSpeed, 2);
      cl.print("m/s");
    } else if (minutes) {
      tempVal = cableSpeed * 60;
      cl.print(tempVal, 1);
      cl.print("m/m");
    }
    cl.print("|");
  }
  if (limitSwitch) {
    if (maxPayErr) {
      cl.print("Max Limit!|");
      cl.print("Red Indicator|");
    } else if (minPayErr) {
      cl.print("Min Limit!|");
      cl.print("Red Indicator|");
    } else {
      cl.print("Limits ON|");
      cl.print("Red Indicator|");
    }
  } else {
    cl.print("Limits OFF|");
    cl.print("Green Indicator|");
  }

  if (offsetActive) {
    cl.print("Offset ON|");
    cl.print("Red Indicator");
  } else {
    cl.print("Offset OFF|");
    cl.print("Green Indicator");
  }
}

//*****************************************************************Encoder Functions*****************************************************************
float GetCablePayout() {
  encoderCount = HSC.CNT1.readPosition();
  encoderCurrentCount = encoderCount;
  if (encoderCurrentCount > encoderPreviousCount) {
    retainCount = retainCount + (encoderCurrentCount - encoderPreviousCount);
  } else {
    retainCount = retainCount - (encoderPreviousCount - encoderCurrentCount);
  }
  encoderPreviousCount = encoderCurrentCount;
  // Avoid a potential divide by zero in the following calculations
  if (drumEncRes == 0) {
    drumEncRes = 256;
  }
  // Handle a possible negative pay-in
  if (retainCount < 0) {
    drumCountValue = abs(retainCount);
    drumCountNegative = true;
  } else {
    drumCountValue = retainCount;
    drumCountNegative = false;
  }
  // Use total counts and counts per rotation to figure out number of rotations.
  drumTurns = float(drumCountValue) / drumEncRes;
  // retainCount - (drumTurns * drumEncRes) = counts for a final partial turn
  partialDrumTurn = (float(drumCountValue) / drumEncRes) - drumTurns;
  // Drum width divided by cable diameter = cable turns per layer.
  drumTurnsPerLayer = drumWidth / cableDiameter;
  // Drum diameter + cable outside diameter = true diameter of first layer to center of cable.
  trueDiameter = drumDiameter + cableDiameter;
  // Initialize cable payout to zero
  calcCablePayout = 0;
  // Calculate contribution of complete cable turns on full layers.
  if (drumTurnsPerLayer != 0) {
    while (drumTurns >= drumTurnsPerLayer) {
      // Calculate cable length in current layer and add to total.
      calcCablePayout = calcCablePayout + (drumTurnsPerLayer * trueDiameter * 3.141593);

      // Calculate drum diameter to center of next cable layer.
      trueDiameter = trueDiameter + (2 * cableDiameter);

      // Finished with layer so subtract layer turns from total number remaining.
      drumTurns = drumTurns - drumTurnsPerLayer;
    }
  }
  // Calculate contribution of any remaining turns on final layer.
  if (drumTurns != 0) {
    calcCablePayout = calcCablePayout + (drumTurns * trueDiameter * 3.141593);
  }
  // Calculate contribution of any remaining partial turn.
  calcCablePayout = calcCablePayout + (partialDrumTurn * trueDiameter * 3.141593);
  // Calculate cable payout with scaling factor (1 = no scaling);
  calcCablePayout = calcCablePayout * scaleFactor;
  // Adjust sign for initial calibration drum direction.
  if (drumDirectionNegative == true) {
    calcCablePayout = calcCablePayout * -1;
  }
  if (drumCountNegative == false) {
    calcCablePayout = calcCablePayout * -1;
  }
  //Calculate cable payout starting with full drum amount.*********************************************************************
  calcCablePayout = cableLength + calcCablePayout;
  // Calculate cable payout with stretching factor (1 = no stretching).
  calcCablePayout = calcCablePayout * stretchFactor;
  // Calculate cable speed based on 1 second timer.
  return calcCablePayout;
}

//-----------------------------------------------------------------Interrupt Functions-----------------------------------------------------------------
// Select *****************************************************************
void SelectPressed() {
  noInterrupts();
  selectPressed = true;
  interrupts();
}

// Rotate *****************************************************************
void Rotate() {
  unsigned long currentTime = millis();

  // Logic for rockerUpPin (Clockwise)
  if ((currentTime - lastDebounceTimeUp) > debounceDelay) {
    int currentPressUp = digitalRead(rockerUpPin);
    if (currentPressUp == HIGH && previousPressUp == LOW) {
      encoderCCW = true;  // Clockwise rotation detected
    }
    previousPressUp = currentPressUp;  // Update previous state
    lastDebounceTimeUp = currentTime;  // Update debounce time for rockerUpPin
  }

  // Logic for rockerDownPin (Counterclockwise)
  if ((currentTime - lastDebounceTimeDown) > debounceDelay) {
    int currentPressDown = digitalRead(rockerDownPin);
    if (currentPressDown == HIGH && previousPressDown == LOW) {
      encoderCW = true;  // Counterclockwise rotation detected
    }
    previousPressDown = currentPressDown;  // Update previous state
    lastDebounceTimeDown = currentTime;    // Update debounce time for rockerDownPin
  }
}


// Power Loss *****************************************************************
void PowerLoss() {
  noInterrupts();
  if (drumFile) {
    drumFile.close();
  }
  drumFile = SD.open("DRMCONST.TXT", (O_READ | O_WRITE));
  if (drumFile) {
    drumFile.find("retainCount:");
    drumFile.position();
    drumFile.print(retainCount);
    drumFile.print(",");
    drumFile.close();
  }
  interrupts();
}

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else   // __ARM__
extern char *__brkval;
#endif  // __arm__

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else   // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}
