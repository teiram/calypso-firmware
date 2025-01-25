#include "PS3USBHIDDriver.h"
#include "calypso-debug.h"
#include "tusb.h"

static const uint8_t ps3_hid_report[] = {
    0x05, 0x01,         /*  Usage Page (Desktop),               */
	0x09, 0x04,         /*  Usage (Joystik),                    */
	0xA1, 0x01,         /*  Collection (Application),           */
	0xA1, 0x02,         /*      Collection (Logical),           */
	0x85, 0x01,         /*          Report ID (1),              */
	0x75, 0x08,         /*          Report Size (8),            */
	0x95, 0x01,         /*          Report Count (1),           */
	0x15, 0x00,         /*          Logical Minimum (0),        */
	0x26, 0xFF, 0x00,   /*          Logical Maximum (255),      */
	0x81, 0x03,         /*          Input (Constant, Variable), */
	0x75, 0x01,         /*          Report Size (1),            */
	0x95, 0x13,         /*          Report Count (19),          */
	0x15, 0x00,         /*          Logical Minimum (0),        */
	0x25, 0x01,         /*          Logical Maximum (1),        */
	0x35, 0x00,         /*          Physical Minimum (0),       */
	0x45, 0x01,         /*          Physical Maximum (1),       */
	0x05, 0x09,         /*          Usage Page (Button),        */
	0x19, 0x01,         /*          Usage Minimum (01h),        */
	0x29, 0x13,         /*          Usage Maximum (13h),        */
	0x81, 0x02,         /*          Input (Variable),           */
	0x75, 0x01,         /*          Report Size (1),            */
	0x95, 0x0D,         /*          Report Count (13),          */
	0x06, 0x00, 0xFF,   /*          Usage Page (FF00h),         */
	0x81, 0x03,         /*          Input (Constant, Variable), */
	0x15, 0x00,         /*          Logical Minimum (0),        */
	0x26, 0xFF, 0x00,   /*          Logical Maximum (255),      */
	0x05, 0x01,         /*          Usage Page (Desktop),       */
	0x09, 0x01,         /*          Usage (Pointer),            */
	0xA1, 0x00,         /*          Collection (Physical),      */
	0x75, 0x08,         /*              Report Size (8),        */
	0x95, 0x04,         /*              Report Count (4),       */
	0x35, 0x00,         /*              Physical Minimum (0),   */
	0x46, 0xFF, 0x00,   /*              Physical Maximum (255), */
	0x09, 0x30,         /*              Usage (X),              */
	0x09, 0x31,         /*              Usage (Y),              */
	0x09, 0x32,         /*              Usage (Z),              */
	0x09, 0x35,         /*              Usage (Rz),             */
	0x81, 0x02,         /*              Input (Variable),       */
	0xC0,               /*          End Collection,             */
	0x05, 0x01,         /*          Usage Page (Desktop),       */
	0x95, 0x13,         /*          Report Count (19),          */
	0x09, 0x01,         /*          Usage (Pointer),            */
	0x81, 0x02,         /*          Input (Variable),           */
	0x95, 0x0C,         /*          Report Count (12),          */
	0x81, 0x01,         /*          Input (Constant),           */
	0x75, 0x10,         /*          Report Size (16),           */
	0x95, 0x04,         /*          Report Count (4),           */
	0x26, 0xFF, 0x03,   /*          Logical Maximum (1023),     */
	0x46, 0xFF, 0x03,   /*          Physical Maximum (1023),    */
	0x09, 0x01,         /*          Usage (Pointer),            */
	0x81, 0x02,         /*          Input (Variable),           */
	0xC0,               /*      End Collection,                 */
	0xA1, 0x02,         /*      Collection (Logical),           */
	0x85, 0x02,         /*          Report ID (2),              */
	0x75, 0x08,         /*          Report Size (8),            */
	0x95, 0x30,         /*          Report Count (48),          */
	0x09, 0x01,         /*          Usage (Pointer),            */
	0xB1, 0x02,         /*          Feature (Variable),         */
	0xC0,               /*      End Collection,                 */
	0xA1, 0x02,         /*      Collection (Logical),           */
	0x85, 0xEE,         /*          Report ID (238),            */
	0x75, 0x08,         /*          Report Size (8),            */
	0x95, 0x30,         /*          Report Count (48),          */
	0x09, 0x01,         /*          Usage (Pointer),            */
	0xB1, 0x02,         /*          Feature (Variable),         */
	0xC0,               /*      End Collection,                 */
	0xA1, 0x02,         /*      Collection (Logical),           */
	0x85, 0xEF,         /*          Report ID (239),            */
	0x75, 0x08,         /*          Report Size (8),            */
	0x95, 0x30,         /*          Report Count (48),          */
	0x09, 0x01,         /*          Usage (Pointer),            */
	0xB1, 0x02,         /*          Feature (Variable),         */
	0xC0,               /*      End Collection,                 */
	0xC0                /*  End Collection                      */
}; 

using namespace calypso;

PS3USBHIDDriver::PS3USBHIDDriver(uint8_t* report, uint16_t reportLength):
    m_report(report),
    m_reportLength(reportLength) {}

bool PS3USBHIDDriver::initialize(uint8_t deviceAddress, uint8_t instance) {
    uint8_t report[18];
    if (!tuh_hid_get_report(deviceAddress, instance, 0xf2, 3, report, 17)) {
        USB_DEBUG_LOG(L_WARN, "Unable to request PS3 controller enabling report\n");
        return false;
    }
    return true;
}

std::pair<uint8_t*, uint16_t> PS3USBHIDDriver::customReport() {
    return std::make_pair(m_report, m_reportLength);
}

bool PS3USBHIDDriver::hasCustomReport() {
	return m_reportLength > 0;
}

PS3USBHIDDriver ps3Driver((uint8_t *) ps3_hid_report, sizeof(ps3_hid_report));