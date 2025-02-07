#include <Wire.h>
#include <SD.h>               // For SD card
#include <Ethernet.h>         // For Computer Control (W5100.h was edited to have proper address as the P1AM uses the W5500.h internet driver)
#include <P1AM_Functions.h>   // Custom for writing commands to P1AM
#include "utility/w5100.h"    // Used for retransmission count and timeou
#include <P1AM.h>             // For P1AM
#include <I2C_RTC.h>
#include <Ponoor_PowerSTEP01Library.h>
#include <SPI.h>


//***********************************************************************Defines***********************************************************************
//P1-4ADL2DAL-2
#define ANALOG_SLOT 3
//CHANNELS
#define ANALOG_OUTPUT_CHANNEL1 2
//P1-08TRS
#define OUTPUT_SLOT 1
//CHANNELS
#define STOP_START_OUTPUT_CHANNEL 1
#define OUT_OUTPUT_CHANNEL 2
#define IN_OUTPUT_CHANNEL 3
#define LIMIT_LIGHT_OUTPUT_CHANNEL 4

#define nCS_PIN 7
#define STCK_PIN 6
#define nSTBY_nRESET_PIN 5
#define nBUSY_PIN 3
#define RETURN_ACC 1000  // Acceleration for return movement
#define RETURN_DEC 1000  // Deceleration for return movement

powerSTEP driver(0, nCS_PIN, nSTBY_nRESET_PIN);
DS3231 RTC;

//******************************************************************Function Prototypes***********************************************************************

// Global Variables
int timeInterval, repeats, stepLengthMicrosteps, maxMicrosteps;
char timeUnit[10], actionAtMax[10];
bool startNow, returnTriggered = false;
int startDay, startMonth, startYear, startHours, startMinutes, startSeconds;
int totalTraveledMicrosteps = 0;
const long maxCableLengthMicrosteps = 1024000 ;
const float MICROSTEPS_PER_METER = 1024000.0 / 5000.0;  // 204.8 microsteps/m
const int RETURN_SPEED = 500;                          // Slower speed for return movement

// Non-blocking control variables
bool isMoving = false;
bool routineComplete = false;  // Added to track routine completion
unsigned long intervalStartTime;
int currentRepeat = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("Stepper Motor Control Program Starting...");

  // Initialize hardware
  pinMode(nSTBY_nRESET_PIN, OUTPUT);
  pinMode(nCS_PIN, OUTPUT);
  digitalWrite(nSTBY_nRESET_PIN, HIGH);
  digitalWrite(nCS_PIN, HIGH);

  SPI.begin();
  SPI.setDataMode(SPI_MODE3);
  driver.SPIPortConnect(&SPI);

  // Configure motor
  driver.configStepMode(STEP_FS_128);
  driver.setMaxSpeed(1000);
  driver.setFullSpeed(2000);
  driver.setAcc(2000);
  driver.setDec(2000);
  driver.setSlewRate(SR_520V_us);
  driver.setOCThreshold(8);
  driver.setOCShutdown(OC_SD_ENABLE);
  driver.setPWMFreq(PWM_DIV_1, PWM_MUL_0_75);
  driver.setVoltageComp(VS_COMP_DISABLE);
  driver.setSwitchMode(SW_USER);
  driver.setOscMode(INT_16MHZ);
  driver.setRunKVAL(64);
  driver.setAccKVAL(64);
  driver.setDecKVAL(64);
  driver.setHoldKVAL(8);
  driver.setParam(ALARM_EN, 0x8F);
  driver.getStatus();

  Serial.println("Motor initialized successfully.");

  initializeRTC();
  setRTCTime();
  handleUserInput();
  waitForStartTime();
  Serial.println("Starting routine...");
}

void loop() {
  if (!routineComplete) {
    if (!returnTriggered) {
      static unsigned long intervalMillis = 0;
      static unsigned long movementStartTime = 0;

      // Calculate interval once
      if (intervalMillis == 0) {
        intervalMillis = timeInterval * 1000UL;  // Base in seconds
        if (strcmp(timeUnit, "minutes") == 0) intervalMillis *= 60;
        else if (strcmp(timeUnit, "hours") == 0) intervalMillis *= 3600;
        else if (strcmp(timeUnit, "days") == 0) intervalMillis *= 86400;
      }

      if (!isMoving) {
        if (currentRepeat < repeats) {
          // Check interval AFTER previous movement completed
          if (millis() - movementStartTime >= intervalMillis || currentRepeat == 0) {
            executeRoutine();
            movementStartTime = millis();  // Start timing after movement begins
            currentRepeat++;               // Increment here to avoid double-counting
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
  }
}

// Date handling functions
void adjustStartDate() {
  int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };  // Removed 'const'
  while (startDay > daysInMonth[startMonth - 1]) {
    // Update February days dynamically
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
      // Update for new year's February
      if ((startYear % 4 == 0 && startYear % 100 != 0) || (startYear % 400 == 0)) {
        daysInMonth[1] = 29;
      } else {
        daysInMonth[1] = 28;
      }
    }
  }
}

void initializeRTC() {
  RTC.begin();
  RTC.setHourMode(false);
  Serial.println("RTC initialized successfully.");
}

void setRTCTime() {
  Serial.println("Set current RTC time:");
  int hours = getValidatedInput("Enter Hours (0-23): ", 0, 23);
  int minutes = getValidatedInput("Enter Minutes (0-59): ", 0, 59);
  int seconds = getValidatedInput("Enter Seconds (0-59): ", 0, 59);
  int day = getValidatedInput("Enter Day (1-31): ", 1, 31);
  int month = getValidatedInput("Enter Month (1-12): ", 1, 12);
  int year = getValidatedInput("Enter Year (e.g., 2025): ", 2000, 2099);

  RTC.setYear(year - 2000);
  RTC.setMonth(month);
  RTC.setDay(day);
  RTC.setHours(hours);
  RTC.setMinutes(minutes);
  RTC.setSeconds(seconds);
}

void handleUserInput() {
  while (true) {
    timeInterval = getValidatedInput("Enter Time Interval (1-59): ", 1, 59);

    const char* timeUnitChoices[] = { "seconds", "minutes", "hours", "days" };
    getValidatedChoice("Time Unit (seconds/minutes/hours/days): ", timeUnitChoices, 4, timeUnit);

    repeats = getValidatedInput("Number of Repeats (1-100): ", 1, 100);

    int cableLengthMeters = getValidatedInput("Total Cable Length (m) (10-5000): ", 10, 5000);
    maxMicrosteps = round(cableLengthMeters * MICROSTEPS_PER_METER);

    int stepLengthMeters = getValidatedInput("Step Length (m): ", 1, cableLengthMeters);
    stepLengthMicrosteps = round(stepLengthMeters * MICROSTEPS_PER_METER);

    if (stepLengthMicrosteps * repeats > maxMicrosteps) {
      Serial.println("Total movement exceeds cable length!");
      continue;
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
      int delaySeconds = (int)(delayHours * 3600);

      int currentSeconds = RTC.getSeconds() + RTC.getMinutes() * 60 + RTC.getHours() * 3600;
      int totalSeconds = currentSeconds + delaySeconds;

      startHours = totalSeconds / 3600;
      startMinutes = (totalSeconds % 3600) / 60;
      startSeconds = totalSeconds % 60;
      startDay = RTC.getDay();
      startMonth = RTC.getMonth();
      startYear = RTC.getYear() + 2000;

      if (startHours >= 24) {
        startHours -= 24;
        startDay++;
        adjustStartDate();
      }
    }
    break;
  }
}

void waitForStartTime() {
  if (startNow) return;

  Serial.println("Waiting for start time...");
  while (true) {
    if (RTC.getYear() + 2000 == startYear && RTC.getMonth() == startMonth && RTC.getDay() == startDay && RTC.getHours() == startHours && RTC.getMinutes() == startMinutes && RTC.getSeconds() >= startSeconds) {
      break;
    }
    delay(50);
  }
}

// Modify executeRoutine() and returnToStart()
void executeRoutine() {
  if (!isMoving) {
    // Store original parameters
    int originalSpeed = driver.getMaxSpeed();

    // Execute move
    moveMotor(stepLengthMicrosteps, false);
    totalTraveledMicrosteps += stepLengthMicrosteps;
    displayRealTimeFeedback(currentRepeat + 1, repeats);

    // Immediate restore of speed (accel/decel maintained)
    driver.setMaxSpeed(originalSpeed);
  }
}

void returnToStart() {
  static int originalAcc, originalDec, originalSpeed;
  static int returnMicrosteps = 0;  // Store distance before resetting

  if (!isMoving) {
    // Save original parameters
    originalAcc = driver.getAcc();
    originalDec = driver.getDec();
    originalSpeed = driver.getMaxSpeed();
    returnMicrosteps = totalTraveledMicrosteps;  // Store accumulated distance

    // Configure return movement
    driver.setMaxSpeed(RETURN_SPEED);
    driver.setAcc(RETURN_ACC);
    driver.setDec(RETURN_DEC);

    // Initiate return with stored distance
    moveMotor(returnMicrosteps, true);
  } else {
    if (!driver.busyCheck()) {
      // Restore parameters and reset AFTER movement completes
      driver.setAcc(originalAcc);
      driver.setDec(originalDec);
      driver.setMaxSpeed(originalSpeed);
      totalTraveledMicrosteps = 0;  // Reset only after successful return

      returnTriggered = false;
      routineComplete = true;
      Serial.println("Return complete. Speed restored.");
    }
  }
}

void moveMotor(int microsteps, bool reverse) {
  if (reverse) driver.move(REV, microsteps);
  else driver.move(FWD, microsteps);
  isMoving = true;
}

void monitorMovement() {
  if (!driver.busyCheck()) {
    isMoving = false;
    driver.softStop();
    // Only reset interval timer when movement actually completes
    intervalStartTime = millis();
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
    while (Serial.available() == 0)
      ;
    value = Serial.parseInt();
    if (value >= min && value <= max) return value;
    Serial.println("Invalid input. Please try again.");
  }
}

void getValidatedChoice(const char* prompt, const char* choices[], int numChoices, char* output) {
  char input[10] = { 0 };  // Initialize buffer to zeros
  while (true) {
    Serial.print(prompt);

    // Wait for data to be available
    while (!Serial.available())
      ;

    // Read and trim input
    size_t bytesRead = Serial.readBytesUntil('\n', input, sizeof(input) - 1);
    input[bytesRead] = 0;  // Null-terminate

    // Trim trailing whitespace and control characters
    for (int i = strlen(input) - 1; i >= 0; i--) {
      if (isspace(input[i]) || input[i] == '\r') input[i] = 0;
      else break;
    }

    // Debug output (uncomment for testing)
    // Serial.print("Received: ");
    // Serial.println(input);

    // Compare with valid choices
    for (int i = 0; i < numChoices; i++) {
      if (strcasecmp(input, choices[i]) == 0) {
        strncpy(output, choices[i], 9);
        output[9] = 0;  // Ensure null termination
        return;
      }
    }
    Serial.println("Invalid choice. Try again.");
  }
}