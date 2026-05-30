// ----------------------------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include "usb_audio.h"
#include "util.h"

// ----------------------------------------------------------------------------
// Speaker ring buffer (USB → DAC). Stores 16-bit speaker samples sign-
// extended into the low 16 bits of uint32_t slots.
// ----------------------------------------------------------------------------
#define SPK_DATA_LEN (2048)
static uint32_t spkData[SPK_DATA_LEN];
static volatile uint32_t spkDataHead = 0;
static volatile uint32_t spkDataTail = 0;

// ----------------------------------------------------------------------------
static inline bool spkFifoIsFull(void)
{
  return (((spkDataHead + 1) & (SPK_DATA_LEN - 1)) == spkDataTail);
}

// ----------------------------------------------------------------------------
uint32_t usb_audio_spkFifoLevel(void)
{
  return ((spkDataHead - spkDataTail) & (SPK_DATA_LEN - 1));
}

// ----------------------------------------------------------------------------
// Audio control state
// ----------------------------------------------------------------------------
static uint32_t sampFreq = AUDIO_SAMPLE_RATE;
static uint8_t  clkValid = 1;
static volatile bool mic_streaming_active = false;

// ----------------------------------------------------------------------------
bool usb_audio_write_mic(const int32_t* mic_samples, int32_t n_frames)
{
  if((tud_mounted() == false) || (mic_streaming_active == false))
  {
    return false;
  }

  // Pack 24-bit little-endian, one sample per frame (mono).
  uint8_t buf[FRAMES_PER_BUFFER * AUDIO_MIC_BYTES * AUDIO_MIC_CH];
  uint8_t* ptr = buf;
  for(int32_t i = 0; i < n_frames; i++)
  {
    const int32_t s = mic_samples[i];
    *ptr++ = (uint8_t)((s >> 8)  & 0xFF);
    *ptr++ = (uint8_t)((s >> 16) & 0xFF);
    *ptr++ = (uint8_t)((s >> 24) & 0xFF);
  }
  tud_audio_write(buf, (uint16_t)(n_frames * AUDIO_MIC_BYTES * AUDIO_MIC_CH));
  return true;
}

// ----------------------------------------------------------------------------
int32_t usb_audio_read_spk(int16_t* out, int32_t n_frames)
{
  uint32_t tail = spkDataTail;
  int32_t i;
  for(i = 0; i < n_frames; i++)
  {
    if(spkDataHead != tail)
    {
      out[i] = (int16_t)spkData[tail];
      tail = ((tail + 1) & (SPK_DATA_LEN - 1));
    }
    else
    {
      out[i] = 0;
    }
  }
  spkDataTail = tail;
  return i;
}

// (feedback EP disabled for debug)

// ----------------------------------------------------------------------------
void tud_mount_cb(void) {}
void tud_umount_cb(void) {}
void tud_suspend_cb(bool remote_wakeup_en) { (void)remote_wakeup_en; }
void tud_resume_cb(void) {}

// ----------------------------------------------------------------------------
// Speaker data received from host → push to spkData ring buffer
// ----------------------------------------------------------------------------
bool tud_audio_rx_done_pre_read_cb(
  uint8_t rhport,
  uint16_t n_bytes_received,
  uint8_t func_id,
  uint8_t ep_out,
  uint8_t cur_alt_setting
)
{
  (void)rhport;
  (void)func_id;
  (void)ep_out;

  if(cur_alt_setting == 0)
  {
    return true;
  }

  uint8_t buf[AUDIO_EP_SPK_SZ];
  const uint16_t count = tud_audio_read(buf, n_bytes_received);
  const int16_t* samples = (const int16_t*)buf;
  const int32_t n_samples = count / 2;

  for(int32_t i = 0; i < n_samples; i++)
  {
    if(spkFifoIsFull() == false)
    {
      spkData[spkDataHead++] = (uint32_t)(uint16_t)samples[i];
      spkDataHead &= (SPK_DATA_LEN - 1);
    }
  }

  return true;
}

// ----------------------------------------------------------------------------
bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const* p_request)
{
  const uint8_t ctrlSel  = TU_U16_HIGH(p_request->wValue);
  const uint8_t entityID = TU_U16_HIGH(p_request->wIndex);

  if(entityID == AUDIO_CLK_ID)
  {
    if(ctrlSel == AUDIO_CS_CTRL_SAM_FREQ)
    {
      if(p_request->bRequest == AUDIO_CS_REQ_CUR)
      {
        static uint32_t cur_freq;
        cur_freq = AUDIO_SAMPLE_RATE;
        return tud_control_xfer(rhport, p_request, &cur_freq, sizeof(cur_freq));
      }
      if(p_request->bRequest == AUDIO_CS_REQ_RANGE)
      {
        static struct __attribute__((packed))
        {
          uint16_t wNumSubRanges;
          int32_t bMin;
          int32_t bMax;
          uint32_t bRes;
        } range;
        range.wNumSubRanges = 1;
        range.bMin = AUDIO_SAMPLE_RATE;
        range.bMax = AUDIO_SAMPLE_RATE;
        range.bRes = 0;
        return tud_control_xfer(rhport, p_request, &range, sizeof(range));
      }
    }
    if(ctrlSel == AUDIO_CS_CTRL_CLK_VALID)
    {
      return tud_control_xfer(rhport, p_request, &clkValid, sizeof(clkValid));
    }
  }

  return false;
}

// ----------------------------------------------------------------------------
bool tud_audio_set_req_entity_cb(
  uint8_t rhport,
  tusb_control_request_t const* p_request,
  uint8_t* pBuff
)
{
  (void)rhport;
  (void)p_request;
  (void)pBuff;
  return false;
}

// ----------------------------------------------------------------------------
bool tud_audio_set_req_ep_cb(uint8_t rhport, tusb_control_request_t const* p_request, uint8_t* pBuff)
{
  (void)rhport; (void)p_request; (void)pBuff;
  return false;
}

bool tud_audio_set_req_itf_cb(uint8_t rhport, tusb_control_request_t const* p_request, uint8_t* pBuff)
{
  (void)rhport; (void)p_request; (void)pBuff;
  return false;
}

bool tud_audio_get_req_ep_cb(uint8_t rhport, tusb_control_request_t const* p_request)
{
  (void)rhport; (void)p_request;
  return false;
}

bool tud_audio_get_req_itf_cb(uint8_t rhport, tusb_control_request_t const* p_request)
{
  (void)rhport; (void)p_request;
  return false;
}

// ----------------------------------------------------------------------------
bool tud_audio_set_itf_close_EP_cb(uint8_t rhport, tusb_control_request_t const* p_request)
{
  (void)rhport;
  const uint8_t itf = TU_U16_LOW(p_request->wIndex);
  if(itf == ITF_NUM_AUDIO_MIC)
  {
    mic_streaming_active = false;
    usb_dpram->ep_buf_ctrl[EPNUM_AUDIO_MIC].in = 0;
  }
  else if(itf == ITF_NUM_AUDIO_SPK)
  {
    usb_dpram->ep_buf_ctrl[EPNUM_AUDIO_SPK].out = 0;
  }
  return true;
}

// ----------------------------------------------------------------------------
bool tud_audio_tx_done_pre_load_cb(
  uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting
)
{
  (void)rhport; (void)itf; (void)ep_in; (void)cur_alt_setting;
  mic_streaming_active = true;
  return true;
}

// ----------------------------------------------------------------------------
bool tud_audio_tx_done_post_load_cb(
  uint8_t rhport, uint16_t n_bytes_copied, uint8_t itf,
  uint8_t ep_in, uint8_t cur_alt_setting
)
{
  (void)rhport; (void)n_bytes_copied; (void)itf;
  (void)ep_in; (void)cur_alt_setting;
  return true;
}
