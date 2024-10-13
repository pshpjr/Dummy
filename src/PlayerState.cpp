//
// Created by pshpj on 24. 10. 11.
//

#include "PlayerState.h"
#include "Player.h"
#include <CLogger.h>


psh::PlayerState* psh::PlayerState::RecvPacket(Player* player, CRecvBuffer& buffer)
{
    ePacketType type;
    buffer >> type;

    auto it = _packetHandlers.find(type);
    if (it != _packetHandlers.end()) {
        return it->second(player, buffer);
    } else {
        ASSERT_CRASH(false,"Invalid Type");
        return this;
    }
}

void psh::PlayerState::Disconnect(Player* player)
{
    player->Disconnect();
}

psh::PlayerState* psh::DisconnectStateRefector::Get() {
    static DisconnectStateRefector instance;
    return &instance;
}

psh::PlayerState* psh::DisconnectStateRefector::Update(Player* player, int time)
{
    player->LoginLogin();
    return LoginLoginStateRefector::Get();
}

psh::PlayerState* psh::LoginLoginStateRefector::Get()
{
    static LoginLoginStateRefector instance;
    return &instance;
}

psh::PlayerState* psh::LoginLoginStateRefector::Update(Player* player, int time)
{
    return PlayerState::Update(player, time);
}

psh::LoginLoginStateRefector::LoginLoginStateRefector() {
    // 패킷 핸들러 등록
    _packetHandlers[eLogin_ResLogin] =
        [this](Player* player, CRecvBuffer& buffer) -> PlayerState* {
            // 패킷 처리 로직
            eLoginResult result;
            ID id;
            AccountNo accountNo;
            GetLogin_ResLogin(buffer, accountNo, id, result, player->_key);

            ASSERT_CRASH(id == player->_id, "Login Result ID Wrong");
            ASSERT_CRASH(result == psh::eLoginResult::LoginSuccess, "Login Failed");
            ASSERT_CRASH(accountNo == player->_accountNo, "Invalid AccountNo");

            player->CheckPacket(eLogin_ResLogin);
            player->GameLogin();

            return GameLoginStateRefector::Get();
        };

    // 다른 패킷 타입에 대한 핸들러도 필요시 등록
}

psh::PlayerState* psh::GameLoginStateRefector::Get()
{
    static GameLoginStateRefector instance;
    return &instance;
}

psh::PlayerState* psh::GameLoginStateRefector::Update(Player* player, int time)
{
    return PlayerState::Update(player, time);
}

psh::GameLoginStateRefector::GameLoginStateRefector()
{
    _packetHandlers[eGame_ResLogin] =
        [this](Player* player, CRecvBuffer& buffer) -> PlayerState* {
            AccountNo accountNo;
            bool result;
            GetGame_ResLogin(buffer,accountNo, result);
            player->CheckPacket(eGame_ResLogin);
            ASSERT_CRASH(accountNo == player->_accountNo, "GameLogin Account Wrong");
            ASSERT_CRASH(result , "LoginFail");
            return this;
    };

    _packetHandlers[eGame_ResLevelEnter] = [this](Player* player, CRecvBuffer& buffer) -> PlayerState* {
        AccountNo accountNo;
        ObjectID myId;
        ServerType server;
        GetGame_ResLevelEnter(buffer,accountNo, myId,server);

        player->LevelChangeComp();
        player->_containGroup = server;
        player->_me = myId;
        player->needUpdate = false;
        switch (server)
        {
        case ServerType::Village:
            return VillageStateRefector::Get();
        case ServerType::Easy:
            [[fallthrough]];
        case ServerType::Hard:
            return PveStateRefector::Get();
            break;
        case ServerType::Pvp:
            return PvpStateRefector::Get();
            break;
        default:
            ASSERT_CRASH(false,"InvalidServer");
            return this;
        };
    };
}


psh::PlayerState* psh::GameStateRefector::Update(Player* player, int time)
{
    if (!player->needUpdate)
        return this;
    if (player->_hp <= 0)
    {
        return this;
    }

    //이동중이면 도착 전 까진 이동만 함.
    player->UpdateLocation(time);
    //플레이어 멈춘 상태. 타겟이 있다면 공격.
    if (player->_target != -1)
    {
        if (player->_attackCooldown > 0)
        {
            player->_attackCooldown -= time;
            return this;
        }
        auto minSkill = player->_templateID*3;
        if(minSkill<0)
        {
            __debugbreak();
        }
        player->Attack(RandomUtil::Rand(minSkill,minSkill+2));
        player->_attackCooldown += 3000;
    }
    //멈췄는데 타겟이 없다면 이동 시도
    else if(NeedAct(_permil.move))
    {
        int dx = RandomUtil::Rand(-1* _permil.moveOffset, _permil.moveOffset);
        int dy = RandomUtil::Rand(-1* _permil.moveOffset, _permil.moveOffset);
        auto dest = player->_location + FVector(dx, dy);
        dest = Clamp(dest, 0, _permil.moveRange);

        if ((dest - player->_spawnLocation).Size() > _permil.moveOffset *2)
        {
            player->Move(player->_spawnLocation, Player::goSpawn);

            player->_target = -1;
        }
        else
        {
            player->Move(dest, Player::randomMove);
        }
    }
    else if (NeedAct(_permil.disconnect))
    {
        player->Disconnect();
        return DisconnectWaitStateRefector::Get();
    }
    if (NeedAct(_permil.chat))
    {
        player->Chat();
    }

    return this;
}

psh::GameStateRefector::GameStateRefector()
{
    _packetHandlers[eGame_ResLevelEnter] = [this](Player* player, CRecvBuffer& buffer) -> PlayerState*
    {
        ASSERT_CRASH(false,"Invaid Level Change");
        return this;
    };

    _packetHandlers[eGame_ResCreateActor] = [this](Player* player, CRecvBuffer& buffer) -> PlayerState* {
        ObjectID objectId;
        bool isSpawn;
        eObjectType objType;
        TemplateID templateId;
        FVector loc;
        FVector dir;

        GetGame_ResCreateActor(buffer, objectId, objType,templateId, loc, dir, isSpawn);

        if (objectId == player->_me)
        {
            player->_spawnLocation = loc;
            player->_location = loc;
            player->_destination = loc;
            player->needUpdate = true;
            player->_templateID = templateId;
        }

        else if (player->_target == -1
            && objType == eObjectType::Monster
            && NeedAct(_permil.target))
        {
            player->SetTarget(objectId,loc);
        }
        return this;
    };

    _packetHandlers[eGame_ResChracterDetail] = [this](Player* player, CRecvBuffer& buffer) -> PlayerState* {
        ObjectID objectId;
        int hp;
        GetGame_ResChracterDetail(buffer,objectId,hp);
        if (objectId == player->_me)
        {
            player->_hp = hp;
        }
        return this;
    };

    _packetHandlers[eGame_ResPlayerDetail] = [this](Player* player, CRecvBuffer& buffer) -> PlayerState* {
        ObjectID objectId;
        Nickname nick;
        GetGame_ResPlayerDetail(buffer, objectId, nick);
        return this;
    };

    _packetHandlers[eGame_ResDestroyActor] = [this](Player* player, CRecvBuffer& buffer) -> PlayerState* {
        ObjectID objectId;
        bool isDead;
        char cause;
        GetGame_ResDestroyActor(buffer, objectId, isDead,cause);
        if (player->_me == objectId)
        {
            if(player->_hp > 0)
            {
                player->_logger.Write(L"DummyDisconnect",CLogger::LogLevel::Error,L"Invalid Destroy HP > 0, AccountNo : %d, hp : %d",player->_accountNo,player->_hp);
            }
            player->Disconnect();

            return DisconnectWaitStateRefector::Get();
        }
        if (player->_target == objectId)
        {
            player->_target = -1;
        }
        return this;
    };

    _packetHandlers[eGame_ResGetCoin] = [this](Player* player, CRecvBuffer& buffer) -> PlayerState* {
        ObjectID objectId;
        uint8 value;
        GetGame_ResGetCoin(buffer, objectId, value);
        ASSERT_CRASH(player->_me == objectId, L"Invalid Recv");
        player->_coin += value;
        return this;
    };

    _packetHandlers[eGame_ResMoveStop] = [this](Player* player, CRecvBuffer& buffer) -> PlayerState* {
        FVector loc;
        ObjectID id;
        GetGame_ResMoveStop(buffer, id, loc);

        if (id == player->_me)
        {
            player->Stop(loc);
        }
        else if(id == player->_target)
        {
           player->SetTarget(id,loc);
        }
        return this;
    };

    _packetHandlers[eGame_ResMove] = [this](Player* player, CRecvBuffer& buffer) -> PlayerState* {
        FVector loc;
        ObjectID id;
        eObjectType objType;
        GetGame_ResMove(buffer, id,objType, loc);
        if(id == player->_me)
        {
            player->CheckPacket(eGame_ResMove);
        }
        else if (id == player->_target)
        {
            player->SetTarget(id,loc);
        }
        return this;
    };

    _packetHandlers[eGame_ResAttack] = [this](Player* player, CRecvBuffer& buffer) -> PlayerState* {
        ObjectID attacker;
        char attackType;

        FVector dir;
        GetGame_ResAttack(buffer, attacker, attackType, dir);
        if(attacker == player->_me)
        {
            player->CheckPacket(eGame_ResAttack);
        }
        return this;
    };

    _packetHandlers[eGame_ResHit] = [this](Player* player, CRecvBuffer& buffer) -> PlayerState* {
        ObjectID victim;
        ObjectID attacker;
        int hp;
        GetGame_ResHit(buffer, victim, attacker, hp);
        if (victim == player->_me)
        {
            player->_hp = hp;

            if (player->_hp<= 0)
            {
                player->Disconnect();

                return DisconnectWaitStateRefector::Get();
            }

        }
        return this;
    };

    _packetHandlers[eGame_ResChat] = [this](Player* player, CRecvBuffer& buffer) -> PlayerState* {
        ObjectID target;
        String chat;
        GetGame_ResChat(buffer, target,chat);
        return this;
    };

}

psh::PlayerState* psh::LevelChangeStateRefector::Get()
{
    static LevelChangeStateRefector instance;
    return &instance;
}

psh::PlayerState* psh::LevelChangeStateRefector::Update(Player* player, int time)
{
    return this;
}

psh::LevelChangeStateRefector::LevelChangeStateRefector()
    : GameStateRefector{}
{
    _packetHandlers[eGame_ResLevelEnter] = [this](Player* player, CRecvBuffer& buffer) -> PlayerState* {
        AccountNo accountNo;
        ServerType server;
        ObjectID myID;
        GetGame_ResLevelEnter(buffer, accountNo, myID, server);
        player->CheckPacket(eGame_ResLevelEnter);
        ASSERT_CRASH(accountNo == player->_accountNo, "InvalidAccountNO");
        ASSERT_CRASH(server == player->_containGroup, L"Invaid Group move");
        player->_me = myID;
        player->_target = -1;
        player->_isMove = false;
        player->_moveTime = 0;
        player->_toDestinationTime = 0;
        player->LevelChangeComp();
        player->_containGroup = server;
        player->_destination = {0, 0};
        player->_location = {0, 0};
        switch (server)
        {
        case ServerType::Village:
            return VillageStateRefector::Get();
        case ServerType::Easy:
            [[fallthrough]];
        case ServerType::Hard:
            return PveStateRefector::Get();
            break;
        case ServerType::Pvp:
            return PvpStateRefector::Get();
            break;
        default:
            ASSERT_CRASH(false, L"Invalid Group");
            return nullptr;
        }

        _stateType = levelChange;
    };

    _packetHandlers[eGame_ResCreateActor] = [this](Player* player, CRecvBuffer& buffer) -> PlayerState* {
        ObjectID objectId;
        bool isSpawn;
        eObjectType group;
        char charType;
        FVector loc;
        FVector dir;

        GetGame_ResCreateActor(buffer, objectId, group, charType, loc, dir, isSpawn);
        return this;
    };

    _packetHandlers[eGame_ResDestroyActor] = [this](Player* player, CRecvBuffer& buffer) -> PlayerState* {
        ObjectID objectId;
        bool isDead;
        char cause;
        GetGame_ResDestroyActor(buffer, objectId, isDead,cause);
        return this;
    };

    _packetHandlers[eGame_ResMoveStop] = [this](Player* player, CRecvBuffer& buffer) -> PlayerState* {
        ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
        FVector loc;
        ObjectID id;
        GetGame_ResMoveStop(buffer, id, loc);
        return this;
    };


    _packetHandlers[eGame_ResMoveStop] = [this](Player* player, CRecvBuffer& buffer) -> PlayerState* {
        FVector loc;
        ObjectID id;
        GetGame_ResMoveStop(buffer, id, loc);
        return this;
    };

    _packetHandlers[eGame_ResMove] = [this](Player* player, CRecvBuffer& buffer) -> PlayerState* {
        ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
        FVector loc;
        ObjectID id;
        eObjectType objType;
        GetGame_ResMove(buffer, id,objType, loc);
        if(id == player->_me)
        {
            player->CheckPacket(eGame_ResMove);
        }
        return this;
    };

    _packetHandlers[eGame_ResAttack] = [this](Player* player, CRecvBuffer& buffer) -> PlayerState* {
        ObjectID attacker;
        char attackType;

        FVector dir;
        GetGame_ResAttack(buffer, attacker, attackType, dir);
        if(attacker == player->_me)
        {
            player->CheckPacket(eGame_ResAttack);
        }
        return this;
    };


}

psh::PlayerState* psh::VillageStateRefector::Get()
{
    static VillageStateRefector instance;
    return &instance;
}

psh::PlayerState* psh::VillageStateRefector::Update(Player* player, int time)
{
    if(NeedAct(_permil.toField))
    {
        auto max = static_cast<int>(ServerType::End)-1;
        auto target = static_cast<ServerType>(RandomUtil::Rand(1, max));
        player->ReqLevelChange(target);
        player->_target = -1;
        return LevelChangeStateRefector::Get();
    }
    return GameStateRefector::Update(player,time);
}

psh::VillageStateRefector::VillageStateRefector()
    : GameStateRefector{}
{
    _packetHandlers[eGame_ResAttack] = [this](Player* player, CRecvBuffer& buffer) -> PlayerState* {
        ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
        ASSERT_CRASH(false, L"Village Cannot Attack");
        return this;
    };

    _packetHandlers[eGame_ResHit] = [this](Player* player, CRecvBuffer& buffer) -> PlayerState* {
        ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
        ASSERT_CRASH(false, L"Village Cannot hit");
        return this;
    };
}

psh::PlayerState* psh::PveStateRefector::Get()
{
    static PveStateRefector instance;
    return &instance;
}

psh::PlayerState* psh::PveStateRefector::Update(Player* player, int time)
{
    if(NeedAct(_permil.toVillage))
    {
        auto target = static_cast<ServerType>(0);
        player->ReqLevelChange(target);
        player->_target = -1;
        return LevelChangeStateRefector::Get();
    }

    return GameStateRefector::Update(player, time);
}

psh::PveStateRefector::PveStateRefector()
    : GameStateRefector{}
{
    _packetHandlers[eGame_ResMove] = [this](Player* player, CRecvBuffer& buffer) -> PlayerState* {
        FVector loc;
        ObjectID id;
        eObjectType objType;
        GetGame_ResMove(buffer, id, objType, loc);

        if (id == player->_me)
        {
            player->CheckPacket(eGame_ResMove);
        }
        else if (player->_target == -1
            && objType == eObjectType::Monster
            && (loc - player->_spawnLocation).Size() <= 300)
        {
            player->SetTarget(id,loc);
        }
        else if (id == player->_target)
        {
            player->SetTarget(id,loc);
        }
        return this;
    };
}

psh::PlayerState* psh::PvpStateRefector::Get()
{
    static PvpStateRefector instance;
    return &instance;
}

psh::PlayerState* psh::PvpStateRefector::Update(Player* player, int time)
{
    if (NeedAct(_permil.toVillage))
    {
        auto target = static_cast<ServerType>(0);
        player->ReqLevelChange(target);
        player->_target = -1;
        return LevelChangeStateRefector::Get();
    }

    return GameStateRefector::Update(player, time);
}

psh::PvpStateRefector::PvpStateRefector()
    : GameStateRefector{}
{
    _packetHandlers[eGame_ResMove] = [this](Player* player, CRecvBuffer& buffer) -> PlayerState* {
        FVector loc;
        ObjectID id;
        eObjectType objType;
        GetGame_ResMove(buffer, id, objType, loc);

        if (id == player->_me)
        {
            player->CheckPacket(eGame_ResMove);
        }
        else if (player->_target == -1
            && (loc - player->_spawnLocation).Size() <= 300)
        {
            player->SetTarget(id,loc);
        }
        else if (id == player->_target)
        {
            player->SetTarget(id,loc);
        }
        return this;
    };
}

psh::PlayerState* psh::DisconnectWaitStateRefector::Get()
{
    static DisconnectWaitStateRefector instance;
    return &instance;
}

psh::PlayerState* psh::DisconnectWaitStateRefector::Update(Player* player, int time)
{
    return this;
}

psh::DisconnectWaitStateRefector::DisconnectWaitStateRefector()
    : GameStateRefector{}
{
    _packetHandlers[eGame_ResLevelEnter] = [this](Player* player, CRecvBuffer& buffer) -> PlayerState* {
        AccountNo accountNo;
        ObjectID myId;
        ServerType server;
        GetGame_ResLevelEnter(buffer, accountNo, myId, server);
        return this;
    };
}
