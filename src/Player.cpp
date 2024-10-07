#include "Player.h"
#include "SendBuffer.h"
#include "PacketGenerated.h"
#include "PlayerState.h"
#include "Server.h"
#include "TextFileReader.h"

void Player::Update(int milli)
{
    if (_moveDelay > 0)
    {
        _moveDelay -= milli;
    }

    if (needUpdate)
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


void Player::Move(psh::FVector location, moveReason reason)
{
   

    if (_state->GetType() == StateType::levelChange)
        __debugbreak();

    psh::FVector offset = {};
    if (_isMove)
    {
        offset = _dir * _moveTime * speedPerMS;
        _location += offset;

        _toDestinationTime = 0;
        _moveTime = 0;
        _isMove = false;
    }

    if (location == _location)
        return;


    auto dir = (location - _location).Normalize();
    auto newDestination = location - dir * 40;
    _attackDir = dir;

    if (_moveDelay > 0)
    {
        return;
    }

    if ((newDestination - _location).Size()  > gPermil.moveOffset*2)
    {
        _target = -1;
        return;
    }
    if ((newDestination - _location).Size() < 5)
    {
        return;
    }
    if ((newDestination - _dest).Size() < 5)
    {
        return;
    }


    moveCount++;

    _moveDelay += 500;
    Req(psh::eGame_ReqMove,location.X,_dest.X);

    _isMove = true;
    _dest = newDestination;

    _dir = dir;

    _toDestinationTime = (_dest - _location).Size() / speedPerMS;
    writeMoveDebug(reason);
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
    psh::MakeGame_ReqAttack(attack, type,_attackDir);
    Req(psh::eGame_ReqAttack,_target,static_cast<int>( _containGroup));
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

