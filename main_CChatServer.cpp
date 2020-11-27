#include "CChatServer.h"
#include "CMonitoringLanClient.h"
#include <conio.h>
#include "textparser.h"
#include "CProcessUsage.h"
#include <time.h>

CCrashDump _Dump;

LONG CCrashDump::_DumpCount = 0;

WCHAR ServerIP[16];
USHORT ServerPort;
DWORD ServerThreadNum;
DWORD ServerIOCPNum;
DWORD ServerMaxSession;

WCHAR ClientIP[16];
USHORT ClientPort;
DWORD ClientThreadNum;
DWORD ClientIOCPNum;

WCHAR monitorClientIP[16];
USHORT monitorClientPort;
DWORD monitorClientThreadNum;
DWORD monitorClientIOCPNum;


INT64 PacketNum;

int wmain() {
	timeBeginPeriod(1);
	//Config ����
	CProcessUsage ProcessUsage;
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

	Parse.GetValue(L"IP", monitorClientIP);
	Parse.GetValue(L"PORT", (DWORD*)&monitorClientPort);
	Parse.GetValue(L"THREAD_NUMBER", &monitorClientThreadNum);
	Parse.GetValue(L"IOCP_NUMBER", &monitorClientIOCPNum);


	CChatServer* server = new CChatServer;
	CMonitoringLanClient* monitorClient = new CMonitoringLanClient;
	server->Start(INADDR_ANY, ServerPort, ServerThreadNum, ServerIOCPNum, ServerMaxSession, false);
	server->_LoginClient.Start(ClientIP, ClientPort, ClientThreadNum, ClientIOCPNum);
	monitorClient->Start(monitorClientIP, monitorClientPort, monitorClientThreadNum, monitorClientIOCPNum);
	while (1) {
	//1�ʸ��� List ��Ȳ ���
		wprintf(L"================ChatServer====================\n");
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
		wprintf(L"================ChatLanClient================\n");
		wprintf(L"[TokenCount: %lld]\n", server->_LoginClient.GetTokenCount());
		wprintf(L"[Lan Send TPS: %d]\n", server->_LoginClient._SendTPS);
		wprintf(L"[Lan Recv TPS: %d]\n", server->_LoginClient._RecvTPS);
		wprintf(L"[Lan Connnect Sucess Count: %d]\n", server->_LoginClient._ConnectSuccess);
		wprintf(L"[Lan Connect Fail Count: %d]\n", server->_LoginClient._ConnectFail);
		wprintf(L"[Lan Disconnect Count: %d]\n", server->_LoginClient._DisCount);
		wprintf(L"[PacketNum: %lld]\n", PacketNum);
		wprintf(L"================MonitorClient===============\n");
		wprintf(L"[Monitor Connnect Sucess Count: %d]\n", monitorClient->_ConnectSuccess);
		ProcessUsage.UpdateProcessTime();
		ProcessUsage.PrintProcessInfo();
		wprintf(L"\n\n");

		//MonitorServer�� ������ ����
		DWORD TimeStamp = time(NULL);
		// ������Ʈ ChatServer ���� ���� ON / OFF
		monitorClient->TransferData(dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN, 1, TimeStamp);
		// ������Ʈ ChatServer CPU ����
		monitorClient->TransferData(dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU, (int)ProcessUsage.ProcessTotal(), TimeStamp);
		// ������Ʈ ChatServer �޸� ��� MByte
		monitorClient->TransferData(dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM, NULL, TimeStamp);
		// ä�ü��� ���� �� (���ؼ� ��)
		monitorClient->TransferData(dfMONITOR_DATA_TYPE_CHAT_SESSION, server->GetClientCount(), TimeStamp);
		// ä�ü��� �������� ����� �� (���� ������)
		monitorClient->TransferData(dfMONITOR_DATA_TYPE_CHAT_PLAYER, server->GetPlayerCount(), TimeStamp);
		// ä�ü��� UPDATE ������ �ʴ� �ʸ� Ƚ��
		monitorClient->TransferData(dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS, server->_MsgTPS, TimeStamp);
		// ä�ü��� ��ŶǮ ��뷮
		monitorClient->TransferData(dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL, CPacket::GetAllocCount(), TimeStamp);
		// ä�ü��� UPDATE MSG Ǯ ��뷮
		monitorClient->TransferData(dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, server->GetMsgQueueCount(), TimeStamp);

		server->_AcceptTPS = 0;
		server->_SendTPS = 0;
		server->_RecvTPS = 0;
		server->_MsgTPS = 0;
		server->_LoginClient._RecvTPS = 0;
		server->_LoginClient._SendTPS = 0;
		if (_kbhit()) {
			WCHAR cmd = _getch();
			if (cmd == L'q' || cmd == L'Q')
				CCrashDump::Crash();
		}
		Sleep(999);
	}
}