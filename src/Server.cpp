#include "Server.h"

#include <filesystem>

#include "DummyData.h"

#include "DummyGroup.h"
#include "MonitorProtocol.h"
#include "TextFileReader.h"



Server::Server() : IOCP()
{
    try
    {
        dummyParser.Init(L"DummySetting.toml");
    }
    catch (std::exception& e)
    {
        std::cout << e.what();
    }

    gChatText = new TextFileReader(L"chatData.txt");
  
    dummyParser.GetValue(L"Base.IP", gData.ip);
    dummyParser.GetValue(L"Base.Port", gData.port);
    dummyParser.GetValue(L"Base.useDB", gData.useDB);
    dummyParser.GetValue(L"Base.maxPlayer", gData.maxPlayer);
    dummyParser.GetValue(L"Base.playerPerGroup", gData.playerPerGroup);
    dummyParser.GetValue(L"Base.startAccount", gData.startAccount);
    dummyParser.GetValue(L"Base.reconnectTime", gData.reconnect);

    dummyParser.GetValue(L"permil.loopMs", gPermil.loopMs);
    dummyParser.GetValue(L"permil.move", gPermil.move);
    dummyParser.GetValue(L"permil.moveRange", gPermil.moveRange);
    dummyParser.GetValue(L"permil.moveOffset", gPermil.moveOffset);
    dummyParser.GetValue(L"permil.disconnect", gPermil.disconnect);
    dummyParser.GetValue(L"permil.setTarget", gPermil.target);
    dummyParser.GetValue(L"permil.fieldChange", gPermil.fieldChange);
    dummyParser.GetValueOrDefault(L"permil.chat", gPermil.chat, L"25");

    dummyParser.GetValue(L"monitor.useMonitor", _useMonitor);

    if (!_useMonitor)
    {
        return;
    }
    dummyParser.GetValue(L"monitor.ip", _monitorServerIP);
    dummyParser.GetValue(L"monitor.port", _monitorServerPort);
}


// 모니터 로그인 전송
void Server::SendLogin()
{
    auto buffer = SendBuffer::Alloc();
    buffer << en_PACKET_SS_MONITOR_LOGIN << static_cast<WORD>(1) << static_cast<char>(
        static_cast<long>(0));
    SendPacket(_monitorSession, buffer);
}

// 모니터링 데이터 전송
void Server::SendMonitorData(const en_PACKET_SS_MONITOR_DATA_UPDATE_TYPE type, const int value)
{
    auto buffer = SendBuffer::Alloc();
    buffer << en_PACKET_SS_MONITOR_DATA_UPDATE << static_cast<WORD>(1) << static_cast<
        char>(0) << type << value << static_cast<int>(time(nullptr));
    SendPacket(_monitorSession, buffer);
}

void Server::OnStart()
{
    int groups = (gData.maxPlayer + gData.playerPerGroup -1)/ gData.playerPerGroup;

    _dummyMonitors.resize(groups);

    for (int i = 0; i < groups; ++i)
    {
        CreateGroup<DummyGroup>(_dummyMonitors[i]);
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }
}

void Server::OnRecvPacket(SessionID sessionId, RecvBuffer& buffer)
{
    DebugBreak();
}

void Server::OnMonitorRun()
{
    if(_useMonitor == false)
        return;
    if (_monitorSession == InvalidSessionID())
    {
        auto client = GetClientSession(_monitorServerIP, _monitorServerPort);
        if (client.HasError())
        {
            return;
        }
        _monitorSession = client.Value();
        SetSessionStaticKey(_monitorSession, 0);
        SendLogin();
    }
    else if (!IsValidSession(_monitorSession))
    {
        std::cout << "monitor Connection fail"<<std::endl;
        _monitorSession = InvalidSessionID();
        return;
    }

    int max = 0;
    int sum = 0;
    for(auto data: _dummyMonitors)
    {
        sum += data.delay;
        max = std::max(data.maxDelay,max);
    }
    auto avg = int(sum/_dummyMonitors.size());

    SendMonitorData(dfMONITOR_DATA_TYPE_GAME_DUMMY_DELAY_AVG,avg );
    SendMonitorData(dfMONITOR_DATA_TYPE_GAME_DUMMY_DELAY_MAX, max);
}
