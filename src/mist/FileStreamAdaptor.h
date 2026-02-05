#ifndef FILE_STREAM_ADAPTOR_H
#define FILE_STREAM_ADAPTOR_H

#include "Stream.h"
#include "fat_compat.h"

namespace calypso {

    class FileStreamAdaptor: public Stream {
    private:
        FIL m_file;
        uint32_t m_position;
        uint32_t m_size;
        bool m_attached;
    public:
        FileStreamAdaptor();
        void attach(FIL *file);
        int16_t read();
        uint32_t read(void *buffer, uint32_t size);
        bool available();
        uint32_t size() const;
        bool seekCur(int32_t offset);
        bool seekSet(uint32_t pos);
        uint32_t position() const;
        void close();
    };

}

#endif //FILE_STREAM_ADAPTOR_H