#ifndef __COMMON_STRUCT__
#define __COMMON_STRUCT__
#include <Windows.h>
#include <WinSock2.h>
#include "RingBuffer_Lock.h"
#include "Lockfree_ObjectPool.h"
#include "LockfreeQueue.h"
#include "PacketBuffer.h"

enum { SEND, RECV, UPDATE, CONNECT };
enum { eLAN, eNET };
enum { ACCEPT, RECVCOM, SENDCOM, UPDATECOM, PQCS, RECVPOST, SENDPOST, DIS, RELEASE, GQCS};
enum {LANHEADER = 2, NETHEADER = 5};

#define DEBUGNUM	200
#define dfPACKETNUM	200
struct stRELEASE {
	LONG64 IOCount;
	LONG64 ReleaseFlag;
	stRELEASE() {
		IOCount = 0;
		ReleaseFlag = TRUE;
	}
};

struct stOVERLAPPED {
	WSAOVERLAPPED Overlapped;
	INT64 SessionID;
	int Type;
};

struct stDEBUG {
	INT64 SessionID;
	short FuncNum;
	short IOCount;
	DWORD ThreadID;
	SOCKET Sock;
	stDEBUG() {
		Sock = NULL;
		SessionID = -1;
		FuncNum = -1;
		IOCount = -1;
		ThreadID = -1;
	}
};

struct stSESSION {
	SOCKET sock;
	SOCKET closeSock;
	INT64 SessionID;
	INT64 SessionIndex;
	SOCKADDR_IN clientaddr;
	SOCKADDR_IN serveraddr;
	stOVERLAPPED SendOverlapped;
	stOVERLAPPED RecvOverlapped;
	stOVERLAPPED UpdateOverlapped;
	stOVERLAPPED ConnectOverlapped;
	CRingBuffer RecvQ;
	CLockfreeQueue<CPacket*> SendQ;
	volatile LONG SendFlag;
	CPacket* PacketArray[200];
	int PacketCount;
	//여기부턴 debug용
	/*int iSendbyte;
	DWORD iRecvbyte;
	DWORD sendComtime;
	DWORD recvComtime;
	DWORD updateComtime;
	INT64 sendComSessionID;
	INT64 recvComSessionID;
	INT64 updateComSessionID;
	INT64 releaseSessionID;
	INT64 PQCSSessionID;
	INT64 PQCSCnt;
	DWORD recvret;
	DWORD sendret;
	DWORD sendtime;
	DWORD recvtime;
	DWORD sendTh;
	DWORD recvTh;
	DWORD DisTh;
	DWORD ReleaseTh;
	DWORD sendComTh;
	DWORD recvComTh;
	DWORD updateComTh;
	DWORD Distime;
	DWORD DisIO;
	SOCKET sendsock;
	SOCKET recvsock;
	DWORD sendErr;
	DWORD recvErr;
	DWORD recvLen[2];
	DWORD debugCnt;
	stDEBUG debug[DEBUGNUM];*/
	__declspec(align(64))
		LONG64 IOCount;
	LONG64 ReleaseFlag;
};

//ChatServer
struct CPlayer {
public:
	INT64 _SessionID;
	//INT64 _LastAccountNo;
	INT64 _AccountNo;
	WCHAR _ID[20];
	WCHAR _Nickname[20];
	char _SessionKey[64];
	short _SectorX;
	short _SectorY;
	//short _LastSectorX;
	//short _LastSectorY;
	DWORD _dwRecvTime;
	//INT64 _LastSessionID;
	//BYTE _LastMsg;
	//WCHAR _MSG[1024];
	//WORD _MSGLen;

};

struct st_UPDATE_MSG {
	BYTE byType;
	CPacket* pPacket;
	INT64 SessionID;
};

struct st_TOKEN {
	char Token[64];
	DWORD UpdateTime;
};

//LoginServer
struct st_CLIENT {
	BYTE Type;
	stSESSION* pSession;
	INT64 SessionID;
};

struct st_ACCOUNT {
	INT64 SessionID;
	INT64 Parameter;
};

struct st_LOGINSESSION {
	INT64 SessionID;
	INT64 AccountNo;
	char Token[64];

};


#endif
