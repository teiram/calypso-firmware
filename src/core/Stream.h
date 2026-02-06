#ifndef STREAM_H
#define STREAM_H

#include <cinttypes>

namespace calypso {
    class Stream {
    public:
        virtual int16_t read() = 0;
        virtual uint32_t read(void *buffer, uint32_t size) = 0;
        virtual bool available() = 0;
        virtual bool seekCur(int32_t offset) = 0;
        virtual bool seekSet(uint32_t pos) = 0;
        virtual uint32_t position() const = 0;
        virtual uint32_t size() const = 0;
        virtual void close() = 0;
    };

}

#endif //STREAM_H