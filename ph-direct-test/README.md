# pH direct ESP32 voltage test
Po -> 20k ohm -> junction/GPIO34 -> 10k ohm -> common GND.
PH V+=verified USB-fed 5V/VIN, both G=ESP GND; Do/To disconnected.
External ADC and level converter are unused. Never connect 5V Po directly to GPIO34.
Measured calibrated ESP32 millivolts are averaged over 32 conversions.
Po estimate = pin voltage * 3. Verify against a multimeter and actual resistor values.
Upload: pio run -e esp32dev -t upload --upload-port COM3
Run logger with Python and pyserial: python live_log.py
Open http://127.0.0.1:8123. Pause preserves rows; Resume clears them.
No pH conversion is applied. Remove BNC short and reconnect probe for water tests.
