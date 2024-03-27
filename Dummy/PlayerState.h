#pragma once
#include <memory>
#include <PacketGenerated.h>

#include "DummyData.h"
#include "Rand.h"

class CRecvBuffer;
class Player;

enum StateType
{
    other,
    levelChange
};

template <typename T>
class stateSingleton
{
public:
    static T* Get()
    {
        static std::unique_ptr<T> _instance = std::make_unique<T>();
        return _instance.get();
    }
};
    
class PlayerState
{
public:
    PlayerState() = default;
    virtual ~PlayerState() = default;
    virtual PlayerState* Update(Player* player, int time) { return this; }
    virtual PlayerState* RecvPacket(Player* player,CRecvBuffer& buffer) {return this;}
    PlayerState* Disconnect(Player* player);

    virtual bool ValidDisconnect() { return false; }
    StateType GetType() { return stateType; }
    static bool needAct(int permil){return psh::RandomUtil::Rand(1,1000) <= permil;}
    DummyPercent permil = gPermil;
    StateType stateType = other;
};

class DisconnectState : public PlayerState, public stateSingleton<DisconnectState>
{
public:
    PlayerState* Update(Player* player, int time) override;
    PlayerState* RecvPacket(Player* player,CRecvBuffer& buffer) override;
};

class LoginLoginState : public PlayerState, public stateSingleton<LoginLoginState>
{
public:
    PlayerState* Update(Player* player, int time) override;
    PlayerState* RecvPacket(Player* player,CRecvBuffer& buffer) override;
};
    
class GameLoginState final : public PlayerState, public stateSingleton<GameLoginState>
{
public:
    PlayerState* Update(Player* player, int time) override;
    PlayerState* RecvPacket(Player* player,CRecvBuffer& buffer) override;
};

class GameState : public PlayerState
{
public:
    PlayerState* Update(Player* player, int time) override;
    PlayerState* RecvPacket(Player* player, CRecvBuffer& buffer) final;
protected:
    virtual PlayerState* HandlePacket(psh::ePacketType type,Player* player,CRecvBuffer& buffer);
};

class LevelChangeState final : public GameState, public stateSingleton<LevelChangeState>
{
public:
    LevelChangeState()
    {
        stateType = levelChange;
    }
    PlayerState* Update(Player* player, int time) override {return this;}

protected:
    PlayerState* HandlePacket(psh::ePacketType type, Player* player, CRecvBuffer& buffer) override;
};

class VillageState : public GameState, public stateSingleton<VillageState>
{
public:
    PlayerState* Update(Player* player, int time) override;
protected:
    PlayerState* HandlePacket(psh::ePacketType type, Player* player, CRecvBuffer& buffer) override;
};

class PveState : public GameState, public stateSingleton<PveState>
{
    
public:
    PlayerState* Update(Player* player, int time) override;
    PlayerState* HandlePacket(psh::ePacketType type, Player* player, CRecvBuffer& buffer) override;
};

class PvpState : public GameState, public stateSingleton<PvpState>
{
public:
    PlayerState* HandlePacket(psh::ePacketType type, Player* player, CRecvBuffer& buffer) override;
};

class IdleState;
class HuntState;

class DisconnectWaitState : public GameState, public stateSingleton<DisconnectWaitState>
{
public:
    DisconnectWaitState() = default;
    ~DisconnectWaitState() override = default;
    bool ValidDisconnect() override { return true; }
};

