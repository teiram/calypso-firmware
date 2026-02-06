#ifndef TZX_DEFINITIONS_H
#define TZX_DEFINITIONS_H


#define TZX_INPUT_BUFFER_SIZE       18
#define TZX_OUTPUT_BUFFER_SIZE      64
#define TZX_SYMBOL_BUFFER_SIZE      128
#define TZX_MAX_BLOCK_OFFSETS       8

#define TZX_STANDARD_SPEED_BLOCK  0x10
#define TZX_TURBO_SPEED_BLOCK     0x11
#define TZX_PURE_TONE             0x12
#define TZX_PULSE_SEQUENCE        0x13
#define TZX_PUREDATA_BLOCK        0x14
#define TZX_GENERALIZED_DATA_BLOCK  0x19
#define TZX_PAUSE                 0x20
#define TZX_GROUP_START           0x21
#define TZX_GROUP_END             0x22
#define TZX_JUMP_TO_BLOCK         0x23
#define TZX_LOOP_START            0x24
#define TZX_LOOP_END              0x25
#define TZX_CALL_SEQUENCE         0x26
#define TZX_RETURN_FROM_SEQUENCE  0x27
#define TZX_STOP_48K_MODE         0x2A
#define TZX_SET_SIGNAL_LEVEL      0x2B
#define TZX_TEXT_DESCRIPTION      0x30
#define TZX_MESSAGE_BLOCK         0x31
#define TZX_ARCHIVE_INFO          0x32
#define TZX_HARDWARE_TYPE         0x33
#define TZX_CUSTOM_INFO           0x35
#define TZX_GLUE_BLOCK            0x5A

#define STD_LEADER_LENGTH   619                 //2168 t-states
#define STD_SYNC1_LENGTH    191                 //667  t-states
#define STD_SYNC2_LENGTH    210                 //735  t-states
#define STD_ZERO_LENGTH     244                 //855  t-states
#define STD_ONE_LENGTH      489                 //1710 t-states
#define STD_HEADER_PULSES   8063                //8063 semicycles
#define STD_DATA_PULSES     3223                //3223 semicycles
#define STD_FINAL_PAUSE     1000                //1000 msecs

typedef struct {
  uint16_t end_pause;
  uint16_t data_length;
} __attribute__((packed))  tzx_standard_block_t;

typedef struct {
  uint16_t pulse_length;
  uint16_t pulse_count;
} __attribute__((packed))  tzx_puretone_block_t;

typedef struct {
  uint16_t pilot_length;
  uint16_t sync_1st_pulse_length;
  uint16_t sync_2nd_pulse_length;
  uint16_t zero_length;
  uint16_t one_length;
  uint16_t pilot_pulses;
  uint8_t bits_last_byte;
  uint16_t end_pause;
  uint8_t data_length[3];
} __attribute__((packed)) tzx_turbospeed_block_t;

typedef struct {
  uint16_t zero_length;
  uint16_t one_length;
  uint8_t bits_last_byte;
  uint16_t end_pause;
  uint8_t data_length[3];
} __attribute__((packed))  tzx_puredata_block_t;

typedef struct {
  uint32_t block_length;
  uint16_t end_pause;
  uint32_t symbols_in_pilot_sync;
  uint8_t max_pulses_per_pilot_sync;
  uint8_t pilot_sync_alphabet_symbols;
  uint32_t symbols_in_data_stream;
  uint8_t max_pulses_per_data_symbol;
  uint8_t data_alphabet_symbols;
} __attribute__((packed)) tzx_generalized_data_block_t;

#endif /* TZX_DEFINITIONS_H */
