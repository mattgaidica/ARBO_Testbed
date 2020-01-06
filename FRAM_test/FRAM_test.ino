#include <SPI.h>
SPISettings mySettings(400000, MSBFIRST, SPI_MODE0);

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
const uint32_t maxMem = 0x1FFFF;

const int FRAM_CS = 38;
const int FRAM_HOLD = 2;
int32_t val;

union ArrayToInteger {
 byte array[4];
 int32_t integer;
};

void setup() {
  Serial.begin(9600);
  while (!Serial) {};
  SPI.begin();
  pinMode(FRAM_CS, OUTPUT);
  pinMode(FRAM_HOLD, OUTPUT);
  digitalWrite(FRAM_HOLD, HIGH);
  Serial.println("Bit bashin...");
}

void loop() {
  bool fram_online = fram_deviceId();
  Serial.println(fram_online);
  uint32_t memAdd = 10;
  int saveData = 0x12345678;
  fram_writeInt(memAdd, saveData);
  fram_writeInt(memAdd+1, saveData);
  val = fram_readInt(memAdd);
  Serial.println(val, HEX);
  delay(3000);
}

void fram_writeInt(uint32_t memAdd, int data) {
  byte myBuf[8] = {WRITE, memAdd >> 16, memAdd >> 8, memAdd, data >> 24, data >> 16, data >> 8, data};
  fram_writeEnable();
  fram_on();
  SPI.transfer(&myBuf, sizeof(myBuf));
  fram_off();
  fram_writeDisable();
}
void fram_writeByte(uint32_t memAdd, byte data) {
  byte myBuf[5] = {WRITE, memAdd >> 16, memAdd >> 8, memAdd, data};
  fram_writeEnable();
  fram_on();
  SPI.transfer(&myBuf, sizeof(myBuf));
  fram_off();
  fram_writeDisable();
}

// technically could read larger data sizes
int fram_readInt(uint32_t memAdd) {
  // op-code, addr, addr, addr, dummy 8-cycles, read 4 bytes (int)
  byte myBuf[9] = {FSTRD, memAdd >> 16, memAdd >> 8, memAdd};
  fram_on();
  SPI.transfer(&myBuf, sizeof(myBuf));
  fram_off();
  ArrayToInteger converter;
  converter.array[0] = myBuf[8];
  converter.array[1] = myBuf[7];
  converter.array[2] = myBuf[6];
  converter.array[3] = myBuf[5];
  return converter.integer;
}
byte fram_readByte(uint32_t memAdd) {
  // op-code, addr, addr, addr, empty for read data
  byte myBuf[5] = {READ, memAdd >> 16, memAdd >> 8, memAdd};
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
void print_buffer(byte arry[], int siz) {
  for (int i = 0; i < siz; i++) {
    Serial.print(i); Serial.print(": ");
    Serial.println(arry[i], HEX);
  }
}
