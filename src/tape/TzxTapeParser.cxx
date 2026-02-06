#include "TzxTapeParser.h"
#include "tzxdefinitions.h"
#include "calypso-debug.h"
#include <cstring>
using namespace calypso;


#define ENCTIME(value) (uint16_t) ((((float) value) / 3.5) + 0.5)
#define sinfo  info.standard_block
#define ginfo  info.generalized_block

TzxTapeParser::TzxTapeParser():
    m_numBlocks(0),
    m_startBlock(0),
    m_currentPosition(0),
    m_streamSize(0) {    
}

const char *TzxTapeParser::type() {
    return TYPE;
}

void TzxTapeParser::init() {
    memset(&m_context, 0, sizeof(tzx_context_t));
    m_numBlocks = m_startBlock = m_currentPosition = m_streamSize = 0;
}

bool TzxTapeParser::insert(Stream& stream) {
    TAPE_DEBUG_LOG(L_DEBUG, "TzxTapeParser::insert\n");
    stream.seekSet(10);
    m_streamSize = stream.size();
    calculateRelevantOffsets(stream);
    return m_tzxState == TZX_STATE_INITIALIZED;
}

const char *TzxTapeParser::currentStatus() {
        snprintf(m_statusBuffer, 32, "B:%d/%d, T:%02x, P:%ld/%ld",
        m_context.current_block,
        m_numBlocks,
        m_context.blockId, 
        m_currentPosition,
        m_streamSize);

    return m_statusBuffer;
}

uint8_t TzxTapeParser::startBlock() {
    return m_startBlock;
}

uint8_t TzxTapeParser::numBlocks() {
    return m_numBlocks;
}

void TzxTapeParser::setStartBlock(uint8_t startBlock) {
    m_startBlock = startBlock;
}

bool TzxTapeParser::playing() {
    return m_tzxState != TZX_STATE_ERROR && m_tzxState != TZX_STATE_IDLE;
}

void TzxTapeParser::rewind(Stream &stream) {
    stream.seekSet(10);
    m_currentPosition = stream.position();
    m_context.current_block = 0;
    m_tzxState == TZX_STATE_INITIALIZED;
}

uint8_t TzxTapeParser::log2(uint8_t value) {
    switch (value) {
    case 2:
        return 1;
    case 4:
        return 2;
    case 8:
        return 3;
    case 16:
        return 4;
    case 32:
        return 5;
    case 64:
        return 6;
    case 128:
        return 7;
    case 0:     //Represents 256
        return 8;
    default:
        return 0;
    }
}

inline bool TzxTapeParser::isBlockIncluded(uint8_t block, uint8_t relevant_blocks[]) {
    for (uint8_t i = 0; i < TZX_MAX_BLOCK_OFFSETS; i++) {
        if (relevant_blocks[i] == 255) {
            break;
        } else if (relevant_blocks[i] == block) {
            return true;
        }
    }
    return false;
}

void TzxTapeParser::calculateRelevantOffsets(Stream &stream) {
    uint8_t relevant_blocks[TZX_MAX_BLOCK_OFFSETS];
  
    uint32_t saved_offset = stream.position();
    uint32_t block_size = 0;
    uint8_t num_blocks = 0;
    int16_t next_value = 0;
    memset(&relevant_blocks, 255, TZX_MAX_BLOCK_OFFSETS);
    for (uint8_t pass = PASS_FIND_BLOCKS; pass <= PASS_FIND_OFFSETS; pass++) {
        stream.seekSet(saved_offset);
        bool eof = false;
        uint8_t current_block = 0;
        uint8_t index = 0;
        if (pass == PASS_FIND_OFFSETS && relevant_blocks[0] == 255) {
            //No relevant block found
            break;
        }
        while (!eof) {
            if (pass == PASS_FIND_OFFSETS && isBlockIncluded(current_block, relevant_blocks)) {
                m_blockOffsets[index++] = {current_block, stream.position()};
                TAPE_DEBUG_LOG(L_INFO, "Stored offset for block: %d\n", current_block);
            }
            next_value = stream.read();
            if (next_value > -1) {
                m_context.blockId = next_value & 0xff;
                TAPE_DEBUG_LOG(L_DEBUG, "Block Id: 0x%02x\n", m_context.blockId);       
                switch (m_context.blockId) {
                case TZX_PUREDATA_BLOCK: {
                    tzx_puredata_block_t *blockp = (tzx_puredata_block_t *) m_inputBuffer;
                    stream.read((void *) blockp, sizeof(tzx_puredata_block_t));
                    block_size = blockp->data_length[0] + 
                        ((uint32_t) blockp->data_length[1] * 256) +
                        ((uint32_t) blockp->data_length[2] * 65536);
                    break;
                }
                case TZX_STANDARD_SPEED_BLOCK: {
                    tzx_standard_block_t *blockp = (tzx_standard_block_t *) m_inputBuffer;
                    stream.read((void*) blockp, sizeof(tzx_standard_block_t));
                    block_size = blockp->data_length;
                    break;
                }
                case TZX_TURBO_SPEED_BLOCK: {
                    tzx_turbospeed_block_t *blockp =(tzx_turbospeed_block_t *) m_inputBuffer;
                    stream.read((void*) blockp, sizeof(tzx_turbospeed_block_t));
                    TAPE_DEBUG_DUMP(L_DEBUG, "turbo", blockp, sizeof(tzx_turbospeed_block_t));
                    block_size = blockp->data_length[0] + 
                        ((uint32_t) blockp->data_length[1] * 256) +
                        ((uint32_t) blockp->data_length[2] * 65536);
                    break;
                }
                case TZX_GENERALIZED_DATA_BLOCK: {
                    tzx_generalized_data_block_t *blockp = (tzx_generalized_data_block_t*) m_inputBuffer;
                    stream.read((void *) blockp, sizeof(tzx_generalized_data_block_t));
                    block_size = blockp->block_length - sizeof(tzx_generalized_data_block_t) + 4;
                    break;
                }
                case TZX_MESSAGE_BLOCK:
                    next_value = stream.read(); 
                case TZX_TEXT_DESCRIPTION:
                case TZX_GROUP_START:
                    next_value = stream.read();
                    block_size = next_value & 0xff;
                    break;
                case TZX_GROUP_END:
                    block_size = 0;
                    break;
                case TZX_ARCHIVE_INFO:
                    stream.read(&block_size, 2);
                    break;
                case TZX_LOOP_START:
                    block_size = 2;
                    break;
                case TZX_LOOP_END:
                    block_size = 0;
                    break;
                case TZX_STOP_48K_MODE:
                    block_size = 4;
                    break;
                case TZX_SET_SIGNAL_LEVEL:
                    block_size = 5;
                    break;
                case TZX_PURE_TONE:
                    block_size = 4;
                    break;
                case TZX_PULSE_SEQUENCE:
                    block_size = (stream.read() & 0xff) * 2;
                    break;
                case TZX_PAUSE:
                    block_size = 2;  
                    break;
                case TZX_CUSTOM_INFO: {
                    uint32_t offset;
                    stream.seekCur(16); //Skip ID string 16 chars
                    stream.read(&offset, sizeof(offset));
                    block_size = offset;
                    break;
                }    
                case TZX_GLUE_BLOCK:
                    block_size = 9;
                    break;
                case TZX_CALL_SEQUENCE: {
                    stream.read(&block_size, 2);
                    if (pass == PASS_FIND_BLOCKS) {
                        do {
                            stream.read(&next_value, 2);
                            if (!isBlockIncluded(current_block + next_value, relevant_blocks)) {
                                relevant_blocks[index] = current_block + next_value;
                                TAPE_DEBUG_LOG(L_INFO, "Found sequence call jump destination: %d\n", relevant_blocks[index]);
                                index++;
                            }
                        } while (--block_size > 0);
                    }
                    block_size = block_size * 2;
                    break;
                }
                case TZX_JUMP_TO_BLOCK: {
                    if (pass == PASS_FIND_BLOCKS) {
                        stream.read(&next_value, 2);
                        if (!isBlockIncluded(current_block + next_value, relevant_blocks)) {
                            relevant_blocks[index] = current_block + next_value;
                            TAPE_DEBUG_LOG(L_INFO, "Found block jump destination: %d\n", relevant_blocks[index]);
                            index++;
                        }
                        block_size = 0;
                    } else {
                        block_size = 2;
                    }
                    break;
                }
                case TZX_RETURN_FROM_SEQUENCE:
                    block_size = 0;
                    break;
                default:
                    TAPE_DEBUG_LOG(L_WARN, "Unsupported block type: %d\n", m_context.blockId);
                    eof = true;
                }
                stream.seekCur(block_size);
                if (pass == PASS_FIND_BLOCKS) {
                    num_blocks = current_block;
                }
                if (current_block == 255) {
                    TAPE_DEBUG_LOG(L_WARN, "Unable to calculate offsets over the first 255 blocks\n");
                    eof = true;
                }
                current_block++;
            } else {
                eof = true;
            }
        }
    }
    stream.seekSet(saved_offset);
    m_numBlocks = num_blocks;
    m_tzxState = TZX_STATE_INITIALIZED;
}

inline uint32_t TzxTapeParser::getBlockOffset(uint8_t block) {
    block_offset_t offset;
    for (uint8_t i = 0; i < TZX_MAX_BLOCK_OFFSETS; i++) {
        if (m_blockOffsets[i].block == block) {
            return m_blockOffsets[i].offset;
        }
    }
    TAPE_DEBUG_LOG(L_WARN, "Unable to find offset for block: %d\n");
    m_tzxState = TZX_STATE_ERROR;
    return 0;
}

inline uint8_t TzxTapeParser::transformGdbFlags(uint8_t flags) {
    switch (flags) {
    case 0:
        return 0;
    case 1:
        return PulseRenderer::VALUE_KEEP_POL;
    case 2:
        return PulseRenderer::VALUE_FORCE_ZERO;
    case 3:
        return PulseRenderer::VALUE_FORCE_ONE;
    default:
        return 255;
    }
}

bool TzxTapeParser::needsAttention() {
    return m_tzxState == TZX_STATE_FINDBLOCK || m_tzxState == TZX_STATE_READBLOCK || m_tzxState == TZX_STATE_INITIALIZED;
}

inline void TzxTapeParser::writePulseWithFlags(PulseRenderer &pulseRenderer, const PulseRenderer::Transition &transition) {
    if (m_context.next_pulse_flags) {
        PulseRenderer::Transition flagged(transition);
        flagged.flags |= m_context.next_pulse_flags;
        m_context.next_pulse_flags = 0;
        pulseRenderer.write(flagged);
    } else {
        pulseRenderer.write(transition);
    }
}

void TzxTapeParser::renderStep(PulseRenderer &pulseRenderer, Stream &stream) {
    int16_t next_value;
    m_currentPosition = stream.position();
    switch (m_tzxState) {
    case TZX_STATE_INITIALIZED:
        m_tzxState = TZX_STATE_FINDBLOCK;
        break;
    case TZX_STATE_FINDBLOCK: {
        next_value = stream.read();
        if (next_value > -1) {
            m_context.blockId = next_value & 0xff;
            memset(&m_blockInfo, 0, sizeof(block_info_t));
            TAPE_DEBUG_LOG(L_DEBUG, "Block type: 0x%02x, position:%ld\n", m_context.blockId, stream.position());

            m_context.last_offset = m_context.current_offset;
            m_context.current_offset = stream.position() - 1;

            switch (m_context.blockId) {
            case TZX_PUREDATA_BLOCK: {
                tzx_puredata_block_t *blockp = (tzx_puredata_block_t *) m_inputBuffer;
                int16_t readc = stream.read((void *) blockp, sizeof(tzx_puredata_block_t));   
                if (readc == sizeof(tzx_puredata_block_t)) {
                    m_blockInfo.sinfo.zero_length = ENCTIME(blockp->zero_length);
                    m_blockInfo.sinfo.one_length =  ENCTIME(blockp->one_length); 
                    m_blockInfo.sinfo.bits_last_byte = blockp->bits_last_byte;
                    m_blockInfo.end_pause = blockp->end_pause;
                    m_blockInfo.size = blockp->data_length[0] + 
                        ((uint32_t) blockp->data_length[1] * 256) +
                        ((uint32_t) blockp->data_length[2] * 65536);
                    m_tzxState = TZX_STATE_READBLOCK;
                } else {
                    TAPE_DEBUG_LOG(L_WARN, "Unable to read block header\n");
                    m_tzxState = TZX_STATE_ERROR;
                }
                break;
            }
            case TZX_STANDARD_SPEED_BLOCK: {
                tzx_standard_block_t *blockp = (tzx_standard_block_t *) m_inputBuffer;
                int16_t readc = stream.read((void*) blockp, sizeof(tzx_standard_block_t));
                if (readc == sizeof(tzx_standard_block_t)) {
                    m_blockInfo.sinfo.zero_length = STD_ZERO_LENGTH;
                    m_blockInfo.sinfo.one_length = STD_ONE_LENGTH;
                    m_blockInfo.sinfo.bits_last_byte = 8;
                    m_blockInfo.end_pause = blockp->end_pause;
                    m_blockInfo.size = blockp->data_length;
                    m_blockInfo.pilot_length = STD_LEADER_LENGTH;
                    m_blockInfo.sinfo.sync1_length = STD_SYNC1_LENGTH;
                    m_blockInfo.sinfo.sync2_length = STD_SYNC2_LENGTH;
                    next_value = stream.read(); //Get the type of block
                    if (next_value > -1) {
                        m_blockInfo.sinfo.block_type = next_value & 0xff;
                        m_blockInfo.sinfo.pilot_pulses = m_blockInfo.sinfo.block_type < 128 ? 
                            STD_HEADER_PULSES : STD_DATA_PULSES;
                        stream.seekCur(-1); //Compensate for block_type prefetch
                        m_tzxState = TZX_STATE_READBLOCK;
                    } else {
                        TAPE_DEBUG_LOG(L_INFO, "!EOF\n");
                        m_tzxState = TZX_STATE_ERROR;
                    }
                }
                break;
            }
            case TZX_TURBO_SPEED_BLOCK: {
                tzx_turbospeed_block_t *blockp =(tzx_turbospeed_block_t *) m_inputBuffer;
                int16_t readc = stream.read((void*) blockp, sizeof(tzx_turbospeed_block_t));
                if (readc == sizeof(tzx_turbospeed_block_t)) {
                    m_blockInfo.sinfo.zero_length = ENCTIME(blockp->zero_length);
                    m_blockInfo.sinfo.one_length = ENCTIME(blockp->one_length);
                    m_blockInfo.sinfo.bits_last_byte = blockp->bits_last_byte;
                    m_blockInfo.end_pause = blockp->end_pause;
                    m_blockInfo.size = blockp->data_length[0] + 
                        ((uint32_t) blockp->data_length[1] * 256) +
                        ((uint32_t) blockp->data_length[2] * 65536);
                    m_blockInfo.pilot_length = ENCTIME(blockp->pilot_length);
                    m_blockInfo.sinfo.sync1_length = ENCTIME(blockp->sync_1st_pulse_length);
                    m_blockInfo.sinfo.sync2_length = ENCTIME(blockp->sync_2nd_pulse_length);
                    m_blockInfo.sinfo.pilot_pulses = blockp->pilot_pulses;
                    m_tzxState = TZX_STATE_READBLOCK;
                }
                break;
            }
            case TZX_GENERALIZED_DATA_BLOCK: {
                tzx_generalized_data_block_t *blockp = (tzx_generalized_data_block_t*) m_inputBuffer;
                int16_t readc = stream.read((void *) blockp, sizeof(tzx_generalized_data_block_t));
                if (readc == sizeof(tzx_generalized_data_block_t)) {
                    m_blockInfo.size = blockp->block_length;
                    m_blockInfo.end_pause = blockp->end_pause;
                    m_blockInfo.pilot_length = blockp->symbols_in_pilot_sync;
                    m_blockInfo.ginfo.max_pulses_pilot_symbol = blockp->max_pulses_per_pilot_sync;
                    m_blockInfo.ginfo.pilot_symbol_count = blockp->pilot_sync_alphabet_symbols;
                    m_blockInfo.ginfo.data_length = blockp->symbols_in_data_stream;
                    m_blockInfo.ginfo.max_pulses_data_symbol = blockp->max_pulses_per_data_symbol;
                    m_blockInfo.ginfo.data_symbol_count = blockp->data_alphabet_symbols;
                    if (m_blockInfo.pilot_length > 0) {
                        m_blockInfo.ginfo.state = GDB_STATE_BEFORE_PILOT;
                    } else if (m_blockInfo.ginfo.data_length > 0) {
                        m_blockInfo.ginfo.state = GDB_STATE_BEFORE_DATA;
                    } else {
                        m_blockInfo.ginfo.state = GDB_STATE_BEFORE_PAUSE;
                    }
                    m_blockInfo.ginfo.end_offset = stream.position() + m_blockInfo.size - sizeof(tzx_generalized_data_block_t) + 4;
                    m_tzxState = TZX_STATE_READBLOCK;
                }
                break;
            }
            case TZX_TEXT_DESCRIPTION:
            case TZX_GROUP_START: {
                next_value = stream.read();
                stream.seekCur(next_value & 0xff);
                m_tzxState = TZX_STATE_READBLOCK;
                m_context.current_block++;
                break;
            }
            case TZX_HARDWARE_TYPE: {
                next_value = stream.read();
                stream.seekCur(3 * (next_value & 0xff));
                m_tzxState = TZX_STATE_FINDBLOCK;
                m_context.current_block++;
                break;
            }
            case TZX_MESSAGE_BLOCK: {
                stream.seekCur(1); //Skip time to display of message
                next_value = stream.read(); //Length of text message
                stream.seekCur(next_value & 0xff);
                m_tzxState = TZX_STATE_FINDBLOCK;
                break;
            }            
            case TZX_GROUP_END: {
                m_tzxState = TZX_STATE_READBLOCK;
                m_context.current_block++;
                break;
            }
            case TZX_STOP_48K_MODE:
                stream.seekCur(4);
                if (m_context.mode48k) {
                    m_tzxState = TZX_STATE_IDLE;
                    //Set the offset for the next block to resume properly
                    m_context.current_offset = stream.position();
                } else {
                    m_tzxState = TZX_STATE_FINDBLOCK;
                }
                m_context.current_block++;
                break;
            case TZX_SET_SIGNAL_LEVEL:
                stream.seekCur(4);
                next_value = stream.read(); //1 to force high, 0 to force low
                m_context.next_pulse_flags = (next_value & 0xff) ? PulseRenderer::VALUE_FORCE_ONE : PulseRenderer::VALUE_FORCE_ZERO;
                m_tzxState = TZX_STATE_FINDBLOCK;
                m_context.current_block++;
                break;                     
            case TZX_ARCHIVE_INFO: {
                uint16_t* skip = (uint16_t*) m_inputBuffer;
                if (stream.read((void*) skip, 2) > -1) {
                    stream.seekCur(*skip);
                    m_tzxState = TZX_STATE_FINDBLOCK;
                    m_context.current_block++;
                } else {
                    m_tzxState = TZX_STATE_ERROR;
                }
                break;
            }
            case TZX_LOOP_START: {
                stream.read(&(m_context.loop_count), 2);
                m_context.loop_start = stream.position();
                m_context.current_block++;
                m_context.loop_start_block = m_context.current_block;
                m_tzxState = TZX_STATE_FINDBLOCK;
                break;
            }
            case TZX_LOOP_END: {
                if (m_context.loop_start > 0) {
                    if (m_context.loop_count-- > 0) {
                        stream.seekSet(m_context.loop_start);
                        m_context.current_block = m_context.loop_start_block;
                    } else {
                        m_context.loop_start = 0;
                        m_context.current_block++;
                    }
                    m_tzxState = TZX_STATE_FINDBLOCK;
                } else {
                    TAPE_DEBUG_LOG(L_ERROR, "Unmatched loop end\n");
                    m_tzxState = TZX_STATE_ERROR;
                }
                break;
            }
            case TZX_PURE_TONE: {
                tzx_puretone_block_t *blockp =(tzx_puretone_block_t *) m_inputBuffer;
                int16_t readc = stream.read((void*) blockp, sizeof(tzx_puretone_block_t));
                if (readc == sizeof(tzx_puretone_block_t)) {
                    m_blockInfo.size = blockp->pulse_count;
                    m_blockInfo.pilot_length = ENCTIME(blockp->pulse_length);
                    m_tzxState = TZX_STATE_READBLOCK;
                }
                break;
            }
            case TZX_PULSE_SEQUENCE: {
                next_value = stream.read();
                m_blockInfo.size = next_value & 0xff;
                m_tzxState = TZX_STATE_READBLOCK;
                break;
            }
            case TZX_PAUSE: {
                uint16_t pause_value;
                stream.read(&pause_value, 2);
                m_blockInfo.size = 0;
                m_blockInfo.end_pause = pause_value;
                if (m_blockInfo.end_pause) {
                    m_tzxState = TZX_STATE_READBLOCK;
                } else {
                    m_tzxState = TZX_STATE_IDLE;
                    m_context.current_offset = stream.position();
                    m_context.current_block++;
                }
                break;
            }
            case TZX_CUSTOM_INFO: {
                uint32_t offset;
                stream.seekCur(16); //Skip ID string 16 chars
                stream.read(&offset, sizeof(offset));
                stream.seekCur(offset);
                m_context.current_block++;
                m_tzxState = TZX_STATE_FINDBLOCK;
                break;
            }
            case TZX_GLUE_BLOCK: {
                stream.seekCur(9);  //Skip 9 bytes of description of "Glue" Block
                m_tzxState = TZX_STATE_FINDBLOCK;
                m_context.current_block++;
                break;
            }
            case TZX_CALL_SEQUENCE: {
                stream.read(&m_context.call_count, 2);
                m_context.next_call_offset = stream.position();
                m_context.call_start_block = m_context.current_block;
                TAPE_DEBUG_LOG(L_DEBUG, "Call count: %d, next call offset :%d\n", m_context.call_count, m_context.next_call_offset);
                m_context.call_count--;
            }
            JUMP_TO_BLOCK:
            case TZX_JUMP_TO_BLOCK: {
                int16_t* offset = (int16_t*) m_inputBuffer;
                if (stream.read((void*) offset, 2) > -1) {
                    TAPE_DEBUG_LOG(L_DEBUG, "Jumping to block got offset: %d, in block %d\n", *offset, m_context.current_block);
                    m_context.current_block = m_context.current_block + *offset;
                    TAPE_DEBUG_LOG(L_DEBUG, "Jumping to block: %d\n", m_context.current_block);
                    stream.seekSet(getBlockOffset(m_context.current_block));
                    m_tzxState = TZX_STATE_FINDBLOCK;
                } else {
                    m_tzxState = TZX_STATE_ERROR;
                }
                break;
            }
            case TZX_RETURN_FROM_SEQUENCE: {
                m_context.next_call_offset += 2;
                if (m_context.call_count > 0) {
                    m_context.call_count--;
                    TAPE_DEBUG_LOG(L_DEBUG, "On return from sequence and next offset: %d\n", m_context.next_call_offset);
                    goto JUMP_TO_BLOCK;
                } else {
                    stream.seekSet(m_context.next_call_offset);
                    m_context.next_call_offset = 0;
                    m_context.current_block = m_context.call_start_block + 1;
                }
                m_tzxState = TZX_STATE_FINDBLOCK;
                break; 
            }
            default: {
                m_tzxState = TZX_STATE_ERROR;
                TAPE_DEBUG_LOG(L_WARN, "Unsupported block type: %d\n", m_context.blockId);
            }
        }
    } else {
        TAPE_DEBUG_LOG(L_INFO, "EOF\n");
        m_tzxState = TZX_STATE_IDLE;
    }
    }
    if (m_tzxState == TZX_STATE_READBLOCK) {
        TAPE_DEBUG_LOG(L_DEBUG, "Block size: %d\n", m_blockInfo.size);
    }
    break;

    case TZX_STATE_READBLOCK:
        if (!pulseRenderer.full()) {
            if (m_context.blockId == TZX_PULSE_SEQUENCE) {
                if (m_blockInfo.size-- > 0) {
                    uint16_t pulse_length;
                    stream.read(&pulse_length, 2);
                    writePulseWithFlags(pulseRenderer, {ENCTIME(pulse_length), 0});
                } else {
                    m_context.current_block++;
                    m_tzxState = TZX_STATE_FINDBLOCK;
                }
                break;
            }
            if (m_context.blockId == TZX_PURE_TONE) {
                if (m_blockInfo.size-- > 0) {
                    writePulseWithFlags(pulseRenderer, {m_blockInfo.pilot_length, 0});
                } else {
                    m_context.current_block++;
                    m_tzxState = TZX_STATE_FINDBLOCK;
                }
                break;
            }
            if (m_context.blockId == TZX_GENERALIZED_DATA_BLOCK) {
                uint16_t length;
                switch (m_blockInfo.ginfo.state) {
                case GDB_STATE_BEFORE_PILOT:   //Get the pilot symbol alphabet offset and 
                    m_blockInfo.ginfo.alphabet_offset = stream.position();
                    m_blockInfo.ginfo.symbol_size = 1 + 2 * m_blockInfo.ginfo.max_pulses_pilot_symbol;
                    TAPE_DEBUG_LOG(L_DEBUG, "Pilot alphabet in position %ld\n", stream.position());
                    m_blockInfo.ginfo.alphabet_size = m_blockInfo.ginfo.pilot_symbol_count ? 
                        m_blockInfo.ginfo.pilot_symbol_count * m_blockInfo.ginfo.symbol_size :
                        256 * m_blockInfo.ginfo.symbol_size;
                    TAPE_DEBUG_LOG(L_DEBUG, "Alphabet size: %d\n", m_blockInfo.ginfo.alphabet_size);
                    if (m_blockInfo.ginfo.alphabet_size < TZX_SYMBOL_BUFFER_SIZE) {
                        //Cache the alphabet
                        TAPE_DEBUG_LOG(L_DEBUG, "Caching alphabet\n");
                        stream.read((void *) m_symbolBuffer, m_blockInfo.ginfo.alphabet_size);
                        m_blockInfo.ginfo.mode = GDB_MODE_CACHED_ALPHABET;
                    } else {
                        //Skip the alphabet if we don't have enough size
                        stream.seekCur(m_blockInfo.ginfo.alphabet_size);
                    }
                    m_blockInfo.ginfo.data_offset = stream.position();
                    TAPE_DEBUG_LOG(L_DEBUG, "Pilot data in position %ld\n", stream.position());
                    m_blockInfo.ginfo.state = GDB_STATE_PILOT_GRP;
                case GDB_STATE_PILOT_GRP:
                    if (!m_blockInfo.ginfo.mode & GDB_MODE_CACHED_ALPHABET) {
                        stream.seekSet(m_blockInfo.ginfo.data_offset);
                    }
                    if (m_blockInfo.pilot_length-- > 0) {
                        m_blockInfo.ginfo.symbol = stream.read() & 0xff;
                        stream.read(&m_blockInfo.ginfo.u.pilot.repetitions, 2);
                        m_blockInfo.ginfo.data_offset += 3;
                        m_blockInfo.ginfo.state = GDB_STATE_PILOT_SYMBOL;
                    } else {
                        m_blockInfo.ginfo.state = GDB_STATE_BEFORE_DATA;
                        break;
                    }
                case GDB_STATE_PILOT_SYMBOL:
                    if (m_blockInfo.ginfo.u.pilot.repetitions-- > 0) {
                        if (!m_blockInfo.ginfo.mode & GDB_MODE_CACHED_ALPHABET) { 
                            stream.seekSet(m_blockInfo.ginfo.alphabet_offset + (m_blockInfo.ginfo.symbol * m_blockInfo.ginfo.symbol_size));
                            m_blockInfo.ginfo.symbol_flags = transformGdbFlags(stream.read() & 0xff);
                        } else {
                            m_blockInfo.ginfo.symbol_offset = m_symbolBuffer + (m_blockInfo.ginfo.symbol * m_blockInfo.ginfo.symbol_size);
                            m_blockInfo.ginfo.symbol_flags = transformGdbFlags(*m_blockInfo.ginfo.symbol_offset);
                            m_blockInfo.ginfo.symbol_offset++;
                        }
                        m_blockInfo.ginfo.pulses = 0;
                        m_blockInfo.ginfo.state = GDB_STATE_PILOT_PULSE;
                    } else {
                        m_blockInfo.ginfo.state = GDB_STATE_PILOT_GRP;
                        break;
                    }
                case GDB_STATE_PILOT_PULSE:
                    if (m_blockInfo.ginfo.pulses < m_blockInfo.ginfo.max_pulses_pilot_symbol) {
                        if (m_blockInfo.ginfo.mode & GDB_MODE_CACHED_ALPHABET) {
                            length = *((uint16_t*) m_blockInfo.ginfo.symbol_offset);
                        } else {
                            stream.read(&length, 2);
                        }
                        if (length > 0) {
                            writePulseWithFlags(pulseRenderer, {ENCTIME(length), m_blockInfo.ginfo.pulses == 0 ? m_blockInfo.ginfo.symbol_flags : (uint8_t) 0});
                            m_blockInfo.ginfo.pulses++;
                            m_blockInfo.ginfo.symbol_offset += 2;
                        } else {
                            m_blockInfo.ginfo.state = GDB_STATE_PILOT_SYMBOL;
                        }
                    } else {
                        m_blockInfo.ginfo.state = GDB_STATE_PILOT_SYMBOL;
                    }
                    break;
                case GDB_STATE_BEFORE_DATA:
                    m_blockInfo.ginfo.alphabet_offset = stream.position();
                    TAPE_DEBUG_LOG(L_DEBUG, "Data alphabet offset: %ld\n", m_blockInfo.ginfo.alphabet_offset);
                    TAPE_DEBUG_LOG(L_DEBUG, "Number of symbols: %d\n", m_blockInfo.ginfo.data_symbol_count);
                    TAPE_DEBUG_LOG(L_DEBUG, "Max pulses per data symbol: %d\n", m_blockInfo.ginfo.max_pulses_data_symbol);
                    m_blockInfo.ginfo.symbol_size = 1 + 2 * m_blockInfo.ginfo.max_pulses_data_symbol;
                    TAPE_DEBUG_LOG(L_DEBUG, "Symbol size: %d\n", m_blockInfo.ginfo.symbol_size);
                    m_blockInfo.ginfo.alphabet_size = m_blockInfo.ginfo.data_symbol_count ? 
                        m_blockInfo.ginfo.data_symbol_count * m_blockInfo.ginfo.symbol_size :
                        256 * m_blockInfo.ginfo.symbol_size;
                    TAPE_DEBUG_LOG(L_DEBUG, "Alphabet size: %d\n", m_blockInfo.ginfo.alphabet_size);
                    if (m_blockInfo.ginfo.alphabet_size < TZX_SYMBOL_BUFFER_SIZE) {
                        TAPE_DEBUG_LOG(L_DEBUG, "Caching data alphabet with size: %d\n", m_blockInfo.ginfo.alphabet_size);
                        stream.read((void *) m_symbolBuffer, m_blockInfo.ginfo.alphabet_size);
                        m_blockInfo.ginfo.mode = GDB_MODE_CACHED_ALPHABET;
                        m_blockInfo.ginfo.u.data.data_buffer_offset = m_blockInfo.ginfo.alphabet_size;
                        m_blockInfo.ginfo.u.data.data_buffer_size = TZX_SYMBOL_BUFFER_SIZE - m_blockInfo.ginfo.alphabet_size;
                        TAPE_DEBUG_LOG(L_DEBUG, "Data offset in buffer: %d\n", m_blockInfo.ginfo.u.data.data_buffer_offset);
                        TAPE_DEBUG_LOG(L_DEBUG, "Room for stream data: %d\n", m_blockInfo.ginfo.u.data.data_buffer_size);
                    } else {
                        stream.seekCur(m_blockInfo.ginfo.alphabet_size);
                        //Keep size in the buffer to cache the current symbol
                        m_blockInfo.ginfo.u.data.data_buffer_offset = m_blockInfo.ginfo.symbol_size;
                        m_blockInfo.ginfo.u.data.data_buffer_size = TZX_SYMBOL_BUFFER_SIZE - m_blockInfo.ginfo.symbol_size;
                        m_blockInfo.ginfo.mode = 0;
                    }
                    m_blockInfo.ginfo.data_offset = stream.position();
                    TAPE_DEBUG_LOG(L_DEBUG, "Offset at data start: %ld\n", m_blockInfo.ginfo.data_offset);
                    m_blockInfo.ginfo.u.data.bits_per_symbol = log2(m_blockInfo.ginfo.data_symbol_count);
                    //DEBUGMV(F("Bits per symbol: "), gdata_block_info.u.data.bits_per_symbol);
                    m_blockInfo.ginfo.state = GDB_STATE_DATA_NEXTBLOCK;
                case GDB_STATE_DATA_NEXTBLOCK:
                    if (!m_blockInfo.ginfo.mode & GDB_MODE_CACHED_ALPHABET) {
                        stream.seekSet(m_blockInfo.ginfo.data_offset);
                    }
                    m_blockInfo.ginfo.u.data.block_bytes = stream.read(m_symbolBuffer + m_blockInfo.ginfo.u.data.data_buffer_offset, 
                        m_blockInfo.ginfo.u.data.data_buffer_size);
                    m_blockInfo.ginfo.data_offset += m_blockInfo.ginfo.u.data.block_bytes;
                    m_blockInfo.ginfo.u.data.byte_index = 0;
                    m_blockInfo.ginfo.state = GDB_STATE_DATA_BYTE;
                case GDB_STATE_DATA_BYTE:
                    if (m_blockInfo.ginfo.u.data.block_bytes -- > 0) {
                        m_blockInfo.current_value = *(m_symbolBuffer + m_blockInfo.ginfo.u.data.data_buffer_offset + m_blockInfo.ginfo.u.data.byte_index);
                        m_blockInfo.ginfo.u.data.remaining_symbols = 8 / m_blockInfo.ginfo.u.data.bits_per_symbol;
                        m_blockInfo.ginfo.u.data.byte_index++;
                        m_blockInfo.ginfo.state = GDB_STATE_DATA_SYMBOL;
                    } else {
                        m_blockInfo.ginfo.state = GDB_STATE_DATA_NEXTBLOCK;
                        break;
                    }
                case GDB_STATE_DATA_SYMBOL:
                    if (m_blockInfo.ginfo.data_length > 0) {
                        if (m_blockInfo.ginfo.u.data.remaining_symbols-- > 0) {
                            m_blockInfo.ginfo.data_length--;
                            m_blockInfo.ginfo.symbol = (m_blockInfo.current_value & 
                                SYMBOL_MASK[m_blockInfo.ginfo.u.data.bits_per_symbol - 1]) >> (8 - m_blockInfo.ginfo.u.data.bits_per_symbol);
                            m_blockInfo.current_value <<= m_blockInfo.ginfo.u.data.bits_per_symbol;
                            if (!m_blockInfo.ginfo.mode & GDB_MODE_CACHED_ALPHABET) {
                                stream.seekSet(m_blockInfo.ginfo.alphabet_offset + (m_blockInfo.ginfo.symbol * m_blockInfo.ginfo.symbol_size));
                                //Get the whole symbol into the buffer
                                stream.read(m_symbolBuffer, m_blockInfo.ginfo.symbol_size);
                                m_blockInfo.ginfo.symbol_offset = m_symbolBuffer;
                            } else {
                                m_blockInfo.ginfo.symbol_offset = m_symbolBuffer + (m_blockInfo.ginfo.symbol * m_blockInfo.ginfo.symbol_size); 
                            }
                            m_blockInfo.ginfo.symbol_flags = transformGdbFlags(*m_blockInfo.ginfo.symbol_offset);
                            m_blockInfo.ginfo.symbol_offset++;
                            m_blockInfo.ginfo.pulses = 0;
                            m_blockInfo.ginfo.state = GDB_STATE_DATA_PULSE;
                        } else {
                            m_blockInfo.ginfo.state = GDB_STATE_DATA_BYTE;
                        }
                    } else {
                        m_blockInfo.ginfo.state = GDB_STATE_BEFORE_PAUSE;
                    }
                    break;
                case GDB_STATE_DATA_PULSE:
                    if (m_blockInfo.ginfo.pulses < m_blockInfo.ginfo.max_pulses_data_symbol) {
                        length = * ((uint16_t *) m_blockInfo.ginfo.symbol_offset);
                        if (length > 0) {
                            writePulseWithFlags(pulseRenderer, {ENCTIME(length), m_blockInfo.ginfo.pulses == 0 ? m_blockInfo.ginfo.symbol_flags : (uint8_t) 0});
                            m_blockInfo.ginfo.pulses++;
                            m_blockInfo.ginfo.symbol_offset += 2;
                        } else {
                            m_blockInfo.ginfo.state = GDB_STATE_DATA_SYMBOL;
                        }
                    } else {
                        m_blockInfo.ginfo.state = GDB_STATE_DATA_SYMBOL;
                    }
                    break;
                case GDB_STATE_BEFORE_PAUSE:
                    if (m_blockInfo.end_pause > 0) {
                        TAPE_DEBUG_LOG(L_DEBUG, "Final pause: %d\n", m_blockInfo.end_pause);
                        writePulseWithFlags(pulseRenderer, {m_blockInfo.end_pause, PulseRenderer::END_PAUSE});
                    }
                    TAPE_DEBUG_LOG(L_DEBUG, "Finished block data offset: %ld\n", m_blockInfo.ginfo.data_offset);
                    TAPE_DEBUG_LOG(L_DEBUG, "Seeking to: %ld\n", m_blockInfo.ginfo.end_offset);
                    stream.seekSet(m_blockInfo.ginfo.end_offset);
                    m_context.current_block++;
                    m_tzxState = TZX_STATE_FINDBLOCK;
                    m_context.blockId = 0;
                }
                break;
            }

            if (m_blockInfo.sinfo.pending_bits > 0) {
                writePulseWithFlags(pulseRenderer, {(m_blockInfo.current_value & 0x80) ? 
                    m_blockInfo.sinfo.one_length : m_blockInfo.sinfo.zero_length, PulseRenderer::VALUE_PULSE});
                m_blockInfo.current_value <<= 1;
                m_blockInfo.sinfo.pending_bits--;
            } else {
                if (m_blockInfo.sinfo.pilot_pulses > 0) {
                    writePulseWithFlags(pulseRenderer, {m_blockInfo.pilot_length, 0});
                    m_blockInfo.sinfo.pilot_pulses--;
                } else if (m_blockInfo.sinfo.sync1_length > 0) {
                    writePulseWithFlags(pulseRenderer, {m_blockInfo.sinfo.sync1_length, 0});
                    m_blockInfo.sinfo.sync1_length = 0;
                } else if (m_blockInfo.sinfo.sync2_length > 0) {
                    writePulseWithFlags(pulseRenderer, {m_blockInfo.sinfo.sync2_length, 0});
                    m_blockInfo.sinfo.sync2_length = 0;
                } else if (m_blockInfo.size > 0) {
                    next_value = stream.read();
                    if (next_value > -1) {
                        m_blockInfo.current_value = next_value & 0xff;
                        m_blockInfo.sinfo.pending_bits = m_blockInfo.size == 1 ? 
                        m_blockInfo.sinfo.bits_last_byte : 8;
                        m_blockInfo.size--;
                    } else {
                        TAPE_DEBUG_LOG(L_ERROR, "Unexpected EOF\n");
                        m_tzxState = TZX_STATE_ERROR;
                        break;
                    }
                } else {
                    //Handle final pause
                    if (m_blockInfo.end_pause > 0) {
                        TAPE_DEBUG_LOG(L_INFO, "Final pause: %d\n", m_blockInfo.end_pause);
                        writePulseWithFlags(pulseRenderer, {m_blockInfo.end_pause, PulseRenderer::END_PAUSE});
                    }
                    m_context.current_block++;
                    m_tzxState = TZX_STATE_FINDBLOCK;
                }
            }
        }
        break;
    case TZX_STATE_ERROR:
        break;
    default:
        ;
    }
}

