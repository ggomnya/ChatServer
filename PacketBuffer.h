#pragma once
#include <stdlib.h>
#include <windows.h>
#include <memory.h>
#include "TLS_ObjectPool.h"
#include "CCrashDump.h"

extern INT64 PacketNum;

#define dfMAX_PACKET_BUFFER_SIZE	1000
#define dfPACKET_CODE		0x77
#define dfPACKET_KEY		0x32

class CPacket {
protected:

#pragma pack(push, 1)
	struct stPACKET_HEADER {
		BYTE byCode;
		WORD shLen;
		BYTE byRandKey;
		BYTE byCheckSum;
	};
#pragma pack(pop)
	char* _Buffer;
	int _iFront;
	int _iRear;
	int _iBufferSize;
	int _iDataSize;
	LONG _RefCount;
	bool _bEncodeFlag;
	bool _bHeaderFlag;
	int _CheckSum;
	int _iHeader;
	static TLS_CObjectPool<CPacket>* _ObjectPool;

public:
	friend class CNetServer;
	friend class CLanServer;
	friend class CLanClient;
	friend class TLS_CObjectPool<CPacket>;
	static void Initial(int iBlockNum, bool bPlacementNew = false) {
		if(_ObjectPool == NULL)
			_ObjectPool = new TLS_CObjectPool<CPacket>(iBlockNum, bPlacementNew);
	}

	static CPacket* Alloc() {

		CPacket* p = _ObjectPool->Alloc();
		p->Clear();
		p->AddRef();
		return p;
	}

	static LONG64 GetAllocCount() {
		return _ObjectPool->GetAllocCount();
	}

	static LONG64 GetUseCount() {
		return _ObjectPool->GetUseCount();
	}
	void Free();

	void AddRef();

	class EX {
	public:
		char* _Buffer;
		EX(int iBufferSize);
		~EX();
	};
	friend EX;
	enum en_PACKET {
		eBUFFER_DEFAULT = 1000
	};

private: CPacket();
private: CPacket(int iBufferSize);
private: char* GetHeaderPtr();
public:
	virtual ~CPacket();
	void Release();
	void Clear();

	int GetBufferSize();
	int GetDataSize();
	int GetHeaderSize();

	char* GetBufferPtr();

	int MoveWritePos(int iSize);
	int MoveReadPos(int iSize);

	int GetData(char* chpDest, int iSize);
	int PutData(char* chpSrc, int iSrcSize);

	void SetHeader_2();
	void SetHeader_5();
	BYTE GetCheckSum();

	void Encode();
	void Decode();


	CPacket& operator << (BYTE byValue);
	CPacket& operator << (char chValue);

	CPacket& operator << (short shValue);
	CPacket& operator << (WORD wValue);

	CPacket& operator << (int iValue);
	CPacket& operator << (DWORD dwValue);
	CPacket& operator << (float fValue);

	CPacket& operator << (__int64 iValue);
	CPacket& operator << (double dValue);
	CPacket& operator << (UINT uiValue);
	CPacket& operator << (UINT64 ullValue);

	CPacket& operator >> (BYTE& byValue);
	CPacket& operator >> (char& chValue);

	CPacket& operator >> (short& shValue);
	CPacket& operator >> (WORD& wValue);

	CPacket& operator >> (int& iValue);
	CPacket& operator >> (DWORD& dwValue);
	CPacket& operator >> (float& fValue);

	CPacket& operator >> (__int64& iValue);
	CPacket& operator >> (double& dValue);
	CPacket& operator >> (UINT& uiValue);
	CPacket& operator >> (UINT64& ullValue);

};