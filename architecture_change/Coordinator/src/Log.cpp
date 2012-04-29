#include "Log.h"

void Log(int code, LPCTSTR lpszFormat, ...)
{
	std::wstring codes = L"";
	bool bPrintConsole = false;

	switch(code) {
		case -1:
			codes = L"[ERROR] ";
			bPrintConsole = true;
			break;
		case 1:
			codes = L"[CONSOLE] ";
			bPrintConsole = true;
			break;
		case 2:
			codes = L"[LOG] ";
			break;
		default:
			codes = L"[LOG] ";
			break;
	}

	va_list args;
    va_start(args, lpszFormat);
    int nBuf;
    TCHAR szBuffer[512]; // get rid of this hard-coded buffer
	nBuf = _vsntprintf(szBuffer, 511, lpszFormat, args);
	std::wstring w = szBuffer;
	codes += w;
	::OutputDebugString(codes.c_str());
	if(bPrintConsole)
		wprintf(L"[%s] %s", codes.c_str(), w.c_str());
    va_end(args);

}