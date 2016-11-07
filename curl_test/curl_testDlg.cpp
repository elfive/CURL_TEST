
// curl_testDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "curl_test.h"
#include "curl_testDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



Ccurl_testDlg* pDlgSelf;
Ccurl_testDlg::Ccurl_testDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_CURL_TEST_DIALOG, pParent),
	m_HttpCallback(this),
	m_HttpClient(m_HttpCallback)
{
	pDlgSelf = this;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

Ccurl_testDlg::~Ccurl_testDlg()
{
}

void Ccurl_testDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(Ccurl_testDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &Ccurl_testDlg::OnStart)
	ON_BN_CLICKED(IDC_BUTTON2, &Ccurl_testDlg::OnStop)
	ON_BN_CLICKED(IDC_BUTTON3, &Ccurl_testDlg::OnStart2)
	ON_BN_CLICKED(IDC_BUTTON4, &Ccurl_testDlg::OnStop2)
	ON_BN_CLICKED(ctl_GET, &Ccurl_testDlg::OnGET)
	ON_BN_CLICKED(ctl_POST, &Ccurl_testDlg::OnPOST)
	ON_BN_CLICKED(ctl_POSTS, &Ccurl_testDlg::OnPOSTS)
	ON_BN_CLICKED(ctl_GETS, &Ccurl_testDlg::OnGETS)
END_MESSAGE_MAP()

BOOL Ccurl_testDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);
	return TRUE;
}

void Ccurl_testDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR Ccurl_testDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

Ccurl_testDlg::HttpCallback::HttpCallback(Ccurl_testDlg * pDlg)
{
	m_pDlg = pDlg;
}

void Ccurl_testDlg::HttpCallback::OnSuccess(wstring ResponseHeader, wstring ResponseContent, Client_long numLong, void * userData)
{
	string userdata = ((string *)userData)->c_str();
	CString Msg = L"process time:%ld\n%s";
	if (userdata == "你大爷") 
	{
		Msg.Format(Msg, (long)numLong, CString(ResponseContent.c_str()));
		m_pDlg->MessageBox(Msg, L"你大爷");
	}
	if (userdata == "蛤蛤") 
	{
		Msg.Format(Msg, (long)numLong, CString(ResponseContent.c_str()));
		m_pDlg->MessageBox(Msg, L"蛤蛤");
	}
	return;
}

void Ccurl_testDlg::HttpCallback::OnTransferProgress(WebClient::TransferProgressInfo & ProgressInfo, void * userdata)
{
	CString Progress;
	long long h = 0, m = 0, s = 0;
	h = ProgressInfo.SecondRemain / 3600;
	m = (ProgressInfo.SecondRemain - h * 3600) / 60;
	s = ProgressInfo.SecondRemain - h * 3600 - m * 60;

	Progress.Format(L"下载进度：%.2f%%\t\t下载速度：%.2fKB/s\t\t剩余字节数：%lldBytes\t\t已下载字节数：%lld\t\t总大小：%lld\t\t剩余时间：%lldh:%lldm:%llds\n", ProgressInfo.TransferedPercent, (double)(ProgressInfo.AvgSpeed / 1024), ProgressInfo.BytesRemain, ProgressInfo.BytesTransfered, ProgressInfo.FileTotalBytes, h, m, s);


	OutputDebugString(Progress);
}

void Ccurl_testDlg::HttpCallback::OnTransferEvent(WebClient::TransferEvent Event, string message, Client_long numLong, void * userdata)
{
	CString Msg = L"";
	switch (Event)
	{
	case WebClient::TRANSFER_CONNECTING:
		Msg.Format(L"Download: %s [%s]\n", L"正在连接", CString(message.c_str()));
		OutputDebugString(Msg);
		break;
	case WebClient::TRANSFER_START:
		Msg.Format(L"Download: %s [%s]\n", L"正在下载", CString(message.c_str()));
		OutputDebugString(Msg);
		break;
	case WebClient::TRANSFER_CANCELING:
		Msg.Format(L"Download: %s [%s]\n", L"正在取消下载", CString(message.c_str()));
		OutputDebugString(Msg);
		break;
	case WebClient::TRANSFER_CANCELED:
		Msg.Format(L"Download: %s 下载用时：%lds [%s]\n", L"停止下载", (long)numLong, CString(message.c_str()));
		OutputDebugString(Msg);
		break;
	case WebClient::TRANSFER_COMPELETE:
		Msg.Format(L"Download: %s 用时%lds [%s]\n", L"下载完成", (long)numLong, CString(message.c_str()));
		OutputDebugString(Msg);
		break;
	case WebClient::TRANSFER_CLOSED:
		Msg.Format(L"Download: %s [%s]\n", L"下载过程出现异常导致下载中断", CString(message.c_str()));
		OutputDebugString(Msg);
		break;
	default:
		break;
	}

}

void Ccurl_testDlg::HttpCallback::OnFailed(string ErrDescription, void * userdata)
{
	//m_pDlg->MessageBox(CString(ErrDescription.c_str()), L"ActionFailed", MB_OK + MB_ICONERROR);
	OutputDebugString(CString(ErrDescription.c_str()));
}

void Ccurl_testDlg::OnStart()
{

	downloadid1 = m_HttpClient.DownloadFile("http://cdnpatch.popkart.com/lexian/kart_patch/SBLREGQWSPIBYDS/PopKart_Setup_P2162.exe", "C:\\Users\\Administrator\\Desktop\\popkart.exe", "", &userdata);
}

void Ccurl_testDlg::OnStop()
{
	if (downloadid1 != 0)
	{
		m_HttpClient.CloseClient(downloadid1);
	}
}

void Ccurl_testDlg::OnStart2()
{
	downloadid2 = m_HttpClient.DownloadFile("http://cdnpatch.popkart.com/lexian/kart_patch/SBLREGQWSPIBYDS/PopKart_Patch_P2162.exe", "C:\\Users\\Administrator\\Desktop\\popkart_patch.exe", "", &userdata);

}

void Ccurl_testDlg::OnStop2()
{
	if (downloadid2 != 0)
	{
		m_HttpClient.CloseClient(downloadid2);
	}
}


void Ccurl_testDlg::OnPOST()
{
	WebClient::HttpHeader headers(m_header_sep);
	headers.AddRequestHeader("FUCK", "BAIDU");
	m_HttpClient.POST("http://api.map.baidu.com/highacciploc/v1?qterm=pc&ak=FpsbxG8zCKo02Ot6mU1KutVelSHzPDgI&extensions=1&coord=bd09ll", "", "", &userdata2);
}


void Ccurl_testDlg::OnGET()
{
	m_HttpClient.GET("http://api.map.baidu.com/location/ip?ak=FpsbxG8zCKo02Ot6mU1KutVelSHzPDgI&coor=bd09ll", "", &userdata);
}


void Ccurl_testDlg::OnPOSTS()
{
	// TODO: 在此添加控件通知处理程序代码
}


void Ccurl_testDlg::OnGETS()
{
	m_HttpClient.GETS("https://api.map.baidu.com/location/ip?ak=FpsbxG8zCKo02Ot6mU1KutVelSHzPDgI&coor=bd09ll", "", &userdata);

}
