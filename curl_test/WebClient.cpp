#include "stdafx.h"
#include "WebClient.h"
#include "StringProcess.h"
#include <sys/stat.h>
using namespace std;



static char error_buffer[CURL_ERROR_SIZE]; 

WebClient::CURLClient::CURLClient(HttpRequestInfo * pData, ResponseCallback & callback)
	:m_Callback(callback)
{
	m_ReuqestData = pData;
	m_Status = NOTINITIALIZED;
}

WebClient::CURLClient::~CURLClient()
{
}

bool WebClient::CURLClient::InitCURL()
{
	if (m_Status != NOTINITIALIZED) return true;

	m_Status = INITIALIZING;

	CURLcode res;
	string Msg = "";

	m_CURL = curl_easy_init();
	if (NULL == m_CURL)
	{
		Msg = "Failed to create CURL connection";
		m_Callback.OnFailed(Msg, m_ReuqestData->userdata);
		PrintD(Msg);
		goto exitthread;
	}

	switch (m_ReuqestData->RequestType)
	{
	case HTTP_POST:
	{
		res = curl_easy_setopt(m_CURL, CURLOPT_POSTFIELDS, m_ReuqestData->postdata.c_str());
		if (res != CURLE_OK)
		{
			Msg = "Failed to set PostData\nError:\n" + (string)error_buffer;
			m_Callback.OnFailed(Msg, m_ReuqestData->userdata);
			PrintD(Msg);
			goto exitthread;
		}
		curl_easy_setopt(m_CURL, CURLOPT_POSTFIELDSIZE, (long)strlen(m_ReuqestData->postdata.c_str()));
	}
	case HTTP_GET:
	{
		//设置获取响应体回调函数
		res = curl_easy_setopt(m_CURL, CURLOPT_WRITEFUNCTION, GetResponseData);
		if (res != CURLE_OK)
		{
			Msg = "Failed to set content callback function\nErrorBuffer:\n" + (string)error_buffer;
			m_Callback.OnFailed(Msg, m_ReuqestData->userdata);
			PrintD(Msg);
			goto exitthread;
		}
		m_responsecontent = L"";

		//设置获取响应头回调函数
		res = curl_easy_setopt(m_CURL, CURLOPT_HEADERFUNCTION, GetResponseHeader);
		if (res != CURLE_OK)
		{
			Msg = "Failed to set header callback function\nErrorBuffer:\n" + (string)error_buffer;
			m_Callback.OnFailed(Msg, m_ReuqestData->userdata);
			PrintD(Msg);
			goto exitthread;
		}
		m_responseheader = L"";

		//设置TimeOut
		res = curl_easy_setopt(m_CURL, CURLOPT_TIMEOUT, m_ReuqestData->timeout);
		if (res != CURLE_OK)
		{
			Msg = "Failed to set TimeOut\nError:\n" + (string)error_buffer;
			m_Callback.OnFailed(Msg, m_ReuqestData->userdata);
			PrintD(Msg);
			goto exitthread;
		}

		//设置响应体函数的最后一个参数
		res = curl_easy_setopt(m_CURL, CURLOPT_WRITEDATA, this);
		res = curl_easy_setopt(m_CURL, CURLOPT_HEADERDATA, this);
		break;
	}
	case HTTP_DOWNLOAD:
	{	
		m_Callback.OnTransferEvent(TRANSFER_CONNECTING, m_ReuqestData->url, 0, m_ReuqestData->userdata);
		//设置URL
		res = curl_easy_setopt(m_CURL, CURLOPT_URL, m_ReuqestData->url.c_str());
		if (res != CURLE_OK)
		{
			Msg = "Failed to set URL\nError:\n" + (string)error_buffer;
			m_Callback.OnFailed(Msg, m_ReuqestData->userdata);
			PrintD(Msg);
			goto exitthread;
		}

		//获取文件大小
		curl_easy_setopt(m_CURL, CURLOPT_HEADER, 1);    //只需要header头  
		curl_easy_setopt(m_CURL, CURLOPT_NOBODY, 1);    //不需要body  
		//curl_easy_setopt(m_CURL, CURLOPT_WRITEFUNCTION, GotFileLength);

		m_Callback.OnTransferEvent(TRANSFER_GET_LENGTH, "", 0, m_ReuqestData->userdata);
		res = curl_easy_perform(m_CURL);
		if (res == CURLE_OK)
		{
			double size = 0;
			if (CURLE_OK != curl_easy_getinfo(m_CURL, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &size))
			{
				TotalFileSize = -1;
				Msg = "Failed to get download file size.\nError:\n" + (string)error_buffer;
				m_Callback.OnTransferEvent(TRANSFER_GET_LENGTH_FAILED, Msg, 0, m_ReuqestData->userdata);
				PrintD(Msg);
				goto exitthread;
			}
			TotalFileSize = (curl_off_t)size;

			if (TotalFileSize <= 0)
			{
				Msg = "Can't get the file size.but will continue to download";
				m_Callback.OnTransferEvent(TRANSFER_LENGTH_ERROR, Msg, TotalFileSize, m_ReuqestData->userdata);
				PrintD(Msg);
			}
			else 
			{
				m_Callback.OnTransferEvent(TRANSFER_GOT_LENGTH, m_ReuqestData->url, TotalFileSize, m_ReuqestData->userdata);
			}
		}
		else 
		{
			Msg = "Failed to set URL\nError:\n" + (string)error_buffer;
			m_Callback.OnTransferEvent(TRANSFER_GET_LENGTH_FAILED, Msg, 0, m_ReuqestData->userdata);
			PrintD(Msg);
			goto exitthread;
		}
		if (m_RequestHeader != NULL)
			curl_slist_free_all(m_RequestHeader);
		curl_easy_reset(m_CURL);
		
		//下载文件并写入硬盘
		BOOL RES = m_FileReadAndWrite.Open(m_ReuqestData->dFilePath.c_str(), CFile::modeCreate | CFile::modeWrite | CFile::modeNoInherit | CFile::modeNoTruncate, NULL);

		if (!RES)
		{
			Msg = "open file failed";
			m_Callback.OnTransferEvent(TRANSFER_CLOSED, Msg, 0, m_ReuqestData->userdata);
			PrintD(Msg);
			goto exitthread;
		}

		isFileOpened = true;
		m_FileReadAndWrite.SetLength(TotalFileSize);

		ResumeBytes = (curl_off_t)m_FileReadAndWrite.GetRealDataLength();
		if (ResumeBytes > 0)
		{
			// Set a point to resume transfer
			curl_easy_setopt(m_CURL, CURLOPT_RESUME_FROM_LARGE, ResumeBytes);
		}

		//设置获取响应体回调函数
		res = curl_easy_setopt(m_CURL, CURLOPT_WRITEFUNCTION, WriteToFile);
		if (res != CURLE_OK)
		{
			Msg = "Failed to set content callback function\nErrorBuffer:\n" + (string)error_buffer;
			m_Callback.OnFailed(Msg, m_ReuqestData->userdata);
			PrintD(Msg);
			goto exitthread;

		}

		m_DATA.FileHandle = &m_FileReadAndWrite;
		m_DATA.pClient = this;

		res = curl_easy_setopt(m_CURL, CURLOPT_WRITEDATA, (void*)&m_DATA);
		res = curl_easy_setopt(m_CURL, CURLOPT_NOPROGRESS, false);
		res = curl_easy_setopt(m_CURL, CURLOPT_HEADER, false);							//需要关闭header输出
		res = curl_easy_setopt(m_CURL, CURLOPT_XFERINFOFUNCTION, ProgressCallback);

		res = curl_easy_setopt(m_CURL, CURLOPT_XFERINFODATA, this);
		curl_easy_setopt(m_CURL, CURLOPT_MAXREDIRS, 5);									//设置重定向的最大次数  
		curl_easy_setopt(m_CURL, CURLOPT_FOLLOWLOCATION, true);							//设置301、302跳转跟随location  
		break;
	}
	case HTTP_UPLOAD:
	{
		break;
	}
	case HTTPS_GET:
	{
		curl_easy_setopt(m_CURL, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(m_CURL, CURLOPT_SSL_VERIFYHOST, 0L);

		//设置获取响应体回调函数
		res = curl_easy_setopt(m_CURL, CURLOPT_WRITEFUNCTION, GetResponseData);
		if (res != CURLE_OK)
		{
			Msg = "Failed to set content callback function\nErrorBuffer:\n" + (string)error_buffer;
			m_Callback.OnFailed(Msg, m_ReuqestData->userdata);
			PrintD(Msg);
			goto exitthread;
		}
		m_responsecontent = L"";

		//设置获取响应头回调函数
		res = curl_easy_setopt(m_CURL, CURLOPT_HEADERFUNCTION, GetResponseHeader);
		if (res != CURLE_OK)
		{
			Msg = "Failed to set header callback function\nErrorBuffer:\n" + (string)error_buffer;
			m_Callback.OnFailed(Msg, m_ReuqestData->userdata);
			PrintD(Msg);
			goto exitthread;
		}
		m_responseheader = L"";


		//设置响应体函数的最后一个参数
		res = curl_easy_setopt(m_CURL, CURLOPT_WRITEDATA, this);
		res = curl_easy_setopt(m_CURL, CURLOPT_HEADERDATA, this);
		break;
	}
	case HTTPS_POST:
	{}
	case HTTPS_DOWNLOAD:
	{}
	case HTTPS_UPLOAD:
	{}
	default:
	{
		break;
	}
	}
	
	//设置错误信息缓冲块
	res = curl_easy_setopt(m_CURL, CURLOPT_ERRORBUFFER, error_buffer);
	if (res != CURLE_OK)
	{
		Msg = "Failed to set error buffer";
		m_Callback.OnFailed(Msg, m_ReuqestData->userdata);
		PrintD(Msg);
		goto exitthread;
	}

	curl_easy_setopt(m_CURL, CURLOPT_FOLLOWLOCATION, m_ReuqestData->bRedirecte);


	//设置请求头
	if (m_ReuqestData->header != "")
	{
		string::size_type pos1, pos2;
		pos2 = m_ReuqestData->header.find(m_header_sep);
		pos1 = 0;
		while (string::npos != pos2)
		{
			string tmp_header = m_ReuqestData->header.substr(pos1, pos2 - pos1);
			m_RequestHeader = curl_slist_append(m_RequestHeader, tmp_header.c_str());
			pos1 = pos2 + m_header_sep.size();
			pos2 = m_ReuqestData->header.find(m_header_sep, pos1);
		}

		/* set our custom set of headers */
		res = curl_easy_setopt(m_CURL, CURLOPT_HTTPHEADER, m_RequestHeader);
		if (res != CURLE_OK)
		{
			Msg = "Failed to set requestheader\nError:\n" + (string)error_buffer;
			m_Callback.OnFailed(Msg, m_ReuqestData->userdata);
			PrintD(Msg);
			goto exitthread;
		}
	}

	//设置URL
	res = curl_easy_setopt(m_CURL, CURLOPT_URL, m_ReuqestData->url.c_str());
	if (res != CURLE_OK)
	{
		Msg = "Failed to set URL\nError:\n" + (string)error_buffer;
		m_Callback.OnFailed(Msg, m_ReuqestData->userdata);
		PrintD(Msg);
		goto exitthread;
	}

	//设置http返回值大于400时请求失败
	res = curl_easy_setopt(m_CURL, CURLOPT_FAILONERROR, 1L);



	//设置代理
	SetProxy(&m_ReuqestData->proxyInfo);
	curl_easy_setopt(m_CURL, CURLOPT_HTTPPROXYTUNNEL, m_ReuqestData->bEnableProxy);

	m_Status = INITIALIZED;
	return true;

exitthread:
	m_Status = NOTINITIALIZED;
	Clear();
	return false;
}

void WebClient::CURLClient::SetProxy(HttpProxyInfo * ProxyInfo)
{
	if (m_Status != INITIALIZED || m_Status != INITIALIZING) return;
	if (ProxyInfo->host.size() != 0 && ProxyInfo->port.size() != 0)
	{
		curl_easy_setopt(m_CURL, CURLOPT_PROXY, ProxyInfo->host.c_str());
		curl_easy_setopt(m_CURL, CURLOPT_PROXYPORT, ProxyInfo->port.c_str());
		if (ProxyInfo->username.size() != 0)
		{
			curl_easy_setopt(m_CURL, CURLOPT_PROXYUSERPWD, (ProxyInfo->username+ ":" + ProxyInfo->password).c_str());
		}
		curl_easy_setopt(m_CURL, CURLOPT_PROXYTYPE, ProxyInfo->type);
	}
	else 
	{
		curl_easy_setopt(m_CURL, CURLOPT_PROXY, NULL);
	}
}

void WebClient::CURLClient::AddRequestHeader(string Header)
{
	if (m_Status != INITIALIZED || m_Status != INITIALIZING) return;
	if(Header!="")
		m_header = m_header + Header + m_header_sep;
	return;
}

void WebClient::CURLClient::SetRequestHeader(string Header)
{
	if (m_Status != INITIALIZED || m_Status != INITIALIZING) return;
	m_header = Header;
	return;
}

void WebClient::CURLClient::SendRequest()
{
	string Msg = "";
	CURLcode res;
	//执行
	if (m_Status == NOTINITIALIZED || m_Status == INITIALIZING)
	{
		Msg = "Client NOT initialized!";
		m_Callback.OnFailed(Msg, m_ReuqestData->userdata);
		PrintD(Msg);
		Clear();
		return;
	}

	if (m_Status >= CONNECT)
	{
		Msg = "Client is started connecting!";
		m_Callback.OnFailed("Client is started connecting!", m_ReuqestData->userdata);
		PrintD(Msg);
		return;
	}
	
	m_Status = CONNECT;

	switch (m_ReuqestData->RequestType) 
	{
	case HTTP_DOWNLOAD:
	case HTTP_UPLOAD:
		m_Callback.OnTransferEvent(TRANSFER_START, m_ReuqestData->url, 0, m_ReuqestData->userdata);
		SetTimer();
		break;
	default:
		break;
	}
	time_t start_time,end_time;
	time(&start_time);
	
	res = curl_easy_perform(m_CURL);
	
	time(&end_time);
	m_ProcessTime = end_time - start_time;

	switch (m_ReuqestData->RequestType)
	{
	case HTTP_GET:
	case HTTP_POST:
	{
		if (res != CURLE_OK)
		{
			Msg = "Failed to send POST/GET request\nError:\n" + (string)error_buffer;
			m_Callback.OnFailed(Msg, m_ReuqestData->userdata);
			PrintD(Msg);
		}
		else 
		{
			m_Callback.OnSuccess(m_responseheader, m_responsecontent, m_ProcessTime, m_ReuqestData->userdata);
		}
		break;
	}
	case HTTPS_GET:
	case HTTPS_POST: 
	{
		if (res != CURLE_OK)
		{
			Msg = "Failed to send HTTPS POST/GET request\nError:\n" + (string)error_buffer;
			m_Callback.OnFailed(Msg, m_ReuqestData->userdata);
			PrintD(Msg);
		}
		else
		{
			m_Callback.OnSuccess(m_responseheader, m_responsecontent, m_ProcessTime, m_ReuqestData->userdata);
		}
		break;
	}
	case HTTP_DOWNLOAD:
	case HTTP_UPLOAD:
	{
		m_Status = DISCONNECTING;
		KillTimer();
		if (bStopTransfer && (res == CURLE_OK || res == CURLE_ABORTED_BY_CALLBACK))
			m_Callback.OnTransferEvent(TRANSFER_CANCELED, m_ReuqestData->url, m_ProcessTime, m_ReuqestData->userdata);
		else if (m_ProgressInfo.BytesRemain == 0 && res == CURLE_OK)
			m_Callback.OnTransferEvent(TRANSFER_COMPELETE, m_ReuqestData->url, m_ProcessTime, m_ReuqestData->userdata);
		else
			m_Callback.OnTransferEvent(TRANSFER_CLOSED, curl_easy_strerror(res), 0, m_ReuqestData->userdata);
		break;
	}
	default:
	{
	}
	break;
	}
	
	if (isFileOpened) 
	{
		m_FileReadAndWrite.Close();
		isFileOpened = false;
	}

	Clear();
	return;
}

void WebClient::CURLClient::StopTransfering() 
{
	if (m_Status == CONNECT || m_Status == TRANSFERING) 
	{
		if(m_ReuqestData->RequestType == HTTP_DOWNLOAD || m_ReuqestData->RequestType == HTTP_UPLOAD || m_ReuqestData->RequestType == HTTPS_DOWNLOAD || m_ReuqestData->RequestType == HTTPS_DOWNLOAD)
			m_Callback.OnTransferEvent(TRANSFER_CANCELING, m_ReuqestData->url, 0, m_ReuqestData->userdata);
		bStopTransfer = true;
		KillTimer();
	}
}

void WebClient::CURLClient::Clear()
{
	if (m_CURL != NULL) 
	{
		StopTransfering();

		if (m_RequestHeader != NULL)
			curl_slist_free_all(m_RequestHeader);

		curl_easy_cleanup(m_CURL);
		m_Status = NOTINITIALIZED;
	}
}

void WebClient::CURLClient::OnTimer(void* pData) 
{
	CURLClient * pClient = (CURLClient *)pData;

	if (pClient->m_TimerInterval == 0)
	{
		pClient->m_isSetTimer = false;
	}
	else 
	{
		int interval = pClient->m_TimerInterval;
		do
		{
			pClient->m_Callback.OnTransferProgress(pClient->m_ProgressInfo, pClient->m_ReuqestData->userdata);
			Sleep(interval);
		} while (!pClient->m_killtimer);
	}
}

void WebClient::CURLClient::SetTimer() 
{
	if (m_isSetTimer) return;
	m_TimerInterval = m_ReuqestData->CallbackTimeInterval;
	m_timer_thread.create_thread(bind(&OnTimer, this));
	m_isSetTimer = true;
}

void WebClient::CURLClient::KillTimer() 
{
	if (!m_isSetTimer) return;
	m_killtimer = true;
	m_timer_thread.join_all();
	m_TimerInterval = 0;
	m_isSetTimer = false;
}

//回调函数
int WebClient::CURLClient::GetResponseData(char* data, size_t size, size_t nmemb, void * callbackdata)
{
	size_t sizes = size * nmemb;
	CURLClient * pClient = (CURLClient *)callbackdata;
	pClient->m_Status = TRANSFERING;
	string content;
	content.append(data, sizes);
	pClient->m_responsecontent = UTF8ToUnicode(content);
	return sizes;
}

size_t WebClient::CURLClient::GetResponseHeader(char *data, size_t size, size_t nmemb, void * callbackdata)
{
	size_t sizes = size * nmemb;
	if (callbackdata == NULL)
		return sizes;
	CURLClient * pClient = (CURLClient *)callbackdata;
	if (pClient->m_ReuqestData->RequestType == WebClient::HTTP_GET || pClient->m_ReuqestData->RequestType == WebClient::HTTP_POST)
	{
		string header;
		header.append(data, sizes);
		pClient->m_responseheader += UTF8ToUnicode(header);
	}
	return sizes;
}

int WebClient::CURLClient::ProgressCallback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
	CURLClient * pClient = (CURLClient *)clientp;

	double AvgSpeed = 0;

	// curl_get_info必须在curl_easy_perform之后调用
	curl_easy_getinfo(pClient->m_CURL, CURLINFO_SPEED_DOWNLOAD, &AvgSpeed);

	curl_off_t totalbytes = 0;
	curl_off_t transferedbytes = 0;
	double progress = 0;

	if (pClient->m_ReuqestData->RequestType = HTTP_DOWNLOAD) 
	{
		totalbytes = dltotal;
		transferedbytes = dlnow;
	}
	else if (pClient->m_ReuqestData->RequestType = HTTP_UPLOAD) 
	{
		totalbytes = ultotal;
		transferedbytes = ulnow;
	}

	pClient->m_ProgressInfo.BytesRemain = totalbytes - transferedbytes;
	pClient->m_ProgressInfo.BytesTransfered = transferedbytes + pClient->ResumeBytes;

	// Time remaining  
	if (AvgSpeed != 0)
	{
		pClient->m_ProgressInfo.SecondRemain = (curl_off_t)(pClient->m_ProgressInfo.BytesRemain / AvgSpeed);
	}

	if (pClient->TotalFileSize == -1)
		pClient->m_ProgressInfo.FileTotalBytes = totalbytes;
	else
		pClient->m_ProgressInfo.FileTotalBytes = pClient->TotalFileSize;

	if (pClient->m_ProgressInfo.FileTotalBytes != 0)
	{
		progress = (double)(((double)pClient->m_ProgressInfo.BytesTransfered / pClient->m_ProgressInfo.FileTotalBytes * 100));
	}
	pClient->m_ProgressInfo.TransferedPercent = progress;

	pClient->m_ProgressInfo.AvgSpeed = AvgSpeed;

	if (pClient->bStopTransfer)
	{
		return 1;
	}

	return 0;
}

//写入文件
size_t WebClient::CURLClient::WriteToFile(char *buffer, size_t size, size_t nmemb, void *userdata)
{
	WriteFileData *pData = (WriteFileData *)(userdata);
	pData->pClient->m_Status = TRANSFERING;
	pData->FileHandle->Write(buffer, nmemb * size);
	pData->FileHandle->Flush();
	return size * nmemb;
}

void WebClient::CURLClient::DebugPrint(const char *date, const char *time, const char *file, const int line, const char *func, string msg)
{
#ifdef PRINT_DEBUG_ERROR
	CString Msg;
	Msg.Format(L"===============Error===============\nTime:%s %s\nFile:%s\nLine:%s\nFucntion:%s\nError:%s\n=============End Error=============\n", date, time, file, line, func, msg.c_str());
	OutputDebugString(Msg);
#endif // PRINT_DEBUG_ERROR
}



WebClient::HttpHeader::HttpHeader(string seperator)
{
	m_header = m_defaultheader;
	if (seperator.size() == 0)
		m_seperator = m_header_sep;
	else
		m_seperator = seperator;
}

WebClient::HttpHeader::~HttpHeader()
{
	m_header = "";
}

void WebClient::HttpHeader::AddRequestHeader(string Key, string Value)
{
	if(Key!="")
		m_header = m_header + Key + ": " + Value + m_seperator;
	return;
}

void WebClient::HttpHeader::Reset()
{
	m_header = m_defaultheader;
}



WebClient * pClientSelf;
WebClient::WebClient(ResponseCallback & callback)
	:m_Callback(callback)
{
	pClientSelf = this;
	/* In windows, this will init the winsock stuff */
	curl_global_init(CURL_GLOBAL_ALL);

	bExitThread = false;
	_beginthread(Distributor, 0, NULL);
}

WebClient::~WebClient()
{
	bExitThread = true;
	for (m_Pointor = m_ClientMap.begin(); m_Pointor != m_ClientMap.end(); m_Pointor++)
	{
		m_Pointor->second->StopTransfering();
	}

	for (size_t i = 0; i < m_RequestQueue.size(); i++) 
	{
		m_RequestQueue.pop();
	}
	if(m_ThreadGroup.size()!=0)
		m_ThreadGroup.join_all();
	curl_global_cleanup();
}

void WebClient::SendHttpReuqestThread(void * pData)
{
	extern WebClient * pClientSelf;
	HttpRequestInfo * data = (HttpRequestInfo*)pData;
	pClientSelf->m_Client = new CURLClient(data, pClientSelf->m_Callback);

	if (pClientSelf->m_Client->InitCURL())
	{
		pClientSelf->m_ClientMap[data->id] = pClientSelf->m_Client;
		if (data->header != "")
			pClientSelf->m_Client->SetRequestHeader(data->header);
		pClientSelf->m_Client->SendRequest();
	}
	else
	{
		pClientSelf->m_Callback.OnFailed("thread: send post request failed", data->userdata);
		PrintD("thread: send post request failed");
	}
	pClientSelf->m_ClientMap.erase(data->id);
	delete data;
	data = NULL;
	delete pClientSelf->m_Client;
	pClientSelf->m_Client = NULL;
}

void WebClient::Distributor(void * pData) 
{
	extern WebClient * pClientSelf;
	do 
	{
		if (pClientSelf->m_RequestQueue.size() != 0) 
		{
			//使用boost线程group管理
			pClientSelf->m_ThreadGroup.create_thread(bind(&SendHttpReuqestThread, pClientSelf->m_RequestQueue.front()));
			//_beginthread(SendHttpReuqestThread, 0, pClientSelf->m_RequestQueue.front());
			pClientSelf->m_RequestQueue.pop();
		}
		else 
		{
			Sleep(10);
		}
	} while (!pClientSelf->bExitThread);
}

int WebClient::POST(string url, string postdata, string header, void * userdata)
{
	m_ThreadData = new HttpRequestInfo;
	m_ThreadData->id = ++m_ClientID;
	m_ThreadData->RequestType = HTTP_POST;
	m_ThreadData->postdata = postdata;
	m_ThreadData->header = header;
	m_ThreadData->url = url;
	m_ThreadData->userdata = userdata;
	m_RequestQueue.push(m_ThreadData);
	m_ThreadData = NULL;
	return m_ClientID;
}

int WebClient::GET(string url, string header, void * userdata)
{
	m_ThreadData = new HttpRequestInfo;
	m_ThreadData->id = ++m_ClientID;
	m_ThreadData->RequestType = HTTP_GET;
	m_ThreadData->header = header;
	m_ThreadData->url = url;
	m_ThreadData->userdata = userdata;
	m_RequestQueue.push(m_ThreadData);
	m_ThreadData = NULL;
	return m_ClientID;
}

int WebClient::DownloadFile(string url, string savepath, string header, void * userdata)
{
	m_ThreadData = new HttpRequestInfo;
	m_ThreadData->id = ++m_ClientID;
	m_ThreadData->RequestType = HTTP_DOWNLOAD;
	m_ThreadData->header = header;
	m_ThreadData->url = url;
	m_ThreadData->userdata = userdata;
	m_ThreadData->dFilePath = savepath;
	m_ThreadData->bRedirecte = true;
	m_RequestQueue.push(m_ThreadData);
	m_ThreadData = NULL;
	return m_ClientID;
}

bool WebClient::CloseClient(int ClientID) 
{
	if (m_ClientMap.find(ClientID) != m_ClientMap.end()) 
	{
		m_ClientMap[ClientID]->StopTransfering();
	}
	return true;
}

int WebClient::GETS(string url, string header, void * userdata) 
{
	m_ThreadData = new HttpRequestInfo;
	m_ThreadData->id = ++m_ClientID;
	m_ThreadData->RequestType = HTTPS_GET;
	m_ThreadData->header = header;
	m_ThreadData->url = url;
	m_ThreadData->userdata = userdata;
	m_RequestQueue.push(m_ThreadData);
	m_ThreadData = NULL;
	return m_ClientID;
}