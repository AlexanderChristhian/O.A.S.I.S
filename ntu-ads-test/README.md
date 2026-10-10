# NTU-only physical bench test

Standalone PlatformIO firmware and a local live table for testing turbidity sensor
voltage. The main OASIS simulation/control firmware is separate. See
[test results and calibration plan](RESULTS-2026-10-10.md).

## Connections

| Connection | Destination |
| --- | --- |
| NTU V, ADC V, level converter HV | Verified USB-fed ESP32 VIN/5V |
| NTU G, ADC G, ADC ADDR, converter grounds | ESP32 GND |
| NTU A | ADC A0 directly |
| Converter LV | ESP32 3V3 |
| ESP32 D21 / ADC SDA | Converter LV1 / HV1 |
| ESP32 D22 / ADC SCL | Converter LV2 / HV2 |

No analog divider is used. ADC inputs must remain within its actual supply
voltage. Leave NTU D and unused ADC/converter channels disconnected.
Firmware assumes ADS1115 at 0x48; confirm chip identity and check output voltage
with a multimeter before interpreting values.

## Run

From this directory, with PlatformIO and Python available:

```sh
pio run -e esp32dev
pio device list
pio run -e esp32dev -t upload --upload-port COM3
python -m pip install -r requirements.txt
python live_log.py
```

COM3 is the fallback port used on the test computer. The logger also locates the
CP210x device with USB VID/PID 10C4:EA60 and serial 0001 if its port changes.
Adapt the device selection for other boards. Close the logger before uploading
or opening another serial monitor; only one application can hold the port.

Open http://127.0.0.1:8123. The table shows sample name, latest window voltage,
DFRobot NTU estimate and friend-model NTU estimate. Voltage is averaged over
64 ADC conversions; it is not an instantaneous single conversion. Both NTU
models remain uncalibrated. The DFRobot curve is used only from 2.5 to 4.2V;
the friend model uses 1 + 5783 ln(3.22/V), clamped to 0–3000.

Pause freezes displayed readings and averages. Resume clears them and starts
fresh; previous saved logs remain intact. Mark each water sample before collecting.
The serial reader closes a failed USB handle and retries every two seconds.
Stop the logger with Ctrl+C. Files in logs/ are local and ignored by Git.

Compare samples using consistent placement, depth, container and lighting.
Known certified NTU standards and independent validation are required before
using either model as an actual NTU measurement.
