#include <ARBO_Testbed.h>
#include <ARBO.h>
#include <ADS129X.h>
//#include <SD.h>

const int takeSamples = 1000;
volatile int32_t ads_ch1;
volatile int isr_samples = 0;
volatile bool doWrite = false;

void setup() {
  arbo_init(true);
  //digitalWrite(SD_CS, LOW);
  //  SD.begin(SD_CS);
  //  while (!SD.begin(SD_CS)) {
  //    Serial.println("Waiting for card...");
  //    digitalWrite(RED_LED, HIGH);
  //    delay(200);
  //    digitalWrite(RED_LED, LOW);
  //    delay(50);
  //  }
  //  delay(100); // debounce
  //  Serial.println("Card initialized...");

  //  if (SD.exists("ARBO.txt")) {
  //    Serial.println("Removing ARBO.txt");
  //    SD.remove("ARBO.txt");
  //  }
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
  ads_wreg(0x0D, 0b00000000); // RLD_SENSP
  ads_wreg(0x0E, 0b00000000); // RLD_SENSN
  byte chReg   = 0b01100101; // XXXXX101 for test
  byte chDis   = 0b10000001;
  ads_wreg(0x05, chReg); // ch1
  ads_wreg(0x06, chDis); // ch2
  ads_wreg(0x07, chDis); // ch3
  ads_wreg(0x08, chDis); // ch4
  ads_wreg(0x09, chDis); // ch5 disabled
  ads_wreg(0x0A, chDis); // ch6 disabled

  ads_cmd(ADS_OP_RDATAC); // cont conversion
  ads_startConv();
//  digitalWrite(ADS_CS, LOW);

  attachInterrupt(digitalPinToInterrupt(ADS_DRDY), ads_log, CHANGE);
  Serial.println("looping...");
  digitalWrite(RED_LED, HIGH);
}

void loop() {
  //  if (isr_samples >= takeSamples) {
  //    detachInterrupt(digitalPinToInterrupt(ADS_DRDY));
  //    digitalWrite(GRN_LED, HIGH);
  //    delay(100);
  //    digitalWrite(GRN_LED, LOW);
  //    delay(100);
  //  } else {
  if (doWrite) {
    Serial.println(ads_ch1);
    //      Serial.println(isr_samples);
    //            digitalWrite(ADS_CS, HIGH); // ADS -> SD
    //      digitalWrite(SD_CS, LOW);
    //      File sd = SD.open("ARBO.txt", FILE_WRITE);
    //      sd.write((byte*)&ads_ch1, 4);
    //      sd.close();
    //      digitalWrite(SD_CS, HIGH); // SD -> ADS
    //      digitalWrite(ADS_CS, LOW);
    delay(30);
    doWrite = false;
  }
  //  }
}

void ads_log() {
  if (!doWrite && !digitalRead(ADS_DRDY)) {
    doWrite = true;
    ads_read();
    isr_samples++;
  }
}
