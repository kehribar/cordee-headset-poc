// ----------------------------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include "usb_descriptors.h"
#include "tusb.h"
#include "pico/unique_id.h"

// ----------------------------------------------------------------------------
tusb_desc_device_t const desc_device = {
  .bLength            = sizeof(tusb_desc_device_t),
  .bDescriptorType    = TUSB_DESC_DEVICE,
  .bcdUSB             = 0x0200,
  .bDeviceClass       = TUSB_CLASS_MISC,
  .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
  .bDeviceProtocol    = MISC_PROTOCOL_IAD,
  .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
  .idVendor           = 0xCafe,
  .idProduct          = 0x4011,
  .bcdDevice          = 0x0100,
  .iManufacturer      = 0x01,
  .iProduct           = 0x02,
  .iSerialNumber      = 0x03,
  .bNumConfigurations = 0x01
};

uint8_t const* tud_descriptor_device_cb(void)
{
  return (uint8_t const*)&desc_device;
}

// ----------------------------------------------------------------------------
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + AUDIO_FUNC_DESC_LEN)

uint8_t const desc_configuration[] = {
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

  // -------------------------------------------------------------------------
  // Interface Association: 3 interfaces starting at ITF_NUM_AUDIO_CONTROL
  // -------------------------------------------------------------------------
  TUD_AUDIO_DESC_IAD(ITF_NUM_AUDIO_CONTROL, 3, 0x00),

  // -------------------------------------------------------------------------
  // Interface 0: Audio Control
  // -------------------------------------------------------------------------
  TUD_AUDIO_DESC_STD_AC(ITF_NUM_AUDIO_CONTROL, 0, 0x00),

  TUD_AUDIO_DESC_CS_AC(
    0x0200,
    AUDIO_FUNC_IO_BOX,
    AUDIO_AC_ENTITIES_LEN,
    AUDIO_CS_AS_INTERFACE_CTRL_LATENCY_POS
  ),

  // Clock Source
  TUD_AUDIO_DESC_CLK_SRC(
    AUDIO_CLK_ID,
    AUDIO_CLOCK_SOURCE_ATT_INT_FIX_CLK,
    (AUDIO_CTRL_R << AUDIO_CLOCK_SOURCE_CTRL_CLK_FRQ_POS),
    0x00,
    0x00
  ),

  // Speaker: USB → device (Input Terminal)
  TUD_AUDIO_DESC_INPUT_TERM(
    AUDIO_SPK_IT_ID,
    AUDIO_TERM_TYPE_USB_STREAMING,
    0x00,
    AUDIO_CLK_ID,
    AUDIO_SPK_CH,
    AUDIO_CHANNEL_CONFIG_NON_PREDEFINED,
    0x00,
    0x00,
    0x00
  ),

  // Speaker: device → line out (Output Terminal)
  TUD_AUDIO_DESC_OUTPUT_TERM(
    AUDIO_SPK_OT_ID,
    AUDIO_TERM_TYPE_OUT_DESKTOP_SPEAKER,
    AUDIO_SPK_IT_ID,
    AUDIO_SPK_IT_ID,
    AUDIO_CLK_ID,
    0x0000,
    0x00
  ),

  // Mic: input (Input Terminal)
  TUD_AUDIO_DESC_INPUT_TERM(
    AUDIO_MIC_IT_ID,
    AUDIO_TERM_TYPE_IN_GENERIC_MIC,
    0x00,
    AUDIO_CLK_ID,
    AUDIO_MIC_CH,
    AUDIO_CHANNEL_CONFIG_NON_PREDEFINED,
    0x00,
    0x00,
    0x00
  ),

  // Mic: → USB (Output Terminal)
  TUD_AUDIO_DESC_OUTPUT_TERM(
    AUDIO_MIC_OT_ID,
    AUDIO_TERM_TYPE_USB_STREAMING,
    AUDIO_MIC_IT_ID,
    AUDIO_MIC_IT_ID,
    AUDIO_CLK_ID,
    0x0000,
    0x00
  ),

  // -------------------------------------------------------------------------
  // Interface 1: Audio Streaming — Speaker (OUT, 1ch 16-bit)
  // -------------------------------------------------------------------------
  // Alt 0: zero bandwidth
  TUD_AUDIO_DESC_STD_AS_INT(ITF_NUM_AUDIO_SPK, 0x00, 0x00, 0x00),

  // Alt 1: active
  TUD_AUDIO_DESC_STD_AS_INT(ITF_NUM_AUDIO_SPK, 0x01, 0x01, 0x00),

  TUD_AUDIO_DESC_CS_AS_INT(
    AUDIO_SPK_IT_ID,
    AUDIO_CTRL_NONE,
    AUDIO_FORMAT_TYPE_I,
    AUDIO_DATA_FORMAT_TYPE_I_PCM,
    AUDIO_SPK_CH,
    AUDIO_CHANNEL_CONFIG_NON_PREDEFINED,
    0x00
  ),

  TUD_AUDIO_DESC_TYPE_I_FORMAT(AUDIO_SPK_BYTES, AUDIO_SPK_BITS),

  TUD_AUDIO_DESC_STD_AS_ISO_EP(
    EPNUM_AUDIO_SPK,
    (uint8_t)((uint8_t)TUSB_XFER_ISOCHRONOUS |
              (uint8_t)TUSB_ISO_EP_ATT_ASYNCHRONOUS |
              (uint8_t)TUSB_ISO_EP_ATT_DATA),
    AUDIO_EP_SPK_SZ,
    0x01
  ),

  TUD_AUDIO_DESC_CS_AS_ISO_EP(
    AUDIO_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK,
    AUDIO_CTRL_NONE,
    AUDIO_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_UNDEFINED,
    0x0000
  ),

  // -------------------------------------------------------------------------
  // Interface 2: Audio Streaming — Mic (IN, 1ch 24-bit)
  // -------------------------------------------------------------------------
  // Alt 0: zero bandwidth
  TUD_AUDIO_DESC_STD_AS_INT(ITF_NUM_AUDIO_MIC, 0x00, 0x00, 0x00),

  // Alt 1: active
  TUD_AUDIO_DESC_STD_AS_INT(ITF_NUM_AUDIO_MIC, 0x01, 0x01, 0x00),

  TUD_AUDIO_DESC_CS_AS_INT(
    AUDIO_MIC_OT_ID,
    AUDIO_CTRL_NONE,
    AUDIO_FORMAT_TYPE_I,
    AUDIO_DATA_FORMAT_TYPE_I_PCM,
    AUDIO_MIC_CH,
    AUDIO_CHANNEL_CONFIG_NON_PREDEFINED,
    0x00
  ),

  TUD_AUDIO_DESC_TYPE_I_FORMAT(AUDIO_MIC_BYTES, AUDIO_MIC_BITS),

  TUD_AUDIO_DESC_STD_AS_ISO_EP(
    (uint8_t)(EPNUM_AUDIO_MIC | 0x80),
    (uint8_t)((uint8_t)TUSB_XFER_ISOCHRONOUS |
              (uint8_t)TUSB_ISO_EP_ATT_ASYNCHRONOUS |
              (uint8_t)TUSB_ISO_EP_ATT_DATA),
    AUDIO_EP_MIC_SZ,
    0x01
  ),

  TUD_AUDIO_DESC_CS_AS_ISO_EP(
    AUDIO_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK,
    AUDIO_CTRL_NONE,
    AUDIO_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_UNDEFINED,
    0x0000
  ),
};

uint8_t const* tud_descriptor_configuration_cb(uint8_t index)
{
  (void)index;
  return desc_configuration;
}

// ----------------------------------------------------------------------------
// String descriptors
// ----------------------------------------------------------------------------
enum {
  STRID_LANGID = 0,
  STRID_MANUFACTURER,
  STRID_PRODUCT,
  STRID_SERIAL,
};

static char const* string_desc_arr[] = {
  (const char[]){ 0x09, 0x04 },
  "kehribar.me",
  "Cordee Headset",
  NULL,
};

static uint16_t _desc_str[32 + 1];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  (void)langid;
  size_t chr_count;

  if(index == STRID_LANGID)
  {
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  }
  else if(index == STRID_SERIAL)
  {
    static const char hex[] = "0123456789ABCDEF";
    pico_unique_board_id_t bid;
    pico_get_unique_board_id(&bid);
    chr_count = (2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES);
    for(size_t i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; i++)
    {
      _desc_str[1 + (2 * i) + 0] = hex[(bid.id[i] >> 4) & 0xF];
      _desc_str[1 + (2 * i) + 1] = hex[(bid.id[i] >> 0) & 0xF];
    }
  }
  else
  {
    if(index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))
    {
      return NULL;
    }

    const char* str = string_desc_arr[index];
    chr_count = strlen(str);
    size_t const max_count = (sizeof(_desc_str) / sizeof(_desc_str[0])) - 1;
    if(chr_count > max_count)
    {
      chr_count = max_count;
    }

    for(size_t i = 0; i < chr_count; i++)
    {
      _desc_str[1 + i] = str[i];
    }
  }

  _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | ((2 * chr_count) + 2));
  return _desc_str;
}
