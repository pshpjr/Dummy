#pragma once
#include <Types.h>
struct DummyPercent
{
	int loopMs;
	int move;
	int field;
	int moveRange;
	int moveOffset;
	int disconnect;
	int target;

};
extern DummyPercent gPermil;

struct ServerData
{
	String ip;
	Port port;
	bool useDB;
	int maxPlayer;
	int playerPerGroup;
	int reconnect;
};

extern ServerData gData;