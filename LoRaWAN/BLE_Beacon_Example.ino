/*
 * Simple BLE Beacon for ESP32
 * 
 * This code configures an ESP32 board as a simple BLE beacon
 * that can be tracked by the LoRa Lighthouse Node.
 * 
 * Upload this code to a second ESP32 board and place it in
 * another room to track its presence and signal strength.
 * 
 * The Lighthouse Node will detect this beacon during BLE scans
 * and report its presence and RSSI in the LoRaWAN uplink.
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEAdvertising.h>

// Beacon configuration
#define BEACON_NAME "Tracked_Beacon"
#define ADVERTISING_INTERVAL 100  // Advertising interval in milliseconds

void setup() {
  Serial.begin(115200);
  Serial.println("\n========================================");
  Serial.println("BLE Beacon for LoRa Lighthouse Tracking");
  Serial.println("========================================\n");
  
  // Initialize BLE
  Serial.println("Initializing BLE...");
  BLEDevice::init(BEACON_NAME);
  
  // Create BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  
  // Get advertising instance
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  
  // Configure advertising parameters
  pAdvertising->setMinInterval(ADVERTISING_INTERVAL);
  pAdvertising->setMaxInterval(ADVERTISING_INTERVAL);
  
  // Start advertising
  pAdvertising->start();
  
  // Display beacon information
  Serial.println("BLE Beacon started successfully!");
  Serial.println("\n--- Beacon Information ---");
  Serial.print("Name: ");
  Serial.println(BEACON_NAME);
  Serial.print("MAC Address: ");
  Serial.println(BLEDevice::getAddress().toString().c_str());
  Serial.print("Advertising Interval: ");
  Serial.print(ADVERTISING_INTERVAL);
  Serial.println(" ms");
  Serial.println("==========================\n");
  
  Serial.println("IMPORTANT: Copy the MAC Address above and update");
  Serial.println("the BEACON_ADDRESS in your LoRaWAN.ino file!");
  Serial.println("\nExample:");
  Serial.print("#define BEACON_ADDRESS \"");
  Serial.print(BLEDevice::getAddress().toString().c_str());
  Serial.println("\"\n");
  
  Serial.println("The beacon is now broadcasting continuously.");
  Serial.println("The Lighthouse Node should be able to detect it.\n");
}

void loop() {
  // Nothing to do here - beacon advertises automatically
  // Just print a heartbeat every 10 seconds
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 10000) {
    Serial.println("Beacon is active...");
    lastPrint = millis();
  }
  
  delay(100);
}
