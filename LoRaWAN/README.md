# LoRa Lighthouse Node - Crowd Detection System

## Overview
This firmware implements a crowd detection system for the Heltec LoRa 32 ESP32 board. It uses passive BLE and Wi-Fi scanning to estimate crowd density and transmits the data via LoRaWAN.

## Features

### 1. **BLE Device Scanning**
- Performs active BLE scanning for 5 seconds per cycle
- Tracks unique devices using MAC addresses
- Uses `std::set` to automatically deduplicate devices
- Clears results after each scan to manage memory

### 2. **Wi-Fi Network Scanning**
- Scans for Wi-Fi networks including hidden SSIDs
- Counts unique networks using BSSID (MAC address)
- Automatically cleans up scan results

### 3. **Unique Device Counting**
- Combines BLE and Wi-Fi counts for total device estimate
- Each device is counted only once per scan window
- Resets counters for each transmission cycle

### 4. **Crowd Level Classification**
Simple threshold-based model:
- **CALM**: 0-5 devices detected
- **MODERATE**: 6-15 devices detected  
- **CROWDED**: 16+ devices detected

You can adjust these thresholds by modifying:
```cpp
#define CALM_THRESHOLD 5
#define MODERATE_THRESHOLD 15
```

## LoRaWAN Payload Format

The firmware sends 7 bytes of data:

| Byte | Description | Type |
|------|-------------|------|
| 0-1 | BLE device count | uint16_t (big-endian) |
| 2-3 | Wi-Fi network count | uint16_t (big-endian) |
| 4-5 | Total device count | uint16_t (big-endian) |
| 6 | Crowd level | uint8_t (0=CALM, 1=MODERATE, 2=CROWDED) |

### Decoding Example (JavaScript/Node-RED)
```javascript
function Decoder(bytes, port) {
  var decoded = {};
  
  // BLE device count (bytes 0-1)
  decoded.ble_count = (bytes[0] << 8) | bytes[1];
  
  // WiFi network count (bytes 2-3)
  decoded.wifi_count = (bytes[2] << 8) | bytes[3];
  
  // Total device count (bytes 4-5)
  decoded.total_count = (bytes[4] << 8) | bytes[5];
  
  // Crowd level (byte 6)
  decoded.crowd_level = bytes[6];
  decoded.crowd_level_text = ["CALM", "MODERATE", "CROWDED"][bytes[6]];
  
  return decoded;
}
```

### Decoding Example (Python/TTN)
```python
```javascript
function decodeUplink(input) {
    const bytes = input.bytes || [];
    const u16 = (hi, lo) => (((hi || 0) << 8) | (lo || 0)) >>> 0;

    return {
        data: {
            ble_count: u16(bytes[0], bytes[1]),
            wifi_count: u16(bytes[2], bytes[3]),
            total_count: u16(bytes[4], bytes[5]),
            crowd_level: bytes[6] || 0,
            crowd_level_text: ['CALM', 'MODERATE', 'CROWDED'][bytes[6] || 0]
        }
    };
}
```

## Configuration

### Scan Parameters
```cpp
#define BLE_SCAN_TIME 5      // BLE scan duration (seconds)
#define WIFI_SCAN_TIME 5     // WiFi scan timeout (seconds)
```

### LoRaWAN Settings
```cpp
uint32_t appTxDutyCycle = 30000;  // Transmission interval (15 seconds)
```

### Your LoRaWAN Credentials
The code includes your existing credentials:
- **OTAA Mode**: Enabled (recommended)
- **Device EUI**: 70:B3:D5:7E:D0:07:43:59
- **App Key**: CC:4E:EA:15:71:B5:82:99:35:3C:D1:5D:12:6E:DA:5D

## Arduino IDE Setup

### Required Libraries
1. **Heltec ESP32 Dev-Boards** - Install from Board Manager
2. **BLE** - Built into ESP32 core
3. **WiFi** - Built into ESP32 core

### Board Configuration
1. **Board**: "WiFi LoRa 32(V2)" or "WiFi LoRa 32(V3)" depending on your version
2. **Upload Speed**: 921600
3. **CPU Frequency**: 240MHz
4. **Flash Frequency**: 80MHz
5. **LoRaWAN Region**: Select your region (e.g., EU868, US915)
6. **LoRaWAN Debug Level**: None (for production)

## Upload and Monitor

1. Connect your Heltec LoRa 32 board via USB
2. Select the correct COM port in Arduino IDE
3. Click Upload
4. Open Serial Monitor (115200 baud) to see output

## Serial Output Example

```
========================================
LoRa Lighthouse Node - Crowd Detection
========================================
Configuration:
  Scan Window: 5 seconds
  Calm Threshold: <= 5 devices
  Moderate Threshold: <= 15 devices
  Crowded Threshold: > 15 devices
  LoRaWAN TX Interval: 15 seconds
========================================

========== Crowd Detection Cycle ==========
Starting BLE scan...
BLE scan complete. Found 3 unique devices.
Starting WiFi scan...
WiFi scan complete. Found 8 unique networks.

--- Detection Summary ---
BLE Devices: 3
WiFi Networks: 8
Total Devices: 11
Crowd Level: MODERATE
==========================================

LoRaWAN payload prepared:
  BLE: 3
  WiFi: 8
  Total: 11
  Level: 1
```

## Power Consumption Notes

- **BLE Active Scan**: ~80-100mA during scan
- **WiFi Scan**: ~120-150mA during scan
- **LoRa TX**: ~120mA during transmission
- **Deep Sleep**: ~5-10mA (not currently implemented)

For battery-powered operation, consider:
1. Reducing scan frequency
2. Implementing deep sleep between transmissions
3. Using BLE passive scan instead of active scan

## Troubleshooting

### No BLE devices detected
- Ensure BLE devices nearby are advertising
- Check that BLE initialization succeeded in Serial Monitor
- Try increasing `BLE_SCAN_TIME`

### No WiFi networks found
- Verify you're in an area with WiFi coverage
- Check Serial Monitor for WiFi scan errors
- Ensure antenna is properly connected

### LoRaWAN not joining
- Verify your credentials in The Things Network/ChirpStack
- Check you selected the correct LoRaWAN region
- Ensure you're in range of a LoRaWAN gateway
- Check antenna connection

## Customization Ideas

1. **Adjust thresholds** based on your environment
2. **Add temporal filtering** to smooth out fluctuations
3. **Implement device tracking** across multiple scans
4. **Add battery monitoring** to the payload
5. **Include RSSI values** for distance estimation
6. **Log data to SD card** for offline analysis

## Privacy Considerations

This system only collects:
- MAC address counts (anonymized)
- Network SSIDs (public information)
- No personal data or tracking

Ensure compliance with local privacy regulations (GDPR, etc.) before deployment.

## License

Based on Heltec Automation example code.
