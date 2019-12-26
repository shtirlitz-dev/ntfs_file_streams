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
#include "CommonFunc.h"
#include "FileSimple.h"
#include <tchar.h>
#include <iostream>

using namespace std;

namespace
{
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
		wcout << L"  /e:mask1;mask2 - masks to exclude files or directories\n";
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

	bool starts_with(wstring_view str, wstring_view beg)
	{
		return str.size() >= beg.size() && str.substr(0, beg.size()) == beg;
	}

	ULONGLONG ReadSize(wstring_view str)
	{
		wchar_t* e;
		ULONGLONG ul = wcstoul(str.data(), &e, 10);
		wstring_view suffix(e);
		if (suffix == L"K" || suffix == L"k")
			ul *= 1024;
		else if (suffix == L"M" || suffix == L"m")
			ul *= 1024 * 1024;
		else if (suffix == L"G" || suffix == L"g")
			ul *= 1024 * 1024 * 1024;
		else if (suffix != L"")
			throw invalid_argument("Invalid block size");
		return ul;
	}



	template<class T>
	wostream& operator<<(wostream &o, const vector<T>& v)
	{
		o << L"{ ";
		for (auto& it : v)
			o << it << " ";
		o << L"}";
		return o;
	}

	class ITarWriter
	{
	public:
		virtual ~ITarWriter() {}
		virtual void Write(const void* buf, DWORD size) = 0;
		virtual bool IsMyFile(const filesystem::path& path) { return false; }
		ULONGLONG written_total = 0;
	};

	class TarWriterFiles : public ITarWriter
	{
		filesystem::path name;
		ULONGLONG part_size;
		ULONGLONG written_current = 0;
		ULONGLONG current_part = 0;

	public:
		TarWriterFiles(filesystem::path name, ULONGLONG part_size)
			:name(name), part_size(part_size)
		{

		}
		virtual void Write(const void* buf, DWORD size) override
		{

		}
	};
	class TarWriterTest : public ITarWriter
	{
	public:
		virtual void Write(const void* buf, DWORD size) override
		{
			written_total += size;
		}
	};

}


void WriteTarDirectory(ITarWriter * writer, const DirItem& item, const vector<wstring>& exclude, const filesystem::path& rel_path, const wstring& prefix);
void WriteTarFile(ITarWriter * writer, const DirItem& item, const vector<wstring>& exclude, const filesystem::path& rel_path, const wstring& prefix);
void WriteTarStream(ITarWriter * writer, const DirItem& item, const filesystem::path& rel_path, const wstring& prefix);

void TarFiles(ITarWriter * writer, experimental::generator<DirItem>&& items, const vector<wstring>& exclude,
	const filesystem::path& rel_path, const wstring& prefix)
{
	for (auto& it : items)
	{
		if (mask_match(it.name.c_str(), exclude))
			continue;
		switch (it.type)
		{
		case DirItem::Dir:
			WriteTarDirectory(writer, it, exclude, rel_path, prefix);
			break;
		case DirItem::File:
			WriteTarFile(writer, it, exclude, rel_path, prefix);
			break;
		case DirItem::Stream:
			WriteTarStream(writer, it, rel_path, prefix);
			break;
		case DirItem::Invalid: {
			//HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
			//CONSOLE_SCREEN_BUFFER_INFO bi;
			//GetConsoleScreenBufferInfo(hStdOut, &bi);
			//SetConsoleTextAttribute(hStdOut, FOREGROUND_RED| FOREGROUND_INTENSITY);
			wcout << prefix << L"* " << it.name.filename().c_str() << L"  *** not found *** " << endl;
			//SetConsoleTextAttribute(hStdOut, bi.wAttributes);
			}
			break;
		}
	}
}

void WriteTarDirectory(ITarWriter * writer, const DirItem& item, const vector<wstring>& exclude,
	const filesystem::path& rel_path, const wstring& prefix)
{
	wcout << prefix << L"> " << item.name.filename().c_str() << endl;
//	TarFiles(writer, directory_items(item.name), exclude, rel_path / item.name.filename(), prefix + L"  ");
	TarFiles(writer, get_files(item.name), exclude, rel_path / item.name.filename(), prefix + L"  ");
	//wcout << L"end " << item.c_str() << endl;
}

void PrintFileData(const DirItem& item, const filesystem::path& rel_path, const wstring& prefix)
{
	//wcout << prefix << L"+ " << item.name.c_str() << endl;
	//wcout << prefix << L"+ " << (rel_path / item.name.filename()).c_str() << endl;
	wcout << prefix << L"+ " << item.name.filename().c_str() << L"   " << item.size << endl;
}

void WriteTarFile(ITarWriter * writer, const DirItem& item, const vector<wstring>& exclude, const filesystem::path& rel_path, const wstring& prefix)
{
	if (writer->IsMyFile(item.name)) // do not add tar itself to the tar
		return;

	PrintFileData(item, rel_path, prefix);

	FileSimple fs(item.name.c_str());
	if (!fs.IsOpen())
	{
		wcout << prefix << L"* ********** error opening " << item.name << endl;
		return;
	}
	// GetFileInformationByHandle  BY_HANDLE_FILE_INFORMATION
	//	write streams
	TarFiles(writer, get_streams(item.name, L""), exclude, rel_path, prefix);
}

void WriteTarStream(ITarWriter * writer, const DirItem& item, const filesystem::path& rel_path, const wstring& prefix)
{
	PrintFileData(item, rel_path, prefix);

	wstring fn = item.name.c_str();
	// directory name "dir\\:stream" -> "dir:stream"
	auto ix = fn.find(L"\\:");
	if (ix != wstring::npos)
		fn.erase(ix, 1);
	FileSimple fs(fn.c_str());
	if (!fs.IsOpen())
	{
		wcout << prefix << L"* ********** error opening " << fn << endl;
		return;
	}
}

int Tar(int argc, TCHAR **argv)
{
	if (argc < 3 || _tcscmp(argv[2], L"/?") == 0)
		return ShowHelpTar(filesystem::path(argv[0]).filename());

	bool test = false;
	ULONGLONG part_size = 0;
	wstring pass;
	filesystem::path tarname;
	std::vector<wstring> exclude;
	std::vector<filesystem::path> items;

	for (int n = 2; n < argc; ++n)
	{
		wstring_view param(argv[n]);
		if (param == L"/t")
			test = true;
		else if (starts_with(param, L"/b:"))
			part_size = ReadSize(param.substr(3));
		else if (starts_with(param, L"/p:"))
			pass = param.substr(3);
		else if (starts_with(param, L"/e:"))
			exclude = split(param.substr(3), L';');
		else if (starts_with(param, L"/"))
			throw invalid_argument("unrecognized option");
		else if (tarname.empty())
			tarname = param;
		else
			items.emplace_back(param);
	}

	bool set_ext = tarname.empty() || !tarname.has_extension();
	if (tarname.empty())
		tarname = filesystem::current_path().filename();
	if (set_ext)
		tarname.replace_extension(L".star");

	wcout << L"Writing " << tarname.c_str();
	if(test)
		wcout << L", test";
	if(part_size)
		wcout << L", block size=" << part_size;
	if (!pass.empty())
		wcout << L", pass=" << pass;
	if (!exclude.empty())
		wcout << L", exclude=" << exclude;
	if (!items.empty())
		wcout << L", items=" << items;
	else
		wcout << L", current dir";
	wcout << endl << endl;

	auto gen = items.empty() ?
		get_files(filesystem::current_path()) : 
		get_files_multi(items);

	unique_ptr<ITarWriter> writer(
		test ? (ITarWriter*)new TarWriterTest() :
		(ITarWriter*)new TarWriterFiles(tarname, part_size) );

	TarFiles(writer.get(), std::move(gen), exclude, L"", L"");

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
