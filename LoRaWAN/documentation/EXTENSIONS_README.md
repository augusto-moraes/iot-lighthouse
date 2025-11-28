# LoRa Lighthouse Advanced Extensions

This document describes the two advanced extensions added to the LoRa Lighthouse Node project.

## Extension 4: Device Tracking Using BLE

### Overview
The Lighthouse Node now tracks a specific BLE beacon device in addition to performing general crowd detection. This allows you to monitor the presence and location (via signal strength) of a specific device.

### How It Works
1. **Beacon Configuration**: A second LoRa V3 ESP Development board is configured as a BLE beacon
2. **Detection**: During each BLE scan, the system looks for the beacon's unique MAC address
3. **Recording**: The system records:
   - **Presence**: Whether the beacon is detected (true/false)
   - **Signal Strength (RSSI)**: The received signal strength in dBm (or -128 if not found)
4. **Transmission**: This data is included in the LoRaWAN uplink payload

### Configuration

In `LoRaWAN.ino`, update the beacon MAC address:

```cpp
#define BEACON_ADDRESS "24:6f:28:aa:bb:cc"  // Replace with your beacon's MAC
```

To find your beacon's MAC address, you can:
- Check the Serial Monitor output during scanning
- Use a BLE scanner app on your phone
- Look for the device address in the BLE logs

### Beacon Setup (Second ESP32 Board)

To configure another ESP32 board as a BLE beacon, use this simple code:

```cpp
#include <BLEDevice.h>
#include <BLEServer.h>

void setup() {
  Serial.begin(115200);
  
  // Initialize BLE with a unique name
  BLEDevice::init("Tracked_Beacon");
  
  // Start advertising
  BLEServer *pServer = BLEDevice::createServer();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();
  
  Serial.println("BLE Beacon started!");
  Serial.print("MAC Address: ");
  Serial.println(BLEDevice::getAddress().toString().c_str());
}

void loop() {
  delay(2000);
}
```

### Payload Data

The beacon tracking data is transmitted as:
- **Byte 7**: Beacon detected (0 = not found, 1 = detected)
- **Byte 8**: Beacon RSSI (signed int8, -128 to +127 dBm)

### Dashboard Integration

The TTN payload formatter now includes:
```javascript
{
  beacon_detected: true/false,
  beacon_rssi: -65  // Signal strength in dBm
}
```

You can use this data to:
- Display beacon presence status on your dashboard
- Show signal strength as a proximity indicator
- Track device movement between rooms
- Trigger alerts when beacon enters/leaves range

### RSSI Interpretation

| RSSI Range | Distance | Interpretation |
|------------|----------|----------------|
| -30 to 0   | Very Close | < 1 meter |
| -50 to -30 | Close | 1-5 meters |
| -70 to -50 | Medium | 5-15 meters |
| -90 to -70 | Far | 15-30 meters |
| < -90      | Very Far | > 30 meters |
| -128       | Not Found | Out of range |

---

## Extension 5: Static vs Mobile Environment Detection

### Overview
The system analyzes WiFi scan results over time to determine if the environment is "static" (stable, fixed location) or "mobile" (changing, moving environment).

### How It Works

1. **WiFi Network Comparison**: The system compares WiFi access points between consecutive scans
2. **Change Analysis**: Uses the Jaccard distance metric to calculate network similarity:
   - Calculates networks that appear in both scans (intersection)
   - Calculates all unique networks across both scans (union)
   - Change ratio = 1 - (intersection / union)
3. **Classification**:
   - **STATIC**: Change ratio < 30% (stable environment)
   - **MOBILE**: Change ratio ≥ 30% (changing environment)
   - **UNKNOWN**: Not enough data (first scan or too few networks)

### Configuration

In `LoRaWAN.ino`:

```cpp
#define WIFI_CHANGE_THRESHOLD 0.3    // 30% change threshold
#define MIN_NETWORKS_FOR_ANALYSIS 3  // Minimum networks needed
```

You can adjust these values:
- **Lower threshold** (e.g., 0.2): More sensitive, detects smaller changes
- **Higher threshold** (e.g., 0.5): Less sensitive, only detects major changes
- **Min networks**: Ensures reliable analysis (recommended: 3-5)

### Use Cases

#### Static Environment
- **Office building**: Same access points visible
- **Home**: Stable network environment
- **Fixed monitoring location**: Consistent network scan results

#### Mobile Environment
- **Moving vehicle**: WiFi networks change as you move
- **Walking around**: Different APs visible in different locations
- **Crowded events**: Network topology changes with crowd movement

### Payload Data

Environment classification is transmitted as:
- **Byte 9**: Environment type (0 = STATIC, 1 = MOBILE, 2 = UNKNOWN)

### Dashboard Integration

The TTN payload formatter includes:
```javascript
{
  environment_type: 0,      // 0, 1, or 2
  environment_text: "STATIC" // "STATIC", "MOBILE", or "UNKNOWN"
}
```

You can use this to:
- Display environment stability status
- Filter or categorize crowd data based on environment
- Detect when the monitoring device is moved
- Adjust crowd detection algorithms based on environment type

### Algorithm Details

**Jaccard Similarity Index**:
```
Similarity = |Networks_Current ∩ Networks_Previous| / |Networks_Current ∪ Networks_Previous|
Change Ratio = 1 - Similarity
```

**Example**:
- Scan 1: Networks {A, B, C, D, E} (5 networks)
- Scan 2: Networks {B, C, D, F, G} (5 networks)
- Intersection: {B, C, D} (3 common)
- Union: {A, B, C, D, E, F, G} (7 unique)
- Similarity: 3/7 = 0.43
- Change Ratio: 1 - 0.43 = 0.57 (57% change)
- Classification: MOBILE (> 30% threshold)

---

## Complete Payload Format

The extended payload is now 10 bytes:

| Byte | Description | Type | Values |
|------|-------------|------|---------|
| 0-1  | BLE device count | uint16 | 0-65535 |
| 2-3  | WiFi network count | uint16 | 0-65535 |
| 4-5  | Total device count | uint16 | 0-65535 |
| 6    | Crowd level | uint8 | 0=CALM, 1=MODERATE, 2=CROWDED |
| 7    | Beacon detected | uint8 | 0=false, 1=true |
| 8    | Beacon RSSI | int8 | -128 to +127 dBm |
| 9    | Environment type | uint8 | 0=STATIC, 1=MOBILE, 2=UNKNOWN |

---

## Testing the Extensions

### Testing BLE Beacon Tracking

1. Configure a second ESP32 as a BLE beacon (see code above)
2. Note the beacon's MAC address from Serial Monitor
3. Update `BEACON_ADDRESS` in the Lighthouse Node code
4. Upload code to both devices
5. Place the beacon in different locations/distances
6. Monitor Serial output to see detection and RSSI values
7. Verify data appears in TTN dashboard

**Expected Serial Output**:
```
Starting BLE scan...
  >> Tracked beacon found! Address: 24:6f:28:aa:bb:cc, RSSI: -65
BLE scan complete. Found 12 unique devices.
  Tracked beacon: DETECTED

--- Detection Summary ---
Tracked Beacon: PRESENT (RSSI: -65 dBm)
```

### Testing Environment Detection

1. Upload the code and let it run for at least 2 scan cycles
2. For **static** test: Keep the device in one location
3. For **mobile** test: Move the device to different locations between scans
4. Monitor Serial output for environment classification

**Expected Serial Output (Static)**:
```
WiFi scan complete. Found 8 unique networks.
  Environment: STATIC (change ratio: 0.12)
    Common networks: 7, Total unique: 8
```

**Expected Serial Output (Mobile)**:
```
WiFi scan complete. Found 6 unique networks.
  Environment: MOBILE (change ratio: 0.45)
    Common networks: 5, Total unique: 9
```

---

## TTN Dashboard Visualization Ideas

### For Beacon Tracking
- **Status Indicator**: Green when beacon detected, red when not found
- **Signal Strength Gauge**: Visual representation of RSSI (-90 to -30 dBm)
- **Proximity Chart**: Historical graph of RSSI over time
- **Alert System**: Notifications when beacon enters/leaves area

### For Environment Detection
- **Environment Badge**: "STATIC" or "MOBILE" indicator
- **Change Trend**: Graph showing network change ratio over time
- **Data Validation**: Filter crowd data when environment is MOBILE
- **Location Context**: Use environment type to validate sensor placement

---

## Troubleshooting

### Beacon Not Detected
- Verify beacon MAC address matches `BEACON_ADDRESS`
- Check that beacon is powered on and advertising
- Ensure beacon is within BLE range (usually <30m)
- Try active scanning: Already enabled by default
- Check Serial Monitor for all detected devices

### Environment Always "UNKNOWN"
- Wait for at least 2 scan cycles (minimum 1 minute)
- Ensure WiFi scan finds at least `MIN_NETWORKS_FOR_ANALYSIS` networks
- Check that WiFi scanning is working (see network count in Serial)
- Lower `MIN_NETWORKS_FOR_ANALYSIS` if in a low-WiFi area

### RSSI Values Seem Wrong
- RSSI is always negative (0 to -128)
- More negative = weaker signal = farther away
- RSSI of -128 means beacon not found (default value)
- RSSI can fluctuate due to interference, obstacles, etc.

---

## Performance Considerations

### Memory Usage
- Each WiFi network stored uses ~50 bytes (BSSID + metadata)
- Storing previous scan adds minimal overhead (<1KB for 20 networks)
- BLE tracking adds negligible memory (~10 bytes)

### Power Consumption
- BLE active scanning: Slightly higher power than passive
- WiFi scanning: Main power consumer (~100mA during scan)
- Recommend deep sleep between scans for battery applications

### Scan Timing
- BLE scan: 5 seconds (configurable via `BLE_SCAN_TIME`)
- WiFi scan: ~5 seconds (automatic)
- Total cycle time: ~10-15 seconds
- LoRaWAN transmission: 30 seconds (configurable via `appTxDutyCycle`)

---

## Future Enhancements

Possible additions to these extensions:
1. **Multiple Beacon Tracking**: Track 2-3 beacons simultaneously
2. **RSSI History**: Store last N RSSI values for trending
3. **Smart Environment**: Adjust crowd thresholds based on environment type
4. **Geo-fencing**: Trigger alerts based on beacon presence
5. **Advanced Movement Detection**: Use RSSI gradient to detect approach/departure
6. **WiFi Network Fingerprinting**: Use network signatures for location detection

---

## References

- ESP32 BLE Documentation: https://github.com/nkolban/ESP32_BLE_Arduino
- LoRaWAN Specification: https://lora-alliance.org/
- Jaccard Index: https://en.wikipedia.org/wiki/Jaccard_index
- RSSI and Distance: https://www.bluetooth.com/blog/proximity-and-rssi/
