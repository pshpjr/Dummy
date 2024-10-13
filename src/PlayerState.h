//
// Created by pshpj on 24. 10. 11.
//

#ifndef PLAYERSTATE_REFECTOR_H
#define PLAYERSTATE_REFECTOR_H


#include "PacketGenerated.h"
#include <unordered_map>
#include <functional>

#include "DummyData.h"
#include "Player.h"
#include "Rand.h"

class PlayerRefector;

namespace psh
{
    enum StateType
    {
        other,
        levelChange
    };

    class PlayerState {
    public:
        virtual ~PlayerState() = default;
        virtual PlayerState* Update(Player* player, int time) { return this; }
        virtual PlayerState* RecvPacket(Player* player, CRecvBuffer& buffer);
        virtual void Enter(PlayerRefector* player) {}
        virtual void Exit(PlayerRefector* player) {}
        virtual bool ValidDisconnect() { return false; }
        void Disconnect(Player* player);
        StateType GetType() const
        {return _stateType;}
    protected:
        static bool NeedAct(int permil){return psh::RandomUtil::Rand(1,1000) <= permil;}
        DummyPercent _permil = gPermil;

        using PacketHandler = std::function<PlayerState*(Player*, CRecvBuffer&)>;
        std::unordered_map<ePacketType, PacketHandler> _packetHandlers{};
        StateType _stateType{other};
    };

    class DisconnectStateRefector : public PlayerState {
    public:
        static PlayerState* Get();

        PlayerState* Update(Player* player, int time) override;

    private:
        DisconnectStateRefector() = default;

    };

    class LoginLoginStateRefector : public PlayerState {
    public:
        static PlayerState* Get();

        PlayerState* Update(Player* player, int time) override;

    private:
        LoginLoginStateRefector();
    };

    class GameLoginStateRefector : public PlayerState {
    public:
        static PlayerState* Get();

        PlayerState* Update(Player* player, int time) override;

    private:
        GameLoginStateRefector();
    };

    class GameStateRefector : public PlayerState {
    public:
        PlayerState* Update(Player* player, int time) override;

        GameStateRefector();
    };

    class LevelChangeStateRefector : public GameStateRefector {
    public:
        static PlayerState* Get();

        PlayerState* Update(Player* player, int time) override;

    private:
        LevelChangeStateRefector();

    };

    class VillageStateRefector : public GameStateRefector {
    public:
        static PlayerState* Get();

        PlayerState* Update(Player* player, int time) override;

    private:
        VillageStateRefector();
    };

    class PveStateRefector : public GameStateRefector {
    public:
        static PlayerState* Get();

        PlayerState* Update(Player* player, int time) override;

    private:
        PveStateRefector();
    };

    class PvpStateRefector : public GameStateRefector {
    public:
        static PlayerState* Get();

        PlayerState* Update(Player* player, int time) override;

    private:
        PvpStateRefector();
    };

    class DisconnectWaitStateRefector : public GameStateRefector {
    public:
        static PlayerState* Get();

        PlayerState* Update(Player* player, int time) override;
        bool ValidDisconnect() override { return true; }
    private:
        DisconnectWaitStateRefector();
    };
}

#endif //PLAYERSTATE_REFECTOR_H
