#include <SPI.h>
SPISettings mySettings(1000000, MSBFIRST, SPI_MODE0);

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
const int FRAM_HOLD = 2;
const int ADS_CS = 11;
const int ADS_PWDN = 10;
const int ADS_DRDY = 12;
const int ADS_START = 6;
const int SD_CS = 4;
const int GRN_LED = 8;
const int RED_LED = 13;
// END ARBO TESTBED

int32_t memAddr = 0;
int32_t saveData = 0x12345678;
const int uBufSz = 500; // should have 32Kbytes SRAM, 1000 is too big, how to calculate?
int32_t uBuf[uBufSz];
unsigned long StartTime;
unsigned long EndTime;

union ArrayToInteger {
  byte array[4];
  int32_t integer;
};

void setup() {
  Serial.begin(9600);
  while (!Serial) {};
  SPI.begin();
  pinMode(ADS_CS, OUTPUT);
  pinMode(FRAM_CS, OUTPUT);
  pinMode(FRAM_HOLD, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  pinMode(GRN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  digitalWrite(ADS_CS, HIGH);
  digitalWrite(SD_CS, HIGH);
  digitalWrite(FRAM_CS, HIGH);
  digitalWrite(FRAM_HOLD, HIGH);
  digitalWrite(RED_LED, LOW);
  digitalWrite(GRN_LED, LOW);
  Serial.println("Bit bashin...");
  StartTime = millis();
}

void loop() {
  //  bool fram_online = fram_deviceId();
  //  Serial.println(fram_online);
  //  fram_writeInt(memAddr, memAddr);
  
  while (memAddr < uBufSz) {
//    fram_writeInt(int32Addr(memAddr), 0x01020304);
    uBuf[memAddr] = fram_readInt(int32Addr(memAddr));
    //    Serial.println(int32Addr(memAddr));
//    Serial.print(int32Addr(memAddr)); Serial.print(": ");
//    Serial.println(uBuf[memAddr], HEX);
    memAddr++;
  }
//uBuf[0] = 0x01020304;
//Serial.println(uBuf[0],HEX);

  memAddr = 0;
  Serial.println(millis() - StartTime);
  delay(10000);
  StartTime = millis();
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

// HELPERS
void print_buffer(byte arry[], int sz) {
  for (int i = 0; i < sz; i++) {
    Serial.print(i); Serial.print(": ");
    Serial.println(arry[i], HEX);
  }
}
