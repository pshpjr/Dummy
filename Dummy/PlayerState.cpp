#include "PlayerState.h"

#include <PacketGenerated.h>

#include "CLogger.h"
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

            ASSERT_CRASH(id == player->_id, "Login Result ID Wrong");
            ASSERT_CRASH(result == psh::eLoginResult::LoginSuccess, "Login Result ID Wrong");
            ASSERT_CRASH(accountNo == player->_accountNo, "InvalidAccountNO");

            player->CheckPacket(type);

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
            player->_containGroup = server;
            player->_me = myId;
            player->needUpdate = false;
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
                ASSERT_CRASH(false,"InvalidServer");
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
    if (!player->needUpdate)
        return this;
    if (player->_hp <= 0)
    {
        return this;
    }


    //이동중이면 도착 전 까진 이동만 함. 
    if(player->_isMove)
    {
        player->_moveTime +=time;
        
        if(player->_toDestinationTime<=player->_moveTime)
        {
            player->Stop();
        }
    }
    //플레이어 멈춘 상태. 타겟이 있다면 공격.
     else if (player->_target != -1)
     {

        if (player->_attackCooldown > 0) 
        {
            player->_attackCooldown -= time;
            return this;
        }

        player->Attack(psh::RandomUtil::Rand(0,2));
        player->_attackCooldown += 1000;
     }
    //멈췄는데 타겟이 없다면 이동 시도
    else if(needAct(permil.move))
    {
        int dx = psh::RandomUtil::Rand(-1* permil.moveOffset, permil.moveOffset);
        int dy = psh::RandomUtil::Rand(-1* permil.moveOffset, permil.moveOffset);
        auto dest = player->_location + psh::FVector(dx, dy);
        dest = Clamp(dest, 0, permil.moveRange);

        if ((dest - player->_spawnLocation).Size() > permil.moveOffset *2)
        {
            player->Move(player->_spawnLocation, Player::goSpawn);
 
            player->_target = -1;
        }
        else 
        {
            player->Move(dest, Player::randomMove);
        }
    }
    else if (needAct(permil.disconnect))
    {
        player->Disconnect();
        return DisconnectWaitState::Get();
    }
    if (needAct(permil.chat))
    {
        player->Chat();
    }

    return this;
}

PlayerState* GameState::RecvPacket(Player* player, CRecvBuffer& buffer)
{
    psh::ePacketType type;
    while (buffer.CanPopSize() != 0)
    {

        buffer >> type;

        auto next = HandlePacket(type,player,buffer);
       
        if (next != this)
            return next;
    }
    ASSERT_CRASH(buffer.CanPopSize() == 0, "can pop from recv buffer");
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
        break;
    case psh::eGame_ResCreateActor:
        {
            psh::ObjectID objectId;
            bool isSpawn;
            psh::eCharacterGroup group;
            char charType;
            psh::FVector loc;
            psh::FVector dir;

            psh::GetGame_ResCreateActor(buffer, objectId,group,charType, loc, dir, isSpawn);

            if (objectId == player->_me)
            {
                if (loc == psh::FVector(0, 0))
                    __debugbreak();

                player->_spawnLocation = loc;
                player->_location = loc;
                player->_dest = loc;
                player->needUpdate = true;
            }

            else if (player->_target == -1 
                && group == psh::eCharacterGroup::Monster
                && needAct(permil.target))
            {
                player->_target = objectId;
                player->Move(loc);
            }
        }
        break;
        case psh::eGame_ResChracterDetail:
            {
                psh::ObjectID objectId;
                int hp;
                psh::GetGame_ResChracterDetail(buffer,objectId,hp);
                if (objectId == player->_me)
                {
                    player->_hp = hp;
                }
            }
        break;
        case psh::eGame_ResPlayerDetail:
            {
            psh::ObjectID objectId;
            psh::Nickname nick;
            psh::GetGame_ResPlayerDetail(buffer, objectId, nick);
            }
        break;
    case psh::eGame_ResDestroyActor:
    {
        ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
        psh::ObjectID objectId;
        bool isDead;
        char cause;
        psh::GetGame_ResDestroyActor(buffer, objectId, isDead,cause);
        if (player->_me == objectId)
        {
            if(player->_hp > 0)
            {
                player->_logger.Write(L"DummyDisconnect",CLogger::LogLevel::Error,L"Disconnect HP > 0, AccountNo : %d, hp : %d",player->_accountNo,player->_hp);
            }
            player->Disconnect();

            return DisconnectWaitState::Get();
        }
        else if (player->_target == objectId)
        {
            player->_target = -1;
        }
    }
    break;
    case psh::eGame_ResGetCoin:
        {
        psh::ObjectID objectId;
        uint8 value;
        psh::GetGame_ResGetCoin(buffer, objectId, value);
        ASSERT_CRASH(player->_me == objectId, L"Invalid Recv");
        player->_coin += value;
        }
        break;
    case psh::eGame_ResMoveStop:
        {
            ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
            psh::FVector loc;
            psh::ObjectID id;
            psh::GetGame_ResMoveStop(buffer, id, loc);
            if (id == player->_me)
            {
                player->_location = loc;
                player->Stop();
            }
            else if(id == player->_target)
            {
                if(player->_isMove)
                   player->Move(loc);
            }
        }
        break;
    case psh::eGame_ResMove:
        {
            ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
            psh::FVector loc;
            psh::ObjectID id;
            psh::eCharacterGroup group;
            psh::GetGame_ResMove(buffer, id,group, loc);
            if(id == player->_me)
            {
                player->CheckPacket(type);
            }
            else if (id == player->_target)
            {
                player->Move(loc);
            }
        }
        break;
    case psh::eGame_ResAttack:
        {

            ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
            psh::ObjectID attacker;
            char attackType;

            psh::FVector dir;
            psh::GetGame_ResAttack(buffer, attacker, attackType, dir);
            if(attacker == player->_me)
            {
                player->CheckPacket(type);
            }
        }
        break;
    case psh::eGame_ResDraw:
    {
        psh::FVector loc;
        psh::GetGame_ResDraw(buffer,loc);
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
                player->_hp = hp;

                if (player->_hp<= 0)
                {
                    player->Disconnect();

                    return DisconnectWaitState::Get();
                }

            }
        }
        break;
    case psh::eGame_ResChat:
    {
        psh::ObjectID target;
        String chat;
        psh::GetGame_ResChat(buffer, target,chat);
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
    switch (type)
    {
    case psh::eGame_ResLevelEnter:
    {
        psh::AccountNo accountNo;
        psh::ServerType server;
        psh::ObjectID myID;
        psh::GetGame_ResLevelEnter(buffer, accountNo, myID, server);
        player->CheckPacket(type);
        ASSERT_CRASH(accountNo == player->_accountNo, "InvalidAccountNO");
        ASSERT_CRASH(server == player->_containGroup, L"Invaid Group move");
        player->_me = myID;
        player->_target = -1;
        player->_isMove = false;
        player->_moveTime = 0;
        player->_toDestinationTime = 0;
        player->LevelChangeComp();
        player->_containGroup = server;
        player->_dest = { 0,0 };
        player->_location = { 0,0 };
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
            ASSERT_CRASH(false, L"Invalid Group");
            return nullptr;
        }
    }
    case psh::eGame_ResCreateActor:
    {
        psh::ObjectID objectId;
        bool isSpawn;
        psh::eCharacterGroup group;
        char charType;
        psh::FVector loc;
        psh::FVector dir;

        psh::GetGame_ResCreateActor(buffer, objectId, group, charType, loc, dir, isSpawn);
    }
    break;
    case psh::eGame_ResMoveStop:
    {
        ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
        psh::FVector loc;
        psh::ObjectID id;
        psh::GetGame_ResMoveStop(buffer, id, loc);
    }
    break;
    default :
        return GameState::HandlePacket(type, player, buffer);
    }
    return this;
}

PlayerState* VillageState::Update(Player* player, int time)
{
    if(needAct(permil.toField))
    {
        auto max = static_cast<int>(psh::ServerType::End)-1;
        auto target = static_cast<psh::ServerType>(psh::RandomUtil::Rand(1, max));
        player->ReqLevelChange(target);
        player->_target = -1;
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
        return GameState::HandlePacket(type,player,buffer);
    }
}

PlayerState* PveState::Update(Player* player, int time)
{
    if(needAct(permil.toVillage))
    {
        auto target = static_cast<psh::ServerType>(0);
        player->ReqLevelChange(target);
        player->_target = -1;
        return LevelChangeState::Get();
    }

    return GameState::Update(player, time);
}

PlayerState* PveState::HandlePacket(psh::ePacketType type, Player* player, CRecvBuffer& buffer)
{
    switch (type)
    {
    case psh::eGame_ResAttack:
    {
        ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
        psh::ObjectID objectId;
        char attackType;
        psh::FVector dir;
        psh::GetGame_ResAttack(buffer, objectId, attackType, dir);
        if (objectId == player->_me)
        {

            player->CheckPacket(type);
        }
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
            player->_hp = hp;

            if (player->_hp <= 0)
            {
                player->Disconnect();

                return DisconnectWaitState::Get();
            }
        }
    }
    break;
    case psh::eGame_ResMove:
    {
        ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
        psh::FVector loc;
        psh::ObjectID id;
        psh::eCharacterGroup group;
        psh::GetGame_ResMove(buffer, id, group, loc);

        if (id == player->_me)
        {
            player->CheckPacket(type);
        }
        else if (player->_target == -1 
            && group == psh::eCharacterGroup::Monster 
            && (loc - player->_spawnLocation).Size() <= 300)
        {
            player->_target = id;
            player->Move(loc);
        }
        else if (id == player->_target)
        {
            player->Move(loc);
        }
    }
    break;
    default:
        return GameState::HandlePacket(type, player, buffer);
    }
    return this;
}

PlayerState* PvpState::Update(Player* player, int time)
{
    if (needAct(permil.toVillage))
    {
        auto target = static_cast<psh::ServerType>(0);
        player->ReqLevelChange(target);
        player->_target = -1;
        return LevelChangeState::Get();
    }

    return GameState::Update(player, time);
}

PlayerState* PvpState::HandlePacket(psh::ePacketType type, Player* player, CRecvBuffer& buffer)
{
    switch (type)
    {
    case psh::eGame_ResAttack:
    {
        ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
        psh::ObjectID objectId;
        char attackType;
        psh::FVector dir;
        psh::GetGame_ResAttack(buffer, objectId, attackType,dir);
        if (objectId == player->_me)
        {
            player->CheckPacket(type);
        }
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
            player->_hp = hp;

            if (player->_hp <= 0)
            {
                player->Disconnect();

                return DisconnectWaitState::Get();
            }
        }
    }
    break;
    case psh::eGame_ResMove:
    {
        ASSERT_CRASH(player->_me != -1, L"NotCreated But RecvPacket");
        psh::FVector loc;
        psh::ObjectID id;
        psh::eCharacterGroup group;
        psh::GetGame_ResMove(buffer, id, group, loc);
        if (id == player->_me)
        {
            player->CheckPacket(type);
        }
        else if (player->_target == -1 
            && (loc - player->_spawnLocation).Size() <= 300)
        {
            player->_target = id;
            player->Move(loc);
        }
        else if (id == player->_target)
        {

            player->Move(loc, Player::targetMove);
        }
    }
    break;
    case psh::eGame_ResCreateActor:
    {
        psh::ObjectID objectId;
        bool isSpawn;
        psh::eCharacterGroup group;
        char charType;
        psh::FVector loc;
        psh::FVector dir;

        psh::GetGame_ResCreateActor(buffer, objectId,group,charType, loc, dir, isSpawn);
        if (player->_me == objectId)
        {
            player->_spawnLocation = loc;
            player->_location = loc;
            player->_dest = loc;
            player->needUpdate = true;
        }
        else if (player->_target == -1
            && player->_me != objectId
            && group != psh::eCharacterGroup::Item
            && needAct(permil.target))
        {
            player->_target = objectId;
            player->Move(loc, Player::newTarget);
        }
    }
    break;
    default:
        return GameState::HandlePacket(type, player, buffer);
        break;
    }
    return this;
}

PlayerState* DisconnectWaitState::HandlePacket(psh::ePacketType type, Player* player, CRecvBuffer& buffer)
{
    switch (type)
    {

    case psh::eGame_ResLevelEnter:
    {
        psh::AccountNo accountNo;
        psh::ObjectID myId;
        psh::ServerType server;
        psh::GetGame_ResLevelEnter(buffer, accountNo, myId, server);

    }
    break;
    default:
        return  GameState::HandlePacket(type, player, buffer);
        break;
    }
    return this;
}
