#include "DummyGroup.h"

#include <codecvt>

#include "IOCP.h"
#include "Player.h"

void DummyGroup::OnCreate()
{
    g_AccountNo = (int(GetGroupID()) -1) * gData.playerPerGroup;

   
    for (int i = 0; i < gData.playerPerGroup; i++)
    {
        _accounts.push(g_AccountNo + i);
    }
}

void DummyGroup::OnUpdate(int milli)
{
   
    if (chrono::steady_clock::now() > _nextMonitor)
    {
        printf("Group %d , work : %lld QUeued : %d, Handled : %lld\n", int(GetGroupID()), GetWorkTime(),GetQueued(),GetJobTps() );
        _nextMonitor += 1s;
    }

    auto toConnect =  _maxPlayerCount - _players.size();
    
    for(int i = 0; i<toConnect;i++)
    {
        auto newPlayer = _iocp->GetClientSession(_ip,_port);
        auto id = newPlayer.Value();
        _iocp->MoveSession(id,GetGroupID());
        auto account = _accounts.top(); _accounts.pop();
        _players.emplace(id, make_unique<Player>(id, account, _iocp,_dummyLogger));
    }

    for(auto& [_,player] : _players)
    {
        player->Update(milli);
    }

    auto now = std::chrono::steady_clock::now();
    while (!_deleteWait.empty())
    {
        auto& front = _deleteWait.front();
        if (front.first <= now)
        {
            DeleteActor(front.second);
            _deleteWait.pop();
        }
        else
        {
            break;
        }
    }


}

void DummyGroup::OnEnter(SessionID id)
{
    
}

void DummyGroup::OnLeave(SessionID id)
{
    auto it = _players.find(id);
    auto player = it->second.get();
    if(!player->State()->ValidDisconnect())
    {
        std::string cState = typeid(*player->State()).name();
        String wState = String(cState.begin(),cState.end());
        //String toPrint = std::format(L"InvalidDisconnect.  state : {}",typeid(player->State()).name());
        _dummyLogger.Write(L"DummyDisconnect",CLogger::LogLevel::Error
            ,L"InvalidDisconnect AccountNO : %lld, state : %s",player->_accountNo,wState.c_str());
    }

    _deleteWait.emplace(std::chrono::steady_clock::now() + std::chrono::milliseconds(gData.reconnect), player->_sessionId);
}

void DummyGroup::OnRecv(SessionID id, CRecvBuffer& recvBuffer)
{
    auto player = _players.find(id);

    player->second->RecvPacket(recvBuffer);
    ASSERT_CRASH(recvBuffer.CanPopSize() == 0, "can pop from recv buffer");
}

void DummyGroup::DeleteActor(SessionID id)
{
    auto it = _players.find(id);
    auto player = it->second.get();
    _accounts.push(player->_accountNo);
    _players.erase(it);
}
