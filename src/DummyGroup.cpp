#include "DummyGroup.h"

#include <algorithm>

#include "IOCP.h"
#include "Player.h"
#include "PlayerState.h"
#include <CoreGlobal.h>

using namespace std::chrono_literals;

DummyGroup::DummyGroup(Server::dummyMonitor& monitor)
    :  _ip(gData.ip), _port(gData.port)
    , _maxPlayerCount(gData.playerPerGroup)
    , _useDB(gData.useDB)
, _monitor(monitor)
{
    SetLoopMs(gPermil.loopMs);
}

void DummyGroup::OnCreate()
{
    g_AccountNo = (long(GetGroupID()) -1) * gData.playerPerGroup;

   psh::RandomUtil::SRand(long(time(nullptr)));
    for (int i = 0; i < gData.playerPerGroup; i++)
    {
        _accounts.push(g_AccountNo + i+ gData.startAccount);
    }
}

void DummyGroup::OnUpdate(int milli)
{
    const auto now = psh::SteadyClock::Now();
    // Never let failed connects prevent reclamation of disconnected players/accounts.
    CleanupDisconnected(now);
    if (static_cast<Server*>(_iocp)->ServerDown())
    {
        EnterServerDown();
        return;
    }
    const auto ip = Ip::Parse(_ip);
    if (!ip.has_value()) return;

    if (now >= _nextConnectAttempt)
    {
        const auto toConnect = std::min(_maxPlayerCount - static_cast<int>(_players.size()), 5);
        for (int i = 0; i < toConnect && !_accounts.empty(); ++i)
        {
            auto connection = _iocp->GetClientSession(*ip, _port);
            if (connection.HasError())
            {
                if (connection.Error() == NetErrc::ConnectionRefused)
                {
                    EnterServerDown();
                    return;
                }
                // Other failures do not establish ServerDown or settle pending resets.
                gLogger->Write(L"Connection fail", Logger::LogLevel::Invalid,
                               L"Err when Connect Errcode : %d", connection.Error().GetNativeCode());
                _nextConnectAttempt = now + std::chrono::milliseconds(std::max(gData.reconnect, 1000));
                break;
            }
            RegisterPlayer(connection.Value());
        }
    }

    for (auto& [id, player] : _players)
    {
        if (!_disconnectedSessions.contains(id)) player->Update(milli);
    }


}

void DummyGroup::OnEnter(SessionID id)
{
    
}

void DummyGroup::OnLeave(SessionID id, int wsaErrCode)
{
    const auto it = _players.find(id);
    if (it == _players.end() || !_disconnectedSessions.insert(id).second) return;
    const auto& player = it->second;
    if (static_cast<Server*>(_iocp)->ServerDown())
    {
        EnterServerDown();
        ++_serverStopDisconnects;
    }
    else if (!player->State()->ValidDisconnect())
    {
        const std::string state = typeid(*player->State()).name();
        PendingDisconnect disconnect{player->_accountNo, String(state.begin(), state.end()), wsaErrCode};
        if (NetError::FromNativeCode(wsaErrCode) == NetErrc::ConnectionReset)
            _pendingDisconnects.insert_or_assign(player->_accountNo, std::move(disconnect));
        else
            RecordInvalid(disconnect);
    }
    _monitor.pendingDisconnects = _pendingDisconnects.size();
    _monitor.serverStopDisconnects = _serverStopDisconnects;
    _deleteWait.emplace(psh::SteadyClock::Now() + std::chrono::milliseconds(gData.reconnect), id);
}

void DummyGroup::OnRecv(SessionID id, RecvBuffer& recvBuffer)
{
    const auto player = _players.find(id);
    if (static_cast<Server*>(_iocp)->ServerDown() || player == _players.end() || _disconnectedSessions.contains(id))
    {
        // Consume queued traffic without advancing gameplay or response measurements.
        while (recvBuffer.CanPopSize() > 0)
        {
            psh::uint8 ignored;
            recvBuffer >> ignored;
        }
        return;
    }
    player->second->RecvPacket(recvBuffer);
    ASSERT_CRASH(recvBuffer.CanPopSize() == 0, "can pop from recv buffer");
}

void DummyGroup::DeleteActor(SessionID id)
{
    const auto it = _players.find(id);
    if (it == _players.end()) return;
    _accounts.push(it->second->_accountNo);
    _players.erase(it);
    _disconnectedSessions.erase(id);
}

void DummyGroup::CleanupDisconnected(psh::SteadyClock::TimePoint now)
{
    while (!_deleteWait.empty() && _deleteWait.front().first <= now)
    {
        DeleteActor(_deleteWait.front().second);
        _deleteWait.pop();
    }
}

void DummyGroup::RegisterPlayer(SessionID id)
{
    const auto account = _accounts.top();
    _accounts.pop();
    if (const auto pending = _pendingDisconnects.find(account); pending != _pendingDisconnects.end())
    {
        RecordInvalid(pending->second);
        _pendingDisconnects.erase(pending);
        _monitor.pendingDisconnects = _pendingDisconnects.size();
    }
    _players.emplace(id, std::make_unique<Player>(id, account, _iocp, _dummyLogger));
    _iocp->MoveSession(id, GetGroupID());
}

void DummyGroup::RecordInvalid(const PendingDisconnect& disconnect)
{
    ++_invalidDisconnects;
    _dummyLogger.Write(L"DummyDisconnect", Logger::LogLevel::Error,
                       L"InvalidDisconnect AccountNO : %lld, state : %s, WSAError : %d",
                       disconnect.account, disconnect.state.c_str(), disconnect.error);
    PrintDisconnectSummary("InvalidDisconnect");
}

void DummyGroup::EnterServerDown()
{
    if (!_serverDown)
    {
        _serverDown = true;
        _serverStopDisconnects += _pendingDisconnects.size();
        _pendingDisconnects.clear();
        PrintDisconnectSummary("ServerDown");
    }
    // Stop the entire dummy once, including other groups; no recovery/probe loop.
    static_cast<Server*>(_iocp)->StopForServerDown();
}

void DummyGroup::PrintDisconnectSummary(const char* event)
{
    _monitor.invalidDisconnects = _invalidDisconnects;
    _monitor.serverStopDisconnects = _serverStopDisconnects;
    _monitor.pendingDisconnects = _pendingDisconnects.size();
    _dummyLogger.Write(L"DummyDisconnect", Logger::LogLevel::System,
                       L"group : %ld, event : %s, invalid : %llu, server_stop : %llu, pending : %llu",
                       static_cast<long>(GetGroupID()), event, _invalidDisconnects, _serverStopDisconnects,
                       _pendingDisconnects.size());
}
