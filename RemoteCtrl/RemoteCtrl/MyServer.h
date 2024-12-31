#pragma once
#include <MSWSock.h>
#include "MyThread.h"
#include "MyQueue.h"
#include "CommonTool.h"
#include <map>
#include <vector>

enum MyOperator {
	ENone,
	EAccept,
	ERecv,
	ESend,
	EError
};

class CMyServer;
class MyClient;

typedef std::shared_ptr<MyClient> PCLIENT;

class MyOverlapped {
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator; // 操作 参见MyOperator
	std::vector<char> m_buffer; // 缓冲区
	ThreadWorker m_worker; // 处理函数
	CMyServer* m_server; // 服务器对象
	PCLIENT m_client; // 对应的客户端
	WSABUF m_wsabuffer;
};

template<MyOperator>class AccpetOverlapped;
typedef AccpetOverlapped<EAccept> ACCEPTOVERLAPPED;
template<MyOperator>class RecvOverlapped;
typedef RecvOverlapped<ERecv> RECVOVERLAPPED;
template<MyOperator>class SendOverlapped;
typedef SendOverlapped<ESend> SENDOVERLAPPED;

class MyClient : public ThreadFuncBase{
public:
	MyClient();

	~MyClient() {
		closesocket(m_sock);
	}

	void SetOverlapped(PCLIENT& ptr);

	operator SOCKET() {
		return m_sock;
	}

	operator PVOID() {
		return &m_buffer[0];
	}

	operator LPOVERLAPPED();

	operator LPDWORD() {
		return &m_received;
	}

	LPWSABUF RecvWSABuffer();

	LPWSABUF SendWSABuffer();

	DWORD& flags() { return m_flags; }

	sockaddr_in* GetLocalAddr() { return &m_laddr; }
	sockaddr_in* GetRemoteAddr() { return &m_raddr; }

	size_t GetBuffferSize() const {
		return m_buffer.size();
	}

	int Recv();

	int Send(void* buffer, size_t nSize);

	int SendData(std::vector<char>& data);
private:
	SOCKET m_sock;
	DWORD m_received;
	DWORD m_flags;
	std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
	std::shared_ptr<RECVOVERLAPPED> m_recv;
	std::shared_ptr<SENDOVERLAPPED> m_send;
	std::vector<char> m_buffer;
	size_t m_used; // 已经使用的缓冲区大小
	sockaddr_in m_raddr;
	sockaddr_in m_laddr;
	bool m_isbusy;
	MySendQueue<std::vector<char>> m_vecSend; // 发送数据队列
};

template<MyOperator>
class AccpetOverlapped : public MyOverlapped, ThreadFuncBase
{
public:
	AccpetOverlapped();

	int AcceptWorker();

	PCLIENT m_client;
};


template<MyOperator>
class RecvOverlapped : public MyOverlapped, ThreadFuncBase
{
public:
	RecvOverlapped();

	int RecvWorker() {
		int ret = m_client->Recv();
		return ret;
	}
};


template<MyOperator>
class SendOverlapped : public MyOverlapped, ThreadFuncBase
{
public:
	SendOverlapped();

	int SendWorker() {
		// TODO：
		/*
		* 1. send可能不会立即完成
		*/
		return -1;
	}
};

template<MyOperator>
class ErrorOverlapped : public MyOverlapped, ThreadFuncBase
{
public:
	ErrorOverlapped() :m_operator(EError), m_worker(this, &ErrorOverlapped::ErrorWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024);
	}

	int ErrorWorker() {
		// TODO
		return -1;
	}
};
typedef ErrorOverlapped<EError> ERROROVERLAPPED;

class CMyServer : public ThreadFuncBase
{
public:
	CMyServer(const std::string& ip = "0.0.0.0", short port = 9527) : m_pool(10) {
		m_hIOCP = INVALID_HANDLE_VALUE;
		m_sock = INVALID_SOCKET;
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(port);
		m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
	}

	~CMyServer() {}

	bool StartService() {
		CreateSocket();

		if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == -1) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			return false;
		}

		if (listen(m_sock, 3) == -1) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			return false;
		}

		m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
		if (m_hIOCP == NULL) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}

		CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0);

		m_pool.Invoke();

		m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&CMyServer::threadIocp));

		if (!NewAccept()) return false;

		return true;
	}

	bool NewAccept();
private:
	int threadIocp();

	void CreateSocket() {
		m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		int opt = 1;
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
	}

private:
	MyThreadPool m_pool;
	HANDLE m_hIOCP;
	SOCKET m_sock;
	std::map<SOCKET, PCLIENT> m_client;
	sockaddr_in m_addr;
};
