#include <ARBO_Testbed.h>
#include <ARBO.h>
#include <SPI.h>
#include "SdFat.h" // see dataLogger SdFat example

SPISettings SPI_SD(SPI_FREQ, MSBFIRST, SPI_MODE0);

char fileName[13] = "";
SdFat sd;
SdFile file;
int fileNumber = 0;

volatile int32_t ads_ch1;
volatile int32_t ads_ch2;
volatile int32_t ads_ch3;
volatile int32_t ads_ch4;
volatile int32_t useData;
volatile int isr_samples = 0;
bool writeFlag = false;

const int takeSamples = 250 * 4 * 30; // max=30,000, includes 4 channels
int incomingByte = 0;
int useChannel = 0;

void setup() {
  arbo_init(true);
  //  Serial.println(fram_deviceId());
  //  Serial.println("Starting up...");
  //  while (!ads_deviceId()) {
  //    Serial.println("Booting ADS129x");
  //  }
  //  Serial.println("ADS129x online...");

  while (!sd.begin(SD_CS, SPI_SD)) {
    Serial.println("No SD card. Insert and cycle power.");
    arbo_blink(2000);
  }
  arbo_blink(500);

  // had to turn on the RLD to get things working
  // delay in loop causes issues for some reason
  ads_cmd(ADS_OP_SDATAC); // stop data while setup
  ads_wreg(0x01, 0b01000110); // config1
  ads_wreg(0x02, 0b00010001); // config2
  ads_wreg(0x03, 0b11001100); // config3
  ads_wreg(0x0D, 0b00000001); // RLD_SENSP
  ads_wreg(0x0E, 0b00000001); // RLD_SENSN
  byte chReg   = 0b00000000; // XXXXX101 for test, 000 for normal
  byte chDis   = 0b10000001;
  ads_wreg(0x05, chReg); // ch1
  ads_wreg(0x06, chReg); // ch2
  ads_wreg(0x07, chReg); // ch3
  ads_wreg(0x08, chReg); // ch4
  if (ADSchs >= 6) {
    ads_wreg(0x09, chDis); // ch5 disabled
    ads_wreg(0x0A, chDis); // ch6 disabled
  }

  ads_cmd(ADS_OP_RDATAC); // cont conversion
  ads_startConv();
  //  if (!sd.begin(SD_CS, SPI_SD)) {
  //    Serial.println("SD begin error");
  //  }
  //  if (!sd.wipe(&Serial)) {
  //    Serial.println("SD wipe error");
  //  }

  attachInterrupt(digitalPinToInterrupt(ADS_DRDY), ads_log, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SD_DET), triggerWrite, CHANGE);
}

void loop() {
  if (Serial.available() > 0) {
    incomingByte = Serial.read();
    if (incomingByte != 10) { // \n
      useChannel = incomingByte - 48; // convert to ch#
    }
  }
  switch (useChannel) {
    case 1:
      useData = ads_ch1; break;
    case 2:
      useData = ads_ch2; break;
    case 3:
      useData = ads_ch3; break;
    case 4:
      useData = ads_ch4; break;
  }
  digitalWrite(RED_LED, writeFlag);
  Serial.println(sign32(rmHeader(useData)));
  delay(50);
}

void triggerWrite() {
  detachInterrupt(digitalPinToInterrupt(SD_DET));
  writeFlag = true;
}

void ads_log() {
  ads_read();
  if (writeFlag) {
    fram_writeInt(int32Addr(isr_samples), ads_ch1); isr_samples++;
    fram_writeInt(int32Addr(isr_samples), ads_ch2); isr_samples++;
    fram_writeInt(int32Addr(isr_samples), ads_ch3); isr_samples++;
    fram_writeInt(int32Addr(isr_samples), ads_ch4); isr_samples++;
  }
  if (isr_samples == takeSamples) {
    writeFlag = false;
    detachInterrupt(digitalPinToInterrupt(ADS_DRDY));
    sd_write();
    isr_samples = 0;
    attachInterrupt(digitalPinToInterrupt(SD_DET), triggerWrite, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ADS_DRDY), ads_log, CHANGE);
  }
}

void sd_write() {
  // write in 512 byte chunks (128x 32-bit integers)
  int bufSz = 512;
  sd_openFile();
  for (int memAddr = 0; memAddr < takeSamples * 4; memAddr += bufSz) {
    // takeSamples*4-1 is your final memAddr
    if ((takeSamples * 4 - 1) < (memAddr + bufSz)) {
      bufSz = takeSamples * 4 - memAddr; // should modify bufSz on last write
    }
    byte framBuf[bufSz];
    digitalWrite(RED_LED, LOW);
    fram_readChunk(memAddr, framBuf, sizeof(framBuf));
    file.write(framBuf, sizeof(framBuf));
    digitalWrite(RED_LED, HIGH);
  }
  file.close();
}

void sd_openFile() {
  incFileName();
  if (sd.begin(SD_CS, SPI_SD)) {
    while (sd.exists(fileName)) {
      incFileName();
    }
    if (file.open(fileName, O_WRONLY | O_CREAT | O_EXCL)) {
      //      Serial.println(fileName);
    }
  } else {
    // HANDLE THIS!
    //    Serial.println("SD card removed, stopping.");
  }
}

void incFileName() {
  sprintf(fileName, "%08d.arbo", fileNumber);
  fileNumber++;
}
