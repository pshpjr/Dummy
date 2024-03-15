#include "DummyGroup.h"

#include "IOCP.h"
#include "Player.h"

void DummyGroup::OnCreate()
{
    g_AccountNo = GetGroupID() * gData.playerPerGroup;
}

void DummyGroup::OnUpdate(int milli)
{
    if (chrono::steady_clock::now() > _nextMonitor)
    {
        printf("Group %d , work : %d QUeued : %d", GetGroupID(), GetWorkTime(),GetQueued() );
        _nextMonitor += 1s;
    }

    auto toConnect =  _maxPlayerCount - _players.size();
    
    for(int i = 0; i<toConnect;i++)
    {
        auto newPlayer = _iocp->GetClientSession(_ip,_port);
        auto id = newPlayer.Value();
        _iocp->MoveSession(id,GetGroupID());

        _players.emplace(id, make_unique<Player>(id,g_AccountNo++, _iocp));
    }

    for(auto& [_,player] : _players)
    {
        player->Update(milli);
    }
}

void DummyGroup::OnEnter(SessionID id)
{
    
}

void DummyGroup::OnLeave(SessionID id)
{
    auto player = _players.find(id);

    ASSERT_CRASH(player->second->State()->ValidDisconnect(),L"InvalidDisconnect");

    auto eraseCount = _players.erase(id);
    ASSERT_CRASH(eraseCount == 1,"Disconnec Err");
    
    _dummyLogger.Write(L"DummyDisconnect",CLogger::LogLevel::Error,L"TODO");
}

void DummyGroup::OnRecv(SessionID id, CRecvBuffer& recvBuffer)
{
    auto player = _players.find(id);

    player->second->RecvPacket(recvBuffer);

        
}
