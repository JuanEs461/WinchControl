#include <Ethernet.h>
#include <P1AM.h>
#include <RS232_Functions.h>

#define TEST_INPUT_SLOT 5
#define TEST_STATE_INPUT_CHANNEL 1
#define TEST_ESTOP_INPUT_CHANNEL 2
#define TEST_MANUAL_START_INPUT_CHANNEL 3
#define TEST_LIMITSWITCH_INPUT_CHANNEL 4

byte mac[] = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
//IPAddress ip(192,168,0,2);
EthernetServer server(80);
EthernetClient client;

bool previousManualSwitch = false, manualSwitch = false, manualState = false, previousManualState = false;
bool ethernetDebug = true;

void setup() {
  PORT1_RS232_BEGIN(9600);
  Serial.begin(9600);
  while(!Serial);
  while(!P1.init());
  clearScreen();
  manualState = P1.readDiscrete(TEST_INPUT_SLOT, TEST_STATE_INPUT_CHANNEL);   // Read the manual or computer switch to determine which state to start in
  previousManualState = manualState;                                          // Preparing for which state to start the program in
  manualSwitch = P1.readDiscrete(TEST_INPUT_SLOT, TEST_STATE_INPUT_CHANNEL);  // Read the switch to determine which position it is to ensure proper reloads occur
}

void loop() {

  previousManualSwitch = manualSwitch;
  manualSwitch = P1.readDiscrete(TEST_INPUT_SLOT, TEST_STATE_INPUT_CHANNEL);
  if (manualSwitch != previousManualSwitch) {  // Checking if the manual switch has been toggled
    previousManualState = manualState;
    manualState = !manualState;  // toggle the state
    ethernetDebug = true;        // flag to tell PLC to check ethernet connection
  }

   if (!manualState && ethernetDebug) {
    clearScreen();
    delay(50);
    Serial1.print("waiting for ethernetconnection");
    //error check ethernet connection, same error checking that occurs at setup
    Serial.println("waiting for ethernet");
    delay(100);
    if(!server){
      Serial.println("ethernet begin");
    Ethernet.begin(mac);
    } else {
      Serial.println("ethernet has begun");
    }
    Serial.println("going to if");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      clearScreen();
      delay(50);
      Serial1.print("Ethernet shield was not found.");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      clearScreen();
      delay(50);
      Serial1.print("Ethernet cable is   not connected.");
      Serial.println("no cable");
    } else {
      Serial.println("no issues found");
    }
    while (true) {  //wait for a fix
      Serial.println("in while");
      manualSwitch = P1.readDiscrete(TEST_INPUT_SLOT, TEST_STATE_INPUT_CHANNEL);
      manualState = manualSwitch;
      if (manualSwitch) {
        Serial.println("manualSwitch");
        break;
      }
      if(Ethernet.linkStatus() == LinkON){
        Serial.println("connected");
        break;
      }
      // if (Ethernet.begin(mac, ip) == 1) {  //if ethernet signs on then continue
      //   Serial.println("mac");
      //   break;
      // } else if (Ethernet.begin(mac) == 0) {
      //   Serial.println("cam");

      // }
      delay(500);
    }

    if(!server){
      Serial.println("begin");
      server.begin();  //start the html server
    } else {
      Serial.println("server is real");
    }
    clearScreen();
    delay(100);
    Serial1.print(Ethernet.localIP());  //function that says what IP the server is at
    if(!manualState){
      Serial1.print(" Comp");
    } else {
      Serial1.print(" Man");
    }
    client.stop();
    ethernetDebug = false;
  } else if (manualState && ethernetDebug) {  // If the ethernet connection is lost while in computer control and manual mode is entered 0.0.0.0 will be displayed
    server.begin();                           //start the html server
    clearScreen();
    setCursor(0x00);
    delay(100);
    Serial1.print(Ethernet.localIP());  //function that says what IP the server is at
    Serial1.print(" Man");
    client.stop();
    ethernetDebug = false;
  }
}