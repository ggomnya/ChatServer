#pragma once
#include "CLanClient.h"
#include <unordered_map>
#include "CommonProtocol_Login.h"
using namespace std;
//key - AccountNo, value - ¿Œ¡ı Token


class CLoginClient : public CLanClient {
private:
	friend class CChatServer;
	struct st_TOKEN {
		char Token[64];
		DWORD UpdateTime;
	};
	static SRWLOCK _srwTOKEN;
	static CObjectPool<st_TOKEN> _TokenPool;
	static unordered_map<INT64, st_TOKEN*> _TokenMap;

public:
	CLoginClient();
	void MPLoginServerLogin(CPacket* pPacket, WORD Type, BYTE ServerType, WCHAR* ServerName);
	void MPResNewClientLogin(CPacket* pPacket, WORD Type, INT64 AccountNo, INT64 Parameter);
	
	INT64 GetTokenCount();
	
	virtual void OnEnterJoinServer(INT64 SessionID);
	virtual void OnLeaveServer(INT64 SessionID);

	virtual void OnRecv(INT64 SessionID, CPacket* pRecvPacket);
	//virtual void OnSend(INT64 SessionID, int SendSize) = 0;

	virtual void OnError(int errorcode, const WCHAR* Err);

};
