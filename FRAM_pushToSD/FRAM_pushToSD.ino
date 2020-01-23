#include <ARBO_Testbed.h>
#include <ARBO.h>

//#include <SPI.h>
#include "SdFat.h" // see dataLogger SdFat example
#include "ArduinoLowPower.h"
#include <RTCZero.h>
#include "ICM_20948.h"
//https://community.atmel.com/forum/samd21-sleep-current
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

#define FILE_BASE_NAME "XX"
RTCZero rtc;
ICM_20948_SPI myICM;
SdFat sd;
SdFile file;

int32_t memAddr = 0;
//const int uBufSz = 500; // should have 32Kbytes SRAM, 1000 is too big, how to calculate?
//int32_t uBuf[uBufSz];
byte uBuf[4 * 500]; // 200 int32 + 4 for op-code
unsigned long StartTime;
unsigned long EndTime;

#define error(msg) sd.errorHalt(F(msg)) // from sdfat

void setup() {
  arbo_init(true); //ARBO

  const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
  char fileName[13] = FILE_BASE_NAME "00.arbo";

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

  //  fram_sleep();
  //  ads_cmd(SDATAC);
  //  ads_cmd(STANDBY);
  //  digitalWrite(ADS_PWDN, LOW);

  //  myICM.begin(ACCEL_CS, SPI, SPI_FREQ);
  //  Serial.print("Accel: ");
  //  Serial.println( myICM.statusString() );
  //  myICM.sleep( true );
  //  myICM.lowPower( true );

  //  rtc.begin();
  //  rtc.standbyMode();
  //    Serial.end();
  //  LowPower.deepSleep(5000);

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
  arbo_blink(500);
}
