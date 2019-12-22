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
#include "UnicodeStream.h"
#include "UnicodeFuncts.h"


enum Format { Multibyte, LitteEndian, BigEndian, Utf8 };

// UTF8:
//  byte 0xxxxxxx -> like ASCII
//  byte 10xxxxxx alone is not valid
//  byte 110xxxxx must be followed by 10xxxxxx
//  byte 1110xxxx must be followed by 10xxxxxx, 10xxxxxx
//  byte 11110xxx must be followed by 10xxxxxx, 10xxxxxx, 10xxxxxx
//  byte 111110xx must be followed by 10xxxxxx, 10xxxxxx, 10xxxxxx, 10xxxxxx
//  byte 1111110x must be followed by 10xxxxxx, 10xxxxxx, 10xxxxxx, 10xxxxxx, 10xxxxxx
//  byte 1111111x alone not valid

template <class GetCh>
std::experimental::generator<WCHAR> ConvertUtf8(GetCh in)
{
	static char arrMasks[] = { 0, 0b0001'1111, 0b0000'1111, 0b0000'0111, 0b0000'0011, 0b0000'0001 };
	bool bUtf8 = false;
	bool bError = false;
	int ich;
	while (ich = in(), ich != -1)
	{
		char uc = (char)ich;
		if ((uc & 0b1000'0000) == 0)
		{
			co_yield (uint8_t)uc;
			continue; // usual ascii
		}
		if ((uc & 0b1100'0000) == 0b1000'0000 || (uc & 0b1111'1110) == 0b1111'1110)
		{
			bError = true; // illegal symbol
			co_yield (uint8_t)uc;
			continue; // usual ascii
		}
		int nCount =
			((uc & 0b1110'0000) == 0b1100'0000) ? 1 : ((uc & 0b1111'0000) == 0b1110'0000) ? 2 : ((uc & 0b1111'1000) == 0b1111'0000) ? 3 : ((uc & 0b1111'1100) == 0b1111'1000) ? 4 : ((uc & 0b1111'1110) == 0b1111'1100) ? 5 : -1; // this is covered by "(uc & 0b1111'1110) == 0b1111'1110"
		if (nCount == -1) // should not happen
			return;
		uint32_t nPoint = uc & arrMasks[nCount];
		while (nCount--)
		{
			ich = in();
			if (ich == -1 || (uc = (char)ich, (uc & 0b1100'0000) != 0b1000'0000))
			{
				bError = true; // illegal symbol
				nPoint = (uint8_t)ich; // output the original first symbol and continue from the symbol after first
				break;
			}
			nPoint = (nPoint << 6) | (uc & 0b0011'1111);
		}
		bUtf8 = true;

		if (nPoint < 0x10000) // encoded with 1 symbol
			co_yield (WCHAR)nPoint;
		else { // encoded with 2 surrogates
			nPoint -= 0x10000;
			co_yield (0xD800 + (WCHAR)(nPoint >> 10));
			co_yield (0xDC00 + (WCHAR)(nPoint & 0x3FF));
		}
	}
}

template <class GetCh>
std::experimental::generator<WCHAR> ConvertPair(GetCh in, bool bLE)
{
	while (true)
	{
		int b1 = in();
		if (b1 == -1)
			return;
		int b2 = in();
		if (b2 == -1)
			return;
		co_yield bLE ? wchar_t((b2 << 8) | b1) : wchar_t((b1 << 8) | b2);
	}
}


template <class GetCh>
std::experimental::generator<WCHAR> ConvertMultibyte(GetCh in)
{
	// accumulate entire string and convert it using current system codepage
	std::string accum;
	accum.reserve(1000);
	while (true)
	{
		int ich = in();
		if (ich != -1)
			accum += (char)ich;
		if (ich == -1 || ich == '\r' || ich == '\n')
		{
			std::wstring uni = ToWideChar(accum, CP_ACP);
			for (auto c : uni)
				co_yield c;
			accum = "";
		}
		if (ich == -1)
			return;
	}
}

std::experimental::generator<std::wstring> GetStrings(CharStream *pStream)
{
	char buf[2000];
	int count = pStream->Read(buf, (int) sizeof(buf));
	if (!count)
		return;

	int pos = 0;
	Format fmt = Multibyte;
	if (count >= 2 && (WORD&)buf[0] == 0xFEFF) { // FF FE  - BOM LittleEndian, usual, sampe of space "20 00"
		pos = 2;
		fmt = LitteEndian;
	}
	else if (count >= 2 && (WORD&)buf[0] == 0xFFFE) { // FE FF  - BOM BigEndian, unusual, sample of space "00 20"
		pos = 2;
		fmt = BigEndian;
	}
	else if (count >= 3 && ((DWORD&)buf[0] & 0xFFFFFF) == 0xBFBBEF) { // EF BB BF - BOM utf8
		pos = 3;
		fmt = Utf8;
	}
	else if (IsUtf8(buf, count, count < 1000)) {
		fmt = Utf8;
	}
	else if (IsUnicodeLE(buf, count)) {
		fmt = LitteEndian;
	}
	else if (IsUnicodeBE(buf, count)) {
		fmt = BigEndian;
	}

	// pusher provides input stream, one char at once, -1 means end of data
	auto pusher = [&pos, &buf, &count, pStream]() -> int {
		if (pos == count) {
			pos = 0;
			count = pStream->Read(buf, (int)sizeof(buf));
		}
		if (pos < count)
			return 0xFF & (int)buf[pos++];
		return -1;
	};

	std::wstring str;
	str.reserve(1000);
	std::experimental::generator<WCHAR> gen;
	if (fmt == Utf8)
		gen = ConvertUtf8(pusher);
	else if (fmt == LitteEndian || fmt == BigEndian)
		gen = ConvertPair(pusher, fmt == LitteEndian);
	else
		gen = ConvertMultibyte(pusher);

	wchar_t prev = 0;
	for (wchar_t ch : gen) {
		bool pair = (prev ^ ch) == ('\r' ^ '\n');
		prev = pair ? 0 : ch;
		bool rn = ch == '\r' || ch == '\n'; // carriage-return, line-feed
		if (pair && rn)  // consider cr+lf or lf+cr as 1 symbol
			continue;
		if (rn || ch == 0x2028 || ch == 0x2029 || ch == 0x85) // line-separator, paragraph-separator, next-line
		{
			str += '\n';
			co_yield str;
			str = L"";
		}
		else
			str += ch;
	}
	if (!str.empty())
		co_yield str;
}
