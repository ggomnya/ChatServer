#pragma once
#include <Windows.h>
#define dfMAX_RINGBUFFER_SIZE	15000



class CRingBuffer {
private:

	char* _Buffer;
	int _iBufferSize;
	int _iFront;
	int _iRear;
	SRWLOCK _srwLock;

	void Initial(int iBufferSize);

public:
	CRingBuffer(int iBufferSize = dfMAX_RINGBUFFER_SIZE);
	~CRingBuffer();
	void ReSize(int iSize);

	int GetBufferSize();
	int GetUseSize();
	int GetFreeSize();

	int DirectEnqueueSize();
	int DirectDequeueSize();

	int Enqueue(char* chpData, int iSize);
	int Dequeue(char* chpDest, int iSize);

	int Peek(char* chpDest, int iSize);

	void MoveRear(int iSize);
	void MoveFront(int iSize);

	void ClearBuffer();

	char* GetFrontBufferPtr();
	char* GetRearBufferPtr();
	char* GetBufferPtr();

	void Lock();
	void Unlock();

};