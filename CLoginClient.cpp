#include "CLoginClient.h"



void CLoginClient::MPLoginServerLogin(CPacket* pPacket, WORD Type, BYTE ServerType, WCHAR* ServerName) {
	*pPacket << Type << ServerType;
	pPacket->PutData((char*)ServerName, 64);
}

void CLoginClient::MPResNewClientLogin(CPacket* pPacket, WORD Type, INT64 AccountNo, INT64 Parameter) {
	*pPacket << Type << AccountNo << Parameter;
}

void CLoginClient::OnEnterJoinServer(INT64 SessionID) {
	//서버에 내 IP와 TYPE 보내기
	CPacket* pSendPacket = CPacket::Alloc();
	WCHAR ServerName[32];
	swprintf_s(ServerName, L"ChatServer");
	MPLoginServerLogin(pSendPacket, en_PACKET_SS_LOGINSERVER_LOGIN, dfSERVER_TYPE_CHAT, ServerName);
	SendPacket(pSendPacket);
	pSendPacket->Free();
}

void CLoginClient::OnLeaveServer(INT64 SessionID) {
	ReConnect();
}

void CLoginClient::OnRecv(INT64 SessionID, CPacket* pRecvPacket) {
	WORD Type;
	INT64 AccountNo;
	char SessionKey[64];
	INT64 Parameter;
	*pRecvPacket >> Type >> AccountNo;
	pRecvPacket->GetData(SessionKey, 64);
	*pRecvPacket >> Parameter;

	if (Type != en_PACKET_SS_REQ_NEW_CLIENT_LOGIN) {
		CCrashDump::Crash();
	}
	AcquireSRWLockExclusive(&srwTOKEN);
	auto it = TokenMap.find(AccountNo);
	//처음 온 Account인 경우
	if (it == TokenMap.end()) {
		TokenMap.insert(make_pair(AccountNo, SessionKey));
	}
	else {
		it->second = SessionKey;
	}
	ReleaseSRWLockExclusive(&srwTOKEN);
	CPacket* pSendPacket = CPacket::Alloc();
	MPResNewClientLogin(pSendPacket, en_PACKET_SS_RES_NEW_CLIENT_LOGIN, AccountNo, Parameter);
	SendPacket(pSendPacket);
	pSendPacket->Free();
}

void CLoginClient::OnError(int errorcode, const WCHAR* Err) {
	wprintf(L"errcode: %d, %s\n", errorcode, Err);
	return;
}