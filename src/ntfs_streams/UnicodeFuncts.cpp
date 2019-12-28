/***********************************************************************

  Copyright (c) 2019 Vsevolod Lukyanin
  All rights reserved.

  This file is a part of project ntfs_file_streams:
  https://github.com/shtirlitz-dev/ntfs_file_streams

  The tool that enables to list, create, copy, delete, show, write and
  archive NTFS files streams.
  About file streams:
  https://docs.microsoft.com/en-us/windows/win32/fileio/file-streams

***********************************************************************/

#include "pch.h"
#include "UnicodeFuncts.h"


bool IsUtf8(const char * buf, int count, bool itsAll)
{
	// UTF8:
	//  byte 0xxxxxxx -> like ASCII
	//  byte 10xxxxxx alone is not valid
	//  byte 110xxxxx must be followed by 10xxxxxx
	//  byte 1110xxxx must be followed by 10xxxxxx, 10xxxxxx
	//  byte 11110xxx must be followed by 10xxxxxx, 10xxxxxx, 10xxxxxx
	//  byte 111110xx must be followed by 10xxxxxx, 10xxxxxx, 10xxxxxx, 10xxxxxx
	//  byte 1111110x must be followed by 10xxxxxx, 10xxxxxx, 10xxxxxx, 10xxxxxx, 10xxxxxx
	//  byte 1111111x alone not valid
	const char * end = buf + count;
	while (buf < end)
	{
		char uc = *buf++;
		if ((uc & 0b1000'0000) == 0)
			continue; // usual ascii
		int nCount =
			((uc & 0b1110'0000) == 0b1100'0000) ? 1 : ((uc & 0b1111'0000) == 0b1110'0000) ? 2 : ((uc & 0b1111'1000) == 0b1111'0000) ? 3 : ((uc & 0b1111'1100) == 0b1111'1000) ? 4 : ((uc & 0b1111'1110) == 0b1111'1100) ? 5 : 0;
		if (!nCount)
			return false;
		while (nCount--)
		{
			if (buf == end)
				return !itsAll;
			if ((*buf++ & 0b1100'0000) != 0b1000'0000)
				return false;
		}
	}
	return true;
}

bool IsUnicodeLE(char * buf, int count)
{
	INT nFlags = IS_TEXT_UNICODE_STATISTICS;
	::IsTextUnicode(buf, count, &nFlags);
	return (nFlags & IS_TEXT_UNICODE_STATISTICS) != 0;
}
bool IsUnicodeBE(char * buf, int count)
{
	INT nFlags = IS_TEXT_UNICODE_REVERSE_STATISTICS;
	::IsTextUnicode(buf, count, &nFlags);
	return (nFlags & IS_TEXT_UNICODE_REVERSE_STATISTICS) != 0;
}

std::wstring ToWideChar(std::string_view str, UINT codepage)
{
	int count = str.empty() ? 0 : ::MultiByteToWideChar(codepage, 0, str.data(), (int)str.size(), nullptr, 0);
	if (count <= 0)
		return std::wstring();

	std::wstring strWide(count, L'\0');
	::MultiByteToWideChar(codepage, 0, str.data(), (int)str.size(), strWide.data(), count);
	return strWide;
}


std::string ToChar(std::wstring_view str, UINT codepage) // CP_ACP, CP_UTF8
{
	int count = str.empty() ? 0 : ::WideCharToMultiByte(codepage, 0, str.data(), (int)str.size(), nullptr, 0, nullptr, nullptr);
	if (count <= 0)
		return std::string();

	std::string strChar(count, '\0');
	::WideCharToMultiByte(codepage, 0, str.data(), (int)str.size(), strChar.data(), count, nullptr, nullptr);
	return strChar;
}