function decodeUplink(input) {
    const bytes = input.bytes || [];
    const len = bytes.length;

    // Read u16 (big endian) with bounds check
    const u16at = (idx) => {
        if (len > idx + 1) {
            return (((bytes[idx] & 0xFF) << 8) | (bytes[idx + 1] & 0xFF)) >>> 0;
        }
        return 0;
    };

    // Helper to convert uint8_t to int8_t (signed). If undefined, return provided default.
    const toInt8 = (byte, defaultValue = -128) => {
        if (typeof byte === 'undefined') return defaultValue;
        const val = byte & 0xFF;
        return val > 127 ? val - 256 : val;
    };

    const getByte = (idx, defaultValue = 0) => (len > idx ? (bytes[idx] & 0xFF) : defaultValue);

    const bleCount = u16at(0);
    const wifiCount = u16at(2);
    const totalCount = u16at(4);
    const crowdLevel = getByte(6, 0);
    const beaconDetected = getByte(7, 0) === 1;
    const beaconRssi = toInt8(len > 8 ? bytes[8] : undefined, -128);
    const environmentType = getByte(9, 2);

    const crowdLevelText = ['CALM', 'MODERATE', 'CROWDED'][crowdLevel] || 'UNKNOWN';
    const environmentText = ['STATIC', 'MOBILE', 'UNKNOWN'][environmentType] || 'UNKNOWN';

    const warnings = [];
    if (len < 7) warnings.push('Payload too short: fewer than 7 bytes - unexpected payload');
    else if (len < 10) warnings.push('Short payload detected (7 bytes) - extended fields defaulted');

    return {
        warnings: warnings,
        data: {
            ble_count: bleCount,
            wifi_count: wifiCount,
            total_count: totalCount,
            crowd_level: crowdLevel,
            crowd_level_text: crowdLevelText,
            beacon_detected: beaconDetected,
            beacon_rssi: beaconRssi,
            environment_type: environmentType,
            environment_text: environmentText
        }
    };
}