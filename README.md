# 📟 BikeNavESP32 Dashboard

A high-performance, real-time navigation dashboard for motorcycles and bicycles. This project acts as the physical interface for the **BikeNav** ecosystem, receiving and visualizing telemetry and guidance data from the Android Companion app via low-latency BLE.

## 🌌 Features

- **Real-time Navigation**: Synchronized turn-by-turn guidance with maneuver icons, distance, and ETA.
- **Smart Speedometer**: Integrated speed display with visual safety alerts:
  - **White**: Normal speeds (< 55 km/h)
  - **Orange**: Caution threshold (55 - 70 km/h)
  - **Red**: Danger/High-speed threshold (> 70 km/h)
- **Phone Health Monitoring**: Live sync of phone battery percentage (🔋) and signal strength (📶).
- **Communication Hub**: Incoming call notifications with caller ID and remote accept/decline actions.
- **Context Awareness**: "Smart Throttling" support to maximize battery life during cruising.

## 🛠️ Hardware Stack

- **MCU**: ESP32 (38-pin DevKit recommended)
- **Display**: 3.5" TFT with **ST7796** Driver (320x480 resolution)
- **Touch**: Capacitive or Resistive touch controller support
- **UI Framework**: [LVGL v8.3](https://lvgl.io/)

**TIP**
Search on `Aliexpress` for: **3.5 inch TFT LCD 320x480 ST7796 for ESP32** OR `Google` for **Cheap Yellow Display ESP32 (3.5 inch)** to skip DIYing your own PCB or Display.

## 📱 Screenshots

| Screenshot                         | Description |
| :--------------------------------- | :---------- | ------------------- |
| ![Screenshot](screenshots/nav.jpg  | width=200)  | Navigation Screen   |
| ![Screenshot](screenshots/call.jpg | width=200)  | Incoming Call Popup |

## 🚀 Deployment

### Quick Start (Pre-compiled Binary)

The easiest way to get started is by flashing the unified firmware file:

1.  Download [BikeNav_v1_Full.bin](https://github.com/rabbihossain/BikeNavESP32/releases/download/1.0/BikeNav_v1_Full.bin).
2.  Use a web flasher like [ESP Web Flasher](https://web.esphome.io/).
3.  Set the flashing address to **0x0**.
4.  Flash and Reboot.

### Development Setup (PlatformIO)

1.  Install [PlatformIO IDE](https://platformio.org/).
2.  Clone this repository.
3.  Adjust `platformio.ini` if using different pin configurations.
4.  Build and Upload:
    ```bash
    pio run -t upload
    ```

## 📡 Protocol

The system communicates via BLE using a lightweight JSON schema.

```json
{
  "nav": {
    "m": "Maneuver_ID",
    "d": "Distance",
    "t": "ETA"
  },
  "speed": "65",
  "bat": 85,
  "sig": 4
}
```

## 🤝 System Architecture

This project is part of a dual-component system. To function correctly, it must be paired with the [**BikeNavMobile**](https://github.com/rabbihossain/BikeNavMobile) companion android application.

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.
