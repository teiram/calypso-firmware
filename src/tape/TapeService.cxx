#include "TapeService.h"
#include "hardware/gpio.h"

#include "calypso-debug.h"

using namespace calypso;

const char* TapeService::name() {
    return NAME;
}

TapeService::TapeService(PulseRenderer &pulseRenderer, uint8_t gpioMotor):
    m_pulseRenderer(pulseRenderer),
    m_configuration(DEFAULT_CONFIGURATION),
    m_gpioMotor(gpioMotor),
    m_stream(nullptr),
    m_tapeParser(nullptr),
    m_play(false),
    m_attached(false) {    
}

bool TapeService::init() {
    if (m_configuration.senseMotor) {
        gpio_init(m_gpioMotor);
        gpio_set_dir(m_gpioMotor, false);
        m_lastMotorValue = gpio_get(m_gpioMotor);
    }
    return true;
}

void TapeService::cleanup() {
}

const char* TapeService::currentStatus() {
    if (m_tapeParser != nullptr) {
        return m_tapeParser->currentStatus();
    } else {
        return nullptr;
    }
}

uint8_t TapeService::startBlock() {
    return m_tapeParser != nullptr ? m_tapeParser->startBlock() : 0;
}

void TapeService::setStartBlock(uint8_t startBlock) {
    if (m_tapeParser != nullptr) {
        if (startBlock < m_tapeParser->numBlocks()) {
            m_tapeParser->setStartBlock(startBlock);
        }
    }
}

uint8_t TapeService::numBlocks() {
    return m_tapeParser != nullptr ? m_tapeParser->numBlocks() : 0;
}

bool TapeService::insert(Stream *stream, TapeParser *tapeParser) {
    TAPE_DEBUG_LOG(L_DEBUG, "TapeService::insert\n");
    if (tapeParser->insert(*stream)) {
        m_attached = true;
        m_stream = stream;
        m_tapeParser = tapeParser;
        m_configuration = tapeParser->configuration();
        return true;
    } else {
        TAPE_DEBUG_LOG(L_WARN, "Unable to insert into TapeService\n");
        return false;
    }
}

void TapeService::detach() {
    TAPE_DEBUG_LOG(L_DEBUG, "TapeService::detach\n");
    m_attached = false;
}

bool TapeService::playing() {
    return m_play && m_tapeParser != nullptr && m_tapeParser->playing();
}

void TapeService::play() {
    TAPE_DEBUG_LOG(L_DEBUG, "TapeService::play\n");
    m_pulseRenderer.enable();
    m_play = true;
}

TapeParser* TapeService::tapeParser() {
    return m_tapeParser;
}

void TapeService::stop() {
    TAPE_DEBUG_LOG(L_DEBUG, "TapeService::play\n");
    m_play = false;
    m_pulseRenderer.disable();
}

void TapeService::eject() {
    TAPE_DEBUG_LOG(L_DEBUG, "TapeService::eject\n");
    m_stream->close();
    m_play = false;
    m_stream = nullptr;
    m_tapeParser = nullptr;
    m_pulseRenderer.disable();
}

bool TapeService::needsAttention() {
    bool motorValue = false;
    if (m_configuration.senseMotor) {
        motorValue = gpio_get(m_gpioMotor);
        if (motorValue != m_lastMotorValue) {
            TAPE_DEBUG_LOG(L_DEBUG, "Motor value changed to %d\n", motorValue);
        }
        m_lastMotorValue = motorValue;
    }
    return m_play
        && (!m_configuration.senseMotor || !motorValue)
        && m_stream != nullptr
        && m_tapeParser != nullptr 
        && m_tapeParser->needsAttention();
}

void TapeService::attention() {
    m_tapeParser->renderStep(m_pulseRenderer, *m_stream);
    if (!m_tapeParser->playing()) {
        if (!m_attached) {
            // If the OSD has been closed, we are detached, do the cleanup here
            TAPE_DEBUG_LOG(L_DEBUG, "Automatically ejecting on detached end of play\n");
            eject();
        } else {
            stop();
            m_tapeParser->rewind(*m_stream);
        }
    }
}
