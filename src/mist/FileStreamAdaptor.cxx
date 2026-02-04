#include "FileStreamAdaptor.h"
#include <cstring>

using namespace calypso;

FileStreamAdaptor::FileStreamAdaptor():
    m_attached(false) {
    memset(&m_file, 0, sizeof(FIL));
}

void FileStreamAdaptor::attach(FIL* file) {
    memcpy(&m_file, file, sizeof(FIL));
    m_position = 0;
    f_lseek(&m_file, m_position);
    m_size = f_size(file);
    m_attached = true;
}

void FileStreamAdaptor::detach() {
    if (m_attached) {
        f_close(&m_file);
    }
    m_attached = false;
}

uint32_t FileStreamAdaptor::size() const {
    return m_size;
}

int16_t FileStreamAdaptor::read() {
    int8_t value;
    UINT btr;
    if (f_read(&m_file, &value, 1, &btr) == FR_OK) {
        if (btr == 1) {
            m_position++;
            return value & 0xff;
        } else {
            return -1;
        }
    } else {
        return -1;
    }
}

uint32_t FileStreamAdaptor::read(void *buffer, uint32_t size) {
    UINT btr;
    if (f_read(&m_file, buffer, size, &btr) == FR_OK) {
        m_position += btr;
        return btr;
    } else {
        return 0;
    }
}

uint32_t FileStreamAdaptor::position() const {
    return m_position;
}

bool FileStreamAdaptor::seekCur(int32_t offset) {
    uint32_t pos = m_position + offset;
    if (f_lseek(&m_file, pos) == FR_OK) {
        m_position = pos;
        return true;
    } else {
        return false;
    }
}

bool FileStreamAdaptor::seekSet(uint32_t position) {
    if (f_lseek(&m_file, position) == FR_OK) {
        m_position = position;
        return true;
    } else {
        return false;
    }
}

bool FileStreamAdaptor::available() {
    return m_position < m_size - 1;
}
