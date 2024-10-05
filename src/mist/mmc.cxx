#include "mmc.h"
#include "SPISDCard.h"
#include "calypso-debug.h"
#include <stdio.h>
using namespace calypso;

extern SPISDCard sdcard;

unsigned char MMC_Init(void) {
    if (!sdcard.init()) {
        return CARDTYPE_NONE;
    } else {
        return sdcard.isSDHC() ? CARDTYPE_SDHC : CARDTYPE_SD;
    }
}

unsigned char MMC_Read(unsigned long lba, unsigned char *pReadBuffer) {
    MMC_DEBUG_LOG(L_DEBUG, "MMC_Read(lba=%d, pReadBuffer=%x)\n", lba, pReadBuffer);
    if (!sdcard.readSector(lba, pReadBuffer)) {
        MMC_DEBUG_LOG(L_ERROR, "Error reading sector at lba=%d\n", lba);
        return 0;
    } else {
        MMC_DEBUG_DUMP(L_TRACE, "Sector Read:", pReadBuffer, 512);
        return 1;
    }
}

unsigned char MMC_Write(unsigned long lba, const unsigned char *pWriteBuffer) {
    MMC_DEBUG_LOG(L_DEBUG, "MMC_Write(lba=%d, pWriteBuffer=%x)\n", lba, pWriteBuffer);
    if (!sdcard.writeSector(lba, pWriteBuffer)) {
        MMC_DEBUG_LOG(L_ERROR, "Error writing sector at lba=%d\n", lba);
        return 0;
    } else {
        return 1;
    }
}

unsigned char MMC_ReadMultiple(unsigned long lba, unsigned char *pReadBuffer, unsigned long nBlockCount) {
    MMC_DEBUG_LOG(L_DEBUG, "MMC_ReadMultiple(lba=%d, pReadBuffer=%x, nBlockCount=%d)\n", lba, pReadBuffer, nBlockCount); 
    while (nBlockCount--) {
      if (!sdcard.readSector(lba, pReadBuffer)) {
        MMC_DEBUG_LOG(L_ERROR, "Error reading sector at lba=%d\n", lba);
        return 0;
      }
      lba++;
      if (pReadBuffer != NULL) pReadBuffer += 512;
    }
    return 1;
}

unsigned char MMC_WriteMultiple(unsigned long lba, const unsigned char *pWriteBuffer, unsigned long nBlockCount) {
    MMC_DEBUG_LOG(L_DEBUG, "MMC_WriteMultiple(lba=%d, pWriteBuffer=%x, nBlockCount=%d)\n", lba, pWriteBuffer, nBlockCount); 
    while (nBlockCount--) {
        if (!sdcard.writeSector(lba, pWriteBuffer)) {
            MMC_DEBUG_LOG(L_ERROR, "Error writing sector at lba=%d\n", lba);
            return 0;
        }
        lba++;
        pWriteBuffer += 512;
    }
    return 1;
}

unsigned char MMC_GetCSD(unsigned char *buffer) {
    return sdcard.cmd9(buffer);
}

unsigned char MMC_GetCID(unsigned char *buffer) {
    return sdcard.cmd10(buffer);
}

unsigned char MMC_CheckCard() {
    return 1;
}

unsigned char MMC_IsSDHC() {
    return sdcard.isSDHC() ? 1 : 0;
}

// MMC get capacity
unsigned long MMC_GetCapacity() {
    MMC_DEBUG_LOG(L_DEBUG, "MMC_GetCapacity\n"); 

	unsigned long result = 0;
	unsigned char CSDData[16];
 
	MMC_GetCSD(CSDData);
    MMC_DEBUG_DUMP(L_TRACE, "CSDData:", CSDData, 16);

	if ((CSDData[0] & 0xC0) == 0x40) {
        //CSD Version 2.0 - SDHC
	    result = (CSDData[7] & 0x3f) << 26;
	    result |= CSDData[8] << 18;
	    result |= CSDData[9] << 10;
	    result += 1024;
	} else {    
	    int blocksize = CSDData[5] & 15;	// READ_BL_LEN
        blocksize = 1 << (blocksize - 9);		// Now a scalar:  physical block size / 512.
	    result = (CSDData[6] & 3) << 10;
	    result |= CSDData[7] << 2;
	    result |= (CSDData[8] >> 6) & 3;		// result now contains C_SIZE
	    int cmult = (CSDData[9] & 3) << 1;
	    cmult |= (CSDData[10] >> 7) & 1;
	    ++result;
	    result <<= cmult + 2;
	    result *= blocksize;	// Scale by the number of 512-byte chunks per block.
	}
    printf("Calculated SD capacity is %ld\n", result);
    return result;
}

char mmc_inserted() {
    return 1;
}

