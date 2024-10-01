#ifndef CALYPSO_DEBUG_H
#define CALYPSO_DEBUG_H

#include "Util.h"
#include <cstdio>

#define L_TRACE 0
#define L_DEBUG 1
#define L_INFO  2
#define L_WARN 3
#define L_ERROR 4

#ifdef DEBUG
    #define DEBUG_LOG(...)  printf(__VA_ARGS__)
    #define DEBUG_DUMP(name, buffer, size) do { printf("%s: \n", name); calypso::Util::hexdump((uint8_t *)buffer, (uint16_t) size, 0);} while (0);
#else
    #define DEBUG_LOG
    #define DEBUG_DUMP
#endif //DEBUG

#ifdef DEBUG_MMC
    #ifndef DEBUG_MMC_LEVEL
        #define DEBUG_MMC_LEVEL L_WARN
    #endif
    #define MMC_DEBUG_LOG(level, ...) do { if (level >= DEBUG_MMC_LEVEL) DEBUG_LOG(__VA_ARGS__); } while (0);
    #define MMC_DEBUG_DUMP(level, name, buffer, size) do { if (level >= DEBUG_MMC_LEVEL) DEBUG_DUMP(name, buffer, size); } while (0);
#else
    #define MMC_DEBUG_LOG
    #define MMC_DEBUG_DUMP
#endif //DEBUG_MMC

#ifdef DEBUG_SPI
    #ifndef DEBUG_SPI_LEVEL
        #define DEBUG_SPI_LEVEL L_WARN
    #endif
    #define SPI_DEBUG_LOG(level, ...) do { if (level >= DEBUG_SPI_LEVEL) DEBUG_LOG(__VA_ARGS__); } while (0);
    #define SPI_DEBUG_DUMP(level, name, buffer, size) do { if (level >= DEBUG_SPI_LEVEL) DEBUG_DUMP(name, buffer, size); } while (0);
#else
    #define SPI_DEBUG_LOG
    #define SPI_DEBUG_DUMP
#endif //DEBUG_SPI

#ifdef DEBUG_SD
    #ifndef DEBUG_SD_LEVEL
        #define DEBUG_SD_LEVEL L_WARN
    #endif
    #define SD_DEBUG_LOG(level, ...) do { if (level >= DEBUG_SD_LEVEL) DEBUG_LOG(__VA_ARGS__); } while (0);
    #define SD_DEBUG_DUMP(level, name, buffer, size) do { if (level >= DEBUG_SD_LEVEL) DEBUG_DUMP(name, buffer, size); } while (0);
#else
    #define SD_DEBUG_LOG
    #define SD_DEBUG_DUMP
#endif //DEBUG_SD

#ifdef DEBUG_USB
    #ifndef DEBUG_USB_LEVEL
        #define DEBUG_USB_LEVEL L_WARN
    #endif
    #define USB_DEBUG_LOG(level, ...) do { if (level >= DEBUG_USB_LEVEL) DEBUG_LOG(__VA_ARGS__); } while (0);
    #define USB_DEBUG_DUMP(level, name, buffer, size) do { if (level >= DEBUG_USB_LEVEL) DEBUG_DUMP(name, buffer, size); } while (0);
#else
    #define USB_DEBUG_LOG
    #define USB_DEBUG_DUMP
#endif //DEBUG_USB

#ifdef DEBUG_PS2
    #ifndef DEBUG_PS2_LEVEL
        #define DEBUG_PS2_LEVEL L_WARN
    #endif
    #define PS2_DEBUG_LOG(level, ...) do { if (level >= DEBUG_PS2_LEVEL) DEBUG_LOG(__VA_ARGS__); } while (0);
    #define PS2_DEBUG_DUMP(level, name, buffer, size) do { if (level >= DEBUG_PS2_LEVEL) DEBUG_DUMP(name, buffer, size); } while (0);
#else
    #define PS2_DEBUG_LOG
    #define PS2_DEBUG_DUMP
#endif //DEBUG_PS2

#endif //CALYPSO_DEBUG_H