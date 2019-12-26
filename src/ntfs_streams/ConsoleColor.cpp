#include "pch.h"
#include "ConsoleColor.h"


ConsoleColor::ConsoleColor(WORD wAttributes, DWORD nStdHandle)
{
	hStdOut = GetStdHandle(nStdHandle);
	CONSOLE_SCREEN_BUFFER_INFO bi;
	if (hStdOut != INVALID_HANDLE_VALUE) {
		GetConsoleScreenBufferInfo(hStdOut, &bi);
		wDefault = bi.wAttributes;
	}
	if (wAttributes != NoColor)
		SetColor(wAttributes);
}


ConsoleColor::~ConsoleColor()
{
	SetColor();
}

void ConsoleColor::SetColor(WORD wAttributes) // FOREGROUND_RED | FOREGROUND_INTENSITY, 0xFFFF to reset
{
	if (wAttributes == NoColor)
		wAttributes = wDefault;
	if(hStdOut != INVALID_HANDLE_VALUE && wAttributes != NoColor)
		SetConsoleTextAttribute(hStdOut, wAttributes);
}
