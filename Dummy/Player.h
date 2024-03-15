#pragma once
#include <ContentTypes.h>
#include <PacketGenerated.h>

#include "PlayerState.h"
#include "Session.h"
#include "Types.h"
#include "FVector.h"
#include "SingleThreadCircularQ.h"





class IOCP;
class Player
{
    static constexpr float speedPerMS = 400 / 1000.0f;
    friend class PlayerState;
    friend class LoginLoginState;
    friend class GameState;
    friend class DisconnectState;
    friend class GameLoginState;
public:
    
    Player(SessionID id,psh::AccountNo accountNo, IOCP* server) : _accountNo(accountNo), containGroup(psh::ServerType::End), _packets(),
                                                                  _sessionId(id),
                                                                  _server(server)
    {
    }
    
    void CheckPacket(psh::ePacketType type);


    unsigned int GetActionDelay();


    
    void LoginLogin();
    void GameLogin();
    void ReqLevelChange(psh::ServerType type);
    void LevelChangeComp();

    void Update(int milli);
    void Move(psh::FVector location);
    void Disconnect();
    void Attack(char type);
    void RecvPacket(CRecvBuffer& buffer);

    PlayerState* State() { return _state; }
    void Stop();

    psh::ObjectID _me = -1;
    psh::ObjectID _target = -1;
    psh::AccountNo _accountNo;
    psh::ServerType containGroup;



    void Req(psh::ePacketType type,int opt = -1)
    {
        _packets.Enqueue({type,opt,std::chrono::steady_clock::now()});
    }
    
    struct moveData
    {
        bool isSend = false;
        psh::FVector dest{};
    };

    struct packet
    {
        psh::ePacketType type;
        int opt = -1;
        std::chrono::steady_clock::time_point request;
    };
    
    //서버 패킷 디버깅 관련
    unsigned int _actionDelay;
    SingleThreadCircularQ<packet,32> _packets;

    //계정 및 통신 관련
    IOCP* _server = nullptr;
    psh::ID _id = psh::ID(L"ID_1");
    psh::Password _password = psh::Password(L"PWD_1");
    psh::SessionKey _key;
    SessionID _sessionId = InvalidSessionID();

    //업데이트
    PlayerState* _state = DisconnectState::Get();

    //플레이어 정보
    int hp;
    bool isMove = false; 

    psh::FVector _location = {0,0};
    psh::FVector _dest = { 0,0 };
    float toDestinationTime = 0;
    float moveTime = 0;
    float attackCooldown = 0;
    int coin = 0;
};


class Target
{
    psh::FVector location;
    psh::FVector destination;
    int hp;
    psh::ObjectID _id;
};