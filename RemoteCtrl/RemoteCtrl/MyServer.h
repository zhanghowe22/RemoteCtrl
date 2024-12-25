#pragma once
#include "MyThread.h"
#include <map>

class MyClient {

};

enum MyOperator {
	ENone,
	EAccept,
	ERecv,
	ESend,
	EError
};

class MyOverlapped {
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator; // 操作 参见MyOperator
	std::vector<char> m_buffer; // 缓冲区
	ThreadWorker m_worker; // 处理函数
};

template<MyOperator>
class AccpetOverlapped : public MyOverlapped, ThreadFuncBase
{
public:
	AccpetOverlapped() :m_operator(EAccept), m_worker(this, &AccpetOverlapped::AcceptWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024);
	}

	int AcceptWorker() {
		// TODO
	}
};
typedef AccpetOverlapped<EAccept> ACCEPTOVERLAPPED;

template<MyOperator>
class RecvOverlapped : public MyOverlapped, ThreadFuncBase
{
public:
	RecvOverlapped() :m_operator(ERecv), m_worker(this, &RecvOverlapped::RecvWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024 * 256);
	}

	int RecvWorker() {
		// TODO
	}
};
typedef RecvOverlapped<ERecv> RECVOVERLAPPED;

template<MyOperator>
class SendOverlapped : public MyOverlapped, ThreadFuncBase
{
public:
	SendOverlapped() :m_operator(ESend), m_worker(this, &SendOverlapped::SendWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024 * 256);
	}

	int SendWorker() {
		// TODO
	}
};
typedef SendOverlapped<ESend> SENDOVERLAPPED;

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
	}
};
typedef ErrorOverlapped<EError> ERROROVERLAPPED;

class CMyServer :
	public ThreadFuncBase
{
public:
	CMyServer(const std::string& ip = "0.0.0.0", short port = 9527) : m_pool(10) {
		m_hIOCP = INVALID_HANDLE_VALUE;
		m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

		int opt = 1;
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = inet_addr(ip.c_str());

		if (bind(m_sock, (sockaddr*)&addr, sizeof(addr)) == -1) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			return;
		}

		if (listen(m_sock, 3) == -1) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			return;
		}

		m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
		if (m_hIOCP == NULL) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return;
		}

		CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0);

		m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&CMyServer::threadIocp));
	}

	~CMyServer() {}

private:
	int threadIocp() {
		DWORD tranferred = 0;
		ULONG_PTR CompletionKey = 0;
		OVERLAPPED* lpOverlapped = NULL;

		if (GetQueuedCompletionStatus(m_hIOCP, &tranferred, &CompletionKey, &lpOverlapped, INFINITE)) {
			if (tranferred > 0 && CompletionKey != 0)
			{
				MyOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, MyOverlapped, m_overlapped);
				switch (pOverlapped->m_operator) {
				case EAccept:
				{
					ACCEPTOVERLAPPED* pOver = (ACCEPTOVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
				}
				break;

				case ERecv:
				{
					RECVOVERLAPPED* pOver = (RECVOVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
				}
				break;

				case ESend:
				{
					SENDOVERLAPPED* pOver = (SENDOVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
				}
				break;

				case EError:
				{
					ERROROVERLAPPED* pOver = (ERROROVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
				}
				break;
				}
			}
			else {
				return -1;
			}
		}
		return 0;
	}

private:
	MyThreadPool m_pool;
	HANDLE m_hIOCP;
	SOCKET m_sock;
	std::map<SOCKET, std::shared_ptr< MyClient*>> m_client;
};

