#include "stdafx.h"
#include "FileIOControl.h"

FileAccess::FileAccess()
{
}

FileAccess::~FileAccess()
{
}

BOOL FileAccess::Open(string strFileName, UINT nOpenFlags, CFileException * pError)
{
	if (strFileName == "")return FALSE;
	m_filepath = strFileName;
	m_cfgpath = strFileName + ".cfg";

	if (m_file.Open(CString(strFileName.c_str()), nOpenFlags, pError))
	{
		m_datalength = GetRealDataLength();
		m_file.Seek(m_datalength, CFile::begin);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

bool FileAccess::SetLength(ULONGLONG ullFileLength)
{
	m_file.SetLength(ullFileLength);
	m_datalength = GetRealDataLength();
	m_file.Seek(m_datalength, CFile::begin);
	return WriteCfg(REAL_FILE_SIZE, ullFileLength);
}

void FileAccess::Close()
{
	WriteCfg(REAL_DATA_LENGTH, m_datalength);
	m_file.Close();
}

ULONGLONG FileAccess::GetLength()
{
	return m_file.GetLength();
}

ULONGLONG FileAccess::GetRealDataLength()
{
	CString size;
	GetPrivateProfileString(L"FILE", L"REALDATALENGTH", L"0", size.GetBuffer(FILE_ACCESS_MAX_SIZE), FILE_ACCESS_MAX_SIZE, CString(m_cfgpath.c_str()));
	size.ReleaseBuffer();

	stringstream strValue;
	strValue << CStringTostring(size).c_str();
	ULONGLONG datalength;
	strValue >> datalength;

	return datalength;
}

void FileAccess::Write(const void * lpBuf, UINT nCount)
{
	m_file.Write(lpBuf, nCount);
	m_datalength += nCount;
	WriteCfg(REAL_DATA_LENGTH, m_datalength);
	return;
}

void FileAccess::Flush()
{
	m_file.Flush();
}

bool FileAccess::WriteCfg(CGFFLAG cFlag, CString strValue)
{
	CString KeyName = L"";
	switch (cFlag)
	{
	case REAL_DATA_LENGTH:
		KeyName = L"WRITEDLENGTH";
		break;
	case REAL_FILE_SIZE:
		KeyName = L"TOTALSIZE";
		break;
	default:
		return false;
		break;
	}
	WritePrivateProfileString(L"FILE", KeyName, strValue, CString(m_cfgpath.c_str()));
	return true;
}

bool FileAccess::WriteCfg(CGFFLAG cFlag, ULONGLONG ullValue)
{
	CString KeyName = L"";
	switch (cFlag)
	{
	case REAL_DATA_LENGTH:
		KeyName = L"REALDATALENGTH";
		break;
	case REAL_FILE_SIZE:
		KeyName = L"TOTALSIZE";
		break;
	default:
		return false;
		break;
	}
	stringstream strValue;
	strValue << ullValue;
	char * cvalue = new char[FILE_ACCESS_MAX_SIZE];
	strValue >> cvalue;

	CString value(cvalue);
	delete[]cvalue;
	cvalue = NULL;

	WritePrivateProfileString(L"FILE", KeyName, value, CString(m_cfgpath.c_str()));
	return true;
}