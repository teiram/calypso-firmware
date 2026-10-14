#ifndef PLAYER_UTIL_H
#define PLAYER_UTIL_H

#include <cinttypes>

// 16.16 fixed point helpers for the wavetable engine. This is deliberately a
// separate header from the firmware's core/Util.h: both used to be called
// Util.h with the same UTIL_H include guard, so a translation unit that pulled
// in both would silently get one of them skipped. The class is renamed for the
// same reason -- the core utility is in namespace calypso.
class PlayerUtil {
public:
    static constexpr int32_t fract16(double lv) {
        return (int32_t) (lv * 65536.0);
    }

    static inline int32_t fract16Mul(int32_t fract16, int16_t value) {
        return fract16 == 0x10000 ? value : (fract16 * value) >> 16;
    }

    static inline int32_t fract32Mul(int32_t fract16, int32_t value) {
        return fract16 == 0x10000 ? value : ((int64_t) fract16 * value) >> 16;
    }

};

#endif /* PLAYER_UTIL_H */
