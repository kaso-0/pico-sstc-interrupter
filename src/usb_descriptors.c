#include "tusb.h"

//--------------------------------------------------------------------+
// Device Descriptor
//--------------------------------------------------------------------+

tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = 0xCafe,
    .idProduct          = 0x4010,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 1
};

uint8_t const* tud_descriptor_device_cb(void) {
    return (uint8_t const*)&desc_device;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+
#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_MIDI_DESC_LEN)
#define EPNUM_MIDI_OUT    0x01
#define EPNUM_MIDI_IN     0x81
#define MIDI_EP_SIZE      64

uint8_t const desc_configuration[] = {
    // Configuration descriptor
    TUD_CONFIG_DESCRIPTOR(1, 2, 0, CONFIG_TOTAL_LEN, 0x00, 100),

    // MIDI descriptor (interface 0, no string index)
    TUD_MIDI_DESCRIPTOR(0, 0, EPNUM_MIDI_OUT, EPNUM_MIDI_IN, MIDI_EP_SIZE)
};


uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

static const char* string_desc_arr[] = {
    (const char[]){0x09, 0x04}, // 0: English (0x0409)
    "My MIDI Manufacturer",     // 1: Manufacturer
    "My MIDI Device",           // 2: Product
    "123456",                   // 3: Serial Number
};

static uint16_t _desc_str[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;

    if (index == 0) {
        _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * sizeof(uint16_t));
        _desc_str[1] = 0x0409; // English (US)
        return _desc_str;
    }

    if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) return NULL;

    const char* str = string_desc_arr[index];
    uint8_t len = (uint8_t)strlen(str);

    if (len > 31) len = 31;

    _desc_str[0] = (TUSB_DESC_STRING << 8) | ((2 * len) + 2);
    for (uint8_t i = 0; i < len; i++) {
        _desc_str[1 + i] = str[i];
    }

    return _desc_str;
}
