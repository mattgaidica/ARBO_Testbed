#include <ARBO_Testbed.h>
#include <ARBO.h>
#include <SPI.h>
//SPISettings mySettings(SPI_FREQ, MSBFIRST, SPI_MODE0);

int32_t val;

void setup() {
  Serial.begin(9600);
  while (!Serial) {};
  SPI.begin();
  pinMode(ADS_CS, OUTPUT);
  pinMode(FRAM_CS, OUTPUT);
  pinMode(ACCEL_CS, OUTPUT);
  pinMode(FRAM_HOLD, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  pinMode(GRN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  digitalWrite(ADS_CS, HIGH);
  digitalWrite(SD_CS, HIGH);
  digitalWrite(ACCEL_CS, HIGH);
  digitalWrite(FRAM_CS, HIGH);
  digitalWrite(FRAM_HOLD, HIGH);
  digitalWrite(RED_LED, LOW);
  digitalWrite(GRN_LED, LOW);
  Serial.println("Bit bashin...");
}

void loop() {
  bool fram_online = fram_deviceId();
  Serial.println(fram_online);
//  uint32_t memAdd = 10;
//  int saveData = 0x12345678;
//  fram_writeInt(memAdd, saveData);
//  fram_writeInt(memAdd+1, saveData);
//  val = fram_readInt(memAdd);
//  Serial.println(val, HEX);
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
