// Dummy.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "Server.h"
#include "BuildVersion.h"
#include "CrashDump.h"
#include "ExceptionTrace.h"
#include "CloseSignalHandler.h"
#include "Console.h"

#include <atomic>
int main()
{
    psh::util::RunWithTrace("Dummy main", []
    {
        CrashDump::SetBuildInfo(BuildVersion::Metadata);
        std::locale::global(std::locale("ko_KR.UTF-8"));

        std::atomic<Server*> signalServer{nullptr};
        std::unique_ptr<Server> server;
        psh::util::CloseSignalListener listener([&]
        {
            if (auto* target = signalServer.load())
                target->Stop();
        });
        server = std::make_unique<Server>();
        signalServer.store(server.get());

        server->ClientInit(8, 8, 50,true,true,false);
        server->Start();
        psh::util::println(u"start");
        server->Wait();
    });
}
