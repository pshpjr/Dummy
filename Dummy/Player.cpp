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
    containGroup = type;
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
    auto moveBuffer = SendBuffer::Alloc();

    psh::MakeGame_ReqMove(moveBuffer, location);


    //psh::MakeGame_ReqMove(moveBuffer,_accountNo,_destinations[moveIndex]);

    //moveIndex = (++moveIndex)%4;
    Req(psh::eGame_ReqMove);

    toDestinationTime = (_location - location).Size() / speedPerMS;
    isMove = true;
    _server->SendPacket(_sessionId,moveBuffer);
}

void Player::Disconnect()
{
    _server->DisconnectSession(_sessionId);
}

void Player::Attack(char type)
{
    Req(psh::eGame_ReqAttack);
}

void Player::RecvPacket(CRecvBuffer& buffer)
{
    _state = _state->RecvPacket(this, buffer);
    ASSERT_CRASH(buffer.CanPopSize() == 0);
}

void Player::Stop()
{
    isMove = false;
    toDestinationTime = 0;
}

