#include "user_io.h"
#include "data_io.h"
#include <string.h>

#define ACE_EXTENSION "ACE"

extern char rom_direct_upload;

static void data_io_file_tx_send(FIL *file) {
  FSIZE_t bytes2send = f_size(file);
  UINT br;

  /* transmit the entire file using one transfer */
  iprintf("Selected file of %llu bytes to send\n", bytes2send);

  while(bytes2send) {
    iprintf(".");

    unsigned short c, chunk = (bytes2send > SECTOR_BUFFER_SIZE) ? SECTOR_BUFFER_SIZE: bytes2send;
    char *p;

    if (rom_direct_upload && fat_uses_mmc()) {
      // upload directly from the SD-Card if the core supports that
      bytes2send = (file->obj.objsize + 511) & 0xfffffe00;
      file->obj.objsize = bytes2send; // hack to foul FatFs think the last block is a full sector
      DISKLED_ON
      f_read(file, 0, bytes2send, &br);
      DISKLED_OFF
      bytes2send = 0;
    } else {
      DISKLED_ON
      f_read(file, sector_buffer, chunk, &br);
      DISKLED_OFF

#ifdef HAVE_QSPI
      if (user_io_get_core_features() & FEAT_QSPI) {
        qspi_write_block(sector_buffer, chunk);
      } else {
#endif
        EnableFpga();
        SPI(DIO_FILE_TX_DAT);

//      spi_write(sector_buffer, chunk); // DMA -- too fast for some cores
        for(p = sector_buffer, c=0;c < chunk;c++)
          SPI(*p++);

        DisableFpga();
#ifdef HAVE_QSPI
      }
#endif
      bytes2send -= chunk;
    }
  }
}

static void data_io_file_tx_send_ace(FIL *file) {
    FSIZE_t bytes2read = f_size(file);
    UINT br;

    iprintf("Sending ACE file of %llu bytes\n", bytes2read);
    uint8_t state = 0;
    uint8_t count = 0;
    uint32_t sentbytes = 0;
    bool eofmark = false;
    while (bytes2read && !eofmark) {
        uint16_t chunk = bytes2read > SECTOR_BUFFER_SIZE ? SECTOR_BUFFER_SIZE : bytes2read;
        DISKLED_ON
        f_read(file, sector_buffer, chunk, &br);
        DISKLED_OFF
        EnableFpga();
        SPI(DIO_FILE_TX_DAT);
        for (uint8_t *p = sector_buffer, c = 0; c < chunk && !eofmark; c++) {
            uint8_t value = *p++;
            switch (state) {
                case 0:
                    if (value != 0xED) {
                        SPI(value);
                        sentbytes++;
                    } else {
                        state = 1;
                    }
                    break;
                case 1:
                    if (value != 0) {
                        count = value;
                        state = 2;
                    } else {
                        eofmark = true;
                    }
                    break;
                case 2:
                    for (int i = 0; i < count; i++) {
                        SPI(value);
                        sentbytes++;
                    }
                    state = 0;
                    break;
            }
        }
        DisableFpga();
    }
    iprintf("Sent %d bytes\n", sentbytes);
}

void data_io_file_tx(FIL* file, char index, const char* ext) {
  data_io_file_tx_prepare(file, index, ext);
  if (!strncasecmp(ACE_EXTENSION, ext, 3)) {
    /* Dirty test. Use some sort of filters approach */
    data_io_file_tx_send_ace(file);
  } else {
    data_io_file_tx_send(file);
  }
  data_io_file_tx_done();
}


