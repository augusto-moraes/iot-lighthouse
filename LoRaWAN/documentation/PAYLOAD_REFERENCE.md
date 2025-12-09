# LoRaWAN Payload Reference

## Payload Structure (10 bytes)

```
Offset | Bytes | Field                | Type   | Range/Values
-------|-------|----------------------|--------|---------------------------
0-1    | 2     | BLE Device Count     | uint16 | 0 - 65535
2-3    | 2     | WiFi Network Count   | uint16 | 0 - 65535  
4-5    | 2     | Total Device Count   | uint16 | 0 - 65535
6      | 1     | Crowd Level          | uint8  | 0=CALM, 1=MODERATE, 2=CROWDED
7      | 1     | Beacon Detected      | uint8  | 0=Not Found, 1=Detected
8      | 1     | Beacon RSSI          | int8   | -128 to +127 dBm
9      | 1     | Environment Type     | uint8  | 0=STATIC, 1=MOBILE, 2=UNKNOWN
```

## Decoding Example

### Raw Payload (Hex)
```
00 0F 00 08 00 17 01 01 BF 00
```

### Decoded Values

| Field | Hex | Decimal | Meaning |
|-------|-----|---------|---------|
| BLE Count | `00 0F` | 15 | 15 BLE devices detected |
| WiFi Count | `00 08` | 8 | 8 WiFi networks found |
| Total Count | `00 17` | 23 | 23 total devices |
| Crowd Level | `01` | 1 | MODERATE crowd |
| Beacon Detected | `01` | 1 | Beacon is present |
| Beacon RSSI | `BF` | -65 | Signal strength: -65 dBm |
| Environment | `00` | 0 | STATIC environment |

### JavaScript Decoder (TTN)

```javascript
function decodeUplink(input) {
    const bytes = input.bytes || [];
    const u16 = (hi, lo) => (((hi || 0) << 8) | (lo || 0)) >>> 0;
    const toInt8 = (byte) => {
        const val = byte || 0;
        return val > 127 ? val - 256 : val;
    };

    return {
        data: {
            ble_count: u16(bytes[0], bytes[1]),
            wifi_count: u16(bytes[2], bytes[3]),
            total_count: u16(bytes[4], bytes[5]),
            crowd_level: bytes[6] || 0,
            crowd_level_text: ['CALM', 'MODERATE', 'CROWDED'][bytes[6] || 0],
            beacon_detected: (bytes[7] || 0) === 1,
            beacon_rssi: toInt8(bytes[8]),
            environment_type: bytes[9] || 2,
            environment_text: ['STATIC', 'MOBILE', 'UNKNOWN'][bytes[9] || 2],
            // Note: environment is UNKNOWN only on the first scan; thereafter it's STATIC or MOBILE
        }
    };
}
```

## Sample Decoded Payloads

### Example 1: Calm, Static Environment, No Beacon
```json
{
  "ble_count": 3,
  "wifi_count": 2,
  "total_count": 5,
  "crowd_level": 0,
  "crowd_level_text": "CALM",
  "beacon_detected": false,
  "beacon_rssi": -128,
  "environment_type": 0,
  "environment_text": "STATIC"
}
```

### Example 2: Crowded, Mobile Environment, Beacon Present
```json
{
  "ble_count": 12,
  "wifi_count": 8,
  "total_count": 20,
  "crowd_level": 2,
  "crowd_level_text": "CROWDED",
  "beacon_detected": true,
  "beacon_rssi": -52,
  "environment_type": 1,
  "environment_text": "MOBILE"
}
```

### Example 3: Moderate Crowd, First Scan
```json
{
  "ble_count": 7,
  "wifi_count": 4,
  "total_count": 11,
  "crowd_level": 1,
  "crowd_level_text": "MODERATE",
  "beacon_detected": true,
  "beacon_rssi": -78,
  "environment_type": 2,
  "environment_text": "UNKNOWN"
}
```

## Interpreting Values

### Crowd Levels
- **CALM (0)**: 0-5 devices - Low activity
- **MODERATE (1)**: 6-15 devices - Normal activity  
- **CROWDED (2)**: 16+ devices - High activity

### RSSI Signal Strength
- **-30 to 0 dBm**: Very close (<1m) - Excellent signal
- **-50 to -30 dBm**: Close (1-5m) - Good signal
- **-70 to -50 dBm**: Medium (5-15m) - Fair signal
- **-90 to -70 dBm**: Far (15-30m) - Weak signal
- **< -90 dBm**: Very far (>30m) - Very weak
- **-128 dBm**: Not found / Out of range

### Environment Types
- **STATIC (0)**: Stable WiFi networks, fixed location
- **MOBILE (1)**: Changing WiFi networks, moving environment
- **UNKNOWN (2)**: Not enough data (first scan or insufficient networks)

## Integration Examples

### Node-RED Dashboard
```javascript
// Color-code based on crowd level
var color;
switch(msg.payload.crowd_level) {
    case 0: color = "green"; break;   // CALM
    case 1: color = "yellow"; break;  // MODERATE
    case 2: color = "red"; break;     // CROWDED
}
msg.color = color;

// Add beacon status
if (msg.payload.beacon_detected) {
    msg.beacon_status = "Present (RSSI: " + msg.payload.beacon_rssi + " dBm)";
} else {
    msg.beacon_status = "Not Found";
}

return msg;
```

### InfluxDB Line Protocol
```
crowd_detection,device=lighthouse01 ble_count=15,wifi_count=8,total_count=23,crowd_level=1,beacon_rssi=-65,environment_type=0
```

### Grafana Query Examples
```sql
-- Average crowd level over time
SELECT mean("crowd_level") FROM "crowd_detection" 
WHERE time > now() - 1h GROUP BY time(5m)

-- Beacon presence tracking
SELECT "beacon_detected", "beacon_rssi" FROM "crowd_detection"
WHERE time > now() - 24h

-- Environment stability
SELECT "environment_text" FROM "crowd_detection"
WHERE time > now() - 6h
```

## Testing Payloads

You can test the decoder with these sample hex payloads:

```
Example 1 (Calm + Static):
00 03 00 02 00 05 00 00 80 00

Example 2 (Crowded + Beacon):
00 0C 00 08 00 14 02 01 CC 01

Example 3 (Moderate + Unknown):
00 07 00 04 00 0B 01 01 B2 02
```

## Troubleshooting

### Decoder Issues
- Ensure TTN payload formatter is configured correctly
- Check byte order (big-endian for uint16)
- Verify RSSI signed conversion (-128 to +127)
- Confirm payload length is exactly 10 bytes

### Unexpected Values
- **Total != BLE + WiFi**: Check for counting logic errors
- **RSSI always -128**: Beacon not detected or wrong MAC address
- **Environment always UNKNOWN**: Wait for second scan cycle
- **Crowd level doesn't match count**: Check threshold configuration
