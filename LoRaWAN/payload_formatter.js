function decodeUplink(input) {
    const bytes = input.bytes || [];
    const u16 = (hi, lo) => (((hi || 0) << 8) | (lo || 0)) >>> 0;

    return {
        data: {
            ble_count: u16(bytes[0], bytes[1]),
            wifi_count: u16(bytes[2], bytes[3]),
            total_count: u16(bytes[4], bytes[5]),
            crowd_level: bytes[6] || 0,
            crowd_level_text: ['CALM', 'MODERATE', 'CROWDED'][bytes[6] || 0]
        }
    };
}