
// curl_test.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// Ccurl_testApp: 
// �йش����ʵ�֣������ curl_test.cpp
//

class Ccurl_testApp : public CWinApp
{
public:
	Ccurl_testApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern Ccurl_testApp theApp;