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
    m_play(false) {    
}

bool TapeService::init() {
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
    TZX_DEBUG_LOG(L_DEBUG, "TapeService::insert\n");
    m_stream = stream;
    m_tapeParser = tapeParser;
    m_pulseRenderer.enable();
    return m_tapeParser->insert(*stream);
}

bool TapeService::playing() {
    return m_play && m_tapeParser != nullptr && m_tapeParser->playing();
}

void TapeService::play() {
    m_play = true;
}

void TapeService::stop() {
    m_play = false;
}

void TapeService::eject() {
    TZX_DEBUG_LOG(L_DEBUG, "TapeService::eject\n");
    m_stream = nullptr;
    m_tapeParser = nullptr;
    m_pulseRenderer.disable();
}

bool TapeService::needsAttention() {
    return m_play && m_stream != nullptr && m_tapeParser != nullptr && m_tapeParser->needsAttention();
}

void TapeService::attention() {
    m_tapeParser->renderStep(m_pulseRenderer, *m_stream);
}
