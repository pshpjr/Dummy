#include "Player.h"
#include "SendBuffer.h"
#include "PacketGenerated.h"
#include "PlayerState.h"
#include "Server.h"

void Player::Update(int milli)
{
    _state = _state->Update(this, milli);
}

inline static HashMap<psh::ePacketType,psh::ePacketType> expect = 
{
    {psh::eLogin_ReqLogin,psh::ePacketType::eLogin_ResLogin},
    {psh::eLogin_ReqRegister,psh::ePacketType::eLogin_ResRegister},
    {psh::eGame_ReqLogin,psh::ePacketType::eGame_ResLogin},
    {psh::eGame_ReqLevelEnter,psh::ePacketType::eGame_ResLevelEnter},
    {psh::eLogin_ReqRegister,psh::ePacketType::eLogin_ResRegister},
    {psh::eGame_ReqMove,psh::ePacketType::eGame_ResMove},
    {psh::eGame_ReqAttack,psh::ePacketType::eGame_ResAttack}
};

void Player::CheckPacket(psh::ePacketType type)
{
    packet data;
    bool peekResult = _packets.Peek(data);


    ASSERT_CRASH(peekResult,L"NotRequestPacket");
    ASSERT_CRASH(type == expect.at(data.type),L"RecvInvalidPacket");
    _packets.Dequeue();

    _actionDelay += std::chrono::duration_cast<chrono::milliseconds>(std::chrono::steady_clock::now() - data.request).count();
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


void Player::Move(psh::FVector location)
{
    if (_state->GetType() == StateType::levelChange)
        __debugbreak();

    psh::FVector offset = {};
    if (_isMove)
    {
        auto dist = (_dest - _location).Normalize();
        if (isnan(dist.X))
        {
            _location = _dest;
        }
        else
        {
            offset = dist* _moveTime* speedPerMS;
            _location += offset;
        }
        _toDestinationTime = 0;
        _moveTime = 0;
        _isMove = false;
    }

    auto dst = (location - _location).Size();

    if (dst < 30)
    {
        return;
    }
    else 
    {
        auto dir = (location - _location).Normalize();
        _dest = location - dir * 30;
    }

    if (_location == _dest)
    {
        return;
    }

    Req(psh::eGame_ReqMove,location.X,_dest.X);
    

    _toDestinationTime = (_dest - _location).Size() / speedPerMS;
    _isMove = true;

    auto moveBuffer = SendBuffer::Alloc();

    psh::MakeGame_ReqMove(moveBuffer, _dest);
    _server->SendPacket(_sessionId,moveBuffer);


}

void Player::Disconnect()
{
    _server->DisconnectSession(_sessionId);
 
}

void Player::Attack(char type)
{
    auto attack = SendBuffer::Alloc();
    psh::MakeGame_ReqAttack(attack, type);
    Req(psh::eGame_ReqAttack);
    _server->SendPacket(_sessionId, attack);
}

void Player::RecvPacket(CRecvBuffer& buffer)
{
    _state = _state->RecvPacket(this, buffer);
    ASSERT_CRASH(buffer.CanPopSize() == 0, "can pop from recv buffer");
}


void Player::Stop()
{
    _isMove = false;
    _toDestinationTime = 0;
    _moveTime = 0;
    _location = _dest;
}

