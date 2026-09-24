# ESP32-S3 USB HID Web Typing Interface

## This project turns an ESP32-S3 into a USB HID keyboard with a built-in web interface that allows keyboard input from a web browser to be sent to the ESP32 and forwarded as USB HID keyboard input to another computer.

## Features

* USB HID Keyboard (TinyUSB)
* Web interface hosted directly on the ESP32
* Keyboard input from a web browser
* WebSocket communication between browser and ESP32
* Keyboard input independent of the keyboard layout configured on the connected USB host
* Support for letters, numbers, function keys, navigation keys, numpad keys and additional international keys
* Support for left and right Ctrl, Shift, Alt and Meta modifiers
* USB HID connection status reported to the web interface
* Web interface is enabled only while the ESP32 USB HID device is connected
* mDNS support (`.local` address)
* Serial debug output
* PlatformIO compatible

---

## Hardware Required

### Main Board

* ESP32-S3 development board with native USB
* Tested with: ESP32-S3 DevKit

### Wiring

No additional buttons or external hardware are required.

* USB-C data cable
* Wi-Fi connection

---

## Software Setup (PlatformIO)

### Install Tools

* Visual Studio Code
* PlatformIO extension

The project uses the Arduino framework for the ESP32-S3.

---

## Wi-Fi Configuration

Wi-Fi credentials are stored in a separate header file to keep them out of the repository.

Before building the project:

1. Copy `src/wifi_credentials.example.h` to `src/wifi_credentials.h`.
2. Open `src/wifi_credentials.h` and enter your Wi-Fi network name and password:

```cpp
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
```

3. Save the file.

The `wifi_credentials.h` file contains your personal Wi-Fi credentials and must not be committed to the repository.

The example file `wifi_credentials.example.h` is provided as a template and can safely remain in the repository.

---

## Uploading and Running

### USB Connections

The ESP32-S3 uses its native USB connection for the HID keyboard.

The USB connection is used to present the ESP32-S3 to the host computer as a USB HID keyboard.

### Upload Firmware

Use PlatformIO "Upload".

### Serial Monitor

Use PlatformIO "Monitor".

The firmware outputs information about startup, Wi-Fi connection, IP address and USB HID connection status.

After successfully connecting to Wi-Fi, the ESP32 prints its assigned IP address and its mDNS address.

---

## Web Interface

Open the web interface in a browser using the IP address reported by the ESP32 or the configured mDNS hostname:

http://web-typing-interface.local

The web page contains a single text input area.

The input area is disabled while the USB HID device is not connected to a host computer. Once the ESP32-S3 USB HID device is detected, the input area is enabled automatically.

Keyboard input is processed directly in the browser and transmitted to the ESP32 via WebSocket.

---

## Keyboard Input

The web interface supports USB HID keyboard input for:

* Letters (`A`–`Z`)
* Number row (`0`–`9`)
* Enter, Escape, Backspace, Tab and Space
* Main keyboard punctuation keys
* Caps Lock
* Function keys (`F1`–`F24`)
* Navigation and system keys
* Arrow keys
* Numeric keypad
* Additional international and IME keys

Modifier keys are supported independently for left and right:

* Left / Right Ctrl
* Left / Right Shift
* Left / Right Alt
* Left / Right Meta

Modifier keys are tracked while typing and are included in the keyboard report when another key is sent.

---

## WebSocket Communication

The browser establishes a WebSocket connection to port `81` of the ESP32.

Keyboard input is handled using the browser's `KeyboardEvent.code` value. Unlike the character produced by a keyboard key, `KeyboardEvent.code` identifies the physical key independently of the keyboard layout configured on the connected computer.

The corresponding USB HID keyboard code is sent to the ESP32 together with the currently active modifier keys. The ESP32 then generates a USB HID keyboard report for the connected computer.

This approach makes the keyboard input independent of the keyboard layout configured on the USB host. The ESP32 does not send characters such as `y` or `z`; it sends the corresponding physical keyboard key as a USB HID code. The connected computer interprets this key according to its own keyboard layout.

For keyboard input, the browser sends a two-byte binary packet:

```text
Byte 0: USB HID modifier mask
Byte 1: USB HID key code
```

The first byte contains the state of the following modifier keys:

* Left / Right Ctrl
* Left / Right Shift
* Left / Right Alt
* Left / Right Meta

Modifier keys themselves are not sent as individual keyboard reports. Their state is included in the modifier mask when another key is transmitted.

On the ESP32, the two bytes are used to create a USB HID keyboard report. The report is sent to the connected USB host and the key is released again after a short delay.

The ESP32 also sends the following status messages to the browser:

```text
connected
disconnected
```

These messages indicate whether the ESP32 USB HID device is currently mounted by the USB host.

---

## mDNS

The ESP32 advertises the web interface using mDNS.

The default hostname is:

`web-typing-interface.local`

This allows the web interface to be accessed without manually determining the ESP32's IP address, provided that mDNS is available on the local network.

---

## Debugging Notes

The Serial Monitor can be used to verify:

* ESP32 startup
* Wi-Fi connection
* Assigned IP address
* WebSocket client connections
* Received HID modifier masks
* Received HID key codes
* USB HID connection and disconnection

The WebSocket server listens on port `81`.

The HTTP web server listens on port `80`.

---

## Notes

* The ESP32-S3 requires a native USB connection for USB HID functionality.
* A USB-C cable capable of data transfer is required.
* The ESP32-S3 connects to Wi-Fi during startup.
* The web interface requires a browser with WebSocket support.
* Keyboard input is generated by the ESP32 as USB HID input, so the connected computer treats it like input from a physical USB keyboard.
* The current implementation sends individual key press/release reports. It does not implement human-like typing delays, text buffering or macro execution.

---

## License

MIT License

---

## Credits

This project uses the following open-source projects:

* [Arduino ESP32 Core](https://github.com/espressif/arduino-esp32) – Arduino framework and ESP32 support
* [TinyUSB](https://github.com/hathach/tinyusb) – USB stack used for USB HID functionality
* [arduinoWebSockets](https://github.com/Links2004/arduinoWebSockets) – WebSocket communication between the web interface and the ESP32
* [PlatformIO](https://platformio.org/) – development and build environment
