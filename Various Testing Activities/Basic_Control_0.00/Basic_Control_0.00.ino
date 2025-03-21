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

#define START_STOP 1
#define FWD_REV 2

bool Start_Stop = false;
bool Fwd_Rev = false;
int Input_Count = 0;
float Input_Volts = 0;
int Output_Count = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  while (!P1.init()){
    ; //Wait for modules to sign on 
  }

  analogWriteResolution(13); //Set to use DAC at 13-bit resolution
}


void loop() {
  // put your main code here, to run repeatedly:
  Start_Stop = P1.readDiscrete(INPUT_SLOT, START_STOP);
  Fwd_Rev = P1.readDiscrete(INPUT_SLOT, FWD_REV);

  if (Fwd_Rev == false) {
  Serial.println("forward");
  P1.writeDiscrete(HIGH, OUTPUT_SLOT, 2);
  P1.writeDiscrete(LOW, OUTPUT_SLOT, 3);

  } else if (Fwd_Rev == true) {
  Serial.println("reverse");
  P1.writeDiscrete(HIGH, OUTPUT_SLOT, 3);
  P1.writeDiscrete(LOW, OUTPUT_SLOT, 2);
  }
  if (Start_Stop == false) {
    Serial.println("stopped");
    P1.writeDiscrete(LOW, OUTPUT_SLOT, 1);

  } else if (Start_Stop == true) {
    Serial.println("running");
    P1.writeDiscrete(HIGH, OUTPUT_SLOT, 1);

    Input_Count = P1.readAnalog(ANALOG_SLOT, 1);
    Input_Volts = 10 * ((float)Input_Count / 8191);

    Serial.print("The input voltage is");
    Serial.println(Input_Volts);

   // P1.writeAnalog(Output_Count, ANALOG_SLOT, 7);
  }
}
