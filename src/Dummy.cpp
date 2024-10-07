// Dummy.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include "Server.h"
#pragma comment (lib,"Netlib_d.lib")
int main()
{
    std::locale::global(std::locale("ko_KR.UTF-8"));

    auto server = std::make_unique<Server>();

    server->ClientInit(4, 4, 50,true,false,false);
    server->Start();
    server->Wait();
}
