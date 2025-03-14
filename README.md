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

## Testing

There are two primary methods for testing the ESP32-S3 Fluoroscopy Controller:

### MATLAB Testing (R2023b)
MATLAB provides built-in functionality to communicate with Bluetooth devices directly from the command console:

1. Type `blelist` into the command console to display all advertising BLE devices visible to your computer
2. Connect to the controller by executing: `b = ble("Fluoro Sim Controls")`
3. Once connected, you'll see a box showing services and characteristics of the device
4. Access the characteristic by running: `n = characteristic(b, "1848", "03C4")`
5. Read the data packet with: `read(n)`
6. Always clear the connection when finished by typing `clear` in the console

**Note:** On disconnect, the controller is programmed to perform a software restart and will begin advertising again.

### NRF Connect (Phone App)
This free mobile application offers an alternative testing method:

1. Scan for BLE devices (the app will show many more devices than MATLAB)
2. Locate and connect to the "Fluoro Sim Controls" device
3. View the service (0x1848) and characteristic (0x03C4)
4. Test functionality by pressing buttons on the controller to observe value changes

While NRF Connect is more accessible, it may be less intuitive than MATLAB for development purposes, particularly for viewing the complete characteristic data frame.

### Integration Testing with Client Applications

When integrating the controller with your client application:

1. Implement client-side BLE connectivity using your platform's Bluetooth API
2. Subscribe to the characteristic (0x03C4) notifications
3. Parse the data according to the `NEW_GamepadReport` structure
4. Verify all joysticks and buttons map correctly to your application's functions

The client application should handle the BLE connection management and properly interpret the controller data packets based on the data structure defined in the technical documentation.

## Notes

- The controller auto-restarts on client disconnect
- Joystick values are scaled to 8-bit (0-255) from the ESP32's 12-bit ADC
- ESP-NOW functionality was removed but hooks remain for potential reimplementation 

## Contact

For questions or support: chasetheiler@gmail.com or theil027@umn.edu
