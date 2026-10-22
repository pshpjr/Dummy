#include "Server.h"

#include <filesystem>

#include "DummyData.h"

#include "DummyGroup.h"
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
    dummyParser.GetValue(L"permil.moveRange", gPermil.moveRange);
    dummyParser.GetValue(L"permil.moveOffset", gPermil.moveOffset);
    dummyParser.GetValue(L"permil.setTarget", gPermil.target);

    dummyParser.GetValue(L"interval.move", gInterval.move);
    dummyParser.GetValue(L"interval.chat", gInterval.chat);
    dummyParser.GetValue(L"session.median", gInterval.sessionMedian);
    dummyParser.GetValue(L"session.mean", gInterval.sessionMean);
    dummyParser.GetValue(L"interval.fieldChange", gInterval.fieldChange);
    dummyParser.GetValue(L"interval.toVillage", gInterval.toVillage);
    dummyParser.GetValue(L"interval.toField", gInterval.toField);

}


void Server::OnStart()
{
    int groups = (gData.maxPlayer + gData.playerPerGroup -1)/ gData.playerPerGroup;

    _dummyMonitors.resize(groups);

    for (int i = 0; i < groups; ++i)
    {
        if (ServerDown()) break;
        CreateGroup<DummyGroup>(_dummyMonitors[i]);
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }
}

void Server::OnRecvPacket(SessionID sessionId, RecvBuffer& buffer)
{
    psh::util::DebugBreak();
}

void Server::StopForServerDown()
{
    if (!_serverDown.exchange(true, std::memory_order_acq_rel))
    {
        std::cout << "[Dummy] event=ServerDown action=stop" << std::endl;
        Stop();
    }
}

void Server::OnEnd()
{
    // All worker threads have joined. Any remaining unresolved resets belong to
    // the confirmed server-down interval and must not become Invalid on teardown.
    unsigned long long invalid = 0, serverStop = 0, pending = 0;
    for (const auto& monitor : _dummyMonitors)
    {
        invalid += monitor.invalidDisconnects;
        serverStop += monitor.serverStopDisconnects;
        pending += monitor.pendingDisconnects;
    }
    if (ServerDown())
    {
        serverStop += pending;
        pending = 0;
    }
    std::cout << "[Dummy] event=" << (ServerDown() ? "ServerStop" : "Stopped")
              << " invalid=" << invalid << " server_stop=" << serverStop
              << " pending=" << pending << std::endl;
}
