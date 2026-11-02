// On-device firmware update from a UF2 file on the SD card (/FIRMWARE.UPG).
//
// This implements the three hooks the embedded mist-firmware menu already
// calls (CheckFirmware / GetFirmwareVersion / WriteFirmware) for real, in the
// spirit of libs/mist-firmware/firmware.c: the file is validated (CRC32 over
// the whole payload, carried in an appended metadata block instead of a
// leading UPGRADE header), then streamed into the RP2040 flash while the
// firmware is running.
//
// Self-flashing in two phases, because the image (~2-8 MB) is far larger than
// SRAM and flash sectors cannot be erased while code executes from them:
//
//   1. Staging (this file, normal flash-resident code): validate the UF2 and
//      copy its payload into the staging slot at 0x10800000. Erase/program
//      windows never touch the running image, IRQs are only masked around
//      each flash call.
//   2. Finisher (RAM-resident, never returns): copy staging -> main image
//      region sector by sector, then trigger a watchdog reset by direct
//      register access. After the first sector erase nothing may execute from
//      flash, so the finisher, its buffer and every function it calls must
//      live in RAM; it must not call libc (memcpy/printf) at all.
//
// Power loss during phase 2 leaves the bootrom with an invalid image, which
// makes it fall back to USB mass storage (BOOTSEL) - recoverable, not bricked.

#include <string.h>
#include <stdio.h>

#include "hardware/address_mapped.h"
#include "hardware/dma.h"
#include "hardware/flash.h"
#include "hardware/regs/psm.h"
#include "hardware/regs/watchdog.h"
#include "hardware/structs/psm.h"
#include "hardware/structs/sio.h"
#include "hardware/structs/watchdog.h"
#include "hardware/sync.h"

extern "C" {
// Submodule headers have no extern "C" guards.
#include "mist-firmware/errors.h"
#include "mist-firmware/fat_compat.h"
}
#include "hardware.h"

#include "FirmwareUpdate.h"

// ---------------------------------------------------------------------------
// UF2 format (pico-sdk boot/uf2.h). The generic MS spec magics are also
// accepted so files built with uf2conv.py style tools still parse.
// ---------------------------------------------------------------------------

static const uint32_t UF2_MAGIC_START0 = 0x0A324655u;
static const uint32_t UF2_MAGIC_START1 = 0x9E5D5157u; // pico-sdk boot/uf2.h
static const uint32_t UF2_MAGIC_END = 0x0AB16F30u;    // pico-sdk boot/uf2.h
static const uint32_t UF2_MAGIC_START1_SPEC = 0x9E5E5A46u;
static const uint32_t UF2_MAGIC_END_SPEC = 0x0AB16F6Du;
static const uint32_t UF2_FLAG_FAMILY_ID_PRESENT = 0x00002000u;
static const uint32_t RP2040_FAMILY_ID = 0xE48BFF56u;

static const char UF2_META_MAGIC[8] = {'C', 'L', 'P', 'F', 'W', 'M', 'E', 'T'};

// ---------------------------------------------------------------------------
// CRC32 (reflected, polynomial 0xEDB88320 - same as upstream CalculateCRC32
// and zlib.crc32; final() == zlib.crc32()).
// ---------------------------------------------------------------------------

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, uint32_t len) {
    static const uint32_t table[16] = {
        0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC,
        0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
        0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C,
        0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C,
    };
    while (len--) {
        crc ^= *data++;
        crc = (crc >> 4) ^ table[crc & 0xF];
        crc = (crc >> 4) ^ table[crc & 0xF];
    }
    return crc;
}

static uint32_t crc32_final(const uint8_t *data, uint32_t len) {
    return ~crc32_update(0xFFFFFFFFu, data, len);
}

// ---------------------------------------------------------------------------
// UF2 helpers
// ---------------------------------------------------------------------------

namespace {

struct Uf2BlockHeader {
    uint32_t magic_start0;
    uint32_t magic_start1;
    uint32_t flags;
    uint32_t target_addr;
    uint32_t payload_size;
    uint32_t block_no;
    uint32_t num_blocks;
    uint32_t family_id; // file_size for container files, family id otherwise
};

bool valid_uf2_magics(const Uf2BlockHeader &header, const uint8_t *block) {
    uint32_t end;
    memcpy(&end, block + 508, sizeof(end));
    if (header.magic_start0 != UF2_MAGIC_START0) return false;
    if (header.magic_start1 != UF2_MAGIC_START1 &&
        header.magic_start1 != UF2_MAGIC_START1_SPEC) return false;
    return end == UF2_MAGIC_END || end == UF2_MAGIC_END_SPEC;
}

Uf2BlockHeader block_header(const uint8_t *block) {
    Uf2BlockHeader header;
    memcpy(&header, block, sizeof(header));
    return header;
}

// Read and validate the trailing metadata block. Returns 0 on success.
int read_meta(FIL &file, Uf2Meta *meta) {
    FSIZE_t size = f_size(&file);
    if (size < 1024 || (size & 511)) return -1;

    if (f_lseek(&file, size - 512) != FR_OK) return -1;
    if (FileReadBlock(&file, sector_buffer) != FR_OK) return -1;

    Uf2BlockHeader header = block_header(sector_buffer);
    if (!valid_uf2_magics(header, sector_buffer)) return -1;
    if (header.target_addr != FW_UPDATE_META_ADDR) return -1;
    if (header.payload_size != 256) return -1;
    if (!(header.flags & UF2_FLAG_FAMILY_ID_PRESENT)) return -1;
    if (header.family_id != RP2040_FAMILY_ID) return -1;

    memcpy(meta, sector_buffer + 32, sizeof(*meta));
    if (memcmp(meta->magic, UF2_META_MAGIC, sizeof(meta->magic))) return -1;
    if (crc32_final(reinterpret_cast<uint8_t *>(meta), 68) != meta->header_crc32) return -1;
    // Fields are 16-byte strings; force termination for safe printf use.
    meta->version[15] = 0;
    meta->date[15] = 0;
    meta->git[15] = 0;

    if (meta->payload_size == 0 || meta->payload_size > FW_UPDATE_IMAGE_MAX) return -1;
    if (meta->payload_size & 255) return -1;
    if (meta->num_blocks != meta->payload_size / 256) return -1;
    if (size != (static_cast<FSIZE_t>(meta->num_blocks) + 1) * 512) return -1;
    return 0;
}

// Cluster link map for FileReadNextBlock, same pattern as upstream firmware.c.
// 99 entries support ~48 file fragments, plenty for a freshly copied file.
DWORD clmt[99];

// Stream every image block, validate the block headers and accumulate the
// payload CRC32. With stage=true each payload is programmed into the staging
// slot as it is read. Returns 0 when the payload CRC matches the metadata.
int stream_blocks(FIL &file, const Uf2Meta &meta, bool stage) {
    file.cltbl = clmt;
    clmt[0] = count_of(clmt);
    if (f_lseek(&file, CREATE_LINKMAP) != FR_OK) {
        iprintf("Upgrade: cannot build cluster link map\r");
        return -1;
    }
    if (f_lseek(&file, 0) != FR_OK) return -1;

    const uint32_t staging_off = FW_UPDATE_STAGING_BASE - FW_UPDATE_FLASH_BASE;
    uint32_t crc = 0xFFFFFFFFu;

    for (uint32_t n = 0; n < meta.num_blocks; n++) {
        if (FileReadNextBlock(&file, sector_buffer) != FR_OK) {
            iprintf("Upgrade: read error at block %lu\r", (unsigned long)n);
            return -1;
        }
        Uf2BlockHeader header = block_header(sector_buffer);
        uint32_t expect_addr = FW_UPDATE_FLASH_BASE + n * 256;
        if (!valid_uf2_magics(header, sector_buffer) ||
            header.payload_size != 256 ||
            header.family_id != RP2040_FAMILY_ID ||
            header.block_no != n || header.num_blocks != meta.num_blocks ||
            header.target_addr != expect_addr) {
            iprintf("Upgrade: bad block %lu (addr %08lx)\r",
                    (unsigned long)n, (unsigned long)header.target_addr);
            return -1;
        }

        const uint8_t *payload = sector_buffer + 32;
        crc = crc32_update(crc, payload, 256);
        if (stage) {
            uint32_t irq = save_and_disable_interrupts();
            flash_range_program(staging_off + n * 256, payload, 256);
            restore_interrupts(irq);
        }
    }

    if (~crc != meta.payload_crc32) {
        iprintf("Payload CRC mismatch! from header: %08lX, calculated: %08lX\r",
                (unsigned long)meta.payload_crc32, (unsigned long)~crc);
        return -1;
    }
    return 0;
}

// Erase the staging area for the incoming image, one sector per call so
// interrupts stay enabled in between.
void erase_staging(uint32_t extent) {
    const uint32_t staging_off = FW_UPDATE_STAGING_BASE - FW_UPDATE_FLASH_BASE;
    for (uint32_t off = 0; off < extent; off += FLASH_SECTOR_SIZE) {
        DISKLED_ON
        uint32_t irq = save_and_disable_interrupts();
        flash_range_erase(staging_off + off, FLASH_SECTOR_SIZE);
        restore_interrupts(irq);
        DISKLED_OFF
        if ((off & (64 * FLASH_SECTOR_SIZE - 1)) == 0) {
            iprintf("Upgrade: staging erase %lu%%\r",
                    (unsigned long)(100ull * off / extent));
        }
    }
}

// Read back the staged image from XIP and check it against the metadata.
bool verify_staged(const Uf2Meta &meta) {
    const uint8_t *staged = reinterpret_cast<const uint8_t *>(FW_UPDATE_STAGING_BASE);
    uint32_t crc = crc32_final(staged, meta.payload_size);
    if (crc != meta.payload_crc32) {
        iprintf("Upgrade: staged image CRC mismatch! expected: %08lX, read back: %08lX\r",
                (unsigned long)meta.payload_crc32, (unsigned long)crc);
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Phase 2 - the point of no return. Everything below runs from RAM after the
// first erase: no libc, no printf, no flash-resident calls other than the
// RAM-resident flash_range_erase/flash_range_program.
// ---------------------------------------------------------------------------

// Immediate reset via the watchdog registers, replicating
// watchdog_reboot(0, 0, 0) without calling into flash.
void __no_inline_not_in_flash_func(firmware_update_reset)(void) {
    hw_clear_bits(&watchdog_hw->ctrl, WATCHDOG_CTRL_ENABLE_BITS);
    watchdog_hw->scratch[4] = 0;
    // Reset everything apart from ROSC and XOSC, as _watchdog_enable() does.
    hw_set_bits(&psm_hw->wdsel,
                PSM_WDSEL_BITS & ~(PSM_WDSEL_ROSC_BITS | PSM_WDSEL_XOSC_BITS));
    hw_clear_bits(&watchdog_hw->ctrl,
                  WATCHDOG_CTRL_PAUSE_DBG0_BITS |
                  WATCHDOG_CTRL_PAUSE_DBG1_BITS |
                  WATCHDOG_CTRL_PAUSE_JTAG_BITS);
    hw_set_bits(&watchdog_hw->ctrl, WATCHDOG_CTRL_TRIGGER_BITS);
    for (;;) {
        __compiler_memory_barrier();
    }
}

void __no_inline_not_in_flash_func(firmware_update_finisher)(uint32_t extent) {
    // Copy buffer in .bss (RAM). The source pointers are volatile so the
    // compiler cannot turn these loops into flash-resident memcpy/memcmp
    // calls; the buffer itself must stay non-volatile for flash_range_program.
    static uint32_t chunk[FLASH_SECTOR_SIZE / sizeof(uint32_t)];
    const uint32_t staging_off = FW_UPDATE_STAGING_BASE - FW_UPDATE_FLASH_BASE;

    for (uint32_t off = 0; off < extent; off += FLASH_SECTOR_SIZE) {
        volatile const uint32_t *src =
            reinterpret_cast<volatile const uint32_t *>(FW_UPDATE_STAGING_BASE + off);
        for (uint32_t i = 0; i < count_of(chunk); i++) {
            chunk[i] = src[i];
        }

        // Skip sectors that already hold the right content; makes re-running
        // an update a no-op and a version bump cheaper.
        volatile const uint32_t *cur =
            reinterpret_cast<volatile const uint32_t *>(FW_UPDATE_FLASH_BASE + off);
        bool same = true;
        for (uint32_t i = 0; i < count_of(chunk); i++) {
            if (chunk[i] != cur[i]) {
                same = false;
                break;
            }
        }

        if (!same) {
            flash_range_erase(off, FLASH_SECTOR_SIZE);
            flash_range_program(off, reinterpret_cast<const uint8_t *>(chunk),
                                 FLASH_SECTOR_SIZE);
        }
        sio_hw->gpio_togl = 1u << LED_GPIO;
    }

    firmware_update_reset();
}

} // namespace

// ---------------------------------------------------------------------------
// Hooks called by libs/mist-firmware menu.c
// ---------------------------------------------------------------------------

extern "C" {

unsigned char CheckFirmware(char *name) {
    iprintf("CheckFirmware: name:%s\n", name);
    Error = ERROR_FILE_NOT_FOUND;

    FIL file;
    if (f_open(&file, name, FA_READ) != FR_OK) {
        iprintf("Cannot open firmware file!\r");
        return 0;
    }

    Error = ERROR_INVALID_DATA;
    Uf2Meta meta;
    if (read_meta(file, &meta) == 0) {
        iprintf("Upgrade file version : %s\r", meta.version);
        iprintf("Upgrade file date    : %s\r", meta.date);
        iprintf("Upgrade ROM size     : %lu\r", (unsigned long)meta.payload_size);
        if (stream_blocks(file, meta, false) == 0) {
            f_close(&file);
            Error = ERROR_NONE;
            return 1;
        }
    } else {
        iprintf("Invalid upgrade file header!\r");
    }
    f_close(&file);
    return 0;
}

char *GetFirmwareVersion(char *name) {
    static char v[16];
    FIL file;

    if (f_open(&file, name, FA_READ) != FR_OK) return NULL;

    Uf2Meta meta;
    bool ok = read_meta(file, &meta) == 0;
    f_close(&file);
    if (!ok) return NULL;

    memcpy(v, meta.version, sizeof(meta.version));
    v[sizeof(v) - 1] = 0;
    return v;
}

void WriteFirmware(char *name) {
    iprintf("WriteFirmware: name:%s\n", name);

    FIL file;
    if (f_open(&file, name, FA_READ) != FR_OK) return;

    Uf2Meta meta;
    if (read_meta(file, &meta) != 0) {
        iprintf("Upgrade: invalid metadata block\r");
        f_close(&file);
        return;
    }

    iprintf("Upgrade: flashing version %s (%s, %lu bytes)\r",
            meta.version, meta.date, (unsigned long)meta.payload_size);

    uint32_t extent = (meta.payload_size + FLASH_SECTOR_SIZE - 1) & ~(FLASH_SECTOR_SIZE - 1);
    erase_staging(extent);
    if (stream_blocks(file, meta, true) != 0) {
        f_close(&file);
        return;
    }
    f_close(&file);

    if (!verify_staged(meta)) return;

#ifdef FIRMWARE_UPDATE_DRY_RUN
    iprintf("Upgrade: dry run, skipping the final flash programming\r");
    return;
#endif

    iprintf("Upgrade: programming firmware, do not power off!\r");
    DISKLED_ON

    // Point of no return: quiesce everything, then finish from RAM.
    uint32_t irq = save_and_disable_interrupts();
    for (uint32_t ch = 0; ch < NUM_DMA_CHANNELS; ch++) {
        dma_channel_abort(ch);
    }
    firmware_update_finisher(extent);

    // Not reached.
    restore_interrupts(irq);
    DISKLED_OFF
}

} // extern "C"
