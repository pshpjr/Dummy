#include "Server.h"
#include "DummyData.h"

#include "DummyGroup.h"
#include "TextFileReader.h"

Server::Server() : IOCP(false)
{
    try
    {
        dummyParser.Init(L"DummySetting.txt");
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
    dummyParser.GetValue(L"permil.toField", gPermil.toField);
    dummyParser.GetValue(L"permil.toVillage", gPermil.toVillage);
    dummyParser.GetValueOrDefault(L"permil.chat", gPermil.chat, L"100");

}

void Server::OnStart()
{
    int groups = (gData.maxPlayer + gData.playerPerGroup -1)/ gData.playerPerGroup;
    for (int i = 0; i < groups; ++i)
    {
        CreateGroup<DummyGroup>();
    }
}

void Server::OnRecvPacket(SessionID sessionId, CRecvBuffer& buffer)
{
    DebugBreak();
}

void Server::OnMonitorRun()
{
    PrintMonitorString();
}
