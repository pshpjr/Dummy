#pragma once
#include <ContentTypes.h>
#include <PacketGenerated.h>

#include "PlayerState.h"
#include "Session.h"
#include "Types.h"
#include "FVector.h"
#include "SingleThreadCircularQ.h"


class CLogger; class IOCP; class DummyGroup;
class Player
{
    static constexpr float speedPerMS = 200 / 1000.0f;
    friend class PlayerState;
    friend class LoginLoginState;
    friend class GameState;
    friend class DisconnectState;
    friend class GameLoginState;
public:
    
    Player(SessionID id,psh::AccountNo accountNo, IOCP* server, CLogger& logger) : _accountNo(accountNo), _containGroup(psh::ServerType::End), _packets(),
                                                                  _sessionId(id),
                                                                  _server(server),_logger(logger)
    {

        _id = psh::ID(std::format(L"ID_{:d}", accountNo));
        _password = psh::Password(std::format(L"PASS_{:d}", accountNo));
    }
    
    void CheckPacket(psh::ePacketType type);


    unsigned int GetActionDelay();


    
    void LoginLogin();
    void GameLogin();
    void ReqLevelChange(psh::ServerType type);
    void LevelChangeComp();

    void Update(int milli);

    void Disconnect();
    void Attack(char type);
    void RecvPacket(CRecvBuffer& buffer);

    PlayerState* State() const { return _state; }
    void Stop();

    psh::ObjectID _me = -1;
    psh::ObjectID _target = -1;
    psh::AccountNo _accountNo = -1;
    psh::ServerType _containGroup;
    CLogger& _logger;
    DummyGroup* _owner = nullptr;

    void Req(psh::ePacketType type,int opt = -1,int opt2 = -1)
    {
        auto now = std::chrono::steady_clock::now();
        auto result = _packets.Enqueue({type,now,_me,opt,opt2});


        ASSERT_CRASH(result, "");
    }
    
    //struct moveData
    //{
    //    bool isSend = false;
    //    psh::FVector dest{};
    //};

    struct packet
    {
        psh::ePacketType type;
        std::chrono::steady_clock::time_point request;
        psh::ObjectID obj = -1;
        int opt = -1;
        int opt2 = -1;
    };
    
    //서버 패킷 디버깅 관련
    unsigned int _actionDelay = 0;
    SingleThreadCircularQ<packet,32> _packets;

 /*   class Target
    {
        psh::FVector location;
        psh::FVector destination;
        psh::FVector direction;
        psh::ObjectID id = -1;
        bool isMove = false;
        int hp;
    };

    Target _target;*/
    //계정 및 통신 관련
    IOCP* _server = nullptr;
    psh::ID _id = psh::ID(L"ID_1");
    psh::Password _password = psh::Password(L"PWD_1");
    psh::SessionKey _key;
    SessionID _sessionId = InvalidSessionID();
    
    //업데이트
    PlayerState* _state = DisconnectState::Get();

    //플레이어 정보
    int _hp = 0;
    bool _isMove = false; 
    bool needUpdate = true;

    //디버깅
    int moveCount = 0;
    enum moveReason {
        None,
        targetMove,
        targetStrop,
        newTarget,
        goSpawn,
        randomMove
    };
    void Move(psh::FVector location, moveReason reason = None);
    struct MoveDebug {
        moveReason reason;
        psh::ObjectID target;
        psh::FVector location;
        psh::FVector dest;
    };

    static const int moveDataSize = 100;
    int moveDataIndex = 0;

    MoveDebug moveData[100];

    void writeMoveDebug(moveReason reason)
    {



        moveData[moveDataIndex++ % moveDataSize] = { reason,_target,_location,_dest };

    }


    psh::FVector _location = {0,0};
    psh::FVector _dest = { 0,0 };
    psh::FVector _dir = { 0,0 };
    psh::FVector _attackDir = { 0,0 };
    psh::FVector _spawnLocation = { 0,0 };

    int _moveDelay = 0;
    float _toDestinationTime = 0;
    float _moveTime = 0;
    float _attackCooldown = 0;
    int _coin = 0;

    //삭제 및 재접속 관련
    int _disconnectWait = 0;
};


