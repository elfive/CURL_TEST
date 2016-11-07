#pragma once
#include <string>
#include <unordered_map>
#include <queue>
#include <curl\curl.h>
#include <boost\thread\thread_pool.hpp>
#include <boost\bind\bind.hpp>
#include "FileIOControl.h"

#define PrintD(msg) WebClient::CURLClient::DebugPrint(__DATE__, __TIME__, __FILE__, __LINE__, __FUNCTION__, msg);
typedef time_t Client_long;

using namespace std; 
using namespace boost;

const string m_header_sep = "|";

class WebClient
{
public:

	typedef enum
	{
		HTTP_GET = 1,
		HTTP_POST = 2,
		HTTP_DOWNLOAD = 3,
		HTTP_UPLOAD = 4,
		HTTPS_GET = 5,
		HTTPS_POST = 6,
		HTTPS_DOWNLOAD = 7,
		HTTPS_UPLOAD = 8
	}RequestType;

	typedef enum
	{
		TRANSFER_CONNECTING = 0,
		TRANSFER_GET_LENGTH = 1,
		TRANSFER_GOT_LENGTH = 2,
		TRANSFER_LENGTH_ERROR = 3,
		TRANSFER_GET_LENGTH_FAILED = 4,
		TRANSFER_START = 5,
		TRANSFER_CANCELING =6,
		TRANSFER_CANCELED = 7,
		TRANSFER_COMPELETE = 8,
		TRANSFER_CLOSED = 16,
	}TransferEvent;

	struct TransferProgressInfo
	{
		curl_off_t FileTotalBytes = 0;
		curl_off_t BytesTransfered = 0;
		curl_off_t BytesRemain = 0;
		curl_off_t SecondRemain = 0;
		double TransferedPercent = 0;
		double AvgSpeed = 0;
	};

	struct HttpProxyInfo
	{
		string host = "";
		string port = "";
		string username = "";
		string password = "";
		curl_proxytype type = CURLPROXY_HTTP;
	};

	struct HttpRequestInfo
	{
		int id = 0;
		string url = "";
		string postdata = "";
		string header = "";
		bool bRedirecte = true;
		RequestType RequestType = HTTP_POST;
		void * userdata = NULL;
		int timeout = 10;
		int CallbackTimeInterval = 1000;	//ms
		bool bEnableProxy = false;
		HttpProxyInfo proxyInfo;
		string dFilePath = "";
	};

	class HttpHeader
	{
	public:
		HttpHeader(string seperator = m_header_sep);
		~HttpHeader();

		//ÖØÔØ¸³ÖµºÅ
		HttpHeader& operator=(HttpHeader &header)
		{
			m_header = header.m_header;
			m_seperator = header.m_seperator;
			return *this;
		}

		HttpHeader& operator=(string &header)
		{
			m_header = header;
			return *this;
		}

		operator string()
		{
			return this->m_header;
		};

		void AddRequestHeader(string Key, string Value);
		void Reset();

	private:
		string m_header = "";
		string m_seperator = m_header_sep;
		const string m_defaultheader = "Content-Type: application/x-www-form-urlencoded" + m_seperator;

	};

	class ResponseCallback
	{
	public:
		virtual void OnSuccess(wstring ResponseHeader, wstring ResponseContent, Client_long numLong, void * userData = NULL) = 0;
		virtual void OnTransferProgress(TransferProgressInfo & ProgressInfo, void * userdata) = 0;
		virtual void OnTransferEvent(TransferEvent Event, string message, Client_long numLong, void * userdata) = 0;
		virtual void OnFailed(string ErrDescription, void * userdata = NULL) = 0;
	};

	WebClient(ResponseCallback & callback);
	~WebClient();

	//Http
	int POST(string url, string postdata, string header, void * userdata);
	int GET(string url, string header, void * userdata);
	int DownloadFile(string url, string savepath, string header, void * userdata);

	//Https
	int GETS(string url, string header, void * userdata);

	bool CloseClient(int ClientID);

private:
	class CURLClient
	{
	public:
		CURLClient(HttpRequestInfo * pData, ResponseCallback & callback);
		~CURLClient();
		wstring m_responsecontent = L"";
		wstring m_responseheader = L"";
		FileAccess m_FileReadAndWrite;
		curl_off_t TotalFileSize = 0;
		curl_off_t ResumeBytes = -1;
		bool bStopTransfer = false;
		bool isFileOpened = false;
		static void DebugPrint(const char * date, const char * time, const char * file, const int line, const char * func, string msg);

	private:
		typedef enum 
		{
			NOTINITIALIZED = 0,
			INITIALIZING = 1,
			INITIALIZED = 2,
			DISCONNECTED = 3,
			CONNECT = 4,
			TRANSFERING = 5,
			DISCONNECTING = 6
		}ClientStatus;

		struct WriteFileData
		{
			CURLClient * pClient = NULL;
			FileAccess * FileHandle = NULL;
		};
		WriteFileData m_DATA;
		ClientStatus m_Status;

		HttpRequestInfo * m_ReuqestData = NULL;
		bool m_isInited = false;
		CURL * m_CURL = NULL;
		string m_header = "";
		string m_ResponseData = "";
		string m_ErrInfo = "";
		ResponseCallback & m_Callback;
		TransferProgressInfo m_ProgressInfo;
		thread_group m_timer_thread;
		bool m_killtimer = false;
		int m_TimerInterval = 0;
		time_t m_ProcessTime = 0;

		struct curl_slist * m_RequestHeader = NULL;
		bool m_isSetTimer = false;

		void SetProxy(HttpProxyInfo * ProxyInfo);
		void SetTimer();
		void KillTimer();

		static int GetResponseData(char * data, size_t size, size_t nmemb, void * callbackdata);
		static size_t GetResponseHeader(char * data, size_t size, size_t nmemb, void * callbackdata);
		static int ProgressCallback(void * clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
		static size_t WriteToFile(char * buffer, size_t size, size_t nmemb, void * userdata);
		static void OnTimer(void * pData);



	public:

		void SendRequest();
		void StopTransfering();
		bool InitCURL();
		void AddRequestHeader(string Header);
		void SetRequestHeader(string Header = "");
		void Clear();
	};
	CURLClient * m_Client;
	typedef unordered_map<int ,CURLClient *> CURLMap;
	CURLMap m_ClientMap;

	typedef CURLMap::iterator MapPointor;
	MapPointor m_Pointor;

	int m_ClientID = 0;

	typedef queue<HttpRequestInfo *> RequestQueue;
	RequestQueue m_RequestQueue;

	bool bExitThread = false;
	thread_group m_ThreadGroup;

	ResponseCallback & m_Callback;
	HttpRequestInfo * m_ThreadData;



	static void SendHttpReuqestThread(void * pData);
	static void Distributor(void * pData);
};
