#include "CChatServer.h"
//#include "SystemLog.h"
CChatServer::CChatServer() {
	_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	_hUpdateThread = (HANDLE)_beginthreadex(NULL, 0, UpdateThreadFunc, this, 0, NULL);
	_dwHeartbeatTime = timeGetTime();
	_HeartbeatDis = 0;
	_MsgTPS = 0;
	_SessionNotFound = 0;
	_SessionMiss = 0;
	_dwTokenTime = timeGetTime();
}

unsigned int WINAPI CChatServer::UpdateThread(LPVOID lParam) {
	/*SYSLOG_DIRECTORY(L"Log_Heartbeat");
	SYSLOG_LEVEL(LEVEL_DEBUG);*/
	while (1) {
		int retval = WaitForSingleObject(_hEvent, 1000*10);
		//_BlockCount++;
		if (retval == WAIT_OBJECT_0) {
			while (1) {
				if (_MsgQueue.Size() == 0)
					break;
				/*if(timeGetTime() - _dwHeartbeatTime > 1000*10)
					CheckHeartbeat();*/
				/*if (timeGetTime() - _dwTokenTime > 1000 * 15)
					CheckToken();*/
				st_UPDATE_MSG* updateMsg;
				_MsgQueue.Dequeue(&updateMsg);
				_MsgTPS++;
				switch (updateMsg->byType) {
				case dfJOIN: {
					JoinMSG(updateMsg->SessionID);
					break;
				}
				case dfPACKET: {
					WORD Type;
					*(updateMsg->pPacket) >> Type;
					//��Ŷ Ÿ�Կ� ���� ó��
					switch (Type) {
					case en_PACKET_CS_CHAT_REQ_LOGIN:
					{
						ReqLogin(updateMsg->pPacket, updateMsg->SessionID);
						break;
					}
					case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
					{
						ReqSectorMove(updateMsg->pPacket, updateMsg->SessionID);
						break;
					}
					case en_PACKET_CS_CHAT_REQ_MESSAGE:
					{
						ReqMessage(updateMsg->pPacket, updateMsg->SessionID);
						break;
					}
					case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
					{
						ReqHeartbeat(updateMsg->pPacket, updateMsg->SessionID);
						break;
					}
					default:
						Disconnect(updateMsg->SessionID);
						break;
					}
					updateMsg->pPacket->Free();
					break;
				}
				case dfLEAVE: {
					LeaveMSG(updateMsg->SessionID);
					break;
				}
				default:
					CCrashDump::Crash();
					break;
				}
				_UpdateMsgPool.Free(updateMsg);
			}
		}
		//�ƹ� �޼����� ���� ���� ��� Heartbeat Check
		else if (retval == WAIT_TIMEOUT) {
			//CheckHeartbeat();
		}
	}
}

void CChatServer::JoinMSG(INT64 SessionID) {
	CPlayer* newPlayer = _PlayerPool.Alloc();
	newPlayer->_LastSessionID = newPlayer->_SessionID;
	newPlayer->_SessionID = SessionID;
	newPlayer->_dwRecvTime = timeGetTime();
	newPlayer->_LastAccountNo = newPlayer->_AccountNo;
	newPlayer->_LastSectorX = newPlayer->_SectorX;
	newPlayer->_LastSectorY = newPlayer->_SectorY;
	newPlayer->_SectorX = -1;
	newPlayer->_SectorY = -1;
	newPlayer->_LastMsg = 0;
	newPlayer->_AccountNo = -1;
	newPlayer->_MSGLen = 0;
	memset((char*)newPlayer->_MSG, 0, 2048);
	InsertPlayerMap(newPlayer);
}

void CChatServer::LeaveMSG(INT64 SessionID) {
	CPlayer* pPlayer = FindPlayerMap(SessionID);
	if (pPlayer == NULL) {
		//�̰��� �־ �ȵ�
		CCrashDump::Crash();
		return;
	}
	else {
		//���Ϳ� ���� ���
		if (pPlayer->_SectorX == -1 && pPlayer->_SectorY == -1) {
			RemovePlayerMap(pPlayer);
		}
		//���Ϳ� �����ϴ� ���
		else {
			bool retval = Sector_RemovePlayer(pPlayer);
			if (!retval) {
				//�־ �ȵ�
				CCrashDump::Crash();
				return;
			}
			RemovePlayerMap(pPlayer);
		}
		//���̰� ���� �������� Ȯ���ϱ�
		if (memcmp((char*)pPlayer->_MSG, L"=", pPlayer->_MSGLen) != 0) {
			CCrashDump::Crash();
		}
		_PlayerPool.Free(pPlayer);
	}

}
//PlayerMap���� ������ ���� �� ���ΰ�?
void CChatServer::CheckHeartbeat() {
	_dwHeartbeatTime = timeGetTime();
	for (auto it = _PlayerMap.begin(); it != _PlayerMap.end();) {
		//�� ��� ���� ����
		if (it->second->_dwRecvTime < _dwHeartbeatTime - 1000*10) {
			//CCrashDump::Crash();
			CPlayer* pPlayer = it->second;
			/*/
			/*_LOG(L"HEARTBEAT", LEVEL_DEBUG, L"SessionID: %ld, AccountNo:%ld, RecvTime: %d\n", 
				pPlayer->_SessionID, pPlayer->_AccountNo, pPlayer->_dwRecvTime);*/
			//_PlayerPool.Free(pPlayer);
			_HeartbeatDis++;
			Disconnect(it->second->_SessionID);
			
		}
		it++;
	}
}

void CChatServer::CheckToken() {
	_dwTokenTime = timeGetTime();
	AcquireSRWLockExclusive(&CLoginClient::_srwTOKEN);
	for (auto it = CLoginClient::_TokenMap.begin(); it != CLoginClient::_TokenMap.end();) {
		if (it->second->UpdateTime < _dwTokenTime - 1000 * 15) {
			st_TOKEN* Token = it->second;
			it = CLoginClient::_TokenMap.erase(it);
			CLoginClient::_TokenPool.Free(Token);
		}
		else
			it++;
	}
	ReleaseSRWLockExclusive(&CLoginClient::_srwTOKEN);
}

//Req function
void CChatServer::ReqLogin(CPacket* pPacket, INT64 SessionID) {
	CPlayer* pPlayer = FindPlayerMap(SessionID);
	if (pPlayer == NULL) {
		CPacket* pSendPacket = CPacket::Alloc();
		MPResLogin(pSendPacket, en_PACKET_CS_CHAT_RES_LOGIN, 0, 0);
		SendPacket(SessionID, pSendPacket);
		pSendPacket->Free();
		Disconnect(SessionID);
		CCrashDump::Crash();
	}
	else {
		if (pPacket->GetDataSize() != 152) {
			Disconnect(SessionID);
			return;
		}
		//TokenMap�� ���� �ش� ��ū�� ��ȿ���� Ȯ��
		bool bLogin = true;
		*pPacket >> pPlayer->_AccountNo;
		pPacket->GetData((char*)pPlayer->_ID, 40);
		pPacket->GetData((char*)pPlayer->_Nickname, 40);
		pPacket->GetData(pPlayer->_SessionKey, 64);
		AcquireSRWLockExclusive(&CLoginClient::_srwTOKEN);
		st_TOKEN* pToken = CLoginClient::FindToken(pPlayer->_AccountNo);
		//�ش� account�� ���� ���
		if (pToken == NULL) {
			ReleaseSRWLockExclusive(&CLoginClient::_srwTOKEN);
			_SessionNotFound++;
			Disconnect(SessionID);
			return;
		}
		else {
			//��ū�� �ٸ� ���
			if (memcmp((char*)pToken->Token, (char*)pPlayer->_SessionKey, 64)!=0) {
				_SessionMiss++;
				Disconnect(SessionID);
				bLogin = false;
			}
			//����� ��ū�� ���� ���
			CLoginClient::_TokenMap.erase(pPlayer->_AccountNo);
			ReleaseSRWLockExclusive(&CLoginClient::_srwTOKEN);
			CLoginClient::_TokenPool.Free(pToken);
		}
		if (bLogin) {
			pPlayer->_dwRecvTime = timeGetTime();
			pPlayer->_LastMsg = en_PACKET_CS_CHAT_RES_LOGIN;
			CPacket* pSendPacket = CPacket::Alloc();
			MPResLogin(pSendPacket, en_PACKET_CS_CHAT_RES_LOGIN, 1, pPlayer->_AccountNo);
			SendPacket(SessionID, pSendPacket, NET, true);
			pSendPacket->Free();
		}
	}
	
}

void CChatServer::ReqSectorMove(CPacket* pPacket, INT64 SessionID) {
	INT64 AccountNo;
	WORD SectorX;
	WORD SectorY;
	if (pPacket->GetDataSize() != 12) {
		Disconnect(SessionID);
		return;
	}
	*pPacket >> AccountNo >> SectorX >> SectorY;
	CPlayer* pPlayer = FindPlayerMap(SessionID);
	if (pPlayer == NULL) {
		Disconnect(SessionID);
		//CCrashDump::Crash();
	}
	else {
		//AccountNo�� Ʋ�� ���
		if (pPlayer->_AccountNo != AccountNo) {
			//CCrashDump::Crash();
			Disconnect(SessionID);
			return;
		}
		//�߸��� ���ͷ� �̵��Ϸ� �ϴ� ���
		if (SectorX < 0 || SectorX>50 || SectorY < 0 || SectorY>50) {
			Disconnect(SessionID);
			return;
		}
		pPlayer->_dwRecvTime = timeGetTime();
		pPlayer->_LastMsg = en_PACKET_CS_CHAT_RES_SECTOR_MOVE;
		
		//���� ������ �ȵ� ���
		if (pPlayer->_SectorX == -1 && pPlayer->_SectorY == -1) {
			pPlayer->_SectorX = SectorX;
			pPlayer->_SectorY = SectorY;
			Sector_AddPlayer(pPlayer);
		}
		//������ �� ���
		else {
			bool retval = Sector_RemovePlayer(pPlayer);
			if (!retval) {
				//�־ �ȵ�
				//CCrashDump::Crash();
				return;
			}
			pPlayer->_SectorX = SectorX;
			pPlayer->_SectorY = SectorY;
			Sector_AddPlayer(pPlayer);
		}
		CPacket* pSendPacket = CPacket::Alloc();
		MPResSectorMove(pSendPacket, en_PACKET_CS_CHAT_RES_SECTOR_MOVE, AccountNo, SectorX, SectorY);
		SendPacket(SessionID, pSendPacket, NET, true);
		pSendPacket->Free();
	}
}

void CChatServer::ReqMessage(CPacket* pPacket, INT64 SessionID) {
	INT64 AccountNo;
	WORD MessageLen;
	WCHAR Message[1024];
	CPlayer* pPlayer = FindPlayerMap(SessionID);
	if (pPlayer == NULL) {
		Disconnect(SessionID);
		//CCrashDump::Crash();
	}
	else {
		if (pPacket->GetDataSize() < 10) {
			Disconnect(SessionID);
			return;
		}
		*pPacket >> AccountNo;
		//AccountNo�� Ʋ�� ���
		if (pPlayer->_AccountNo != AccountNo) {
			//CCrashDump::Crash();
			Disconnect(SessionID);
			return;
		}
		pPlayer->_dwRecvTime = timeGetTime();
		pPlayer->_LastMsg = en_PACKET_CS_CHAT_RES_MESSAGE;
		//���� ������ �ȵ� ���
		if (pPlayer->_SectorX == -1 && pPlayer->_SectorY == -1) {
			//CCrashDump::Crash();
			Disconnect(SessionID);
		}
		else {
			*pPacket >> MessageLen;
			if (pPacket->GetDataSize() != MessageLen) {
				Disconnect(SessionID);
				return;
			}
			pPacket->GetData((char*)Message, MessageLen);
			memcpy((char*)pPlayer->_MSG, Message, MessageLen);
			pPlayer->_MSGLen = MessageLen;
			//�ڽ� �ֺ� 9�� ���Ϳ� �޼��� ������
			CPacket* pSendPacket = CPacket::Alloc();
			MPResMessage(pSendPacket, en_PACKET_CS_CHAT_RES_MESSAGE, AccountNo, pPlayer->_ID,
				pPlayer->_Nickname, MessageLen, Message);
			WORD SectorX = pPlayer->_SectorX;
			WORD SectorY = pPlayer->_SectorY;
			for (int i = 0; i < 3; i++) {
				for (int j = 0; j < 3; j++) {
					if (SectorX - 1 + i < 0)
						continue;
					if (SectorX - 1 + i >= 100)
						continue;
					if (SectorY - 1 + j < 0)
						continue;
					if (SectorY - 1 + j >= 100)
						continue;
					SendPacketSector(pSendPacket, SectorX - 1 + i, SectorY - 1 + j);
				}
			}
			pSendPacket->Free();
		}
	}
}

void CChatServer::ReqHeartbeat(CPacket* pPacket, INT64 SessionID) {
	CPlayer* pPlayer = FindPlayerMap(SessionID);
	if (pPlayer == NULL) {
		Disconnect(SessionID);
		//CCrashDump::Crash();
	}
	else {
		if (pPacket->GetDataSize() > 0) {
			Disconnect(SessionID);
			return;
		}
		pPlayer->_dwRecvTime = timeGetTime();
		pPlayer->_LastMsg = en_PACKET_CS_CHAT_REQ_HEARTBEAT;
	}
}

//MP function
void CChatServer::MPResLogin(CPacket* pPacket, WORD Type, BYTE Status, INT64 AccountNo) {
	*pPacket << Type << Status << AccountNo;
}

void CChatServer::MPResSectorMove(CPacket* pPacket, WORD Type, INT64 AccountNo, WORD SectorX, WORD SectorY) {
	*pPacket << Type << AccountNo << SectorX << SectorY;
}

void CChatServer::MPResMessage(CPacket* pPacket, WORD Type, INT64 AccountNo, WCHAR* ID, WCHAR* Nickname,
	WORD MessageLen, WCHAR* Message) {
	*pPacket << Type << AccountNo;
	pPacket->PutData((char*)ID, 40);
	pPacket->PutData((char*)Nickname, 40);
	*pPacket << MessageLen;
	pPacket->PutData((char*)Message, MessageLen);
}

void CChatServer::InsertPlayerMap(CPlayer* pPlayer) {
	_PlayerMap.insert(make_pair(pPlayer->_SessionID, pPlayer));
}

CPlayer* CChatServer::FindPlayerMap(INT64 SessionID) {
	auto it = _PlayerMap.find(SessionID);
	if (it != _PlayerMap.end()) {
		return it->second;
	}
	else return NULL;
}

void CChatServer::RemovePlayerMap(CPlayer* pPlayer) {
	_PlayerMap.erase(pPlayer->_SessionID);
}


//Sector function
void CChatServer::Sector_AddPlayer(CPlayer* pPlayer) {
	_Sector[pPlayer->_SectorY][pPlayer->_SectorX].push_back(pPlayer);
}

bool CChatServer::Sector_RemovePlayer(CPlayer* pPlayer) {
	auto IterEnd = _Sector[pPlayer->_SectorY][pPlayer->_SectorX].end();
	for (auto it = _Sector[pPlayer->_SectorY][pPlayer->_SectorX].begin(); it != IterEnd;) {
		//ã�� ���
		if ((*it) == pPlayer) {
			_Sector[pPlayer->_SectorY][pPlayer->_SectorX].erase(it);
			return true;
		}
		it++;
	}
	return false;
}

void CChatServer::SendPacketSector(CPacket* pPacket, WORD SectorX, WORD SectorY) {
	auto IterEnd = _Sector[SectorY][SectorX].end();
	for (auto it = _Sector[SectorY][SectorX].begin(); it != IterEnd;) {
		SendPacket((*it)->_SessionID, pPacket, NET, true);
		it++;
	}
}


INT64 CChatServer::GetPlayerPoolCount() {
	return _PlayerPool.GetAllocCount();
}
INT64 CChatServer::GetUpdateMsgPoolCount() {
	return _UpdateMsgPool.GetAllocCount();
}
INT64 CChatServer::GetPlayerCount() {
	return _PlayerMap.size();
}
INT64 CChatServer::GetMsgQueueCount() {
	return _MsgQueue.Size();
}


void CChatServer::OnClientJoin(SOCKADDR_IN clientaddr, INT64 SessionID) {
	st_UPDATE_MSG* updateMsg = _UpdateMsgPool.Alloc();
	updateMsg->byType = dfJOIN;
	updateMsg->pPacket = NULL;
	updateMsg->SessionID = SessionID;
	_MsgQueue.Enqueue(updateMsg);
	SetEvent(_hEvent);
	
}

void CChatServer::OnClientLeave(INT64 SessionID) {
	st_UPDATE_MSG* updateMsg = _UpdateMsgPool.Alloc();
	updateMsg->byType = dfLEAVE;
	updateMsg->pPacket = NULL;
	updateMsg->SessionID = SessionID;
	_MsgQueue.Enqueue(updateMsg);
	SetEvent(_hEvent);
}

bool CChatServer::OnConnectRequest(SOCKADDR_IN clientaddr) {
	return true;
}

void CChatServer::OnRecv(INT64 SessionID, CPacket* pRecvPacket) {
	if (pRecvPacket->GetDataSize() < 2) {
		Disconnect(SessionID);
		return;
	}
	st_UPDATE_MSG* updateMsg = _UpdateMsgPool.Alloc();
	updateMsg->byType = dfPACKET;
	updateMsg->pPacket = pRecvPacket;
	updateMsg->SessionID = SessionID;
	pRecvPacket->AddRef();
	_MsgQueue.Enqueue(updateMsg);
	SetEvent(_hEvent);
}

void CChatServer::OnError(int errorcode, const WCHAR* Err) {
	wprintf(L"errcode: %d, %s\n", errorcode, Err);
	return;
}