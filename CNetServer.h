#pragma once
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "winmm.lib")

#include <process.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <stdio.h>
#include <stack>
//#include <queue>
//#include <list>
#include <locale.h>
#include "PacketBuffer.h"
#include "LockfreeQueue.h"
#include "RingBuffer_Lock.h"
#include "CCrashDump.h"
using namespace std;



class CNetServer {
private:
	enum { SEND, RECV, UPDATE };

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

	struct stSESSION {
		SOCKET sock;
		SOCKET closeSock;
		INT64 SessionID;
		INT64 SessionIndex;
		SOCKADDR_IN clientaddr;
		stOVERLAPPED SendOverlapped;
		stOVERLAPPED RecvOverlapped;
		stOVERLAPPED UpdateOverlapped;
		CRingBuffer RecvQ;
		CLockfreeQueue<CPacket*> SendQ;
		volatile LONG SendFlag;
		CPacket* PacketArray[200];
		int PacketCount;
		//여기부턴 debug용
		int iSendbyte;
		char pack[1024];
		DWORD sendComtime;
		DWORD recvComtime;
		INT64 Dis;
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
		DWORD Distime;
		DWORD DisIO;
		SOCKET tempsock;
		//list<int> rel;
		__declspec(align(16))
			LONG64 IOCount;
		LONG64 ReleaseFlag;
	};



#define HEADER	5
	SRWLOCK srwINDEX;
	stSESSION* _SessionList;
	stack<int> _IndexSession;
	//queue<int> _IndexSession;
	INT64 _SessionIDCnt;
	SOCKET _Listen_sock;
	HANDLE _hcp;
	HANDLE* _hWorkerThread;
	HANDLE _hAcceptThread;

	int _MaxSession;
	LONG _SessionCnt;
	int _NumThread;
public:
	int _AcceptCount;
	int _AcceptTPS;
	int _SendTPS;
	int _RecvTPS;
	int _DisCount;

	CNetServer();
	static unsigned int WINAPI WorkerThreadFunc(LPVOID lParam) {
		((CNetServer*)lParam)->WorkerThread(lParam);
		return 0;
	}
	static unsigned int WINAPI AcceptThreadFunc(LPVOID lParam) {
		((CNetServer*)lParam)->AcceptThread(lParam);
		return 0;
	}
	unsigned int WINAPI WorkerThread(LPVOID lParam);

	unsigned int WINAPI AcceptThread(LPVOID lParam);

	bool Start(ULONG OpenIP, USHORT Port, int NumWorkerthread, int NumIOCP, int MaxSession, 
		int iBlockNum = 100, bool bPlacementNew = false, bool Restart = false);
	
	void Stop();
	int GetClientCount();
	bool _Disconnect(stSESSION* pSession);
	bool Disconnect(INT64 SessionID);
	bool _SendPacket(stSESSION* pSession, CPacket* pSendPacket);
	bool SendPacket(INT64 SessionID, CPacket* pSendPacket);
	stSESSION* FindSession(INT64 SessionID);
	void ReleaseSession(stSESSION* pSession);

	void RecvPost(stSESSION* pSession);
	void SendPost(stSESSION* pSession);
	void Release(stSESSION* pSession);


	

	virtual void OnClientJoin(SOCKADDR_IN clientaddr, INT64 SessionID) = 0;
	virtual void OnClientLeave(INT64 SessionID) = 0;

	virtual bool OnConnectRequest(SOCKADDR_IN clientaddr) = 0;

	virtual void OnRecv(INT64 SessionID, CPacket* pRecvPacket) = 0;
	//virtual void OnSend(INT64 SessionID, int SendSize) = 0;

	virtual void OnError(int errorcode, const WCHAR* Err) = 0;

};