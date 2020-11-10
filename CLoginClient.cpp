#include "CLoginClient.h"
SRWLOCK CLoginClient::_srwTOKEN;
CObjectPool<st_TOKEN>CLoginClient::_TokenPool;
unordered_map<INT64, st_TOKEN*>CLoginClient::_TokenMap;

extern INT64 PacketNum;

CLoginClient::CLoginClient() {
	InitializeSRWLock(&_srwTOKEN);
}

void CLoginClient::MPLoginServerLogin(CPacket* pPacket, WORD Type, BYTE ServerType, WCHAR* ServerName) {
	*pPacket << Type << ServerType;
	pPacket->PutData((char*)ServerName, 64);
}

void CLoginClient::MPResNewClientLogin(CPacket* pPacket, WORD Type, INT64 AccountNo, INT64 Parameter) {
	*pPacket << Type << AccountNo << Parameter;
}

INT64 CLoginClient::GetTokenCount() {
	return _TokenMap.size();
}

void CLoginClient::OnEnterJoinServer(INT64 SessionID) {
	//서버에 내 IP와 TYPE 보내기
	CPacket* pSendPacket = CPacket::Alloc();
	//InterlockedIncrement64(&PacketNum);
	WCHAR ServerName[32];
	swprintf_s(ServerName, L"ChatServer");
	MPLoginServerLogin(pSendPacket, en_PACKET_SS_LOGINSERVER_LOGIN, dfSERVER_TYPE_CHAT, ServerName);
	SendPacket(pSendPacket);
	pSendPacket->Free();
	//InterlockedDecrement64(&PacketNum);
}

void CLoginClient::OnLeaveServer(INT64 SessionID) {
	ReConnect();
}

void CLoginClient::OnRecv(INT64 SessionID, CPacket* pRecvPacket) {
	WORD Type;
	INT64 AccountNo;
	INT64 Parameter;
	*pRecvPacket >> Type >> AccountNo;

	if (Type != en_PACKET_SS_REQ_NEW_CLIENT_LOGIN) {
		CCrashDump::Crash();
	}
	AcquireSRWLockExclusive(&_srwTOKEN);
	st_TOKEN* pToken = FindToken(AccountNo);
	//처음 온 Account인 경우
	if (pToken == NULL) {
		st_TOKEN* newToken = _TokenPool.Alloc();
		pRecvPacket->GetData(newToken->Token, 64);
		newToken->UpdateTime = timeGetTime();
		InsertToken(newToken, AccountNo);
	}
	else {
		pRecvPacket->GetData(pToken->Token, 64);
		pToken->UpdateTime = timeGetTime();
	}
	ReleaseSRWLockExclusive(&_srwTOKEN);
	*pRecvPacket >> Parameter;
	CPacket* pSendPacket = CPacket::Alloc();
	//InterlockedIncrement64(&PacketNum);
	MPResNewClientLogin(pSendPacket, en_PACKET_SS_RES_NEW_CLIENT_LOGIN, AccountNo, Parameter);
	SendPacket(pSendPacket);
	pSendPacket->Free();
	//InterlockedDecrement64(&PacketNum);
}

void CLoginClient::OnError(int errorcode, const WCHAR* Err) {
	wprintf(L"errcode: %d, %s\n", errorcode, Err);
	return;
}

