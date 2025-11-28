# System Architecture Diagram

## Overall System Flow

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        LoRa Lighthouse Node                             │
│                         (ESP32 + LoRa)                                  │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                ┌───────────────────┼───────────────────┐
                │                   │                   │
                ▼                   ▼                   ▼
    ┌───────────────────┐  ┌───────────────┐  ┌────────────────┐
    │   BLE Scanning    │  │ WiFi Scanning │  │ Beacon Tracking│
    │                   │  │               │  │                │
    │ • Passive scan    │  │ • Active scan │  │ • Look for MAC │
    │ • Count devices   │  │ • Count APs   │  │ • Measure RSSI │
    │ • 5 second window │  │ • Get BSSIDs  │  │ • Report status│
    └───────────────────┘  └───────────────┘  └────────────────┘
                │                   │                   │
                └───────────────────┼───────────────────┘
                                    ▼
                        ┌───────────────────────┐
                        │  Environment Analysis │
                        │                       │
                        │ • Compare WiFi scans  │
                        │ • Calculate changes   │
                        │ • Classify STATIC/    │
                        │   MOBILE/UNKNOWN      │
                        └───────────────────────┘
                                    │
                                    ▼
                        ┌───────────────────────┐
                        │   Crowd Detection     │
                        │                       │
                        │ • Total = BLE + WiFi  │
                        │ • Apply thresholds    │
                        │ • Classify level      │
                        └───────────────────────┘
                                    │
                                    ▼
                        ┌───────────────────────┐
                        │  Prepare LoRaWAN      │
                        │  Payload (10 bytes)   │
                        └───────────────────────┘
                                    │
                                    ▼
                        ┌───────────────────────┐
                        │   LoRaWAN Transmit    │
                        │   to TTN Gateway      │
                        └───────────────────────┘
                                    │
                                    ▼
                        ┌───────────────────────┐
                        │  The Things Network   │
                        │  (TTN)                │
                        └───────────────────────┘
                                    │
                                    ▼
                        ┌───────────────────────┐
                        │  Payload Formatter    │
                        │  (JavaScript)         │
                        └───────────────────────┘
                                    │
                                    ▼
                        ┌───────────────────────┐
                        │  Dashboard / Database │
                        │  • Grafana            │
                        │  • Node-RED           │
                        │  • InfluxDB           │
                        └───────────────────────┘
```

## Data Flow Details

### 1. BLE Scanning
```
[Start Scan] → [Detect Devices] → [Check for Beacon] → [Count Unique]
                      │                    │
                      ▼                    ▼
              [Store in Set]      [Record RSSI if found]
```

### 2. WiFi Scanning
```
[Start Scan] → [Get Networks] → [Extract BSSIDs] → [Count Unique]
                                        │
                                        ▼
                              [Store Current Scan]
                                        │
                                        ▼
                              [Compare with Previous]
                                        │
                                        ▼
                              [Calculate Change Ratio]
```

### 3. Environment Analysis Algorithm
```
Current Scan: {A, B, C, D, E}
Previous Scan: {B, C, D, F, G}

Intersection: {B, C, D}        (common networks)
Union: {A, B, C, D, E, F, G}  (all unique networks)

Similarity = |Intersection| / |Union| = 3 / 7 = 0.43
Change Ratio = 1 - Similarity = 0.57 (57% change)

If Change Ratio ≥ 0.30 → MOBILE
If Change Ratio < 0.30 → STATIC
If Not Enough Data → UNKNOWN
```

### 4. Payload Construction
```
┌─────┬─────┬─────┬─────┬─────┬─────┬──────┬─────┬─────┬─────┐
│ 0-1 │ 2-3 │ 4-5 │  6  │  7  │  8  │   9  │     │     │     │
├─────┼─────┼─────┼─────┼─────┼─────┼──────┼─────┼─────┼─────┤
│ BLE │WiFi │Total│Crowd│Bcn  │RSSI │Env   │     │     │     │
│Count│Count│Count│Level│Det  │     │Type  │     │     │     │
└─────┴─────┴─────┴─────┴─────┴─────┴──────┴─────┴─────┴─────┘
  u16   u16   u16   u8    u8    i8     u8

Example:
00 0F  00 08  00 17  01    01    BF     00
  15     8      23   MOD   YES   -65   STATIC
```

## Component Interactions

### Primary Lighthouse Node
```
┌────────────────────────────────────────┐
│  ESP32 + LoRa                          │
│                                        │
│  ┌──────────────┐   ┌──────────────┐  │
│  │  BLE Radio   │   │  WiFi Radio  │  │
│  │              │   │              │  │
│  │ • Scan for   │   │ • Scan for   │  │
│  │   devices    │   │   APs        │  │
│  │ • Track      │   │ • Compare    │  │
│  │   beacon     │   │   changes    │  │
│  └──────────────┘   └──────────────┘  │
│          │                  │          │
│          └────────┬─────────┘          │
│                   ▼                    │
│         ┌──────────────────┐           │
│         │  LoRa Transceiver│           │
│         │                  │           │
│         │  • Encode data   │           │
│         │  • Transmit      │           │
│         │    every 30s     │           │
│         └──────────────────┘           │
└────────────────────────────────────────┘
              │
              │ LoRaWAN
              ▼
       [TTN Gateway]
```

### Secondary BLE Beacon
```
┌────────────────────────────┐
│  ESP32 Board               │
│                            │
│  ┌──────────────────────┐  │
│  │   BLE Advertising    │  │
│  │                      │  │
│  │ • Broadcast MAC      │  │
│  │ • Continuous         │  │
│  │ • 100ms interval     │  │
│  └──────────────────────┘  │
│             │              │
│             ▼ Broadcast    │
└─────────────────────────────┘
              │
              │ BLE Signal (RSSI)
              ▼
    [Detected by Lighthouse]
```

## Timing Diagram

```
Time (seconds)
0        5        10       15       20       25       30
│        │        │        │        │        │        │
├─BLE────┤                                             
│ Scan   │
│        ├─WiFi───┤
│        │ Scan   │
│        │        ├─Process─┤
│        │        │ + Encode│
│        │        │         ├─LoRa TX─┤
│        │        │         │         │
│        │        │         │         ├─Sleep──────────►
│        │        │         │         │
│◄───────┴────────┴─────────┴─────────┴────30s cycle──►
│
└─Repeat────────────────────────────────────────────────►
```

## Decision Flow

### Crowd Level Classification
```
Total Devices?
    │
    ├─ ≤ 5  ────────────► CALM
    │
    ├─ 6-15 ────────────► MODERATE
    │
    └─ ≥ 16 ────────────► CROWDED
```

### Environment Classification
```
First Scan?
    │
    ├─ YES ────────────► UNKNOWN
    │
    └─ NO
        │
        Enough Networks?
            │
            ├─ NO ────────────► UNKNOWN
            │
            └─ YES
                │
                Calculate Change Ratio
                    │
                    ├─ < 30% ───────► STATIC
                    │
                    └─ ≥ 30% ───────► MOBILE
```

### Beacon Detection
```
During BLE Scan
    │
    For Each Detected Device
        │
        MAC == BEACON_ADDRESS?
            │
            ├─ YES ───► Set beaconDetected = true
            │          Record RSSI
            │
            └─ NO ────► Continue scanning

End of Scan
    │
    beaconDetected?
        │
        ├─ TRUE ───► Include in payload
        │            (detected=1, RSSI=actual)
        │
        └─ FALSE ──► Include in payload
                     (detected=0, RSSI=-128)
```

## Hardware Setup Diagram

```
        Room A                              Room B
┌─────────────────────┐          ┌──────────────────────┐
│                     │          │                      │
│  ┌──────────────┐   │          │   ┌──────────────┐  │
│  │  Lighthouse  │   │          │   │ BLE Beacon   │  │
│  │  Node        │   │          │   │ (ESP32)      │  │
│  │              │   │          │   │              │  │
│  │ • BLE Scan   │←──┼─────BLE──┼──►│ • Advertise  │  │
│  │ • WiFi Scan  │←──┼──WiFi────┼───│              │  │
│  │ • LoRa TX    │   │          │   └──────────────┘  │
│  └──────────────┘   │          │                      │
│         │           │          │                      │
│         │ LoRaWAN   │          │                      │
│         ▼           │          │                      │
└─────────────────────┘          └──────────────────────┘
          │
          │
          ▼
   [LoRaWAN Gateway]
          │
          │ Internet
          ▼
   [The Things Network]
          │
          ▼
   [Your Application]
```

## State Machine

```
┌─────────────┐
│   DEVICE    │
│   INIT      │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│   JOIN      │
│   NETWORK   │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│   SEND      │
│  (Scan &    │◄─────┐
│  Transmit)  │      │
└──────┬──────┘      │
       │             │
       ▼             │
┌─────────────┐      │
│   CYCLE     │      │
│  (Schedule) │      │
└──────┬──────┘      │
       │             │
       ▼             │
┌─────────────┐      │
│   SLEEP     │      │
│  (~30 sec)  │──────┘
└─────────────┘
```

## Data Structures

### Key Variables
```cpp
// BLE Tracking
std::set<std::string> uniqueBLEDevices;  // All detected BLE devices
uint16_t bleDeviceCount;                 // Count of unique devices
bool beaconDetected;                     // Tracked beacon found?
int8_t beaconRSSI;                       // Beacon signal strength

// WiFi Analysis  
std::set<std::string> uniqueWiFiNetworks;     // Current scan
std::set<std::string> previousWiFiNetworks;   // Previous scan
uint16_t wifiNetworkCount;                    // Count of APs
bool firstWiFiScan;                           // Initialization flag

// Environment
EnvironmentType environmentType;         // STATIC/MOBILE/UNKNOWN
float wifiChangeRatio;                   // Change percentage

// Crowd Detection
uint16_t totalUniqueDevices;            // BLE + WiFi
CrowdLevel currentCrowdLevel;           // CALM/MODERATE/CROWDED
```

## Extension Points

Future enhancements could add:

```
┌────────────────────────────────────┐
│  Additional Features               │
├────────────────────────────────────┤
│ • Multiple beacon tracking         │
│ • RSSI history/trending            │
│ • Advanced movement detection      │
│ • WiFi network fingerprinting      │
│ • Adaptive thresholds              │
│ • Edge processing/ML               │
│ • Battery monitoring               │
│ • GPS location tracking            │
└────────────────────────────────────┘
```

This architecture provides a solid foundation for crowd detection with device tracking and environment analysis capabilities!
