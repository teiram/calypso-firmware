#include "ace_processor.h"
#include "fat_compat.h"
#include "data_io.h"

static void uncompress_send_ace(FIL *file, int index, const char *name, const char *ext) {
    FSIZE_t bytes2read = f_size(file);
    UINT br;

    iprintf("Sending ACE file of %llu bytes\n", bytes2read);
    uint8_t state = 0;
    uint8_t count = 0;
    uint32_t sentbytes = 0;
    bool eofmark = false;
    data_io_file_tx_prepare(file, index, "ACE");
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
    data_io_file_tx_done();
}

data_io_processor_t ACE_PROCESSOR = {
    .id = "ACE",
    .file_tx_send = &uncompress_send_ace
};

uint8_t ace_processor_register() {
    return data_io_add_processor(&ACE_PROCESSOR);
}
