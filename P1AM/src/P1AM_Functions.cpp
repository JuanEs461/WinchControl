#include "P1AM_Functions.h"

void clearOutIn(int Output_Slot, int Out_Channel, int In_Channel){
  P1.writeDiscrete(LOW, Output_Slot, Out_Channel);
  P1.writeDiscrete(LOW, Output_Slot, In_Channel);
}

void runOut(int Output_Slot, int Out_Channel, int In_Channel){
  P1.writeDiscrete(HIGH, Output_Slot, Out_Channel);
  P1.writeDiscrete(LOW, Output_Slot, In_Channel);
}
void runIn(int Output_Slot, int Out_Channel, int In_Channel){ 
  P1.writeDiscrete(LOW, Output_Slot, Out_Channel);
  P1.writeDiscrete(HIGH, Output_Slot, In_Channel);
}
void stopSignal(int Output_Slot, int Stop_Start_Channel){
  P1.writeDiscrete(LOW, Output_Slot, Stop_Start_Channel);
}
void startSignal(int Output_Slot, int Stop_Start_Channel){
  P1.writeDiscrete(HIGH, Output_Slot, Stop_Start_Channel);
}

int readJoystick(int Analog_Slot, int Input_Channel){
  int Input_Value = 0;
  int Output_Value = 0;
  for (int i = 0; i < 10; i++){                      
    Input_Value = Input_Value + P1.readAnalog(Analog_Slot, Input_Channel);     //read the joystick value
    delay(1);
  } 
  Input_Value = Input_Value/10;                        //average the joystick values 
  Output_Value = Input_Value/2;                      //convert the 13bit input to a 12bit output
  return Output_Value;
}

