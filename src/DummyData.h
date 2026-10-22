#pragma once
#include <Types.h>
#include <atomic>
#include <SocketTypes.h>

struct DummyPercent
{
	int loopMs;
	int moveRange;
	int moveOffset;
	//	몬스터가 보일 때 타겟으로 잡을 확률(‰). 시간이 아니라 몬스터가 나타날 때마다 굴린다.
	int target;
};

extern DummyPercent gPermil;

//	행동별 평균 간격(초). 틱마다 loopMs ÷ 간격 확률로 굴린다. 0 이하면 하지 않는다.
struct DummyInterval
{
	double move;
	double chat;
	//	세션 길이(초). 접속할 때마다 로그정규분포로 뽑고, 지나면 스스로 끊는다. median이 0 이하면 끊지 않는다.
	double sessionMedian;
	double sessionMean;
	//	필드 → 다른 필드
	double fieldChange;
	//	필드 → 마을
	double toVillage;
	//	마을 → 필드
	double toField;
};

extern DummyInterval gInterval;

class TextFileReader;
extern TextFileReader* gChatText;

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