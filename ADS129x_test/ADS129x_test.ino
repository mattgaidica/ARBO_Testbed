#include <SPI.h>
SPISettings mySettings(400000, MSBFIRST, SPI_MODE1);

// ADS Opcode Commands (Table 15)
// SYSTEM COMMANDS
const byte WAKEUP  = 0x02;
const byte STANDBY = 0x04;
const byte RESET   = 0x06;
const byte START   = 0x08;
const byte STOP    = 0x0A;
// DATA READ COMMANDS
const byte RDATAC = 0x10;
const byte SDATAC = 0x11;
const byte RDATA  = 0x12;
// REGISTER READ COMMANDS (2-bytes)
const byte RREG = 0x20;
const byte WREG = 0x40;

const byte DEVID  = 0x00;

const int FRAM_CS = 38;
const int ADS_CS = 11;
const int ADS_PWDN = 10;
const int ADS_DRDY = 12;
const int ADS_START = 6;

const int GRN_LED = 8;

volatile int32_t ads_ch1 = 0;

union ArrayToInteger {
  byte array[4];
  int32_t integer;
};

void setup() {
  Serial.begin(115200);
  while (!Serial) {};
  SPI.begin();
  pinMode(ADS_CS, OUTPUT);
  pinMode(ADS_PWDN, OUTPUT);
  pinMode(ADS_START, OUTPUT);
  pinMode(FRAM_CS, OUTPUT);
  pinMode(GRN_LED, OUTPUT);
  
  digitalWrite(ADS_PWDN, HIGH);
  digitalWrite(ADS_START, LOW); // tie to low to use commands
  digitalWrite(ADS_CS, HIGH);
  digitalWrite(FRAM_CS, HIGH);
  digitalWrite(GRN_LED, LOW);
  //  Serial.println("Starting up...");
  //  while (!ads_deviceId()) {
  //    Serial.println("Booting ADS129x");
  //  }
  //  Serial.println("ADS129x online...");

  // had to turn on the RLD to get things working
  // delay in loop causes issues for some reason

  ads_cmd(SDATAC); // stop data while setup
  ads_wreg(0x01, 0b00000110); // config1
  ads_wreg(0x02, 0b00010000); // config2
  ads_wreg(0x03, 0b11001100); // config3
  byte chReg   = 0b00000000; // XXXXX101 for test
  byte chDis   = 0b10000001;
  ads_wreg(0x05, chReg); // ch1
  ads_wreg(0x06, chReg); // ch2
  ads_wreg(0x07, chReg); // ch3
  ads_wreg(0x08, chReg); // ch4
  ads_wreg(0x09, chDis); // ch5 disabled
  ads_wreg(0x0A, chDis); // ch6 disabled

  ads_cmd(RDATAC); // cont conversion
  ads_startConv();
  digitalWrite(ADS_CS, LOW);

  pinMode(ADS_DRDY, INPUT);
  attachInterrupt(digitalPinToInterrupt(12), ads_log, CHANGE);
  Serial.println("looping...");
}

void loop() {
  //  int val = ads_read();
  //  Serial.println(val);
  //  ads_endConv();
  //  delay(20);
  digitalWrite(GRN_LED, LOW);
  Serial.println(ads_ch1);
  delay(5);
}

void ads_log() {
  //  ads_read();
  //  Serial.println(ads_ch1);
  digitalWrite(GRN_LED, HIGH);
  ads_read();
}

void ads_wreg(byte rrrrr, byte data) {
  ads_on();
  byte myBuf[3] = {WREG | rrrrr, 0x00, data}; // op-code then 0x00 to write 1 register
  SPI.transfer(&myBuf, sizeof(myBuf));
  ads_off();
}

int ads_read() {
  size_t bufSz = (24 + (4 * 24)) / 8;
  byte myBuf[bufSz] = {};
  //  while (digitalRead(ADS_DRDY)) {} // wait for _DRDY to go LOW
  ads_on();
  SPI.transfer(&myBuf, sizeof(myBuf));
  ads_off();
  ads_ch1 = sign24to32(myBuf[3], myBuf[4], myBuf[5]);
  //  int i;
  //  print_buffer(myBuf,sizeof(myBuf));
  //  for (i = 1; i <= 1; i++) {
  //    int bufStart = (i * 3);
  //    if (i == 1) {
  //      Serial.println(sign24to32(myBuf[bufStart], myBuf[bufStart + 1], myBuf[bufStart + 2]));
  //    } else {
  //      Serial.print(sign24to32(myBuf[bufStart], myBuf[bufStart + 1], myBuf[bufStart + 2]));
  //      Serial.print("\t");
  //    }
  //}

  //  ArrayToInteger converter;
  //  converter.array[0] = 0x00;
  //  converter.array[1] = myBuf[3];
  //  converter.array[2] = myBuf[4];
  //  converter.array[3] = myBuf[5];
  //  return converter.integer;
}

void ads_startConv() { // could replace with START cmd
  digitalWrite(ADS_START, HIGH);
}
void ads_endConv() {
  digitalWrite(ADS_START, LOW);
}

void ads_cmd(byte cmd) {
  ads_on();
  SPI.transfer(&cmd, 1);
  ads_off();
}

bool ads_deviceId() {
  ads_on();
  byte myBuf[3] = {SDATAC, RREG | DEVID, 0x00}; // stop data continuous, read 1 reg
  SPI.transfer(&myBuf, sizeof(myBuf));
  ads_off();
  if (myBuf[0] == 0x90 || myBuf[0] == 0x91 || myBuf[0] == 0x92) { // ADS129x
    return true;
  } else {
    Serial.println("deviceId buffer:");
    print_buffer(myBuf, sizeof(myBuf));
    return false;
  }
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
int32_t sign24to32(byte b1, byte b2, byte b3) {
  ArrayToInteger converter;
  converter.array[0] = b3;
  converter.array[1] = b2;
  converter.array[2] = b1;
  converter.array[3] = 0;
  if (converter.integer & 0x00800000) {
    return converter.integer |= 0xFF000000;
  } else {
    return converter.integer;
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
