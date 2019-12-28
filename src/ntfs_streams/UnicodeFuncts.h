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

#include <string>

bool IsUtf8(const char * buf, int count, bool itsAll);
bool IsUnicodeLE(char * buf, int count);
bool IsUnicodeBE(char * buf, int count);
std::wstring ToWideChar(std::string_view str, UINT codepage); // CP_ACP, CP_UTF8
std::string ToChar(std::wstring_view str, UINT codepage); // CP_ACP, CP_UTF8
