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

bool mask_match(const wchar_t* str, const wstring_view& masks)
{
	return false; // TODO
}

bool mask_match(const wchar_t* str, const vector<wstring> &masks)
{
	for (auto& it : masks)
		if (mask_match(str, it))
			return true;
	return false;
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

