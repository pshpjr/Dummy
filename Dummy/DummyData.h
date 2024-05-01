#pragma once
#include <Types.h>
#include <atomic>
struct DummyPercent
{
	int loopMs;
	int move;
	int toField;
	int toVillage;
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
	int startAccount;
	int reconnect;
};

extern ServerData gData;

struct DummyDelay
{
	std::atomic<int> delay;
	std::atomic<int> count;
	std::atomic<int> max;
};

extern DummyDelay gDelay;