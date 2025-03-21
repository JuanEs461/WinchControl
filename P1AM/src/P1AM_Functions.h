#ifndef P1AM_FUNCTIONS_H
#define P1AM_FUNCTIONS_H

#include <SPI.h>
#include <Ethernet.h>
#include <P1AM.h>

void clearOutIn(int Output_Slot, int Out_Channel, int In_Channel);
void runOut(int Output_Slot, int Out_Channel, int In_Channel);
void runIn(int Output_Slot, int Out_Channel, int In_Channel);
void stopSignal(int Output_Slot, int Stop_Start_Channel);
void startSignal(int Output_Slot, int Stop_Start_Channel);

int readJoystick(int Analog_Slot, int Input_channel);

void GetAjaxData(EthernetClient cl, int Analog_Slot, int Analog_Input_Channel ,int Input_Slot, int Out_Input_Channel, int In_Input_Channel, bool startState);
#endif