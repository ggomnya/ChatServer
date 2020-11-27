#pragma once
#include <new.h>
#include <stdlib.h>
#include "Lockfree_ObjectPool.h"


template <class DATA>
class TLS_CObjectPool {
private:
	#define MAXNUM	1000
	struct st_DATA;
	//데이터 뒤에 포인터를 붙여 포인터가 st_DATA의 앞을 가르키게 만들어 free할 때
	//ObjectPool에 st_DATA 구조체를 반납할 수 있도록 한다.
	struct st_DATA_INFO {
		DATA Data;
		st_DATA* ptr;
	};
	struct st_DATA {
		st_DATA_INFO _Data[MAXNUM];
		LONG _iAllocCount;
		LONG _iFreeCount;
		st_DATA() {
			for (int i = 0; i < MAXNUM; i++) {
				_Data[i].ptr = (st_DATA*)this;
			}
			_iAllocCount = 0;
			_iFreeCount = 0;
		}
	};

	LONG _TlsIndex;
	CObjectPool<st_DATA>* _ObjectPool;
public:
	TLS_CObjectPool(int iBlockNum = 0, bool bPlacementNew = false) {
		_ObjectPool = new CObjectPool<st_DATA>(iBlockNum, bPlacementNew);
		_TlsIndex = TlsAlloc();
		if (_TlsIndex == TLS_OUT_OF_INDEXES) {
			CCrashDump::Crash();
		}
	}
	virtual ~TLS_CObjectPool() {
		delete _ObjectPool;
	}

	DATA* Alloc(void) {
		st_DATA* DataBundle = (st_DATA*)TlsGetValue(_TlsIndex);
		if (DataBundle == NULL) {
			DataBundle = _ObjectPool->Alloc();
			TlsSetValue(_TlsIndex, DataBundle);
		}
		DATA* tempDATA = &DataBundle->_Data[DataBundle->_iAllocCount++].Data;
		if (DataBundle->_iAllocCount == MAXNUM) {
			TlsSetValue(_TlsIndex, NULL);
		}
		return tempDATA;
	}

	void Free(DATA* pData) {
		st_DATA* FreeData = ((st_DATA_INFO*)pData)->ptr;
		int retval = InterlockedIncrement(&FreeData->_iFreeCount);
		if (retval == MAXNUM) {
			FreeData->_iAllocCount = 0;
			FreeData->_iFreeCount = 0;
			_ObjectPool->Free(FreeData);
		}
	}

	int GetAllocCount() {
		return _ObjectPool->GetAllocCount();
	}

	int GetUseCount() {
		return _ObjectPool->GetUseCount();
	}
};

