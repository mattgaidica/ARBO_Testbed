#include <SPI.h>
#include "ArduinoLowPower.h"
#include "ICM_20948.h"
#include <RTCZero.h>
#define SPI_FREQ 1000000
RTCZero zerortc;
ICM_20948_SPI myICM;

const int ADS_PWDN = 10;
const int FRAM_CS = 38;
const int ADS_CS = 11;
const int SD_CS = 4;
const int ACCEL_CS = 5;

// Set how often alarm goes off here
const byte alarmSeconds = 3;
const byte alarmMinutes = 0;
const byte alarmHours = 0;

volatile bool alarmFlag = false; // Start awake

void setup() {
  pinMode(ADS_PWDN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(ADS_PWDN, LOW);
  digitalWrite(LED_BUILTIN, LOW);

  pinMode(ACCEL_CS, OUTPUT);
  pinMode(FRAM_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
//  digitalWrite(ACCEL_CS, HIGH);
  digitalWrite(ADS_CS, HIGH);
  digitalWrite(SD_CS, HIGH);
  digitalWrite(FRAM_CS, HIGH);

  Serial.begin(115200);
  while (!Serial) {};

  SPI.begin();
  myICM.begin(ACCEL_CS, SPI, SPI_FREQ);
  Serial.print("Accel: ");
  Serial.println( myICM.statusString() );
  myICM.sleep( true );
  myICM.lowPower( true );

  zerortc.begin(); // Set up clocks and such
  resetAlarm();  // Set alarm
  zerortc.attachInterrupt(alarmMatch); // Set up a handler for the alarm

  //  LowPower.deepSleep(5000);
}
void loop() {
  if (alarmFlag == true) {
    alarmFlag = false;  // Clear flag
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("Alarm went off - I'm awake!");
  }
  resetAlarm();  // Reset alarm before returning to sleep
  Serial.println("Alarm set, going to sleep now.");
  digitalWrite(LED_BUILTIN, LOW);
  zerortc.standbyMode();    // Sleep until next alarm match
}

void alarmMatch(void) {
  alarmFlag = true; // Set flag
}

void resetAlarm(void) {
  byte seconds = 0;
  byte minutes = 0;
  byte hours = 0;
  byte day = 1;
  byte month = 1;
  byte year = 1;

  zerortc.setTime(hours, minutes, seconds);
  zerortc.setDate(day, month, year);

  zerortc.setAlarmTime(alarmHours, alarmMinutes, alarmSeconds);
  zerortc.enableAlarm(zerortc.MATCH_HHMMSS);
}
