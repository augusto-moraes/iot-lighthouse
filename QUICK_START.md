# Quick Start Guide - Advanced Extensions

## 🎯 What Was Added

Two advanced extensions to your LoRa Lighthouse Node:

1. **BLE Beacon Tracking** - Track a specific device's location via signal strength
2. **Environment Detection** - Detect if you're in a static or mobile environment

## ⚡ Quick Setup (5 Minutes)

### Step 1: Configure the Beacon MAC Address

**Option A - If you have a second ESP32:**
1. See [`BLE_Beacon/README.md`](BLE_Beacon/README.md) for detailed setup
2. Upload `BLE_Beacon/BLE_Beacon.ino` to the second ESP32
3. Open Serial Monitor - it will show the MAC address
4. Open `LoRaWAN/secrets.h` and update:
   ```cpp
   #define BEACON_MAC_ADDRESS "24:6f:28:aa:bb:cc"  // Your beacon's MAC
   ```

**Option B - No second ESP32?**
- Leave `BEACON_MAC_ADDRESS` as the default `"00:00:00:00:00:00"`
- The system will work fine - beacon will just show as "not detected" (expected behavior)

### Step 2: Configure LoRaWAN Credentials

In the same `LoRaWAN/secrets.h` file, update your TTN credentials:

```cpp
#define LORAWAN_DEV_EUI { 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x07, 0x43, 0x59 }
#define LORAWAN_APP_EUI { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define LORAWAN_APP_KEY { 0xCC, 0x4E, 0xEA, 0x15, 0x71, 0xB5, 0x82, 0x99, 0x35, 0x3C, 0xD1, 0x5D, 0x12, 0x6E, 0xDA, 0x5D }
```

Get these from TTN Console → Applications → Your App → End Devices

See [CONFIGURATION.md](CONFIGURATION.md) for detailed conversion instructions.

### Step 3: Upload to Your Lighthouse Node

1. Open `LoRaWAN/LoRaWAN.ino` in Arduino IDE
2. Select your board (Heltec WiFi LoRa 32 V3)
3. Click Upload
4. Done! ✅

> **Note:** Your credentials are loaded from `secrets.h` automatically

### Step 4: Update TTN Payload Formatter

1. Go to TTN Console → Your Application → Payload Formatters
2. Select "Uplink" 
3. Copy all contents from `LoRaWAN/payload_formatter.js`
3. Paste into TTN
4. Save! ✅

### Step 5: Test It!

Open Serial Monitor (115200 baud) - you should see:

```
========================================
LoRa Lighthouse Node - Crowd Detection
========================================
Configuration:
  Tracked Beacon: 24:6f:28:aa:bb:cc
  WiFi Change Threshold: 30.0%
  ...

Starting BLE scan...
BLE scan complete. Found 12 unique devices.
  Tracked beacon: DETECTED (or NOT FOUND)

Starting WiFi scan...
WiFi scan complete. Found 8 unique networks.
  Environment: STATIC (or MOBILE or UNKNOWN)

--- Detection Summary ---
BLE Devices: 12
WiFi Networks: 8
Total Devices: 20
Crowd Level: CROWDED
Tracked Beacon: PRESENT (RSSI: -65 dBm)
Environment: STATIC
==========================================
```

## 📊 What You'll See in TTN

Your uplink data will now include:

```json
{
  "ble_count": 12,
  "wifi_count": 8,
  "total_count": 20,
  "crowd_level": 2,
  "crowd_level_text": "CROWDED",
  "beacon_detected": true,        ← NEW!
  "beacon_rssi": -65,              ← NEW!
  "environment_type": 0,           ← NEW!
  "environment_text": "STATIC"     ← NEW!
}
```

## 🎨 Dashboard Ideas

### New Widgets You Can Add:

**Beacon Status Indicator**
- Green light when beacon detected
- Red when not found
- Shows "Beacon Present" or "Beacon Not Found"

**Signal Strength Gauge**
- Range: -90 dBm (far) to -30 dBm (close)
- Color-coded: Red (weak) → Yellow → Green (strong)

**Environment Badge**
- Shows "STATIC" or "MOBILE"
- Helps validate sensor placement

**Beacon Proximity Chart**
- Graph RSSI over time
- Watch device movement patterns

## 🔍 Understanding the Values

### Beacon RSSI (Signal Strength)
- **-30 to -50**: Very close (same room, near device)
- **-50 to -70**: Medium distance (same room, far corner)
- **-70 to -90**: Far (different room nearby)
- **-128**: Not detected (too far or beacon off)

### Environment Type
- **STATIC**: WiFi networks stay the same → Fixed location ✅
- **MOBILE**: WiFi networks changing → Moving around 🚶
- **UNKNOWN**: Not enough scans yet (wait 1-2 minutes)

## ⚙️ Optional Adjustments

### Make Environment Detection More/Less Sensitive

In `LoRaWAN.ino` around line ~95:

```cpp
#define WIFI_CHANGE_THRESHOLD 0.3    // Default: 30% change
```

- **More sensitive** (detect smaller changes): `0.2` (20%)
- **Less sensitive** (only big changes): `0.5` (50%)

### Change Crowd Thresholds

Around line ~85:

```cpp
#define CALM_THRESHOLD 5         // 0-5 devices = calm
#define MODERATE_THRESHOLD 15    // 6-15 = moderate, 16+ = crowded
```

Adjust these based on your environment!

## 🐛 Troubleshooting

### "Beacon always shows NOT FOUND"
- Check MAC address matches exactly (case-sensitive!)
- Make sure beacon ESP32 is powered on
- Move beacon closer (<10 meters for testing)
- Check Serial Monitor - do you see the beacon in the scan results?

### "Environment always shows UNKNOWN"  
- **Normal!** First scan always shows UNKNOWN
- Wait 30-60 seconds for second scan
- If still UNKNOWN: Check that WiFi scan finds networks (see Serial Monitor)

### "No data in TTN"
- Check LoRaWAN credentials (devEui, appEui, appKey)
- Verify gateway is online
- Watch Serial Monitor for "JOIN" messages

## 📚 Need More Details?

Check these files for comprehensive documentation:

- **`EXTENSIONS_README.md`** - Full feature documentation
- **`PAYLOAD_REFERENCE.md`** - Payload format details
- **`ARCHITECTURE.md`** - System diagrams and flow
- **`IMPLEMENTATION_SUMMARY.md`** - Technical implementation

## ✅ Success Checklist

- [ ] Code uploaded to Lighthouse Node
- [ ] Beacon MAC address configured (or using default)
- [ ] TTN payload formatter updated
- [ ] Serial Monitor shows scan results
- [ ] Data appears in TTN console
- [ ] New fields visible in uplink data
- [ ] Environment detection working (after 2 scans)
- [ ] Beacon tracking working (if configured)

## 🎉 You're Done!

Your Lighthouse Node now has:
- ✅ Original crowd detection (BLE + WiFi counting)
- ✅ Beacon tracking with signal strength
- ✅ Automatic environment detection
- ✅ Extended 10-byte payload
- ✅ Ready for advanced dashboards

**Next Steps:**
1. Let it run for a few cycles and watch the Serial Monitor
2. Verify data appears in TTN with new fields
3. Create dashboard widgets for beacon and environment
4. Experiment with moving the device or beacon around
5. Enjoy your enhanced IoT monitoring system! 🚀

---

**Need help?** Check the detailed docs or the implementation summary!
