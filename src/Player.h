#include "Macro.h"
#pragma once
#include <ContentTypes.h>
#include <PacketGenerated.h>

#include "Session.h"
#include "SteadyClock.h"
#include "Types.h"
#include "FVector.h"
#include "SingleThreadCircularQ.h"


namespace psh {
    class PlayerState;
}

class Logger; class IOCP; class DummyGroup;
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

    //  끊는 이유. dummy.disconnects 메트릭의 reason 라벨이다.
    enum class DisconnectReason {
        SessionEnd,
        //  내 HP가 0 이하가 되는 Hit를 받았다.
        Death,
        //  HP가 0 이하일 때 내 액터가 지워졌다.
        DestroyDead,
        //  HP가 남았는데 내 액터가 지워졌다. 서버와 상태가 어긋난 것이다.
        DestroyAlive,
        LoginFail,
        End
    };

    Player(SessionID id,psh::AccountNo accountNo, IOCP* server, Logger& logger);
    //  응답을 받지 못한 요청을 대기 수에서 뺀다.
    ~Player();
    void CheckPacket(psh::ePacketType type);

    void LoginLogin();
    void GameLogin();
    void ReqLevelChange(psh::ServerType type);
    void LevelChangeComp();
    void Chat();
    void CalculateLocation();
    void SetTarget(psh::ObjectID id, psh::FVector location, int range);
    void UpdateLocation(int time);
    void Update(int milli);
    void Disconnect(DisconnectReason reason);
    void Attack(char type);
    void RecvPacket(RecvBuffer& buffer);
    psh::PlayerState* State() const { return _state; }
    void Stop(psh::FVector newLocation);
    void Move(psh::FVector destination, moveReason reason = None);

    //  응답을 기다릴 요청을 기록한다. 보낸 수와 대기 수 메트릭도 여기서 센다.
    void Req(psh::ePacketType type,int opt = -1,int opt2 = -1,int opt3 = -1,int opt4 = -1);

    // 디버깅 정보
    struct packet
    {
        psh::ePacketType type;
        psh::SteadyClock::TimePoint request{};
        psh::ObjectID obj = -1;
        int opt = -1;
        int opt2 = -1;
        int opt3 = -1;
        int opt4 = -1;
    };
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
    //  이 시각이 지나면 스스로 끊는다. 생성할 때 로그정규분포로 정한다.
    psh::SteadyClock::TimePoint _sessionEnd = psh::SteadyClock::TimePoint::max();
    //  끊기 시작했다. 사망하면 Hit와 Destroy가 연달아 와서 사유를 한 번만 세려고 둔다.
    bool _disconnecting = false;

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
    Logger& _logger;
};


