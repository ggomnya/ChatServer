#include "PacketBuffer.h"



TLS_CObjectPool<CPacket>* CPacket::_ObjectPool = NULL;

void CPacket::Free() {
	int retval = InterlockedDecrement(&_RefCount);
	//InterlockedDecrement64(&PacketNum);
	if (retval == 0) {
		_ObjectPool->Free(this);
	}
}

void CPacket::AddRef() {
	//InterlockedIncrement64(&PacketNum);
	InterlockedIncrement(&_RefCount);
}

CPacket::EX::EX(int iBufferSize) {
	_Buffer = (char*)malloc(iBufferSize);
}
CPacket::EX::~EX() {
	free(_Buffer);
}

CPacket::CPacket() {
	_iBufferSize = eBUFFER_DEFAULT;
	_iFront = 5;
	_iRear = 5;
	_iDataSize = 0;
	_Buffer = (char*)malloc(_iBufferSize);
	_RefCount = 0;
	_iHeader = 0;
	_bEncodeFlag = false;
	_bHeaderFlag = false;
}
CPacket::CPacket(int iBufferSize) {
	_iBufferSize = iBufferSize;
	_iFront = 5;
	_iRear = 5;
	_iDataSize = 0;
	_Buffer = (char*)malloc(_iBufferSize);
	_RefCount = 0;
	_iHeader = 0;
	_bEncodeFlag = false;
	_bHeaderFlag = false;
}

CPacket::~CPacket() {
	free(_Buffer);
}

void CPacket::Release() {

}
void CPacket::Clear() {
	_iFront = 5;
	_iRear = 5;
	_iDataSize = 0;
	_bEncodeFlag = false;
	_bHeaderFlag = false;
	_iHeader = 0;
	_CheckSum = 0;
}

int CPacket::GetBufferSize() {
	return _iBufferSize - 5;
}
int CPacket::GetDataSize() {
	return _iRear - _iFront;
}

int CPacket::GetHeaderSize() {
	if (_iHeader == 0)
		return 5;
	else return 2;
}

char* CPacket::GetHeaderPtr() {
	return _Buffer + _iHeader;
}

char* CPacket::GetBufferPtr() {
	return _Buffer;
}

int CPacket::MoveWritePos(int iSize) {
	//if (iSize < 0) return -1;
	int iTempSize = iSize;
	//if (_iBufferSize - _iRear < iTempSize) {
	//	iTempSize = _iBufferSize - _iRear;
	//}
	_iRear += iTempSize;
	_iDataSize += iTempSize;
	return iTempSize;
}
int CPacket::MoveReadPos(int iSize) {
	//if (iSize < 0) return -1;
	int iTempSize = iSize;
	//if (_iDataSize < iTempSize) {
	//	iTempSize = _iDataSize;
	//}
	_iFront += iTempSize;
	_iDataSize -= iTempSize;
	return iTempSize;
}

int CPacket::GetData(char* chpDest, int iSize) {
	int iTempSize = iSize;
	/*if (_iDataSize < iTempSize) {
		iTempSize = _iDataSize;
	}*/
	memcpy_s(chpDest, iTempSize, _Buffer + _iFront, iTempSize);
	_iFront += iTempSize;
	//_iDataSize -= iTempSize;
	return iTempSize;
}
int CPacket::PutData(char* chpSrc, int iSrcSize) {
	int iTempSize = iSrcSize;
	/*if (_iBufferSize - _iRear < iTempSize) {
		iTempSize = _iBufferSize - _iRear;
	}*/
	memcpy_s(_Buffer + _iRear, iTempSize, chpSrc, iTempSize);
	_iRear += iTempSize;
	//_iDataSize += iTempSize;
	return iTempSize;
}

void CPacket::SetHeader_2() {
	_iHeader = 3;
	_iDataSize = _iRear - _iFront;
	memcpy_s(_Buffer + _iHeader, 2, &_iDataSize, 2);
}

void CPacket::SetHeader_5() {
	if (_bHeaderFlag == true)
		return;
	_iHeader = 0;
	_iDataSize = _iRear - _iFront;
	stPACKET_HEADER stPacketHeader;
	stPacketHeader.byCode = dfPACKET_CODE;
	stPacketHeader.shLen = _iDataSize;
	stPacketHeader.byRandKey = rand() % 256;
	//stPacketHeader.byRandKey = 0x31;
	stPacketHeader.byCheckSum = GetCheckSum();
	
	memcpy_s(_Buffer + _iHeader, 5, &stPacketHeader, 5);
	_bHeaderFlag = true;
}

BYTE CPacket::GetCheckSum() {
	int temp = 0;
	int i;
	for (i = _iFront; i < _iRear; i++) {
		temp += (BYTE) * (_Buffer + i);
	}
	return temp % 256;
}

void CPacket::Encode() {
	if (_bEncodeFlag == true)
		return;
	BYTE byCode = dfPACKET_KEY;
	BYTE byRandKey = _Buffer[3];
	int idx = 4;
	BYTE P = _Buffer[idx] ^ (byRandKey + idx - 3);
	BYTE E = P ^ (byCode + idx - 3);
	_Buffer[idx] = E;
	idx++;
	while (idx != _iRear) {
		P = _Buffer[idx] ^ (P + byRandKey + idx - 3);
		E = P ^ (E + byCode + idx - 3);
		_Buffer[idx] = E;
		idx++;
	}
	_bEncodeFlag = true;
}

void CPacket::Decode() {
	BYTE byCode = dfPACKET_KEY;
	BYTE byRandKey = _Buffer[3];
	WORD idx = 4;
	BYTE E = _Buffer[idx];
	BYTE P = E ^ (byCode + idx - 3);
	BYTE tempE;
	BYTE tempP;
	BYTE tempD;
	_Buffer[idx] = P ^ (byRandKey + idx - 3);
	idx++;
	while (idx != _iRear) {
		tempE = _Buffer[idx];
		tempP = tempE ^ (E + byCode + idx - 3);
		tempD = tempP ^ (P + byRandKey + idx - 3);
		P = tempP;
		E = tempE;
		_CheckSum += tempD;
		_Buffer[idx] = tempD;
		idx++;
	}
	_CheckSum %= 256;
}


CPacket& CPacket::operator << (BYTE byValue) {
	int iTempSize = sizeof(BYTE);
	/*if (_iBufferSize - _iRear < iTempSize) {
		CPacket::EX* err = new CPacket::EX(_iBufferSize);
		memcpy_s(err->_Buffer, _iDataSize, _Buffer + _iFront, _iDataSize);
		throw(err);
	}*/
	memcpy_s(_Buffer + _iRear, iTempSize, &byValue, iTempSize);
	_iRear += iTempSize;
	//_iDataSize += iTempSize;
	return *this;

	
}
CPacket& CPacket::operator << (char chValue) {
	int iTempSize = sizeof(char);
	/*if (_iBufferSize - _iRear < iTempSize) {
		CPacket::EX* err = new CPacket::EX(_iBufferSize);
		memcpy_s(err->_Buffer, _iDataSize, _Buffer + _iFront, _iDataSize);
		throw(err);
	}*/
	memcpy_s(_Buffer + _iRear, iTempSize, &chValue, iTempSize);
	_iRear += iTempSize;
	//_iDataSize += iTempSize;
	return *this;
}

CPacket& CPacket::operator << (short shValue) {
	int iTempSize = sizeof(short);
	/*if (_iBufferSize - _iRear < iTempSize) {
		CPacket::EX* err = new CPacket::EX(_iBufferSize);
		memcpy_s(err->_Buffer, _iDataSize, _Buffer + _iFront, _iDataSize);
		throw(err);
	}*/
	memcpy_s(_Buffer + _iRear, iTempSize, &shValue, iTempSize);
	_iRear += iTempSize;
	//_iDataSize += iTempSize;
	return *this;
}
CPacket& CPacket::operator << (WORD wValue) {
	int iTempSize = sizeof(WORD);
	/*if (_iBufferSize - _iRear < iTempSize) {
		CPacket::EX* err = new CPacket::EX(_iBufferSize);
		memcpy_s(err->_Buffer, _iDataSize, _Buffer + _iFront, _iDataSize);
		throw(err);
	}*/
	memcpy_s(_Buffer + _iRear, iTempSize, &wValue, iTempSize);
	_iRear += iTempSize;
	//_iDataSize += iTempSize;
	return *this;
}

CPacket& CPacket::operator << (int iValue) {
	int iTempSize = sizeof(int);
	/*if (_iBufferSize - _iRear < iTempSize) {
		CPacket::EX* err = new CPacket::EX(_iBufferSize);
		memcpy_s(err->_Buffer, _iDataSize, _Buffer + _iFront, _iDataSize);
		throw(err);
	}*/
	memcpy_s(_Buffer + _iRear, iTempSize, &iValue, iTempSize);
	_iRear += iTempSize;
	//_iDataSize += iTempSize;
	return *this;
}
CPacket& CPacket::operator << (DWORD dwValue) {
	int iTempSize = sizeof(DWORD);
	/*if (_iBufferSize - _iRear < iTempSize) {
		CPacket::EX* err = new CPacket::EX(_iBufferSize);
		memcpy_s(err->_Buffer, _iDataSize, _Buffer + _iFront, _iDataSize);
		throw(err);
	}*/
	memcpy_s(_Buffer + _iRear, iTempSize, &dwValue, iTempSize);
	_iRear += iTempSize;
	//_iDataSize += iTempSize;
	return *this;
}
CPacket& CPacket::operator << (float fValue) {
	int iTempSize = sizeof(float);
	//if (_iBufferSize - _iRear < iTempSize) {
	//	CPacket::EX* err = new CPacket::EX(_iBufferSize);
	//	memcpy_s(err->_Buffer, _iDataSize, _Buffer + _iFront, _iDataSize);
	//	throw(err);
	//}
	memcpy_s(_Buffer + _iRear, iTempSize, &fValue, iTempSize);
	_iRear += iTempSize;
	//_iDataSize += iTempSize;
	return *this;
}

CPacket& CPacket::operator << (__int64 iValue) {
	int iTempSize = sizeof(__int64);
	/*if (_iBufferSize - _iRear < iTempSize) {
		CPacket::EX* err = new CPacket::EX(_iBufferSize);
		memcpy_s(err->_Buffer, _iDataSize, _Buffer + _iFront, _iDataSize);
		throw(err);
	}*/
	memcpy_s(_Buffer + _iRear, iTempSize, &iValue, iTempSize);
	_iRear += iTempSize;
	//_iDataSize += iTempSize;
	return *this;
}
CPacket& CPacket::operator << (double dValue) {
	int iTempSize = sizeof(double);
	/*if (_iBufferSize - _iRear < iTempSize) {
		CPacket::EX* err = new CPacket::EX(_iBufferSize);
		memcpy_s(err->_Buffer, _iDataSize, _Buffer + _iFront, _iDataSize);
		throw(err);
	}*/
	memcpy_s(_Buffer + _iRear, iTempSize, &dValue, iTempSize);
	_iRear += iTempSize;
	//_iDataSize += iTempSize;
	return *this;
}

CPacket& CPacket::operator << (UINT uiValue) {
	int iTempSize = sizeof(UINT);
	/*if (_iBufferSize - _iRear < iTempSize) {
		CPacket::EX* err = new CPacket::EX(_iBufferSize);
		memcpy_s(err->_Buffer, _iDataSize, _Buffer + _iFront, _iDataSize);
		throw(err);
	}*/
	memcpy_s(_Buffer + _iRear, iTempSize, &uiValue, iTempSize);
	_iRear += iTempSize;
	//_iDataSize += iTempSize;
	return *this;
}

CPacket& CPacket::operator << (UINT64 ullValue) {
	int iTempSize = sizeof(UINT64);
	/*if (_iBufferSize - _iRear < iTempSize) {
		CPacket::EX* err = new CPacket::EX(_iBufferSize);
		memcpy_s(err->_Buffer, _iDataSize, _Buffer + _iFront, _iDataSize);
		throw(err);
	}*/
	memcpy_s(_Buffer + _iRear, iTempSize, &ullValue, iTempSize);
	_iRear += iTempSize;
	//_iDataSize += iTempSize;
	return *this;
}

CPacket& CPacket::operator >> (BYTE& byValue) {
	int iTempSize = sizeof(BYTE);
	/*if (_iDataSize < iTempSize) {
		CPacket::EX* err = new CPacket::EX(_iBufferSize);
		memcpy_s(err->_Buffer, _iDataSize, _Buffer + _iFront, _iDataSize);
		throw(err);
	}*/
	memcpy_s(&byValue, iTempSize, _Buffer + _iFront, iTempSize);
	_iFront += iTempSize;
	//_iDataSize -= iTempSize;
	return *this;
}
CPacket& CPacket::operator >> (char& chValue) {
	int iTempSize = sizeof(char);
	/*if (_iDataSize < iTempSize) {
		CPacket::EX* err = new CPacket::EX(_iBufferSize);
		memcpy_s(err->_Buffer, _iDataSize, _Buffer + _iFront, _iDataSize);
		throw(err);
	}*/
	memcpy_s(&chValue, iTempSize, _Buffer + _iFront, iTempSize);
	_iFront += iTempSize;
	//_iDataSize -= iTempSize;
	return *this;
}

CPacket& CPacket::operator >> (short& shValue) {
	int iTempSize = sizeof(short);
	/*if (_iDataSize < iTempSize) {
		CPacket::EX* err = new CPacket::EX(_iBufferSize);
		memcpy_s(err->_Buffer, _iDataSize, _Buffer + _iFront, _iDataSize);
		throw(err);
	}*/
	memcpy_s(&shValue, iTempSize, _Buffer + _iFront, iTempSize);
	_iFront += iTempSize;
	//_iDataSize -= iTempSize;
	return *this;
}
CPacket& CPacket::operator >> (WORD& wValue) {
	int iTempSize = sizeof(WORD);
	/*if (_iDataSize < iTempSize) {
		CPacket::EX* err = new CPacket::EX(_iBufferSize);
		memcpy_s(err->_Buffer, _iDataSize, _Buffer + _iFront, _iDataSize);
		throw(err);
	}*/
	memcpy_s(&wValue, iTempSize, _Buffer + _iFront, iTempSize);
	_iFront += iTempSize;
	//_iDataSize -= iTempSize;
	return *this;
}

CPacket& CPacket::operator >> (int& iValue) {
	int iTempSize = sizeof(int);
	/*if (_iDataSize < iTempSize) {
		CPacket::EX* err = new CPacket::EX(_iBufferSize);
		memcpy_s(err->_Buffer, _iDataSize, _Buffer + _iFront, _iDataSize);
		throw(err);
	}*/
	memcpy_s(&iValue, iTempSize, _Buffer + _iFront, iTempSize);
	_iFront += iTempSize;
	//_iDataSize -= iTempSize;
	return *this;
}
CPacket& CPacket::operator >> (DWORD& dwValue) {
	int iTempSize = sizeof(DWORD);
	/*if (_iDataSize < iTempSize) {
		CPacket::EX* err = new CPacket::EX(_iBufferSize);
		memcpy_s(err->_Buffer, _iDataSize, _Buffer + _iFront, _iDataSize);
		throw(err);
	}*/
	memcpy_s(&dwValue, iTempSize, _Buffer + _iFront, iTempSize);
	_iFront += iTempSize;
	//_iDataSize -= iTempSize;
	return *this;
}
CPacket& CPacket::operator >> (float& fValue) {
	int iTempSize = sizeof(float);
	/*if (_iDataSize < iTempSize) {
		CPacket::EX* err = new CPacket::EX(_iBufferSize);
		memcpy_s(err->_Buffer, _iDataSize, _Buffer + _iFront, _iDataSize);
		throw(err);
	}*/
	memcpy_s(&fValue, iTempSize, _Buffer + _iFront, iTempSize);
	_iFront += iTempSize;
	//_iDataSize -= iTempSize;
	return *this;
}

CPacket& CPacket::operator >> (__int64& iValue) {
	int iTempSize = sizeof(__int64);
	/*if (_iDataSize < iTempSize) {
		CPacket::EX* err = new CPacket::EX(_iBufferSize);
		memcpy_s(err->_Buffer, _iDataSize, _Buffer + _iFront, _iDataSize);
		throw(err);
	}*/
	memcpy_s(&iValue, iTempSize, _Buffer + _iFront, iTempSize);
	_iFront += iTempSize;
	//_iDataSize -= iTempSize;
	return *this;
}
CPacket& CPacket::operator >> (double& dValue) {
	int iTempSize = sizeof(double);
	/*if (_iDataSize < iTempSize) {
		CPacket::EX* err = new CPacket::EX(_iBufferSize);
		memcpy_s(err->_Buffer, _iDataSize, _Buffer + _iFront, _iDataSize);
		throw(err);
	}*/
	memcpy_s(&dValue, iTempSize, _Buffer + _iFront, iTempSize);
	_iFront += iTempSize;
	//_iDataSize -= iTempSize;
	return *this;
}

CPacket& CPacket::operator >> (UINT& uiValue) {
	int iTempSize = sizeof(UINT);
	/*if (_iDataSize < iTempSize) {
		CPacket::EX* err = new CPacket::EX(_iBufferSize);
		memcpy_s(err->_Buffer, _iDataSize, _Buffer + _iFront, _iDataSize);
		throw(err);
	}*/
	memcpy_s(&uiValue, iTempSize, _Buffer + _iFront, iTempSize);
	_iFront += iTempSize;
	//_iDataSize -= iTempSize;
	return *this;
}

CPacket& CPacket::operator >> (UINT64& ullValue) {
	int iTempSize = sizeof(UINT64);
	/*if (_iDataSize < iTempSize) {
		CPacket::EX* err = new CPacket::EX(_iBufferSize);
		memcpy_s(err->_Buffer, _iDataSize, _Buffer + _iFront, _iDataSize);
		throw(err);
	}*/
	memcpy_s(&ullValue, iTempSize, _Buffer + _iFront, iTempSize);
	_iFront += iTempSize;
	//_iDataSize -= iTempSize;
	return *this;
}
