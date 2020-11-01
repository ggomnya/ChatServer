#pragma once
#include "CLanClient.h"
#include <unordered_map>
#include "CommonProtocol_Login.h"
using namespace std;
//key - AccountNo, value - ¿Œ¡ı Token
extern SRWLOCK srwTOKEN;
extern unordered_map<INT64, string> TokenMap;

class CLoginClient : public CLanClient {

public:
	void MPLoginServerLogin(CPacket* pPacket, WORD Type, BYTE ServerType, WCHAR* ServerName);
	void MPResNewClientLogin(CPacket* pPacket, WORD Type, INT64 AccountNo, INT64 Parameter);
	virtual void OnEnterJoinServer(INT64 SessionID);
	virtual void OnLeaveServer(INT64 SessionID);

	virtual void OnRecv(INT64 SessionID, CPacket* pRecvPacket);
	//virtual void OnSend(INT64 SessionID, int SendSize) = 0;

	virtual void OnError(int errorcode, const WCHAR* Err);

};
