//***********************************************************************Libraries***********************************************************************
#include <SD.h>               // For SD card
#include <Ethernet.h>         // For Computer Control (W5100.h was edited to have proper address as the P1AM uses the W5500.h internet driver)
#include <P1AM.h>             // For P1AM
#include <P1_HSC.h>           // For high speed counter
#include <P1AM_Functions.h>   // Custom for writing commands to P1AM
#include <P1AM_Serial.h>      // For serial ports
#include <RS232_Functions.h>  // Custom for writing commands to LCD
#include "utility/w5100.h"    // Used for retransmission count and timeout
#include <Wire.h>             // For I2C communication
#include <RTClib.h>           // For DS3231 RTC (Adafruit RTClib)
#include <SPI.h>

RTC_DS3231 rtc;  // Use RTClib for DS3231

//**********************************************************
//P1-4ADL2DAL-2
#define ANALOG_SLOT 3
//CHANNELS
#define ANALOG_OUTPUT_CHANNEL1 2  //Speed
#define ANALOG_INPUT_CHANNEL1 1
//P1-08TRS
#define OUTPUT_SLOT 1
//CHANNELS
#define STOP_START_OUTPUT_CHANNEL 1
#define OUT_OUTPUT_CHANNEL 2  //FWD
#define IN_OUTPUT_CHANNEL 3   //REV
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

// Global Variables
int timeInterval, repeats;
char timeUnit[10], actionAtMax[10];
bool startNow, returnTriggered = false;
int startDay, startMonth, startYear, startHours, startMinutes, startSeconds;
float totalDistanceTraveled = 0.0;  // Total distance traveled in meters
const float MOTOR_SPEED = 5.0;      // Motor speed in meters per second (5V = 5 m/s)
unsigned long movementStartTime = 0;  // Tracks when the motor started moving
float currentPayout = 0.0;  // Tracks the current payout in meters
int stepLengthMeters;        // Step length in meters
int maxCableLength;          // Maximum cable length in meters

// Non-blocking control variables
bool isMoving = false;
bool routineComplete = false;
unsigned long intervalStartTime;
int currentRepeat = 0;

// LCD update variables
unsigned long lastLCDUpdate = 0;
const long lcdInterval = 1000;  // Update LCD every 1 second

void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;           // Wait for Serial Monitor to open
  delay(1000);  // Small delay to ensure Serial Monitor is ready

  while (!P1.init())
    ;  // Wait for Modules to Sign on

  Serial.println("Motor Control Program Starting...");

  initializeRTC();
  setRTCTime();       // Ask for time first
  handleUserInput();  // Then ask for other parameters
  waitForStartTime();
  Serial.println("Starting routine...");
}

void loop() {
  if (!routineComplete) {
    if (!returnTriggered) {
      static unsigned long intervalMillis = 0;
      static unsigned long movementStartTime = 0;

      if (intervalMillis == 0) {
        intervalMillis = timeInterval * 1000UL;
        if (strcmp(timeUnit, "minutes") == 0) intervalMillis *= 60;
        else if (strcmp(timeUnit, "hours") == 0) intervalMillis *= 3600;
        else if (strcmp(timeUnit, "days") == 0) intervalMillis *= 86400;
      }

      if (!isMoving) {
        if (currentRepeat < repeats) {
          if (millis() - movementStartTime >= intervalMillis || currentRepeat == 0) {
            executeRoutine();
            movementStartTime = millis();
            currentRepeat++;
          }
        } else {
          handleRoutineCompletion();
        }
      } else {
        monitorMovement();
      }
    } else {
      returnToStart();
    }
  } else {
    routineComplete = false;
    returnTriggered = false;
    currentRepeat = 0;
    totalDistanceTraveled = 0.0;

    while (Serial.available()) Serial.read();

    Serial.println("\n\n--- New Routine Setup ---");
    handleUserInput();
    waitForStartTime();
    Serial.println("Starting new routine...");
  }
}

// Date handling functions
void adjustStartDate() {
  int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  while (startDay > daysInMonth[startMonth - 1]) {
    if (startMonth == 2) {
      if ((startYear % 4 == 0 && startYear % 100 != 0) || (startYear % 400 == 0)) {
        daysInMonth[1] = 29;
      } else {
        daysInMonth[1] = 28;
      }
    }

    if (startDay > daysInMonth[startMonth - 1]) {
      startDay -= daysInMonth[startMonth - 1];
      startMonth++;
    }

    if (startMonth > 12) {
      startMonth = 1;
      startYear++;
      if ((startYear % 4 == 0 && startYear % 100 != 0) || (startYear % 400 == 0)) {
        daysInMonth[1] = 29;
      } else {
        daysInMonth[1] = 28;
      }
    }
  }
}

void initializeRTC() {
  rtc.begin();

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, resetting time to compile time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  Serial.println("RTC initialized successfully.");
}

void setRTCTime() {
  Serial.println("Set current RTC time:");

  int hours = getValidatedInput("Enter Hours (0-23): ", 0, 23);
  int minutes = getValidatedInput("Enter Minutes (0-59): ", 0, 59);
  int seconds = getValidatedInput("Enter Seconds (0-59): ", 0, 59);
  int day = getValidatedInput("Enter Day (1-31): ", 1, 31);
  int month = getValidatedInput("Enter Month (1-12): ", 1, 12);
  int year = getValidatedInput("Enter Year (2000-2099): ", 2000, 2099);

  rtc.adjust(DateTime(year, month, day, hours, minutes, seconds));

  DateTime now = rtc.now();
  Serial.println("RTC time set to:");
  Serial.print(now.hour());
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.print(now.second());
  Serial.print(" ");
  Serial.print(now.day());
  Serial.print("/");
  Serial.print(now.month());
  Serial.print("/");
  Serial.println(now.year());

  delay(2000);
}

void handleUserInput() {
  while (true) {
    timeInterval = getValidatedInput("Enter Time Interval (1-59): ", 1, 59);

    const char* timeUnitChoices[] = { "seconds", "minutes", "hours", "days" };
    getValidatedChoice("Time Unit (seconds/minutes/hours/days): ", timeUnitChoices, 4, timeUnit);

    repeats = getValidatedInput("Number of Repeats (1-100): ", 1, 100);

    maxCableLength = getValidatedInput("Total Cable Length (m) (10-5000): ", 10, 5000);

    while (true) {
      stepLengthMeters = getValidatedInput("Step Length (m): ", 1, maxCableLength);
      if (stepLengthMeters * repeats > maxCableLength) {
        Serial.println("Error: Total movement exceeds cable length!");
        Serial.print("Maximum allowed steps: ");
        Serial.println(maxCableLength / stepLengthMeters);
      } else {
        break;
      }
    }

    const char* actionChoices[] = { "return", "stop" };
    getValidatedChoice("Action at max (return/stop): ", actionChoices, 2, actionAtMax);

    startNow = getValidatedInput("Start Immediately? (1=Yes, 0=No): ", 0, 1);

    if (!startNow) {
      Serial.println("Enter delay in decimal hours (e.g., 0.25 for 15 minutes):");
      char input[20];
      while (Serial.available() == 0)
        ;
      Serial.readBytesUntil('\n', input, sizeof(input));
      input[strcspn(input, "\r\n")] = 0;

      float delayHours = atof(input);
      if (delayHours <= 0) {
        Serial.println("Invalid delay! Must be a positive number.");
        continue;
      }

      int delaySeconds = (int)(delayHours * 3600);
      DateTime now = rtc.now();
      DateTime startTime = now + TimeSpan(delaySeconds);

      startHours = startTime.hour();
      startMinutes = startTime.minute();
      startSeconds = startTime.second();
      startDay = startTime.day();
      startMonth = startTime.month();
      startYear = startTime.year();

      if (startHours >= 24) {
        startHours -= 24;
        startDay++;
        adjustStartDate();
      }

      Serial.println("Start time calculated:");
      Serial.print(startHours);
      Serial.print(":");
      Serial.print(startMinutes);
      Serial.print(":");
      Serial.print(startSeconds);
      Serial.print(" on ");
      Serial.print(startDay);
      Serial.print("/");
      Serial.print(startMonth);
      Serial.print("/");
      Serial.println(startYear);
    }

    break;
  }
}

void waitForStartTime() {
  if (startNow) return;

  Serial.println("Waiting for start time...");
  while (true) {
    DateTime now = rtc.now();
    if (now.year() == startYear && now.month() == startMonth && now.day() == startDay && now.hour() == startHours && now.minute() == startMinutes && now.second() >= startSeconds) {
      break;
    }
    delay(50);
  }
}

void executeRoutine() {
  if (!isMoving) {
    moveMotor(false);  // Move forward
    displayRealTimeFeedback(currentRepeat + 1, repeats);
  }
}

void returnToStart() {
  if (!isMoving) {
    moveMotor(true);  // Move in reverse
  } else {
    monitorMovement();
    if (!isMoving) {
      returnTriggered = false;
      routineComplete = true;
      Serial.println("Return complete.");
    }
  }
}

void moveMotor(bool reverse) {
  if (reverse) {
    runIn(OUTPUT_SLOT, OUT_OUTPUT_CHANNEL, IN_OUTPUT_CHANNEL);  // Reverse direction
  } else {
    runOut(OUTPUT_SLOT, OUT_OUTPUT_CHANNEL, IN_OUTPUT_CHANNEL);  // Forward direction
  }
  P1.writeAnalog(2048, ANALOG_SLOT, ANALOG_OUTPUT_CHANNEL1);  // Set speed to 5V (5 m/s)
  movementStartTime = millis();
  isMoving = true;
}

void stopMotor() {
  stopSignal(OUTPUT_SLOT, STOP_START_OUTPUT_CHANNEL);
  clearOutIn(OUTPUT_SLOT, OUT_OUTPUT_CHANNEL, IN_OUTPUT_CHANNEL);
  isMoving = false;
  currentPayout = 0.0;
}

void monitorMovement() {
  if (isMoving) {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - movementStartTime;
    float elapsedSeconds = elapsedTime / 1000.0;
    currentPayout = MOTOR_SPEED * elapsedSeconds;

    if (currentPayout >= stepLengthMeters) {
      stopMotor();
      intervalStartTime = millis();
      currentPayout = 0.0;
    }
  }
}

void handleRoutineCompletion() {
  if (strcmp(actionAtMax, "return") == 0) {
    returnTriggered = true;
  } else {
    routineComplete = true;
    Serial.println("Routine complete. Stopping.");
  }
}

void displayRealTimeFeedback(int repeat, int totalRepeats) {
  float progress = ((float)repeat / totalRepeats) * 100.0;
  Serial.print("Repeat ");
  Serial.print(repeat);
  Serial.print(" of ");
  Serial.print(totalRepeats);
  Serial.print(" (");
  Serial.print(progress, 1);
  Serial.println("% completed)");
}

int getValidatedInput(const char* prompt, int min, int max) {
  int value;
  while (true) {
    Serial.print(prompt);
    while (!Serial.available())
      ;
    while (Serial.available() > 0 && !isdigit(Serial.peek())) {
      Serial.read();
    }
    value = Serial.parseInt();
    Serial.println(value);
    if (value >= min && value <= max) return value;
    Serial.print("Invalid input! Please enter between ");
    Serial.print(min);
    Serial.print(" and ");
    Serial.println(max);
  }
}

void getValidatedChoice(const char* prompt, const char* choices[], int numChoices, char* output) {
  char input[10] = { 0 };
  while (true) {
    Serial.print(prompt);
    while (!Serial.available())
      ;
    size_t bytesRead = Serial.readBytesUntil('\n', input, sizeof(input) - 1);
    input[bytesRead] = 0;
    for (int i = strlen(input) - 1; i >= 0; i--) {
      if (isspace(input[i]) || input[i] == '\r') input[i] = 0;
      else break;
    }
    for (int i = 0; i < numChoices; i++) {
      if (strcasecmp(input, choices[i]) == 0) {
        strncpy(output, choices[i], 9);
        output[9] = 0;
        return;
      }
    }
    Serial.println("Invalid choice. Try again.");
  }
}
