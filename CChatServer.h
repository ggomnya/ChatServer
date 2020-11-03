#pragma once
#include "CNetServer.h"
#include "CLoginClient.h"
#include "CommonProtocol_Login.h"
#include <list>
#include <unordered_map>
using namespace std;

class CChatServer : public CNetServer {
private:
#define dfJOIN		0
#define dfPACKET	1
#define dfLEAVE		2
#define dfHEARTBEAT	40

	DWORD _dwHeartbeatTime;
	DWORD _dwTokenTime;
	HANDLE _hUpdateThread;
	HANDLE _hEvent;
	CObjectPool<st_UPDATE_MSG> _UpdateMsgPool;
	CLockfreeQueue<st_UPDATE_MSG*> _MsgQueue;
	CObjectPool<CPlayer> _PlayerPool;
	list<CPlayer*> _Sector[100][100];
	unordered_map<INT64, CPlayer*> _PlayerMap;

public:
	CLoginClient _LoginClient;
	INT64 _HeartbeatDis;
	INT64 _MsgTPS;
	INT64 _BlockCount;
	INT64 _SessionNotFound;
	INT64 _SessionMiss;
	CChatServer();
	static unsigned int WINAPI UpdateThreadFunc(LPVOID lParam) {
		((CChatServer*)lParam)->UpdateThread(lParam);
		return 0;
	}
	unsigned int WINAPI UpdateThread(LPVOID lParam);

	void JoinMSG(INT64 SessionID);
	void LeaveMSG(INT64 SessionID);

	void CheckHeartbeat();
	void CheckToken();

	void ReqLogin(CPacket* pPacket, INT64 SessionID);
	void ReqSectorMove(CPacket* pPacket, INT64 SessionID);
	void ReqMessage(CPacket* pPacket, INT64 SessionID);
	void ReqHeartbeat(CPacket* pPacket, INT64 SessionID);

	void MPResLogin(CPacket* pPacket, WORD Type, BYTE Status, INT64 SessionID);
	void MPResSectorMove(CPacket* pPacket, WORD Type, INT64 AccountNo, WORD SectorX, WORD SectorY);
	void MPResMessage(CPacket* pPacket, WORD Type, INT64 AccountNo, WCHAR* ID, WCHAR* Nickname,
		WORD MessageLen, WCHAR* Message);

	void InsertPlayerMap(CPlayer* pPlayer);
	CPlayer* FindPlayerMap(INT64 SessionID);
	void RemovePlayerMap(CPlayer* pPlayer);

	//Sector function
	void Sector_AddPlayer(CPlayer* pPlayer);
	bool Sector_RemovePlayer(CPlayer* pPlayer);
	void SendPacketSector(CPacket* pPacket, WORD SectorX, WORD SectorY);

	INT64 GetPlayerPoolCount();
	INT64 GetUpdateMsgPoolCount();
	INT64 GetPlayerCount();
	INT64 GetMsgQueueCount();


	virtual void OnClientJoin(SOCKADDR_IN clientaddr, INT64 SessionID);
	virtual void OnClientLeave(INT64 SessionID);
	virtual bool OnConnectRequest(SOCKADDR_IN clientaddr);
	virtual void OnRecv(INT64 SessionID, CPacket* pRecvPacket);
	//virtual void OnSend(INT64 SessionID, int SendSize) = 0;
	virtual void OnError(int errorcode, const WCHAR* Err);
};