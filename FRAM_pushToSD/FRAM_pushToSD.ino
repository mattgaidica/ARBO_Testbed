#include <SPI.h>
#include "SdFat.h" // see dataLogger SdFat example
#include "ArduinoLowPower.h"
#include <RTCZero.h>
#include "ICM_20948.h"

// Power considerations when sleeping (standby mode is deep sleep) requires
// that you watch how many peripherals are left running in standby. The
// SAMD21's internal Vreg can only supply 50uA in standby. At 25C the current
// consumption is show in the table below:
//
//     Peripheral   Current (uA)
//                   @25C       @85C
//     ----------   ----------------
//     RTC           5.056    51.898
//     TC           24.133    71.008
//     TCC          31.572    78.415
//     ADC          23.139    70.335
//     UART         28.637    75.686
//     SPI           4.987    52.221
//     I2C           4.987    52.261
//     WDT           5.007    52.548
//     AC           23.173    70.533
//     EIC           5.107    52.741
//     PTC          27.720    75.176

ICM_20948_SPI myICM;
#define FILE_BASE_NAME "XX"
#define SPI_FREQ 1000000
SPISettings mySettings(SPI_FREQ, MSBFIRST, SPI_MODE0);
RTCZero rtc;
SdFat sd;
SdFile file;

const byte STANDBY = 0x04;
const byte SDATAC = 0x11;

// op-codes
const byte WREN  = 0b00000110;
const byte WRDI  = 0b00000100;
const byte RDSR  = 0b00000101;
const byte WRSR  = 0b00000001;
const byte READ  = 0b00000011;
const byte WRITE = 0b00000010;
const byte RDID  = 0b10011111;
const byte FSTRD = 0b00001011;
const byte SLEEP = 0b10111001;
const int32_t maxMem = 0x1FFFF; // minMem = 0x00000

// ARBO TESTBED v1
const int FRAM_CS = 38;
const int ADS_CS = 11;
const int SD_CS = 4;
const int ACCEL_CS = 5;
const int FRAM_HOLD = 2;
const int ADS_PWDN = 10;
const int ADS_DRDY = 12;
const int ADS_START = 6;
const int GRN_LED = 8;
const int RED_LED = 13;
// END ARBO TESTBED

int32_t memAddr = 0;
//const int uBufSz = 500; // should have 32Kbytes SRAM, 1000 is too big, how to calculate?
//int32_t uBuf[uBufSz];
byte uBuf[4 * 500]; // 200 int32 + 4 for op-code
unsigned long StartTime;
unsigned long EndTime;

union ArrayToInteger {
  byte array[4];
  int32_t integer;
};

#define error(msg) sd.errorHalt(F(msg)) // from sdfat

void setup() {
  //  pinMode(ADS_CS, OUTPUT);
  pinMode(ACCEL_CS, OUTPUT);
  pinMode(FRAM_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(ACCEL_CS, HIGH);
  digitalWrite(ADS_CS, HIGH);
  //  digitalWrite(SD_CS, HIGH);
  digitalWrite(FRAM_CS, HIGH);

  pinMode(FRAM_HOLD, OUTPUT);
  pinMode(ADS_PWDN, OUTPUT);
  pinMode(GRN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(ADS_START, OUTPUT);
  pinMode(ACS_DRDY, INPUT);
  digitalWrite(ADS_PWDN, HIGH);
  digitalWrite(FRAM_HOLD, HIGH);
  digitalWrite(ADS_START, HIGH);
  digitalWrite(GRN_LED, LOW);
  digitalWrite(RED_LED, LOW);

  const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
  char fileName[13] = FILE_BASE_NAME "00.arbo";

  Serial.begin(115200);
  while (!Serial) {};

  if (!sd.begin(SD_CS, SD_SCK_MHZ(1))) {
    sd.initErrorHalt();
  }
  while (sd.exists(fileName)) {
    if (fileName[BASE_NAME_SIZE + 1] != '9') {
      fileName[BASE_NAME_SIZE + 1]++;
    } else if (fileName[BASE_NAME_SIZE] != '9') {
      fileName[BASE_NAME_SIZE + 1] = '0';
      fileName[BASE_NAME_SIZE]++;
    } else {
      error("Can't create file name");
    }
  }
  if (!file.open(fileName, O_WRONLY | O_CREAT | O_EXCL)) {
    error("file.open");
  }
  Serial.print(F("Logging to: "));
  Serial.println(fileName);

  //  SPI.begin();

  for (int32_t i = 0; i < 1000; i++) {
    fram_writeInt(int32Addr(i), i);
  }
  StartTime = millis();

  for (int j = 0; j < 50; j++) {
    fram_readChunk(0);
    file.write(&uBuf, sizeof(uBuf));
  }
  file.close();

  Serial.println(millis() - StartTime);

  fram_sleep();
  //  ads_cmd(SDATAC);
  //  ads_cmd(STANDBY);
  digitalWrite(ADS_PWDN, LOW);

  myICM.begin(ACCEL_CS, SPI, SPI_FREQ);
  Serial.print("Accel: ");
  Serial.println( myICM.statusString() );
  myICM.sleep( true );
  myICM.lowPower( true );

  digitalWrite(ACCEL_CS, HIGH);
  digitalWrite(ADS_CS, HIGH);
  digitalWrite(SD_CS, HIGH);
  digitalWrite(FRAM_CS, HIGH);
  //  rtc.begin();
  //  rtc.standbyMode();
  //    Serial.end();
  //  LowPower.deepSleep(5000);
  SYSCTRL->XOSC32K.reg |=  (SYSCTRL_XOSC32K_RUNSTDBY | SYSCTRL_XOSC32K_ONDEMAND); // set external 32k oscillator to run when idle or sleep mode is chosen
  REG_GCLK_CLKCTRL  |= GCLK_CLKCTRL_ID(GCM_EIC) |  // generic clock multiplexer id for the external interrupt controller
                       GCLK_CLKCTRL_GEN_GCLK1 |  // generic clock 1 which is xosc32k
                       GCLK_CLKCTRL_CLKEN;       // enable it
  while (GCLK->STATUS.bit.SYNCBUSY);              // write protected, wait for sync
  EIC->WAKEUP.reg |= EIC_WAKEUP_WAKEUPEN4;        // Set External Interrupt Controller to use channel 4 (pin 6)

  //PM->SLEEP.reg |= PM_SLEEP_IDLE_CPU;  // Enable Idle0 mode - sleep CPU clock only
  //PM->SLEEP.reg |= PM_SLEEP_IDLE_AHB; // Idle1 - sleep CPU and AHB clocks
  PM->SLEEP.reg |= PM_SLEEP_IDLE_APB; // Idle2 - sleep CPU, AHB, and APB clocks
  // It is either Idle mode or Standby mode, not both.
  SysTick->CTRL = 0;
  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;   // Enable Standby or "deep sleep" mode
  __WFI();
//  USBDevice.detach();
}

void loop() {
  //  bool fram_online = fram_deviceId();
  //  Serial.println(fram_online);
  //  fram_writeInt(memAddr, memAddr);

  //  while (memAddr < uBufSz) {
  //    //    fram_writeInt(int32Addr(memAddr), 0x01020304);
  //    uBuf[memAddr] = fram_readInt(int32Addr(memAddr));
  //    //    Serial.println(int32Addr(memAddr));
  //    //    Serial.print(int32Addr(memAddr)); Serial.print(": ");
  //    //    Serial.println(uBuf[memAddr], HEX);
  //    memAddr++;
  //  }
  //uBuf[0] = 0x01020304;
  //Serial.println(uBuf[0],HEX);

  //  memAddr = 0;

  //  delay(10000);
  //  StartTime = millis();
  //  digitalWrite(GRN_LED, LOW);
  //  delay(200);
  //  digitalWrite(GRN_LED, HIGH);
  //  delay(200);
}

int32_t int32Addr(int32_t thisInt) {
  return 4 * (thisInt);
}

void fram_writeInt(int32_t memAddr, int32_t data) {
  byte myBuf[8] = {WRITE, memAddr >> 16, memAddr >> 8, memAddr, data >> 24, data >> 16, data >> 8, data};
  fram_writeEnable();
  fram_on();
  SPI.transfer(&myBuf, sizeof(myBuf));
  fram_off();
  fram_writeDisable();
}
void fram_writeByte(int32_t memAddr, byte data) {
  byte myBuf[5] = {WRITE, memAddr >> 16, memAddr >> 8, memAddr, data};
  fram_writeEnable();
  fram_on();
  SPI.transfer(&myBuf, sizeof(myBuf));
  fram_off();
  fram_writeDisable();
}

byte *fram_readChunk(int32_t memAddr) {
  // op-code, addr, addr, addr, dummy 8-cycles, read 4 bytes (int)
  //  uBuf[0] = READ;
  //  uBuf[1] = memAddr >> 16;
  //  uBuf[2] = memAddr >> 8;
  //  uBuf[3] = memAddr;
  byte myBuf[4] = {READ, memAddr >> 16, memAddr >> 8, memAddr};
  fram_on();
  SPI.transfer(&myBuf, sizeof(myBuf));
  SPI.transfer(&uBuf, sizeof(uBuf));
  fram_off();
}

// technically could read larger data sizes
int fram_readInt(int32_t memAddr) {
  // op-code, addr, addr, addr, dummy 8-cycles, read 4 bytes (int)
  byte myBuf[8] = {READ, memAddr >> 16, memAddr >> 8, memAddr};
  fram_on();
  SPI.transfer(&myBuf, sizeof(myBuf));
  fram_off();
  ArrayToInteger converter;
  converter.array[0] = myBuf[7];
  converter.array[1] = myBuf[6];
  converter.array[2] = myBuf[5];
  converter.array[3] = myBuf[4];
  return converter.integer;
}
byte fram_readByte(int32_t memAddr) {
  // op-code, addr, addr, addr, empty for read data
  byte myBuf[5] = {READ, memAddr >> 16, memAddr >> 8, memAddr};
  fram_on();
  SPI.transfer(&myBuf, sizeof(myBuf));
  fram_off();
  return myBuf[4];
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

void fram_on() {
  SPI.beginTransaction(mySettings);
  digitalWrite(FRAM_CS, LOW);
}
void fram_off() {
  digitalWrite(FRAM_CS, HIGH);
  SPI.endTransaction();
}
void fram_sleep() {
  byte myBuf[1] = {SLEEP};
  fram_on();
  SPI.transfer(&myBuf, sizeof(myBuf));
  fram_off();
}
void fram_wake() {
  // just toggle CS to wake
  digitalWrite(FRAM_CS, LOW);
  digitalWrite(FRAM_CS, HIGH);
}
void fram_writeEnable() {
  byte myBuf[1] = {WREN};
  fram_on();
  SPI.transfer(&myBuf, sizeof(myBuf));
  fram_off();
}
void fram_writeDisable() {
  byte myBuf[1] = {WRDI};
  fram_on();
  SPI.transfer(&myBuf, sizeof(myBuf));
  fram_off();
}

void ads_cmd(byte cmd) {
  ads_on();
  SPI.transfer(&cmd, 1);
  ads_off();
}
void ads_on() {
  SPI.beginTransaction(mySettings);
  digitalWrite(ADS_CS, LOW);
}
void ads_off() {
  digitalWrite(ADS_CS, HIGH);
  SPI.endTransaction();
}

// HELPERS
void print_buffer(byte arry[], int sz) {
  for (int i = 0; i < sz; i++) {
    Serial.print(i); Serial.print(": ");
    Serial.println(arry[i], HEX);
  }
}
