#include "DummyGroup.h"

#include <codecvt>

#include "IOCP.h"
#include "Player.h"
#include "PlayerState.h"
#include <CoreGlobal.h>

using namespace std::chrono_literals;

DummyGroup::DummyGroup()
    :  _ip(gData.ip), _port(gData.port)
    , _maxPlayerCount(gData.playerPerGroup)
    , _nextMonitor(std::chrono::steady_clock::now() + 1s)
    , _useDB(gData.useDB)
{

    SetLoopMs(gPermil.loopMs);
}

void DummyGroup::OnCreate()
{
    g_AccountNo = (long(GetGroupID()) -1) * gData.playerPerGroup;

   
    for (int i = 0; i < gData.playerPerGroup; i++)
    {
        _accounts.push(g_AccountNo + i+ gData.startAccount);
    }
}

void DummyGroup::OnUpdate(int milli)
{
   
    auto toConnect = std::min(static_cast<unsigned long long>(_maxPlayerCount - _players.size()), 5ull);
    
    for(int i = 0; i< toConnect;i++)
    {
        auto newPlayer = _iocp->GetClientSession(_ip,_port);
        if (newPlayer.HasError())
        {
            gLogger->Write(L"Connection fail", CLogger::LogLevel::Invalid, L"Err when Connect Errcode : %d", newPlayer.Error());
            return;
        }


        auto id = newPlayer.Value();
        _iocp->MoveSession(id,GetGroupID());
        auto account = _accounts.top(); _accounts.pop();
        _players.emplace(id, std::make_unique<Player>(id, account, _iocp,_dummyLogger));
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

    if (std::chrono::steady_clock::now() > _nextMonitor)
    {
        int delaySum = 0;
        unsigned int delayMax = 0;
        for (auto& [_, player] : _players)
        {

            delaySum += player->_actionDelay;

            delayMax = std::max(delayMax, player->_actionDelay);
            player->_actionDelay = 0;
        }

        int delayAvg = delaySum / _players.size();

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
