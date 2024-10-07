#pragma once
#include <chrono>

#include "CLogger.h"
#include "Group.h"
#include "SettingParser.h"
#include "ContentTypes.h"
#include "DummyData.h"
class Player;

class DummyGroup : public Group
{
public:

    DummyGroup();
    
    void OnCreate() override;
    void OnUpdate(int milli) override;
    void OnEnter(SessionID id) override;
    void OnLeave(SessionID id, int wsaErrCode) override;
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
    
    HashMap<SessionID,std::unique_ptr<Player>> _players;
    std::chrono::steady_clock::time_point _nextMonitor;
    bool _useDB;

    std::queue<std::pair<std::chrono::steady_clock::time_point,SessionID>> _deleteWait;
};
