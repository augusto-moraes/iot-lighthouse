/* Heltec Automation LoRaWAN Crowd Detection System
 *
 * Function:
 * 1. Scan for BLE devices and Wi-Fi networks to estimate crowd density
 * 2. Count unique devices during scan windows
 * 3. Classify crowd level (calm/moderate/crowded) using threshold model
 * 4. Track specific BLE beacon device (presence and RSSI)
 * 5. Detect static vs mobile environment based on WiFi AP and BLE device changes
 * 6. Transmit crowd data via LoRaWAN protocol
 *  
 * Description:
 * 1. Uses passive BLE scanning to detect nearby Bluetooth devices
 * 2. Scans Wi-Fi networks to count access points and connected devices
 * 3. Implements threshold-based crowd classification
 * 4. Tracks a specific BLE beacon and reports its presence/signal strength
 * 5. Analyzes WiFi scan stability to classify environment as static or mobile
 * 6. Sends aggregated data through LoRaWAN
 * 
 * HelTec AutoMation, Chengdu, China
 * 成都惠利特自动化科技有限公司
 * www.heltec.org
 *
 * */

#include "LoRaWan_APP.h"
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <WiFi.h>
#include <set>
#include <string>
#include "secrets.h"  // LoRaWAN credentials and beacon address

/* OTAA para - Loaded from secrets.h */
uint8_t devEui[] = LORAWAN_DEV_EUI;
uint8_t appEui[] = LORAWAN_APP_EUI;
uint8_t appKey[] = LORAWAN_APP_KEY;

/* ABP para*/
uint8_t nwkSKey[] = { 0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda,0x85 };
uint8_t appSKey[] = { 0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef,0x67 };
uint32_t devAddr =  ( uint32_t )0x007e6ae1;

/*LoraWan channelsmask, default channels 0-7*/ 
uint16_t userChannelsMask[6]={ 0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000 };

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t  loraWanClass = CLASS_A;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 30000;

/*OTAA or ABP*/
bool overTheAirActivation = true;

/*ADR enable*/
bool loraWanAdr = true;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = true;

/* Application port */
uint8_t appPort = 2;

/*!
* Number of trials to transmit the frame, if the LoRaMAC layer did not
* receive an acknowledgment. The MAC performs a datarate adaptation,
* according to the LoRaWAN Specification V1.0.2, chapter 18.4, according
* to the following table:
*
* Transmission nb | Data Rate
* ----------------|-----------
* 1 (first)       | DR
* 2               | DR
* 3               | max(DR-1,0)
* 4               | max(DR-1,0)
* 5               | max(DR-2,0)
* 6               | max(DR-2,0)
* 7               | max(DR-3,0)
* 8               | max(DR-3,0)
*
* Note, that if NbTrials is set to 1 or 2, the MAC will not decrease
* the datarate, in case the LoRaMAC layer did not receive an acknowledgment
*/
uint8_t confirmedNbTrials = 4;

// ====== Crowd Detection Configuration ======
#define BLE_SCAN_TIME 20          // BLE scan duration in seconds
#define WIFI_SCAN_TIME 20         // Time to wait for WiFi scan completion

// Crowd level thresholds
#define CALM_THRESHOLD 75         // 0-5 devices = calm
#define MODERATE_THRESHOLD 125    // 6-15 devices = moderate
                                 // 16+ devices = crowded

// ====== BLE Beacon Tracking Configuration ======
// Beacon address is loaded from secrets.h
#define BEACON_ADDRESS BEACON_MAC_ADDRESS
#define RSSI_NOT_FOUND -128      // RSSI value when beacon is not detected

// ====== Environment Detection Configuration ======
#define WIFI_CHANGE_THRESHOLD 0.05  // 5% change threshold for mobile detection
#define MIN_DEVICES_FOR_ANALYSIS 3  // Minimum devices (WiFi + BLE) needed for analysis
#define MIN_NETWORKS_FOR_ANALYSIS MIN_DEVICES_FOR_ANALYSIS // compatibility alias

// Crowd level enumeration
enum CrowdLevel {
  CALM = 0,
  MODERATE = 1,
  CROWDED = 2
};

// Environment type enumeration
enum EnvironmentType {
  STATIC = 0,
  MOBILE = 1,
  UNKNOWN = 2  // Not enough data yet
};

// ====== BLE Scanning Variables ======
BLEScan* pBLEScan;
std::set<std::string> uniqueBLEDevices;
uint16_t bleDeviceCount = 0;

// ====== BLE Beacon Tracking Variables ======
bool beaconDetected = false;
int8_t beaconRSSI = RSSI_NOT_FOUND;
String targetBeaconAddress = BEACON_ADDRESS;

// ====== WiFi Scanning Variables ======
std::set<std::string> uniqueWiFiNetworks;
std::set<std::string> previousWiFiNetworks;
uint16_t wifiNetworkCount = 0;
bool firstWiFiScan = true;

// ====== Combined Device Scanning Variables (WiFi + BLE) ======
std::set<std::string> previousDevices;   // union of previous BLE and WiFi device IDs (RAM only)
RTC_DATA_ATTR bool firstDeviceScan = true;  // RTC memory survives deep sleep
uint16_t combinedDeviceCount = 0;

// ====== RTC Memory for Device Tracking Across Deep Sleep ======
#define MAX_RTC_DEVICES 256  // Maximum devices to track in RTC memory
RTC_DATA_ATTR uint32_t previousDeviceHashes[MAX_RTC_DEVICES];  // Hash of device addresses
RTC_DATA_ATTR uint16_t previousDeviceCount = 0;  // Count of stored device hashes

// ====== Environment Detection Variables ======
RTC_DATA_ATTR EnvironmentType environmentType = UNKNOWN;  // RTC memory survives deep sleep
// 'deviceChangeRatio' is the Jaccard distance across ALL devices (WiFi + BLE)
RTC_DATA_ATTR float deviceChangeRatio = 0.0;  // RTC memory survives deep sleep

// ====== Crowd Detection Variables ======
uint16_t totalUniqueDevices = 0;
CrowdLevel currentCrowdLevel = CALM;

// ====== BLE Callback Class ======
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // Get MAC address as unique identifier
    String arduinoAddress = advertisedDevice.getAddress().toString();
    std::string address = arduinoAddress.c_str();
    uniqueBLEDevices.insert(address);
    
    // Check if this is our tracked beacon
    if (arduinoAddress.equalsIgnoreCase(targetBeaconAddress)) {
      beaconDetected = true;
      beaconRSSI = advertisedDevice.getRSSI();
      Serial.print("  >> Tracked beacon found! Address: ");
      Serial.print(arduinoAddress);
      Serial.print(", RSSI: ");
      Serial.println(beaconRSSI);
    }
  }
};

// ====== BLE Scanning Function ======
void scanBLEDevices() {
  Serial.println("Starting BLE scan...");
  
  // Clear previous scan results
  uniqueBLEDevices.clear();
  
  // Reset beacon tracking for this scan
  beaconDetected = false;
  beaconRSSI = RSSI_NOT_FOUND;
  
  // Start BLE scan (returns pointer to results)
  BLEScanResults* foundDevices = pBLEScan->start(BLE_SCAN_TIME, false);
  
  // Update count
  bleDeviceCount = uniqueBLEDevices.size();
  
  Serial.print("BLE scan complete. Found ");
  Serial.print(bleDeviceCount);
  Serial.println(" unique devices.");
  
  // Report beacon status
  if (beaconDetected) {
    Serial.println("  Tracked beacon: DETECTED");
  } else {
    Serial.println("  Tracked beacon: NOT FOUND");
  }
  
  // Clear scan results to free memory
  pBLEScan->clearResults();
}

// ====== WiFi Scanning Function ======
void scanWiFiNetworks() {
  Serial.println("Starting WiFi scan...");
  
  // Store previous scan results before clearing
  if (!firstWiFiScan) {
    previousWiFiNetworks = uniqueWiFiNetworks;
  }
  
  // Clear current scan results
  uniqueWiFiNetworks.clear();
  
  // Disconnect from any networks to ensure clean scan
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  // Start WiFi scan (async = false, show_hidden = true)
  int networkCount = WiFi.scanNetworks(false, true);
  
  if (networkCount == -1) {
    Serial.println("WiFi scan failed!");
    wifiNetworkCount = 0;
    return;
  }
  
  // Process scan results and count unique networks
  for (int i = 0; i < networkCount; i++) {
    // Use BSSID (MAC address) as unique identifier
    String bssid = WiFi.BSSIDstr(i);
    uniqueWiFiNetworks.insert(bssid.c_str());
  }
  
  wifiNetworkCount = uniqueWiFiNetworks.size();
  
  Serial.print("WiFi scan complete. Found ");
  Serial.print(wifiNetworkCount);
  Serial.println(" unique networks.");
  
  // Analyze environment stability
  analyzeEnvironment();
  
  // Mark that we've completed at least one scan
  firstWiFiScan = false;
  
  // Clean up
  WiFi.scanDelete();
}

// ====== Simple String Hash Function (djb2) ======
uint32_t hashString(const std::string& str) {
  uint32_t hash = 5381;
  for (char c : str) {
    hash = ((hash << 5) + hash) + c;  // hash * 33 + c
  }
  return hash;
}

// ====== Environment Analysis Function ======
void analyzeEnvironment() {
  // Build combined set of current WiFi networks and BLE devices
  std::set<std::string> combinedCurrent = uniqueWiFiNetworks;
  for (const auto& dev : uniqueBLEDevices) {
    combinedCurrent.insert(dev);
  }

  // Build set of current device hashes
  std::set<uint32_t> currentHashes;
  for (const auto& device : combinedCurrent) {
    currentHashes.insert(hashString(device));
  }

  // Count combined devices
  combinedDeviceCount = combinedCurrent.size();

  // Need at least two scans to compare; initialize baseline on first run
  if (firstDeviceScan) {
    environmentType = UNKNOWN;
    deviceChangeRatio = 0.0;
    Serial.println("  Environment: UNKNOWN (first device scan)");
    
    // Store current device hashes in RTC memory for next wake cycle
    previousDeviceCount = min((uint16_t)currentHashes.size(), (uint16_t)MAX_RTC_DEVICES);
    uint16_t idx = 0;
    for (const auto& h : currentHashes) {
      if (idx >= MAX_RTC_DEVICES) break;
      previousDeviceHashes[idx++] = h;
    }
    
    firstDeviceScan = false;
    return;
  }

  // Build set of previous device hashes from RTC memory
  std::set<uint32_t> previousHashes;
  for (uint16_t i = 0; i < previousDeviceCount; i++) {
    previousHashes.insert(previousDeviceHashes[i]);
  }

  // Calculate intersection (common devices by hash)
  uint16_t intersectionCount = 0;
  for (const auto& h : currentHashes) {
    if (previousHashes.find(h) != previousHashes.end()) {
      intersectionCount++;
    }
  }

  // Calculate union size
  std::set<uint32_t> unionHashes = currentHashes;
  for (const auto& h : previousHashes) {
    unionHashes.insert(h);
  }
  uint16_t unionCount = unionHashes.size();
  
  // Calculate change ratio using Jaccard distance
  // Change ratio = 1 - (intersection / union)
  if (unionCount > 0) {
    float similarity = (float)intersectionCount / (float)unionCount;
    deviceChangeRatio = 1.0 - similarity;
  } else {
    deviceChangeRatio = 0.0;
  }
  
  // Classify environment
  if (deviceChangeRatio > WIFI_CHANGE_THRESHOLD) {
    environmentType = MOBILE;
    Serial.print("  Environment: MOBILE (change ratio: ");
  } else {
    environmentType = STATIC;
    Serial.print("  Environment: STATIC (change ratio: ");
  }
  Serial.print(deviceChangeRatio, 2);
  Serial.println(")");
  
  Serial.print("    Common devices: ");
  Serial.print(intersectionCount);
  Serial.print(", Total unique: ");
  Serial.println(unionCount);

  // Store current device hashes in RTC memory for next wake cycle
  previousDeviceCount = min((uint16_t)currentHashes.size(), (uint16_t)MAX_RTC_DEVICES);
  uint16_t idx = 0;
  for (const auto& h : currentHashes) {
    if (idx >= MAX_RTC_DEVICES) break;
    previousDeviceHashes[idx++] = h;
  }
}

// ====== Crowd Level Classification Function ======
CrowdLevel classifyCrowdLevel(uint16_t deviceCount) {
  if (deviceCount <= CALM_THRESHOLD) {
    return CALM;
  } else if (deviceCount <= MODERATE_THRESHOLD) {
    return MODERATE;
  } else {
    return CROWDED;
  }
}

// ====== Main Scanning and Classification Function ======
void performCrowdDetection() {
  Serial.println("\n========== Crowd Detection Cycle ==========");
  
  // Perform BLE scan
  scanBLEDevices();
  
  // Perform WiFi scan
  scanWiFiNetworks();
  
  // Calculate total unique devices
  totalUniqueDevices = bleDeviceCount + wifiNetworkCount;
  
  // Classify crowd level
  currentCrowdLevel = classifyCrowdLevel(totalUniqueDevices);
  
  // Print summary
  Serial.println("\n--- Detection Summary ---");
  Serial.print("BLE Devices: ");
  Serial.println(bleDeviceCount);
  Serial.print("WiFi Networks: ");
  Serial.println(wifiNetworkCount);
  Serial.print("Total Devices: ");
  Serial.println(totalUniqueDevices);
  Serial.print("Combined Unique Devices (WiFi + BLE): ");
  Serial.println(combinedDeviceCount);
  Serial.print("Crowd Level: ");
  
  switch (currentCrowdLevel) {
    case CALM:
      Serial.println("CALM");
      break;
    case MODERATE:
      Serial.println("MODERATE");
      break;
    case CROWDED:
      Serial.println("CROWDED");
      break;
  }
  
  // Beacon tracking summary
  Serial.print("Tracked Beacon: ");
  if (beaconDetected) {
    Serial.print("PRESENT (RSSI: ");
    Serial.print(beaconRSSI);
    Serial.println(" dBm)");
  } else {
    Serial.println("NOT FOUND");
  }
  
  // Environment type summary
  Serial.print("Environment: ");
  switch (environmentType) {
    case STATIC:
      Serial.println("STATIC");
      break;
    case MOBILE:
      Serial.println("MOBILE");
      break;
    case UNKNOWN:
      Serial.println("UNKNOWN");
      break;
  }
  Serial.print("Device Change Ratio: ");
  Serial.print(deviceChangeRatio, 2);
  Serial.println("\n");
  
  Serial.println("==========================================\n");
}

/* Prepares the payload of the frame */
static void prepareTxFrame( uint8_t port )
{
  /*appData size is LORAWAN_APP_DATA_MAX_SIZE which is defined in "commissioning.h".
  *appDataSize max value is LORAWAN_APP_DATA_MAX_SIZE.
  *if enabled AT, don't modify LORAWAN_APP_DATA_MAX_SIZE, it may cause system hanging or failure.
  *if disabled AT, LORAWAN_APP_DATA_MAX_SIZE can be modified, the max value is reference to lorawan region and SF.
  *for example, if use REGION_CN470, 
  *the max value for different DR can be found in MaxPayloadOfDatarateCN470 refer to DataratesCN470 and BandwidthsCN470 in "RegionCN470.h".
  */
  
  // Perform crowd detection before preparing transmission
  performCrowdDetection();
  
  // Prepare payload with crowd detection data
  // Payload format:
  // Byte 0-1: BLE device count (uint16_t, big-endian)
  // Byte 2-3: WiFi network count (uint16_t, big-endian)
  // Byte 4-5: Total device count (uint16_t, big-endian)
  // Byte 6: Crowd level (uint8_t: 0=CALM, 1=MODERATE, 2=CROWDED)
  // Byte 7: Beacon detected (uint8_t: 0=false, 1=true)
  // Byte 8: Beacon RSSI (int8_t: signed, -128 if not found)
  // Byte 9: Environment type (uint8_t: 0=STATIC, 1=MOBILE, 2=UNKNOWN)
  
  appDataSize = 10;
  
  // BLE device count (bytes 0-1)
  appData[0] = (bleDeviceCount >> 8) & 0xFF;  // High byte
  appData[1] = bleDeviceCount & 0xFF;          // Low byte
  
  // WiFi network count (bytes 2-3)
  appData[2] = (wifiNetworkCount >> 8) & 0xFF; // High byte
  appData[3] = wifiNetworkCount & 0xFF;         // Low byte
  
  // Total device count (bytes 4-5)
  appData[4] = (totalUniqueDevices >> 8) & 0xFF; // High byte
  appData[5] = totalUniqueDevices & 0xFF;         // Low byte
  
  // Crowd level (byte 6)
  appData[6] = (uint8_t)currentCrowdLevel;
  
  // Beacon detected (byte 7)
  appData[7] = beaconDetected ? 1 : 0;
  
  // Beacon RSSI (byte 8) - cast to uint8_t for transmission
  appData[8] = (uint8_t)beaconRSSI;
  
  // Environment type (byte 9)
  appData[9] = (uint8_t)environmentType;
  
  Serial.println("LoRaWAN payload prepared:");
  Serial.print("  BLE: ");
  Serial.println(bleDeviceCount);
  Serial.print("  WiFi: ");
  Serial.println(wifiNetworkCount);
  Serial.print("  Total: ");
  Serial.println(totalUniqueDevices);
  Serial.print("  Level: ");
  Serial.println(currentCrowdLevel);
  Serial.print("  Beacon: ");
  Serial.print(beaconDetected ? "YES" : "NO");
  if (beaconDetected) {
    Serial.print(" (RSSI: ");
    Serial.print(beaconRSSI);
    Serial.print(" dBm)");
  }
  Serial.println();
  Serial.print("  Environment: ");
  Serial.println(environmentType);
}

//if true, next uplink will add MOTE_MAC_DEVICE_TIME_REQ 


void setup() {
  Serial.begin(115200);
  
  // Initialize MCU and LoRa board
  Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);
  
  // Initialize BLE
  Serial.println("Initializing BLE...");
  BLEDevice::init("LoRa_Lighthouse");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);  // Active scan uses more power but gets more data
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);        // Less or equal to setInterval value
  Serial.println("BLE initialized.");
  
  // WiFi will be initialized in scan mode when needed
  Serial.println("WiFi ready for scanning.");
  
  Serial.println("\n========================================");
  Serial.println("LoRa Lighthouse Node - Crowd Detection");
  Serial.println("========================================");
  Serial.println("Configuration:");
  Serial.print("  Scan Window: ");
  Serial.print(BLE_SCAN_TIME);
  Serial.println(" seconds");
  Serial.print("  Calm Threshold: <= ");
  Serial.print(CALM_THRESHOLD);
  Serial.println(" devices");
  Serial.print("  Moderate Threshold: <= ");
  Serial.print(MODERATE_THRESHOLD);
  Serial.println(" devices");
  Serial.print("  Crowded Threshold: > ");
  Serial.print(MODERATE_THRESHOLD);
  Serial.println(" devices");
  Serial.print("  Tracked Beacon: ");
  Serial.println(targetBeaconAddress);
  Serial.print("  Device Change Threshold (WiFi + BLE): ");
  Serial.print(WIFI_CHANGE_THRESHOLD * 100);
  Serial.println("%");
  Serial.print("  LoRaWAN TX Interval: ");
  Serial.print(appTxDutyCycle / 1000);
  Serial.println(" seconds");
  Serial.println("========================================\n");
}

void loop()
{
  switch( deviceState )
  {
    case DEVICE_STATE_INIT:
    {
#if(LORAWAN_DEVEUI_AUTO)
      LoRaWAN.generateDeveuiByChipID();
#endif
      LoRaWAN.init(loraWanClass,loraWanRegion);
      //both set join DR and DR when ADR off 
      LoRaWAN.setDefaultDR(3);
      break;
    }
    case DEVICE_STATE_JOIN:
    {
      LoRaWAN.join();
      break;
    }
    case DEVICE_STATE_SEND:
    {
      prepareTxFrame( appPort );
      LoRaWAN.send();
      deviceState = DEVICE_STATE_CYCLE;
      break;
    }
    case DEVICE_STATE_CYCLE:
    {
      // Schedule next packet transmission
      txDutyCycleTime = appTxDutyCycle + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND );
      LoRaWAN.cycle(txDutyCycleTime);
      deviceState = DEVICE_STATE_SLEEP;
      break;
    }
    case DEVICE_STATE_SLEEP:
    {
      LoRaWAN.sleep(loraWanClass);
      break;
    }
    default:
    {
      deviceState = DEVICE_STATE_INIT;
      break;
    }
  }
}