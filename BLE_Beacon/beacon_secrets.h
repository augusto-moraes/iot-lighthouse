/*
 * Beacon Secrets/Configuration
 * 
 * This file contains beacon-specific configuration.
 * For now it's minimal, but can be extended for future features
 * like multiple beacon support, custom advertising data, etc.
 */

#ifndef BEACON_SECRETS_H
#define BEACON_SECRETS_H

// ====== Beacon Identification ======
// Change this to identify your beacon in a multi-beacon system
#define BEACON_ID "DogHowler"

// ====== Optional Features ======
// Uncomment to enable:
// #define ENABLE_STATUS_LED        // Enable status LED blinking
// #define ENABLE_SERIAL_DEBUG      // Enhanced serial output (default is on)

#endif // BEACON_SECRETS_H
