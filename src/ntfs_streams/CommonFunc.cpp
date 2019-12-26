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
#include "CommonFunc.h"


using namespace std;

wstring_view RemoveAtEnd(wstring_view str, wstring_view end)
{
	int diff = (int)str.size() - (int)end.size();
	if (diff >= 0 && str.substr(diff) == end)
		return str.substr(0, diff);
	return str;
}

wstring FileSizeStr(ULONGLONG fsize)
{
	wstring sz = to_wstring(fsize);
	for (int pos = (int)sz.size() - 3; pos > 0; pos -= 3)
		sz.insert(pos, 1, ' ');
	return sz;
}

template<typename DATA>
ULONGLONG FileSizeFrom(const DATA& data)
{
	LARGE_INTEGER filesize;
	filesize.LowPart = data.nFileSizeLow;
	filesize.HighPart = data.nFileSizeHigh;
	return (ULONGLONG)filesize.QuadPart;
}

std::vector<wstring> split(wstring_view str, wchar_t delim)
{
	std::vector<wstring> res;
	size_t pos;
	while (pos = str.find(delim), pos != wstring::npos)
	{
		res.emplace_back(str.substr(0, pos));
		str = str.substr(pos + 1);
	}
	if (!str.empty())
		res.emplace_back(str);
	return res;
}

wchar_t char_upper(wchar_t ch)
{
	return (wchar_t)CharUpperW((LPWSTR)(DWORD_PTR)(DWORD)ch);
}
bool mask_match(const wchar_t* str, const wchar_t* mask)
{
	while (*mask) {
		char tpl = *mask++;
		switch (tpl) {
		case '*':
			switch (*mask)
			{
			case 0: return true; // любое окончание data
			case '*': continue; // 2 or more * 
			default: // str must contain remainder of wildcard
				while (*str)
				{
					if (mask_match(str, mask))
						return true;
					str++;
				}
				return false;
			}
			break;
		case '?': // any char
			if (!*str++)
				return false;
			break;
		default:
			if (wchar_t ch = *str++; ch != tpl && char_upper(ch) != char_upper(tpl))
				return false;
			break;
		}
	}
	return *str == '\0';
}

bool mask_match(const wchar_t* str, const vector<wstring> &masks)
{
	for (auto& it : masks)
		if (mask_match(str, it.c_str()))
			return true;
	return false;
}

bool is_stream_name(const wchar_t* entry)
{
	// a:something    file (drive letter)
	// something:a    stream
	// a:/something:a stream
	// \\?\c:\        directory 
	const wchar_t* dp = wcsrchr(entry, L':');
	if (!dp)
		return false;
	bool is_drive = dp == entry + 1 ||
		(dp == entry + 5 && wcsncmp(entry, L"\\\\?\\", 4) == 0);
	return !is_drive;
}


experimental::generator<DirItem> get_streams(filesystem::path entry, const wchar_t * sep)
{
	// Enumerate file's streams and print their sizes and names
	WIN32_FIND_STREAM_DATA fsd;
	HANDLE hFind = ::FindFirstStreamW(entry.c_str(), FindStreamInfoStandard, &fsd, 0);
	for (bool ok = hFind != INVALID_HANDLE_VALUE; ok; ok = !!::FindNextStreamW(hFind, &fsd))
	{
		if (wcscmp(fsd.cStreamName, L"::$DATA") == 0) // this is the main stream
			continue;
		wstring_view stream_name = RemoveAtEnd(fsd.cStreamName, L":$DATA"); // name without ":$DATA" in the end
		filesystem::path full = wstring(entry) + sep + wstring(stream_name);
		DirItem it = { DirItem::Stream, full,  (ULONGLONG)fsd.StreamSize.QuadPart };
		co_yield it;
	}
	if (hFind != INVALID_HANDLE_VALUE)
		::FindClose(hFind);
}

std::experimental::generator<DirItem> get_files(std::filesystem::path path)
{
	for (auto& dir_stream : get_streams(path, L"\\")) // stream for directory itself
		co_yield dir_stream;

	WIN32_FIND_DATA ffd;
	HANDLE hFind = FindFirstFile((path / L"*").c_str(), &ffd);
	if (hFind == INVALID_HANDLE_VALUE)
		return;

	do
	{
		if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
			continue;
		DirItem::Type type = ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? DirItem::Dir : DirItem::File;
		co_yield DirItem{ type, path / ffd.cFileName, FileSizeFrom(ffd),
			ffd.dwFileAttributes, ffd.ftLastWriteTime };
	} while (FindNextFile(hFind, &ffd) != 0);
	FindClose(hFind);
}

experimental::generator<DirItem> directory_items(filesystem::path path)
{
	for (auto& dir_stream : get_streams(path, L"\\")) // stream for directory itself
		co_yield dir_stream;

	for (auto& entry : filesystem::directory_iterator(path))
	{
		if (entry.is_directory())
			co_yield DirItem{ DirItem::Dir, entry.path(),  0 };
		else if (entry.is_regular_file())
		{
			co_yield DirItem{ DirItem::File, entry.path(),  entry.file_size() };

			for (auto& file_stream : get_streams(entry.path(), L""))
				co_yield file_stream;
		}
	}
}

std::experimental::generator<DirItem> get_files_multi(const std::vector<filesystem::path>& items)
{
	for (auto& p : items)
	{
		if (p == L".") {
			for (auto& it : get_files(filesystem::current_path()))
				co_yield it;
			continue;
		}
		WIN32_FILE_ATTRIBUTE_DATA adata = {};
		DirItem::Type type =
			!GetFileAttributesEx(p.c_str(), GetFileExInfoStandard, &adata) ? DirItem::Invalid :
			adata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? DirItem::Dir :
			is_stream_name(p.c_str()) ? DirItem::Stream : DirItem::File;
		co_yield DirItem{ type, p, FileSizeFrom(adata), adata.dwFileAttributes, adata.ftLastWriteTime };
	}
}
