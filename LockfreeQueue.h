#pragma once
#include <Windows.h>
#include "Lockfree_ObjectPool.h"
#include "LockfreeStack.h"

template <class DATA>
class CLockfreeQueue {
	struct st_NODE {
		DATA _Data;
		st_NODE* Next;
		st_NODE() {
			_Data = NULL;
			Next = NULL;
		}
	};

	struct st_DUMMY_NODE {
		__declspec(align(16))
		st_NODE* pNode;
		LONG64 Counter;
	};

	struct st_STACK {
		LONG64 ptr;
		LONG64 Info;
		LONG64 head;
		LONG64 tail;
	};

	st_DUMMY_NODE _head;
	st_DUMMY_NODE _tail;
	LONG64 _lSize;
	CObjectPool<st_NODE>* _Objectpool;
	//CLockfreeStack<st_STACK> _Stack;
public:
	CLockfreeQueue() {
		_Objectpool = new CObjectPool<st_NODE>(0, false);
		_lSize = 0;
		_head.Counter = 0;
		_head.pNode = _Objectpool->Alloc();
		_head.pNode->Next = NULL;
		_tail = _head;
	}

	~CLockfreeQueue() {
		while (_head.pNode != NULL) {
			st_NODE* temp = _head.pNode->Next;
			_Objectpool->Free(_head.pNode);
			_head.pNode = temp;
		}
		delete _Objectpool;
	}

	void Enqueue(DATA Data) {
		st_NODE* newNode = _Objectpool->Alloc();
		newNode->_Data = Data;
		newNode->Next = NULL;
		st_DUMMY_NODE tail;
		st_NODE* pNextNode;
		LONG64 Counter;
		while (1) {
			tail = _tail;
			pNextNode = tail.pNode->Next;
			Counter = tail.Counter + 1;
			/*st_STACK temp;
			temp.ptr = (LONG64)newNode;
			temp.Info = 1;
			temp.tail = (LONG64)tail.pNode;*/

			if (pNextNode == NULL) {
				if (InterlockedCompareExchange64((LONG64*)&tail.pNode->Next, (LONG64)newNode, (LONG64)pNextNode) == (LONG64)pNextNode) {
					//_Stack.Push(temp);
					InterlockedCompareExchange128((LONG64*)&_tail.pNode, (LONG64)Counter, (LONG64)newNode, (LONG64*)&tail.pNode);
					break;
				}
			}
			else {
				InterlockedCompareExchange128((LONG64*)&_tail.pNode, (LONG64)Counter, (LONG64)pNextNode, (LONG64*)&tail.pNode);
			}
		}
		InterlockedIncrement64(&_lSize);
	}

	void Dequeue(DATA* Data) {
		/*LONG64 retval = InterlockedDecrement64(&_lSize);
		if (retval < 0) {
			InterlockedIncrement64(&_lSize);
			return;
		}*/
		st_DUMMY_NODE head;
		st_NODE* pNewHead;
		LONG64 Counter;
		st_DUMMY_NODE tail;
		while (1) {
			head = _head;
			pNewHead = head.pNode->Next;
			if (pNewHead == NULL) {
				Data = NULL;
				return;
			}

			Counter = head.Counter + 1;
			*Data = pNewHead->_Data;
			/*st_STACK temp;
			temp.ptr = (LONG64)pNewHead;
			temp.Info = 2;
			temp.head = (LONG64)head.pNode;*/
			if (InterlockedCompareExchange128((LONG64*)&_head.pNode, (LONG64)Counter, (LONG64)pNewHead, (LONG64*)&head.pNode)) {
				//_Stack.Push(temp);
				//head에서 tail을 앞지르는 상황 만들지 않기
				tail = _tail;
				Counter = tail.Counter + 1;
				if (head.pNode == tail.pNode) {
					InterlockedCompareExchange128((LONG64*)&_tail.pNode, (LONG64)Counter, (LONG64)head.pNode->Next, (LONG64*)&tail.pNode);
				}
				_Objectpool->Free(head.pNode);
				break;
			}
		}
		InterlockedDecrement64(&_lSize);
	}
	
	void Clear() {
		while (_lSize!=0) {
			DATA Data;
			CCrashDump::Crash();
			Dequeue(&Data);
			if (Data != NULL)
				Data->Free();
		}
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