#pragma once
#include <ContentTypes.h>
#include <PacketGenerated.h>

#include "Session.h"
#include "Types.h"
#include "FVector.h"
#include "SingleThreadCircularQ.h"


namespace psh {
    class PlayerState;
}

class CLogger; class IOCP; class DummyGroup;
class Player
{
    static constexpr float SPEED_PER_MS = 200 / 1000.0f;
    friend class PlayerState;
    friend class LoginLoginState;
    friend class GameState;
    friend class DisconnectState;
    friend class GameLoginState;


public:
    enum moveReason {
        None,
        targetMove,
        targetStop,
        newTarget,
        goSpawn,
        randomMove
    };

    Player(SessionID id,psh::AccountNo accountNo, IOCP* server, CLogger& logger);
    void CheckPacket(psh::ePacketType type);
    unsigned int GetActionDelay();

    void LoginLogin();
    void GameLogin();
    void ReqLevelChange(psh::ServerType type);
    void LevelChangeComp();
    void Chat();
    void CalculateLocation();
    void SetTarget(psh::ObjectID id, psh::FVector location);
    void UpdateLocation(int time);
    void Update(int milli);
    void Disconnect();
    void Attack(char type);
    void RecvPacket(CRecvBuffer& buffer);
    psh::PlayerState* State() const { return _state; }
    void Stop(psh::FVector newLocation);
    void Move(psh::FVector destination, moveReason reason = None);

    void Req(psh::ePacketType type,int opt = -1,int opt2 = -1,int opt3 = -1,int opt4 = -1)
    {
        auto now = std::chrono::steady_clock::now();
        auto result = _packets.Enqueue({type,now,_me,opt,opt2,opt3,opt4});


        ASSERT_CRASH(result, "");
    }

    // 디버깅 정보
    struct packet
    {
        psh::ePacketType type;
        std::chrono::steady_clock::time_point request;
        psh::ObjectID obj = -1;
        int opt = -1;
        int opt2 = -1;
        int opt3 = -1;
        int opt4 = -1;
    };
    unsigned int _actionDelay = 0;
    SingleThreadCircularQ<packet, 32> _packets;

    //이동 디버깅
    struct MoveDebug {
        moveReason reason;
        psh::ObjectID target;
        psh::FVector location;
        psh::FVector dest;
    };

    // static const int moveDataSize = 100;
    // int moveDataIndex = 0;
    // MoveDebug moveData[100];
    // void writeMoveDebug(moveReason reason)
    // {
    //     moveData[moveDataIndex++ % moveDataSize] = { reason, _target, _location, _dest };
    // }

    // 플레이어 위치 및 이동 정보
    psh::FVector _location = {0, 0};
    psh::FVector _destination = {0, 0};
    psh::FVector _moveDir = {0, 0};
    psh::FVector _attackDir = {0, 0};
    psh::FVector _spawnLocation = {0, 0};
    int _moveDelay = 0;
    int _toDestinationTime = 0;
    int _moveTime = 0;
    int _attackCooldown = 0;

    // 계정 및 통신 정보
    psh::ObjectID _me = -1;
    psh::ObjectID _target = -1;
    psh::AccountNo _accountNo = -1;
    psh::TemplateID _templateID = -1;
    psh::ServerType _containGroup;
    psh::ID _id = psh::ID(L"ID_1");
    psh::Password _password = psh::Password(L"PWD_1");
    psh::SessionKey _key;
    SessionID _sessionId = InvalidSessionID();
    DummyGroup* _owner = nullptr;

    // 삭제 및 재접속 관련
    int _disconnectWait = 0;

    // 서버 정보
    IOCP* _server = nullptr;

    // 플레이어 상태 및 기타 정보
    int _hp = 0;
    bool _isMove = false;
    bool needUpdate = true;
    int _coin = 0;
    int moveCount = 0;

    // 플레이어 상태
    psh::PlayerState* _state;

    // 로깅
    CLogger& _logger;
};


