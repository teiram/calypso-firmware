#ifndef MIST_FPGA_H
#define MIST_FPGA_H

#ifdef __cplusplus
extern "C" {
#endif

/* Include here to have the proper linkage type*/
#include "mist-firmware/user_io.h"
#include "mist-firmware/errors.h"

unsigned char ConfigureFpga(const char *name);

#ifdef __cplusplus
}
#endif

#endif //MIST_FPGA_H