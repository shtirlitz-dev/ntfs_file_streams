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

#include "Tar.h"
#include "ntfs_streams.h"
#include "tchar.h"
#include <iostream>
#include <filesystem>

using namespace std;

int ShowHelpTar(filesystem::path filename)
{
	ShowCopyright();
	wcout << L"\ncommand line arguments for '" << filename.c_str() << L" tar':\n\n";
	wcout << L"tar [options] <tar-file> [<item1> <item2> ...]\n";
	wcout << L"where <itemN> are files or directories to add to <tar-file>\n";
	wcout << L"If <itemN> are not specified, all items of the current directory are added\n";
	wcout << L"Default file extension of tar-file is .star\n";
	wcout << L"options:\n";
	wcout << L"  /t             - test: valid console output but tar-file is not created\n";
	wcout << L"  /b:size        - divide output in blocks of specified size, suffixes K, M, G\n";
	wcout << L"  /p:password    - password to encrypt tar-file\n";
	wcout << L"  /e:mask        - mask to exclude files or directories\n";
	return 0;
}

int ShowHelpUntar(filesystem::path filename)
{
	ShowCopyright();
	wcout << L"\ncommand line arguments for '" << filename.c_str() << L" untar':\n\n";
	wcout << L"untar [options] <tar-file> [<dir>]\n";
	wcout << L"where <dir> is directory to extract files from <tar-file> to, default is current\n";
	wcout << L"options:\n";
	wcout << L"  /l             - do not extract, only list file, directory and stream names\n";
	wcout << L"  /p:password    - password to decrypt tar-file\n";
	wcout << L"  /f:sym         - write streams as files, sym replaces ':'\n";
	return 0;
}

int Tar(int argc, TCHAR **argv)
{
	if (argc < 3 || _tcscmp(argv[2], L"/?") == 0)
		return ShowHelpTar(filesystem::path(argv[0]).filename());

	wcout << L"tar/untar function is not implemented yet\n";
	return 0;
}
int Untar(int argc, TCHAR **argv)
{
	if (argc < 3 || _tcscmp(argv[2], L"/?") == 0)
		return ShowHelpUntar(filesystem::path(argv[0]).filename());

	wcout << L"tar/untar function is not implemented yet\n";
	return 0;
}
