#ifndef REMOTE_PLAYER_H
#define REMOTE_PLAYER_H

#include "Player.h"
#include "PlayerCommandQueue.h"

class RemotePlayer : public Player {
public:
    RemotePlayer(Player &realPlayer, PlayerCommandQueue &queue);
    
    virtual bool init() override;
    virtual void playNote(uint8_t channel, uint8_t pitch, uint8_t velocity) override;
    virtual void stopNote(uint8_t channel, uint8_t pitch) override;
    virtual void allNotesOff() override;
    virtual void changeProgram(uint8_t channel, uint8_t program) override;
    virtual void setVolume(uint8_t channel, uint8_t volume) override;
    virtual void setPan(uint8_t channel, uint8_t pan) override;
    virtual void setPitchBending(uint8_t channel, uint16_t bending) override;
    virtual void synthTask() override;
    virtual void showStats() override;
    virtual uint16_t audioBufferCount() const override;
    virtual uint16_t audioBufferSamples() const override;
    
private:
    Player &m_realPlayer;
    PlayerCommandQueue &m_queue;
};

void applyCommand(Player &player, const PlayerCommand &cmd);

#endif /* REMOTE_PLAYER_H */
