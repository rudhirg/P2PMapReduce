#include <stdio.h>
#include <string>
#include <wtypes.h>
#include <stdarg.h>
#include <tchar.h>

#define ERR -1
#define CONSOLE 1
#define LOG 2

void Log(int code, LPCTSTR lpszFormat, ...);