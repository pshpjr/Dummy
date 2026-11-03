#pragma once
#include <string>
#include <unordered_set>

#include "Logger.h"
#include "Group.h"
#include "Metrics.h"
#include "SettingParser.h"
#include "ContentTypes.h"
#include "DummyData.h"
#include "Server.h"
#include "SteadyClock.h"
class Player;

class DummyGroup : public Group
{
public:

    DummyGroup(Server::dummyMonitor& monitor);
    
    void OnCreate() override;
    void OnUpdate(int milli) override;
    void OnEnter(SessionID id) override;
    void OnLeave(SessionID id, int wsaErrCode) override;
    void OnRecv(SessionID id, RecvBuffer& recvBuffer) override;
    void DeleteActor(SessionID id);
private:
    struct PendingDisconnect
    {
        psh::AccountNo account;
        String state;
        int error;
    };
    void CleanupDisconnected(psh::SteadyClock::TimePoint now);
    void RegisterPlayer(SessionID id);
    void RecordInvalid(const PendingDisconnect& disconnect);
    void EnterServerDown();
    void PrintDisconnectSummary(const char* event);
    //  플레이어를 state·reason으로 세어 게이지에 반영하고, 오래 멈춘 플레이어를 센다.
    void UpdatePlayerMetrics(psh::SteadyClock::TimePoint now);

    //  state·reason 조합마다 하나. 그룹마다 같은 시리즈에 더하므로 바뀐 만큼만 Add한다.
    struct PlayerCount
    {
        bool sleeping = false;
        psh::metrics::Gauge players;
        psh::metrics::Gauge over30s;
        psh::metrics::Gauge over5m;
        //  이번 집계 값과 게이지에 반영해 둔 값. 순서는 players, over30s, over5m.
        psh::int64 counted[3]{};
        psh::int64 published[3]{};
    };

    std::stack<psh::AccountNo> _accounts;
    psh::AccountNo g_AccountNo = 0;
    Logger _dummyLogger;
    String _ip;
    Port _port;
    int _maxPlayerCount;
    
    SettingParser _dummySettings;
    
    HashMap<SessionID,std::unique_ptr<Player>> _players;
    bool _useDB;

    std::queue<std::pair<psh::SteadyClock::TimePoint,SessionID>> _deleteWait;
    // Keep the verdict after the old Player is deleted, keyed to its replacement account.
    HashMap<psh::AccountNo, PendingDisconnect> _pendingDisconnects;
    std::unordered_set<SessionID> _disconnectedSessions;
    bool _serverDown = false;
    psh::SteadyClock::TimePoint _nextConnectAttempt{};
    HashMap<std::string, PlayerCount> _playerCounts;
    int _metricElapsedMs = 0;
    unsigned long long _invalidDisconnects = 0;
    unsigned long long _serverStopDisconnects = 0;
    Server::dummyMonitor& _monitor;
};
