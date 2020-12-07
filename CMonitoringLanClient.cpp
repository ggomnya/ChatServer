#include "CMonitoringLanClient.h"

void CMonitoringLanClient::MPMonitorLogin(CPacket* pPacket, WORD Type, int ServerNo) {
	*pPacket << Type << ServerNo;
}

void CMonitoringLanClient::MPMonitorDataUpdate(CPacket* pPacket, WORD Type, BYTE DataType, int DataValue, int TimeStamp) {
	*pPacket << Type << DataType << DataValue << TimeStamp;
}

void CMonitoringLanClient::TransferData(BYTE DataType, int DataValue, int TimeStamp) {
	if (DataType == dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM) {
		PDH_FMT_COUNTERVALUE counterVal;
		PdhCollectQueryData(_processMemQuery);
		PdhGetFormattedCounterValue(_processMemTotal, PDH_FMT_LARGE, NULL, &counterVal);
		DataValue = (counterVal.largeValue / (1000 * 1000));

	}

	CPacket* pSendPacket = CPacket::Alloc();
	MPMonitorDataUpdate(pSendPacket, en_PACKET_SS_MONITOR_DATA_UPDATE, DataType, DataValue, TimeStamp);
	SendPacket(pSendPacket);
	pSendPacket->Free();
}

void CMonitoringLanClient::OnEnterJoinServer(INT64 SessionID) {
	//서버에 내 정보 정송하기
	CPacket* pSendPacket = CPacket::Alloc();
	MPMonitorLogin(pSendPacket, en_PACKET_SS_MONITOR_LOGIN, CHAT);
	SendPacket(pSendPacket);
	pSendPacket->Free();
}

void CMonitoringLanClient::OnLeaveServer(INT64 SessionID) {
	ReConnect();
}

void CMonitoringLanClient::OnRecv(INT64 SessionID, CPacket* pRecvPacket) {
	return;
}

void CMonitoringLanClient::OnError(int errorcode, const WCHAR* Err) {
	return;
}