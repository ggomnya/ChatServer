#pragma once
#include "CNetServer.h"
#include "CommonProtocol_Login.h"
#include <list>
#include <unordered_map>
using namespace std;

extern SRWLOCK srwTOKEN;
extern unordered_map<INT64, string> TokenMap;

class CChatServer : public CNetServer {
private:
#define dfJOIN		0
#define dfPACKET	1
#define dfLEAVE		2
#define dfHEARTBEAT	40
	struct st_UPDATE_MSG {
		BYTE byType;
		CPacket* pPacket;
		INT64 SessionID;
	};

	class CPlayer {
	public:
		INT64 _SessionID;
		INT64 _LastAccountNo;
		INT64 _AccountNo;
		WCHAR _ID[20];
		WCHAR _Nickname[20];
		char _SessionKey[64];
		short _SectorX;
		short _SectorY;
		short _LastSectorX;
		short _LastSectorY;
		DWORD _dwRecvTime;
		INT64 _LastSessionID;
		BYTE _LastMsg;

	};

	DWORD _dwHeartbeatTime;
	HANDLE _hUpdateThread;
	HANDLE _hEvent;
	CObjectPool<st_UPDATE_MSG> _UpdateMsgPool;
	CLockfreeQueue<st_UPDATE_MSG*> _MsgQueue;
	CObjectPool<CPlayer> _PlayerPool;
	list<CPlayer*> _Sector[100][100];
	//unordered_map<INT64, CPlayer*> _AcceptMap;
	unordered_map<INT64, CPlayer*> _PlayerMap;

public:
	INT64 _HeartbeatDis;
	INT64 _MsgTPS;
	INT64 _BlockCount;
	CChatServer();
	static unsigned int WINAPI UpdateThreadFunc(LPVOID lParam) {
		((CChatServer*)lParam)->UpdateThread(lParam);
		return 0;
	}
	unsigned int WINAPI UpdateThread(LPVOID lParam);

	void JoinMSG(INT64 SessionID);
	void LeaveMSG(INT64 SessionID);

	void CheckHeartbeat();

	void ReqLogin(CPacket* pPacket, INT64 SessionID);
	void ReqSectorMove(CPacket* pPacket, INT64 SessionID);
	void ReqMessage(CPacket* pPacket, INT64 SessionID);
	void ReqHeartbeat(CPacket* pPacket, INT64 SessionID);

	void MPResLogin(CPacket* pPacket, WORD Type, BYTE Status, INT64 SessionID);
	void MPResSectorMove(CPacket* pPacket, WORD Type, INT64 AccountNo, WORD SectorX, WORD SectorY);
	void MPResMessage(CPacket* pPacket, WORD Type, INT64 AccountNo, WCHAR* ID, WCHAR* Nickname,
		WORD MessageLen, WCHAR* Message);

	/*void InsertAcceptMap(INT64 SessionID, CPlayer* pPlayer);
	CPlayer* FindAcceptMap(INT64 SessionID);
	void RemoveAcceptMap(CPlayer* pPlayer);*/

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