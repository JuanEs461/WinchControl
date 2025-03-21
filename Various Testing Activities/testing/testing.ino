#include <P1AM.h>
#include <P1AM_Serial.h>
#include <RS232_Functions.h>
#include <SD.h>               // For SD card
int cursorLocation = 0x1C;
float value = 9.9990;
#define BackButton 12
File testFile;
const int chipSelect = SDCARD_SS_PIN;
void setup() {
  // put your setup code here, to run once:
  pinMode(SWITCH_BUILTIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
}


void loop() {
  if(digitalRead(SWITCH_BUILTIN)){
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
}


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
