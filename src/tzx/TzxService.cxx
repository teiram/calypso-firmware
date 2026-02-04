#include "TzxService.h"
#include "tzxdefinitions.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include <cstring>
#include "calypso-debug.h"

using namespace calypso;


#define ENCTIME(value) (uint16_t) ((((float) value) / 3.5) + 0.5)
#define sinfo  info.standard_block
#define ginfo  info.generalized_block

static int64_t isr_handler(alarm_id_t alarm_id, void *user_data) {
    TzxService* p = (TzxService*) user_data;
    p->isrHandler();
    return 0;
}

const char* TzxService::name() {
    return NAME;
}

TzxService::TzxService(transition_t buffer[], uint8_t gpio):
    m_buffer(buffer, m_context),
    m_gpio(gpio),
    m_tzxState(TZX_STATE_IDLE) {
    
    memset((void*) &m_context, 0, sizeof(tzx_context_t));
}

bool TzxService::init() {
    TZX_DEBUG_LOG(L_DEBUG, "TzxService::init. Gpio=%d\n", m_gpio);
    gpio_init(m_gpio);
    gpio_set_dir(m_gpio, true);
    gpio_put(m_gpio, true);
    return true;
}

void TzxService::cleanup() {
}

const char* TzxService::currentStatus() {
    snprintf(m_currentStatus, 32, "B:%d/%d, T:%02x, P:%ld/%ld",
        m_context.current_block,
        m_numBlocks,
        m_context.blockId, 
        m_stream->position(),
        m_stream->size());

    return m_currentStatus;
}

uint8_t TzxService::startBlock() const {
    return m_startBlock;
}

void TzxService::setStartBlock(uint8_t startBlock) {
    if (startBlock < m_numBlocks) {
        m_startBlock = startBlock;
    }
}

void TzxService::togglePause() {
    switch (m_tzxState) {
        case TZX_STATE_IDLE:
        case TZX_STATE_PAUSING:
           return;
        case TZX_STATE_PAUSED:
            m_tzxState = TZX_STATE_RESUMING;
            break;
        default:
            m_tzxState = TZX_STATE_PAUSING;
    }
}

void TzxService::play() {
    TZX_DEBUG_LOG(L_DEBUG, "TzxService::play\n");
    if (m_tzxState == TZX_STATE_INITIALIZED) {
        m_context.isr_enabled = true;
        m_context.next_isr_time = 1000;
        isr_handler(0, this);
        m_tzxState = TZX_STATE_FINDBLOCK;
    }
}

void TzxService::insert(Stream *stream) {
    TZX_DEBUG_LOG(L_DEBUG, "TzxService::insert\n");
    m_stream = stream;
    m_stream->seekSet(10);
    calculateRelevantOffsets();
}

void TzxService::eject() {
    TZX_DEBUG_LOG(L_DEBUG, "TzxService::eject\n");
    m_stream = nullptr;
    m_context.isr_enabled = false;
}

bool TzxService::tapeLoaded() const {
    return m_stream != nullptr && m_tzxState == TZX_STATE_INITIALIZED;
}

bool TzxService::playing() const {
    return m_tzxState == TZX_STATE_FINDBLOCK || m_tzxState == TZX_STATE_READBLOCK;
}

bool TzxService::needsAttention() {
    return m_stream != nullptr && m_tzxState != TZX_STATE_IDLE && m_tzxState != TZX_STATE_ERROR;
}

uint8_t TzxService::log2(uint8_t value) {
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

inline bool TzxService::isBlockIncluded(uint8_t block, uint8_t relevant_blocks[]) {
    for (uint8_t i = 0; i < TZX_MAX_BLOCK_OFFSETS; i++) {
        if (relevant_blocks[i] == 255) {
            break;
        } else if (relevant_blocks[i] == block) {
            return true;
        }
    }
    return false;
}

void TzxService::calculateRelevantOffsets() {
    uint8_t relevant_blocks[TZX_MAX_BLOCK_OFFSETS];
  
    uint32_t saved_offset = m_stream->position();
    uint32_t block_size = 0;
    uint8_t num_blocks = 0;
    int16_t next_value = 0;
    memset(&relevant_blocks, 255, TZX_MAX_BLOCK_OFFSETS);
    for (uint8_t pass = PASS_FIND_BLOCKS; pass <= PASS_FIND_OFFSETS; pass++) {
        m_stream->seekSet(saved_offset);
        bool eof = false;
        uint8_t current_block = 0;
        uint8_t index = 0;
        if (pass == PASS_FIND_OFFSETS && relevant_blocks[0] == 255) {
            //No relevant block found
            break;
        }
        while (!eof) {
            if (pass == PASS_FIND_OFFSETS && isBlockIncluded(current_block, relevant_blocks)) {
                m_blockOffsets[index++] = {current_block, m_stream->position()};
                TZX_DEBUG_LOG(L_INFO, "Stored offset for block: %d\n", current_block);
            }
            next_value = m_stream->read();
            if (next_value > -1) {
                m_context.blockId = next_value & 0xff;
                TZX_DEBUG_LOG(L_DEBUG, "Block Id: 0x%02x\n", m_context.blockId);       
                switch (m_context.blockId) {
                case TZX_PUREDATA_BLOCK: {
                    tzx_puredata_block_t *blockp = (tzx_puredata_block_t *) m_inputBuffer;
                    m_stream->read((void *) blockp, sizeof(tzx_puredata_block_t));
                    block_size = blockp->data_length[0] + 
                        ((uint32_t) blockp->data_length[1] * 256) +
                        ((uint32_t) blockp->data_length[2] * 65536);
                    break;
                }
                case TZX_STANDARD_SPEED_BLOCK: {
                    tzx_standard_block_t *blockp = (tzx_standard_block_t *) m_inputBuffer;
                    m_stream->read((void*) blockp, sizeof(tzx_standard_block_t));
                    block_size = blockp->data_length;
                    break;
                }
                case TZX_TURBO_SPEED_BLOCK: {
                    tzx_turbospeed_block_t *blockp =(tzx_turbospeed_block_t *) m_inputBuffer;
                    m_stream->read((void*) blockp, sizeof(tzx_turbospeed_block_t));
                    TZX_DEBUG_DUMP(L_DEBUG, "turbo", blockp, sizeof(tzx_turbospeed_block_t));
                    block_size = blockp->data_length[0] + 
                        ((uint32_t) blockp->data_length[1] * 256) +
                        ((uint32_t) blockp->data_length[2] * 65536);
                    break;
                }
                case TZX_GENERALIZED_DATA_BLOCK: {
                    tzx_generalized_data_block_t *blockp = (tzx_generalized_data_block_t*) m_inputBuffer;
                    m_stream->read((void *) blockp, sizeof(tzx_generalized_data_block_t));
                    block_size = blockp->block_length - sizeof(tzx_generalized_data_block_t) + 4;
                    break;
                }
                case TZX_MESSAGE_BLOCK:
                    next_value = m_stream->read(); 
                case TZX_TEXT_DESCRIPTION:
                case TZX_GROUP_START:
                    next_value = m_stream->read();
                    block_size = next_value & 0xff;
                    break;
                case TZX_GROUP_END:
                    block_size = 0;
                    break;
                case TZX_ARCHIVE_INFO:
                    m_stream->read(&block_size, 2);
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
                    block_size = (m_stream->read() & 0xff) * 2;
                    break;
                case TZX_PAUSE:
                    block_size = 2;  
                    break;
                case TZX_CUSTOM_INFO: {
                    uint32_t offset;
                    m_stream->seekCur(16); //Skip ID string 16 chars
                    m_stream->read(&offset, sizeof(offset));
                    block_size = offset;
                    break;
                }    
                case TZX_GLUE_BLOCK:
                    block_size = 9;
                    break;
                case TZX_CALL_SEQUENCE: {
                    m_stream->read(&block_size, 2);
                    if (pass == PASS_FIND_BLOCKS) {
                        do {
                            m_stream->read(&next_value, 2);
                            if (!isBlockIncluded(current_block + next_value, relevant_blocks)) {
                                relevant_blocks[index] = current_block + next_value;
                                TZX_DEBUG_LOG(L_INFO, "Found sequence call jump destination: %d\n", relevant_blocks[index]);
                                index++;
                            }
                        } while (--block_size > 0);
                    }
                    block_size = block_size * 2;
                    break;
                }
                case TZX_JUMP_TO_BLOCK: {
                    if (pass == PASS_FIND_BLOCKS) {
                        m_stream->read(&next_value, 2);
                        if (!isBlockIncluded(current_block + next_value, relevant_blocks)) {
                            relevant_blocks[index] = current_block + next_value;
                            TZX_DEBUG_LOG(L_INFO, "Found block jump destination: %d\n", relevant_blocks[index]);
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
                    TZX_DEBUG_LOG(L_WARN, "Unsupported block type: %d\n", m_context.blockId);
                    eof = true;
                }
                m_stream->seekCur(block_size);
                if (pass == PASS_FIND_BLOCKS) {
                    num_blocks = current_block;
                }
                if (current_block == 255) {
                    TZX_DEBUG_LOG(L_WARN, "Unable to calculate offsets over the first 255 blocks\n");
                    eof = true;
                }
                current_block++;
            } else {
                eof = true;
            }
        }
    }
    m_stream->seekSet(saved_offset);
    m_startBlock = 0;
    m_numBlocks = num_blocks;
    m_tzxState = TZX_STATE_INITIALIZED;
}

inline uint32_t TzxService::getBlockOffset(uint8_t block) {
    block_offset_t offset;
    for (uint8_t i = 0; i < TZX_MAX_BLOCK_OFFSETS; i++) {
        if (m_blockOffsets[i].block == block) {
            return m_blockOffsets[i].offset;
        }
    }
    TZX_DEBUG_LOG(L_WARN, "Unable to find offset for block: %d\n");
    m_tzxState = TZX_STATE_ERROR;
    return 0;
}

inline uint8_t TzxService::transformGdbFlags(uint8_t flags) {
    switch (flags) {
    case 0:
        return 0;
    case 1:
        return VALUE_KEEP_POL;
    case 2:
        return VALUE_FORCE_ZERO;
    case 3:
        return VALUE_FORCE_ONE;
    default:
        return 255;
    }
}


void TzxService::attention() {
    int16_t next_value;
    switch (m_tzxState) {
    case TZX_STATE_DRAINING:
        if (m_buffer.available()) {
            break;
        } else {
            m_tzxState = TZX_STATE_PAUSING;
        }
    case TZX_STATE_PAUSING:
        if (m_context.isr_enabled) {
            m_context.isr_enabled = 0;
        }
        m_tzxState = TZX_STATE_PAUSED;
        break;
    case TZX_STATE_RESUMING:
        m_stream->seekSet(m_context.current_offset);
        m_tzxState = TZX_STATE_FINDBLOCK;
        break;
    case TZX_STATE_FINDBLOCK: {
        next_value = m_stream->read();
        if (next_value > -1) {
            m_context.blockId = next_value & 0xff;
            memset(&m_blockInfo, 0, sizeof(block_info_t));
            TZX_DEBUG_LOG(L_DEBUG, "Block type: 0x%02x, position:%ld\n", m_context.blockId, m_stream->position());

            m_context.last_offset = m_context.current_offset;
            m_context.current_offset = m_stream->position() - 1;

            switch (m_context.blockId) {
            case TZX_PUREDATA_BLOCK: {
                tzx_puredata_block_t *blockp = (tzx_puredata_block_t *) m_inputBuffer;
                int16_t readc = m_stream->read((void *) blockp, sizeof(tzx_puredata_block_t));   
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
                    TZX_DEBUG_LOG(L_WARN, "Unable to read block header\n");
                    m_tzxState = TZX_STATE_ERROR;
                }
                break;
            }
            case TZX_STANDARD_SPEED_BLOCK: {
                tzx_standard_block_t *blockp = (tzx_standard_block_t *) m_inputBuffer;
                int16_t readc = m_stream->read((void*) blockp, sizeof(tzx_standard_block_t));
                if (readc == sizeof(tzx_standard_block_t)) {
                    m_blockInfo.sinfo.zero_length = STD_ZERO_LENGTH;
                    m_blockInfo.sinfo.one_length = STD_ONE_LENGTH;
                    m_blockInfo.sinfo.bits_last_byte = 8;
                    m_blockInfo.end_pause = blockp->end_pause;
                    m_blockInfo.size = blockp->data_length;
                    m_blockInfo.pilot_length = STD_LEADER_LENGTH;
                    m_blockInfo.sinfo.sync1_length = STD_SYNC1_LENGTH;
                    m_blockInfo.sinfo.sync2_length = STD_SYNC2_LENGTH;
                    next_value = m_stream->read(); //Get the type of block
                    if (next_value > -1) {
                        m_blockInfo.sinfo.block_type = next_value & 0xff;
                        m_blockInfo.sinfo.pilot_pulses = m_blockInfo.sinfo.block_type < 128 ? 
                            STD_HEADER_PULSES : STD_DATA_PULSES;
                        m_stream->seekCur(-1); //Compensate for block_type prefetch
                        m_tzxState = TZX_STATE_READBLOCK;
                    } else {
                        TZX_DEBUG_LOG(L_INFO, "!EOF\n");
                        m_tzxState = TZX_STATE_ERROR;
                    }
                }
                break;
            }
            case TZX_TURBO_SPEED_BLOCK: {
                tzx_turbospeed_block_t *blockp =(tzx_turbospeed_block_t *) m_inputBuffer;
                int16_t readc = m_stream->read((void*) blockp, sizeof(tzx_turbospeed_block_t));
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
                int16_t readc = m_stream->read((void *) blockp, sizeof(tzx_generalized_data_block_t));
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
                    m_blockInfo.ginfo.end_offset = m_stream->position() + m_blockInfo.size - sizeof(tzx_generalized_data_block_t) + 4;
                    m_tzxState = TZX_STATE_READBLOCK;
                }
                break;
            }
            case TZX_TEXT_DESCRIPTION:
            case TZX_GROUP_START: {
                next_value = m_stream->read();
                m_stream->seekCur(next_value & 0xff);
                m_tzxState = TZX_STATE_READBLOCK;
                m_context.current_block++;
                break;
            }
            case TZX_HARDWARE_TYPE: {
                next_value = m_stream->read();
                m_stream->seekCur(3 * (next_value & 0xff));
                m_tzxState = TZX_STATE_FINDBLOCK;
                m_context.current_block++;
                break;
            }
            case TZX_MESSAGE_BLOCK: {
                m_stream->seekCur(1); //Skip time to display of message
                next_value = m_stream->read(); //Length of text message
                m_stream->seekCur(next_value & 0xff);
                m_tzxState = TZX_STATE_FINDBLOCK;
                break;
            }            
            case TZX_GROUP_END: {
                m_tzxState = TZX_STATE_READBLOCK;
                m_context.current_block++;
                break;
            }
            case TZX_STOP_48K_MODE:
                m_stream->seekCur(4);
                if (m_context.mode48k) {
                    m_tzxState = TZX_STATE_PAUSING;
                    //Set the offset for the next block to resume properly
                    m_context.current_offset = m_stream->position();
                } else {
                    m_tzxState = TZX_STATE_FINDBLOCK;
                }
                m_context.current_block++;
                break;
            case TZX_SET_SIGNAL_LEVEL:
                m_stream->seekCur(4);
                next_value = m_stream->read(); //1 to force high, 0 to force low
                m_context.next_pulse_flags = (next_value & 0xff) ? VALUE_FORCE_ONE : VALUE_FORCE_ZERO;
                m_tzxState = TZX_STATE_FINDBLOCK;
                m_context.current_block++;
                break;                     
            case TZX_ARCHIVE_INFO: {
                uint16_t* skip = (uint16_t*) m_inputBuffer;
                if (m_stream->read((void*) skip, 2) > -1) {
                    m_stream->seekCur(*skip);
                    m_tzxState = TZX_STATE_FINDBLOCK;
                    m_context.current_block++;
                } else {
                    m_tzxState = TZX_STATE_ERROR;
                }
                break;
            }
            case TZX_LOOP_START: {
                m_stream->read(&(m_context.loop_count), 2);
                m_context.loop_start = m_stream->position();
                m_context.current_block++;
                m_context.loop_start_block = m_context.current_block;
                m_tzxState = TZX_STATE_FINDBLOCK;
                break;
            }
            case TZX_LOOP_END: {
                if (m_context.loop_start > 0) {
                    if (m_context.loop_count-- > 0) {
                        m_stream->seekSet(m_context.loop_start);
                        m_context.current_block = m_context.loop_start_block;
                    } else {
                        m_context.loop_start = 0;
                        m_context.current_block++;
                    }
                    m_tzxState = TZX_STATE_FINDBLOCK;
                } else {
                    TZX_DEBUG_LOG(L_ERROR, "Unmatched loop end\n");
                    m_tzxState = TZX_STATE_ERROR;
                }
                break;
            }
            case TZX_PURE_TONE: {
                tzx_puretone_block_t *blockp =(tzx_puretone_block_t *) m_inputBuffer;
                int16_t readc = m_stream->read((void*) blockp, sizeof(tzx_puretone_block_t));
                if (readc == sizeof(tzx_puretone_block_t)) {
                    m_blockInfo.size = blockp->pulse_count;
                    m_blockInfo.pilot_length = ENCTIME(blockp->pulse_length);
                    m_tzxState = TZX_STATE_READBLOCK;
                }
                break;
            }
            case TZX_PULSE_SEQUENCE: {
                next_value = m_stream->read();
                m_blockInfo.size = next_value & 0xff;
                m_tzxState = TZX_STATE_READBLOCK;
                break;
            }
            case TZX_PAUSE: {
                uint16_t pause_value;
                m_stream->read(&pause_value, 2);
                m_blockInfo.size = 0;
                m_blockInfo.end_pause = pause_value;
                if (m_blockInfo.end_pause) {
                    m_tzxState = TZX_STATE_READBLOCK;
                } else {
                    m_tzxState = TZX_STATE_DRAINING;
                    m_context.current_offset = m_stream->position();
                    m_context.current_block++;
                }
                break;
            }
            case TZX_CUSTOM_INFO: {
                uint32_t offset;
                m_stream->seekCur(16); //Skip ID string 16 chars
                m_stream->read(&offset, sizeof(offset));
                m_stream->seekCur(offset);
                m_context.current_block++;
                m_tzxState = TZX_STATE_FINDBLOCK;
                break;
            }
            case TZX_GLUE_BLOCK: {
                m_stream->seekCur(9);  //Skip 9 bytes of description of "Glue" Block
                m_tzxState = TZX_STATE_FINDBLOCK;
                m_context.current_block++;
                break;
            }
            case TZX_CALL_SEQUENCE: {
                m_stream->read(&m_context.call_count, 2);
                m_context.next_call_offset = m_stream->position();
                m_context.call_start_block = m_context.current_block;
                TZX_DEBUG_LOG(L_DEBUG, "Call count: %d, next call offset :%d\n", m_context.call_count, m_context.next_call_offset);
                m_context.call_count--;
            }
            JUMP_TO_BLOCK:
            case TZX_JUMP_TO_BLOCK: {
                int16_t* offset = (int16_t*) m_inputBuffer;
                if (m_stream->read((void*) offset, 2) > -1) {
                    TZX_DEBUG_LOG(L_DEBUG, "Jumping to block got offset: %d, in block %d\n", *offset, m_context.current_block);
                    m_context.current_block = m_context.current_block + *offset;
                    TZX_DEBUG_LOG(L_DEBUG, "Jumping to block: %d\n", m_context.current_block);
                    m_stream->seekSet(getBlockOffset(m_context.current_block));
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
                    TZX_DEBUG_LOG(L_DEBUG, "On return from sequence and next offset: %d\n", m_context.next_call_offset);
                    goto JUMP_TO_BLOCK;
                } else {
                    m_stream->seekSet(m_context.next_call_offset);
                    m_context.next_call_offset = 0;
                    m_context.current_block = m_context.call_start_block + 1;
                }
                m_tzxState = TZX_STATE_FINDBLOCK;
                break; 
            }
            default: {
                m_tzxState = TZX_STATE_ERROR;
                TZX_DEBUG_LOG(L_WARN, "Unsupported block type: %d\n", m_context.blockId);
            }
        }
    } else {
        TZX_DEBUG_LOG(L_INFO, "EOF\n");
        m_tzxState = TZX_STATE_CLOSE;
    }
    }
    if (m_tzxState == TZX_STATE_READBLOCK) {
        TZX_DEBUG_LOG(L_DEBUG, "Block size: %d\n", m_blockInfo.size);
    }
    break;

    case TZX_STATE_READBLOCK:
        if (!m_buffer.full()) {
            if (m_context.blockId == TZX_PULSE_SEQUENCE) {
                if (m_blockInfo.size-- > 0) {
                    uint16_t pulse_length;
                    m_stream->read(&pulse_length, 2);
                    m_buffer.write({ENCTIME(pulse_length), 0});
                } else {
                    m_context.current_block++;
                    m_tzxState = TZX_STATE_FINDBLOCK;
                }
                break;
            }
            if (m_context.blockId == TZX_PURE_TONE) {
                if (m_blockInfo.size-- > 0) {
                    m_buffer.write({m_blockInfo.pilot_length, 0});
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
                    m_blockInfo.ginfo.alphabet_offset = m_stream->position();
                    m_blockInfo.ginfo.symbol_size = 1 + 2 * m_blockInfo.ginfo.max_pulses_pilot_symbol;
                    TZX_DEBUG_LOG(L_DEBUG, "Pilot alphabet in position %ld\n", m_stream->position());
                    m_blockInfo.ginfo.alphabet_size = m_blockInfo.ginfo.pilot_symbol_count ? 
                        m_blockInfo.ginfo.pilot_symbol_count * m_blockInfo.ginfo.symbol_size :
                        256 * m_blockInfo.ginfo.symbol_size;
                    TZX_DEBUG_LOG(L_DEBUG, "Alphabet size: %d\n", m_blockInfo.ginfo.alphabet_size);
                    if (m_blockInfo.ginfo.alphabet_size < TZX_SYMBOL_BUFFER_SIZE) {
                        //Cache the alphabet
                        TZX_DEBUG_LOG(L_DEBUG, "Caching alphabet\n");
                        m_stream->read((void *) m_symbolBuffer, m_blockInfo.ginfo.alphabet_size);
                        m_blockInfo.ginfo.mode = GDB_MODE_CACHED_ALPHABET;
                    } else {
                        //Skip the alphabet if we don't have enough size
                        m_stream->seekCur(m_blockInfo.ginfo.alphabet_size);
                    }
                    m_blockInfo.ginfo.data_offset = m_stream->position();
                    TZX_DEBUG_LOG(L_DEBUG, "Pilot data in position %ld\n", m_stream->position());
                    m_blockInfo.ginfo.state = GDB_STATE_PILOT_GRP;
                case GDB_STATE_PILOT_GRP:
                    if (!m_blockInfo.ginfo.mode & GDB_MODE_CACHED_ALPHABET) {
                        m_stream->seekSet(m_blockInfo.ginfo.data_offset);
                    }
                    if (m_blockInfo.pilot_length-- > 0) {
                        m_blockInfo.ginfo.symbol = m_stream->read() & 0xff;
                        m_stream->read(&m_blockInfo.ginfo.u.pilot.repetitions, 2);
                        m_blockInfo.ginfo.data_offset += 3;
                        m_blockInfo.ginfo.state = GDB_STATE_PILOT_SYMBOL;
                    } else {
                        m_blockInfo.ginfo.state = GDB_STATE_BEFORE_DATA;
                        break;
                    }
                case GDB_STATE_PILOT_SYMBOL:
                    if (m_blockInfo.ginfo.u.pilot.repetitions-- > 0) {
                        if (!m_blockInfo.ginfo.mode & GDB_MODE_CACHED_ALPHABET) { 
                            m_stream->seekSet(m_blockInfo.ginfo.alphabet_offset + (m_blockInfo.ginfo.symbol * m_blockInfo.ginfo.symbol_size));
                            m_blockInfo.ginfo.symbol_flags = transformGdbFlags(m_stream->read() & 0xff);
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
                            m_stream->read(&length, 2);
                        }
                        if (length > 0) {
                            m_buffer.write({ENCTIME(length), m_blockInfo.ginfo.pulses == 0 ? m_blockInfo.ginfo.symbol_flags : (uint8_t) 0});
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
                    m_blockInfo.ginfo.alphabet_offset = m_stream->position();
                    TZX_DEBUG_LOG(L_DEBUG, "Data alphabet offset: %ld\n", m_blockInfo.ginfo.alphabet_offset);
                    TZX_DEBUG_LOG(L_DEBUG, "Number of symbols: %d\n", m_blockInfo.ginfo.data_symbol_count);
                    TZX_DEBUG_LOG(L_DEBUG, "Max pulses per data symbol: %d\n", m_blockInfo.ginfo.max_pulses_data_symbol);
                    m_blockInfo.ginfo.symbol_size = 1 + 2 * m_blockInfo.ginfo.max_pulses_data_symbol;
                    TZX_DEBUG_LOG(L_DEBUG, "Symbol size: %d\n", m_blockInfo.ginfo.symbol_size);
                    m_blockInfo.ginfo.alphabet_size = m_blockInfo.ginfo.data_symbol_count ? 
                        m_blockInfo.ginfo.data_symbol_count * m_blockInfo.ginfo.symbol_size :
                        256 * m_blockInfo.ginfo.symbol_size;
                    TZX_DEBUG_LOG(L_DEBUG, "Alphabet size: %d\n", m_blockInfo.ginfo.alphabet_size);
                    if (m_blockInfo.ginfo.alphabet_size < TZX_SYMBOL_BUFFER_SIZE) {
                        TZX_DEBUG_LOG(L_DEBUG, "Caching data alphabet with size: %d\n", m_blockInfo.ginfo.alphabet_size);
                        m_stream->read((void *) m_symbolBuffer, m_blockInfo.ginfo.alphabet_size);
                        m_blockInfo.ginfo.mode = GDB_MODE_CACHED_ALPHABET;
                        m_blockInfo.ginfo.u.data.data_buffer_offset = m_blockInfo.ginfo.alphabet_size;
                        m_blockInfo.ginfo.u.data.data_buffer_size = TZX_SYMBOL_BUFFER_SIZE - m_blockInfo.ginfo.alphabet_size;
                        TZX_DEBUG_LOG(L_DEBUG, "Data offset in buffer: %d\n", m_blockInfo.ginfo.u.data.data_buffer_offset);
                        TZX_DEBUG_LOG(L_DEBUG, "Room for stream data: %d\n", m_blockInfo.ginfo.u.data.data_buffer_size);
                    } else {
                        m_stream->seekCur(m_blockInfo.ginfo.alphabet_size);
                        //Keep size in the buffer to cache the current symbol
                        m_blockInfo.ginfo.u.data.data_buffer_offset = m_blockInfo.ginfo.symbol_size;
                        m_blockInfo.ginfo.u.data.data_buffer_size = TZX_SYMBOL_BUFFER_SIZE - m_blockInfo.ginfo.symbol_size;
                        m_blockInfo.ginfo.mode = 0;
                    }
                    m_blockInfo.ginfo.data_offset = m_stream->position();
                    TZX_DEBUG_LOG(L_DEBUG, "Offset at data start: %ld\n", m_blockInfo.ginfo.data_offset);
                    m_blockInfo.ginfo.u.data.bits_per_symbol = log2(m_blockInfo.ginfo.data_symbol_count);
                    //DEBUGMV(F("Bits per symbol: "), gdata_block_info.u.data.bits_per_symbol);
                    m_blockInfo.ginfo.state = GDB_STATE_DATA_NEXTBLOCK;
                case GDB_STATE_DATA_NEXTBLOCK:
                    if (!m_blockInfo.ginfo.mode & GDB_MODE_CACHED_ALPHABET) {
                        m_stream->seekSet(m_blockInfo.ginfo.data_offset);
                    }
                    m_blockInfo.ginfo.u.data.block_bytes = m_stream->read(m_symbolBuffer + m_blockInfo.ginfo.u.data.data_buffer_offset, 
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
                                m_stream->seekSet(m_blockInfo.ginfo.alphabet_offset + (m_blockInfo.ginfo.symbol * m_blockInfo.ginfo.symbol_size));
                                //Get the whole symbol into the buffer
                                m_stream->read(m_symbolBuffer, m_blockInfo.ginfo.symbol_size);
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
                            m_buffer.write({ENCTIME(length), m_blockInfo.ginfo.pulses == 0 ? m_blockInfo.ginfo.symbol_flags : (uint8_t) 0});
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
                        TZX_DEBUG_LOG(L_DEBUG, "Final pause: %d\n", m_blockInfo.end_pause);
                        m_buffer.write({m_blockInfo.end_pause, END_PAUSE});
                    }
                    TZX_DEBUG_LOG(L_DEBUG, "Finished block data offset: %ld\n", m_blockInfo.ginfo.data_offset);
                    TZX_DEBUG_LOG(L_DEBUG, "Seeking to: %ld\n", m_blockInfo.ginfo.end_offset);
                    m_stream->seekSet(m_blockInfo.ginfo.end_offset);
                    m_context.current_block++;
                    m_tzxState = TZX_STATE_FINDBLOCK;
                    m_context.blockId = 0;
                }
                break;
            }

            if (m_blockInfo.sinfo.pending_bits > 0) {
                m_buffer.write({(m_blockInfo.current_value & 0x80) ? 
                    m_blockInfo.sinfo.one_length : m_blockInfo.sinfo.zero_length, VALUE_PULSE});
                m_blockInfo.current_value <<= 1;
                m_blockInfo.sinfo.pending_bits--;
            } else {
                if (m_blockInfo.sinfo.pilot_pulses > 0) {
                    m_buffer.write({m_blockInfo.pilot_length, 0});
                    m_blockInfo.sinfo.pilot_pulses--;
                } else if (m_blockInfo.sinfo.sync1_length > 0) {
                    m_buffer.write({m_blockInfo.sinfo.sync1_length, 0});
                    m_blockInfo.sinfo.sync1_length = 0;
                } else if (m_blockInfo.sinfo.sync2_length > 0) {
                    m_buffer.write({m_blockInfo.sinfo.sync2_length, 0});
                    m_blockInfo.sinfo.sync2_length = 0;
                } else if (m_blockInfo.size > 0) {
                    next_value = m_stream->read();
                    if (next_value > -1) {
                        m_blockInfo.current_value = next_value & 0xff;
                        m_blockInfo.sinfo.pending_bits = m_blockInfo.size == 1 ? 
                        m_blockInfo.sinfo.bits_last_byte : 8;
                        m_blockInfo.size--;
                    } else {
                        TZX_DEBUG_LOG(L_ERROR, "Unexpected EOF\n");
                        m_tzxState = TZX_STATE_ERROR;
                        break;
                    }
                } else {
                    //Handle final pause
                    if (m_blockInfo.end_pause > 0) {
                        TZX_DEBUG_LOG(L_INFO, "Final pause: %d\n", m_blockInfo.end_pause);
                        m_buffer.write({m_blockInfo.end_pause, END_PAUSE});
                    }
                    m_context.current_block++;
                    m_tzxState = TZX_STATE_FINDBLOCK;
                }
            }
        }
        break;
    case TZX_STATE_ERROR:
    case TZX_STATE_CLOSE:
        m_blockInfo.size = 0;
        m_blockInfo.end_pause = 0;
        m_blockInfo.sinfo.pilot_pulses = 1;
        m_blockInfo.pilot_length = 1000;
        m_context.isr_enabled = false;
        TZX_DEBUG_LOG(L_DEBUG, "Closedown!\n");
        m_tzxState = TZX_STATE_CLOSEDOWN;
        break;
    default:
        ;
    }
  
    /*
    if (m_tzxState == TZX_STATE_PREBUFFER && m_buffer.full() && !m_context.isr_enabled) {
        m_context.isr_enabled = 1;
        m_tzxState == TZX_STATE_FINDBLOCK;
        isr_handler(0, this);
    }
    */
}


void TzxService::isrHandler() {
    if (m_context.isr_enabled) {
        if (alarm_pool_add_alarm_in_us(alarm_pool_get_default(), m_context.next_isr_time, isr_handler, this, false) < 0) {
            TZX_DEBUG_LOG(L_ERROR, "Error setting alarm for next transition\n");
        }
    }
    gpio_put(m_gpio, m_context.inverted_output ^ m_context.level);
 
    if (m_buffer.available()) {
        transition_t duration(*m_buffer.head());
        if (duration.flags & VALUE_PULSE) {
            if (m_context.visited++ == 1) {
                m_buffer.pop();
                m_context.visited = 0;
            }
        } else if ((duration.flags & END_PAUSE)) {
            if (m_context.visited++ == 0) {
                duration.value = 1500;
                duration.flags = 0;
                m_buffer.head()->value -= 1; //Compensate real pause transition
            } else {
                m_context.visited = 0;
                m_buffer.pop();
            }
        } else {
            m_buffer.pop();
        }

        if (duration.flags & END_PAUSE) {
            m_context.next_isr_time = ((uint32_t) duration.value) * 1000;
            m_context.level = 0;
        } else {
            m_context.next_isr_time = (uint32_t) duration.value;
            if (duration.flags & VALUE_FORCE_ZERO) {
                m_context.level = 0;
            } else if (duration.flags & VALUE_FORCE_ONE) {
                m_context.level = 1;
            } else if (!(duration.flags & VALUE_KEEP_POL)) {
                m_context.level = !m_context.level;
            }
        }

    } else if (m_tzxState == TZX_STATE_CLOSEDOWN) {
        if (m_context.visited++ == 0) {
            m_context.next_isr_time = 1000000;
            m_context.level = 0;
        } else {
            m_context.isr_enabled = false;
            m_tzxState = TZX_STATE_IDLE;
        }
    } else {
        // Buffer underrun
        TZX_DEBUG_LOG(L_WARN, "Buffer underrun\n");
        m_context.next_isr_time = 1000000;
    }
}
