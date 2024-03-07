#pragma once
#include "IOCP.h"
#include "Player.h"
#include "SettingParser.h"
class Server :
    public IOCP
{
public:
    Server();

    void OnStart() override;
    void OnRecvPacket(SessionID sessionId, CRecvBuffer& buffer) override;
    void OnMonitorRun() override;
private:
    SettingParser dummyParser;
};

