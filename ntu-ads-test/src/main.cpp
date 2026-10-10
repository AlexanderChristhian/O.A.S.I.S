#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 adc;
bool ready = false;
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  Wire.setTimeOut(100);
  adc.setGain(GAIN_TWOTHIRDS); // +/-6.144V; powered at 5V, A0 directly from NTU A.
  adc.setDataRate(RATE_ADS1115_128SPS);
  ready = adc.begin(0x48);
  Serial.println("NTU TEST: A0 direct, ADC supply=5V, I2C level converter, assumed ADS1115.");
  Serial.println("Voltage scaling requires ADS1115. Compare with a multimeter; PCB labels alone do not identify chip.");
  Serial.println("ms,raw_mean,volts_mean,volts_min,volts_max,status");
}
void loop() {
  Wire.beginTransmission(0x48);
  if (!ready || Wire.endTransmission() != 0) {
    Serial.println("ERROR: ADC missing at 0x48; check power/I2C/ADDR.");
    delay(1000);
    ready = adc.begin(0x48);
    return;
  }
  long sum = 0;
  int16_t lo = 32767, hi = -32768;
  for (int i = 0; i < 64; ++i) {
    int16_t raw = adc.readADC_SingleEnded(0);
    sum += raw;
    if (raw < lo) lo = raw;
    if (raw > hi) hi = raw;
  }
  float mean = sum / 64.0f;
  float volts = mean * 0.0001875f;
  const char* status = lo <= 5 ? "NEAR_ZERO_CHECK" :
                       hi >= 32760 ? "ADC_SATURATED" :
                       hi * 0.0001875f >= 4.95f ? "NEAR_5V_CHECK" : "OK_UNCALIBRATED";
  Serial.printf("%lu,%.2f,%.5f,%.5f,%.5f,%s\n", millis(), mean, volts,
                lo * 0.0001875f, hi * 0.0001875f, status);
  delay(300);
}
