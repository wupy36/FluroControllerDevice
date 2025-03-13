# ESP32-S3 Fluoroscopy Control Panel

A low-latency wireless Bluetooth control interface for fluoroscopy simulation, developed for Mayo Clinic's Anatomical Modeling Lab.

## Features

- Multiple joystick input (5 dual-axis joysticks = 10 analog inputs)
- Expandable button arrays using MCP23017 I/O expanders
- Custom BLE service (0x1848) and characteristic (0x03C4) for data transmission
- Low-latency wireless communication
- LED status indicators for connection and advertising states

## Hardware Requirements

- ESP32-S3 microcontroller
- MCP23017 I/O expander (address: 0x24)
- 5 dual-axis joysticks with push-down functionality
- Connection LEDs (pins 21, 26)
- I2C connections for MCP23017 (SDA: pin 33, SCL: pin 34)

## Pinout

```
Pins [0-15] follow pattern:
[0]      [1]   [2]    [3]   [4]   [5]    [6]   [7]   [8]   [9]   [10]...
NC     sDown  axisY  axisX sDown  axisY  axisX sDown axisY axisX sDown...
```

Where:
- `sDown`: Joystick push-down button (pins 1, 4, 7, 10, 13)
- `axisY`: Joystick Y-axis (pins 2, 5, 8, 11, 14)
- `axisX`: Joystick X-axis (pins 3, 6, 9, 12, 15)

## BLE Data Structure

The controller broadcasts a custom data structure over BLE:

```cpp
struct NEW_GamepadReport{
   uint8_t x1, y1;       // Joystick 1 axes
   uint8_t x2, y2;       // Joystick 2 axes
   uint8_t x3, y3;       // Joystick 3 axes
   uint8_t x4, y4;       // Joystick 4 axes
   uint8_t x5, y5;       // Joystick 5 axes
   uint8_t buttons1;     // MCP port A button states
   uint8_t buttons2;     // MCP port B button states
   uint8_t buttons3;     // Reserved
   uint8_t buttons4;     // Reserved
   uint8_t joysticksDown;// Joystick push-button states
   uint8_t extra2;       // Extra data (currently fixed at 0xEF)
   uint32_t large1-6;    // Reserved space for future expansion
};
```

## Getting Started

1. Connect hardware according to the pinout specification
2. Upload the code to an ESP32-S3
3. Monitor the serial output (115200 baud) for connection status
4. Connect to "Fluoro Sim Controls" from your client device
5. Read characteristic 0x03C4 from service 0x1848 to receive input data

## Notes

- The controller auto-restarts on client disconnect
- Joystick values are scaled to 8-bit (0-255) from the ESP32's 12-bit ADC
- ESP-NOW functionality was removed but hooks remain for potential reimplementation 

## Contact

For questions or support: chasetheiler@gmail.com or theil027@umn.edu
