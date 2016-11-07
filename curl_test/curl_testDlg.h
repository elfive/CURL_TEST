#pragma once
#include "WebClient.h"

#include<string>
using namespace std;

class Ccurl_testDlg : public CDialogEx
{
public:
	class HttpCallback : public WebClient::ResponseCallback
	{
	public:
		HttpCallback::HttpCallback(Ccurl_testDlg* pDlg);
		void OnSuccess(wstring ResponseHeader, wstring ResponseContent, Client_long numLong, void * userData = NULL);
		void OnTransferProgress(WebClient::TransferProgressInfo & ProgressInfo, void * userdata);
		void OnTransferEvent(WebClient::TransferEvent Event, string message, Client_long numLong, void * userdata);
		void OnFailed(string ErrDescription, void * userdata = NULL);
	private:
		Ccurl_testDlg* m_pDlg;
	};

	HttpCallback m_HttpCallback;
	WebClient m_HttpClient;


	string userdata = "Äã´óÒ¯";
	string userdata2 = "¸ò¸ò";

	int downloadid1 = 0;
	int downloadid2 = 0;

	Ccurl_testDlg(CWnd* pParent = NULL);
	~Ccurl_testDlg();
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CRUL_TEST_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	HICON m_hIcon;

	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

private:

public:
	afx_msg void OnStart();
	afx_msg void OnStop();
	afx_msg void OnStart2();
	afx_msg void OnStop2();
	afx_msg void OnPOST();
	afx_msg void OnGET();
	afx_msg void OnPOSTS();
	afx_msg void OnGETS();
};