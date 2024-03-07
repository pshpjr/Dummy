#include "PlayerState.h"

#include <PacketGenerated.h>

#include "Player.h"

PlayerState* PlayerState::Disconnect(Player* player)
{
    player->Disconnect();
    return DisconnectWaitState::Get();
}
PlayerState* DisconnectState::Update(Player* player, int time)
{
    player->LoginLogin();
    return LoginLoginState::Get();
}

PlayerState* DisconnectState::RecvPacket(Player* player,CRecvBuffer& buffer)
{
    ASSERT_CRASH(false,"Recv Packet But not Login");
    return this;
}

PlayerState* LoginLoginState::Update(Player* player, int time)
{
    return this;
}

PlayerState* LoginLoginState::RecvPacket(Player* player,CRecvBuffer& buffer)
{
    psh::ePacketType type;
    buffer >> type;

    switch(type)
    {
    case psh::eLogin_ResLogin:
        {
            psh::eLoginResult result;
            psh::ID id;
            psh::AccountNo accountNo;
            psh::GetLogin_ResLogin(buffer,accountNo,id,result,player->_key);

            player->CheckPacket(type);
            ASSERT_CRASH(id == player->_id, "Login Result ID Wrong");
            ASSERT_CRASH(result == psh::eLoginResult::LoginSuccess, "Login Result ID Wrong");

            player->GameLogin();

            return GameLoginState::Get();
        }
        break;
    default:
        ASSERT_CRASH(false,"Invalid Packet Type");
    break;
    }
    return this;
}

PlayerState* GameLoginState::Update(Player* player, int time)
{
    return this;
}

PlayerState* GameLoginState::RecvPacket(Player* player, CRecvBuffer& buffer)
{
    psh::ePacketType type;
    buffer >> type;

    switch(type)
    {
    case psh::eGame_ResLogin:
        {
            psh::AccountNo accountNo;
            bool result;
            psh::GetGame_ResLogin(buffer,accountNo, result);
            player->CheckPacket(type);
            ASSERT_CRASH(accountNo == player->_accountNo, "GameLogin Account Wrong");
            ASSERT_CRASH(result , "LoginFail");
        }
        break;
    case psh::eGame_ResLevelEnter:
        {
            psh::AccountNo accountNo;
            psh::ObjectID myId;
            psh::ServerType server;
            psh::GetGame_ResLevelEnter(buffer,accountNo, myId,server);
            
            player->LevelChangeComp();

            switch (server)
            {
            case psh::ServerType::Village:
                return VillageState::Get();

            case psh::ServerType::Easy:
                [[fallthrough]];
            case psh::ServerType::Hard:
                return PveState::Get();
                break;
            case psh::ServerType::Pvp:
                return PvpState::Get();
                break;
            default:
                ASSERT_CRASH(false);
            }
            
        }
        break;
    default:
        ASSERT_CRASH(false,"Invalid Packet Type");
        break;
    }
    return this;
}

PlayerState* GameState::Update(Player* player, int time)
{
    //이동중이면 도착 전 까진 이동만 함. 
    if(player->isMove)
    {
        player->toDestinationTime -=time;

        if(player->toDestinationTime<=0)
        {
            player->Stop();
        }
    }
    //플레이어 멈춘 상태. 타겟이 있다면 공격.
    // else if (player->_target != -1)
    // {
    //     player->Attack(psh::RandomUtil::Rand(0,2));
    // }
    //멈췄는데 타겟이 없다면 이동 시도
    else if(needAct(permil.move))
    {
        player->Move({float(psh::RandomUtil::Rand(permil.moveOffset,permil.moveRange))
            ,float(psh::RandomUtil::Rand(permil.moveOffset,permil.moveRange))});
    }
    return this;
}

PlayerState* GameState::RecvPacket(Player* player, CRecvBuffer& buffer)
{
    while (buffer.CanPopSize() != 0)
    {
        psh::ePacketType type;
        buffer >> type;

        auto next = HandlePacket(type,player,buffer);

        if (next != this)
            return next;
    }
    return this;
}

PlayerState* GameState::HandlePacket(psh::ePacketType type, Player* player, CRecvBuffer& buffer)
{
    switch (type)
    {
    case psh::eGame_ResLevelEnter:
        {
            //여기선 레벨 전환 없음. 레벨은 항상 levelChangeState에서. 
            ASSERT_CRASH(false,"Invalid Level change");
        }
    case psh::eGame_ResCreateActor:
        {
            psh::ObjectID objectId;
            bool isSpawn;
            bool isMove;
            psh::eCharacterGroup group;
            char charType;
            psh::FVector loc;
            psh::FVector dir;
            psh::FVector dst;
            psh::GetGame_ResCreateActor(buffer,  objectId,group,charType, loc, dir, dst,isMove,isSpawn);
            if (player->_me == -1)
            {
                player->_me = objectId;
            }
            if (player->_target == -1)
            {
                player->_target = objectId;
            }
        }
        break;
    case psh::eGame_ResDestroyActor:
        {
            //나에 관한 정보 있어야 하고
            ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
            psh::ObjectID objectId;
            bool isDead;
            psh::GetGame_ResDestroyActor(buffer,  objectId, isDead);
            if(objectId == player->_target)
            {
                player->_target = -1;
            }
        }
        break;
    case psh::eGame_ResMoveStop:
        {
            ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
            psh::FVector loc;
            psh::ObjectID id;
            psh::GetGame_ResMoveStop(buffer, id, loc);
            if(id == player->_target)
            {
                player->Move(loc);
            }
        }
        break;
    case psh::eGame_ResMove:
        {
            ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
            psh::FVector loc;
            psh::ObjectID id;
            psh::GetGame_ResMove(buffer, id, loc);
            if(id == player->_me)
            {
                player->CheckPacket(type);
            }
        }
        break;
    case psh::eGame_ResAttack:
        {
            ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
            psh::ObjectID attacker;
            char attackType;
            psh::GetGame_ResAttack(buffer,attacker,attackType);

            if(attacker == player->_me)
            {
                player->CheckPacket(type);
            }
        }
        break;
    case psh::eGame_ResDraw:
    {
        buffer.Ignore();
    }
    break;
    case psh::eGame_ResHit:
        {
            ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
            psh::ObjectID victim;
            psh::ObjectID attacker;
            int hp;
            psh::GetGame_ResHit(buffer, victim, attacker, hp);
            if (victim == player->_me)
            {
                player->hp = hp;
            }
        }
        break;
    default:
        ASSERT_CRASH(false, "Invalid Packet Type");
        break;
    }
    return this;
}

PlayerState* LevelChangeState::HandlePacket(psh::ePacketType type, Player* player, CRecvBuffer& buffer)
{
    if(type == psh::ePacketType::eGame_ResLevelEnter)
    {
        psh::AccountNo accountNo;
        psh::ServerType server;
        psh::ObjectID myID;
        psh::GetGame_ResLevelEnter(buffer, accountNo, myID, server);
        player->CheckPacket(type);
        ASSERT_CRASH(accountNo == player->_accountNo, "InvalidAccountNO");
        ASSERT_CRASH(server == player->containGroup,L"Invaid Group move");
        player->LevelChangeComp();
        player->containGroup = server;
        switch (server)
        {
        case psh::ServerType::Village :
            return VillageState::Get();
        case psh::ServerType::Easy:
            [[fallthrough]];
        case psh::ServerType::Hard:
            return PveState::Get();
            break;
        case psh::ServerType::Pvp:
            return PvpState::Get();
            break;
        default:
            ASSERT_CRASH(false,L"Invalid Group");
            return nullptr;
        }
    }
    else
    {
        return GameState::HandlePacket(type, player, buffer);
    }
}

PlayerState* VillageState::Update(Player* player, int time)
{
    if(needAct(permil.field))
    {
        auto max = static_cast<int>(psh::ServerType::End)-1;
        auto target = static_cast<psh::ServerType>(psh::RandomUtil::Rand(1,1));
        player->ReqLevelChange(target);
        return LevelChangeState::Get();
    }

    return GameState::Update(player, time);
}

PlayerState* VillageState::HandlePacket(psh::ePacketType type, Player* player, CRecvBuffer& buffer)
{
    switch (type)
    {
    case psh::eGame_ResAttack:
        {
            ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
            ASSERT_CRASH(false, L"Village Cannot Attack");
        }
        break;
    case psh::eGame_ResHit:
        {
            ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
            ASSERT_CRASH(false, L"Village Cannot hit");
        }
        break;
    default:
        GameState::HandlePacket(type,player,buffer);
        break;
    }
    return this;
}

PlayerState* PveState::Update(Player* player, int time)
{
    if(needAct(permil.field))
    {
        auto target = static_cast<psh::ServerType>(0);
        player->ReqLevelChange(target);
        return LevelChangeState::Get();
    }

    return GameState::Update(player, time);
}

PlayerState* PveState::RecvPacket(Player* player, CRecvBuffer& buffer)
{
    while (buffer.CanPopSize() != 0)
    {
        psh::ePacketType type;
        buffer >> type;

        switch (type)
        {
        case psh::eGame_ResAttack:
        {
            ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
            psh::ObjectID objectId;
            char attackType;
            psh::GetGame_ResAttack(buffer, objectId, attackType);
            if(objectId == player->_me)
            {
                player->CheckPacket(type);
            }
        }
        break;
        case psh::eGame_ResHit:
        {
            ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
            psh::ObjectID objectId;
            psh::ObjectID attacker;
            int hp;
            psh::GetGame_ResHit(buffer, objectId,attacker, hp);

        }
        break;
        case psh::eGame_ResMove:
        {
            ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
            psh::FVector loc;
            psh::ObjectID id;
            psh::GetGame_ResMove(buffer, id, loc);
            if(id == player->_me)
            {
                player->CheckPacket(psh::eGame_ReqMove);
            }
        }
        break;
        case psh::eGame_ResCreateActor:
        {
            psh::ObjectID objectId;
            bool isSpawn;
            bool isMove;
            psh::eCharacterGroup group;
            char type;
            psh::FVector loc;
            psh::FVector dir;
            psh::FVector dst;
            psh::GetGame_ResCreateActor(buffer,  objectId,group,type, loc, dir, dst,isMove,isSpawn);
            if (player->_me == -1)
            {
                player->_me = objectId;
            }
            else if (player->_target == -1)
            {
                player->_target = objectId;
            }
        }
        break;
        case psh::eGame_ResDestroyActor:
        {
            ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
            psh::ObjectID objectId;
            bool isDead;
            psh::GetGame_ResDestroyActor(buffer,  objectId, isDead);
            if (player->_me == objectId)
            {
                player->Disconnect();
                return DisconnectWaitState::Get();
            }
            if(player->_target == objectId)
            {
                player->_target = -1;
            }
        }
        break;
        default:
            return GameState::HandlePacket(type, player, buffer);
        }
    }
    return this;
}

PlayerState* PvpState::RecvPacket(Player* player, CRecvBuffer& buffer)
{
    while (buffer.CanPopSize() != 0)
    {
        psh::ePacketType type;
        buffer >> type;

        switch (type)
        {
        case psh::eGame_ResAttack:
        {
            ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
            psh::ObjectID objectId;
            char type;
            psh::GetGame_ResAttack(buffer, objectId,type);
            if(objectId == player->_me)
            {
                player->CheckPacket(psh::eGame_ReqAttack);
            }
        }
        break;
        case psh::eGame_ResHit:
        {
            ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
            psh::ObjectID objectId;
            psh::ObjectID attacker;
            int hp;
            psh::GetGame_ResHit(buffer, objectId,attacker, hp);

        }
        break;
        case psh::eGame_ResMove:
        {
            ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
            psh::FVector loc;
            psh::ObjectID id;
            psh::GetGame_ResMove(buffer, id, loc);
            if(id == player->_me)
            {
                player->CheckPacket(psh::eGame_ReqMove);
            }
        }
        break;
        case psh::eGame_ResCreateActor:
        {
            psh::ObjectID objectId;
            bool isSpawn;
            bool isMove;
            psh::eCharacterGroup group;
            char type;
            psh::FVector loc;
            psh::FVector dir;
            psh::FVector dst;
            psh::GetGame_ResCreateActor(buffer,  objectId,group,type, loc, dir, dst,isMove,isSpawn);
            if (player->_me == -1)
            {
                player->_me = objectId;
            }
            else if (player->_target == -1)
            {
                player->_target = objectId;
            }
        }
        break;
        case psh::eGame_ResDestroyActor:
        {
            ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
            psh::ObjectID objectId;
            bool isDead;
            psh::GetGame_ResDestroyActor(buffer,  objectId, isDead);
            if (player->_me == objectId)
            {
                player->Disconnect();
                return DisconnectWaitState::Get();
            }
        }
        break;
        default:
            ASSERT_CRASH(false, "Invalid Packet Type");
            break;
        }
    }
}


PlayerState* DisconnectWaitState::Update(Player* player, int time)
{
    return this;
}

PlayerState* DisconnectWaitState::RecvPacket(Player* player, CRecvBuffer& buffer)
{
    return PlayerState::RecvPacket(player, buffer);
}
