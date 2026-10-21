#pragma once
#include <atomic>
#include "IOCP.h"
#include "MonitorProtocol.h"
#include "SettingParser.h"

class Server :
    public IOCP
{
public:
    struct dummyMonitor
    {
        int players;
        int delay;
        int maxDelay;
        unsigned long long invalidDisconnects = 0;
        unsigned long long serverStopDisconnects = 0;
        unsigned long long pendingDisconnects = 0;
    };

    Server();
    void SendLogin() ;
    void SendMonitorData(en_PACKET_SS_MONITOR_DATA_UPDATE_TYPE type, int value);

    void OnStart() override;
    void OnRecvPacket(SessionID sessionId, RecvBuffer& buffer) override;
    void OnMonitorRun() override;
    void OnEnd() override;
    void StopForServerDown();
    bool ServerDown() const { return _serverDown.load(std::memory_order_acquire); }
private:
    std::atomic<bool> _serverDown{false};
    SettingParser dummyParser;

    Vector<dummyMonitor> _dummyMonitors;
    bool _useMonitor = false;
    SessionID _monitorSession = InvalidSessionID();
    String _monitorServerIP;
    Port _monitorServerPort;
};

