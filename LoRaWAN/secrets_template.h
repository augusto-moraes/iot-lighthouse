/*
 * Secrets Configuration
 * 
 * IMPORTANT: Copy this file to secrets.h and update with your actual credentials
 * The secrets.h file is ignored by git to keep your credentials private
 * 
 * Get your LoRaWAN credentials from:
 * The Things Network Console > Applications > Your App > End Devices
 */

#ifndef SECRETS_H
#define SECRETS_H

// ====== LoRaWAN OTAA Credentials ======
// DevEUI - Device EUI (8 bytes)
#define LORAWAN_DEV_EUI { 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x07, 0x43, 0x59 }

// AppEUI - Application EUI (8 bytes) 
#define LORAWAN_APP_EUI { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }

// AppKey - Application Key (16 bytes)
#define LORAWAN_APP_KEY { 0xCC, 0x4E, 0xEA, 0x15, 0x71, 0xB5, 0x82, 0x99, 0x35, 0x3C, 0xD1, 0x5D, 0x12, 0x6E, 0xDA, 0x5D }

// ====== BLE Beacon Configuration ======
// Beacon MAC Address - Replace with your beacon's address
// Format: "AA:BB:CC:DD:EE:FF" or leave as "00:00:00:00:00:00" if not using beacon tracking
#define BEACON_MAC_ADDRESS "00:00:00:00:00:00"

#endif // SECRETS_H
