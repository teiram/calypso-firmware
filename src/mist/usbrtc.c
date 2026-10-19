// usbrtc.c - USB RTC for the Calypso firmware (RP2040 + TinyUSB host)
// Author: Shaeon (Carlos Palmero)
//
// Implements, on top of TinyUSB host and asynchronously, the same protocol as
// the MiST firmware driver usb/rtc/i2c-tiny.c: an i2c-tiny-usb adapter
// (VID/PID 0403:C631, class 0xFF) with a DS3231 or DS1307 RTC at I2C
// address 0x68.
//
// The MiST code calls GetRTC()/SetRTC() synchronously (OSD menu, user_io
// every second). To avoid blocking the main loop or re-entering tuh_task(),
// everything here is asynchronous and driven by the callers and the stack:
//   - The driver is registered in TinyUSB as an application class driver
//     (usbh_app_driver_get_cb): the stack hands it the vendor interface of
//     the adapter on enumeration and notifies it on disconnection.
//   - Once configured, the driver starts the probe of the RTC.
//   - usbrtc_get_time() returns the cached time and, if the cache is older
//     than RTC_REFRESH_MS, starts a new read in the background.
//   - usbrtc_set_time() updates the cache immediately and starts the write.
//   - Transfer completion callbacks (run from tuh_task) chain the steps.
// No periodic hook in the main loop is needed.

#include <string.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "host/usbh_pvt.h"
#include "usbrtc.h"

#define USBRTC_VID        0x0403
#define USBRTC_PID        0xC631
#define RTC_I2C_ADDR      0x68

#define RTC_REFRESH_MS    500     // how often the time is read
#define RTC_PROBE_RETRY   2000    // probe retry when the RTC does not answer
#define RTC_XFER_TIMEOUT  1000    // TinyUSB has no timeout for control transfers

// i2c-tiny-usb requests
#define CMD_GET_STATUS    3
#define CMD_I2C_IO        4
#define CMD_I2C_BEGIN     1
#define CMD_I2C_END       2
#define I2C_M_RD          0x01
#define STATUS_ADDR_ACK   1

#define REQ_OUT  (TUSB_REQ_TYPE_VENDOR << 5 | TUSB_REQ_RCPT_DEVICE)            // 0x40
#define REQ_IN   (TUSB_DIR_IN_MASK | TUSB_REQ_TYPE_VENDOR << 5 | TUSB_REQ_RCPT_DEVICE) // 0xC0

#define usbrtc_debugf(...)  printf("USBRTC: " __VA_ARGS__)

// MiST ctime_t indexes
enum { T_YEAR = 0, T_MONTH, T_DAY, T_HOUR, T_MIN, T_SEC, T_WDAY };

// --- An operation is a list of I2C_IO requests, each followed by GET_STATUS
typedef struct {
  uint8_t  bmRequestType;
  uint8_t  bRequest;
  uint16_t wValue;
  uint16_t wLength;
  uint8_t *buf;
} step_t;

typedef enum { OP_NONE = 0, OP_PROBE, OP_READ, OP_WRITE } op_t;

static struct {
  uint8_t  daddr;          // 0 = no device
  bool     present;        // the RTC answered at 0x68
  bool     cache_valid;
  uint8_t  cache[7];       // ctime_t
  uint32_t cache_ms;       // time of the last good read
  uint32_t next_ms;        // next scheduled operation

  bool     write_pending;
  uint32_t set_gen;        // incremented on every SetRTC

  // operation in progress
  op_t     op;
  uint32_t op_gen;
  uint32_t op_start_ms;
  const step_t *steps;
  uint8_t  nsteps;
  uint8_t  step;
  bool     in_status;      // GET_STATUS phase of the current request
} rtc;

static tusb_control_request_t setup_pkt;
static CFG_TUSB_MEM_ALIGN uint8_t xfer_buf[8];
static uint8_t reg_ptr[1]  = { 0x00 };
static uint8_t time_raw[7];
static uint8_t write_raw[8];
static uint8_t status_byte;

static const step_t probe_steps[] = {
  // 0-byte probe, sent as an OUT request so the status stage is standard
  { REQ_OUT, CMD_I2C_IO + CMD_I2C_BEGIN + CMD_I2C_END, 0, 0, NULL },
};
static const step_t read_steps[] = {
  { REQ_OUT, CMD_I2C_IO + CMD_I2C_BEGIN, 0,        1, reg_ptr  },  // pointer = 0, no STOP
  { REQ_IN,  CMD_I2C_IO + CMD_I2C_END,   I2C_M_RD, 7, time_raw },  // repeated start + 7 bytes
};
static const step_t write_steps[] = {
  { REQ_OUT, CMD_I2C_IO + CMD_I2C_BEGIN + CMD_I2C_END, 0, 8, write_raw },
};

static uint32_t now_ms(void) { return to_ms_since_boot(get_absolute_time()); }
static uint8_t bcd2bin(uint8_t v) { return (v >> 4) * 10 + (v & 0x0F); }
static uint8_t bin2bcd(uint8_t v) { return ((v / 10) << 4) | (v % 10); }

static void xfer_cb(tuh_xfer_t *xfer);

static bool submit(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue,
                   uint16_t wLength, uint8_t *data) {
  setup_pkt.bmRequestType = bmRequestType;
  setup_pkt.bRequest      = bRequest;
  setup_pkt.wValue        = wValue;
  setup_pkt.wIndex        = RTC_I2C_ADDR;
  setup_pkt.wLength       = wLength;

  if (wLength && !(bmRequestType & TUSB_DIR_IN_MASK)) memcpy(xfer_buf, data, wLength);

  tuh_xfer_t xfer = {
    .daddr       = rtc.daddr,
    .ep_addr     = 0,
    .setup       = &setup_pkt,
    .buffer      = wLength ? xfer_buf : NULL,
    .complete_cb = xfer_cb,
    .user_data   = 0
  };
  return tuh_control_xfer(&xfer);
}

static bool submit_current(void) {
  if (rtc.in_status)
    return submit(REQ_IN, CMD_GET_STATUS, 0, 1, &status_byte);
  const step_t *s = &rtc.steps[rtc.step];
  return submit(s->bmRequestType, s->bRequest, s->wValue, s->wLength, s->buf);
}

static bool start_op(op_t op) {
  switch (op) {
    case OP_PROBE: rtc.steps = probe_steps; rtc.nsteps = 1; break;
    case OP_READ:  rtc.steps = read_steps;  rtc.nsteps = 2; break;
    case OP_WRITE: rtc.steps = write_steps; rtc.nsteps = 1; break;
    default: return false;
  }
  rtc.op = op;
  rtc.op_gen = rtc.set_gen;
  rtc.op_start_ms = now_ms();
  rtc.step = 0;
  rtc.in_status = false;
  if (!submit_current()) {        // control pipe busy (e.g. HID): retry later
    rtc.op = OP_NONE;
    return false;
  }
  return true;
}

static void op_done(bool ok) {
  op_t op = rtc.op;
  rtc.op = OP_NONE;
  uint32_t t = now_ms();

  switch (op) {
    case OP_PROBE:
      rtc.present = ok;
      if (ok) {
        usbrtc_debugf("RTC found at 0x%02X\n", RTC_I2C_ADDR);
        rtc.next_ms = t;                       // read the time now
        start_op(OP_READ);
      } else {
        usbrtc_debugf("no RTC answering at 0x%02X\n", RTC_I2C_ADDR);
        rtc.next_ms = t + RTC_PROBE_RETRY;
      }
      break;

    case OP_READ:
      if (ok && rtc.op_gen == rtc.set_gen) {   // drop reads started before a SetRTC
        rtc.cache[T_YEAR]  = bcd2bin(time_raw[6]) + 100;
        rtc.cache[T_MONTH] = bcd2bin(time_raw[5] & 0x1F);
        rtc.cache[T_DAY]   = bcd2bin(time_raw[4] & 0x3F);
        rtc.cache[T_HOUR]  = bcd2bin(time_raw[2] & 0x3F);
        rtc.cache[T_MIN]   = bcd2bin(time_raw[1] & 0x7F);
        rtc.cache[T_SEC]   = bcd2bin(time_raw[0] & 0x7F);
        rtc.cache[T_WDAY]  = time_raw[3] & 0x07;
        rtc.cache_valid = true;
        rtc.cache_ms = t;
      } else if (!ok) {
        rtc.cache_valid = false;
        rtc.present = false;                   // probe again
      }
      rtc.next_ms = t + RTC_REFRESH_MS;
      if (ok && rtc.write_pending) {           // a SetRTC arrived during the read
        rtc.write_pending = false;
        if (!start_op(OP_WRITE)) rtc.write_pending = true;
      }
      break;

    case OP_WRITE:
      if (!ok) rtc.write_pending = true;       // retry
      rtc.next_ms = t;                         // next poll: read back or retry
      break;

    default:
      break;
  }
}

static void xfer_cb(tuh_xfer_t *xfer) {
  if (rtc.op == OP_NONE || xfer->daddr != rtc.daddr) return;

  if (xfer->result != XFER_RESULT_SUCCESS) {
    op_done(false);
    return;
  }

  if (!rtc.in_status) {
    // I2C_IO request done: copy read data and ask for the status
    const step_t *s = &rtc.steps[rtc.step];
    if ((s->bmRequestType & TUSB_DIR_IN_MASK) && s->wLength) {
      if (xfer->actual_len != s->wLength) { op_done(false); return; }
      memcpy(s->buf, xfer_buf, s->wLength);
    }
    rtc.in_status = true;
  } else {
    // GET_STATUS done
    if (xfer->actual_len != 1 || xfer_buf[0] != STATUS_ADDR_ACK) {
      op_done(false);
      return;
    }
    rtc.in_status = false;
    if (++rtc.step >= rtc.nsteps) {
      op_done(true);
      return;
    }
  }

  if (!submit_current()) op_done(false);
}

static void usbrtc_poll(void) {
  if (!rtc.daddr) return;

  uint32_t t = now_ms();

  if (rtc.op != OP_NONE) {
    // TinyUSB has no control timeout: avoid getting stuck
    if (t - rtc.op_start_ms > RTC_XFER_TIMEOUT) {
      usbrtc_debugf("timeout\n");
      op_done(false);
    }
    return;
  }

  if ((int32_t)(t - rtc.next_ms) < 0) return;

  if (!rtc.present) {
    start_op(OP_PROBE);
  } else if (rtc.write_pending) {
    rtc.write_pending = false;
    if (!start_op(OP_WRITE)) rtc.write_pending = true;
  } else {
    start_op(OP_READ);
  }
}

bool usbrtc_get_time(uint8_t *d) {
  usbrtc_poll();                               // refresh in the background
  if (!rtc.daddr || !rtc.present || !rtc.cache_valid) return false;
  memcpy(d, rtc.cache, 7);
  return true;
}

bool usbrtc_set_time(const uint8_t *d) {
  if (!rtc.daddr || !rtc.present) return false;

  write_raw[0] = 0x00;                         // first register
  write_raw[1] = bin2bcd(d[T_SEC]);            // CH = 0 (DS1307: oscillator running)
  write_raw[2] = bin2bcd(d[T_MIN]);
  write_raw[3] = bin2bcd(d[T_HOUR]);           // bit 6 = 0 -> 24 h mode
  write_raw[4] = d[T_WDAY] & 0x07;
  write_raw[5] = bin2bcd(d[T_DAY]);
  write_raw[6] = bin2bcd(d[T_MONTH]);
  write_raw[7] = bin2bcd((uint8_t)(d[T_YEAR] - 100));

  // update the cache now (the menu reads the time back after every key)
  memcpy(rtc.cache, d, 7);
  rtc.cache_valid = true;
  rtc.cache_ms = now_ms();

  rtc.set_gen++;
  rtc.write_pending = true;
  rtc.next_ms = now_ms();
  usbrtc_poll();                               // start the write now if possible
  return true;
}

// --- TinyUSB application class driver ---------------------------------------
//
// The i2c-tiny-usb is a vendor specific device (class 0xFF) that only uses
// control requests on EP0, so there is no standard class driver for it. This
// registers our own driver in the TinyUSB host stack, the same way the MiST
// firmware registers usb/rtc/i2c-tiny.c in its class list.

static bool usbrtc_drv_init(void) {
  memset(&rtc, 0, sizeof(rtc));
  return true;
}

#if TUSB_VERSION_NUMBER >= 1700
static bool usbrtc_drv_deinit(void) {
  memset(&rtc, 0, sizeof(rtc));
  return true;
}
#endif

static bool usbrtc_drv_open(uint8_t rhport, uint8_t daddr,
                            tusb_desc_interface_t const *itf_desc, uint16_t max_len) {
  (void)rhport;
  (void)max_len;
  uint16_t vid, pid;

  if (itf_desc->bInterfaceClass != TUSB_CLASS_VENDOR_SPECIFIC) return false;
  if (!tuh_vid_pid_get(daddr, &vid, &pid)) return false;
  if (vid != USBRTC_VID || pid != USBRTC_PID) return false;
  if (rtc.daddr) return false;                 // only one RTC

  usbrtc_debugf("i2c-tiny-usb attached (USB address %u)\n", daddr);
  memset(&rtc, 0, sizeof(rtc));
  rtc.daddr = daddr;
  return true;                                 // no endpoints to open: EP0 only
}

static bool usbrtc_drv_set_config(uint8_t daddr, uint8_t itf_num) {
  usbh_driver_set_config_complete(daddr, itf_num);   // let enumeration finish
  rtc.next_ms = now_ms();
  usbrtc_poll();                               // start the probe
  return true;
}

static bool usbrtc_drv_xfer_cb(uint8_t daddr, uint8_t ep_addr,
                               xfer_result_t result, uint32_t xferred_bytes) {
  // Not used: there are no data endpoints, control transfers use their own
  // completion callback (xfer_cb above)
  (void)daddr; (void)ep_addr; (void)result; (void)xferred_bytes;
  return true;
}

static void usbrtc_drv_close(uint8_t daddr) {
  if (daddr != rtc.daddr) return;
  usbrtc_debugf("i2c-tiny-usb detached\n");
  memset(&rtc, 0, sizeof(rtc));
}

static usbh_class_driver_t const usbrtc_driver[] = {{
  .name       = "USBRTC",
  .init       = usbrtc_drv_init,
#if TUSB_VERSION_NUMBER >= 1700
  .deinit     = usbrtc_drv_deinit,
#endif
  .open       = usbrtc_drv_open,
  .set_config = usbrtc_drv_set_config,
  .xfer_cb    = usbrtc_drv_xfer_cb,
  .close      = usbrtc_drv_close,
}};

usbh_class_driver_t const *usbh_app_driver_get_cb(uint8_t *driver_count) {
  *driver_count = 1;
  return usbrtc_driver;
}
