#include <ARBO_Mini.h>
//#include <ARBO_Testbed.h>
#include <ARBO.h>
#include <SPI.h>
#include "SdFat.h" // see dataLogger SdFat example
#include "ICM_20948.h"

ICM_20948_SPI myICM;
//ICM_20948_smplrt_t mySmplrt;
//mySmplrt.g = 54;

SPISettings SPI_SD(SPI_FREQ, MSBFIRST, SPI_MODE0);

char fileName[13] = "";
SdFat sd;
SdFile file;
int fileNumber = 0;

//const int ADSchs = 4;
volatile int32_t ads_ch1;
volatile int32_t ads_ch2;
volatile int32_t ads_ch3;
volatile int32_t ads_ch4;
volatile int isr_samples = 0;

volatile bool fileWritten = false;
const int takeSamples = 250 * 15;// 2 * 60;

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

  // had to turn on the RLD to get things working
  // delay in loop causes issues for some reason
  ads_cmd(ADS_OP_SDATAC); // stop data while setup
  ads_wreg(0x01, 0b01000110); // config1
  ads_wreg(0x02, 0b00010001); // config2
  ads_wreg(0x03, 0b11001100); // config3
  ads_wreg(0x0D, 0b00000011); // RLD_SENSP
  ads_wreg(0x0E, 0b00000011); // RLD_SENSN
  byte chReg   = 0b00000000; // XXXXX101 for test, 000 for normal
  byte chDis   = 0b10000001;
  ads_wreg(0x05, chReg); // ch1
  ads_wreg(0x06, chDis); // ch2
  ads_wreg(0x07, chDis); // ch3
  ads_wreg(0x08, chDis); // ch4
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

  //  attachInterrupt(digitalPinToInterrupt(ADS_DRDY), ads_log, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ACCEL_INT), accelISR, FALLING);
  myICM.begin(ACCEL_CS);
//  myICM.swReset(); // this makes dataReady() fail, prob starts in LP mode?
  Serial.print("Accel: ");
  Serial.println(myICM.statusString());
}

void loop() {
  //  Serial.println(measureBatt());
  //  Serial.println(isr_samples);
  if ( myICM.dataReady() ) {
    myICM.getAGMT();                // The values are only updated when you call 'getAGMT'
    //    printRawAGMT( myICM.agmt );     // Uncomment this to see the raw values, taken directly from the agmt structure
    printScaledAGMT(myICM.agmt);   // This function takes into account the sclae settings from when the measurement was made to calculate the values with units
    delay(30);
  } else {
    Serial.println("Waiting for data");
    delay(500);
  }

  //  if (measureBatt() < 3.6) {
  //    detachInterrupt(digitalPinToInterrupt(ADS_DRDY));
  //  }
}

void ads_log() {
  ads_read();
  fram_writeInt(int32Addr(isr_samples), ads_ch1);
  isr_samples++;
  if (isr_samples == takeSamples && !fileWritten) {
    detachInterrupt(digitalPinToInterrupt(ADS_DRDY));
    sd_write();
    isr_samples = 0;
    attachInterrupt(digitalPinToInterrupt(ADS_DRDY), ads_log, CHANGE);
    //    fileWritten = true;
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
    fram_readChunk(memAddr, framBuf, sizeof(framBuf));
    file.write(framBuf, sizeof(framBuf));
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
      Serial.println(fileName);
    }
  } else {
    // HANDLE THIS!
    Serial.println("SD card removed, stopping.");
  }
}

void incFileName() {
  sprintf(fileName, "%08d.arbo", fileNumber);
  fileNumber++;
}

void accelISR() {
  
}
