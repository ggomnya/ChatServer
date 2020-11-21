#include "CNetServer.h"
#include "SystemLog.h"

extern INT64 PacketNum;

CNetServer::CNetServer() {
	_SessionIDCnt = 0;
	_AcceptCount = 0;
	_AcceptTPS = 0;
	_SessionCnt = 0;
	_SendTPS = 0;
	_RecvTPS = 0;
	_DisCount = 0;
}
unsigned int WINAPI CNetServer::WorkerThread(LPVOID lParam) {
	while (1) {
		int retval;
		DWORD cbTransferred = 0;
		stSESSION* pSession;
		stOVERLAPPED* Overlapped;
		retval = GetQueuedCompletionStatus(_hcp, &cbTransferred, (PULONG_PTR)&pSession, (LPOVERLAPPED*)&Overlapped, INFINITE);
		if (retval == false) {
			DWORD dwError = GetLastError();
			if (dwError != 64)
				Log(L"GQCS_CHAT", LEVEL_WARNING, (WCHAR*)L"SessionID: %lld, Overlapped Type: %d, Overlapped Error:%lld, WSAGetLastError: %d\n", 
					pSession->SessionID, Overlapped->Type, Overlapped->Overlapped.Internal, dwError);
		}
		//종료 처리
		if (Overlapped == NULL) {
			return 0;
		}
		if (&pSession->RecvOverlapped != Overlapped && &pSession->SendOverlapped != Overlapped && &pSession->UpdateOverlapped != Overlapped) {
			CCrashDump::Crash();
		}
		//연결 끊기
		if (cbTransferred == 0) {
			//CCrashDump::Crash();
			DebugFunc(pSession, GQCS);
			_Disconnect(pSession);
		}
		//Recv, Send 동작
		else {
			//recv인 경우
			//g_RecvCnt++;
			if (Overlapped->Type == RECV) {
				DebugFunc(pSession, RECVCOM);
				pSession->iRecvbyte = cbTransferred;
				pSession->recvComSessionID = pSession->SessionID;
				//log
				pSession->recvComtime = timeGetTime();
				pSession->recvComTh = GetCurrentThreadId();
				//log
				//데이터 받아서 SendQ에 넣은 후 Send하기
				pSession->RecvQ.MoveRear(cbTransferred);
				while (1) {
					//헤더 사이즈 이상이 있는지 확인
					if (pSession->RecvQ.GetUseSize() < NETHEADER)
						break;
					CPacket::stPACKET_HEADER stPacketHeader;
					//데이터가 있는지 확인
					pSession->RecvQ.Peek((char*)&stPacketHeader, NETHEADER);
					//헤더 코드 검증
					if (stPacketHeader.byCode != dfPACKET_CODE) {
						_Disconnect(pSession);
						
						break;
					}
					if (stPacketHeader.shLen > dfMAX_PACKET_BUFFER_SIZE- NETHEADER) {
						_Disconnect(pSession);
					
						break;
					}
					if (pSession->RecvQ.GetUseSize() < int(NETHEADER + stPacketHeader.shLen))
						break;
					//pSession->RecvQ.MoveFront(5);
					CPacket* RecvPacket = CPacket::Alloc();
					int retval = pSession->RecvQ.Dequeue(RecvPacket->GetBufferPtr(), stPacketHeader.shLen + 5);
					RecvPacket->MoveWritePos(stPacketHeader.shLen);
					if (retval < stPacketHeader.shLen + NETHEADER) {
						_Disconnect(pSession);
					
						RecvPacket->Free();
						break;
					}
					//여기서 OnRecv호출 전에 Decode를 통해 데이터 변조 유무 체크
					RecvPacket->Decode();
					BYTE CheckSum = RecvPacket->GetCheckSum();
					if (CheckSum != (BYTE)*(RecvPacket->GetHeaderPtr() + 4)) {
						_Disconnect(pSession);
					
						RecvPacket->Free();
						break;
					}
					InterlockedIncrement(&_RecvTPS);
					OnRecv(pSession->SessionID, RecvPacket);
					RecvPacket->Free();
				}

				//Recv 요청하기
				RecvPost(pSession);

			}
			//send 완료 경우
			else if(Overlapped->Type == SEND) {
				//log
				DebugFunc(pSession, SENDCOM);
				pSession->sendComtime = timeGetTime();
				pSession->sendComTh = GetCurrentThreadId();
				pSession->iSendbyte = cbTransferred;
				pSession->sendComSessionID = pSession->SessionID;
				//log
				for (int i = 0; i < pSession->PacketCount; i++) {
					pSession->PacketArray[i]->Free();
				}
				pSession->PacketCount = 0;
				InterlockedExchange(&(pSession->SendFlag), TRUE);
				if (pSession->SendQ.Size() > 0) {
					SendPost(pSession);
				}
			}
			else if (Overlapped->Type == UPDATE) {
				DebugFunc(pSession, UPDATECOM);
				//여기선 IOCount를 줄일 필요가 없다
				//CCrashDump::Crash();
				pSession->updateComtime = timeGetTime();
				pSession->updateComSessionID = pSession->SessionID;
				pSession->updateComTh = GetCurrentThreadId();
				SendPost(pSession);
			}
		}
		ReleaseSession(pSession);
	}
}

unsigned int WINAPI CNetServer::AcceptThread(LPVOID lParam) {
	while (1) {
		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		memset(&clientaddr, 0, sizeof(clientaddr));

		SOCKET client_sock = accept(_Listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			wprintf(L"accept() %d\n", WSAGetLastError());
			return 0;
		}
		_AcceptCount++;
		_AcceptTPS++;
		OnConnectRequest(clientaddr);
		//빈 세션이 없을 경우 연결 끊기
		if (_IndexSession.Size()==0) {
			wprintf(L"accept()_MaxSession\n");
			LINGER optval;
			optval.l_linger = 0;
			optval.l_onoff = 1;
			setsockopt(client_sock, SOL_SOCKET, SO_LINGER, (char*)&optval, sizeof(optval));
			closesocket(client_sock);
			continue;
		}
		
		int curIdx;
		//_IndexSession.Pop(&curIdx);
		_IndexSession.Dequeue(&curIdx);
		_SessionList[curIdx].sock = client_sock;
		_SessionList[curIdx].clientaddr = clientaddr;
		_SessionList[curIdx].SessionID = ++_SessionIDCnt;
		_SessionList[curIdx].SessionIndex = curIdx;
		_SessionList[curIdx].SessionID |= (_SessionList[curIdx].SessionIndex << 48);
		memset(&_SessionList[curIdx].RecvOverlapped, 0, sizeof(_SessionList[curIdx].RecvOverlapped));
		memset(&_SessionList[curIdx].SendOverlapped, 0, sizeof(_SessionList[curIdx].SendOverlapped));
		memset(&_SessionList[curIdx].UpdateOverlapped, 0, sizeof(_SessionList[curIdx].UpdateOverlapped));
		_SessionList[curIdx].RecvOverlapped.Type = RECV;
		_SessionList[curIdx].SendOverlapped.Type = SEND;
		_SessionList[curIdx].UpdateOverlapped.Type = UPDATE;
		_SessionList[curIdx].RecvOverlapped.SessionID = _SessionList[curIdx].SessionID;
		_SessionList[curIdx].SendOverlapped.SessionID = _SessionList[curIdx].SessionID;
		_SessionList[curIdx].UpdateOverlapped.SessionID = _SessionList[curIdx].SessionID;
		_SessionList[curIdx].IOCount = 1;
		_SessionList[curIdx].RecvQ.ClearBuffer();
		_SessionList[curIdx].SendQ.Clear();
		_SessionList[curIdx].ReleaseFlag = TRUE;
		_SessionList[curIdx].SendFlag = TRUE;
		_SessionList[curIdx].PacketCount = 0;
		//log
		_SessionList[curIdx].iRecvbyte = 0;
		_SessionList[curIdx].iSendbyte = 0;
		_SessionList[curIdx].recvComSessionID = 0;
		_SessionList[curIdx].sendComSessionID = 0;
		_SessionList[curIdx].updateComSessionID = 0;
		_SessionList[curIdx].releaseSessionID = 0;
		_SessionList[curIdx].recvret = 0x11223344;
		_SessionList[curIdx].sendret = 0x11223344;
		_SessionList[curIdx].sendtime = 0;
		_SessionList[curIdx].recvtime = 0;
		_SessionList[curIdx].Distime = 0;
		_SessionList[curIdx].sendComtime = 0;
		_SessionList[curIdx].recvComtime = 0;
		_SessionList[curIdx].updateComtime = 0;
		_SessionList[curIdx].sendsock = NULL;
		_SessionList[curIdx].recvsock = NULL;
		_SessionList[curIdx].sendTh = 0;
		_SessionList[curIdx].recvTh = 0;
		_SessionList[curIdx].DisTh = 0;
		_SessionList[curIdx].recvComTh = 0;
		_SessionList[curIdx].sendComTh = 0;
		_SessionList[curIdx].updateComTh = 0;
		_SessionList[curIdx].ReleaseTh = 0;
		_SessionList[curIdx].DisIO = -1;
		_SessionList[curIdx].PQCSSessionID = 0;
		_SessionList[curIdx].sendErr = 0;
		_SessionList[curIdx].recvErr = 0;
		_SessionList[curIdx].PQCSCnt = 0;
		_SessionList[curIdx].recvLen[0] = 0;
		_SessionList[curIdx].recvLen[1] = 0;
		_SessionList[curIdx].debugCnt = 0;
		DebugFunc(&_SessionList[curIdx], ACCEPT);
		//log
		InterlockedIncrement(&_SessionCnt);
		WCHAR szClientIP[16];
		InetNtop(AF_INET, &_SessionList[curIdx].clientaddr.sin_addr, szClientIP, 16);
		//wprintf(L"Accept session %s:%d\n", szClientIP, ntohs(pSession->clientaddr.sin_port));
		CreateIoCompletionPort((HANDLE)_SessionList[curIdx].sock, _hcp, (ULONG_PTR)&_SessionList[curIdx], 0);
		OnClientJoin(_SessionList[curIdx].clientaddr, _SessionList[curIdx].SessionID);
		RecvPost(&_SessionList[curIdx]);

		ReleaseSession(&_SessionList[curIdx]);

	}
}

bool CNetServer::Start(ULONG OpenIP, USHORT Port, int NumWorkerthread, int NumIOCP, int MaxSession, 
	int iBlockNum, bool bPlacementNew, bool Restart) {
	int retval;
	if (!Restart) {
		_wsetlocale(LC_ALL, L"Korean");
		timeBeginPeriod(1);
		WSADATA wsa;
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
			wprintf(L"WSAStartup() %d\n", WSAGetLastError());
			return false;
		}

		//IOCP 생성
		_hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, NumIOCP);
		if (_hcp == NULL) {
			wprintf(L"CreateIoCompletionPort() %d\n", GetLastError());
			return 1;
		}

		_MaxSession = MaxSession;
		_SessionList = new stSESSION[MaxSession];
		for (int i = 0; i < _MaxSession; i++) {
			_SessionList[i].SessionID = -1;
		}

		for (int i = _MaxSession - 1; i >= 0; i--)
			_IndexSession.Enqueue(i);
			//_IndexSession.Push(i);

		//PacketBuffer 초기화
		CPacket::Initial(iBlockNum, bPlacementNew);
	}
	_Listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (_Listen_sock == INVALID_SOCKET) {
		wprintf(L"socket() %d\n", WSAGetLastError());
		return false;
	}


	/*int optval = 0;
	setsockopt(_Listen_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optval));*/

	SOCKADDR_IN serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(OpenIP);
	serveraddr.sin_port = htons(Port);

	retval = bind(_Listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) {
		wprintf(L"bind() %d\n", WSAGetLastError());
		return false;
	}

	retval = listen(_Listen_sock, SOMAXCONN_HINT(2048));
	if (retval == SOCKET_ERROR) {
		wprintf(L"listen() %d\n", WSAGetLastError()); 
	}
	_NumThread = NumWorkerthread;
	_hWorkerThread = new HANDLE[NumWorkerthread];
	for (int i = 0; i < NumWorkerthread; i++) {
		_hWorkerThread[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThreadFunc, this, 0, NULL);
		if (_hWorkerThread[i] == NULL) return 1;
	}
	_hAcceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThreadFunc, this, 0, NULL);
}

void CNetServer::Stop() {
	for (int i = 0; i < _NumThread; i++)
		PostQueuedCompletionStatus(_hcp, 0, NULL, NULL);
	closesocket(_Listen_sock);
	for (int i = 0; i < _NumThread; i++)
		CloseHandle(_hWorkerThread[i]);
	CloseHandle(_hAcceptThread);

}

int CNetServer::GetClientCount() {
	return _SessionCnt;
}

bool CNetServer::_Disconnect(stSESSION* pSession) {
	//CCrashDump::Crash();
	DebugFunc(pSession, DIS);
	int retval = InterlockedExchange(&pSession->sock, INVALID_SOCKET);
	if (retval != INVALID_SOCKET) {
		pSession->closeSock = retval;
		CancelIoEx((HANDLE)pSession->closeSock, NULL);
		//log
		pSession->Distime = timeGetTime();
		pSession->DisTh = GetCurrentThreadId();
		//log
		InterlockedIncrement((LONG*)&_DisCount);
		return true;
	}
	return false;
}


bool CNetServer::Disconnect(INT64 SessionID) {
	//CCrashDump::Crash();
	stSESSION* pSession = FindSession(SessionID);
	if (pSession == NULL)
		return false;
	int retval = InterlockedExchange(&pSession->sock, INVALID_SOCKET);
	if (retval != INVALID_SOCKET) {
		pSession->closeSock = retval;
		CancelIoEx((HANDLE)pSession->closeSock, NULL);
		pSession->DisIO = GetLastError();
		InterlockedIncrement((LONG*)&_DisCount);
		//log
		pSession->Distime = timeGetTime();
		pSession->DisTh = GetCurrentThreadId();
		//log
		ReleaseSession(pSession);
		return true;
	}
	ReleaseSession(pSession);
	return true;
}

bool CNetServer::_SendPacket(stSESSION* pSession, CPacket* pSendPacket, int type) {
	pSendPacket->AddRef();
	if (type == LAN)
		pSendPacket->SetHeader_2();
	else if (type == NET) {
		pSendPacket->SetHeader_5();
		pSendPacket->Encode();
	}
	pSession->SendQ.Enqueue(pSendPacket);
	SendPost(pSession);
	return true;
}

bool CNetServer::SendPacket(INT64 SessionID, CPacket* pSendPacket, int type, bool post) {
	stSESSION* pSession = FindSession(SessionID);
	if (pSession == NULL)
		return false;
	pSendPacket->AddRef();
	if (type == LAN)
		pSendPacket->SetHeader_2();
	else if (type == NET) {
		pSendPacket->SetHeader_5();
		pSendPacket->Encode();
	}
	pSession->SendQ.Enqueue(pSendPacket);
	//SendPost를 Workerthread로 넘기기
	if (post == true) {
		InterlockedIncrement64(&pSession->IOCount);
		pSession->PQCSSessionID = pSession->SessionID;
		pSession->PQCSCnt++;
		DebugFunc(pSession, PQCS);
		int retval = PostQueuedCompletionStatus(_hcp, sizeof(pSession), (ULONG_PTR)pSession, (WSAOVERLAPPED*)&pSession->UpdateOverlapped);
		if (retval == 0)
			CCrashDump::Crash();
	}
	else
		SendPost(pSession);
	ReleaseSession(pSession);
	return true;
}

stSESSION* CNetServer::FindSession(INT64 SessionID) {
	stSESSION* pSession = NULL;
	WORD SessionIdx = SessionID >> 48;
	if (_SessionList[SessionIdx].SessionID == SessionID) {
		pSession = &_SessionList[SessionIdx];
	}
	if (pSession == NULL)
		return pSession;
	int retval = InterlockedIncrement64(&pSession->IOCount);
	if (retval == 1) {
		ReleaseSession(pSession);
	}
	else {
		if (pSession->SessionID != SessionID) {
			ReleaseSession(pSession);
		}
		else {
			return pSession;
		}
	}
	return NULL;
	
}


void CNetServer::ReleaseSession(stSESSION* pSession) {
	int retval = InterlockedDecrement64(&pSession->IOCount);
	if (retval == 0) {
		if (pSession->ReleaseFlag == TRUE)
			Release(pSession);
	}
	else if (retval < 0) {
		CCrashDump::Crash();
	}
}

void CNetServer::Release(stSESSION* pSession) {
	stRELEASE temp;
	INT64 SessionID = pSession->SessionID;
	if (InterlockedCompareExchange128(&pSession->IOCount, (LONG64)FALSE, (LONG64)0, (LONG64*)&temp)) {
		DebugFunc(pSession, RELEASE);
		pSession->releaseSessionID = pSession->SessionID;
		pSession->SessionID = -1;
		InterlockedDecrement(&_SessionCnt);
		while (pSession->SendQ.Size() > 0 || pSession->PacketCount>0) {
			while (pSession->SendQ.Size() > 0) {
				if (pSession->PacketCount >= dfPACKETNUM)
					break;
				pSession->SendQ.Dequeue(&(pSession->PacketArray[pSession->PacketCount]));
				if (pSession->PacketArray[pSession->PacketCount] != NULL)
					pSession->PacketCount++;
			}
			for (int j = 0; j < pSession->PacketCount; j++) {
				pSession->PacketArray[j]->Free();
			}
			pSession->PacketCount = 0;
		}
		LINGER optval;
		optval.l_linger = 0;
		optval.l_onoff = 1;
		setsockopt(pSession->closeSock, SOL_SOCKET, SO_LINGER, (char*)&optval, sizeof(optval));
		closesocket(pSession->closeSock);
		_IndexSession.Enqueue(pSession->SessionIndex);
		//_IndexSession.Push(pSession->SessionIndex);
		OnClientLeave(SessionID);
		//log
		pSession->ReleaseTh = GetCurrentThreadId();
	}
	
}

void CNetServer::RecvPost(stSESSION* pSession) {
	memset(&pSession->RecvOverlapped, 0, sizeof(pSession->RecvOverlapped.Overlapped));
	DWORD recvbyte = 0;
	DWORD lpFlags = 0;
	DebugFunc(pSession, RECVPOST);
	//Recv 요청하기
	if (pSession->RecvQ.GetFrontBufferPtr() <= pSession->RecvQ.GetRearBufferPtr() && (pSession->RecvQ.GetRearBufferPtr() != pSession->RecvQ.GetBufferPtr())) {
		WSABUF recvbuf[2];
		recvbuf[0].buf = pSession->RecvQ.GetRearBufferPtr();
		recvbuf[0].len = pSession->RecvQ.DirectEnqueueSize();
		recvbuf[1].buf = pSession->RecvQ.GetBufferPtr();
		recvbuf[1].len = pSession->RecvQ.GetFreeSize() - pSession->RecvQ.DirectEnqueueSize();
		pSession->recvLen[0] = recvbuf[0].len;
		pSession->recvLen[1] = recvbuf[1].len;
		InterlockedIncrement64(&(pSession->IOCount));
		pSession->recvsock = pSession->sock;
		int retval = WSARecv(pSession->sock, recvbuf, 2, &recvbyte, &lpFlags, (WSAOVERLAPPED*)&pSession->RecvOverlapped, NULL);
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				int WSA = WSAGetLastError();
				pSession->recvErr = WSA;
				_Disconnect(pSession);
				ReleaseSession(pSession);
			}
		}
		//log
		pSession->recvret = WSAGetLastError();

	}
	else {
		WSABUF recvbuf;
		recvbuf.buf = pSession->RecvQ.GetRearBufferPtr();
		recvbuf.len = pSession->RecvQ.DirectEnqueueSize();
		pSession->recvLen[0] = recvbuf.len;
		InterlockedIncrement64(&(pSession->IOCount));
		pSession->recvsock = pSession->sock;
		int retval = WSARecv(pSession->sock, &recvbuf, 1, &recvbyte, &lpFlags, (WSAOVERLAPPED*)&pSession->RecvOverlapped, NULL);
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				int WSA = WSAGetLastError();
				pSession->recvErr = WSA;
				_Disconnect(pSession);
				ReleaseSession(pSession);
			}
		}
		//log
		pSession->recvret = WSAGetLastError();
	}
	//log
	//if (pSession->sock == INVALID_SOCKET)
	//	CancelIo((HANDLE)pSession->closeSock);
	//else {
		pSession->recvtime = timeGetTime();
		pSession->recvTh = GetCurrentThreadId();
		//InterlockedIncrement(&pSession->DisIO);
	//}
}

void CNetServer::SendPost(stSESSION* pSession) {
	int retval = InterlockedExchange(&(pSession->SendFlag), FALSE);
	if (retval == FALSE)
		return;
	if (pSession->SendQ.Size() <= 0) {
		InterlockedExchange(&(pSession->SendFlag), TRUE);
		return;
	}
	DebugFunc(pSession, SENDPOST);
	memset(&pSession->SendOverlapped, 0, sizeof(pSession->SendOverlapped.Overlapped));
	DWORD sendbyte = 0;
	DWORD lpFlags = 0;
	WSABUF sendbuf[dfPACKETNUM];
	DWORD i = 0;
	while (pSession->SendQ.Size() > 0) {
		if (i >= dfPACKETNUM)
			break;
		pSession->SendQ.Dequeue(&(pSession->PacketArray[i]));
		sendbuf[i].buf = pSession->PacketArray[i]->GetHeaderPtr();
		sendbuf[i].len = pSession->PacketArray[i]->GetDataSize() + pSession->PacketArray[i]->GetHeaderSize();
		pSession->PacketCount++;
		i++;
	}
	InterlockedAdd(&_SendTPS, i);
	InterlockedIncrement64(&(pSession->IOCount));
	pSession->sendsock = pSession->sock;
	retval = WSASend(pSession->sock, sendbuf, pSession->PacketCount, &sendbyte, lpFlags, (WSAOVERLAPPED*)&pSession->SendOverlapped, NULL);
	if (retval == SOCKET_ERROR) {
		if (WSAGetLastError() != ERROR_IO_PENDING) {
			int WSA = WSAGetLastError();
			pSession->sendErr = WSA;
			_Disconnect(pSession);
			ReleaseSession(pSession);
		}
	}
	//log
	pSession->sendtime = timeGetTime();
	pSession->sendret = WSAGetLastError();
	pSession->sendTh = GetCurrentThreadId();
	//log
}

void CNetServer::DebugFunc(stSESSION* pSession, int FuncNum) {
	return;
	int idx = InterlockedIncrement(&pSession->debugCnt);
	idx %= DEBUGNUM;
	pSession->debug[idx].FuncNum = FuncNum;
	pSession->debug[idx].IOCount = pSession->IOCount;
	pSession->debug[idx].SessionID = pSession->SessionID;
	pSession->debug[idx].ThreadID = GetCurrentThreadId();
	pSession->debug[idx].Sock = pSession->sock;
	
}
