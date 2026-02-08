#ifndef TAPE_PARSER_H
#define TAPE_PARSER_H

#include "Stream.h"
#include "PulseRenderer.h"
#include "TapeConfiguration.h"

namespace calypso {

    class TapeParser {
    public:
        virtual bool insert(Stream& stream) = 0;
        virtual void rewind(Stream& stream) = 0;
        virtual bool needsAttention() = 0;
        virtual void renderStep(PulseRenderer &pulseRenderer, Stream &stream) = 0;
        virtual const char* type() = 0;
        virtual TapeConfiguration configuration() = 0;
        virtual const char* currentStatus() = 0;
        virtual bool playing() = 0;
        virtual bool hasBlockSupport() { return false; };
        virtual uint8_t numBlocks() { return 1; };
        virtual uint8_t startBlock() { return 0; };
        virtual void setStartBlock(uint8_t startBlock) {};
    };
}

#endif //TAPE_PARSER_H