#include <Wire.h>
#include <RTClib.h>  // For DS3231 RTC
#include <Ponoor_PowerSTEP01Library.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>

#define nCS_PIN 7
#define STCK_PIN 6
#define nSTBY_nRESET_PIN 5
#define nBUSY_PIN 3
#define RETURN_ACC 1000  // Acceleration for return movement
#define RETURN_DEC 1000  // Deceleration for return movement

// Encoder Pin Definitions
#define ENCODER_CLK_PIN 13
#define ENCODER_DT_PIN 12

powerSTEP driver(0, nCS_PIN, nSTBY_nRESET_PIN);
RTC_DS3231 rtc;  // RTClib object

// Global Variables
int timeInterval, repeats;
long stepLengthMicrosteps, maxMicrosteps;  // Changed to long
char timeUnit[10], actionAtMax[10];
bool startNow, returnTriggered = false;
int startDay, startMonth, startYear, startHours, startMinutes, startSeconds;
long totalTraveledMicrosteps = 0;                      // Changed to long to avoid overflow
const float MICROSTEPS_PER_METER = 204800.0 / 1000.0;  // 204.8 microsteps/m
const int RETURN_SPEED = 500;                          // Slower speed for return movement

// RTC Scheduling Variables
DateTime nextTriggerTime;  // Store next scheduled run time
TimeSpan intervalSpan;     // Store user interval as TimeSpan

// Non-blocking control variables
bool isMoving = false;
bool routineComplete = false;
int currentRepeat = 0;

// Encoder Variables
volatile int encoderPos = 0;  // Tracks encoder position
int lastEncoded = 0;

// Encoder Parameters
const int ENCODER_PPR = 18;               // Encoder pulses per revolution (18 P/R for the test encoder)
const float METERS_PER_REVOLUTION = 1.0;  // 1 meter per full encoder rotation

// Motor Control Variables
float targetDistance = 0;   // Target distance in meters
float currentDistance = 0;  // Current distance in meters

// Encoder (Values are defaults for if there is no SD card inserted)
float cableDiameter = 0.00818;
float cableLength = 0;
float drumDiameter = 0.305;
float drumWidth = 0.324;
float scaleFactor = 1;
float stretchFactor = 1;
int drumEncRes = 18;  // Changed to 256 P/R for the actual encoder
float offset = 0;
float trueDiameter;
int encoderCount = 0;
int encoderCurrentCount;
int encoderPreviousCount;
int retainCount;
int drumCountValue;
int drumTurns;
int drumTurnsPerLayer;
float partialDrumTurn;
float calcCablePayout = 0;
float cablePayout = 0;
float cableSpeed = 0;
float previousCablePayout = 0;
bool drumCountNegative, drumDirectionNegative, resetEncoder, resetQuad1;
float maxCablePayedOut = 5000;  // Default min and max values that are set until the user reprograms them
float minCablePayedOut = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;           // Wait for Serial Monitor to open
  delay(1000);  // Small delay to ensure Serial Monitor is ready

  Serial.println("Stepper Motor Control Program Starting...");

  // Initialize hardware
  pinMode(nSTBY_nRESET_PIN, OUTPUT);
  pinMode(nCS_PIN, OUTPUT);
  pinMode(MOSI, OUTPUT);
  pinMode(MISO, OUTPUT);
  pinMode(SCK, OUTPUT);

  // Reset powerSTEP and set CS
  digitalWrite(nSTBY_nRESET_PIN, HIGH);
  digitalWrite(nSTBY_nRESET_PIN, LOW);
  digitalWrite(nSTBY_nRESET_PIN, HIGH);
  digitalWrite(nCS_PIN, HIGH);

  // Start SPI
  SPI.begin();
  SPI.setDataMode(SPI_MODE3);

  driver.SPIPortConnect(&SPI);
  driver.configSyncPin(BUSY_PIN, 0);
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

  // Initialize encoder pins
  pinMode(ENCODER_CLK_PIN, INPUT);
  pinMode(ENCODER_DT_PIN, INPUT);

  // Read initial encoder state
  lastEncoded = digitalRead(ENCODER_CLK_PIN) | (digitalRead(ENCODER_DT_PIN) << 1);

  initializeRTC();
  setRTCTime();       // Ask for time first
  handleUserInput();  // Then ask for other parameters
  waitForStartTime();
  Serial.println("Starting routine...");
}

void loop() {
  updateEncoderISR();
  monitorMotorMovement();
  if (!routineComplete) {
    DateTime now = rtc.now();
    if (!isMoving && (currentRepeat < repeats) && (currentRepeat == 0 || now >= nextTriggerTime)) {
      executeRoutine();
      scheduleNextRun(now);
      currentRepeat++;
    } else if (currentRepeat >= repeats) {
      handleRoutineCompletion();
    }
  } else {
    routineComplete = returnTriggered = false;
    encoderPos = 0;
    Serial.println("New Routine Setup");
    handleUserInput();
    waitForStartTime();
  }
}

// Function to update encoder position
void updateEncoderISR() {
  int MSB = digitalRead(ENCODER_CLK_PIN);
  int LSB = digitalRead(ENCODER_DT_PIN);
  int encoded = (MSB << 1) | LSB;
  int sum = (lastEncoded << 2) | encoded;

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderPos++;
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderPos--;

  lastEncoded = encoded;
}

void initializeEncoder() {
  pinMode(ENCODER_CLK_PIN, INPUT);
  pinMode(ENCODER_DT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK_PIN), updateEncoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_DT_PIN), updateEncoderISR, CHANGE);
  lastEncoded = digitalRead(ENCODER_CLK_PIN) | (digitalRead(ENCODER_DT_PIN) << 1);
  Serial.println("Encoder initialized successfully.");
}

// Function to calculate current distance based on encoder position
float getCurrentDistance() {
  float revolutions = encoderPos / (float)ENCODER_PPR;
  return revolutions * METERS_PER_REVOLUTION;
}
// Function to move the motor until the target distance is reached
void moveMotorToDistance(float distance) {
  targetDistance = distance;
  isMoving = true;
  driver.move(FWD, 1000);  // Move initial steps
  Serial.print("Motor started. Target distance: ");
  Serial.println(targetDistance);
}

float GetCablePayout() {
  encoderCount = encoderPos;
  //encoderCount = HSC.CNT1.readPosition(); //Working with P1AM High speed counnter
  encoderCurrentCount = encoderCount;
  if (encoderCurrentCount > encoderPreviousCount) {
    retainCount = retainCount + (encoderCurrentCount - encoderPreviousCount);
  } else {
    retainCount = retainCount - (encoderPreviousCount - encoderCurrentCount);
  }
  encoderPreviousCount = encoderCurrentCount;
  // Avoid a potential divide by zero in the following calculations
  if (drumEncRes == 0) {
    drumEncRes = 18;  //set encoder resolution
  }
  // Handle a possible negative pay-in
  if (retainCount < 0) {
    drumCountValue = abs(retainCount);
    drumCountNegative = true;
  } else {
    drumCountValue = retainCount;
    drumCountNegative = false;
  }
  // Use total counts and counts per rotation to figure out number of rotations.
  drumTurns = float(drumCountValue) / drumEncRes;
  // retainCount - (drumTurns * drumEncRes) = counts for a final partial turn
  partialDrumTurn = (float(drumCountValue) / drumEncRes) - drumTurns;
  // Drum width divided by cable diameter = cable turns per layer.
  drumTurnsPerLayer = drumWidth / cableDiameter;
  // Drum diameter + cable outside diameter = true diameter of first layer to center of cable.
  trueDiameter = drumDiameter + cableDiameter;
  // Initialize cable payout to zero
  calcCablePayout = 0;
  // Calculate contribution of complete cable turns on full layers.
  if (drumTurnsPerLayer != 0) {
    while (drumTurns >= drumTurnsPerLayer) {
      // Calculate cable length in current layer and add to total.
      calcCablePayout = calcCablePayout + (drumTurnsPerLayer * trueDiameter * 3.141593);

      // Calculate drum diameter to center of next cable layer.
      trueDiameter = trueDiameter + (2 * cableDiameter);

      // Finished with layer so subtract layer turns from total number remaining.
      drumTurns = drumTurns - drumTurnsPerLayer;
    }
  }
  // Calculate contribution of any remaining turns on final layer.
  if (drumTurns != 0) {
    calcCablePayout = calcCablePayout + (drumTurns * trueDiameter * 3.141593);
  }
  // Calculate contribution of any remaining partial turn.
  calcCablePayout = calcCablePayout + (partialDrumTurn * trueDiameter * 3.141593);
  // Calculate cable payout with scaling factor (1 = no scaling);
  calcCablePayout = calcCablePayout * scaleFactor;
  // Adjust sign for initial calibration drum direction.
  if (drumDirectionNegative == true) {
    calcCablePayout = calcCablePayout * -1;
  }
  if (drumCountNegative == false) {
    calcCablePayout = calcCablePayout * -1;
  }
  //Calculate cable payout starting with full drum amount.*********************************************************************
  calcCablePayout = cableLength + calcCablePayout;
  // Calculate cable payout with stretching factor (1 = no stretching).
  calcCablePayout = calcCablePayout * stretchFactor;
  // Calculate cable speed based on 1 second timer.
  return calcCablePayout;
}

void scheduleNextRun(DateTime lastEndTime) {
  nextTriggerTime = lastEndTime + intervalSpan;
  Serial.print("Next run scheduled for: ");
  printDateTime(nextTriggerTime);
}

// Add this helper function to print date/time
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
  rtc.begin();

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, resetting time to compile time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Set RTC to compile-time
  }

  Serial.println("RTC initialized successfully.");
}

void printPrompt(const char* prompt) {
  Serial.println(prompt);  // Print the prompt on a new line
}

void setRTCTime() {
  Serial.println("Set current RTC time:");

  // Get validated inputs
  int hours = getValidatedInput("Enter Hours (0-23): ", 0, 23);
  int minutes = getValidatedInput("Enter Minutes (0-59): ", 0, 59);
  int seconds = getValidatedInput("Enter Seconds (0-59): ", 0, 59);
  int day = getValidatedInput("Enter Day (1-31): ", 1, 31);
  int month = getValidatedInput("Enter Month (1-12): ", 1, 12);
  int year = getValidatedInput("Enter Year (2000-2099): ", 2000, 2099);

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

  delay(2000);  // Display for 2 seconds
}


void handleUserInput() {
  while (true) {
    // Get time interval
    timeInterval = getValidatedInput("Enter Time Interval (1-59): ", 1, 59);

    // Get time unit
    const char* timeUnitChoices[] = { "seconds", "minutes", "hours", "days" };
    getValidatedChoice("Time Unit (seconds/minutes/hours/days): ", timeUnitChoices, 4, timeUnit);

    // Calculate intervalSpan based on user input
    if (strcmp(timeUnit, "seconds") == 0) {
      intervalSpan = TimeSpan(timeInterval);
    } else if (strcmp(timeUnit, "minutes") == 0) {
      intervalSpan = TimeSpan(0, 0, timeInterval, 0);
    } else if (strcmp(timeUnit, "hours") == 0) {
      intervalSpan = TimeSpan(0, timeInterval, 0, 0);
    } else if (strcmp(timeUnit, "days") == 0) {
      intervalSpan = TimeSpan(timeInterval, 0, 0, 0);
    }

    // Get number of repeats
    repeats = getValidatedInput("Number of Repeats (1-100): ", 1, 100);

    // Get total cable length
    int cableLengthMeters = getValidatedInput("Total Cable Length (m) (10-5000): ", 10, 5000);
    maxMicrosteps = round(cableLengthMeters * MICROSTEPS_PER_METER);

    // Get step length with validation
    int stepLengthMeters;
    while (true) {
      stepLengthMeters = getValidatedInput("Step Length (m): ", 1, cableLengthMeters);
      stepLengthMicrosteps = round(stepLengthMeters * MICROSTEPS_PER_METER);
      if (stepLengthMicrosteps * repeats > maxMicrosteps) {
        Serial.println("Error: Total movement exceeds cable length!");
        Serial.print("Maximum allowed steps: ");
        Serial.println(maxMicrosteps / stepLengthMicrosteps);
      } else {
        break;
      }
    }

    // Get action at max
    const char* actionChoices[] = { "return", "stop" };
    getValidatedChoice("Action at max (return/stop): ", actionChoices, 2, actionAtMax);

    // Check if starting immediately
    startNow = getValidatedInput("Start Immediately? (1=Yes, 0=No): ", 0, 1);

    if (!startNow) {
      // Get delay in decimal hours
      Serial.println("Enter delay in decimal hours (e.g., 0.25 for 15 minutes):");
      char input[20];
      while (Serial.available() == 0)
        ;  // Wait for input
      Serial.readBytesUntil('\n', input, sizeof(input));
      input[strcspn(input, "\r\n")] = 0;  // Remove newline/carriage return

      // Validate decimal input
      float delayHours = atof(input);
      if (delayHours <= 0) {
        Serial.println("Invalid delay! Must be a positive number.");
        continue;
      }

      // Calculate delay in seconds
      int delaySeconds = (int)(delayHours * 3600);

      // Get current RTC time
      DateTime now = rtc.now();

      // Calculate new start time using Unix timestamp arithmetic
      DateTime startTime = now + TimeSpan(delaySeconds);  // Auto-handles overflow!

      // Extract updated start time values
      startHours = startTime.hour();
      startMinutes = startTime.minute();
      startSeconds = startTime.second();
      startDay = startTime.day();
      startMonth = startTime.month();
      startYear = startTime.year();  // Full 4-digit year

      // Handle day overflow
      if (startHours >= 24) {
        startHours -= 24;
        startDay++;
        adjustStartDate();  // Adjust date if necessary
      }

      // Display start time
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

    break;  // Exit loop after successful input
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
    float stepDistance = stepLengthMicrosteps / MICROSTEPS_PER_METER;
    moveMotorToDistance(currentDistance + stepDistance);
  }
}

void returnToStart() {
  static int originalAcc, originalDec, originalSpeed;
  static long returnMicrosteps = 0;  // Store distance before resetting

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

void moveMotor(long microsteps, bool reverse) {  // Changed parameter type to long
  if (reverse) driver.move(REV, microsteps);
  else driver.move(FWD, microsteps);
  isMoving = true;
}

/*
void monitorMovement() {
  if (!driver.busyCheck()) {
    isMoving = false;
    driver.softStop();
    Serial.println("Movement completed.");
    scheduleNextRun(rtc.now()); 
  }
}
*/

// Function to monitor motor movement
void monitorMotorMovement() {
  if (isMoving) {
    currentDistance = getCurrentDistance();  // Get the current distance
    Serial.print("Current Distance: ");
    Serial.println(currentDistance);

    if (currentDistance >= targetDistance) {
      driver.softStop();
      isMoving = false;
      Serial.println("Target distance reached.");
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
      ;  // Wait for input

    // Handle non-integer input
    while (Serial.available() > 0 && !isdigit(Serial.peek())) {
      Serial.read();  // Discard invalid characters
    }

    value = Serial.parseInt();
    Serial.println(value);  // Echo input

    if (value >= min && value <= max) return value;
    Serial.print("Invalid input! Please enter between ");
    Serial.print(min);
    Serial.print(" and ");
    Serial.println(max);
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