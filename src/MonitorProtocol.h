// ReSharper disable CppInconsistentNaming
// ReSharper disable IdentifierTypo
// ReSharper disable CppTabsAreDisallowed
#pragma once
#include "Types.h"

// 섹터 50 x 50
enum en_Server_TYPE : BYTE
{
    en_SERVER_MONITOR, en_SERVER_CHAT, en_SERVER_LOGIN, en_SERVER_GAME,
};

enum en_PACKET_TYPE : WORD
{
    ////////////////////////////////////////////////////////
    //
    //   MonitorServer & MoniterTool Protocol / 응답을 받지 않음.
    //
    ////////////////////////////////////////////////////////

    //------------------------------------------------------
    // Monitor Server  Protocol
    //------------------------------------------------------
    en_PACKET_SS_MONITOR = 20000,
    //------------------------------------------------------
    // Server -> Monitor Protocol
    //------------------------------------------------------
    //------------------------------------------------------------
    // LoginServer, GameServer , ChatServer  가 모니터링 서버에 로그인 함
    //
    //
    //	{
    //		BYTE	Type
    //		WORD   groups			// 서버 군. 지금은 1번만 사용
    //		BYTE   ServerNo		//  각 서버마다 고유 번호를 부여하여 사용
    //	}
    //
    //------------------------------------------------------------
    en_PACKET_SS_MONITOR_LOGIN,

    //------------------------------------------------------------
    // 서버가 모니터링서버로 데이터 전송
    // 각 서버는 자신이 모니터링중인 수치를 1초마다 모니터링 서버로 전송.
    //
    // 서버의 다운 및 기타 이유로 모니터링 데이터가 전달되지 못할떄를 대비하여 TimeStamp 를 전달한다.
    // 이는 모니터링 클라이언트에서 계산,비교 사용한다.
    //
    //	{
    //		WORD	Type
    //		WORD	GROUP
    //		BYTE	serverType
    //		BYTE	DataType				// 모니터링 데이터 Type 하단 Define 됨.
    //		int		DataValue				// 해당 데이터 수치.
    //		int		TimeStamp				// 해당 데이터를 얻은 시간 TIMESTAMP  (time() 함수)
    //										// 본래 time 함수는 time_t 타입변수이나 64bit 로 낭비스러우니
    //										// int 로 캐스팅하여 전송. 그래서 2038년 까지만 사용가능
    //	}
    //
    //------------------------------------------------------------
    en_PACKET_SS_MONITOR_DATA_UPDATE, en_PACKET_CS_MONITOR = 25000,
    //------------------------------------------------------
    // Monitor -> Monitor Tool Protocol  (Client <-> Server 프로토콜)
    //------------------------------------------------------
    //------------------------------------------------------------
    // 모니터링 클라이언트(툴) 이 모니터링 서버로 로그인 요청
    //
    //	{
    //		WORD	Type
    //
    //		char	LoginSessionKey[32]		// 로그인 인증 키. (이는 모니터링 서버에 고정값으로 보유)
    //										// 각 모니터링 툴은 같은 키를 가지고 들어와야 함
    //	}
    //
    //------------------------------------------------------------
    en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN,

    //------------------------------------------------------------
    // 모니터링 클라이언트(툴) 모니터링 서버로 로그인 응답
    //
    //	{
    //		WORD	Type
    //
    //		BYTE	Status					// 로그인 결과 0 / 1 / 2 ... 하단 Define
    //	}
    //
    //------------------------------------------------------------
    en_PACKET_CS_MONITOR_TOOL_RES_LOGIN,

    //------------------------------------------------------------
    // 모니터링 서버가 모니터링 클라이언트(툴) 에게 모니터링 데이터 전송
    //
    // 통합 모니터링 방식을 사용 중이므로, 모니터링 서버는 모든 모니터링 클라이언트에게
    // 수집되는 모든 데이터를 바로 전송시켜 준다.
    //
    //
    // 데이터를 절약하기 위해서는 초단위로 모든 데이터를 묶어서 30~40개의 모니터링 데이터를 하나의 패킷으로 만드는게
    // 좋으나  여러가지 생각할 문제가 많으므로 그냥 각각의 모니터링 데이터를 개별적으로 전송처리 한다.
    //
    //	{
    //		WORD	Type
    //
    //		BYTE	ServerNo				// 서버 No
    //		BYTE	DataType				// 모니터링 데이터 Type 하단 Define 됨.
    //		int		DataValue				// 해당 데이터 수치.
    //		int		TimeStamp				// 해당 데이터를 얻은 시간 TIMESTAMP  (time() 함수)
    //										// 본래 time 함수는 time_t 타입변수이나 64bit 로 낭비스러우니
    //										// int 로 캐스팅하여 전송. 그래서 2038년 까지만 사용가능
    //	}
    //
    //------------------------------------------------------------
    en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE,
};

enum en_PACKET_SS_MONITOR_DATA_UPDATE_TYPE : BYTE
{
    dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL, // 서버컴퓨터 CPU 전체 사용률
    dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY, // 서버컴퓨터 논페이지 메모리 MByte
    dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV, // 서버컴퓨터 네트워크 수신량 KByte
    dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND, // 서버컴퓨터 네트워크 송신량 KByte
    dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY, // 서버컴퓨터 사용가능 메모리

    //프로세스별
    dfMONITOR_DATA_TYPE_GAME_SERVER_CPU, // GameServer CPU 사용률
    dfMONITOR_DATA_TYPE_GAME_SERVER_MEM, // GameServer 메모리 사용 MByte
    dfMONITOR_DATA_TYPE_GAME_SERVER_ACCEPT_TPS, // GameServer ACCEPT TPS
    dfMONITOR_DATA_TYPE_GAME_SERVER_SEND_TPS, // GameServer SEND TPS
    dfMONITOR_DATA_TYPE_GAME_SERVER_RECV_TPS, // GameServer RECV TPS
    dfMONITOR_DATA_TYPE_GAME_SERVER_DISCONNECT_TPS, // GameServer Disconnect tps
    dfMONITOR_DATA_TYPE_GAME_SERVER_SESSIONS, // sessions
    //그룹별
    dfMONITOR_DATA_TYPE_FIELD_SERVER_RUN, // 그룹 실행 여부 ON / OFF
    dfMONITOR_DATA_TYPE_FIELD_WORK_TIME, // 그룹 초당 일한 시간
    dfMONITOR_DATA_TYPE_FIELD_MAX_WORK, //그룹 최대 프레임 시간
    dfMONITOR_DATA_TYPE_FIELD_JOB_QUEUE_SIZE, // 그룹 잡큐 사이즈
    dfMONITOR_DATA_TYPE_FIELD_JOB_TPS, // 그룹 잡 초당 처리 횟수
    dfMONITOR_DATA_TYPE_FIELD_PLAYERS, // 그룹 플레이어 수
    dfMONITOR_DATA_TYPE_FIELD_ENTER_TPS, // 그룹 ENTER 초당 횟수
    dfMONITOR_DATA_TYPE_FIELD_LEAVE_TPS, // 그룹 LEAVE 초당 횟수
    dfMONITOR_DATA_TYPE_FIELD_DB_TPS, // 그룹 DB 초당 처리 횟수
    dfMONITOR_DATA_TYPE_FIELD_DB_QUEUE_SIZE, // 그룹 DB 큐 사이즈
    dfMONITOR_DATA_TYPE_FIELD_DB_QUERY_AVG, // 그룹 DB 쿼리 평균
    dfMONITOR_DATA_TYPE_FIELD_DB_ERR, //DB 에러


    dfMONITOR_DATA_TYPE_GAME_CIRCLE_POOL, //circle pool
    dfMONITOR_DATA_TYPE_GAME_SQUARE_POOL, //square pool

    dfMONITOR_DATA_TYPE_GAME_CHAT_COUNT, //채팅 패킷
    dfMONITOR_DATA_TYPE_GAME_MOVE_COUNT, //이동 패킷
    dfMONITOR_DATA_TYPE_GAME_ATTACK_COUNT, //공격 패킷
    dfMONITOR_DATA_TYPE_GAME_DUMMY_DELAY_AVG, //더미 딜레이
    dfMONITOR_DATA_TYPE_GAME_DUMMY_DELAY_MAX, //더미 최대 딜레이
    dfMONITOR_END
};

enum en_PACKET_CS_MONITOR_TOOL_RES_LOGIN
{
    dfMONITOR_TOOL_LOGIN_OK = 1, // 로그인 성공

    dfMONITOR_TOOL_LOGIN_ERR_NOSERVER = 2, // 서버이름 오류 (매칭미스)
    dfMONITOR_TOOL_LOGIN_ERR_SESSIONKEY = 3, // 로그인 세션키 오류
};
