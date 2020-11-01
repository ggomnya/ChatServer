#pragma once
#include <windows.h>
#include <strsafe.h>
#include <time.h>
#include <direct.h>

enum en_LOG_LEVEL { LEVEL_DEBUG, LEVEL_WARNING, LEVEL_ERROR, LEVEL_SYSTEM};

WCHAR g_szLogBuff[1024];
int g_iLogLevel = LEVEL_WARNING;
volatile LONG g_LogCount = 0;

#define _LOG(szType, LogLevel, fmt, ...)			\
do {												\
	if( g_iLogLevel <= LogLevel)					\
	{												\
		wsprintf(g_szLogBuff, fmt, ##__VA_ARGS__);	\
		Log(szType, LogLevel, g_szLogBuff);			\
	}												\
} while(0)											\


#define SYSLOG_DIRECTORY(szDir)						\
do {												\
	_wmkdir(szDir);									\
	_wchdir(szDir);									\
} while(0)											\

#define SYSLOG_LEVEL(iLogLevel)									\
do {															\
	if (iLogLevel >= LEVEL_DEBUG && iLogLevel <= LEVEL_SYSTEM)	\
	g_iLogLevel = iLogLevel;									\
} while(0)														\

void Log(const WCHAR* szType, en_LOG_LEVEL LogLevel, WCHAR* szStringFormat, ...) {

	tm TM;
	time_t t;
	time(&t);
	localtime_s(&TM, &t);
	WCHAR szFileName[256];
	WCHAR szLogFormat[256];
	WCHAR szLogLevel[256];
	LONG LogCount = InterlockedIncrement(&g_LogCount);
	if (LogLevel == LEVEL_DEBUG)
		wcscpy_s(szLogLevel, L"DEBUG");
	else if (LogLevel == LEVEL_WARNING)
		wcscpy_s(szLogLevel, L"WARNING");
	else if(LogLevel == LEVEL_ERROR)
		wcscpy_s(szLogLevel, L"ERROR");
	else if (LogLevel == LEVEL_SYSTEM)
		wcscpy_s(szLogLevel, L"SYSTEM");

	StringCchPrintfW(szFileName, 256, L"%04d%02d_%s.txt", TM.tm_year + 1900, TM.tm_mon + 1, szType);
	StringCchPrintfW(szLogFormat, 256, L"[%s] [%04d-%02d-%02d %02d:%02d:%02d / %s / %010d] ", szType, TM.tm_year + 1900, TM.tm_mon + 1, TM.tm_mday,
		TM.tm_hour, TM.tm_min, TM.tm_sec, szLogLevel, LogCount);

	WCHAR szInMessage[256];
	va_list va;
	va_start(va, szStringFormat);
	DWORD hResult = StringCchVPrintf(szInMessage, 256, szStringFormat, va);
	va_end(va);

	//버퍼를 넘겼을 경우
	if(hResult == STRSAFE_E_INSUFFICIENT_BUFFER) {
		_LOG(L"SYSTEM", LEVEL_ERROR, L"%s", szInMessage);
	}
	

	int iCnt = 5;
	while (iCnt>0) {
		FILE* fp;
		int retval = _wfopen_s(&fp, szFileName, L"ab+");
		if (retval != 0) {
			iCnt--;
			continue;
		}
		fwrite(szLogFormat, sizeof(WCHAR), wcslen(szLogFormat), fp);
		fwrite(szInMessage, sizeof(WCHAR), wcslen(szInMessage), fp);
		fclose(fp);
		//wprintf(L"%s", szInMessage);
		break;
	}

}

void LogHex(const WCHAR* szType, en_LOG_LEVEL LogLevel, const WCHAR* szLog, BYTE* pByte, int iByteLen) {
	tm TM;
	time_t t;
	time(&t);
	localtime_s(&TM, &t);
	WCHAR szFileName[256];
	WCHAR szLogFormat[256];
	WCHAR szLogLevel[256];
	LONG LogCount = InterlockedIncrement(&g_LogCount);
	if (LogLevel == LEVEL_DEBUG)
		wcscpy_s(szLogLevel, L"DEBUG");
	else if (LogLevel == LEVEL_WARNING)
		wcscpy_s(szLogLevel, L"WARNING");
	else if (LogLevel == LEVEL_ERROR)
		wcscpy_s(szLogLevel, L"ERROR");
	else if (LogLevel == LEVEL_SYSTEM)
		wcscpy_s(szLogLevel, L"SYSTEM");

	StringCchPrintfW(szFileName, 256, L"%04d%02d_%s.txt", TM.tm_year + 1900, TM.tm_mon + 1, szType);
	StringCchPrintfW(szLogFormat, 256, L"[%s] [%04d-%02d-%02d %02d:%02d:%02d / %s / %010d] ", szType, TM.tm_year + 1900, TM.tm_mon + 1, TM.tm_mday,
		TM.tm_hour, TM.tm_min, TM.tm_sec, szLogLevel, LogCount);
	WCHAR* szHex = new WCHAR[iByteLen*2 + 1];
	for (int i = 0; i < iByteLen; i++) {
		wprintf(L"%02x", *(pByte + i));
		StringCchPrintfW(szHex + 2*i, 4, L"%02x", *(pByte + i));
	}
	szHex[iByteLen * 2] = L'\n';

	int iCnt = 5;
	while (iCnt > 0) {
		FILE* fp;
		int retval = _wfopen_s(&fp, szFileName, L"ab+");
		if (retval != 0) {
			iCnt--;
			continue;
		}
		fwrite(szLogFormat, sizeof(WCHAR), wcslen(szLogFormat), fp);
		fwrite(szLog, sizeof(WCHAR), wcslen(szLog), fp);
		fwrite(szHex, sizeof(WCHAR), wcslen(szHex), fp);
		fclose(fp);
		break;
	}
	delete szHex;
}