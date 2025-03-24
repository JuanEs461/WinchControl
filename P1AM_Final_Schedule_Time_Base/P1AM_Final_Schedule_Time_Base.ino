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
#define ANALOG_OUTPUT_CHANNEL1 1  //Speed
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
float totalDistanceTraveled = 0.0;    // Total distance traveled in meters
float MOTOR_SPEED = 3.3;              // Motor speed in meters per second (5V = 5 m/s)
unsigned long movementStartTime = 0;  // Tracks when the motor started moving
float currentPayout = 0.0;            // Tracks the current payout in meters
int stepLengthMeters;                 // Step length in meters
int maxCableLength;                   // Maximum cable length in meters
float reverseDistanceTraveled = 0.0;  // Add this to track reverse distance

// RTC Scheduling Variables
DateTime nextTriggerTime;   // Store next scheduled run time
TimeSpan intervalSpan;      // Store user interval as TimeSpan
DateTime routineStartTime;  // Track when the routine should start

// Non-blocking control variables
bool isMoving = false;
bool routineComplete = false;
int currentRepeat = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  delay(1000);
  while (!P1.init())
    ;
  Serial.println("Motor Control Program Starting...");

  initializeRTC();      // Initialize RTC once (no time set yet)
  initializeProgram();  // First-time setup
}

void loop() {
  DateTime now = rtc.now();

  if (!routineComplete) {
    if (!returnTriggered) {
      if (currentRepeat < repeats) {
        if (now >= nextTriggerTime && !isMoving) {
          executeRoutine();
        }
      } else {
        handleRoutineCompletion();
      }
    } else {
      returnToStart();
    }
  } else {
    // Reset and prompt for new input after completion
    Serial.println("Routine completed. Press any key to start a new schedule.");
    while (Serial.available() == 0) delay(50);
    while (Serial.available() > 0) Serial.read();  // Clear buffer

    resetProgramState();  // Reset scheduling parameters
    initializeProgram();  // Collect new parameters (RTC optional)
  }

  if (!returnTriggered) {  // Only monitor movement when not returning
    monitorMovement();
  }
}

void scheduleNextRun(DateTime lastEndTime) {
  // For fixed intervals from the original start time:
  // nextTriggerTime = routineStartTime + (currentRepeat * intervalSpan);

  // For intervals between end times (current behavior):
  nextTriggerTime = lastEndTime + intervalSpan;

  Serial.print("Next action scheduled for: ");
  printDateTime(nextTriggerTime);
}

void printDateTime(DateTime dt) {
  Serial.print(dt.year(), DEC);
  Serial.print('/');
  Serial.print(dt.month(), DEC);
  Serial.print('/');
  Serial.print(dt.day(), DEC);
  Serial.print(' ');
  Serial.print(dt.hour(), DEC);
  Serial.print(':');
  Serial.print(dt.minute(), DEC);
  Serial.print(':');
  Serial.println(dt.second(), DEC);
}

void resetProgramState() {
  // Reset scheduling parameters only
  timeInterval = 0;
  repeats = 0;
  memset(timeUnit, 0, sizeof(timeUnit));
  memset(actionAtMax, 0, sizeof(actionAtMax));
  startNow = false;
  returnTriggered = false;
  totalDistanceTraveled = 0.0;
  movementStartTime = 0;
  currentPayout = 0.0;
  stepLengthMeters = 0;
  maxCableLength = 0;
  intervalSpan = TimeSpan();
  routineStartTime = DateTime();
  isMoving = false;
  currentRepeat = 0;
  routineComplete = false;
}

void initializeProgram() {
  // Check if user wants to update RTC
  int updateRTC = getValidatedInput("Update RTC time? (1=Yes, 0=No): ", 0, 1);
  if (updateRTC) {
    setRTCTime();  // Only set RTC if user chooses
  } else {
    DateTime now = rtc.now();
    Serial.println("Using existing RTC time:");
    printDateTime(now);
  }

  handleUserInput();  // Collect new scheduling parameters
  waitForStartTime();
  Serial.println("Starting new routine...");
}

void executeRoutine() {
  if (!isMoving) {
    startMovement(false);  // Move forward
    displayRealTimeFeedback(currentRepeat + 1, repeats);
  }
}

void initializeRTC() {
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC!");
    while (1)
      ;
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, resetting time to compile time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  Serial.println("RTC initialized successfully.");
}

void adjustStartDate() {
  int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };  // Days in each month
  while (startDay > daysInMonth[startMonth - 1]) {
    // Update February days dynamically for leap years
    if (startMonth == 2) {
      if ((startYear % 4 == 0 && startYear % 100 != 0) || (startYear % 400 == 0)) {
        daysInMonth[1] = 29;  // Leap year
      } else {
        daysInMonth[1] = 28;  // Non-leap year
      }
    }

    if (startDay > daysInMonth[startMonth - 1]) {
      startDay -= daysInMonth[startMonth - 1];  // Subtract days of the current month
      startMonth++;                             // Move to the next month
    }

    if (startMonth > 12) {
      startMonth = 1;  // Reset to January
      startYear++;     // Increment the year
      // Update February days for the new year
      if ((startYear % 4 == 0 && startYear % 100 != 0) || (startYear % 400 == 0)) {
        daysInMonth[1] = 29;  // Leap year
      } else {
        daysInMonth[1] = 28;  // Non-leap year
      }
    }
  }
}

void setRTCTime() {
  Serial.println("Set current RTC time:");

  // Get validated inputs
  int year = getValidatedInput("Enter Year (2000-2099): ", 2000, 2099);
  int month = getValidatedInput("Enter Month (1-12): ", 1, 12);
  int day = getValidatedInput("Enter Day (1-31): ", 1, 31);
  int hours = getValidatedInput("Enter Hours (0-23): ", 0, 23);
  int minutes = getValidatedInput("Enter Minutes (0-59): ", 0, 59);
  int seconds = getValidatedInput("Enter Seconds (0-59): ", 0, 59);

  // Validate the date (e.g., February 30th is invalid)
  if (day > daysInMonth(year, month)) {
    Serial.println("Invalid date! Please enter a valid date.");
    return;
  }

  // Set RTC time
  rtc.adjust(DateTime(year, month, day, hours, minutes, seconds));

  // Display confirmation
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

// Helper function to get the number of days in a month
int daysInMonth(int year, int month) {
  if (month == 2) {
    // Check for leap year
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
      return 29;  // Leap year
    } else {
      return 28;  // Non-leap year
    }
  } else if (month == 4 || month == 6 || month == 9 || month == 11) {
    return 30;  // Months with 30 days
  } else {
    return 31;  // All other months
  }
}

void handleUserInput() {
  DateTime now = rtc.now();

  // Get time interval parameters
  timeInterval = getValidatedInput("Enter Time Interval (1-59): ", 1, 59);

  const char* timeUnitChoices[] = { "s", "m", "h", "d" };
  getValidatedChoice("Time Unit (s(seconds)/m(minutes)/h(hours)/d(days)): ", timeUnitChoices, 4, timeUnit);

  // Convert time unit to TimeSpan
  if (strcmp(timeUnit, "s") == 0) {
    intervalSpan = TimeSpan(timeInterval);
  } else if (strcmp(timeUnit, "m") == 0) {
    intervalSpan = TimeSpan(0, 0, timeInterval, 0);
  } else if (strcmp(timeUnit, "h") == 0) {
    intervalSpan = TimeSpan(0, timeInterval, 0, 0);
  } else if (strcmp(timeUnit, "d") == 0) {
    intervalSpan = TimeSpan(timeInterval, 0, 0, 0);
  }

  // Get movement parameters
  repeats = getValidatedInput("Number of Repeats (1-100): ", 1, 100);
  maxCableLength = getValidatedInput("Total Cable Length (m) (10-5000): ", 10, 5000);

  // Validate step length
  while (true) {
    stepLengthMeters = getValidatedInput("Step Length (m): ", 1, maxCableLength);
    int totalMovement = stepLengthMeters * repeats;
    if (totalMovement > maxCableLength) {
      Serial.print("Error: Total movement exceeds cable length by ");
      Serial.print(totalMovement - maxCableLength);
      Serial.println(" meters!");
      Serial.print("Maximum allowed steps: ");
      Serial.println(maxCableLength / stepLengthMeters);
    } else {
      break;
    }
  }

  // Get action at completion
  const char* actionChoices[] = { "return", "stop" };
  getValidatedChoice("Action at max (return/stop): ", actionChoices, 2, actionAtMax);

  float speed_percent = getValidatedFloatInput("Enter speed percentage (0-100): ", 0.0, 100.0);
  MOTOR_SPEED = speed_percent / 10.0;
  Serial.print("Motor speed set to: ");
  Serial.print(MOTOR_SPEED);
  Serial.println(" (0-10 scale)");

  // Get start time selection
  startNow = getValidatedInput("Start Immediately? (1=Yes, 0=No): ", 0, 1);

  if (!startNow) {
    // Get delay with decimal hour input
    float delayHours = 0;
    while (true) {
      Serial.println("Enter delay in decimal hours (e.g., 0.25 = 15 minutes):");
      while (!Serial.available())
        ;

      String input = Serial.readStringUntil('\n');
      input.trim();

      // Manual float parsing
      bool valid = true;
      int decimalIndex = input.indexOf('.');
      if (decimalIndex == -1) {
        // No decimal point, parse as integer
        delayHours = input.toInt();
      } else {
        // Parse integer and fractional parts separately
        String intPart = input.substring(0, decimalIndex);
        String fracPart = input.substring(decimalIndex + 1);

        if (intPart.length() == 0 && fracPart.length() == 0) {
          valid = false;
        } else {
          delayHours = intPart.toInt() + (fracPart.toInt() / pow(10, fracPart.length()));
        }
      }

      if (valid && delayHours > 0) {
        break;
      }
      Serial.println("Invalid input! Please enter a positive number.");
    }

    // Calculate start time using TimeSpan
    uint32_t delaySeconds = static_cast<uint32_t>(roundf(delayHours * 3600.0f));
    TimeSpan delaySpan = TimeSpan(delaySeconds);
    routineStartTime = now + delaySpan;

    Serial.println("\nScheduled Start Time:");
    Serial.print(routineStartTime.year());
    Serial.print("-");
    if (routineStartTime.month() < 10) Serial.print("0");
    Serial.print(routineStartTime.month());
    Serial.print("-");
    if (routineStartTime.day() < 10) Serial.print("0");
    Serial.print(routineStartTime.day());
    Serial.print(" ");
    if (routineStartTime.hour() < 10) Serial.print("0");
    Serial.print(routineStartTime.hour());
    Serial.print(":");
    if (routineStartTime.minute() < 10) Serial.print("0");
    Serial.print(routineStartTime.minute());
    Serial.print(":");
    if (routineStartTime.second() < 10) Serial.print("0");
    Serial.println(routineStartTime.second());
  } else {
    routineStartTime = now;
    Serial.println("\nStarting immediately at current RTC time:");
    Serial.print(now.year());
    Serial.print("-");
    if (now.month() < 10) Serial.print("0");
    Serial.print(now.month());
    Serial.print("-");
    if (now.day() < 10) Serial.print("0");
    Serial.print(now.day());
    Serial.print(" ");
    if (now.hour() < 10) Serial.print("0");
    Serial.print(now.hour());
    Serial.print(":");
    if (now.minute() < 10) Serial.print("0");
    Serial.print(now.minute());
    Serial.print(":");
    if (now.second() < 10) Serial.print("0");
    Serial.println(now.second());
  }

  // Set initial trigger time
  nextTriggerTime = routineStartTime;
}

void waitForStartTime() {
  if (startNow) return;

  Serial.println("Waiting for start time...");
  while (rtc.now() < routineStartTime) {
    delay(50);  // Check every 50ms to avoid blocking
  }
}

void startMovement(bool reverse) {
  // Stop any previous movement first
  stopMovement();

  if (reverse) {
    // Activate REV direction
    P1.writeDiscrete(HIGH, OUTPUT_SLOT, IN_OUTPUT_CHANNEL);
    P1.writeDiscrete(LOW, OUTPUT_SLOT, OUT_OUTPUT_CHANNEL);
    Serial.println("REV signal activated");
  } else {
    // Activate FWD direction
    P1.writeDiscrete(HIGH, OUTPUT_SLOT, OUT_OUTPUT_CHANNEL);
    P1.writeDiscrete(LOW, OUTPUT_SLOT, IN_OUTPUT_CHANNEL);
    Serial.println("FWD signal activated");
  }

  // Set speed and record start time
  int output_counts = (int)((MOTOR_SPEED / 10.0) * 4095);  // Convert to 0-4095 scale
  P1.writeAnalog(output_counts, ANALOG_SLOT, ANALOG_OUTPUT_CHANNEL1);
  movementStartTime = millis();  // Track time in milliseconds *************************************************************
  isMoving = true;
}
void stopMovement() {
  // Turn off all outputs
  stopSignal(OUTPUT_SLOT, STOP_START_OUTPUT_CHANNEL);  // Stop signal
  Serial.println("Stop signal activated.");

  // Explicitly turn off FWD and REV signals
  P1.writeDiscrete(LOW, OUTPUT_SLOT, OUT_OUTPUT_CHANNEL);  // Turn off FWD
  P1.writeDiscrete(LOW, OUTPUT_SLOT, IN_OUTPUT_CHANNEL);   // Turn off REV
  Serial.println("FWD and REV signals turned off.");

  P1.writeAnalog(0, ANALOG_SLOT, ANALOG_OUTPUT_CHANNEL1);  // Turn off analog output (speed)
  Serial.println("Analog output set to 0V.");

  isMoving = false;
  currentPayout = 0.0;
}

void returnToStart() {
  if (!isMoving) {
    Serial.println("Initiating return to start...");
    reverseDistanceTraveled = 0.0;
    startMovement(true);
  } else {
    unsigned long elapsed = millis() - movementStartTime;
    float elapsedSeconds = elapsed / 1000.0;
    reverseDistanceTraveled = MOTOR_SPEED * elapsedSeconds;

    if (reverseDistanceTraveled >= totalDistanceTraveled) {
      stopMovement();
      returnTriggered = false;
      routineComplete = true;
      totalDistanceTraveled = 0.0;  // Reset AFTER successful return
      Serial.println("Successfully returned to start position");
    }
  }
}


void monitorMovement() {
  if (isMoving && !returnTriggered) {
    unsigned long elapsed = millis() - movementStartTime;  // Use millis() for precision
    float elapsedSeconds = elapsed / 1000.0;
    currentPayout = MOTOR_SPEED * elapsedSeconds;

    if (currentPayout >= stepLengthMeters) {
      totalDistanceTraveled += currentPayout;  // Add FIRST
      stopMovement();                          // Resets currentPayout to 0 AFTER recording
      scheduleNextRun(rtc.now());
      currentRepeat++;
      Serial.print("Movement completed. Total traveled: ");
      Serial.print(totalDistanceTraveled);
      Serial.println(" meters");
    }
  }
}

void handleRoutineCompletion() {
  if (strcmp(actionAtMax, "return") == 0) {
    returnTriggered = true;  // Keep totalDistanceTraveled intact
  } else {
    routineComplete = true;
    totalDistanceTraveled = 0.0;  // Reset only if stopping
    Serial.println("Routine complete. Stopping.");
  }
  reverseDistanceTraveled = 0.0;
  currentPayout = 0.0;
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

float getValidatedFloatInput(const char* prompt, float min, float max) {
  float value;
  while (true) {
    Serial.print(prompt);
    while (Serial.available() == 0) delay(50);  // Wait for input
    String input = Serial.readStringUntil('\n');
    input.trim();
    char* endptr;
    value = strtof(input.c_str(), &endptr);

    // Check if conversion failed
    if (endptr == input.c_str() || *endptr != '\0') {
      Serial.println("Invalid input. Please enter a number.");
      continue;
    }

    // Check bounds
    if (value >= min && value <= max) {
      Serial.println(value);  // Echo valid input
      return value;
    } else {
      Serial.print("Invalid input. Please enter between ");
      Serial.print(min);
      Serial.print(" and ");
      Serial.println(max);
    }
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