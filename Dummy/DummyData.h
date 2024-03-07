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
};
extern DummyPercent gPermil;

struct ServerData
{
	String ip;
	Port port;
	bool useDB;
	int maxPlayer;
	int playerPerGroup;
};

extern ServerData gData;