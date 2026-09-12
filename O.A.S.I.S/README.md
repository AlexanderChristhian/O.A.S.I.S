# O.A.S.I.S. control simulation

This PlatformIO/Wokwi milestone simulates the pH and turbidity sensors with two potentiometers. TDS is intentionally excluded.

For the project overview, group member names/NPM, setup instructions, and demo walkthrough, see the [repository README](../README.md).

## Hardware and pin mapping

Target: ESP32 Dev Module (`esp32dev`), Arduino framework, Espressif32 platform. The LCD uses `marcoschwartz/LiquidCrystal_I2C @ ^1.1.4` at I2C address `0x27`.

| Component | ESP32 GPIO | Configuration |
| --- | --- | --- |
| pH potentiometer | 34 | 12-bit ADC, mapped to 0–14 pH |
| Turbidity potentiometer | 35 | 12-bit ADC, mapped to 0–50 NTU |
| MCK button | 13 | Active-low, INPUT_PULLUP |
| OFF button | 14 | Active-low, INPUT_PULLUP |
| DRINK button | 27 | Active-low, INPUT_PULLUP |
| LCD SDA / SCL | 21 / 22 | I2C, 20 columns × 4 rows |
| Hose pump relay | 18 | Active-low PNP module |
| MCK valve relay | 19 | Active-low PNP module |
| RO pump relay | 23 | Active-low PNP module |
| Drink valve relay | 5 | Active-low PNP module |

Sensor conversions: `pH = raw * 14 / 4095`, `NTU = raw * 50 / 4095`. Wokwi's knob attribute uses its own scale; use the resulting LCD values during a demo. The LCD rounds to two decimals; classification uses the unrounded values.

## Controls

- pH potentiometer (GPIO34): maps 0-4095 to pH 0.0-14.0.
- Turbidity potentiometer (GPIO35): maps 0-4095 to 0-50 NTU.
- `1` / blue button: MCK mode.
- `2` / yellow button: OFF mode (all outputs disabled).
- `3` / green button: DRINK mode.

The 20x4 I2C LCD displays the selected mode, pH, turbidity, and the resulting water class.

Startup mode is OFF. Click a button once to select and retain a mode; releasing it does not reset the mode. Selecting MCK or DRINK starts the permitted route without a separate start button. OFF keeps the ESP32, sensors, and LCD running but disables all pumps and valves. OFF has immediate priority. The diagram disables button bounce, so firmware accepts short clicks without a debounce delay; physical contacts require an appropriate debounce interval. Simultaneous MCK and DRINK requests select OFF. Release the buttons before selecting again. Do not Ctrl-click to hold these mode buttons.

These momentary buttons emulate mutually exclusive selector choices in firmware. They do not emulate the electrical contacts of a maintained three-position selector. The physical selector will need position-reading logic matched to its actual contact arrangement. Wokwi's standard slide switch has only two positions.

## Current quality rules

| Water class | Simulated criteria |
| --- | --- |
| Drinkable | pH 6.5-8.5 and turbidity at most 5 NTU |
| MCK only | pH 6.5-8.5 and turbidity above 5 but below 10 NTU |
| Unsafe | Any other reading |

These potentiometer conversions are simulation mappings, not calibration equations for physical sensors.

The team's preferred drinking-route turbidity target is below 0.3 NTU. The acceptance cutoff in this simulation is inclusive at 5 NTU; the preferred target does not separately gate the relays. These are team-defined simulation criteria, not a certification of drinking-water safety.

The simulation does not model filtration or RO treatment: switching pumps or modes does not change the sensor readings. It routes water based on the current readings only. There are no separate inlet/outlet sensors, treatment delay, or post-treatment verification. MCK ONLY applies above 5 and below 10 NTU with pH in range; exactly 10 NTU is UNSAFE.

## Control flowchart

```mermaid
flowchart TD
    START([Power on]) --> INIT[Initialize ESP32, LCD, buttons, and relay pins]
    INIT --> SAFE[Set all relay outputs OFF]
    SAFE --> INPUTS[Read mode buttons and pH/turbidity potentiometers]

    INPUTS --> PH{"pH between 6.5 and 8.5?"}
    PH -- No --> UNSAFE[Water class: UNSAFE]
    PH -- Yes --> DRINK_NTU{"Turbidity at most 5 NTU?"}
    DRINK_NTU -- Yes --> DRINKABLE[Water class: DRINKABLE]
    DRINK_NTU -- No --> MCK_NTU{"Turbidity below 10 NTU?"}
    MCK_NTU -- Yes --> MCK_ONLY[Water class: MCK ONLY]
    MCK_NTU -- No --> UNSAFE

    DRINKABLE --> MODE{Selected mode}
    MCK_ONLY --> MODE
    UNSAFE --> MODE

    MODE -- MCK --> MCK_OK{"Class is not UNSAFE?"}
    MCK_OK -- Yes --> MCK_ROUTE[Hose pump ON<br/>MCK valve OPEN]
    MCK_OK -- No --> BLOCKED[All relays OFF]

    MODE -- OFF --> BLOCKED

    MODE -- DRINK --> DRINK_OK{"Class is DRINKABLE?"}
    DRINK_OK -- Yes --> DRINK_ROUTE[Hose pump ON<br/>RO pump ON<br/>Drink valve OPEN]
    DRINK_OK -- No --> BLOCKED

    MCK_ROUTE --> APPLY[Write relay outputs]
    DRINK_ROUTE --> APPLY
    BLOCKED --> APPLY
    APPLY --> DISPLAY[Update LCD and serial status]
    DISPLAY --> WAIT[Wait until the next 250 ms sensor cycle]
    WAIT --> INPUTS
```

## Runtime sequence

```mermaid
sequenceDiagram
    actor User
    participant Buttons as Mode buttons
    participant ESP32
    participant Sensors as pH and turbidity pots
    participant Relays as Pump and valve relays
    participant LCD
    participant Serial as Serial monitor

    User->>Buttons: Select MCK, OFF, or DRINK
    Buttons-->>ESP32: Active-low button input
    ESP32->>ESP32: Latch selected mode

    loop Every 250 ms or on mode change
        ESP32->>Sensors: Read GPIO34 and GPIO35 ADC values
        Sensors-->>ESP32: Raw pH and turbidity values
        ESP32->>ESP32: Map readings and classify water
        ESP32->>ESP32: Calculate outputs from mode + water class

        alt Drinking route permitted
            ESP32->>Relays: Hose ON, RO ON, Drink OPEN
        else MCK route permitted
            ESP32->>Relays: Hose ON, MCK OPEN
        else OFF, unsafe, or mode requirement not met
            ESP32->>Relays: All outputs OFF
        end

        ESP32->>LCD: Show mode, pH, NTU, and water class
    end

    loop Every 1 second
        ESP32->>Serial: Print readings, state, and relay outputs
    end
```

## Relay behavior

| Mode | Drinkable | MCK only | Unsafe |
| --- | --- | --- | --- |
| MCK | Hose pump + MCK valve | Hose pump + MCK valve | All off |
| OFF | All off | All off | All off |
| DRINK | Hose pump + RO pump + drink valve | All off | All off |

The Wokwi relay modules use the `pnp` setting. Simulation verification showed active-low behavior: firmware drives IN LOW to activate the load and HIGH to disable it. The LEDs connected to NO indicate active outputs.

## Build and simulate

Run `pio run` in this directory. Then start the Wokwi simulator from VS Code; `wokwi.toml` points to the generated ESP32 firmware and ELF files.
