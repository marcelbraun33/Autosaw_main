# Autosaw Main Firmware

This repository contains the firmware for the Autosaw system built on the Teknic ClearCore controller. The code drives multiple stepper/servo axes and interfaces with a pendant controller and touchscreen.

## Project Purpose
The Autosaw firmware coordinates spindle control and three axes (X, Y, and Z) to automate cutting operations. It provides both manual and semi‑automatic modes, user interface screens, and safety features such as E‑Stop handling.

## Hardware Setup
The board definition is **Teknic ClearCore (ClearCore_clearcore)** as shown in the Visual Micro configuration:

```
        Hardware: Teknic ClearCore (ClearCore_clearcore), Platform=sam, Package=ClearCore
```

Key hardware connections from `Config.h`:

```
#define MOTOR_SPINDLE       ConnectorM0   // Velocity mode (MCVC)
#define MOTOR_FENCE_X       ConnectorM3   // Step/Dir
#define MOTOR_TABLE_Y       ConnectorM2   // Step/Dir
#define MOTOR_ROTARY_Z      ConnectorM1   // Step/Dir

#define SELECTOR_PIN_X        ConnectorIO1   // Jog Fence
#define SELECTOR_PIN_Y        ConnectorIO2   // Jog Table
#define SELECTOR_PIN_Z        ConnectorIO3   // Jog Rotary
#define SELECTOR_PIN_SETUP    ConnectorIO4   // Setup AutoCut
#define SELECTOR_PIN_AUTOCUT  ConnectorIO5   // AutoCut Active
#define GENIE_RESET_PIN 6
```

These map the motors and pendant inputs to the ClearCore connectors. Adjust the `Config.h` constants if your wiring differs.

## Prerequisites
- **Teknic ClearCore SDK** – provides the board support package and libraries.
- **Visual Studio with Visual Micro** *or* the **Arduino IDE** – used to compile and upload.

## Building and Uploading
1. Install the ClearCore board package through the Arduino Board Manager or from the Teknic SDK.
2. Open `Autosaw_main.ino` in Visual Micro (Visual Studio) or the Arduino IDE.
3. Select the board **Teknic ClearCore** and choose the correct COM port.
4. Build and upload the sketch to the ClearCore.

## Folder Layout
- `Config.h` – pin mappings and machine constants.
- `src/` – optional source subfolder for Visual Micro projects.
- `__vm/` – Visual Micro generated files.

After uploading, the ClearCore will control the Autosaw hardware using the defined connectors and settings.
