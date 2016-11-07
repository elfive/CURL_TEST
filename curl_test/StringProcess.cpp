#include "StringProcess.h"
#include "stdafx.h"
#include <string>
using namespace std;

#define IS_NUMBER(c)        ((c) && (((c) >= '0' && (c) <= '9') || ((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F')))

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shlwapi.lib")

LPSTR WINAPI WToA(LPCWSTR wideStr)
{
	int nSize = WideCharToMultiByte(GetACP(), 0, wideStr, -1, NULL, 0, NULL, NULL);
	LPSTR szAnsiStr = (LPSTR)malloc(nSize + 1);
	RtlZeroMemory(szAnsiStr, nSize + 1);
	WideCharToMultiByte(GetACP(), 0, wideStr, -1, szAnsiStr, nSize, NULL, NULL);

	return szAnsiStr;
}

string AsciiToUnicode(const string & szOrig)
{
	wstring wStr;
	const char * curChar = szOrig.c_str();

	while (*curChar)
	{
		if (curChar[0] == '\\' &&
			(curChar[1] == 'u' || curChar[1] == 'U') &&
			IS_NUMBER(curChar[2]) &&
			IS_NUMBER(curChar[3]) &&
			IS_NUMBER(curChar[4]) &&
			IS_NUMBER(curChar[5]))
		{
			char hex[8] = { '0', 'x' };
			hex[2] = curChar[2];
			hex[3] = curChar[3];
			hex[4] = curChar[4];
			hex[5] = curChar[5];

			int i;
			StrToIntExA(hex, STIF_SUPPORT_HEX, &i);
			wStr += (wchar_t)i;

			curChar += 6;
		}
		else
		{
			wStr += (wchar_t)*curChar;
			curChar++;
		}
	}

	if (wStr.size())
	{
		char * szAnsiStr = WToA(wStr.c_str());
		string ret(szAnsiStr);
		free((void*)szAnsiStr);
		return ret;
	}

	return "";
}

wstring UTF8ToUnicode(const string & str)
{
	DWORD dwUnicodeLen;
	wchar_t * pwText;
	//获得转换后的长度，并分配内存
	dwUnicodeLen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
	pwText = new wchar_t[dwUnicodeLen];
	//转为Unicode
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, pwText, dwUnicodeLen);
	wstring returnstr = pwText;
	delete[]pwText;
	pwText = NULL;
	return returnstr;
}

string UnicodeToUTF8(const wstring & str)
{
	char*     pElementText;
	int    iTextLen;
	// wide char to multi char
	iTextLen = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, NULL, 0, NULL, NULL);
	pElementText = new char[iTextLen + 1];
	memset((void*)pElementText, 0, sizeof(char) * (iTextLen + 1));
	::WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, pElementText, iTextLen, NULL, NULL);
	string strText;
	strText = pElementText;
	delete[] pElementText;
	pElementText = NULL;
	return strText;
}

wstring AnsiToUnicode(const string & str)
{
	setlocale(LC_ALL, "chs");
	size_t len = strlen(str.c_str());
	wchar_t * pwide;
	pwide = (wchar_t*)malloc(len * sizeof(wchar_t));
	size_t wlen = 0;
	mbstowcs_s(&wlen, pwide, len, str.c_str(), _TRUNCATE);
	if (wlen >= 0)return wstring(pwide, wlen);
	else return wstring();
}

string GBKToUnicode(const string & str)
{
	//1.转为UNICODE
	int wc = MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)str.size(), 0, 0);

	wchar_t* wUnicode = new wchar_t[wc];
	//wUnicode[wc] = 0;

	MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)str.size(), wUnicode, wc);

	//2.UNICODE->ANSI
	wc = WideCharToMultiByte(CP_UTF8, 0, wUnicode, (int)wcslen(wUnicode), 0, 0, NULL, NULL);
	char * sUnicode = new char[wc];
	//sUnicode[wc] = 0;

	WideCharToMultiByte(CP_UTF8, 0, wUnicode, (int)wcslen(wUnicode), sUnicode, wc, NULL, NULL);

	string _str = sUnicode;

	if (sUnicode) { delete[] sUnicode; sUnicode = NULL; }
	if (wUnicode) { delete[] wUnicode; wUnicode = NULL; }

	return _str;

}

string wstringTostring(const wstring & str)
{
	size_t len = str.size() * 4;
	setlocale(LC_CTYPE, "");
	char *p = new char[len];
	size_t lens = 0;
	wcstombs_s(&lens, p, len, str.c_str(), _TRUNCATE);
	string str1(p);
	delete[] p;
	return str1;
}

wstring stringTowstring(const string & str)
{
	setlocale(LC_CTYPE, "");     //必须调用此函数
	size_t len = str.size() * 2;// 预留字节数
	wchar_t * pwide;
	pwide = (wchar_t*)malloc(len * sizeof(wchar_t));
	size_t wlen = 0;
	mbstowcs_s(&wlen, pwide, len, str.c_str(), _TRUNCATE);
	if (wlen >= 0)return wstring(pwide, wlen);
	else return wstring();
}

string CStringTostring(CString CStringstr)
{
	CT2CA pszConvertedAnsiString(CStringstr);
	string returnstring(pszConvertedAnsiString);
	return returnstring;
}

CString stringToCString(string stringstr)
{
	CString returnCString(stringstr.c_str());
	return returnCString;
}