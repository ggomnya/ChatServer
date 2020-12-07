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
	CLanClient();
	static unsigned int WINAPI WorkerThreadFunc(LPVOID lParam) {
		((CLanClient*)lParam)->WorkerThread(lParam);
		return 0;
	}
	unsigned int WINAPI WorkerThread(LPVOID lParam);
	bool Start(WCHAR* ServerIP, USHORT Port, int NumWorkerthread, int NumIOCP, int iBlockNum = 0, bool bPlacementNew = false);
	bool Connect();
	bool Disconnect(INT64 SessionID);
	bool SendPacket(CPacket* pSendPacket);
	void ReConnect();

	virtual void OnEnterJoinServer(INT64 SessionID) = 0;
	virtual void OnLeaveServer(INT64 SessionID) = 0;
	virtual void OnRecv(INT64 SessionID, CPacket* pRecvPacket) = 0;
	//virtual void OnSend(INT64 SessionID, int SendSize) = 0;
	virtual void OnError(int errorcode, const WCHAR* Err) = 0;

private:
	bool _Disconnect(stSESSION* pSession); 
	void ReleaseSession(stSESSION* pSession);
	void RecvPost(stSESSION* pSession);
	void SendPost(stSESSION* pSession);
	void Release(stSESSION* pSession);
	void RecvComp(stSESSION* pSession, DWORD cbTransferred);
	void SendComp(stSESSION* pSession);

public:
	int _SendTPS;
	int _RecvTPS;
	int _DisCount;
	int _ConnectFail;
	int _ConnectSuccess;
private:
	SRWLOCK srwINDEX;
	stSESSION _ClientSession;
	HANDLE _hcp;
	HANDLE* _hWorkerThread;
	INT64 _SessionIDCnt;
	int _NumThread;
};