#include "CLoginClient.h"
SRWLOCK CLoginClient::_srwTOKEN;
CObjectPool<CLoginClient::st_TOKEN>CLoginClient::_TokenPool;
unordered_map<INT64, CLoginClient::st_TOKEN*>CLoginClient::_TokenMap;

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
	//������ �� IP�� TYPE ������
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
	INT64 Parameter;
	*pRecvPacket >> Type >> AccountNo;

	if (Type != en_PACKET_SS_REQ_NEW_CLIENT_LOGIN) {
		CCrashDump::Crash();
	}
	AcquireSRWLockExclusive(&_srwTOKEN);
	auto it = _TokenMap.find(AccountNo);
	//ó�� �� Account�� ���
	if (it == _TokenMap.end()) {
		st_TOKEN* newToken = _TokenPool.Alloc();
		pRecvPacket->GetData(newToken->Token, 64);
		newToken->UpdateTime = timeGetTime();
		_TokenMap.insert(make_pair(AccountNo, newToken));
	}
	else {
		pRecvPacket->GetData(it->second->Token, 64);
		it->second->UpdateTime = timeGetTime();
	}
	*pRecvPacket >> Parameter;
	ReleaseSRWLockExclusive(&_srwTOKEN);
	CPacket* pSendPacket = CPacket::Alloc();
	MPResNewClientLogin(pSendPacket, en_PACKET_SS_RES_NEW_CLIENT_LOGIN, AccountNo, Parameter);
	SendPacket(pSendPacket);
	pSendPacket->Free();
}

void CLoginClient::OnError(int errorcode, const WCHAR* Err) {
	wprintf(L"errcode: %d, %s\n", errorcode, Err);
	return;
}

