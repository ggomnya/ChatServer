#pragma once
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "winmm.lib")

#include <process.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <stdio.h>
#include <stack>
#include <locale.h>
#include "PacketBuffer.h"
#include "LockfreeQueue.h"
#include "RingBuffer_Lock.h"
#include "CCrashDump.h"
#include "CommonStruct.h"
using namespace std;





class CLanClient {
public:

	/*enum { SEND, RECV, CONNECT };
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
		int Type;
	};

	struct stSESSION {
		SOCKET sock;
		SOCKET closeSock;
		INT64 SessionID;
		INT64 SessionIndex;
		SOCKADDR_IN serveraddr;
		stOVERLAPPED SendOverlapped;
		stOVERLAPPED RecvOverlapped;
		stOVERLAPPED ConnectOverlapped;
		CRingBuffer RecvQ;
		CLockfreeQueue<CPacket*> SendQ;
		volatile LONG SendFlag;
		CPacket* PacketArray[200];
		int PacketCount;
		__declspec(align(16))
			LONG64 IOCount;
		LONG64 ReleaseFlag;
	};*/

private:
#define HEADER	2
	SRWLOCK srwINDEX;
	stSESSION _ClientSession;
	HANDLE _hcp;
	HANDLE* _hWorkerThread;
	INT64 _SessionIDCnt;
	int _NumThread;
public:
	int _SendTPS;
	int _RecvTPS;
	int _DisCount;
	int _ConnectFail;
	int _ConnectSuccess;

	CLanClient();
	static unsigned int WINAPI WorkerThreadFunc(LPVOID lParam) {
		((CLanClient*)lParam)->WorkerThread(lParam);
		return 0;
	}
	unsigned int WINAPI WorkerThread(LPVOID lParam);
	bool Start(WCHAR* ServerIP, USHORT Port, int NumWorkerthread, int NumIOCP, int iBlockNum = 0, bool bPlacementNew = false);
	bool Connect();

	bool _Disconnect(stSESSION* pSession);
	bool Disconnect(INT64 SessionID);
	bool SendPacket(CPacket* pSendPacket);
	void ReleaseSession(stSESSION* pSession);
	void ReConnect();
	void RecvPost(stSESSION* pSession);
	void SendPost(stSESSION* pSession);
	void Release(stSESSION* pSession);


	
	virtual void OnEnterJoinServer(INT64 SessionID) = 0;
	virtual void OnLeaveServer(INT64 SessionID) = 0;

	virtual void OnRecv(INT64 SessionID, CPacket* pRecvPacket) = 0;
	//virtual void OnSend(INT64 SessionID, int SendSize) = 0;

	virtual void OnError(int errorcode, const WCHAR* Err) = 0;

};