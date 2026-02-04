#ifndef TZX_SERVICE_H
#define TZX_SERVICE_H

#include "Service.h"
#include "Stream.h"
#include "CircularBuffer.h"
#include "pico/sync.h"

namespace calypso {

    class TzxService: public Service {
    public:
        static constexpr uint8_t TZX_TRANSITION_BUFFER_SIZE = 64;
        
        typedef struct {
            uint16_t value;
            uint8_t flags:3;
        } __attribute__((packed)) transition_t;


    private:
        constexpr static const char NAME[] = {"TZXService"};
        static constexpr uint8_t TZX_MAX_BLOCK_OFFSETS = 8;
        static constexpr uint8_t PASS_FIND_BLOCKS = 0;
        static constexpr uint8_t PASS_FIND_OFFSETS = 1;
        static constexpr uint8_t TZX_INPUT_BUFFER_SIZE = 18;
        static constexpr uint8_t TZX_SYMBOL_BUFFER_SIZE = 128;

        static constexpr uint8_t END_PAUSE = 1 << 0;
        static constexpr uint8_t VALUE_PULSE = 1 << 1;
        static constexpr uint8_t VALUE_KEEP_POL = 1 << 2;
        static constexpr uint8_t VALUE_FORCE_ZERO = 1 << 3;
        static constexpr uint8_t VALUE_FORCE_ONE = 1 << 4;

        static constexpr uint8_t GDB_STATE_BEFORE_PILOT = 0;
        static constexpr uint8_t GDB_STATE_PILOT_GRP = 1;
        static constexpr uint8_t GDB_STATE_PILOT_SYMBOL = 2;
        static constexpr uint8_t GDB_STATE_PILOT_PULSE = 3;
        static constexpr uint8_t GDB_STATE_BEFORE_DATA = 4;
        static constexpr uint8_t GDB_STATE_DATA_NEXTBLOCK = 5;
        static constexpr uint8_t GDB_STATE_DATA_BYTE = 6;
        static constexpr uint8_t GDB_STATE_DATA_SYMBOL = 7;
        static constexpr uint8_t GDB_STATE_DATA_PULSE = 8;
        static constexpr uint8_t GDB_STATE_BEFORE_PAUSE = 9;
        static constexpr uint8_t GDB_MODE_CACHED_ALPHABET = 1;
        static constexpr uint8_t SYMBOL_MASK[] = {0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff};


        typedef struct {
            uint16_t pilot_pulses;
            uint16_t zero_length;
            uint16_t one_length;
            uint16_t sync1_length;
            uint16_t sync2_length;
            uint8_t block_type;
            uint8_t bits_last_byte:4;
            uint8_t pending_bits:4;
        } __attribute__((packed)) standard_block_t;

        typedef struct {
            uint8_t max_pulses_pilot_symbol;
            uint8_t pilot_symbol_count;
            uint8_t data_symbol_count;
            uint8_t max_pulses_data_symbol;

            uint32_t data_offset;
            uint32_t alphabet_offset;
            uint16_t alphabet_size;
            uint32_t end_offset;
            uint8_t symbol;
            uint8_t pulses;
            uint8_t symbol_flags;
            uint8_t symbol_size;
            uint8_t *symbol_offset;
            uint32_t data_length;
            union {
                struct {
                    uint16_t repetitions;
                } pilot;
                struct {
                    uint8_t data_buffer_offset;
                    uint8_t data_buffer_size;
                    uint8_t byte_index;
                    uint8_t block_bytes;
                    uint8_t remaining_symbols:4;
                    uint8_t bits_per_symbol:4;
                } data;
            } u;
          uint8_t state:4;
          uint8_t mode:1;
        } __attribute__((packed)) generalized_block_t;

        typedef struct {
            uint32_t size;
            uint16_t end_pause;
            uint8_t current_value;
            uint16_t pilot_length;
            union {
                standard_block_t standard_block;
                generalized_block_t generalized_block;
            } info;
        } __attribute__((packed)) block_info_t;

        typedef struct {
            uint8_t blockId;
            uint8_t current_block;
            uint32_t last_offset;
            uint32_t current_offset;
            uint32_t loop_start;
            uint16_t loop_count;
            uint8_t loop_start_block;
            uint32_t next_call_offset;
            uint16_t call_count;
            uint8_t call_start_block;
            uint32_t next_isr_time;
            uint8_t next_pulse_flags:5;
            uint8_t visited:1;
            uint8_t level:1;
            uint8_t mode48k:1;
            uint8_t inverted_output:1;
            uint8_t digital_output:1;
            uint8_t isr_enabled:1;
        } __attribute__((packed)) tzx_context_t;

        typedef struct {
            uint8_t block;
            uint32_t offset;
        } block_offset_t;
        
        typedef enum {
            TZX_STATE_IDLE,
            TZX_STATE_INITIALIZED,
            TZX_STATE_FINDBLOCK,
            TZX_STATE_READBLOCK,
            TZX_STATE_HANDLEJUMPS,
            TZX_STATE_ERROR,
            TZX_STATE_CLOSE,
            TZX_STATE_CLOSEDOWN,
            TZX_STATE_PAUSING,
            TZX_STATE_RESUMING,
            TZX_STATE_PAUSED,
            TZX_STATE_DRAINING
        } TzxState;

        class ContextAwareCircularBuffer: public CircularBuffer<transition_t> {
        public:
            ContextAwareCircularBuffer(transition_t buffer[], tzx_context_t& context):
                CircularBuffer(buffer, TZX_TRANSITION_BUFFER_SIZE),
                m_context(context) {
                    critical_section_init(&m_criticalSection);
                };
            
            void write(const transition_t& value) {
                critical_section_enter_blocking(&m_criticalSection);
                if (!full()) {
                    if (m_context.next_pulse_flags) {
                        transition_t mvalue(value);
                        mvalue.flags |= m_context.next_pulse_flags;
                        m_context.next_pulse_flags = 0;
                        CircularBuffer::write(mvalue);
                    } else {
                        CircularBuffer::write(value);
                    }
                }
                critical_section_exit(&m_criticalSection);
            }

            void pop() {
                critical_section_enter_blocking(&m_criticalSection);
                CircularBuffer::pop();
                critical_section_exit(&m_criticalSection);
            }
        private:
            tzx_context_t &m_context;
            critical_section m_criticalSection;
        };

        Stream* m_stream;
        ContextAwareCircularBuffer m_buffer;
        tzx_context_t m_context;
        block_info_t m_blockInfo;
        uint8_t m_gpio;
        volatile TzxState m_tzxState = TZX_STATE_IDLE;
        block_offset_t m_blockOffsets[TZX_MAX_BLOCK_OFFSETS];
        uint8_t m_inputBuffer[TZX_INPUT_BUFFER_SIZE];
        uint8_t m_symbolBuffer[TZX_SYMBOL_BUFFER_SIZE];

        uint8_t log2(uint8_t value);
        inline static bool isBlockIncluded(uint8_t block, uint8_t relevant_blocks[]);
        void calculateRelevantOffsets();
        uint32_t getBlockOffset(uint8_t block);
        uint8_t transformGdbFlags(uint8_t flags);
        char m_currentStatus[32];
        uint8_t m_startBlock;
        uint8_t m_numBlocks;
    public: 
        TzxService(transition_t buffer[], uint8_t gpio);
        const char* name();
        bool init();
        void cleanup();
        bool needsAttention();
        void attention();

        void togglePause();
        bool tapeLoaded() const;
        bool playing() const;
        void play();
        void insert(Stream *stream);
        void eject();
        
        const char *currentStatus();
        uint8_t startBlock() const;
        void setStartBlock(uint8_t startBlock);
        uint8_t numBlocks() const;

        void isrHandler();
    };

}

#endif //TZX_SERVICE_H