#include "RingBuffer_Lock.h"
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

void CRingBuffer::Initial(int iBufferSize) {
	_Buffer = (char*)malloc(iBufferSize);
	_iBufferSize = iBufferSize;
	_iFront = 0;
	_iRear = 0;
	InitializeSRWLock(&_srwLock);
}

CRingBuffer::CRingBuffer(int iBufferSize) {
	Initial(iBufferSize+1);

}

CRingBuffer::~CRingBuffer() {
	free(_Buffer);
}

void CRingBuffer::ReSize(int iSize) {
	free(_Buffer);
	_Buffer = (char*)malloc(iSize+1);
	_iBufferSize = iSize+1;
	_iFront = 0;
	_iRear = 0;
}

int CRingBuffer::GetBufferSize() {
	return _iBufferSize-1;
}

int CRingBuffer::GetUseSize() {

	return (_iBufferSize - _iFront + _iRear) % (_iBufferSize);

}

int CRingBuffer::GetFreeSize() {

	return (_iBufferSize - _iRear + _iFront - 1) % (_iBufferSize);
}

int CRingBuffer::DirectEnqueueSize() {
	if(_iFront <=_iRear)
		return _iBufferSize - _iRear - (_iFront == 0);
	else
		return _iFront - _iRear-1;
}

int CRingBuffer::DirectDequeueSize() {
	if (_iFront <= _iRear)
		return _iRear - _iFront;
	else
		return _iBufferSize - _iFront;
}

int CRingBuffer::Enqueue(char* chpData, int iSize) {
	int iTempSize = GetFreeSize();
	if (iTempSize > iSize)
		iTempSize = iSize;
	if (iTempSize + _iRear >= _iBufferSize) {
		int iTempSize2 = _iBufferSize - _iRear;
		memcpy_s((_Buffer + _iRear), iTempSize2, chpData, iTempSize2);
		memcpy_s((_Buffer), iTempSize -iTempSize2, chpData + iTempSize2, iTempSize - iTempSize2);
	}
	else 
		memcpy_s((_Buffer + _iRear), iTempSize, chpData, iTempSize);
	_iRear += iTempSize;
	_iRear %= _iBufferSize;

	return iTempSize;
	
}

int CRingBuffer::Dequeue(char* chpDest, int iSize) {
	int iTempSize = GetUseSize();
	if (iTempSize > iSize)
		iTempSize = iSize;

	if (iTempSize + _iFront >= _iBufferSize) {
		int iTempSize2 = _iBufferSize - _iFront;
		memcpy_s(chpDest,iTempSize2, (_Buffer + _iFront), iTempSize2);
		memcpy_s(chpDest + iTempSize2, iTempSize - iTempSize2, _Buffer, iTempSize - iTempSize2);
	}
	else
		memcpy_s(chpDest, iTempSize, (_Buffer + _iFront), iTempSize);
	_iFront += iTempSize;
	_iFront %= _iBufferSize;

	return iTempSize;
}

int CRingBuffer::Peek(char* chpDest, int iSize) {
	int iTempSize = GetUseSize();
	if (iTempSize > iSize)
		iTempSize = iSize;

	if (iTempSize + _iFront >= _iBufferSize) {
		int iTempSize2 = _iBufferSize - _iFront;
		memcpy_s(chpDest, iTempSize2, (_Buffer + _iFront), iTempSize2);
		memcpy_s(chpDest + iTempSize2, iTempSize - iTempSize2, _Buffer, iTempSize - iTempSize2);
	}
	else
		memcpy_s(chpDest, iTempSize, (_Buffer + _iFront), iTempSize);
	return iTempSize;
}

void CRingBuffer::MoveRear(int iSize) {
	_iRear += iSize;
	_iRear %= _iBufferSize;

}

void CRingBuffer::MoveFront(int iSize) {

	_iFront += iSize;
	_iFront %= _iBufferSize;
}

void CRingBuffer::ClearBuffer() {
	_iFront = 0;
	_iRear = 0;
}

char* CRingBuffer::GetFrontBufferPtr() {
	return (_Buffer + _iFront);
}

char* CRingBuffer::GetRearBufferPtr() {
	return (_Buffer + _iRear);
}

char* CRingBuffer::GetBufferPtr() {
	return _Buffer;
}

void CRingBuffer::Lock() {
	AcquireSRWLockExclusive(&_srwLock);
}

void CRingBuffer::Unlock() {
	ReleaseSRWLockExclusive(&_srwLock);
}

/*
int main() {
	srand(150);
	CRingBuffer buf(4);
	const char* sentence = "abcdefghijklmnopqrstuvwxyz 1234567890 abcdefghijklmnopqrstuvwxyz 1234567890 abcde";
	int iTotalSize = 81;
	char result[100];
	char temp[100];
	int ptr = 0;
	int EnqSize = 0;
	int DeqSize = 0;
	int PeekSize = 0;
	int iCount = 0;
	while (1) {
		//EnqSize = 15;
		EnqSize = rand() % 82;
		if (ptr + EnqSize > iTotalSize) {
			EnqSize = iTotalSize - ptr;
		}
		EnqSize = buf.Enqueue((char*)(sentence + ptr), EnqSize);
		ptr += EnqSize;
		ptr %= iTotalSize;
		
		PeekSize = buf.Peek(temp, EnqSize);
		if (EnqSize != PeekSize) {
			printf("EnqSize != PeekSize\n");
			return iCount;
		}
		
		DeqSize = buf.Dequeue(result, EnqSize);
		if (EnqSize != DeqSize) {
			printf("EnqSize != DeqSize\n");
			return iCount;
		}

		if (memcmp(result, temp, EnqSize) != 0) {
			printf("Peek != Dequeue\n");
			return iCount;
		}
		
		result[EnqSize] = '\0';
		printf("%s", result);
		iCount++;
	}
	return 0;
}
*/