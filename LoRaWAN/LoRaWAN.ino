/* Heltec Automation LoRaWAN Crowd Detection System
 *
 * Function:
 * 1. Scan for BLE devices and Wi-Fi networks to estimate crowd density
 * 2. Count unique devices during scan windows
 * 3. Classify crowd level (calm/moderate/crowded) using threshold model
 * 4. Transmit crowd data via LoRaWAN protocol
 *  
 * Description:
 * 1. Uses passive BLE scanning to detect nearby Bluetooth devices
 * 2. Scans Wi-Fi networks to count access points and connected devices
 * 3. Implements threshold-based crowd classification
 * 4. Sends aggregated data through LoRaWAN
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

/* OTAA para*/
uint8_t devEui[] = { 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x07, 0x43, 0x59 };
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appKey[] = { 0xCC, 0x4E, 0xEA, 0x15, 0x71, 0xB5, 0x82, 0x99, 0x35, 0x3C, 0xD1, 0x5D, 0x12, 0x6E, 0xDA, 0x5D };

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
#define BLE_SCAN_TIME 5          // BLE scan duration in seconds
#define WIFI_SCAN_TIME 5         // Time to wait for WiFi scan completion

// Crowd level thresholds
#define CALM_THRESHOLD 5         // 0-5 devices = calm
#define MODERATE_THRESHOLD 15    // 6-15 devices = moderate
                                 // 16+ devices = crowded

// Crowd level enumeration
enum CrowdLevel {
  CALM = 0,
  MODERATE = 1,
  CROWDED = 2
};

// ====== BLE Scanning Variables ======
BLEScan* pBLEScan;
std::set<std::string> uniqueBLEDevices;
uint16_t bleDeviceCount = 0;

// ====== WiFi Scanning Variables ======
std::set<std::string> uniqueWiFiNetworks;
uint16_t wifiNetworkCount = 0;

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
  }
};

// ====== BLE Scanning Function ======
void scanBLEDevices() {
  Serial.println("Starting BLE scan...");
  
  // Clear previous scan results
  uniqueBLEDevices.clear();
  
  // Start BLE scan (returns pointer to results)
  BLEScanResults* foundDevices = pBLEScan->start(BLE_SCAN_TIME, false);
  
  // Update count
  bleDeviceCount = uniqueBLEDevices.size();
  
  Serial.print("BLE scan complete. Found ");
  Serial.print(bleDeviceCount);
  Serial.println(" unique devices.");
  
  // Clear scan results to free memory
  pBLEScan->clearResults();
}

// ====== WiFi Scanning Function ======
void scanWiFiNetworks() {
  Serial.println("Starting WiFi scan...");
  
  // Clear previous scan results
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
  
  // Clean up
  WiFi.scanDelete();
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
  
  appDataSize = 7;
  
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
  
  Serial.println("LoRaWAN payload prepared:");
  Serial.print("  BLE: ");
  Serial.println(bleDeviceCount);
  Serial.print("  WiFi: ");
  Serial.println(wifiNetworkCount);
  Serial.print("  Total: ");
  Serial.println(totalUniqueDevices);
  Serial.print("  Level: ");
  Serial.println(currentCrowdLevel);
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