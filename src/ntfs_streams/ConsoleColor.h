#pragma once

class ConsoleColor
{
	static const WORD NoColor = 0xFFFF;
public:
	ConsoleColor(WORD wAttributes = NoColor, DWORD nStdHandle = STD_OUTPUT_HANDLE);
	~ConsoleColor();
	void SetColor(WORD wAttributes = NoColor); // FOREGROUND_RED | FOREGROUND_INTENSITY, 0xFFFF to reset
protected:
	HANDLE hStdOut;
	WORD wDefault = NoColor;
};

