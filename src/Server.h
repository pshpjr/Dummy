#pragma once
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
    };

    Server();
    void SendLogin() ;
    void SendMonitorData(en_PACKET_SS_MONITOR_DATA_UPDATE_TYPE type, int value);

    void OnStart() override;
    void OnRecvPacket(SessionID sessionId, CRecvBuffer& buffer) override;
    void OnMonitorRun() override;
private:
    SettingParser dummyParser;

    Vector<dummyMonitor> _dummyMonitors;
    bool _useMonitor = false;
    SessionID _monitorSession = InvalidSessionID();
    String _monitorServerIP;
    Port _monitorServerPort;
};

