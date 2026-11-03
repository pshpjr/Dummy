#include "DummyGroup.h"

#include <algorithm>
#include <array>

#include "IOCP.h"
#include "Player.h"
#include "PlayerState.h"
#include <CoreGlobal.h>

using namespace std::chrono_literals;

namespace
{
    //  OnLeave에서 연결이 끊긴 경위. dummy.leaves 메트릭의 kind 라벨이다.
    enum class LeaveKind
    {
        //  더미가 스스로 끊었다.
        Expected,
        //  예상 밖으로 리셋됐다. 같은 계정이 다시 접속하면 Invalid로 확정한다.
        Reset,
        Invalid,
        ServerStop,
        End
    };

    const psh::metrics::Counter& GetLeaveMetric(const LeaveKind kind)
    {
        static const auto metrics = []
        {
            using psh::metrics::Registry;
            const char* names[] = {"expected", "reset", "invalid", "server_stop"};
            std::array<psh::metrics::Counter, static_cast<size_t>(LeaveKind::End)> result;
            for (size_t i = 0; i < result.size(); ++i)
            {
                result[i] = Registry::CreateCounter("dummy.leaves", "sessions that ended, by how they ended",
                                                    {{"kind", names[i]}});
            }
            return result;
        }();
        return metrics[static_cast<size_t>(kind)];
    }

    //  끊긴 세션이 연결되어 있던 시간의 합. rate 비를 취하면 평균 세션 길이다.
    const psh::metrics::Counter& GetSessionSeconds()
    {
        static const auto metric = psh::metrics::Registry::CreateCounter(
            "dummy.session.seconds", "total connected seconds of ended sessions");
        return metric;
    }
}

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
    _metricElapsedMs += milli;
    if (_metricElapsedMs >= 1000)
    {
        _metricElapsedMs = 0;
        UpdatePlayerMetrics(now);
    }
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
    auto kind = LeaveKind::Expected;
    if (static_cast<Server*>(_iocp)->ServerDown())
    {
        EnterServerDown();
        ++_serverStopDisconnects;
        kind = LeaveKind::ServerStop;
    }
    else if (!player->State()->ValidDisconnect())
    {
        const std::string state = typeid(*player->State()).name();
        PendingDisconnect disconnect{player->_accountNo, String(state.begin(), state.end()), wsaErrCode};
        if (NetError::FromNativeCode(wsaErrCode) == NetErrc::ConnectionReset)
        {
            _pendingDisconnects.insert_or_assign(player->_accountNo, std::move(disconnect));
            kind = LeaveKind::Reset;
        }
        else
        {
            RecordInvalid(disconnect);
            kind = LeaveKind::Invalid;
        }
    }
    GetLeaveMetric(kind).Add();
    GetSessionSeconds().Add(psh::SteadyClock::ElapsedMs(player->_connectedAt) / 1000);
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

void DummyGroup::UpdatePlayerMetrics(const psh::SteadyClock::TimePoint now)
{
    for (auto& [key, count] : _playerCounts)
    {
        std::fill(std::begin(count.counted), std::end(count.counted), 0);
    }

    for (auto& [id, player] : _players)
    {
        const auto sleepMs = player->TrackSleep(now);
        const auto state = player->State()->Name();
        const auto reason = Player::SleepName(player->_sleepReason);
        const auto key = std::string(state) + '/' + reason;

        auto it = _playerCounts.find(key);
        if (it == _playerCounts.end())
        {
            using psh::metrics::Registry;
            const psh::metrics::Attributes attributes{{"state", state}, {"reason", reason}};
            PlayerCount count;
            count.sleeping = player->_sleepReason != Player::SleepReason::Acting;
            count.players = Registry::CreateGauge("dummy.players", "dummies by state and why they are not acting",
                                                  attributes);
            if (count.sleeping)
            {
                auto over = [&](const char* label)
                {
                    auto withOver = attributes;
                    withOver.push_back({"over", label});
                    return Registry::CreateGauge("dummy.players.sleeping", "dummies not acting for longer than 'over'",
                                                 withOver);
                };
                count.over30s = over("30s");
                count.over5m = over("5m");
            }
            it = _playerCounts.emplace(key, std::move(count)).first;
        }

        auto& count = it->second;
        ++count.counted[0];
        if (sleepMs >= 30'000) ++count.counted[1];
        if (sleepMs >= 300'000) ++count.counted[2];
    }

    for (auto& [key, count] : _playerCounts)
    {
        count.players.Add(count.counted[0] - count.published[0]);
        if (count.sleeping)
        {
            count.over30s.Add(count.counted[1] - count.published[1]);
            count.over5m.Add(count.counted[2] - count.published[2]);
        }
        std::copy(std::begin(count.counted), std::end(count.counted), std::begin(count.published));
    }
}
