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

// Ethernet and server settings
byte mac[] = { 0x12, 0x56, 0x78, 0x9A, 0xBC, 0xEF };  // MAC address
IPAddress ip(10, 0, 0, 200);                          // Static IP
EthernetServer server(80);                            // HTTP server on port 80

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
const float MOTOR_SPEED = 5.0;        // Motor speed in meters per second (5V = 5 m/s)
unsigned long movementStartTime = 0;  // Tracks when the motor started moving
float currentPayout = 0.0;            // Tracks the current payout in meters
int stepLengthMeters;                 // Step length in meters
int maxCableLength;                   // Maximum cable length in meters

// RTC Scheduling Variables
DateTime nextTriggerTime;   // Store next scheduled run time
TimeSpan intervalSpan;      // Store user interval as TimeSpan
DateTime routineStartTime;  // Track when the routine should start

// Non-blocking control variables
bool isMoving = false;
bool routineComplete = false;
int currentRepeat = 0;
const int chipSelect = 28;

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial (optional)

  // Initialize motor controller with timeout
  Serial.println("Initializing motor controller...");
  unsigned long motorTimeout = millis();
  while (!P1.init()) {
    if (millis() - motorTimeout > 5000) {
      Serial.println("Motor init failed. Retrying...");
      motorTimeout = millis();
    }
  }

  // Initialize Ethernet with DHCP fallback
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Using static IP (DHCP failed).");
    Ethernet.begin(mac, ip); // Assign static IP
  }

  // Check Ethernet hardware
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet hardware not found!");
    while (true); // Halt
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable disconnected!");
  }

  // Initialize SD card
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card failed. Logging disabled.");
  }

  // Start the server
  server.begin();
  Serial.print("Server IP: ");
  Serial.println(Ethernet.localIP());

  initializeRTC();      // Initialize RTC
  initializeProgram();  // Collect initial parameters
}

void loop() {
  // Handle Ethernet client non-blockingly
  EthernetClient client = server.available();
  if (client) {
    Serial.println("Client connected.");
    String currentLine = "";
    bool isPost = false;
    String postData = "";

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        if (isPost) {
          postData += c;
        }

        // End of HTTP request
        if (c == '\n' && currentLine.length() == 0) {
          if (isPost) {
            handlePostRequest(client, postData);
          } else {
            serveHtml(client);
          }
          break;
        }

        // Identify request type
        if (currentLine.startsWith("POST")) {
          isPost = true;
        }

        // Track current line
        if (c == '\n') {
          currentLine = "";
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }

    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
  }

  // Handle routine execution
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
    // Non-blocking reset after routine completion
    static bool waitingForInput = false;
    if (!waitingForInput) {
      Serial.println("Routine completed. Press any key to start a new schedule.");
      waitingForInput = true;
    }
    if (Serial.available() > 0) {
      while (Serial.available() > 0) Serial.read();  // Clear buffer
      resetProgramState();
      initializeProgram();
      routineComplete = false;
      waitingForInput = false;
    }
  }

  monitorMovement();  // Continuously monitor movement
}


void serveHtml(EthernetClient& client) {
  File htmlFile = SD.open("SCHEDULE.htm");
  if (htmlFile) {
    Serial.println("Serving SCHEDULE.htm");
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    while (htmlFile.available()) {
      client.write(htmlFile.read());
    }
    htmlFile.close();
  } else {
    Serial.println("404: SCHEDULE.htm not found");
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("404 File Not Found");
  }
}

void handlePostRequest(EthernetClient& client, String& postData) {
  Serial.println("POST data received:");
  Serial.println(postData);

  // Example: Parse JSON data (use ArduinoJson for better parsing)
  if (postData.indexOf("\"mode\":\"Time\"") > -1) {
    Serial.println("Mode: Time");
  } else if (postData.indexOf("\"mode\":\"Trigger\"") > -1) {
    Serial.println("Mode: Trigger");
  }

  // Respond to the client
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.println("Task received and processed.");
}

void scheduleNextRun(DateTime lastEndTime) {
  nextTriggerTime = lastEndTime + intervalSpan;  // No extra time added
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

  const char* timeUnitChoices[] = { "seconds", "minutes", "hours", "days" };
  getValidatedChoice("Time Unit (seconds/minutes/hours/days): ", timeUnitChoices, 4, timeUnit);

  // Convert time unit to TimeSpan
  if (strcmp(timeUnit, "seconds") == 0) {
    intervalSpan = TimeSpan(timeInterval);
  } else if (strcmp(timeUnit, "minutes") == 0) {
    intervalSpan = TimeSpan(0, 0, timeInterval, 0);
  } else if (strcmp(timeUnit, "hours") == 0) {
    intervalSpan = TimeSpan(0, timeInterval, 0, 0);
  } else if (strcmp(timeUnit, "days") == 0) {
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
    uint32_t delaySeconds = (uint32_t)(delayHours * 3600);
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
  while (true) {
    DateTime now = rtc.now();
    if (now.year() == startYear && now.month() == startMonth && now.day() == startDay && now.hour() == startHours && now.minute() == startMinutes && now.second() >= startSeconds) {
      break;
    }
    delay(50);
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
  P1.writeAnalog(2048, ANALOG_SLOT, ANALOG_OUTPUT_CHANNEL1);
  movementStartTime = rtc.now().unixtime();
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
    startMovement(true);
  } else {
    monitorMovement();

    // Check if we've returned the full distance
    if (currentPayout >= totalDistanceTraveled) {
      stopMovement();
      returnTriggered = false;
      routineComplete = true;
      Serial.println("Successfully returned to start position");
    }
  }
}

void monitorMovement() {
  if (isMoving) {
    DateTime now = rtc.now();
    unsigned long elapsed = now.unixtime() - movementStartTime;
    currentPayout = MOTOR_SPEED * elapsed;

    if (currentPayout >= stepLengthMeters) {
      stopMovement();
      totalDistanceTraveled += currentPayout;
      DateTime endTime = rtc.now();
      scheduleNextRun(endTime);  // Schedule next run after movement ends
      currentRepeat++;           // Increment after movement completes
      Serial.print("Movement completed. Total traveled: ");
      Serial.print(totalDistanceTraveled);
      Serial.println(" meters");
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