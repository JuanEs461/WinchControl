/*
  Testing to find how to control the P1-4ADL2DAL-2 module

  Date: May 16th 2023
  Author: Kailyn Crossman
  Company: AGO Environmental

*/
#include <P1AM.h>

#define ANALOG_SLOT 1
#define INPUT_CHANNEL 1
#define OUTPUT_CHANNEL 1

int output = 0;
int Input_Values[64]; //can be used to inject specific values

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  //   while (!P1.init()){
  //   ; //Wait for modules to sign on 
  // }
  while(!Serial){
    ;// wait for the serial monitor to open
  }
  Serial.println("Starting!");

  for (int i = 0; i < 64; i++){
    Input_Values[i] = 8191; 
  }
  Input_Values[19] = 8191; //can add noise

  analogWriteResolution(12); //Set to use DAC at 12-bit resolution
}

void loop() {

  
  output = readJoystick(ANALOG_SLOT,INPUT_CHANNEL);
  Serial.print("The output is  ");
  Serial.println(output);
  P1.writeAnalog(output, ANALOG_SLOT, OUTPUT_CHANNEL);
  
  delay(2000);

}

int readJoystick(int analog_slot, int intput_channel){
  int Input_Avg = 0;
  int Input_Value = 0;
  int Output_Value = 0;
  for (int i = 0; i < 64; i++){       
    //Input_Value = P1.readAnalog(analog_slot, input_channel);     //read the joystick value       
    Input_Value = Input_Values[i];                                 //inject specific values
    Input_Avg += Input_Value;                                     //add all the values together
  } 
  Input_Avg = Input_Avg/64;                        //average the joystick values 
  Output_Value = Input_Avg/2;                      //convert the 13bit input to a 12bit output
  return Output_Value;
}
