# BLE Beacon - IoT Lighthouse Component

Standalone BLE beacon implementation for the IoT Lighthouse system. This device broadcasts its presence via Bluetooth Low Energy (BLE) for tracking by the main Lighthouse Node.

## Overview

The BLE Beacon runs on a **separate ESP32 device** and continuously advertises its MAC address. The Lighthouse Node detects this beacon during BLE scans and reports:
- **Beacon presence** (detected or not found)
- **Signal strength (RSSI)** - indicates approximate distance
- **Beacon reliability** - detected in uplink payload

## Use Cases

- **Presence Detection**: Know if a specific device (e.g., tracker, wearable) is in range
- **Proximity Monitoring**: Monitor signal strength to estimate distance
- **Asset Tracking**: Track important equipment across multiple Lighthouse Nodes
- **Occupancy Enhancement**: Combine with crowd detection for advanced analytics
- **Geofencing**: Trigger actions when beacon enters/exits range

## Hardware Requirements

- **ESP32 Development Board** (any variant):
  - Heltec WiFi LoRa 32 V3 (same as Lighthouse Node)
  - Standard ESP32-DevKitC
  - SparkFun ESP32 Thing
  - Any ESP32 with BLE capability
- **USB Cable** for programming and power
- **Power source** (USB, battery, or external supply)

## Quick Start

### Step 1: Upload Beacon Code

1. Open Arduino IDE
2. Open `BLE_Beacon.ino` from this folder
3. Select your ESP32 board and COM port
4. Click **Upload**
5. Keep this ESP32 powered on

### Step 2: Note the MAC Address

Open Serial Monitor (115200 baud) and you'll see:

```
========================================
  IoT Lighthouse - BLE Beacon
========================================

Initializing BLE device...
✓ BLE device initialized
✓ BLE server created
✓ BLE advertising started

--- Beacon Configuration ---
Name:               IoT-Lighthouse-Beacon
MAC Address:        24:6f:28:aa:bb:cc
Advertising Rate:   10 Hz (100ms)
----------------------------

📋 NEXT STEPS:
1. Copy the MAC Address above
2. Open LoRaWAN.ino on the Lighthouse Node
3. Update BEACON_MAC_ADDRESS in secrets.h:
   #define BEACON_MAC_ADDRESS "24:6f:28:aa:bb:cc"
4. Upload LoRaWAN.ino to the Lighthouse Node
5. The Lighthouse will detect this beacon!

🔋 Status: Beacon is now broadcasting continuously
💡 Keep this device powered on and within 30m of the Lighthouse
```

### Step 3: Configure Lighthouse Node

On your **Lighthouse Node** (LoRaWAN.ino):

1. Edit `LoRaWAN/secrets.h`
2. Update the MAC address:
   ```cpp
   #define BEACON_MAC_ADDRESS "24:6f:28:aa:bb:cc"
   ```
3. Upload to Lighthouse Node
4. Done! ✅

### Step 4: Verify Detection

- Open Serial Monitor on Lighthouse Node (115200 baud)
- You should see beacon detection status in the scan output
- Example:
  ```
  Starting BLE scan...
    >> Tracked beacon found! Address: 24:6f:28:aa:bb:cc, RSSI: -65
  BLE scan complete. Found 12 unique devices.
    Tracked beacon: DETECTED
  ```

## Configuration

### Beacon Name

To change the beacon's advertised name, edit line in `BLE_Beacon.ino`:

```cpp
#define BEACON_NAME "IoT-Lighthouse-Beacon"
```

**Note**: The name is for identification only. The Lighthouse tracks by MAC address, not name.

### Advertising Interval

Control how frequently the beacon broadcasts:

```cpp
#define ADVERTISING_INTERVAL 100  // milliseconds
```

- **100ms (10 Hz)**: Faster detection, higher power consumption
- **500ms (2 Hz)**: Balanced
- **1000ms (1 Hz)**: Slower detection, lower power consumption

### Optional: Status LED

Uncomment lines to enable LED blinking:

```cpp
#define LED_PIN 25
#define LED_BLINK_INTERVAL 1000
```

Connect an LED to the specified pin for visual status indication.

## Multiple Beacons

You can run multiple Lighthouse Nodes, and each Lighthouse tracks a different beacon:

```
Beacon A (MAC: AA:BB:CC:DD:EE:01)
  ↓ detected by
Lighthouse Node 1

Beacon B (MAC: AA:BB:CC:DD:EE:02)
  ↓ detected by
Lighthouse Node 2

Beacon C (MAC: AA:BB:CC:DD:EE:03)
  ↓ detected by
Lighthouse Node 1 or 3
```

Each Lighthouse can be configured to track one specific beacon by setting the MAC address in `secrets.h`.

## Understanding RSSI

RSSI (Received Signal Strength Indicator) indicates distance:

| RSSI Range | Approximate Distance | Notes |
|-----------|----------------------|-------|
| -30 to -50 dBm | < 1 meter | Very close (same device almost) |
| -50 to -70 dBm | 1-10 meters | Same room |
| -70 to -90 dBm | 10-30 meters | Different room or obstruction |
| < -90 dBm | > 30 meters | Far or very weak signal |
| -128 dBm | Not detected | Too far or beacon off |

**Important**: RSSI varies based on:
- Physical distance
- Obstacles (walls, furniture)
- Antenna orientation
- Environmental interference
- Time of day

Use relative changes more than absolute values.

## Troubleshooting

### Beacon Not Detected by Lighthouse

**Check:**
1. ✅ Beacon power - Is the beacon ESP32 powered on?
2. ✅ MAC address match - Does the MAC in secrets.h match exactly?
3. ✅ Distance - Are devices within 30 meters?
4. ✅ Serial Monitor output - Does beacon show "Beacon is now broadcasting"?

**Try:**
- Move beacon closer to Lighthouse
- Restart beacon (power cycle)
- Check Serial Monitor on both devices for errors
- Verify MAC address is exactly correct (case-sensitive)

### Signal Strength Always Weak

**Try:**
- Move beacon closer
- Move beacon to line-of-sight with Lighthouse
- Check for obstacles or interference
- Verify antenna is properly connected

### MAC Address Not Showing

**Try:**
1. Open Serial Monitor (115200 baud)
2. Wait 2 seconds after upload completes
3. Check baud rate setting
4. Verify USB cable is properly connected

### Beacon Won't Upload

**Try:**
1. Select correct board: "WiFi LoRa 32(V3)" or similar
2. Select correct COM port
3. Try different USB cable
4. Install latest ESP32 board package in Arduino IDE

## Power Consumption

- **Active Broadcasting**: ~10-30 mA (depends on advertising interval)
- **Deep Sleep**: ~5-10 mA (not implemented in basic version)

### For Battery Operation

To extend battery life:
1. Increase `ADVERTISING_INTERVAL` to 500-1000ms
2. Consider power-saving versions (future enhancement)
3. Use larger capacity battery
4. Disable Serial output in production

## Data Flow

```
┌─────────────────────┐
│   BLE Beacon        │
│  (This Device)      │
│  MAC: xx:xx:...     │
│  Broadcasts BLE Adv │
└──────────┬──────────┘
           │ BLE Signal
           │ (30m range)
           ↓
┌─────────────────────┐
│  Lighthouse Node    │
│  LoRaWAN.ino        │
│  Detects beacon     │
│  Measures RSSI      │
└──────────┬──────────┘
           │ LoRaWAN Uplink (10km range)
           │ 10-byte payload
           ↓
┌─────────────────────┐
│  The Things Network │
│  (TTN Console)      │
│  Stores data        │
└─────────────────────┘
           │ Integration
           ↓
┌─────────────────────┐
│  Dashboard/App      │
│  Displays beacon    │
│  presence & RSSI    │
└─────────────────────┘
```

## Advanced: Custom Beacon Logic

To add custom behavior to the beacon:

```cpp
void loop() {
  // Add custom logic here
  // Examples:
  // - Read sensor and adjust advertising based on data
  // - Log beacon activity to SD card
  // - Control GPIO pins
  // - Change beacon name dynamically
}
```

## Files

- `BLE_Beacon.ino` - Main beacon application
- `README.md` - This file

## Next Steps

1. ✅ Upload BLE_Beacon.ino to second ESP32
2. ✅ Note the MAC address
3. ✅ Configure Lighthouse Node with MAC address
4. ✅ Verify detection in Serial Monitor
5. 📊 View beacon data in TTN console
6. 🎨 Create dashboard widgets for beacon status

## Related Documentation

- **[LoRaWAN/README.md](../LoRaWAN/README.md)** - Main Lighthouse Node documentation
- **[LoRaWAN/documentation/EXTENSIONS_README.md](../LoRaWAN/documentation/EXTENSIONS_README.md)** - Beacon tracking extension details
- **[QUICK_START.md](../QUICK_START.md)** - Quick start guide for the full system

## Support

For issues or questions:
1. Check the troubleshooting section above
2. Review Serial Monitor output on both devices
3. Verify MAC address configuration
4. Check antenna connections and power supply
5. Review main project documentation

---

**Built for IoT Lighthouse - An advanced crowd detection and tracking system**
