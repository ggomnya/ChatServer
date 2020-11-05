#include <windows.h>
#include <stdio.h>
#include "CProcessUsage.h"

CProcessUsage::CProcessUsage(HANDLE hProcess) {
	if (hProcess == INVALID_HANDLE_VALUE)
		_hProcess = GetCurrentProcess();

	SYSTEM_INFO SystemInfo;

	GetSystemInfo(&SystemInfo);
	_iNumberOfProcessors = SystemInfo.dwNumberOfProcessors;

	_fProcessTotal = 0;
	_fProcessUser = 0;
	_fProcessKernel = 0;

	_ftProcess_LastKernel.QuadPart = 0;
	_ftProcess_LastUser.QuadPart = 0;
	_ftProcess_LastTime.QuadPart = 0;

	UpdateProcessTime();

}

void CProcessUsage::UpdateProcessTime() {

	ULARGE_INTEGER Kernel;
	ULARGE_INTEGER User;
	ULARGE_INTEGER None;
	ULARGE_INTEGER NowTime;

	ULONGLONG KernelDiff;
	ULONGLONG UserDiff;
	ULONGLONG TimeDiff;
	ULONGLONG Total;

	GetSystemTimeAsFileTime((LPFILETIME)&NowTime);

	GetProcessTimes(_hProcess, (LPFILETIME)&None, (LPFILETIME)&None, (LPFILETIME)&Kernel, (LPFILETIME)&User);


	TimeDiff = NowTime.QuadPart - _ftProcess_LastTime.QuadPart;
	UserDiff = User.QuadPart - _ftProcess_LastUser.QuadPart;
	KernelDiff = Kernel.QuadPart - _ftProcess_LastKernel.QuadPart;

	Total = KernelDiff + UserDiff;

	_fProcessTotal = (float)(Total / (double)_iNumberOfProcessors / (double)TimeDiff * 100.0f);
	_fProcessKernel = (float)(KernelDiff / (double)_iNumberOfProcessors / (double)TimeDiff * 100.0f);
	_fProcessUser = (float)(UserDiff / (double)_iNumberOfProcessors / (double)TimeDiff * 100.0f);

	_ftProcess_LastTime = NowTime;
	_ftProcess_LastKernel = Kernel;
	_ftProcess_LastUser = User;

}

void CProcessUsage::PrintProcessInfo() {
	wprintf(L"[Process: %.1f%% U: %.1f%% K: %.1f%%]\n", _fProcessTotal, _fProcessUser, _fProcessKernel);
}

