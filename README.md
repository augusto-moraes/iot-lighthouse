# IoT Lighthouse - Advanced Crowd Detection System

An IoT project that measures room occupancy using a Heltec ESP32 V3 with LoRa. The system counts WiFi and BLE devices, tracks specific beacons, and analyzes environment stability, transmitting data to The Things Network (TTN).

## 🎯 Features

### Core Functionality
- **BLE Device Scanning**: Detects nearby Bluetooth devices using active scanning
- **WiFi Network Scanning**: Counts visible WiFi access points (including hidden SSIDs)
- **Unique Device Counting**: Deduplicates devices using MAC addresses
- **Crowd Level Classification**: Threshold-based classification (CALM, MODERATE, CROWDED)
- **LoRaWAN Transmission**: Sends data to TTN every 30 seconds

### Advanced Extensions
- **🎯 BLE Beacon Tracking** (Extension 4): Track a specific device's presence and signal strength
- **📊 Environment Detection** (Extension 5): Classify environment as STATIC or MOBILE based on WiFi stability

## 📋 Quick Start

### Hardware Required
- **Primary**: Heltec ESP32 V3 + LoRa (Lighthouse Node)
- **Secondary** (for beacon tracking): Second ESP32 board (BLE Beacon)
- LoRaWAN gateway with TTN connectivity

### Software Setup

1. **Install Arduino IDE** with Heltec ESP32 board support
   - Install **Heltec ESP32 Dev-Boards** from Board Manager
   - **BLE** and **WiFi** libraries are built into ESP32 core

2. **Clone this repository**
   ```bash
   git clone https://github.com/augusto-moraes/iot-lighthouse.git
   cd iot-lighthouse
   ```

3. **Configure Beacon MAC Address** (if using Extension 4)
   - Upload [`LoRaWAN/BLE_Beacon_Example.ino`](LoRaWAN/BLE_Beacon_Example.ino) to second ESP32
   - Note the MAC address from Serial Monitor
   - Edit [`LoRaWAN/secrets.h`](LoRaWAN/secrets.h) and update:
     ```cpp
     #define BEACON_MAC_ADDRESS "24:6f:28:aa:bb:cc"  // Your beacon's MAC
     ```

4. **Configure LoRaWAN Credentials**
   - Get your credentials from TTN Console
   - Edit [`LoRaWAN/secrets.h`](LoRaWAN/secrets.h) and update:
     ```cpp
     #define LORAWAN_DEV_EUI { 0x70, 0xB3, ... }
     #define LORAWAN_APP_EUI { 0x00, 0x00, ... }
     #define LORAWAN_APP_KEY { 0xCC, 0x4E, ... }
     ```

5. **Board Configuration in Arduino IDE**
   - **Board**: "WiFi LoRa 32(V3)" or "WiFi LoRa 32(V2)" depending on your version
   - **Upload Speed**: 921600
   - **CPU Frequency**: 240MHz
   - **Flash Frequency**: 80MHz
   - **LoRaWAN Region**: Select your region (e.g., EU868, US915)
   - **LoRaWAN Debug Level**: None (for production)

6. **Upload to Lighthouse Node**
   - Open [`LoRaWAN/LoRaWAN.ino`](LoRaWAN/LoRaWAN.ino) in Arduino IDE
   - Select the correct COM port
   - Click Upload
   - Open Serial Monitor (115200 baud) to view output

7. **Configure TTN Payload Formatter**
   - Copy contents of [`LoRaWAN/payload_formatter.js`](LoRaWAN/payload_formatter.js)
   - Paste into TTN Console → Application → Payload Formatters → Uplink

## 📊 Payload Format

The system transmits a **10-byte payload** (or **7-byte** for basic version) every 30 seconds:

### Extended Payload (10 bytes)

| Bytes | Field | Type | Description |
|-------|-------|------|-------------|
| 0-1 | BLE Count | uint16 | Number of BLE devices detected |
| 2-3 | WiFi Count | uint16 | Number of WiFi networks found |
| 4-5 | Total Count | uint16 | Total devices (BLE + WiFi) |
| 6 | Crowd Level | uint8 | 0=CALM, 1=MODERATE, 2=CROWDED |
| 7 | Beacon Detected | uint8 | 0=Not Found, 1=Detected |
| 8 | Beacon RSSI | int8 | Signal strength (-128 to +127 dBm) |
| 9 | Environment | uint8 | 0=STATIC, 1=MOBILE, 2=UNKNOWN |

### Basic Payload (7 bytes)

| Bytes | Field | Type | Description |
|-------|-------|------|-------------|
| 0-1 | BLE Count | uint16 | Number of BLE devices detected |
| 2-3 | WiFi Count | uint16 | Number of WiFi networks found |
| 4-5 | Total Count | uint16 | Total devices (BLE + WiFi) |
| 6 | Crowd Level | uint8 | 0=CALM, 1=MODERATE, 2=CROWDED |

### Decoded Example (Extended)
```json
{
  "ble_count": 15,
  "wifi_count": 8,
  "total_count": 23,
  "crowd_level": 1,
  "crowd_level_text": "MODERATE",
  "beacon_detected": true,
  "beacon_rssi": -65,
  "environment_type": 0,
  "environment_text": "STATIC"
}
```

### Decoding Example (JavaScript/Node-RED)
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
            crowd_level_text: ['CALM', 'MODERATE', 'CROWDED'][bytes[6] || 0],
            beacon_detected: bytes[7] === 1,
            beacon_rssi: bytes[8] || -128,
            environment_type: bytes[9] || 2,
            environment_text: ['STATIC', 'MOBILE', 'UNKNOWN'][bytes[9] || 2]
        }
    };
}
```

## 🎯 Extension 4: BLE Beacon Tracking

Track a specific BLE device through your environment:

### Setup
1. Configure second ESP32 as beacon using [`BLE_Beacon_Example.ino`](LoRaWAN/BLE_Beacon_Example.ino)
2. Update `BEACON_ADDRESS` in [`LoRaWAN/secrets.h`](LoRaWAN/secrets.h)
3. Place beacon in target location
4. Monitor presence and signal strength in dashboard

### Use Cases
- Track device movement between rooms
- Proximity-based triggers
- Asset location monitoring
- Presence detection for specific devices

### RSSI Interpretation
- **-30 to 0 dBm**: Very close (<1m)
- **-50 to -30 dBm**: Close (1-5m)
- **-70 to -50 dBm**: Medium (5-15m)
- **-90 to -70 dBm**: Far (15-30m)
- **< -90 dBm**: Very far (>30m)
- **-128 dBm**: Not found

## 📊 Extension 5: Environment Detection

Automatically classifies the environment as static or mobile:

### How It Works
- Compares WiFi networks between consecutive scans using Jaccard distance
- Calculates change ratio to detect movement
- **STATIC**: <30% change (stable location)
- **MOBILE**: ≥30% change (moving/changing environment)
- **UNKNOWN**: Not enough data yet

### Use Cases
- Detect when monitoring device is moved
- Filter data based on environment stability
- Validate sensor placement
- Adapt crowd detection algorithms

### Configuration
```cpp
#define WIFI_CHANGE_THRESHOLD 0.3    // 30% change threshold
#define MIN_NETWORKS_FOR_ANALYSIS 3  // Minimum networks needed
```

## 🔧 Configuration Options

### Crowd Detection Thresholds
Configure in [`LoRaWAN/LoRaWAN.ino`](LoRaWAN/LoRaWAN.ino):

```cpp
#define CALM_THRESHOLD 5         // 0-5 devices = calm
#define MODERATE_THRESHOLD 15    // 6-15 devices = moderate
                                 // 16+ devices = crowded
```

### Scan Parameters
```cpp
#define BLE_SCAN_TIME 5          // BLE scan duration in seconds
#define WIFI_SCAN_TIME 5         // WiFi scan timeout in seconds
uint32_t appTxDutyCycle = 30000; // Transmission interval (30 seconds)
```

### Beacon Tracking
```cpp
#define BEACON_ADDRESS "00:00:00:00:00:00"  // Target beacon MAC
#define RSSI_NOT_FOUND -128                 // Default RSSI when not found
```

### Environment Detection
```cpp
#define WIFI_CHANGE_THRESHOLD 0.3           // 30% change = mobile
#define MIN_NETWORKS_FOR_ANALYSIS 3         // Minimum WiFi networks
```

## 🧪 Testing & Serial Output

### Example Serial Monitor Output
```
========================================
LoRa Lighthouse Node - Crowd Detection
========================================
Configuration:
  Scan Window: 5 seconds
  Calm Threshold: <= 5 devices
  Moderate Threshold: <= 15 devices
  Crowded Threshold: > 15 devices
  Tracked Beacon: 24:6f:28:aa:bb:cc
  WiFi Change Threshold: 30.0%
  LoRaWAN TX Interval: 30 seconds
========================================

========== Crowd Detection Cycle ==========
Starting BLE scan...
  >> Tracked beacon found! Address: 24:6f:28:aa:bb:cc, RSSI: -65
BLE scan complete. Found 12 unique devices.
  Tracked beacon: DETECTED

Starting WiFi scan...
WiFi scan complete. Found 8 unique networks.
  Environment: STATIC (change ratio: 0.12)
    Common networks: 7, Total unique: 8

--- Detection Summary ---
BLE Devices: 12
WiFi Networks: 8
Total Devices: 20
Crowd Level: CROWDED
Tracked Beacon: PRESENT (RSSI: -65 dBm)
Environment: STATIC
==========================================

LoRaWAN payload prepared:
  BLE: 12
  WiFi: 8
  Total: 20
  Level: 2
```

## 📊 Dashboard Integration

### Data Fields Available
All fields are available for visualization:
- `ble_count`, `wifi_count`, `total_count`
- `crowd_level`, `crowd_level_text`
- `beacon_detected`, `beacon_rssi`
- `environment_type`, `environment_text`

### Recommended Widgets
- **Gauge**: Total device count
- **Indicator**: Crowd level (color-coded)
- **Status**: Beacon presence (green/red)
- **Graph**: RSSI over time
- **Badge**: Environment type
- **Timeline**: Historical data

## 🛠️ Troubleshooting

### No BLE devices detected
- ✅ Ensure BLE devices nearby are advertising
- ✅ Check that BLE initialization succeeded in Serial Monitor
- ✅ Try increasing `BLE_SCAN_TIME`

### No WiFi networks found
- ✅ Verify you're in an area with WiFi coverage
- ✅ Check Serial Monitor for WiFi scan errors
- ✅ Ensure antenna is properly connected

### LoRaWAN not joining
- ✅ Verify your credentials in The Things Network/ChirpStack
- ✅ Check you selected the correct LoRaWAN region
- ✅ Ensure you're in range of a LoRaWAN gateway
- ✅ Check antenna connection
- ✅ Monitor TTN Console for join attempts

### Beacon Not Detected
- ✅ Verify MAC address matches exactly
- ✅ Check beacon is powered and advertising
- ✅ Ensure devices are within range (<30m)
- ✅ Check Serial Monitor for all detected devices

### Environment Always "UNKNOWN"
- ✅ Wait for at least 2 scan cycles (~1 minute)
- ✅ Ensure sufficient WiFi networks detected
- ✅ Lower `MIN_NETWORKS_FOR_ANALYSIS` if needed

## ⚡ Power Consumption Notes

- **BLE Active Scan**: ~80-100mA during scan
- **WiFi Scan**: ~120-150mA during scan
- **LoRa TX**: ~120mA during transmission
- **Deep Sleep**: ~5-10mA (not currently implemented)

### For battery-powered operation, consider:
1. Reducing scan frequency
2. Implementing deep sleep between transmissions
3. Using BLE passive scan instead of active scan
4. Increasing `appTxDutyCycle` (transmission interval)

## 🚀 Performance

- **Memory**: ~1 KB additional RAM for extensions
- **Scan Time**: ~10-15 seconds per cycle
- **Power**: ~100mA during WiFi scan, minimal during sleep
- **Range**: LoRaWAN: up to 10km, BLE: up to 30m

## 📁 Project Structure

```
iot-lighthouse/
├── LoRaWAN/
│   ├── LoRaWAN.ino              # Main application code
│   ├── secrets.h                # LoRaWAN credentials
│   ├── secrets_template.h       # Template for credentials
│   ├── payload_formatter.js     # TTN decoder
│   ├── BLE_Beacon_Example.ino   # Beacon setup code
│   ├── documentation/
│   │   ├── EXTENSIONS_README.md     # Detailed feature documentation
│   │   ├── PAYLOAD_REFERENCE.md     # Payload format reference
│   │   ├── ARCHITECTURE.md          # System architecture diagrams
│   │   └── IMPLEMENTATION_SUMMARY.md # Implementation overview
│   └── README.md                # LoRaWAN-specific documentation
├── QUICK_START.md              # Quick start guide
└── README.md                   # This file
```

## 📖 Documentation

- **[LoRaWAN/documentation/EXTENSIONS_README.md](LoRaWAN/documentation/EXTENSIONS_README.md)** - Complete guide to Extensions 4 & 5
- **[LoRaWAN/documentation/PAYLOAD_REFERENCE.md](LoRaWAN/documentation/PAYLOAD_REFERENCE.md)** - Payload format and decoding
- **[LoRaWAN/documentation/ARCHITECTURE.md](LoRaWAN/documentation/ARCHITECTURE.md)** - System architecture and diagrams
- **[LoRaWAN/documentation/IMPLEMENTATION_SUMMARY.md](LoRaWAN/documentation/IMPLEMENTATION_SUMMARY.md)** - Implementation details
- **[LoRaWAN/README.md](LoRaWAN/README.md)** - Original LoRaWAN documentation
- **[QUICK_START.md](QUICK_START.md)** - Quick start guide

## 💡 Customization Ideas

1. **Adjust thresholds** based on your environment
2. **Add temporal filtering** to smooth out fluctuations
3. **Implement device tracking** across multiple scans
4. **Add battery monitoring** to the payload
5. **Include RSSI values** for distance estimation
6. **Log data to SD card** for offline analysis
7. **Multiple beacon tracking** for complex scenarios
8. **Advanced movement detection** algorithms
9. **Machine learning integration** for pattern recognition

## 🔒 Privacy Considerations

This system only collects:
- MAC address counts (anonymized)
- Network SSIDs (public information)
- No personal data or tracking

**Important**: Ensure compliance with local privacy regulations (GDPR, etc.) before deployment.

## 📝 License

Based on Heltec Automation example code.
This project is part of the IoT Lighthouse system developed for crowd detection and environmental monitoring.

## 🤝 Contributing

Contributions are welcome! Areas for enhancement:
- Multiple beacon tracking
- Advanced movement detection
- Machine learning integration
- Additional sensor support
- Dashboard templates
- Power optimization
- Deep sleep implementation

## 📧 Support

For detailed documentation, see the files in the [`LoRaWAN/documentation/`](LoRaWAN/documentation/) directory.

---

**Built with ❤️ using Heltec ESP32 V3, LoRaWAN, and The Things Network**