#ifndef TAPE_PARSER_H
#define TAPE_PARSER_H

#include "Stream.h"
#include "PulseRenderer.h"

namespace calypso {

    class TapeParser {
    public:
        virtual bool insert(Stream& stream) = 0;
        virtual bool needsAttention() = 0;
        virtual void renderStep(PulseRenderer &pulseRenderer, Stream &stream) = 0;
        
        virtual const char* currentStatus() = 0;
        virtual bool playing() = 0;
        virtual uint8_t numBlocks() = 0;
        virtual uint8_t startBlock() = 0;
        virtual void setStartBlock(uint8_t startBlock) = 0;
    };

}

#endif //TAPE_PARSER_H