#include "fpga.h"
#include <stdio.h>
#include "JTAG.h"
#include "pico/time.h"

using namespace calypso;
extern JTAG jtag;

unsigned char ConfigureFpga(const char *bitfile) {
    FIL bfile;
    const char* filename = bitfile ? bitfile : "CORE.RBF";
    if (f_open(&bfile, filename, FA_READ) == FR_OK) {
        uint64_t size = f_size(&bfile);
        printf("FPGA bitstream file %s opened, file size = %ld\n", filename, size);
        uint8_t buffer[512];
        uint readBytes;
        jtag.startProgram();
        while (size > 0) {
            if (f_read(&bfile, buffer, size > 512 ? 512 : size, &readBytes) != FR_OK) {
                printf("Error reading data from bitstream file\n");
                break;
            } else {
                jtag.programChunk(buffer, readBytes);
                size -= readBytes;
            }
        }
        jtag.programPostamble();
        jtag.endProgram();
        if (size > 0) {
            printf("Error during FPGA JTAG programming. Remaining bytes %ld\n", size);
            return ERROR_UPDATE_PROGRESS_FAILED;
        }
        f_close(&bfile);
    } else {
        printf("Error opening bitstream file %s\n", filename);
        return ERROR_BITSTREAM_OPEN;
    }
    return ERROR_NONE;
}
