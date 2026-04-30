# 🚀 NavSystem ESP32 Walkthrough

This guide provides step-by-step instructions on how to build, upload, and run the **NavSystem ESP32** firmware using **PlatformIO**.

## 📌 Prerequisites

Before you begin, ensure you have the following installed:
- **VS Code** with the **PlatformIO IDE** extension.
- **Python 3.x** (required by PlatformIO).
- USB-to-Serial drivers for your ESP32 (usually CP210x or CH340).

## 🛠️ Hardware Specification

The project is pre-configured for the **Sunton ESP32-3248S035** (3.5" TFT with Capacitive/Resistive Touch). The configuration is handled entirely via `build_flags` in `platformio.ini`, so no library modifications are needed.

- **Display Driver:** ST7796
- **Resolution:** 320x480
- **Communication:** SPI

---

## 💻 Running via VS Code (Recommended)

### 1. Outdoor High-Visibility Overhaul
I have completely redesigned the UI for maximum legibility in direct sunlight (bike-mounted use):
- **Light Theme**: The background is now a clean Light Grey (`#F0F0F0`) which provides perfect contrast for the black navigation icons.
- **High-Contrast Header**: The top header is now white (`#FFFFFF`) with a sharp black Speedometer using a larger **Montserrat 40** font.
- **Large Action Card**: The navigation card is now white with a thicker border, ensuring it remains visible even in bright glare.
- **Maximized Icons**: Navigation icons have been enlarged (Zoom 300) for instant recognition at a glance.
- **Bold Typography**: 
    - **Distance**: Changed to a strong "Sport Blue" (`Montserrat 32`) to make it the secondary focal point.
    - **Instructions**: Black text on white for absolute clarity.

### 2. Enhanced Scale
- **Increased Header Height**: Bumped to 80px to accommodate the larger speedometer and clock.
- **Larger Clock**: The clock container and text size have been increased for easier reading.

## 🛠️ Technical Updates

- **`lv_conf.h`**: Enabled `LV_FONT_MONTSERRAT_40` for the high-vis speedometer.
- **`ui_Screen1.c/h`**: Transitioned Speedometer and Clock to **Sport Blue** (`#007AFF`) and updated the layout to handle a 12-hour AM/PM clock format.
- **`main.cpp`**: Implemented **Conditional UI Refreshing** and added **Call Interaction Handlers**. Enabled BLE Notifications so the display can send "Accept/Decline" actions back to the phone when buttons are pressed.
- **`BleService.kt` (Mobile)**: Implemented a **Robust Reconnection Engine**. This includes a 10-second watchdog that force-restarts scanning if the link is dead, and the use of `TRANSPORT_LE` to ensure faster handshakes with the ESP32.
- **`main.cpp`**: Optimized **Advertising Intervals**. The display now restarts its search signal within 500ms of a disconnect and uses power-optimized parameters to remain visible to the phone's background scanner.
- **`ui_Screen1.c`**: Redesigned the Call Overlay with a premium aesthetic.
- **`AndroidManifest.xml`**: Added permissions for `ANSWER_PHONE_CALLS`, `READ_PHONE_STATE`, and `READ_CONTACTS`.

1. **Open Project:**
   - Launch VS Code.
   - Click the **PlatformIO** icon (the ant head) in the left sidebar.
   - Select **Pick a folder** and choose `NavigationSystemESP32`.

2. **Build:**
   - Click the **Checkmark** icon (Build) in the PlatformIO bottom toolbar.
   - Ensure the terminal shows `SUCCESS`.

3. **Upload:**
   - Connect your ESP32 via USB.
   - Click the **Right Arrow** icon (Upload) in the bottom toolbar.

4. **Monitor:**
   - Click the **Plug** icon (Serial Monitor) to view real-time debug logs.
   - Set the baud rate to `115200` (configured automatically in `platformio.ini`).

---

## ⌨️ Running via CLI

If you prefer using the terminal, navigate to the `NavigationSystemESP32` directory and use the following commands:

### 1. Build the project
```bash
pio run
```

### 2. Upload to ESP32
```bash
pio run --target upload
```

### 3. Open Serial Monitor
```bash
pio device monitor
```

---

## ✅ Verification

Once uploaded, check the following:
1. **Display:** The screen should initialize and show the LVGL-designed navigation interface.
2. **Serial Output:** You should see logs indicating:
   - `Hello Arduino! V8.3.11`
   - `Waiting for BLE connection...`
   - `Setup done`
3. **Touch:** If touch data is received, the Serial Monitor will print `Data x ...` and `Data y ...`.

## 🔧 Troubleshooting

- **Failed to Connect:** Ensure your ESP32 is in bootloader mode (hold the `BOOT` button if necessary while connecting).
- **White Screen:** Check if the display driver in `platformio.ini` matches your hardware version.
- **Library Errors:** All libraries are contained in the `lib/` folder. If errors persist, try deleting the `.pio` folder and rebuilding.

---

> [!TIP]
> To update the UI, modify the files in `src/` or use **SquareLine Studio** to export new assets directly into the `src/` directory.
