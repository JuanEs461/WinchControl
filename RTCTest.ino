#include <Wire.h>
#include <RTClib.h>  // For DS3231 RTC
#include <Ponoor_PowerSTEP01Library.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <stdio.h>

#define nCS_PIN 7
#define STCK_PIN 6
#define nSTBY_nRESET_PIN 5
#define nBUSY_PIN 3
#define RETURN_ACC 1000  // Acceleration for return movement
#define RETURN_DEC 1000  // Deceleration for return movement

LiquidCrystal_I2C lcd(0x27, 16, 2);  // Initialize LCD

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

// LCD update variables
unsigned long lastLCDUpdate = 0;
const long lcdInterval = 1000;  // Update LCD every 1 second

void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;           // Wait for Serial Monitor to open
  delay(1000);  // Small delay to ensure Serial Monitor is ready

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Sys Initialized");

  Serial.println("Stepper Motor Control Program Starting...");

  // Prepare pins
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

  // Configure powerSTEP
  driver.SPIPortConnect(&SPI);  // give library the SPI port (only the one on an Uno)

  driver.configSyncPin(BUSY_PIN, 0);  // use SYNC/nBUSY pin as nBUSY,
                                      // thus syncSteps (2nd paramater) does nothing

  driver.configStepMode(STEP_FS_128);  // 1/128 microstepping, full steps = STEP_FS,
                                       // options: 1, 1/2, 1/4, 1/8, 1/16, 1/32, 1/64, 1/128

  driver.setMaxSpeed(1000);   // max speed in units of full steps/s
  driver.setFullSpeed(2000);  // full steps/s threshold for disabling microstepping
  driver.setAcc(2000);        // full steps/s^2 acceleration
  driver.setDec(2000);        // full steps/s^2 deceleration

  driver.setSlewRate(SR_520V_us);  // faster may give more torque (but also EM noise),
                                   // options are: 114, 220, 400, 520, 790, 980(V/us)

  driver.setOCThreshold(8);            // over-current threshold for the 2.8A NEMA23 motor
                                       // used in testing. If your motor stops working for
                                       // no apparent reason, it's probably this. Start low
                                       // and increase until it doesn't trip, then maybe
                                       // add one to avoid misfires. Can prevent catastrophic
                                       // failures caused by shorts
  driver.setOCShutdown(OC_SD_ENABLE);  // shutdown motor bridge on over-current event
                                       // to protect against permanant damage

  driver.setPWMFreq(PWM_DIV_1, PWM_MUL_0_75);  // 16MHz*0.75/(512*1) = 23.4375kHz
                                               // power is supplied to stepper phases as a sin wave,
                                               // frequency is set by two PWM modulators,
                                               // Fpwm = Fosc*m/(512*N), N and m are set by DIV and MUL,
                                               // options: DIV: 1, 2, 3, 4, 5, 6, 7,
                                               // MUL: 0.625, 0.75, 0.875, 1, 1.25, 1.5, 1.75, 2

  driver.setVoltageComp(VS_COMP_DISABLE);  // no compensation for variation in Vs as
                                           // ADC voltage divider is not populated

  driver.setSwitchMode(SW_USER);  // switch doesn't trigger stop, status can be read.
                                  // SW_HARD_STOP: TP1 causes hard stop on connection
                                  // to GND, you get stuck on switch after homing

  driver.setOscMode(INT_16MHZ);  // 16MHz internal oscillator as clock source

  // KVAL registers set the power to the motor by adjusting the PWM duty cycle,
  // use a value between 0-255 where 0 = no power, 255 = full power.
  // Start low and monitor the motor temperature until you find a safe balance
  // between power and temperature. Only use what you need
  driver.setRunKVAL(64);
  driver.setAccKVAL(64);
  driver.setDecKVAL(64);
  driver.setHoldKVAL(8);

  driver.setParam(ALARM_EN, 0x8F);  // disable ADC UVLO (divider not populated),
                                    // disable stall detection (not configured),
                                    // disable switch (not using as hard stop)

  driver.getStatus();  // clears error flags

  Serial.println("Motor initialized successfully.");

  initializeRTC();
  setRTCTime();       // Ask for time first
  handleUserInput();  // Then ask for other parameters
  waitForStartTime();
  Serial.println("Starting routine...");
}

void loop() {

  if (!routineComplete) {
    if (!returnTriggered) {
      DateTime now = rtc.now();  // Get current RTC time

      if (!isMoving) {
        if (currentRepeat < repeats) {
          // First run or interval elapsed
          if (currentRepeat == 0 || now >= nextTriggerTime) {
            executeRoutine();
            scheduleNextRun(now);  // Schedule next run
            currentRepeat++;
          }
        } else {
          handleRoutineCompletion();
        }
      } else {
        monitorMovement();  // Check if movement is complete
      }
    } else {
      returnToStart();
    }
  } else {
    // Reset state and restart input process
    routineComplete = false;
    returnTriggered = false;
    currentRepeat = 0;
    totalTraveledMicrosteps = 0;

    // Clear serial buffer
    while (Serial.available()) Serial.read();

    // Restart input process
    Serial.println("\n\n--- New Routine Setup ---");
    handleUserInput();
    waitForStartTime();
    Serial.println("Starting new routine...");
  }
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


void updateLCDTime() {
  if (millis() - lastLCDUpdate >= lcdInterval) {
    DateTime now = rtc.now();

    // Format time display
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Time: ");
    if (now.hour() < 10) lcd.print("0");
    lcd.print(now.hour());
    lcd.print(":");
    if (now.minute() < 10) lcd.print("0");
    lcd.print(now.minute());
    lcd.print(":");
    if (now.second() < 10) lcd.print("0");
    lcd.print(now.second());

    // Format date display
    lcd.setCursor(0, 1);
    lcd.print("Date: ");
    if (now.day() < 10) lcd.print("0");
    lcd.print(now.day());
    lcd.print("/");
    if (now.month() < 10) lcd.print("0");
    lcd.print(now.month());
    lcd.print("/");
    lcd.print(now.year());

    lastLCDUpdate = millis();
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

  // LCD display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  if (now.hour() < 10) lcd.print("0");
  lcd.print(now.hour());
  lcd.print(":");
  if (now.minute() < 10) lcd.print("0");
  lcd.print(now.minute());
  lcd.print(":");
  if (now.second() < 10) lcd.print("0");
  lcd.print(now.second());

  lcd.setCursor(0, 1);
  lcd.print("Date: ");
  if (now.day() < 10) lcd.print("0");
  lcd.print(now.day());
  lcd.print("/");
  if (now.month() < 10) lcd.print("0");
  lcd.print(now.month());
  lcd.print("/");
  lcd.print(now.year());

  delay(2000);  // Display for 2 seconds
}


void handleUserInput() {
  while (true) {
    // Get time interval
    timeInterval = getValidatedInput("Enter Time Interval (1-59): ", 1, 59);

    // Get time unit
    const char* timeUnitChoices[] = { "s", "m", "h", "d" };
    getValidatedChoice("Time Unit (s(seconds)/m(minutes)/h(hours)/d(days)): ", timeUnitChoices, 4, timeUnit);

    int val = getValidatedInput("enter time", int min, int max)

    SLEEP_MODE_ADC 

    // Calculate intervalSpan based on user input
    if (strcmp(timeUnit, "s") == 0) {
      intervalSpan = TimeSpan(timeInterval);
    } else if (strcmp(timeUnit, "m") == 0) {
      intervalSpan = TimeSpan(0, 0, timeInterval, 0);
    } else if (strcmp(timeUnit, "h") == 0) {
      intervalSpan = TimeSpan(0, timeInterval, 0, 0);
    } else if (strcmp(timeUnit, "d") == 0) {
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
    moveMotor(stepLengthMicrosteps, false);
    totalTraveledMicrosteps += stepLengthMicrosteps;
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

void monitorMovement() {
  if (!driver.busyCheck()) {
    isMoving = false;
    driver.softStop();
    Serial.println("Movement completed.");
    scheduleNextRun(rtc.now());
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
    // max value
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