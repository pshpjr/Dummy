#pragma once
#include <unordered_set>

#include "Logger.h"
#include "Group.h"
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
    void TrackPlayers(psh::SteadyClock::TimePoint now);

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
    int _trackingElapsedMs = 0;
    unsigned long long _invalidDisconnects = 0;
    unsigned long long _serverStopDisconnects = 0;
    Server::dummyMonitor& _monitor;
};
