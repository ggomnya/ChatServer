#pragma once
#pragma comment(lib, "Kernel32.lib")
#include <new.h>
#include <stdlib.h>
#include <windows.h>


template <class DATA>
class CObjectPool {
private:
	struct st_BLOCK_NODE {
		st_BLOCK_NODE() {
			stpNextBlock = NULL;
			iCode = 0x11223344;
		}
		int iCode;
		st_BLOCK_NODE* stpNextBlock;
	};

	struct st_BLOCK_TOP_NODE {
		__declspec(align(16))
		st_BLOCK_NODE* pTopNode;
		LONG64 lCounter;
		st_BLOCK_TOP_NODE() {
			pTopNode = NULL;
			lCounter = 0;
		}
	};

	volatile DWORD _iAllocCount;
	volatile DWORD _iUseCount;
	bool _bPlacementNew;

public:
	CObjectPool(int iBlockNum = 0, bool bPlacementNew = false) {
		_iAllocCount = iBlockNum;
		_iUseCount = 0;
		_bPlacementNew = bPlacementNew;
		_pFreeNode = new st_BLOCK_TOP_NODE;
		st_BLOCK_NODE* temp;
		void* chBlock;
		if (iBlockNum == 0) return;
		if (bPlacementNew) {
			for (int i = 0; i < iBlockNum; i++) {
				chBlock = malloc(sizeof(DATA) + sizeof(st_BLOCK_NODE));
				temp = new (chBlock) st_BLOCK_NODE;

				if (i == 0) {
					_pFreeNode->pTopNode = temp;
					_pFreeNode->lCounter++;
				}
				else {
					temp->stpNextBlock = _pFreeNode->pTopNode;
					_pFreeNode->pTopNode = temp;
					_pFreeNode->lCounter++;
				}
			}
		}
		else {
			for (int i = 0; i < iBlockNum; i++) {
			chBlock = malloc(sizeof(DATA) + sizeof(st_BLOCK_NODE));
				temp = new (chBlock) st_BLOCK_NODE;
				new ((char*)chBlock + sizeof(st_BLOCK_NODE)) DATA;

				if (i == 0) {
					_pFreeNode->pTopNode = temp;
					_pFreeNode->lCounter++;
				}
				else {
					temp->stpNextBlock = _pFreeNode->pTopNode;
					_pFreeNode->pTopNode = temp;
					_pFreeNode->lCounter++;
				}
			}
		}
	}
	virtual ~CObjectPool() {
		if (_bPlacementNew) {
			while (_pFreeNode->pTopNode != NULL) {
				st_BLOCK_NODE* temp = _pFreeNode->pTopNode->stpNextBlock;
				free(_pFreeNode->pTopNode);
				_pFreeNode->pTopNode = temp;
			}
		}
		else {
			while (_pFreeNode->pTopNode != NULL) {
				st_BLOCK_NODE* temp = _pFreeNode->pTopNode->stpNextBlock;
				DATA* tempDATA = (DATA*)((char*)_pFreeNode->pTopNode + sizeof(st_BLOCK_NODE));
				tempDATA->~DATA();
				free(_pFreeNode->pTopNode);
				_pFreeNode->pTopNode = temp;
			}
		}
		
	}

	DATA* Alloc(void) {
		DATA* tempDATA;
		st_BLOCK_NODE* pNewTop;
		st_BLOCK_TOP_NODE pPopTop;
		LONG64 CheckSum;
		st_BLOCK_NODE* temp;
		do {
			pPopTop = *_pFreeNode;
			if (pPopTop.pTopNode == NULL) {
				void* chBlock = malloc(sizeof(DATA) + sizeof(st_BLOCK_NODE));
				temp = new (chBlock) st_BLOCK_NODE;

				tempDATA = new ((char*)chBlock + sizeof(st_BLOCK_NODE)) DATA;
				InterlockedIncrement(&_iAllocCount);
				InterlockedIncrement(&_iUseCount);
				return tempDATA;
			}
			pNewTop = pPopTop.pTopNode->stpNextBlock;
			CheckSum = pPopTop.lCounter + 1;

		} while (!InterlockedCompareExchange128((LONG64*)&_pFreeNode->pTopNode, (LONG64)CheckSum, (LONG64)pNewTop, (LONG64*)&pPopTop.pTopNode));

		if(_bPlacementNew) 
			tempDATA = new ((char*)pPopTop.pTopNode + sizeof(st_BLOCK_NODE)) DATA;
		else	
			tempDATA = (DATA*)((char*)pPopTop.pTopNode + sizeof(st_BLOCK_NODE));
		
		InterlockedIncrement(&_iUseCount);
		return tempDATA;
	}

	bool Free(DATA* pData) {
		st_BLOCK_NODE* temp = (st_BLOCK_NODE*)((char*)pData - sizeof(st_BLOCK_NODE));
		if (temp->iCode != 0x11223344) {
			return false;
		}
		st_BLOCK_TOP_NODE pNewTop;
		st_BLOCK_NODE* pNextNode;
		if (_bPlacementNew) {
			pData->~DATA();
		}
		do {
			pNewTop = *_pFreeNode;
			pNextNode = pNewTop.pTopNode;
			temp->stpNextBlock = pNextNode;
		} while (InterlockedCompareExchange64((LONG64*)&_pFreeNode->pTopNode, (LONG64)temp, (LONG64)pNewTop.pTopNode)!= (LONG64)pNewTop.pTopNode);
		InterlockedDecrement(&_iUseCount);
		return true;
	}

	int GetAllocCount(void) {
		return _iAllocCount;
	}

	int GetUseCount(void) {
		return _iUseCount;
	}

	st_BLOCK_TOP_NODE* _pFreeNode;
};

