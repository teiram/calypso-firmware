#include "RemotePlayer.h"

RemotePlayer::RemotePlayer(Player &realPlayer, PlayerCommandQueue &queue)
    : m_realPlayer(realPlayer), m_queue(queue) {
}

bool RemotePlayer::init() {
    // Forward init to real player - called from core 0 before audio is enabled
    return m_realPlayer.init();
}

void RemotePlayer::playNote(uint8_t channel, uint8_t pitch, uint8_t velocity) {
    PlayerCommand cmd;
    cmd.type = PlayerCommand::CMD_PLAY_NOTE;
    cmd.channel = channel;
    cmd.a = pitch & 0x7f;
    cmd.b = velocity & 0x7f;
    m_queue.push(cmd);
}

void RemotePlayer::stopNote(uint8_t channel, uint8_t pitch) {
    PlayerCommand cmd;
    cmd.type = PlayerCommand::CMD_STOP_NOTE;
    cmd.channel = channel;
    cmd.a = pitch & 0x7f;
    cmd.b = 0;
    m_queue.push(cmd);
}

void RemotePlayer::allNotesOff() {
    PlayerCommand cmd;
    cmd.type = PlayerCommand::CMD_ALL_NOTES_OFF;
    cmd.channel = 0;
    cmd.a = 0;
    cmd.b = 0;
    m_queue.push(cmd);
}

void RemotePlayer::changeProgram(uint8_t channel, uint8_t program) {
    PlayerCommand cmd;
    cmd.type = PlayerCommand::CMD_CHANGE_PROGRAM;
    cmd.channel = channel;
    cmd.a = program & 0x7f;
    cmd.b = 0;
    m_queue.push(cmd);
}

void RemotePlayer::setVolume(uint8_t channel, uint8_t volume) {
    PlayerCommand cmd;
    cmd.type = PlayerCommand::CMD_SET_VOLUME;
    cmd.channel = channel;
    cmd.a = volume & 0x7f;
    cmd.b = 0;
    m_queue.push(cmd);
}

void RemotePlayer::setPan(uint8_t channel, uint8_t pan) {
    PlayerCommand cmd;
    cmd.type = PlayerCommand::CMD_SET_PAN;
    cmd.channel = channel;
    cmd.a = pan & 0x7f;
    cmd.b = 0;
    m_queue.push(cmd);
}

void RemotePlayer::setPitchBending(uint8_t channel, uint16_t bending) {
    PlayerCommand cmd;
    cmd.type = PlayerCommand::CMD_SET_PITCH_BENDING;
    cmd.channel = channel;
    cmd.a = 0;
    cmd.b = bending;
    m_queue.push(cmd);
}

void RemotePlayer::synthTask() {
    // No-op - core 1 drives synthesis
}

void RemotePlayer::showStats() {
    // No-op - stats read from core 1 context
}

uint16_t RemotePlayer::audioBufferCount() const {
    return m_realPlayer.audioBufferCount();
}

uint16_t RemotePlayer::audioBufferSamples() const {
    return m_realPlayer.audioBufferSamples();
}

void applyCommand(Player &player, const PlayerCommand &cmd) {
    switch (cmd.type) {
        case PlayerCommand::CMD_PLAY_NOTE:
            player.playNote(cmd.channel, cmd.a, cmd.b & 0x7f);
            break;
        case PlayerCommand::CMD_STOP_NOTE:
            player.stopNote(cmd.channel, cmd.a);
            break;
        case PlayerCommand::CMD_ALL_NOTES_OFF:
            player.allNotesOff();
            break;
        case PlayerCommand::CMD_CHANGE_PROGRAM:
            player.changeProgram(cmd.channel, cmd.a);
            break;
        case PlayerCommand::CMD_SET_VOLUME:
            player.setVolume(cmd.channel, cmd.a);
            break;
        case PlayerCommand::CMD_SET_PAN:
            player.setPan(cmd.channel, cmd.a);
            break;
        case PlayerCommand::CMD_SET_PITCH_BENDING:
            player.setPitchBending(cmd.channel, cmd.b);
            break;
        case PlayerCommand::CMD_NONE:
        default:
            break;
    }
}
