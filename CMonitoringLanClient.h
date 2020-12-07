#pragma once
#include "CLanClient.h"
#include "CommonProtocol.h"
#include "CommonStruct.h"
#include <Pdh.h>
#pragma comment(lib,"Pdh.lib")


class CMonitoringLanClient : public CLanClient {
public:
	CMonitoringLanClient() {
		PdhOpenQuery(NULL, NULL, &_processMemQuery);
		PdhAddCounter(_processMemQuery, L"\\Process(CChatServer)\\Private Bytes", NULL, &_processMemTotal);
	}

	void MPMonitorLogin(CPacket* pPacket, WORD Type, int ServerNo);
	void MPMonitorDataUpdate(CPacket* pPacket, WORD Type, BYTE DataType, int DataValue, int TimeStamp);
	void TransferData(BYTE DataType, int DataValue, int TimeStamp);

	virtual void OnEnterJoinServer(INT64 SessionID);
	virtual void OnLeaveServer(INT64 SessionID);
	virtual void OnRecv(INT64 SessionID, CPacket* pRecvPacket);
	//virtual void OnSend(INT64 SessionID, int SendSize) = 0;
	virtual void OnError(int errorcode, const WCHAR* Err);

private:
	PDH_HQUERY _processMemQuery;
	PDH_HCOUNTER _processMemTotal;
};