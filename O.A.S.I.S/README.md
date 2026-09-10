# O.A.S.I.S. control simulation

This PlatformIO/Wokwi milestone simulates the pH and turbidity sensors with two potentiometers. TDS is intentionally excluded.

## Controls

- pH potentiometer (GPIO34): maps 0-4095 to pH 0.0-14.0.
- Turbidity potentiometer (GPIO35): maps 0-4095 to 0-50 NTU.
- `1` / blue button: MCK mode.
- `2` / yellow button: AUTO mode.
- `3` / green button: DRINK mode.

The 20x4 I2C LCD displays the selected mode, pH, turbidity, and the resulting water class.

## Current quality rules

| Water class | Simulated criteria |
| --- | --- |
| Drinkable | pH 6.5-8.5 and turbidity below 1 NTU |
| MCK only | pH 6.5-8.5 and turbidity below 10 NTU |
| Unsafe | Any other reading |

These potentiometer conversions are simulation mappings, not calibration equations for physical sensors.

## Control flowchart

```mermaid
flowchart TD
    START([Power on]) --> INIT[Initialize ESP32, LCD, buttons, and relay pins]
    INIT --> SAFE[Set all relay outputs OFF]
    SAFE --> INPUTS[Read mode buttons and pH/turbidity potentiometers]

    INPUTS --> PH{"pH between 6.5 and 8.5?"}
    PH -- No --> UNSAFE[Water class: UNSAFE]
    PH -- Yes --> DRINK_NTU{"Turbidity below 1 NTU?"}
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

    MODE -- AUTO --> AUTO_CLASS{Water class}
    AUTO_CLASS -- DRINKABLE --> DRINK_ROUTE[Hose pump ON<br/>RO pump ON<br/>Drink valve OPEN]
    AUTO_CLASS -- MCK ONLY --> MCK_ROUTE
    AUTO_CLASS -- UNSAFE --> BLOCKED

    MODE -- DRINK --> DRINK_OK{"Class is DRINKABLE?"}
    DRINK_OK -- Yes --> DRINK_ROUTE
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

    User->>Buttons: Select MCK, AUTO, or DRINK
    Buttons-->>ESP32: Active-low button input
    ESP32->>ESP32: Latch selected mode

    loop Every 250 ms
        ESP32->>Sensors: Read GPIO34 and GPIO35 ADC values
        Sensors-->>ESP32: Raw pH and turbidity values
        ESP32->>ESP32: Map readings and classify water
        ESP32->>ESP32: Calculate outputs from mode + water class

        alt Drinking route permitted
            ESP32->>Relays: Hose ON, RO ON, Drink OPEN
        else MCK route permitted
            ESP32->>Relays: Hose ON, MCK OPEN
        else Unsafe or mode requirement not met
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
| AUTO | Hose pump + RO pump + drink valve | Hose pump + MCK valve | All off |
| DRINK | Hose pump + RO pump + drink valve | All off | All off |

The Wokwi relay modules use the `pnp` setting, so the firmware drives their IN pins HIGH to connect COM to NO. The LEDs connected to NO indicate active outputs.

## Build and simulate

Run `pio run` in this directory. Then start the Wokwi simulator from VS Code; `wokwi.toml` points to the generated ESP32 firmware and ELF files.
