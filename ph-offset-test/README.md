# PH-4502C offset adjustment

Keep the previous 5V ADS1115/I2C level-converter connections. Disconnect NTU;
connect PH V+ to verified 5V, both G pins to common GND, and Po directly to A0.
Leave Do and To disconnected. ADC V=5V; converter HV=5V, LV=3.3V;
GPIO21 -> LV1/HV1 -> SDA, GPIO22 -> LV2/HV2 -> SCL; ADDR=GND.

With USB unplugged, disconnect the electrode and temporarily short the module
BNC center contact to its outer shell. Power up and slowly adjust the analog
offset potentiometer toward Po=2.500V. Verify with a multimeter. Usually this
is the potentiometer closest to BNC, but confirm on your specific module.
Power off before removing the short and reconnecting the electrode.
This sets the electronics offset; accurate pH still needs known buffer solutions.

Build/upload: pio run -e esp32dev -t upload --upload-port COM3
Run: python live_log.py
Open http://127.0.0.1:8123. Stop with Ctrl+C before uploading.
The reader reconnects automatically and logs locally. Pause preserves readings;
Resume clears them. Voltage is a 16-conversion average, updated about 4 times/s.
ADS1115 chip identity and voltage scaling must be verified against a multimeter.

## Current voltage-testing display

The current dashboard reads Po directly through the 5V external ADC at A0,
about four voltage windows per second. It supports sample labels and Pause/Resume.
It also displays provisional pH = 7 - 5.70 * (Po - 2.50), explicitly uncalibrated.
The slope and zero point are assumed; a 2.50V adjustment alone is not probe
calibration. For water testing remove the BNC short with power off, connect the
probe, and verify PH V+ is connected to 5V, not GND. Known pH buffers are needed
for actual calibration. Check measured Po with a multimeter.
