#ifndef FIRMWARE_UPDATE_H
#define FIRMWARE_UPDATE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Update file layout. The update file is the regular build artifact
// (calypso-firmware.uf2) with a Calypso metadata block appended by
// tools/uf2meta.py, written by the build as FIRMWARE.UPG and copied to the
// SD card root.
//
// Flash layout (16 MB part, PICO_FLASH_SIZE_BYTES):
//
//   0x10000000 .. 0x10800000  main firmware image (8 MB)
//   0x10800000 .. 0x11000000  staging slot (image is copied here first,
//                             then into the main region from RAM)
//
// The metadata block targets the last 256 bytes of the flash (0x10FFFF00);
// it is parsed from the file, never programmed by the updater.

#define FW_UPDATE_META_ADDR     0x10FFFF00u
#define FW_UPDATE_FLASH_BASE    0x10000000u
#define FW_UPDATE_IMAGE_MAX     0x800000u   // main image region size
#define FW_UPDATE_STAGING_BASE  0x10800000u // staging slot address

// 256-byte payload of the metadata block. Layout must match tools/uf2meta.py.
typedef struct {
    char     magic[8];        // "CLPFWMET"
    char     version[16];     // firmware version (e.g. "0.23")
    char     date[16];        // build date, YYYYMMDD
    char     git[16];         // git short hash
    uint32_t payload_crc32;   // CRC32 of the whole image payload
    uint32_t payload_size;    // image payload size in bytes (multiple of 256)
    uint32_t num_blocks;      // number of image blocks (not counting this block)
    uint32_t header_crc32;    // CRC32 over the preceding 68 bytes
    uint8_t  pad[256 - 72];
} __attribute__((packed)) Uf2Meta;

#ifdef __cplusplus
}
#endif

#endif // FIRMWARE_UPDATE_H
