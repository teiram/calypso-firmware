#ifndef TAPE_SERVICE_H
#define TAPE_SERVICE_H

#include "Service.h"
#include "Stream.h"
#include "PulseRenderer.h"
#include "TapeParser.h"
#include "pico/sync.h"

namespace calypso {

    class TapeService: public Service {
    public:
        static constexpr uint8_t TAPE_TRANSITION_BUFFER_SIZE = 64;
 
    private:
        constexpr static const char NAME[] = {"TapeService"};
        PulseRenderer& m_pulseRenderer;

        Stream* m_stream;
        TapeParser* m_tapeParser;
        bool m_play;
    public:
        TapeService(PulseRenderer &pulseRenderer);
        const char* name();
        bool init();
        void cleanup();
        bool needsAttention();
        void attention();

        bool insert(Stream *stream, TapeParser *parser);
        void play();
        void stop();
        void eject();
        bool playing();
        const char *currentStatus();
        uint8_t startBlock();
        void setStartBlock(uint8_t startBlock);
        uint8_t numBlocks();

    };

}

#endif //TAPE_SERVICE_H