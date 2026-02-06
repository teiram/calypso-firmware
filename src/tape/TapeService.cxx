#include "TapeService.h"
#include "calypso-debug.h"

using namespace calypso;

const char* TapeService::name() {
    return NAME;
}

TapeService::TapeService(PulseRenderer &pulseRenderer):
    m_pulseRenderer(pulseRenderer),
    m_stream(nullptr),
    m_tapeParser(nullptr),
    m_play(false),
    m_attached(false) {    
}

bool TapeService::init() {
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
    m_attached = true;
    m_stream = stream;
    m_tapeParser = tapeParser;
    return m_tapeParser->insert(*stream);
}

void TapeService::detach() {
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
    return m_play && m_stream != nullptr && m_tapeParser != nullptr && m_tapeParser->needsAttention();
}

void TapeService::attention() {
    m_tapeParser->renderStep(m_pulseRenderer, *m_stream);
    if (!m_tapeParser->playing()) {
        if (!m_attached) {
            // If the OSD has been closed, we are detached, do the cleanup here
            eject();
        } else {
            stop();
            m_tapeParser->rewind(*m_stream);
        }
    }
}
