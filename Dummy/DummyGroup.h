#pragma once
#include "CLogger.h"
#include "Group.h"
#include "SettingParser.h"
#include "ContentTypes.h"
#include "DummyData.h"
class Player;

class DummyGroup : public Group
{
public:
    DummyGroup()
        : _dummyLogger(L"DummyLog.txt")
        , _ip(gData.ip), _port(gData.port)
        , _maxPlayerCount(gData.playerPerGroup)
        , _nextMonitor(std::chrono::steady_clock::now()+1s)
        , _useDB(gData.useDB)
    {SetLoopMs(gPermil.loopMs); }
    
    void OnCreate() override;
    void OnUpdate(int milli) override;
    void OnEnter(SessionID id) override;
    void OnLeave(SessionID id) override;
    void OnRecv(SessionID id, CRecvBuffer& recvBuffer) override;
    void DeleteActor(SessionID id);
private:
    std::stack<psh::AccountNo> _accounts;
    psh::AccountNo g_AccountNo = 0;
    CLogger _dummyLogger;
    String _ip;
    Port _port;
    int _maxPlayerCount;
    
    SettingParser _dummySettings;
    
    HashMap<SessionID,unique_ptr<Player>> _players;
    std::chrono::steady_clock::time_point _nextMonitor;
    bool _useDB;

    queue<pair<std::chrono::steady_clock::time_point,SessionID>> _deleteWait;
};
