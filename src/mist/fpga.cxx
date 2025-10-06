#include "fpga.h"
#include <cstdio>
#include <cstring>
#include "JTAG.h"
#include "pico/time.h"

using namespace calypso;
extern JTAG jtag;
constexpr char MENU_NAME_CALYPSO[] = "CORE.CALYPSO.RBF";
constexpr char MENU_NAME_GENERIC[] = "CORE.RBF";
constexpr char MENU_NAME_XILINX[] = "CORE.BIN";

static FRESULT getCoreFile(const char *name, FIL *fd, const char **openedFileName) {
    FRESULT result;
    if (name) {
        *openedFileName = (char *) name;
        return f_open(fd, *openedFileName, FA_READ);
    } else {
        *openedFileName = MENU_NAME_CALYPSO;
        if ((result = f_open(fd, *openedFileName, FA_READ)) != FR_OK) {
            *openedFileName = MENU_NAME_GENERIC;
            if ((result = f_open(fd, *openedFileName, FA_READ)) != FR_OK) {
                *openedFileName = MENU_NAME_XILINX;
                return f_open(fd, *openedFileName, FA_READ);
            }
        } 
        return result;
    }
}

unsigned char ConfigureFpga(const char *bitfile) {
    FIL bfile;
    const char *filename;
    if (getCoreFile(bitfile, &bfile, &filename) == FR_OK) {
        uint64_t size = f_size(&bfile);
        printf("FPGA bitstream file %s opened, file size = %ld\n", filename, size);
        uint8_t buffer[512];
        uint readBytes;
        bool xilinx = strstr(filename, ".BIN") == filename + strlen(filename) - 4;
        printf("Using Xilinx mode: %d\n", xilinx);
        if (xilinx) {
            jtag.startProgramXilinx();
        } else {
            jtag.startProgram();
        }
        while (size > 0) {
            if (f_read(&bfile, buffer, size > 512 ? 512 : size, &readBytes) != FR_OK) {
                printf("Error reading data from bitstream file\n");
                break;
            } else {
                jtag.programChunk(buffer, readBytes);
                size -= readBytes;
            }
        }
        if (xilinx) {
            jtag.endProgramXilinx();
        } else {
            jtag.programPostamble();
            jtag.endProgram();
        }
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
