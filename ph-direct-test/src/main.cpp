#include <Arduino.h>
constexpr int PIN_PH = 34;
constexpr float DIVIDER_FACTOR = 3.0f; // Po -> 20k -> GPIO34 -> 10k -> GND.
void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_PH, ADC_11db);
  Serial.println("PH_DIRECT: GPIO34 via 20k/10k divider; no ADS1115/I2C.");
  Serial.println("ms,raw_mean,Po_est_V,pin_V,min_pin_V,status");
}
void loop() {
  long rawSum = 0, mvSum = 0;
  uint32_t lowMv = 5000;
  bool limit = false;
  for(int i=0; i<32; ++i) {
    int raw = analogRead(PIN_PH);
    uint32_t mv = analogReadMilliVolts(PIN_PH);
    rawSum += raw; mvSum += mv;
    if(mv < lowMv) lowMv = mv;
    limit |= raw <= 5 || raw >= 4090;
    delay(2);
  }
  float pinV = mvSum / 32000.0f;
  Serial.printf("%lu,%.2f,%.5f,%.5f,%.5f,%s\n",millis(),rawSum/32.0f,
                pinV*DIVIDER_FACTOR,pinV,lowMv/1000.0f,
                limit ? "DIRECT_ADC_LIMIT" : "DIRECT_OK");
  delay(200);
}
