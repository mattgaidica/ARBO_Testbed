#include <ARBO_Mini.h>
#include <ARBO.h>
#include <SPI.h>
#include "SdFat.h" // see dataLogger SdFat example

#define FILE_BASE_NAME "XX"
SdFat sd;
SdFile file;

const int ADSchs = 6;
volatile int32_t ads_ch1;
volatile int32_t ads_ch2;
volatile int32_t ads_ch3;
volatile int32_t ads_ch4;
volatile int isr_samples = 0;
const int normCount = 20;
int inorm = 0;
int32_t ads_ch1_norm[normCount];
int32_t ads_ch2_norm[normCount];

volatile bool fileWritten = false;
const int takeSamples = 250 * 5 * 60;
volatile bool doWrite = false;

void setup() {
  arbo_init(true);
  //  digitalWrite(SD_CS, HIGH);

  //  Serial.println("Starting up...");
  //  while (!ads_deviceId()) {
  //    Serial.println("Booting ADS129x");
  //  }
  //  Serial.println("ADS129x online...");

  // had to turn on the RLD to get things working
  // delay in loop causes issues for some reason

  ads_cmd(ADS_OP_SDATAC); // stop data while setup
  ads_wreg(0x01, 0b01100110); // config1
  ads_wreg(0x02, 0b00010001); // config2
  ads_wreg(0x03, 0b11001100); // config3
  ads_wreg(0x0D, 0b00000011); // RLD_SENSP
  ads_wreg(0x0E, 0b00000011); // RLD_SENSN
  byte chReg   = 0b00010000; // XXXXX101 for test
  byte chDis   = 0b10000001;
  ads_wreg(0x05, chReg); // ch1
  ads_wreg(0x06, chReg); // ch2
  ads_wreg(0x07, chDis); // ch3
  ads_wreg(0x08, chDis); // ch4
  ads_wreg(0x09, chDis); // ch5 disabled
  ads_wreg(0x0A, chDis); // ch6 disabled

  ads_cmd(ADS_OP_RDATAC); // cont conversion
  ads_startConv();
  //  digitalWrite(ADS_CS, LOW);

  //  attachInterrupt(digitalPinToInterrupt(ADS_DRDY), ads_log, CHANGE);
  Serial.println("looping...");
  digitalWrite(RED_LED, HIGH);

  Serial.println("Writing SD...");
  unsigned long startMicros = millis();
  //  sd_write();
  Serial.print("millis: "); Serial.println(millis() - startMicros);
}

void loop() {
  //  Serial.println(isr_samples);
  //    Serial.print(ads_ch1 - average(ads_ch1_norm,normCount));
  //    Serial.print('\t');
  //    Serial.println(ads_ch2 - average(ads_ch2_norm,normCount));

  //  ads_ch1_norm[inorm] = ads_ch1;
  //  ads_ch2_norm[inorm] = ads_ch2;
  //  inorm++;
  //
  //  if (inorm > normCount) {
  //    inorm = 0;
  //  }

  Serial.println(measureBatt());
  delay(1000);
  Serial.println(fram_deviceId());
  delay(1000);
}

void ads_log() {
  ads_read();
  fram_writeInt(int32Addr(isr_samples), ads_ch1);
  isr_samples++;
  if (isr_samples == takeSamples && !fileWritten) {
    detachInterrupt(digitalPinToInterrupt(ADS_DRDY));
    sd_write();
    //    attachInterrupt(digitalPinToInterrupt(ADS_DRDY), ads_log, CHANGE);
    //    fileWritten = true;
  }
}

void sd_write() {
  sd_openNew();
  // write in 512 byte chunks (128x 32-bit integers)
  int bufSz = 512;
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

void sd_openNew() {
  const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
  char fileName[13] = FILE_BASE_NAME "00.arbo";
  if (!sd.begin(SD_CS, SD_SCK_MHZ(1))) {
    Serial.println("SD ERROR");
  }
  while (sd.exists(fileName)) {
    if (fileName[BASE_NAME_SIZE + 1] != '9') {
      fileName[BASE_NAME_SIZE + 1]++;
    } else if (fileName[BASE_NAME_SIZE] != '9') {
      fileName[BASE_NAME_SIZE + 1] = '0';
      fileName[BASE_NAME_SIZE]++;
    } else {
      Serial.println("Can't create file name");
    }
  }
  if (!file.open(fileName, O_WRONLY | O_CREAT | O_EXCL)) {
    Serial.println("file.open error");
  }
}

float average (int32_t * array, int len) {
  long sum = 0L ;  // sum will be larger than an item, long for safety.
  for (int i = 0 ; i < len ; i++) {
    sum += array [i] ;
  }
  return  ((float) sum) / len ;  // average will be fractional, so float may be appropriate.
}
