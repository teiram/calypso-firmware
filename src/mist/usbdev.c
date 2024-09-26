#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "debug.h"
#include "utils.h"
#include "usbdev.h"
#include "hardware.h"
#include "mist_cfg.h"

#include "usb.h"
#include "tusb.h"

//TODO: Needs analysis and adaptations. Try to get rid of it


#define WORD(a) (a)&0xff, ((a)>>8)&0xff

#define debug(a) printf a


void usb_dev_open(void) {}
void usb_dev_reconnect(void) {}

uint8_t  usb_cdc_is_configured(void) { return 0; }
uint16_t usb_cdc_write(const char *pData, uint16_t length) { return 0; }
uint16_t usb_cdc_read(char *pData, uint16_t length) { return 0; }

#define MAX_USB   4

usb_device_t device[MAX_USB];

struct {
  uint8_t *last_report;
  uint16_t report_size;
  uint8_t *desc_stored;
  uint16_t desc_len;
} report[MAX_USB] = {
  {0,0}
};


//uint8_t tuh_descriptor_get_device_sync(uint8_t dev_addr, uint8_t *dd, uint16_t len);

void usb_attached(uint8_t dev, uint8_t idx, uint16_t vid, uint16_t pid, uint8_t *desc, uint16_t desclen) {
  usb_device_descriptor_t dd;
  uint8_t r;
  tuh_descriptor_get_device_sync(dev, (uint8_t *) &dd, sizeof dd);
  report[dev].desc_stored = desc;
  report[dev].desc_len = desclen;
  report[dev].report_size = 0;
  device[dev].vid = vid;
  device[dev].pid = pid;
  r = usb_hid_class.init(&device[dev], &dd);
}

void usb_detached(uint8_t dev) {
  if (report[dev].report_size) {
    free(report[dev].last_report);
    report[dev].report_size = 0;
    usb_hid_class.release(&device[dev]);
  }
}

void usb_handle_data(uint8_t dev, uint8_t *desc, uint16_t desclen) {
  if (report[dev].report_size != desclen) {
    if (report[dev].report_size) free(report[dev].last_report);
    if (desclen) report[dev].last_report = (uint8_t *)malloc(desclen);
    report[dev].report_size = desclen;
  }
  memcpy(report[dev].last_report, desc, desclen);
  usb_hid_class.poll(&device[dev]);
}


uint8_t usb_ctrl_req( usb_device_t *dev, uint8_t bmReqType,
                      uint8_t bRequest, uint8_t wValLo, uint8_t wValHi,
                      uint16_t wInd, uint16_t nbytes, uint8_t* dataptr) {
  debug(("usb_ctrl_req: dev %08x reqt %02X req %02X wValLo %02X wValHi %02X wInd %04X nbytes %04X dataptr %08x\n",
         dev, bmReqType, bRequest, wValLo, wValHi, wInd, nbytes, dataptr));

  if (bmReqType == HID_REQ_HIDREPORT && bRequest == USB_REQUEST_GET_DESCRIPTOR && wValLo == 0 && wValHi == HID_DESCRIPTOR_REPORT) {
    uint8_t n = dev - device;
    memcpy(dataptr, report[n].desc_stored, lowest(report[n].desc_len, nbytes));
  }

  return 0;
}

uint8_t usb_in_transfer( usb_device_t *dev, ep_t *ep, uint16_t *nbytesptr, uint8_t* data) {
  debug(("usb_in_transfer: dev %08x, ep %08x, ptr %08X data %08X\n", dev, ep, nbytesptr, data));
  uint8_t n = dev - device;
  if (report[n].report_size) {
    uint16_t this_copy = lowest(report[n].report_size, *nbytesptr);
    memcpy(data, report[n].last_report, this_copy);
    *nbytesptr = this_copy;
  } else {
    *nbytesptr = 0;
  }
  return 0;
}

uint8_t usb_out_transfer( usb_device_t *dev, ep_t *ep, uint16_t nbytes, const uint8_t* data ) {
  debug(("usb_out_transfer: dev %08x, ep %08x, nbytes %04X data %08X\n", dev, ep, nbytes, data));
  return 0;
}

usb_device_t *usb_get_devices() {
  debug(("usb_get_devices\n"));
  return device;
}

uint8_t usb_get_conf_descr( usb_device_t *dev, uint16_t nbytes, uint8_t conf, usb_configuration_descriptor_t* dataptr ) {
  debug(("usb_get_conf_descr: dev %08x, nbytes %04X conf %02X data %08X\n", dev, nbytes, conf, dataptr));
  tuh_descriptor_get_configuration_sync(dev - device, conf, dataptr, nbytes);
  return 0;
}

uint8_t usb_set_conf( usb_device_t *dev, uint8_t conf_value ) {
  debug(("usb_set_conf: dev %08x conf %02X\n", dev, conf_value));
  uint8_t dev_addr = dev - device;
  tuh_configuration_set(dev_addr, conf_value, NULL, 0);
  return 0;
}
