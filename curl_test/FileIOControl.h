#pragma once
#include "stdafx.h"
#include <sstream>
#include "StringProcess.h"

#define FILE_ACCESS_MAX_SIZE 255

class FileAccess
{
private:
	typedef enum
	{
		REAL_DATA_LENGTH = 0,
		REAL_FILE_SIZE = 1
	}CGFFLAG;
	bool WriteCfg(CGFFLAG cFlag, CString strValue);
	bool WriteCfg(CGFFLAG cFlag, ULONGLONG ullValue);


public:
	FileAccess();
	~FileAccess();
	BOOL Open(string strFileName, UINT nOpenFlags, CFileException* pError = NULL);
	ULONGLONG GetRealDataLength();
	bool SetLength(ULONGLONG ullFileLength);
	void Close();
	ULONGLONG GetLength();
	void Write(const void* lpBuf, UINT nCount);
	void Flush();



	operator CFile*()
	{
		return &this->m_file;
	};

private:
	CFile m_file;
	string m_filepath = "";
	string m_cfgpath = "";
	ULONGLONG m_datalength = 0;




};
