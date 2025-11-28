function decodeUplink(input) {
    const bytes = input.bytes || [];
    const u16 = (hi, lo) => (((hi || 0) << 8) | (lo || 0)) >>> 0;
    
    // Helper to convert uint8_t to int8_t (signed)
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
            environment_text: ['STATIC', 'MOBILE', 'UNKNOWN'][bytes[9] || 2]
        }
    };
}