#include "Player.h"
#include "SendBuffer.h"
#include "PacketGenerated.h"
#include "PlayerState.h"
#include "Server.h"
#include "TextFileReader.h"

static const HashMap<psh::ePacketType,psh::ePacketType> expect =
{
    {psh::eLogin_ReqLogin,psh::ePacketType::eLogin_ResLogin},
    {psh::eLogin_ReqRegister,psh::ePacketType::eLogin_ResRegister},
    {psh::eGame_ReqLogin,psh::ePacketType::eGame_ResLogin},
    {psh::eGame_ReqLevelEnter,psh::ePacketType::eGame_ResLevelEnter},
    {psh::eGame_ReqMove,psh::ePacketType::eGame_ResMove},
    {psh::eGame_ReqAttack,psh::ePacketType::eGame_ResAttack}
};

Player::Player(SessionID id, psh::AccountNo accountNo, IOCP* server, CLogger& logger)
    : _accountNo(accountNo),
      _containGroup(psh::ServerType::End),
      _id(std::format(L"ID_{:d}", accountNo)),
      _password(std::format(L"PASS_{:d}", accountNo)),
      _sessionId(id),
      _server(server),
      _state{psh::DisconnectStateRefector::Get()},
      _logger(logger) {}

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

void Player::SetTarget(psh::ObjectID id, psh::FVector location)
{
    _target = id;
    auto dir = (location - _location).Normalize();

    //가까운 곳
    if(Distance(_location, location) <=35)
    {
        if(!(isnan(dir.X) || isnan(dir.Y)))
        {
            _attackDir = dir;
        }

        Stop(_location);
        return;
    }

    //멀리 있으면 위치로
    auto moveDst = location - (dir*35);
    Move(moveDst);
}

void Player::Move(psh::FVector destination, moveReason reason)
{
    if (_state->GetType() == psh::StateType::levelChange)
        __debugbreak();

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

void Player::CheckPacket(psh::ePacketType type)
{
    packet data;
    bool peekResult = _packets.Peek(data);

    ASSERT_CRASH(peekResult,L"NotRequestPacket");
    ASSERT_CRASH(type == expect.at(data.type),L"RecvInvalidPacket");
    _packets.Dequeue();

    _actionDelay += std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - data.request).count();
}

unsigned int Player::GetActionDelay()
{
    unsigned int delay = _actionDelay;
    _actionDelay = 0;
    return delay;
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
        DebugBreak();

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


void Player::Disconnect()
{
    needUpdate = false;
    _server->DisconnectSession(_sessionId);
}

void Player::Attack(char type)
{
    auto attack = SendBuffer::Alloc();
    psh::MakeGame_ReqAttack(attack, type,_attackDir);
    Req(psh::eGame_ReqAttack,_target,static_cast<int>( _containGroup),type);
    _server->SendPacket(_sessionId, attack);
}

void Player::RecvPacket(CRecvBuffer& buffer)
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

