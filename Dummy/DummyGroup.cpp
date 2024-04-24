#include "DummyGroup.h"

#include <codecvt>

#include "IOCP.h"
#include "Player.h"

void DummyGroup::OnCreate()
{
    g_AccountNo = (int(GetGroupID()) -1) * gData.playerPerGroup;

   
    for (int i = 0; i < gData.playerPerGroup; i++)
    {
        _accounts.push(g_AccountNo + i+ gData.startAccount);
    }
}

void DummyGroup::OnUpdate(int milli)
{
   
    auto toConnect =  min(_maxPlayerCount - _players.size(),5);
    
    for(int i = 0; i< toConnect;i++)
    {
        auto newPlayer = _iocp->GetClientSession(_ip,_port);
        auto id = newPlayer.Value();
        _iocp->MoveSession(id,GetGroupID());
        auto account = _accounts.top(); _accounts.pop();
        _players.emplace(id, make_unique<Player>(id, account, _iocp,_dummyLogger));
    }


    for (auto& [_, player] : _players)
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

    if (chrono::steady_clock::now() > _nextMonitor)
    {
        int delaySum = 0;
        int delayMax = 0;
        for (auto& [_, player] : _players)
        {
            //ASSERT_CRASH(player->moveCount < 20);
            player->moveCount = 0;

            delaySum += player->_actionDelay;

            delayMax = max(delayMax, player->_actionDelay);
            player->_actionDelay = 0;
        }

        int delayAvg = delaySum / _players.size();

        printf("Group %d , work : %lld QUeued : %d, Handled : %lld\n delay : %d, max : %d\n", int(GetGroupID()), GetWorkTime(), GetQueued(), GetJobTps(), delayAvg,delayMax);
        _nextMonitor += 1s;
    }

}

void DummyGroup::OnEnter(SessionID id)
{
    
}

void DummyGroup::OnLeave(SessionID id, int wsaErrCode)
{
    auto it = _players.find(id);
    auto player = it->second.get();
    if(!player->State()->ValidDisconnect())
    {
        ASSERT_CRASH(false);
        std::string cState = typeid(*player->State()).name();
        String wState = String(cState.begin(),cState.end());
        //String toPrint = std::format(L"InvalidDisconnect.  state : {}",typeid(player->State()).name());
        _dummyLogger.Write(L"DummyDisconnect",CLogger::LogLevel::Error
            ,L"InvalidDisconnect AccountNO : %lld, state : %s, WSAError : %d",player->_accountNo,wState.c_str(), wsaErrCode);
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
