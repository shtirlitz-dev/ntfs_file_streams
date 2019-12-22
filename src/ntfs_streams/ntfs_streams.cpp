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
#include <iostream>
#include <fcntl.h>
#include <io.h>
#include <tchar.h>
#include "ntfs_streams.h"
#include "FileSimple.h"
#include "UnicodeStream.h"
#include "UnicodeFuncts.h"
#include "Tar.h"

using namespace std;

int wmain(int argc, TCHAR **argv) // main(int argc, char **argv)
{
	_setmode(_fileno(stdout), _O_U16TEXT);  // enable Unicode in console
	_setmode(_fileno(stdin), _O_U16TEXT);  // enable Unicode in console
	_setmode(_fileno(stderr), _O_U16TEXT);  // enable Unicode in console

	try
	{
		if (argc == 1) // only executable
		{
			ShowShortHelp(filesystem::path(argv[0]).filename());
			return ShowListFiles(nullptr, false);
		}

		auto cmd = argv[1];
		const wchar_t* arg2 = argc >= 2 ? argv[2] : nullptr;
		const wchar_t* arg3 = argc >= 3 ? argv[3] : nullptr;

		if (_tcscmp(cmd, L"/?") == 0 ||
			_tcscmp(cmd, L"/help") == 0 ||
			_tcscmp(cmd, L"?") == 0)
		{
			ShowUsage(filesystem::path(argv[0]).filename());
			return 0;
		}

		if (_tcscmp(cmd, L"copy") == 0) // copy src dest
			return CopyStream(arg2, arg3);
		if (_tcscmp(cmd, L"type") == 0) // type src
			return TypeStream(arg2);
		if (_tcscmp(cmd, L"echo") == 0) // echo dest
			return EchoStream(arg2);
		if (_tcscmp(cmd, L"del") == 0) // del src
			return DeleteStream(arg2);
		if (_tcscmp(cmd, L"dir") == 0 || _tcscmp(cmd, L"ls") == 0)
			return ShowListFiles(arg2, true);
		if (_tcscmp(cmd, L"tar") == 0)
			return Tar(argc, argv);
		if (_tcscmp(cmd, L"untar") == 0)
			return Untar(argc, argv);

		return ShowListFiles(cmd, false);
	}
	catch (MyException& e)
	{
		wstring msg = e.msg;
		if (auto pos = msg.find(L"<err>"); pos != wstring::npos)
			msg = msg.substr(0, pos) + GetErrorMessage(e.dwError) + msg.substr(pos + 5);
		if (auto pos = msg.find(L"<path>"); pos != wstring::npos)
			msg = msg.substr(0, pos) + e.path + msg.substr(pos + 6);
		wcerr << msg << endl;
	}
	//catch ( filesystem::filesystem_error& e )
	//{
	//	cerr << e.what() << endl;
	//}
	//catch ( invalid_argument & e )
	//{
	//	cerr << e.what() << endl;
	//}
	catch (exception& e)
	{
		wcerr << ToWideChar(e.what(), CP_ACP) << endl;
	}
	catch (...)
	{
		wcerr << L"Unhandled exception" << endl;
	}
	return 1;
}

wstring FileSizeStr(ULONGLONG fsize)
{
	wstring sz = to_wstring(fsize);
	for (int pos = (int)sz.size() - 3; pos > 0; pos -= 3)
		sz.insert(pos, 1, ' ');
	return sz;
}

wstring_view RemoveAtEnd(wstring_view str, wstring_view end)
{
	int diff = (int)str.size() - (int)end.size();
	if (diff >= 0 && str.substr(diff) == end)
		return str.substr(0, diff);
	return str;
}

void ShowStreamsOnFile(const filesystem::path& filename, bool show_all_files, const wchar_t* prefix)
{
	// Enumerate file's streams and print their sizes and names

	WIN32_FIND_STREAM_DATA fsd;
	HANDLE hFind = ::FindFirstStreamW(filename.c_str(), FindStreamInfoStandard, &fsd, 0);
	for (bool ok = hFind != INVALID_HANDLE_VALUE; ok; ok = !!::FindNextStreamW(hFind, &fsd))
	{
		if (wcscmp(fsd.cStreamName, L"::$DATA") == 0) // this is the main stream
			continue;
		wstring_view stream_name = RemoveAtEnd(fsd.cStreamName, L":$DATA"); // name without ":$DATA" in the end
		int space = show_all_files ? 35 : 15;
		wcout << setw(space) << FileSizeStr(fsd.StreamSize.QuadPart) << L" " << prefix << stream_name << endl;
	}
	if (hFind != INVALID_HANDLE_VALUE)
		::FindClose(hFind);
}

void PrintInfo(const std::filesystem::directory_entry& item)
{
	// no info about item.last_write_time() found
	// but considering it as the same value as FILETIME - works!
	ULONGLONG ftime = item.last_write_time().time_since_epoch().count();
	FILETIME* pft = (FILETIME*)&ftime;
	SYSTEMTIME st;
	FileTimeToLocalFileTime(pft, pft);
	FileTimeToSystemTime(pft, &st);
	TCHAR stime[40];
	swprintf_s(stime, L"%02u.%02u.%04u  %02u:%02u", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute );

	wcout << stime << L" " << setw(17);
	if (item.is_regular_file())
	{
		wcout << FileSizeStr(item.file_size());
	}
	else if (item.is_directory())
	{
		wcout << L"<DIR>         ";
	}
	else
	{
		wcout << L"?";
	}
	wcout << L" " << item.path().filename().c_str() << endl;

}

void ListFiles(const filesystem::path& dir, bool show_all_files)
{
	TCHAR szVolName[MAX_PATH], szFSName[MAX_PATH];
	DWORD dwSN, dwMaxLen, dwVolFlags;
	auto root = dir.root_path();
	if (!::GetVolumeInformation(root.c_str(), szVolName, MAX_PATH, &dwSN,
		&dwMaxLen, &dwVolFlags, szFSName, MAX_PATH))
	{
		//throw MyException{ L"Cannot get volume information for '<path>': <err>", root.c_str(), GetLastError() };
		wcout << L"Cannot get volume information for " << root.c_str() << endl;
	}
	else
	{
		wcout << root.c_str() << L" - Volume Name: " << szVolName << ", File System: " << szFSName << endl;
		if (!(dwVolFlags & FILE_NAMED_STREAMS))
			//throw MyException{ L"Named streams are not supported on '<path>'", root.c_str() };
			wcout << L"Named streams are not supported on " << root.c_str() << endl;
	}

	wcout << L"Directory: " << dir.c_str() << endl << endl;

	/*
	we could use FindFirstFile/FindNextFile and get this info:
	WIN32_FIND_DATA fd;
	DWORD dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	DWORD nFileSizeHigh;
	DWORD nFileSizeLow;
	*/

	ShowStreamsOnFile(dir, show_all_files, L".\\");
	for (auto& item : filesystem::directory_iterator(dir))
	{
		if (show_all_files)
			PrintInfo(item);
		ShowStreamsOnFile(item.path(), show_all_files, item.path().filename().c_str());
	}
}

int ShowListFiles(const wchar_t* dir, bool all)
{
	filesystem::path cvt;
	if (!dir)
		cvt = filesystem::current_path();
	else
		cvt = filesystem::canonical(dir);
	ListFiles(cvt, all);
	return 0;
}

int CopyStream(const wchar_t* src, const wchar_t* dest)
{
	if (!src || !dest)
		throw invalid_argument("Specify source and destination stream");

	// filesystem::copy_file does not work properly (streams are considered as invalid parameter)
	// auto res = filesystem::copy_file( src, dest );

	FileSimple fs_src(src);
	if (!fs_src.IsOpen())
		throw MyException{ L"Failed to open source '<path>': <err>", src, GetLastError() };

	FileSimple fs_dest(dest, true);
	if (!fs_dest.IsOpen())
		throw MyException{ L"Failed to create destination '<path>': <err>", dest, GetLastError() };

	ULONGLONG total = 0;
	while (true)
	{
		BYTE buf[64 * 1024];
		SetLastError(0);
		DWORD dwBytesRead = fs_src.Read(buf, sizeof(buf));
		DWORD dwErr = GetLastError();
		if (dwErr)
			throw MyException{ L"Failed to read '<path>': <err>", src, dwErr };
		if (!dwBytesRead)
			break;
		DWORD dwBytesWritten = fs_dest.Write(buf, dwBytesRead);
		if (dwBytesWritten != dwBytesRead)
			throw MyException{ L"Failed to write '<path>': <err>", dest, GetLastError() };

		total += dwBytesWritten;
		if (dwBytesRead < sizeof(buf))
			break;
	}
	wcout << total << L" bytes copied from " << src << " to " << dest << endl;
	return 0;
}

int TypeStream(const wchar_t* src)
{
	if (!src)
		throw invalid_argument("Specify stream to type");

	FileSimple fs_src(src);
	if (!fs_src.IsOpen())
		throw MyException{ L"Failed to open source '<path>': <err>", src, GetLastError() };

	struct MyStream : public CharStream
	{
		MyStream(FileSimple &fs) : fs(fs) {}
		FileSimple &fs;
		virtual int Read(char* buf, int count) override { return fs.Read(buf, count); }
	};

	MyStream myStream{ fs_src };
	for (auto str : GetStrings(&myStream))
		wcout << str;
	wcout << endl;
	return 0;
}

int EchoStream(const wchar_t* dest)
{
	if (!dest)
		throw invalid_argument("Specify stream to echo to");

	FileSimple fs_dest(dest, true);
	if (!fs_dest.IsOpen())
		throw MyException{ L"Failed to create destination '<path>': <err>", dest, GetLastError() };

	wcout << L"Type Ctrl-Z at the end of your text" << endl;
	wstring str;
	ULONGLONG total = 0;
	while (getline(wcin, str))
	{
		str += L"\r\n";
		string s = ToChar(str, CP_UTF8);
		DWORD towrite = (DWORD)s.size();
		DWORD written = fs_dest.Write(s.c_str(), towrite);
		total += written;
		if (written != towrite)
			throw MyException{ L"Failed to write '<path>': <err>", dest, GetLastError() };
	}

	wcout << total << L" bytes written to " << dest << endl;
	return 0;
}

int DeleteStream(const wchar_t* src)
{
	if (!src)
		throw invalid_argument("Specify stream to delete");

	if (!::DeleteFile(src))
		throw MyException{ L"Failed to delete '<path>': <err>", src, GetLastError() };

	wcout << src << " deleted" << endl;
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// printing
//////////////////////////////////////////////////////////////////////////


void ShowCopyright()
{
	wcout << L"Copyright (c) 2019 Vsevolod Lukyanin\n";
}

void ShowShortHelp(filesystem::path filename)
{
	ShowCopyright();
	wcout << L"\ntype '" << filename.c_str() << L" /?' to get help\n\n";
}

void ShowUsage(filesystem::path filename)
{
	ShowCopyright();
	wcout << L"\ncommand line arguments for " << filename.c_str() << L":\n\n";
	wcout << L"(no args)         - shows short help and lists streams in the current directory\n";
	wcout << L"<dir>             - lists streams in the specified directory\n";
	wcout << L"dir|ls <dir>      - lists all files and streams in the current or specified directory\n";
	wcout << L"copy <src> <dest> - copies contents from src to dest\n";
	wcout << L"type <src>        - writes specified stream to stdout\n";
	wcout << L"echo <dest>       - copies stdin to the specified stream\n";
	wcout << L"del <src>         - deletes specified stream\n";
	wcout << L"tar|untar /?      - more help about tar-function\n";
	wcout << L"del <src>         - deletes specified stream\n";
	wcout << L"/?                - shows this help\n\n";
	wcout << L"where <src> and <dest> are file names or stream names, e.g. file.txt:stream1 or :s1:$DATA\n";
	wcout << L"      <dir> is some directory name, including . or ..\n";
}

wstring GetErrorMessage(DWORD dw)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	wstring ret = (LPTSTR)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}
