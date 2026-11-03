#include "Player.h"
#include "Metrics.h"
#include "SendBuffer.h"
#include "PacketGenerated.h"
#include "PlayerState.h"
#include "Server.h"
#include "TextFileReader.h"

#include <Logger.h>

static const HashMap<psh::ePacketType,psh::ePacketType> expect =
{
    {psh::eLogin_ReqLogin,psh::ePacketType::eLogin_ResLogin},
    {psh::eLogin_ReqRegister,psh::ePacketType::eLogin_ResRegister},
    {psh::eGame_ReqLogin,psh::ePacketType::eGame_ResLogin},
    {psh::eGame_ReqLevelEnter,psh::ePacketType::eGame_ResLevelEnter},
    {psh::eGame_ReqMove,psh::ePacketType::eGame_ResMove},
    {psh::eGame_ReqAttack,psh::ePacketType::eGame_ResAttack}
};

//  타겟과 이만큼 떨어져 멈춘 뒤 공격한다. 가장 짧은 스킬 사거리(30)보다 짧아야 맞는다.
static constexpr float ATTACK_DISTANCE = 25;

Player::Player(SessionID id, psh::AccountNo accountNo, IOCP* server, Logger& logger)
    : _accountNo(accountNo),
      _containGroup(psh::ServerType::End),
      _id(std::format(L"ID_{:d}", accountNo)),
      _password(std::format(L"PASS_{:d}", accountNo)),
      _sessionId(id),
      _server(server),
      _state{psh::DisconnectStateRefector::Get()},
      _logger(logger)
{
    if (gInterval.sessionMedian > 0)
    {
        const auto seconds = psh::RandomUtil::LogNormal(gInterval.sessionMedian, gInterval.sessionMean);
        _sessionEnd = psh::SteadyClock::Now() + std::chrono::milliseconds(static_cast<int64_t>(seconds * 1000));
    }
}


void Player::Update(int milli)
{
    if (_moveDelay > 0) {
        _moveDelay -= milli;
        if (_moveDelay < 0) _moveDelay = 0;
    }

    if (_attackCooldown > 0)
    {
        _attackCooldown -= milli;
        if (_attackCooldown < 0) _attackCooldown = 0;
    }

    UpdateLocation(milli);

    if (needUpdate)
        _state = _state->Update(this, milli);
}

void Player::UpdateLocation(int time)
{
    if(_isMove)
    {
        _moveTime +=time;

        if(_toDestinationTime<=_moveTime)
        {
            Stop(_destination);
        }
    }
}

void Player::CalculateLocation()
{
    if (_isMove)
    {
        psh::FVector delta = _moveDir * _moveTime * SPEED_PER_MS;
        Stop(_location + delta);
    }
}

void Player::SetTarget(psh::ObjectID id, psh::FVector location, int range)
{
    _target = id;
    auto dir = (location - _location).Normalize();
    float dist = Distance(_location, location);
    if(dist > range * 2)
    {
        _target = -1;
        return;
    }
    //가까운 곳
    if(dist <= ATTACK_DISTANCE)
    {
        if(!(std::isnan(dir.X) || std::isnan(dir.Y)))
        {
            _attackDir = dir;
        }

        Stop(_location);
        return;
    }


    //멀리 있으면 위치로
    auto moveDst = location - (dir * ATTACK_DISTANCE);
    Move(moveDst);
}

void Player::Move(psh::FVector destination, moveReason reason)
{
    if (_state->GetType() == psh::StateType::levelChange)
        psh::util::DebugBreak();

    CalculateLocation();

    auto direction = (destination - _location).Normalize();

    if (std::isnan(direction.X) || std::isnan(direction.Y)) return;

    _attackDir = direction;

    if (_moveDelay > 0)
    {
        return;
    }

    float distance = (destination - _location).Size();

    moveCount++;

    _moveDelay += 500;
    Req(psh::eGame_ReqMove,destination.X,_destination.X);

    _isMove = true;
    _destination = destination;
    _moveDir = direction;
    _toDestinationTime = distance/ SPEED_PER_MS;

    auto moveBuffer = SendBuffer::Alloc();
    MakeGame_ReqMove(moveBuffer, _destination);
    _server->SendPacket(_sessionId,moveBuffer);
}

namespace
{
    //  요청 종류마다 시리즈가 하나고, 모든 더미가 같이 쓴다.
    struct RequestMetrics
    {
        //  요청부터 응답까지 걸린 시간
        psh::metrics::Histogram delay;
        psh::metrics::Counter sent;
        //  보냈지만 아직 응답을 받지 못한 요청 수
        psh::metrics::Gauge pending;
    };

    const RequestMetrics& GetRequestMetrics(const psh::ePacketType request)
    {
        static const auto metrics = []
        {
            using psh::metrics::Registry;
            HashMap<psh::ePacketType, RequestMetrics> result;
            const std::pair<psh::ePacketType, const char*> types[] = {
                {psh::eLogin_ReqLogin, "login"},
                {psh::eLogin_ReqRegister, "register"},
                {psh::eGame_ReqLogin, "game_login"},
                {psh::eGame_ReqLevelEnter, "level_enter"},
                {psh::eGame_ReqMove, "move"},
                {psh::eGame_ReqAttack, "attack"},
            };
            for (const auto& [type, name] : types)
            {
                const psh::metrics::Attributes attributes{{"type", name}};
                result.emplace(type, RequestMetrics{
                    Registry::CreateTimeHistogram("dummy.action.delay", "time from request to its response", attributes),
                    Registry::CreateCounter("dummy.requests", "sent requests that expect a response", attributes),
                    Registry::CreateGauge("dummy.requests.pending", "requests waiting for a response", attributes),
                });
            }
            return result;
        }();
        return metrics.at(request);
    }
}

namespace
{
    //  이만큼 행동하지 않으면 멈춘 것으로 보고 플레이어 상태를 로그로 남긴다.
    constexpr psh::int64 STUCK_LOG_MS = 30'000;

    const char* ServerName(const psh::ServerType type)
    {
        switch (type)
        {
        case psh::ServerType::Village: return "village";
        case psh::ServerType::Easy: return "easy";
        case psh::ServerType::Hard: return "hard";
        case psh::ServerType::Pvp: return "pvp";
        default: return "none";
        }
    }

    const char* PacketName(const psh::ePacketType type)
    {
        switch (type)
        {
        case psh::eLogin_ReqLogin: return "req_login";
        case psh::eLogin_ResLogin: return "res_login";
        case psh::eGame_ReqLogin: return "req_game_login";
        case psh::eGame_ResLogin: return "res_game_login";
        case psh::eGame_ReqLevelEnter: return "req_level_enter";
        case psh::eGame_ResLevelEnter: return "res_level_enter";
        case psh::eGame_ReqMove: return "req_move";
        case psh::eGame_ResMove: return "res_move";
        case psh::eGame_ResMoveStop: return "res_move_stop";
        case psh::eGame_ReqAttack: return "req_attack";
        case psh::eGame_ResAttack: return "res_attack";
        case psh::eGame_ResCreateActor: return "res_create_actor";
        case psh::eGame_ResDestroyActor: return "res_destroy_actor";
        case psh::eGame_ResChracterDetail: return "res_character_detail";
        case psh::eGame_ResPlayerDetail: return "res_player_detail";
        case psh::eGame_ResGetCoin: return "res_get_coin";
        case psh::eGame_ResHit: return "res_hit";
        case psh::eGame_ResChat: return "res_chat";
        case psh::ePacketType::None: return "none";
        default: return "other";
        }
    }

}

const char* Player::SleepName(const SleepReason reason)
{
    switch (reason)
    {
    case SleepReason::Acting: return "acting";
    case SleepReason::WaitCreate: return "wait_create";
    case SleepReason::Dead: return "dead";
    case SleepReason::WaitResponse: return "wait_response";
    case SleepReason::Disconnecting: return "disconnecting";
    default: return "unknown";
    }
}

Player::SleepReason Player::Sleep() const
{
    return _state->Sleep(this);
}

void Player::OnRecv(const psh::ePacketType type)
{
    _lastRecvType = type;
    _lastRecv = psh::SteadyClock::Now();
}

psh::int64 Player::TrackSleep(const psh::SteadyClock::TimePoint now)
{
    const auto reason = Sleep();
    if (reason != _sleepReason)
    {
        _sleepReason = reason;
        _sleepSince = now;
        _stuckLogged = false;
    }
    if (reason == SleepReason::Acting)
        return 0;

    const auto sleepMs = psh::SteadyClock::ToMs(now - _sleepSince);
    if (sleepMs >= STUCK_LOG_MS && !_stuckLogged)
    {
        _stuckLogged = true;

        packet pending{};
        const bool hasPending = _packets.Peek(pending);
        _logger.Write(L"DummyStuck", Logger::LogLevel::Error,
                      L"AccountNO : %lld, state : %s, reason : %s, sleep : %lld ms, hp : %d, hpKnown : %d, needUpdate : %d, "
                      L"group : %s, me : %lld, target : %lld, connected : %lld ms, sessionOver : %d, "
                      L"lastRecv : %s %lld ms ago, pending : %s %lld ms",
                      _accountNo, _state->Name(), SleepName(reason), sleepMs, _hp, _hpKnown, needUpdate,
                      ServerName(_containGroup), _me, _target, psh::SteadyClock::ToMs(now - _connectedAt),
                      now >= _sessionEnd, PacketName(_lastRecvType), psh::SteadyClock::ToMs(now - _lastRecv),
                      hasPending ? PacketName(pending.type) : "none",
                      hasPending ? psh::SteadyClock::ToMs(now - pending.request) : 0);
    }
    return sleepMs;
}

Player::~Player()
{
    packet data;
    while (_packets.Peek(data))
    {
        GetRequestMetrics(data.type).pending.Add(-1);
        _packets.Dequeue();
    }
}

void Player::Req(psh::ePacketType type, int opt, int opt2, int opt3, int opt4)
{
    auto now = psh::SteadyClock::Now();
    auto result = _packets.Enqueue({type,now,_me,opt,opt2,opt3,opt4});

    if(result == false)
    {
        psh::util::DebugBreak();
    }

    ASSERT_CRASH(result, "");

    const auto& metrics = GetRequestMetrics(type);
    metrics.sent.Add();
    metrics.pending.Add(1);
}

void Player::CheckPacket(psh::ePacketType type)
{
    packet data;
    bool peekResult = _packets.Peek(data);

    ASSERT_CRASH(peekResult,L"NotRequestPacket");
    ASSERT_CRASH(type == expect.at(data.type),L"RecvInvalidPacket");
    _packets.Dequeue();

    const auto& metrics = GetRequestMetrics(data.type);
    metrics.pending.Add(-1);
    metrics.delay.Record(
        psh::SteadyClock::ElapsedUs(data.request));
}


void Player::LoginLogin()
{
    auto loginBuffer = SendBuffer::Alloc();
    
    psh::MakeLogin_ReqLogin(loginBuffer,_id ,_password);
    
    Req(psh::eLogin_ReqLogin);
    _server->SendPacket(_sessionId,loginBuffer);
}


void Player::GameLogin()
{
    auto gameLogin = SendBuffer::Alloc();
    psh::MakeGame_ReqLogin(gameLogin,_accountNo,_key);
    Req(psh::eGame_ReqLogin);

    _server->SendPacket(_sessionId,gameLogin);
}

void Player::ReqLevelChange(psh::ServerType type)
{
    needUpdate = false;
    auto reqLevelChange = SendBuffer::Alloc();
    psh::MakeGame_ReqLevelEnter(reqLevelChange,_accountNo,type);
    if (type == psh::ServerType::End)
        psh::util::DebugBreak();

    _containGroup = type;

    Req(psh::eGame_ReqLevelEnter);
    _server->SendPacket(_sessionId,reqLevelChange);
}

void Player::LevelChangeComp()
{
    auto levelChange = SendBuffer::Alloc();
    psh::MakeGame_ReqChangeComplete(levelChange,_accountNo);
    _server->SendPacket(_sessionId,levelChange);
}

void Player::Chat()
{
    auto chatPacket = SendBuffer::Alloc();
    psh::MakeGame_ReqChat(chatPacket, gChatText->GetRandString());
    _server->SendPacket(_sessionId, chatPacket);
}


void Player::Disconnect(const DisconnectReason reason)
{
    needUpdate = false;
    if (_disconnecting)
        return;

    _disconnecting = true;
    _server->DisconnectSession(_sessionId);
}

void Player::Attack(char type)
{
    auto attack = SendBuffer::Alloc();
    psh::MakeGame_ReqAttack(attack, type,_attackDir);
    Req(psh::eGame_ReqAttack,_target,static_cast<int>( _containGroup),type);
    _server->SendPacket(_sessionId, attack);
}

void Player::RecvPacket(RecvBuffer& buffer)
{
    while (buffer.CanPopSize() > 0)
    {
        _state = _state->RecvPacket(this, buffer);
    }
    ASSERT_CRASH(buffer.CanPopSize() == 0, "can pop from recv buffer");
}

void Player::Stop(psh::FVector newLocation)
{
    _isMove = false;
    _toDestinationTime = 0;
    _moveTime = 0;
    _location = newLocation;
}

