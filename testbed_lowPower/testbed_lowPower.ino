#include <SPI.h>
#include "ArduinoLowPower.h"
#include "ICM_20948.h"
#include <RTCZero.h>

RTCZero zerortc;
ICM_20948_SPI myICM;
SPISettings SPI_ads(SPI_FREQ, MSBFIRST, SPI_MODE1);
SPISettings SPI_fram(SPI_FREQ, MSBFIRST, SPI_MODE0);

// Set how often alarm goes off here
const byte alarmSeconds = 3;
const byte alarmMinutes = 0;
const byte alarmHours = 0;

volatile bool alarmFlag = false; // Start awake

void setup() {
  Serial.begin(115200);
  while (!Serial) {};
  SPI.begin();
  pinMode(ADS_PWDN, OUTPUT);
  pinMode(ADS_CS, OUTPUT);
  pinMode(ADS_START, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(ADS_PWDN, HIGH);
  digitalWrite(ADS_START, LOW);
  digitalWrite(LED_BUILTIN, LOW);

  pinMode(ACCEL_CS, OUTPUT);
  pinMode(FRAM_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(ACCEL_CS, HIGH);
  digitalWrite(ADS_CS, HIGH);
  digitalWrite(SD_CS, HIGH);
  digitalWrite(FRAM_CS, HIGH);

  ads_cmd(RESET); // stop data while setup
  delay(1); // needs 18clks to reset
  ads_cmd(SDATAC); // stop data while setup
  ads_wreg(0x01, 0b00000110); // config1
  ads_wreg(0x02, 0b00010000); // config2
  ads_wreg(0x03, 0b01001100); // config3
  byte chReg   = 0b00000000; // XXXXX101 for test
  byte chDis   = 0b10000001;
  ads_wreg(0x05, chDis); // ch1
  ads_wreg(0x06, chDis); // ch2
  ads_wreg(0x07, chDis); // ch3
  ads_wreg(0x08, chDis); // ch4
  ads_wreg(0x09, chDis); // ch5 disabled
  ads_wreg(0x0A, chDis); // ch6 disabled

  ads_cmd(STANDBY);
  digitalWrite(ADS_PWDN, LOW);

  myICM.begin(ACCEL_CS, SPI, SPI_FREQ);
  myICM.swReset();
  Serial.print("Accel: ");
  Serial.println( myICM.statusString() );
  myICM.sleep( true );
  myICM.lowPower( true );

  //  zerortc.begin(); // Set up clocks and such
  //  resetAlarm();  // Set alarm
  //  zerortc.attachInterrupt(alarmMatch); // Set up a handler for the alarm
  //  USBDevice.detach();
  //  LowPower.deepSleep(10000);

}
void loop() {
  //  Serial.println(ads_deviceId());
  //  ads_cmd(RDATAC);
  fram_sleep();
  delay(5000);
  fram_on();
  fram_off();
  delay(5000);
  //    ads_cmd(SDATAC);
  //  ads_cmd(STANDBY);
  //    Serial.println("STANDBY");
  //    delay(5000);
  //    ads_cmd(WAKEUP);
  //    Serial.println("WAKEUP");
  //    delay(5000);
  //    ads_cmd(RDATAC);
  //    Serial.println("READ");
  //    delay(2000);



  //  if (alarmFlag == true) {
  //    alarmFlag = false;  // Clear flag
  //    digitalWrite(LED_BUILTIN, HIGH);
  //    Serial.println("Alarm went off - I'm awake!");
  //  }
  //  resetAlarm();  // Reset alarm before returning to sleep
  //  Serial.println("Alarm set, going to sleep now.");
  //  digitalWrite(LED_BUILTIN, LOW);
  //  zerortc.standbyMode();    // Sleep until next alarm match
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

byte ads_deviceId() {
  ads_on();
  byte myBuf[4] = {RREG | DEVID, 0x00}; // stop data continuous, read 1 reg
  SPI.transfer(&myBuf, sizeof(myBuf));
  ads_off();
  return myBuf[2];
}
void ads_cmd(byte cmd) {
  ads_on();
  SPI.transfer(cmd);
  ads_off();
}
void ads_on() {
  SPI.beginTransaction(SPI_ads);
  digitalWrite(ADS_CS, LOW);
}
void ads_off() {
  digitalWrite(ADS_CS, HIGH);
  SPI.endTransaction();
}
void ads_wreg(byte rrrrr, byte data) {
  ads_on();
  byte myBuf[3] = {WREG | rrrrr, 0x00, data}; // op-code then 0x00 to write 1 register
  SPI.transfer(&myBuf, sizeof(myBuf));
  ads_off();
}

void fram_on() {
  SPI.beginTransaction(SPI_fram);
  digitalWrite(FRAM_CS, LOW);
}
void fram_off() {
  digitalWrite(FRAM_CS, HIGH);
  SPI.endTransaction();
}
void fram_sleep() {
  fram_on();
  SPI.transfer(SLEEP);
  fram_off();
}
bool fram_deviceId() {
  byte myBuf[5] = {RDID};
  byte mfgId = 0x04;
  byte contCode = 0x7F;
  byte prodId1 = 0x27;
  byte prodId2 = 0x03;
  fram_on();
  SPI.transfer(&myBuf, sizeof(myBuf));
  fram_off();
  // myBuf[0] is for the op-code
  if (myBuf[1] == mfgId && myBuf[2] == contCode && myBuf[3] == prodId1 && myBuf[4] == prodId2) {
    return true;
  } else {
    return false;
  }
}
void print_buffer(byte arry[], int sz) {
  //  for (int i = 0; i < sz; i++) {
  //    Serial.print(i); Serial.print(": ");
  //    Serial.println(arry[i], HEX);
  //  }
  for (int i = 0; i < sz; i++) {
    if (i == sz - 1) {
      Serial.println(arry[i], DEC);
    } else {
      Serial.print(arry[i], DEC);
      Serial.print("\t");
    }
  }
}
