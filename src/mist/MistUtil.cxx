#include "MistUtil.h"

using namespace calypso;

uint16_t MistUtil::collectBits(uint8_t const* data, uint16_t offset, uint8_t size,
    bool isSigned) {

	// mask unused bits of first byte
	uint8_t mask = 0xff << (offset & 7);
	uint8_t byte = offset / 8;
	uint8_t bits = size;
	uint8_t shift = offset & 7;

	uint16_t rval = (data[byte++] & mask) >> shift;
	mask = 0xff;
	shift = 8 - shift;
	bits -= shift;

	// first byte already contained more bits than we need
	if(shift > size) {
		// mask unused bits
		rval &= (1 << size) - 1;
	} else {
		// further bytes if required
		while (bits) {
			mask = (bits < 8) ? (0xff >> (8 - bits)) : 0xff;
			rval += (data[byte++] & mask) << shift;
			shift += 8;
			bits -= (bits > 8) ? 8: bits;
		}
	}

	if (isSigned) {
		// do sign expansion
		uint16_t signBit = 1 << (size - 1);
		if (rval & signBit) {
			while (signBit) {
				rval |= signBit;
				signBit <<= 1;
			}
		}
	}
	return rval;
}
