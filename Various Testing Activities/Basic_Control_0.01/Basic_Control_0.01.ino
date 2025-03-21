/*
  Tester code for basic functionality

  Goal is to have forward, reverse, start, stop and speed control
  Other functions are sure to be thought of and added

  Date: May 15th 2023
  Author: Kailyn Crossman
  Company: AGO Environmental

*/

#include <P1AM.h>

#define ANALOG_SLOT 1
#define OUTPUT_SLOT 2
#define INPUT_SLOT 4

#define STOP_IN_CHANNEL 1
#define START_IN_CHANNEL 2
#define FWD_IN_CHANNEL 3
#define REV_IN_CHANNEL 4 
#define JOY_IN_CHANNEL 1

#define STOP_START_OUT_CHANNEL 1
#define FWD_OUT_CHANNEL 2
#define REV_OUT_CHANNEL 3
#define JOY_OUT_CHANNEL 1



bool Stop = true;
bool Start = false;
bool Fwd = true;
bool Rev = false;
int Output_Value = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  while (!P1.init()){
    ; //Wait for modules to sign on 
  }
  while (!Serial){
    ; //Wait for serial to be active
  }

  Serial.println("Booting");
  Stop = P1.readDiscrete(INPUT_SLOT, STOP_IN_CHANNEL);  //read the stop input
  Start = P1.readDiscrete(INPUT_SLOT, START_IN_CHANNEL); //read the start input
  Fwd = P1.readDiscrete(INPUT_SLOT, FWD_IN_CHANNEL);        //read the fwd input
  Rev = P1.readDiscrete(INPUT_SLOT, REV_IN_CHANNEL);      //read rev input

  while(Start == true){
    Serial.println("The start button is active, please release to continue Winch boot up");
    Start = P1.readDiscrete(INPUT_SLOT, START_IN_CHANNEL); //read the start input
    delay(500);
  }
  while(Fwd == true || Rev == true){
    Serial.println("The joystick is not at zero, please centre to continue Winch boot up");
    Fwd = P1.readDiscrete(INPUT_SLOT, FWD_IN_CHANNEL);        //read the fwd input
    Rev = P1.readDiscrete(INPUT_SLOT, REV_IN_CHANNEL);      //read rev input
    delay(500);
  }

  analogWriteResolution(12); //Set to use DAC at 12-bit resolution
  Serial.println("Winch has completed boot up");
}


void loop() {
  // put your main code here, to run repeatedly:
  Stop = P1.readDiscrete(INPUT_SLOT, STOP_IN_CHANNEL);  //read the stop input
  Start = P1.readDiscrete(INPUT_SLOT, START_IN_CHANNEL); //read the start input
  Fwd = P1.readDiscrete(INPUT_SLOT, FWD_IN_CHANNEL);        //read the fwd input
  Rev = P1.readDiscrete(INPUT_SLOT, REV_IN_CHANNEL);      //read rev input



  if (Stop == true) {                          //if the stop button is active keep everything off
    Serial.println("stopped");
    stopSignal();
  } else if (Start == true && Stop == false) {                    //if the start button is pressed then do stuff
    Serial.println("running");
    if (Fwd == true && Rev == false) {                               //the fwd/rev pin's default state is forward
      Serial.println("forward");
      runForward();
    } 
    else if (Rev == true && Fwd == false) {                         //if the fwd/rev pin is toggled on it now runs in reverse
      Serial.println("reverse");
      runReverse(); 
    }
    else if (Rev == true && Fwd == true) {
      Serial.println("ERROR FORWARD AND REVERSE ARE BOTH ACTIVE!");
      clearFwdRev();
      Stop = true;
    } else {
      Serial.println("Waiting for input");
      clearFwdRev();
      Stop = true;
    }
    Output_Value = readJoystick(ANALOG_SLOT, JOY_IN_CHANNEL);
      if (Start == true && Stop == false && Output_Value < 4095) {
        //P1.writeAnalog(Output_Value, ANALOG_SLOT, JOY_OUT_CHANNEL);
        startSignal();
      } 
      else {
        ;
      }
  } else {
    Serial.println("stopped");
    stopSignal();
  }
  delay(500);
}


//---------------------FUNCTION DEFINITIONS---------------------

void clearFwdRev(){
  P1.writeDiscrete(LOW, OUTPUT_SLOT, FWD_OUT_CHANNEL);               //turn off fwd output pin 
  P1.writeDiscrete(LOW, OUTPUT_SLOT, REV_OUT_CHANNEL);                //turn off rev output pin
}

void runForward(){
  P1.writeDiscrete(HIGH, OUTPUT_SLOT, FWD_OUT_CHANNEL);               //turn on fwd output pin 
  P1.writeDiscrete(LOW, OUTPUT_SLOT, REV_OUT_CHANNEL);                //turn off rev output pin
}

void runReverse(){
  P1.writeDiscrete(HIGH, OUTPUT_SLOT, REV_OUT_CHANNEL);               //turn on rev output pin 
  P1.writeDiscrete(LOW, OUTPUT_SLOT, FWD_OUT_CHANNEL);                //turn off fwd output pin
}

void stopSignal(){
  P1.writeDiscrete(LOW, OUTPUT_SLOT, STOP_START_OUT_CHANNEL);          //send the off signal
}

void startSignal(){
  P1.writeDiscrete(HIGH, OUTPUT_SLOT, STOP_START_OUT_CHANNEL);         //send start signal
}

int readJoystick(int Analog_Slot, int Input_Channel){
  int Input_Avg = 0;
  int Input_Value = 0;
  int Output_Value = 0;
  for (int i = 0; i < 63; i++){                      
    //Input_Value = P1.readAnalog(Analog_Slot, Input_Channel);     //read the joystick value
    Input_Avg += Input_Value;                                     //add all the values together
  } 
  Input_Avg = Input_Avg/64;                        //average the joystick values 
  Output_Value = Input_Avg/2;                      //convert the 13bit input to a 12bit output
  return Output_Value;
}
