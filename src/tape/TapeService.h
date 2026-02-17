#ifndef TAPE_SERVICE_H
#define TAPE_SERVICE_H

#include "Service.h"
#include "Stream.h"
#include "PulseRenderer.h"
#include "TapeParser.h"
#include "TapeConfiguration.h"
#include "pico/sync.h"

namespace calypso {

    class TapeService: public Service {
    public:
        static constexpr uint8_t TAPE_TRANSITION_BUFFER_SIZE = 64;

    private:
        constexpr static TapeConfiguration DEFAULT_CONFIGURATION = {.initialLevel = 0, .reverseLevel = false, .senseMotor = false};
        constexpr static const char NAME[] = {"TapeService"};
        PulseRenderer& m_pulseRenderer;
        uint8_t m_gpioMotor;
        bool m_lastMotorValue;
        TapeConfiguration m_configuration;
        Stream* m_stream;
        TapeParser* m_tapeParser;
        bool m_play;
    public:
        TapeService(PulseRenderer &pulseRenderer, uint8_t gpioMotor);
        const char* name();
        bool init();
        void cleanup();
        bool needsAttention();
        void attention();

        bool insert(Stream *stream, TapeParser *parser);
        bool inserted();
        void reconfigure(bool senseMotor, bool level, bool invertedOutput);

        void play();
        void stop();
        void eject();
        bool playing();
        bool hasBlockSupport();
        const char* currentStatus();
        TapeParser* tapeParser();
        uint8_t startBlock();
        const char* tapeParserType();
        void setStartBlock(uint8_t startBlock);
        uint8_t numBlocks();

    };

}

#endif //TAPE_SERVICE_H