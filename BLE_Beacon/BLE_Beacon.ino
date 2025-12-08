/*
 * IoT Lighthouse - BLE Beacon Implementation
 * 
 * This is a standalone BLE beacon application designed to be uploaded
 * to a separate ESP32 device for tracking by the LoRa Lighthouse Node.
 * 
 * Hardware: Any ESP32 board (Heltec WiFi LoRa 32 V3 or standard ESP32)
 * 
 * Features:
 * - Configurable beacon name and advertising interval
 * - Reports MAC address via Serial Monitor
 * - Continuous BLE advertising for reliable detection
 * - Low power consumption during operation
 * 
 * Setup:
 * 1. Upload this sketch to a second ESP32 device
 * 2. Open Serial Monitor (115200 baud)
 * 3. Note the MAC Address displayed
 * 4. Update BEACON_ADDRESS in LoRaWAN.ino with this MAC
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEAdvertising.h>

// ====== Beacon Configuration ======
#define BEACON_NAME "IoT-Lighthouse-Beacon"
#define ADVERTISING_INTERVAL 100  // Advertising interval in milliseconds (100ms = 10Hz)

// Status LED (optional - comment out if not using)
// #define LED_PIN 25
// #define LED_BLINK_INTERVAL 1000

void setup() {
  Serial.begin(115200);
  delay(1000);  // Give serial time to initialize
  
  Serial.println("\n\n========================================");
  Serial.println("  IoT Lighthouse - BLE Beacon");
  Serial.println("========================================\n");
  
  // Initialize optional LED
  #ifdef LED_PIN
  pinMode(LED_PIN, OUTPUT);
  Serial.println("✓ LED initialized on pin " + String(LED_PIN));
  #endif
  
  // Initialize BLE
  Serial.println("Initializing BLE device...");
  BLEDevice::init(BEACON_NAME);
  Serial.println("✓ BLE device initialized");
  
  // Create BLE Server (required for proper advertising)
  BLEServer *pServer = BLEDevice::createServer();
  Serial.println("✓ BLE server created");
  
  // Configure advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setMinInterval(ADVERTISING_INTERVAL);
  pAdvertising->setMaxInterval(ADVERTISING_INTERVAL);
  pAdvertising->start();
  Serial.println("✓ BLE advertising started");
  
  // Display beacon information
  Serial.println("\n--- Beacon Configuration ---");
  Serial.print("Name:               ");
  Serial.println(BEACON_NAME);
  
  String macAddress = BLEDevice::getAddress().toString();
  Serial.print("MAC Address:        ");
  Serial.println(macAddress);
  
  Serial.print("Advertising Rate:   ");
  Serial.print(1000 / ADVERTISING_INTERVAL);
  Serial.println(" Hz (" + String(ADVERTISING_INTERVAL) + "ms)");
  
  Serial.println("----------------------------\n");
  
  // Display setup instructions
  Serial.println("📋 NEXT STEPS:");
  Serial.println("1. Copy the MAC Address above");
  Serial.println("2. Open LoRaWAN.ino on the Lighthouse Node");
  Serial.println("3. Update BEACON_MAC_ADDRESS in secrets.h:");
  Serial.print("   #define BEACON_MAC_ADDRESS \"");
  Serial.print(macAddress);
  Serial.println("\"");
  Serial.println("4. Upload LoRaWAN.ino to the Lighthouse Node");
  Serial.println("5. The Lighthouse will detect this beacon!\n");
  
  Serial.println("🔋 Status: Beacon is now broadcasting continuously");
  Serial.println("💡 Keep this device powered on and within 30m of the Lighthouse\n");
}

void loop() {
  // Blink status LED if configured
  #ifdef LED_PIN
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > LED_BLINK_INTERVAL) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    lastBlink = millis();
  }
  #endif
  
  // Print heartbeat message every 30 seconds
  static unsigned long lastStatus = 0;
  if (millis() - lastStatus > 30000) {
    Serial.println("✓ Beacon active - MAC: " + BLEDevice::getAddress().toString());
    lastStatus = millis();
  }
  
  delay(100);
}
