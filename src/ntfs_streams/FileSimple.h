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

class FileSimple
{
public:
	FileSimple() : m_hFile(INVALID_HANDLE_VALUE) {}
	FileSimple(LPCTSTR name, bool write = false, bool sequential = true)
		: m_hFile(INVALID_HANDLE_VALUE)
	{
		if (name) Open(name, write, sequential);
	}
	~FileSimple() { Close(); }
	bool Open(LPCTSTR name, bool write, bool sequential)
	{
		m_hFile = CreateFile(name, write ? GENERIC_WRITE : GENERIC_READ,
			write ? 0 : FILE_SHARE_READ, 0, write ? CREATE_ALWAYS : OPEN_EXISTING,
			sequential ? FILE_FLAG_SEQUENTIAL_SCAN : 0, 0);
		return IsOpen();
	}
	bool OpenRW(LPCTSTR name) // opens for read-write
	{
		m_hFile = CreateFile(name, GENERIC_WRITE | GENERIC_READ, 0, 0, OPEN_ALWAYS, 0, 0);
		return IsOpen();
	}
	bool IsOpen() { return m_hFile != INVALID_HANDLE_VALUE; }
	void Close()
	{
		if (IsOpen()) CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}
	DWORD SetPosition(LONG offset, DWORD from = FILE_BEGIN)//FILE_CURRENT, FILE_END
	{
		if (!IsOpen()) return 0;
		return SetFilePointer(m_hFile, offset, 0, from);
	}
	BOOL SetEOF()
	{
		if (!IsOpen()) return FALSE;
		return SetEndOfFile(m_hFile);
	}
	DWORD Read(void *buffer, DWORD count)
	{
		if (!IsOpen()) return 0;
		if (count == 0) return 0;
		if (!ReadFile(m_hFile, buffer, count, &count, 0)) return 0;
		return count;
	}
	DWORD Write(const void *buffer, DWORD count)
	{
		if (!IsOpen()) return 0;
		if (count == 0) return 0;
		if (!WriteFile(m_hFile, buffer, count, &count, 0)) return 0;
		return count;
	}
	DWORD GetLength()
	{
		if (!IsOpen()) return 0;
		return GetFileSize(m_hFile, 0);
	}
	HANDLE Handle() { return m_hFile; }
protected:
	HANDLE  m_hFile;
};

