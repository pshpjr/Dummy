// Dummy.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "Server.h"
#include "BuildVersion.h"
#include "CrashDump.h"
int main()
{
    CrashDump::SetBuildInfo(BuildVersion::Metadata);
    std::locale::global(std::locale("ko_KR.UTF-8"));

    auto server = std::make_unique<Server>();

    server->ClientInit(8, 8, 50,true,true,false);
    server->Start();
    server->Wait();
}
