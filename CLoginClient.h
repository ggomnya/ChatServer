#pragma once
#include "CLanClient.h"
#include <unordered_map>
#include "CommonProtocol.h"
using namespace std;
//key - AccountNo, value - ÀÎÁõ Token

class CLoginClient : public CLanClient {
public:
	CLoginClient();
	void MPLoginServerLogin(CPacket* pPacket, WORD Type, BYTE ServerType, WCHAR* ServerName);
	void MPResNewClientLogin(CPacket* pPacket, WORD Type, INT64 AccountNo, INT64 Parameter);
	
	INT64 GetTokenCount();

	static void InsertToken(st_TOKEN* pToken, INT64 AccountNo) {
		_TokenMap.insert(make_pair(AccountNo, pToken));
	}
	static st_TOKEN* FindToken(INT64 AccountNo) {
		st_TOKEN* pToken = NULL;
		auto it = _TokenMap.find(AccountNo);
		if (it != _TokenMap.end()) {
			pToken = it->second;
		}
		return pToken;
	}
	static void RemoveToken(INT64 AccountNo) {
		_TokenMap.erase(AccountNo);
	}
	
	virtual void OnEnterJoinServer(INT64 SessionID);
	virtual void OnLeaveServer(INT64 SessionID);
	virtual void OnRecv(INT64 SessionID, CPacket* pRecvPacket);
	//virtual void OnSend(INT64 SessionID, int SendSize) = 0;
	virtual void OnError(int errorcode, const WCHAR* Err);

private:
	friend class CChatServer;
	static SRWLOCK _srwTOKEN;
	static CObjectPool<st_TOKEN> _TokenPool;
	static unordered_map<INT64, st_TOKEN*> _TokenMap;

};
