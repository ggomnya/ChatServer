#pragma once
#include <Windows.h>

class CProcessUsage {
public:
	CProcessUsage(HANDLE hProcess = INVALID_HANDLE_VALUE);

	void UpdateProcessTime();
	float ProcessTotal() {
		return _fProcessTotal;
	}
	float ProcessUser() {
		return _fProcessUser;
	}
	float ProcessKernel() {
		return _fProcessKernel;
	}

	void PrintProcessInfo();

private:
	HANDLE _hProcess;
	int _iNumberOfProcessors;

	float _fProcessTotal;
	float _fProcessUser;
	float _fProcessKernel;

	ULARGE_INTEGER _ftProcess_LastKernel;
	ULARGE_INTEGER _ftProcess_LastUser;
	ULARGE_INTEGER _ftProcess_LastTime;
};