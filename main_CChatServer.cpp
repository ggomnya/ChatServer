#include "CChatServer.h"
#include "CLoginClient.h"
#include <conio.h>

CCrashDump _Dump;

LONG CCrashDump::_DumpCount = 0;
SRWLOCK srwTOKEN;
unordered_map<INT64, string> TokenMap;


int wmain() {
	InitializeSRWLock(&srwTOKEN);
	timeBeginPeriod(1);
	CChatServer* server = new CChatServer;
	CLoginClient* client = new CLoginClient;
	server->Start(INADDR_ANY, 6000, 4, 0, 15000, false);
	client->Start(NULL, 7000, 1, 0);
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
			//wprintf(L"[Update Block Count: %lld]\n", server->_BlockCount);
			wprintf(L"[Heartbeat Disconnect: %lld]\n\n", server->_HeartbeatDis);
			//wprintf(L"[NewCount: %d] [DeleteCount: %d]\n\n", server._NewCount, server._DeleteCount);
			server->_AcceptTPS = 0;
			server->_SendTPS = 0;
			server->_RecvTPS = 0;
			server->_MsgTPS = 0;
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