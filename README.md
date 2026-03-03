# WebServerESP-6205: Wi-Fi Web Server for BO6502

[![Photo](https://github.com/Boogs77/WebServerESP-6205/raw/main/gallery/WebServerESP_image1.png)](https://github.com/Boogs77/WebServerESP-6205/blob/main/gallery/WebServerESP_image1.png)

The **WebServerESP-6205** is a Wi-Fi expansion module for the [BO6502 modular computer](https://github.com/Boogs77/BO6502_65C02-based_modular_computer). Based on the **ESP8266** (ESP-01 or NodeMCU), it acts as a bridge between the BO6502 serial bus and any Wi-Fi network, exposing a lightweight HTTP web server that allows remote monitoring and control of the 65C02-based system directly from a browser.

---

## 🎬 Demo Video

[![Watch the demo](https://img.shields.io/badge/▶%20Watch-Demo%20Video-red?style=for-the-badge)](https://github.com/Boogs77/WebServerESP-6205/raw/main/gallery/WebServerESP_video1.mp4)

> Click the badge above to watch the WebServerESP-6205 in action, or open the video directly from the [`gallery/`](https://github.com/Boogs77/WebServerESP-6205/tree/main/gallery) folder.

---

## 🧠 How It Works

The ESP8266 communicates with the BO6502 via the **Serial Module** ([BO6502 SERIAL](https://github.com/Boogs77/BO6502_65C02-based_modular_computer/tree/main/BO6502%20SERIAL)) using a standard UART interface. The 65C02 sends data through the serial port; the ESP8266 receives it and serves it over HTTP to any connected browser on the same Wi-Fi network.

Key features:
- **Wi-Fi Web Server** hosted on ESP8266 (port 80)
- **UART bridge** between BO6502 Serial Module and ESP8266
- **Real-time data display** from the 65C02 in the browser
- Lightweight firmware written in **C++ (Arduino framework)**

---

## 🔌 Wiring & Connections

The ESP8266 connects to the **BO6502 SERIAL module** (Rev03, address `$CXXX`) via the UART lines. Below is the full connection diagram.

### ESP8266 (ESP-01) ↔ BO6502 SERIAL Module

```
  BO6502 SERIAL Module (Rev03)           ESP8266 ESP-01
  ┌──────────────────────────┐           ┌─────────────┐
  │                          │           │             │
  │  TX  (Serial out → CPU)  │──────────▶│  RX  (GPIO3)│
  │                          │           │             │
  │  RX  (Serial in ← CPU)   │◀──────────│  TX  (GPIO1)│
  │                          │           │             │
  │  GND                     │───────────│  GND        │
  │                          │           │             │
  │  VCC (3.3V)              │───────────│  VCC (3.3V) │
  │                          │           │             │
  └──────────────────────────┘           │  CH_PD ─ VCC│
                                         │  RST   ─ VCC│
                                         └─────────────┘
```

> ⚠️ **Important:** The BO6502 SERIAL module uses **3.3V logic levels** through its onboard level shifter. The ESP8266 also operates at 3.3V. Do **not** connect directly to the BO6502 backplane bus (5V) without a level converter.

### Signal Reference

| ESP8266 Pin | BO6502 SERIAL Signal | Direction    | Notes                          |
|-------------|----------------------|--------------|-------------------------------|
| TX (GPIO1)  | RX                   | ESP → Serial | ESP transmits to BO6502        |
| RX (GPIO3)  | TX                   | Serial → ESP | BO6502 transmits to ESP        |
| GND         | GND                  | —            | Common ground                  |
| VCC         | 3.3V                 | —            | Power from Serial module or ext|
| CH_PD       | —                    | —            | Tie to VCC to enable module    |
| RST         | —                    | —            | Tie to VCC for normal operation|

### BO6502 SERIAL Module Reference

The **BO6502 SERIAL** module (Rev03) provides asynchronous RS-232-compatible UART communication for the BO6502 system:

- **Chip:** MOS **6551 ACIA** (Asynchronous Communications Interface Adapter)
- **Address:** `$CXXX` range on the BO6502 memory map
- **Baud Rate:** Configurable (typically 9600 or 19200 baud)
- **Logic Level:** Onboard MAX232 / level-shifter for RS-232 compatibility; 3.3V side available for direct ESP connection
- **Reference:** [BO6502 SERIAL Module](https://github.com/Boogs77/BO6502_65C02-based_modular_computer/tree/main/BO6502%20SERIAL)

[![Serial PCB](https://github.com/Boogs77/BO6502_65C02-based_modular_computer/blob/main/BO6502%20SERIAL/gallery/Serial_final_rev03.png?raw=true)](https://github.com/Boogs77/BO6502_65C02-based_modular_computer/blob/main/BO6502%20SERIAL/gallery/Serial_final_rev03.png)

---

## ⚙️ Firmware & Setup

The firmware is written for the **Arduino IDE** with the ESP8266 core.

### Required Libraries

- `ESP8266WiFi.h`
- `ESP8266WebServer.h`
- `SoftwareSerial.h` *(optional, if using non-default UART pins)*

### Configuration

Before flashing, edit the following parameters in the sketch:

```cpp
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const int   baudRate = 9600;   // Match BO6502 SERIAL baud rate
```

### Flashing

The pre-compiled binary is available in the [`bin/`](https://github.com/Boogs77/WebServerESP-6205/tree/main/bin) folder and can be flashed directly with `esptool`:

```bash
esptool.py --port /dev/ttyUSB0 --baud 115200 write_flash 0x00000 firmware.bin
```

---

## 🖼️ Project Gallery

[![](https://github.com/Boogs77/WebServerESP-6205/raw/main/gallery/WebServerESP_image1.png)](https://github.com/Boogs77/WebServerESP-6205/blob/main/gallery/WebServerESP_image1.png)

---

## 🔗 Useful Links & Resources

- **BO6502 Modular Computer:** [Main Repository](https://github.com/Boogs77/BO6502_65C02-based_modular_computer)
- **SERIAL Module (Rev03):** [BO6502 SERIAL Reference](https://github.com/Boogs77/BO6502_65C02-based_modular_computer/tree/main/BO6502%20SERIAL)
- **CPU Module:** [BO6502 CPU Reference](https://github.com/Boogs77/BO6502_65C02-based_modular_computer/tree/main/BO6502%20CPU)
- **miniPET Project:** [Boogs77/miniPET_65c02](https://github.com/Boogs77/miniPET_65c02)
- **ESP8266 Arduino Core:** [esp8266/Arduino](https://github.com/esp8266/Arduino)

---

*Created by Boogs77 - 2026*
