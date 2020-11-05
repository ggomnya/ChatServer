#pragma once
#include <windows.h>


class CCpuUsage {
public:
	CCpuUsage(HANDLE hProcess = INVALID_HANDLE_VALUE);

	void UpdateCpuTime();
	float ProcessorTotal() {
		return _fProcessorTotal;
	}
	float ProcessorUser() {
		return _fProcessorUser;
	}
	float ProcessorKernel() {
		return _fProcessorKernel;
	}

	void PrintCPUInfo();

private:
	HANDLE _hProcess;
	int _iNumberOfProcessors;

	float _fProcessorTotal;
	float _fProcessorUser;
	float _fProcessorKernel;


	ULARGE_INTEGER _ftProcessor_LastKernel;
	ULARGE_INTEGER _ftProcessor_LastUser;
	ULARGE_INTEGER _ftProcessor_LastIdle;

};