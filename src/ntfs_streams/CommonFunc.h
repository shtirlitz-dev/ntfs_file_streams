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


#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <experimental/generator>
#include <filesystem>

std::wstring_view RemoveAtEnd(std::wstring_view str, std::wstring_view end);
std::wstring FileSizeStr(ULONGLONG fsize);
std::vector<std::wstring> split(std::wstring_view str, wchar_t delim);
bool mask_match(const wchar_t* str, const wchar_t* mask);
bool mask_match(const wchar_t* str, const std::vector<std::wstring> &masks);

struct DirItem
{
	enum Type { Dir, File, Stream, Invalid };
	Type type;
	std::filesystem::path name;
	ULONGLONG size;
	DWORD    dwFileAttributes; // FILE_ATTRIBUTE_DIRECTORY FILE_ATTRIBUTE_ARCHIVE FILE_ATTRIBUTE_HIDDEN FILE_ATTRIBUTE_NORMAL FILE_ATTRIBUTE_READONLY FILE_ATTRIBUTE_SYSTEM
	FILETIME ftLastWriteTime;
};

bool is_stream_name(const wchar_t* entry);
std::experimental::generator<DirItem> get_streams(std::filesystem::path entry, const wchar_t* sep);
std::experimental::generator<DirItem> get_files(std::filesystem::path path); // WinAPI
std::experimental::generator<DirItem> directory_items(std::filesystem::path path); // std::filesystem
std::experimental::generator<DirItem> get_files_multi(const std::vector<std::filesystem::path>& items);
