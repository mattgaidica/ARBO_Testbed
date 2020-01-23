#include <ARBO_Testbed.h>
#include <ARBO.h>
int32_t val;
void setup() {
  arbo_init(false);
}

void loop() {
  bool fram_online = fram_deviceId();
  Serial.println(fram_online);
  arbo_blink(500);
}
