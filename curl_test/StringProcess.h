#pragma once
#include "stdafx.h"
#include <windows.h>
#include <shlwapi.h>
#include <string>
using namespace std;

string AsciiToUnicode(const string & szOrig);
wstring UTF8ToUnicode(const string & str);
string UnicodeToUTF8(const wstring & str);
wstring AnsiToUnicode(const string & str);
string GBKToUnicode(const string & str);
string wstringTostring(const wstring & str);
wstring stringTowstring(const string & str);
string CStringTostring(CString CStringstr);
CString stringToCString(string stringstr);
