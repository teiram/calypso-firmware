#include "compat.h"
#include <stdio.h>
#include "pico/time.h"
#include "pico/bootrom.h"
#include "mist-firmware/errors.h"
#include "hardware.h"
#include "buttons.h"
#include "mist-firmware/user_io.h"
#include "mist-firmware/data_io.h"
#include "mist-firmware/fat_compat.h"
#include "mmc.h"
#include "db9.h"
#include "spi.h"
#include "ps2keyboard.h"
#include "ps2mouse.h"
#include "mist-firmware/FatFs/diskio.h"
#include "mist-firmware/arc_file.h"
#include "mist-firmware/font.h"
#include "mist-firmware/tos.h"
#include "mist-firmware/fdd.h"
#include "mist-firmware/hdd.h"
#include "mist-firmware/menu.h"
#include "mist-firmware/fpga.h"
#include "mist-firmware/config.h"
#include "mist-firmware/cdc_control.h"
#include "mist-firmware/c64files.h"
#include "hardware/watchdog.h"
#include "joystick.h"
#include "ace_processor.h"

const char version[] = {"$VER:CLP-" VFIRM};

unsigned char Error;
char s[FF_LFN_BUF + 1];

unsigned long storage_size = 0;
uint8_t legacy_mode = DEFAULT_MODE;

void storage_control_poll() {}

void USART_Init(unsigned long baudrate) {}
void USART_Write(unsigned char c) {}
unsigned char USART_Read(void) {return 0;}
void USART_Poll(void) {}

uint8_t *asix_get_mac(void) {
    return NULL;
}

int8_t pl2303_present(void) {
    return 0;
}
void pl2303_settings(uint32_t rate, uint8_t bits, uint8_t parity, uint8_t stop) {}
void pl2303_tx(uint8_t *data, uint8_t len) {}
void pl2303_tx_byte(uint8_t byte) {}
uint8_t pl2303_rx_available(void) {
    return 0;
}
uint8_t pl2303_rx(void) {
    return 0;
}
int8_t pl2303_is_blocked(void) {
    return 0;
}
uint8_t get_pl2303s(void) {
    return 0;
}

char *GetFirmwareVersion(char *name) {
    return NULL;
}

unsigned char CheckFirmware(char *name) {
  printf("CheckFirmware: name:%s\n", name);
  // returns 1 if all ok else 0, setting Error to one of ollowing states
  // ERROR_NONE
  // ERROR_FILE_NOT_FOUND
  // ERROR_INVALID_DATA
  return ERROR_NONE;
}

// This is handled in our mist service
void usb_init() {}
void usb_poll() {}

// Maybe we can implement this, but what for?
void MCUReset() {
    watchdog_reboot(0, 0, 0);
}

void InitADC() {}
void PollADC() {}

void WriteFirmware(char *name) {
    printf("WriteFirmware: name:%s\n", name);
    reset_usb_boot(0, 0);
}

//--------------------------------------------------------------------
// Timer related support
//--------------------------------------------------------------------
void Timer_Init(void) {}

unsigned long GetTimer(unsigned long offset) {
  return (time_us_64() / 1000) + offset;
}

unsigned long CheckTimer(unsigned long time) {
  return (time_us_64() / 1000) >= time;
}

void WaitTimer(unsigned long time) {
    time = GetTimer(time);
    while (!CheckTimer(time)) {
        tight_loop_contents();
    }
}

char GetRTC(unsigned char *d) {
    // implemented as d[0-7] -
    //   [y-100] [m] [d] [H] [M] [S] [Day1-7]
    d[0] = 23;
    d[1] = 12;
    d[2] = 12;
    d[3] = 23;
    d[4] = 58;
    d[5] = 34;
    d[6] = 4;
    return 0;
}

char SetRTC(unsigned char *d) {
    return 0;
}

void InitRTTC() {}

int GetRTTC() {
    return time_us_64() / 1000;
}

//-------------------------------------------------------------
// Buttons support
//-------------------------------------------------------------
unsigned long CheckButton(void) {
    return MenuButton();
}

unsigned char Buttons() {
    return MenuButton();
}

unsigned char UserButton() {
    return 0;
}

void EnableFpgaMinimig() {
    EnableFpga();
}

bool eth_present = 0;



void FatalError(unsigned long error) {
    printf("Fatal error: %lu\r", error);
    sleep_ms(2000);
    reset_usb_boot(0, 0);
}

void HandleFpga(void) {
    unsigned char  c1, c2;
  
    EnableFpga();
    c1 = SPI(0); // cmd request and drive number
    c2 = SPI(0); // track number
    SPI(0);
    SPI(0);
    SPI(0);
    SPI(0);
    DisableFpga();
  
    HandleFDD(c1, c2);
    HandleHDD(c1, c2, 1);
  
    UpdateDriveStatus();
}

void set_legacy_mode(uint8_t mode) {
    if (mode != legacy_mode) {
        printf("Setting legacy mode to %d\n", mode);
        DB9SetLegacy(mode == LEGACY_MODE);
    }
  legacy_mode = mode;
}


#ifdef USB_STORAGE
int GetUSBStorageDevices()
{
  uint32_t to = GetTimer(4000);

  // poll usb 2 seconds or until a mass storage device becomes ready
  while(!storage_devices && !CheckTimer(to)) {
    usb_poll();
    if (storage_devices) {
      usb_poll();
    }
  }

  return storage_devices;
}
#endif

int mist_init() {
    uint8_t mmc_ok = 0;

    DISKLED_ON;


    
    Timer_Init();
    USART_Init(115200);

    iprintf("Minimig by Dennis van Weeren\n");
    iprintf("ARM Controller by Jakub Bednarski\n");
    iprintf("Calypso integration (rp2040) by Manu Teira\n");
    iprintf("Version %s\n", version+5);

    usb_init();

    if(MMC_Init()) mmc_ok = 1;
    else           spi_fast();

#ifdef MMC_AS_USB
    mmc_ok = 0;
#endif

    InitDB9();
    InitADC();

#ifdef USB_STORAGE
    if(UserButton()) USB_BOOT_VAR = (USB_BOOT_VAR == USB_BOOT_VALUE) ? 0 : USB_BOOT_VALUE;

    if(USB_BOOT_VAR == USB_BOOT_VALUE)
      if (!GetUSBStorageDevices()) {
        if(!mmc_ok)
          FatalError(ERROR_FILE_NOT_FOUND);
      } else
        fat_switch_to_usb();  // redirect file io to usb
    else {
#endif
      if(!mmc_ok) {
#ifdef USB_STORAGE
        if(!GetUSBStorageDevices()) {
#ifdef BOOT_FLASH_ON_ERROR
          BootFromFlash();
          return 0;
#else
          FatalError(ERROR_FILE_NOT_FOUND);
#endif
        }

        fat_switch_to_usb();  // redirect file io to usb
#else
        // no file to boot
#ifdef BOOT_FLASH_ON_ERROR
        BootFromFlash();
        return 0;
#else
        FatalError(ERROR_FILE_NOT_FOUND);
#endif
#endif
      }
#ifdef USB_STORAGE
    }
#endif

    if (!FindDrive()) {
#ifdef BOOT_FLASH_ON_ERROR
        BootFromFlash();
        return 0;
#else
        FatalError(ERROR_INVALID_DATA);
#endif
    }

    disk_ioctl(fs.pdrv, GET_SECTOR_COUNT, &storage_size);
    storage_size >>= 11;
    printf("storage size is %ld MB\n", storage_size);

    printf("ChangeDirectoryName\n");
    ChangeDirectoryName((unsigned char *) MIST_ROOT);

    printf("arc_reset()\n");
    arc_reset();

    printf("font_load()\n");
    font_load();

    printf("user_io_init()\n");
    user_io_init();

    printf("data_io_init()\n");
    data_io_init();

    ace_processor_register();

    c64files_init();

    // tos config also contains cdc redirect settings used by minimig
    printf("tos_config_load()\n");
    tos_config_load(-1);

    char mod = -1;

    if((USB_LOAD_VAR != USB_LOAD_VALUE) && !user_io_dip_switch1()) {
        mod = arc_open(MIST_ROOT "/CORE.ARC");
    } else {
        user_io_detect_core_type();
        if(user_io_core_type() != CORE_TYPE_UNKNOWN && !user_io_create_config_name(s, "ARC", CONFIG_ROOT)) {
            // when loaded from USB, try to load the development ARC file
            iprintf("Load development ARC: %s\n", s);
            mod = arc_open(s);
        }
    }

    if(mod < 0 || !strlen(arc_get_rbfname())) {
        fpga_init(NULL); // error opening default ARC, try with default RBF
    } else {
        user_io_set_core_mod(mod);
        strncpy(s, arc_get_rbfname(), sizeof(s)-5);
        strcat(s,".RBF");
        fpga_init(s);
    }

    DB9SetLegacy(0);
    set_legacy_mode(user_io_core_type() == CORE_TYPE_UNKNOWN ? LEGACY_MODE : MIST_MODE);
    return 0;
}

int mist_loop() {
    ps2keyboard_poll();
    ps2mouse_poll();
    cdc_control_poll();
    storage_control_poll();

    if (legacy_mode == LEGACY_MODE) {
        if (user_io_core_type() != CORE_TYPE_UNKNOWN) {
            set_legacy_mode(MIST_MODE);
        }
    } else {
        user_io_poll();
        joystick_poll();

        // MJ: check for legacy core and switch support on
        if (user_io_core_type() == CORE_TYPE_UNKNOWN) {
            set_legacy_mode(LEGACY_MODE);
        }

        // MIST (atari) core supports the same UI as Minimig
        if ((user_io_core_type() == CORE_TYPE_MIST) || (user_io_core_type() == CORE_TYPE_MIST2)) {
            if(!fat_medium_present()) { 
                tos_eject_all();
            }
            HandleUI();
        }

        // call original minimig handlers if minimig core is found
        if ((user_io_core_type() == CORE_TYPE_MINIMIG) || (user_io_core_type() == CORE_TYPE_MINIMIG2)) {
            if(!fat_medium_present()) {
                EjectAllFloppies();
            }
            HandleFpga();
            HandleUI();
        }

        // 8 bit cores can also have a ui if a valid config string can be read from it
        if((user_io_core_type() == CORE_TYPE_8BIT) && user_io_is_8bit_with_config_string()) {
            HandleUI();
        }

        // Archie core will get its own treatment one day ...
        if (user_io_core_type() == CORE_TYPE_ARCHIE) {
            HandleUI();
        }
    }
    return 0;
}


