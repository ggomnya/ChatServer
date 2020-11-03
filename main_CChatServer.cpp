#include "CChatServer.h"
#include <conio.h>
#include "textparser.h"

CCrashDump _Dump;

LONG CCrashDump::_DumpCount = 0;

int wmain() {
	timeBeginPeriod(1);
	//Config 세팅
	WCHAR ServerIP[16];
	USHORT ServerPort;
	DWORD ServerThreadNum;
	DWORD ServerIOCPNum;
	DWORD ServerMaxSession;

	WCHAR ClientIP[16];
	USHORT ClientPort;
	DWORD ClientThreadNum;
	DWORD ClientIOCPNum;
	CINIParse Parse;
	Parse.LoadFile(L"ChatServer_Config.ini");
	Parse.GetValue(L"IP", ServerIP);
	Parse.GetValue(L"PORT", (DWORD*)&ServerPort);
	Parse.GetValue(L"THREAD_NUMBER", &ServerThreadNum);
	Parse.GetValue(L"IOCP_NUMBER", &ServerIOCPNum);
	Parse.GetValue(L"MAX_SESSION", &ServerMaxSession);

	Parse.GetValue(L"IP", ClientIP);
	Parse.GetValue(L"PORT", (DWORD*)&ClientPort);
	Parse.GetValue(L"THREAD_NUMBER", &ClientThreadNum);
	Parse.GetValue(L"IOCP_NUMBER", &ClientIOCPNum);


	CChatServer* server = new CChatServer;

	server->Start(INADDR_ANY, ServerPort, ServerThreadNum, ServerIOCPNum, ServerMaxSession, false);
	server->_LoginClient.Start(ClientIP, ClientPort, ClientThreadNum, ClientIOCPNum);
	DWORD curTime = timeGetTime();
	while (1) {
		//1초마다 List 현황 출력
		if (timeGetTime() - curTime >= 1000) {
			wprintf(L"[Session List Size: %d]\n", server->GetClientCount());
			wprintf(L"[Accept Count: %d]\n", server->_AcceptCount);
			wprintf(L"[AcceptTPS: %d]\n", server->_AcceptTPS);
			wprintf(L"[Disconnect Count: %d]\n", server->_DisCount);
			wprintf(L"[Packet Alloc Count: %lld]\n", CPacket::GetAllocCount());
			wprintf(L"[Send TPS: %d]\n", server->_SendTPS);
			wprintf(L"[Recv TPS: %d]\n", server->_RecvTPS);
			wprintf(L"[PlayerPool Alloc Count: %lld]\n", server->GetPlayerPoolCount());
			wprintf(L"[UpdateMsgPool Alloc Count: %lld]\n", server->GetUpdateMsgPoolCount());
			wprintf(L"[Player Count: %lld]\n", server->GetPlayerCount());
			wprintf(L"[MsgQueue Size: %lld]\n", server->GetMsgQueueCount());
			wprintf(L"[Msg TPS: %lld]\n", server->_MsgTPS);
			wprintf(L"[SessionNotFound: %lld]\n", server->_SessionNotFound);
			wprintf(L"[SessionMiss: %lld]\n", server->_SessionMiss);
			wprintf(L"[Heartbeat Disconnect: %lld]\n", server->_HeartbeatDis);
			wprintf(L"=========================================\n");
			wprintf(L"[TokenCount: %lld]\n", server->_LoginClient.GetTokenCount());
			wprintf(L"[Lan Send TPS: %d]\n", server->_LoginClient._SendTPS);
			wprintf(L"[Lan Recv TPS: %d]\n", server->_LoginClient._RecvTPS);
			wprintf(L"[Lan Connnect Sucess Count: %d]\n", server->_LoginClient._ConnectSuccess);
			wprintf(L"[Lan Connect Fail Count: %d]\n", server->_LoginClient._ConnectFail);
			wprintf(L"[Lan Disconnect Count: %d]\n\n", server->_LoginClient._DisCount);
			//wprintf(L"[Update Block Count: %lld]\n", server->_BlockCount);

			//wprintf(L"[NewCount: %d] [DeleteCount: %d]\n\n", server._NewCount, server._DeleteCount);
			server->_AcceptTPS = 0;
			server->_SendTPS = 0;
			server->_RecvTPS = 0;
			server->_MsgTPS = 0;
			server->_LoginClient._RecvTPS = 0;
			server->_LoginClient._SendTPS = 0;
			curTime = timeGetTime();
		}
		if (_kbhit()) {
			WCHAR cmd = _getch();
			if (cmd == L'x' || cmd == L'X') {
				server->Stop();
			}
			if (cmd == L's' || cmd == L'S') {
				server->Start(INADDR_ANY, 6000, 4, 0, 15000, true);
			}
			if (cmd == L'q' || cmd == L'Q')
				CCrashDump::Crash();
		}
		Sleep(10);
	}
}