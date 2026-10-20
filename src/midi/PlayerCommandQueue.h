#ifndef PLAYER_COMMAND_QUEUE_H
#define PLAYER_COMMAND_QUEUE_H

#include <atomic>
#include <cstddef>
#include <cstdint>

// Lock-free SPSC ring buffer for Player commands.
// Single producer (core 0) - single consumer (core 1).
struct PlayerCommand {
    enum Type : uint8_t {
        CMD_NONE = 0,
        CMD_PLAY_NOTE,
        CMD_STOP_NOTE,
        CMD_ALL_NOTES_OFF,
        CMD_CHANGE_PROGRAM,
        CMD_SET_VOLUME,
        CMD_SET_PAN,
        CMD_SET_PITCH_BENDING,
    };
    
    Type type = CMD_NONE;
    uint8_t channel = 0;
    uint8_t a = 0;      // pitch/velocity/program/volume/pan
    uint16_t b = 0;     // pitch bend value
};

class PlayerCommandQueue {
public:
    static constexpr size_t CAPACITY = 256;
    
    PlayerCommandQueue() {
        m_head.store(0, std::memory_order_relaxed);
        m_tail.store(0, std::memory_order_relaxed);
        m_overflowCount = 0;
    }
    
    // Called from producer (core 0) only
    bool push(const PlayerCommand &cmd) {
        size_t head = m_head.load(std::memory_order_relaxed);
        size_t next_head = (head + 1) % CAPACITY;
        if (next_head == m_tail.load(std::memory_order_acquire)) {
            // Queue full - drop command
            m_overflowCount++;
            return false;
        }
        m_buffer[head] = cmd;
        m_head.store(next_head, std::memory_order_release);
        return true;
    }
    
    // Called from consumer (core 1) only
    bool pop(PlayerCommand &cmd) {
        size_t tail = m_tail.load(std::memory_order_relaxed);
        if (tail == m_head.load(std::memory_order_acquire)) {
            return false; // empty
        }
        cmd = m_buffer[tail];
        m_tail.store((tail + 1) % CAPACITY, std::memory_order_release);
        return true;
    }
    
    uint32_t overflowCount() const {
        return m_overflowCount;
    }
    
    bool isEmpty() const {
        size_t tail = m_tail.load(std::memory_order_relaxed);
        size_t head = m_head.load(std::memory_order_relaxed);
        return tail == head;
    }
    
private:
    PlayerCommand m_buffer[CAPACITY];
    std::atomic<size_t> m_head;
    std::atomic<size_t> m_tail;
    volatile uint32_t m_overflowCount;
};

#endif /* PLAYER_COMMAND_QUEUE_H */
