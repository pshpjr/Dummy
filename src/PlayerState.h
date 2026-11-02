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
        virtual PlayerState* RecvPacket(Player* player, RecvBuffer& buffer);
        virtual void Enter(PlayerRefector* player) {}
        virtual void Exit(PlayerRefector* player) {}
        virtual bool ValidDisconnect() { return false; }
        void Disconnect(Player* player, Player::DisconnectReason reason);
        StateType GetType() const
        {return _stateType;}
    protected:
        static bool NeedAct(int permil){return psh::RandomUtil::Rand(1,1000) <= permil;}
        //  평균 seconds초에 한 번 일어나도록 틱마다 loopMs ÷ 간격 확률로 굴린다. 0 이하면 하지 않는다.
        bool NeedActEvery(double seconds) const
        {
            return seconds > 0 && psh::RandomUtil::Chance(_permil.loopMs / 1000.0 / seconds);
        }
        DummyPercent _permil = gPermil;
        DummyInterval _interval = gInterval;

        using PacketHandler = std::function<PlayerState*(Player*, RecvBuffer&)>;
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

    protected:
        //  이번 틱에 옮겨 갈 레벨. 옮기지 않으면 End.
        //  필드에서는 toVillage로 마을, fieldChange로 다른 필드에 간다.
        [[nodiscard]] virtual ServerType NextLevel(const Player* player) const;

        static ServerType RandomField(ServerType exclude);
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

    protected:
        //  마을에서는 toField로 필드에 간다.
        [[nodiscard]] ServerType NextLevel(const Player* player) const override;

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
