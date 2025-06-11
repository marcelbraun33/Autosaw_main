# Autosaw main

This repository contains firmware for the **Teknic ClearCore** platform. The project files were created with the Visual Micro extension for Visual Studio and rely on that toolchain.

## Prerequisites

- **Visual Studio** (2019 or newer recommended)
- **[Visual Micro](https://www.visualmicro.com/)** extension installed in Visual Studio
- Teknic **ClearCore** board support package installed through Visual Micro/Arduino

## Build and Upload

1. Open `Autosaw_main.vcxproj` in Visual Studio.
2. Ensure Visual Micro is enabled and set the board type to **ClearCore**.
3. Select the desired configuration (Debug/Release) and build the project.
4. Connect the ClearCore board via USB and use Visual Micro's upload button to flash the firmware.

## Running

After uploading, the firmware will start executing on the ClearCore board. The USB serial console can be used for debug messages and interaction.
