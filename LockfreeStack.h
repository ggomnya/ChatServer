#pragma once
#include <Windows.h>
#include "Lockfree_ObjectPool.h"

template <class DATA>
class CLockfreeStack {
private:
	struct st_NODE {
		DATA _Data;
		st_NODE* Next;
	};

	struct st_TOP_NODE {
		__declspec(align(16))
		st_NODE* pTopNode;
		LONG64 lCounter;
	};

	st_TOP_NODE _pTop;
	volatile LONG64 _lSize;
	CObjectPool<st_NODE>* _Objectpool;
public:


	CLockfreeStack() {
		_pTop.pTopNode = NULL;
		_pTop.lCounter = 0;
		_lSize = 0;
		_Objectpool = new CObjectPool<st_NODE>(40000, false);
	}

	~CLockfreeStack() {
		while (_pTop.pTopNode != NULL) {
			st_NODE* temp = _pTop.pTopNode->Next;
			_Objectpool->Free(temp);
			_pTop.pTopNode = temp;
		}
		delete _Objectpool;
	}
	void Push(DATA Data) {
		st_TOP_NODE pNewTop;
		st_NODE* pNewNode = _Objectpool->Alloc();
		pNewNode->_Data = Data;
		st_NODE* pNextNode;
		do {
			pNewTop = _pTop;
			pNextNode =pNewTop.pTopNode;
			pNewNode->Next = pNextNode;
		} while (InterlockedCompareExchange64((LONG64*)&_pTop.pTopNode, (LONG64)pNewNode, (LONG64)pNewTop.pTopNode) != (LONG64)pNewTop.pTopNode);
		InterlockedIncrement64(&_lSize);
	}

	void Pop(DATA* Data) {
		st_TOP_NODE pPopTop;
		st_NODE* pNewTop;
		LONG64 lCounter;
		do {
			pPopTop = _pTop;
			if (pPopTop.pTopNode == NULL)
				return;
			pNewTop = pPopTop.pTopNode->Next;
			lCounter = pPopTop.lCounter + 1;
		} while (!InterlockedCompareExchange128((LONG64*)&_pTop.pTopNode, (LONG64)lCounter,(LONG64)pNewTop, (LONG64*)&pPopTop.pTopNode));
		InterlockedDecrement64(&_lSize);

		*Data = pPopTop.pTopNode->_Data;
		_Objectpool->Free(pPopTop.pTopNode);
	}

	LONG64 Size() {
		return _lSize;
	}

	LONG64 AllocCount() {
		return _Objectpool->GetAllocCount();
	}

	LONG64 UseCount() {
		return _Objectpool->GetUseCount();
	}
};