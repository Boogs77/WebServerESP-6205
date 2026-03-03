# WebServerESP-6205: Wi-Fi Web Server for BO6502

[![Photo](https://github.com/Boogs77/WebServerESP-6205/raw/main/gallery/WebServerESP_image1.png)](https://github.com/Boogs77/WebServerESP-6205/blob/main/gallery/WebServerESP_image1.png)

The **WebServerESP-6205** is a Wi-Fi expansion module for the [BO6502 modular computer](https://github.com/Boogs77/BO6502_65C02-based_modular_computer). Based on the **ESP32-WROOM-32D**, it bridges the BO6502 serial port to a Wi-Fi network, exposing a retro-styled **HTTP terminal** — directly usable from any browser, no software required.

The web interface faithfully reproduces the aesthetics of a **Commodore PET** with a simulated LCD display, a physical-style keyboard, and real-time bidirectional communication via **WebSocket**.

---

## 🎬 Demo Video

[![Watch the demo](https://img.shields.io/badge/▶%20Watch-Demo%20Video-red?style=for-the-badge)]([https://www.youtube.com/watch?v=nsjVCyOfUFU(https://www.youtube.com/watch?v=nsjVCyOfUFU)]

---

## 🧠 How It Works

The ESP32-WROOM-32D receives data from the BO6502 via the **SERIAL module** (Rev03, 6551 ACIA) through the **HW-044 RS-232 to TTL converter**, using **UART2** at **19200 baud 8N1**. The data is served in real time to the browser via **WebSocket** on port 80. The user can also type commands directly in the browser and send them back to the 65C02.

Key features:
- **Wi-Fi Web Server** (port 80) with retro **miniPET-style** HTML interface
- **WebSocket** (`/ws`) for real-time bidirectional communication
- **Simulated LCD display** (20×16 chars) with scanline effect and blinking cursor
- **Clickable PET keyboard** in the browser — sends characters directly to the 65C02
- **ANSI ESC sequence detection** (`ESC[2JESC[H`) to clear the terminal remotely
- **Static IP** configurable directly in the sketch
- **UART2** (GPIO16/GPIO17) dedicated to BO6502 — UART0 free for USB/debug

---

## 🔌 Wiring & Connections

Between the ESP32 and the BO6502 SERIAL module sits the **HW-044** (MAX3232-based RS-232 ↔ TTL converter), which translates the ±12V RS-232 levels of the ACIA output to 3.3V logic compatible with the ESP32.

### Full Chain

```
  BO6502 SERIAL Module (Rev03)
  6551 ACIA — address $CXXX
  19200 baud / 8N1
         │
         │  RS-232 (±12V)  DB9
         ▼
  ┌─────────────────┐
  │    HW-044       │  MAX3232 RS-232 ↔ TTL 3.3V
  │                 │
  │  R1IN  ◀── DB9 TX │  (RS-232 from ACIA TX)
  │  T1OUT ──▶ DB9 RX │  (RS-232 to  ACIA RX)
  │                 │
  │  R1OUT ──▶ TXD  │ ──▶ GPIO16 (RX2) ESP32
  │  T1IN  ◀── RXD  │ ◀── GPIO17 (TX2) ESP32
  │                 │
  │  VCC   ◀── 3.3V │
  │  GND   ◀── GND  │
  └─────────────────┘
         │
         │  TTL 3.3V
         ▼
  ┌───────────────────────┐
  │   ESP32-WROOM-32D     │
  │                       │
  │  GPIO16  (RX2/UART2)  │ ◀── HW-044 R1OUT (data from BO6502)
  │  GPIO17  (TX2/UART2)  │ ──▶ HW-044 T1IN  (data to   BO6502)
  │  3.3V                 │ ──▶ HW-044 VCC
  │  GND                  │ ──▶ HW-044 GND
  │                       │
  │  GPIO1  (TX0/UART0)   │ ──▶ USB / Arduino IDE (debug)
  │  GPIO3  (RX0/UART0)   │ ◀── USB / Arduino IDE (flash)
  └───────────────────────┘

  Baud rate: 19200  │  Format: 8N1  │  Logic HW-044 → ESP32: 3.3V
```

### Pin Reference Table

| ESP32-WROOM-32D | HW-044 Pin | BO6502 SERIAL | Direction       | Notes                               |
|-----------------|------------|---------------|-----------------|-------------------------------------|
| GPIO16 (RX2)    | R1OUT      | ACIA TX       | BO6502 → ESP32  | Receive data from 65C02             |
| GPIO17 (TX2)    | T1IN       | ACIA RX       | ESP32 → BO6502  | Send commands to 65C02              |
| 3.3V            | VCC        | —             | Power           | Powers the HW-044 module            |
| GND             | GND        | —             | Common ground   |                                     |
| GPIO1 (TX0)     | —          | —             | USB/PC          | Reserved: Arduino IDE serial debug  |
| GPIO3 (RX0)     | —          | —             | USB/PC          | Reserved: Arduino IDE upload        |

> ⚠️ **Important:** Connect the ESP32 to the **TTL side** (R1OUT / T1IN) of the HW-044 — never directly to the DB9 RS-232 connector, which carries ±12V and would damage the ESP32.

### BO6502 SERIAL Module Reference

The **BO6502 SERIAL** module (Rev03) provides asynchronous UART communication for the BO6502 system:

- **Chip:** MOS **6551 ACIA** (Asynchronous Communications Interface Adapter)
- **Address:** `$CXXX` range on the BO6502 memory map
- **Baud Rate:** **19200 baud**, 8N1
- **Output:** RS-232 via DB9 connector (MAX232 onboard) → connect to HW-044 input
- **Reference:** [BO6502 SERIAL Module](https://github.com/Boogs77/BO6502_65C02-based_modular_computer/tree/main/BO6502%20SERIAL)

[![Serial PCB](https://github.com/Boogs77/BO6502_65C02-based_modular_computer/blob/main/BO6502%20SERIAL/gallery/Serial_final_rev03.png?raw=true)](https://github.com/Boogs77/BO6502_65C02-based_modular_computer/blob/main/BO6502%20SERIAL/gallery/Serial_final_rev03.png)

---

## ⚙️ Firmware & Setup

The firmware is an **Arduino IDE sketch** (`.ino`), open source and available directly in this repository. No pre-compiled binary is distributed — simply open the sketch in the Arduino IDE, configure your network parameters, and upload.

### Arduino IDE Setup

1. Install the **ESP32 board package**:  
   `File → Preferences → Additional Board Manager URLs`:  
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
2. In `Tools → Board` select **ESP32 Dev Module**.
3. Set `Tools → Upload Speed` to `115200`.

### Required Libraries

Install via `Sketch → Include Library → Manage Libraries`:

| Library | Author |
|---|---|
| **ESPAsyncWebServer** | Mathieu Carbou |
| **AsyncTCP** | Mathieu Carbou |

`WiFi.h` and `HardwareSerial.h` are included with the ESP32 core — no additional installation needed.

### Sketch Configuration

Edit the following parameters at the top of the `.ino` before uploading:

```cpp
// ─── Wi-Fi ───────────────────────────────────────────────────────
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// ─── Static IP (adjust to your network) ──────────────────────────
IPAddress local_IP(192, 168, 1, 150);
IPAddress gateway(192, 168, 1,   1);
IPAddress subnet (255, 255, 255, 0);

// ─── UART2 — BO6502 SERIAL module via HW-044 ─────────────────────
#define RS232_BAUD   19200   // must match 6551 ACIA configuration
#define RS232_RX_PIN 16      // GPIO16 ← HW-044 R1OUT
#define RS232_TX_PIN 17      // GPIO17 → HW-044 T1IN
```

The UART2 port is initialized in `setup()` as:

```cpp
Serial2.begin(RS232_BAUD, SERIAL_8N1, RS232_RX_PIN, RS232_TX_PIN);
```

### Accessing the Terminal

Once the ESP32 connects to the Wi-Fi network, open a browser and navigate to:

```
http://192.168.1.150/
```
*(or the IP shown on the Arduino IDE serial monitor at startup)*

---

## 🖥️ Compatible Systems

The WebServerESP-6205 works with any BO6502-based system equipped with the SERIAL module. It has been tested on:

### [BO6502 Modular Computer](https://github.com/Boogs77/BO6502_65C02-based_modular_computer)
The full modular backplane system, connecting the SERIAL module (Rev03) via DB9 → HW-044 → ESP32.

### [miniPET 65C02](https://github.com/Boogs77/miniPET_65c02)
The compact all-in-one retro computer in a 3D-printed Commodore PET-style enclosure. The miniPET integrates the same BO6502 SERIAL module at address `$CXXX` — the wiring and baud rate are identical.

[![miniPET](https://github.com/Boogs77/miniPET_65c02/raw/main/gallery/miniPET_image1.png)](https://github.com/Boogs77/miniPET_65c02)

> Connect the HW-044 to the miniPET's SERIAL module DB9 port exactly as described in the [Wiring section](#-wiring--connections) above — no firmware changes needed.

---

## 🖼️ Project Gallery

[![](https://github.com/Boogs77/WebServerESP-6205/raw/main/gallery/WebServerESP_image1.png)](https://github.com/Boogs77/WebServerESP-6205/blob/main/gallery/WebServerESP_image1.png)

---

## 🔗 Useful Links & Resources

- **BO6502 Modular Computer:** [Main Repository](https://github.com/Boogs77/BO6502_65C02-based_modular_computer)
- **SERIAL Module (Rev03):** [BO6502 SERIAL Reference](https://github.com/Boogs77/BO6502_65C02-based_modular_computer/tree/main/BO6502%20SERIAL)
- **CPU Module:** [BO6502 CPU Reference](https://github.com/Boogs77/BO6502_65C02-based_modular_computer/tree/main/BO6502%20CPU)
- **miniPET Project:** [Boogs77/miniPET_65c02](https://github.com/Boogs77/miniPET_65c02)
- **ESPAsyncWebServer:** [mathieucarbou/ESPAsyncWebServer](https://github.com/mathieucarbou/ESPAsyncWebServer)
- **ESP32 Arduino Core:** [espressif/arduino-esp32](https://github.com/espressif/arduino-esp32)

---

*Created by Boogs77 - 2026*
