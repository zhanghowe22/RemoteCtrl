#pragma once
#include <list>
#include <mutex>
#include <atomic>
#include "pch.h"
#include "MyThread.h"


// 线程安全的队列，利用IOCP实现
template<class T>
class CMyQueue
{
public:
	enum {
		MQNone,
		MQPush,
		MQPop,
		MQSize,
		MQClear
	};

	typedef struct IocpParam {
		size_t nOperator; // 操作
		T Data; // 数据
		HANDLE hEvent; //pop操作需要的

		IocpParam(int op, const T& data, HANDLE hEve = NULL) {
			nOperator = op;
			Data = data;
			hEvent = hEve;
		}

		IocpParam() {
			nOperator = MQNone;
		}

	}PPARAM; // post Parameter 用于投递信息的结构体

public:
	CMyQueue() {
		m_lock = false;
		m_hCompeletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
		m_hThread = INVALID_HANDLE_VALUE;
		if (m_hCompeletionPort != NULL) {
			m_hThread = (HANDLE)_beginthread(
				&CMyQueue<T>::threadEntry,
				0, this);
		}
	}

	virtual ~CMyQueue() {
		if (m_lock) return;
		m_lock = true;
		PostQueuedCompletionStatus(m_hCompeletionPort, 0, NULL, NULL);
		WaitForSingleObject(m_hThread, INFINITE);
		if (m_hCompeletionPort != NULL) {
			HANDLE hTemp = m_hCompeletionPort;
			m_hCompeletionPort = NULL;
			CloseHandle(hTemp);
		}
	}

	bool PushBack(const T& data) {
		IocpParam* pParam = new IocpParam(MQPush, data);
		if (m_lock) {
			delete pParam;
			return false;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false) delete pParam;
		return ret;
	}

	virtual bool PopFront(T& data) {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam Param(MQPop, data, hEvent);
		if (m_lock) {
			if (hEvent) CloseHandle(hEvent);
			return false;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM),
			(ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return false;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret) {
			data = Param.Data;
		}
		return ret;
	}

	size_t Size() {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam Param(MQSize, T(), hEvent);

		if (m_lock) {
			if (hEvent) CloseHandle(hEvent);
			return -1;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM),
			(ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return -1;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret) {
			return Param.nOperator;
		}
		return -1;
	}

	bool Clear() {
		if (m_lock) return false;
		IocpParam* pParam = new IocpParam(MQClear, T());
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM),
			(ULONG_PTR)pParam, NULL);
		if (ret == false) delete pParam;
		return ret;
	}

protected:
	static void threadEntry(void* arg) {
		CMyQueue<T>* thiz = (CMyQueue<T>*)arg;
		thiz->threadMain();
		_endthread();
	}

	virtual void DealParam(PPARAM* pParam) {
		switch (pParam->nOperator)
		{
		case MQPush:
			m_lstData.push_back(pParam->Data);
			delete pParam;
			break;

		case MQPop:
		{
			if (m_lstData.size() > 0) {
				pParam->Data = m_lstData.front();
				m_lstData.pop_front();
			}
			if (pParam->hEvent != NULL)
				SetEvent(pParam->hEvent);
		}
		break;

		case MQSize:
			pParam->nOperator = m_lstData.size();
			if (pParam->hEvent != NULL)
				SetEvent(pParam->hEvent);
			break;

		case MQClear:
			m_lstData.clear();
			delete pParam;
			break;

		default:
			OutputDebugStringA("Unknown operator!\r\n");
			break;
		}
	}

	virtual void threadMain() {
		DWORD dwTransferred = 0;
		PPARAM* pParam = NULL;
		ULONG_PTR completionKey = 0;
		OVERLAPPED* pOverlAppend = NULL;
		while (GetQueuedCompletionStatus(m_hCompeletionPort,
			&dwTransferred,
			&completionKey,
			&pOverlAppend, INFINITE)) {
			if ((dwTransferred == 0) || (completionKey == NULL)) {
				printf("Thread is prepare to exit!\r\n");
				break;
			}
			pParam = (PPARAM*)completionKey;
			DealParam(pParam);
		}

		while (GetQueuedCompletionStatus(m_hCompeletionPort,
			&dwTransferred,
			&completionKey,
			&pOverlAppend, 0)) {
			if ((dwTransferred == 0) || (completionKey == NULL)) {
				printf("Thread is prepare to exit!\r\n");
				continue;
			}
			pParam = (PPARAM*)completionKey;
			DealParam(pParam);
		}

		HANDLE hTemp = m_hCompeletionPort;
		m_hCompeletionPort = NULL;
		CloseHandle(hTemp);
	}

protected:
	std::list<T> m_lstData;
	HANDLE m_hCompeletionPort;
	HANDLE m_hThread;
	std::atomic<bool> m_lock; // 队列正在析构
};



template<class T>
class MySendQueue : public CMyQueue<T>, public ThreadFuncBase {
public:
	typedef int (ThreadFuncBase::* MYCALLBACK) (T& data);

	MySendQueue(ThreadFuncBase* obj, MYCALLBACK callback)
		: CMyQueue<T>(), m_base(obj), m_callback(callback) 
	{
		m_thread.Start();
		m_thread.UpdateWorker(::ThreadWorker(this, (FUNCTYPE)&MySendQueue<T>::threadTick));
	}

	virtual ~MySendQueue() {
		m_base = NULL;
		m_callback = NULL;
		m_thread.Stop();
	}

protected:
	int threadTick() {
		if (WaitForSingleObject(CMyQueue<T>::m_hThread, 0) != WAIT_TIMEOUT)
			return 0;
		if (CMyQueue<T>::m_lstData.size() > 0) {
			PopFront(); 
		 }
		return 0;
	}

	virtual bool PopFront(T& data) { 
		return false;
	}

	bool PopFront()
	{
		typename CMyQueue<T>::IocpParam* Param = new typename CMyQueue<T>::IocpParam(CMyQueue<T>::MQPop, T());
		if (CMyQueue<T>::m_lock) {
			delete Param;
			return false;
		}
		bool ret = PostQueuedCompletionStatus(CMyQueue<T>::m_hCompeletionPort, sizeof(*Param),
			(ULONG_PTR)&Param, NULL);
		if (ret == false) {
			delete Param;
			return false;
		}
		return ret;
	}

	virtual void DealParam(typename CMyQueue<T>::PPARAM* pParam) {
		switch (pParam->nOperator)
		{
		case CMyQueue<T>::MQPush:
			CMyQueue<T>::m_lstData.push_back(pParam->Data);
			delete pParam;
			break;

		case CMyQueue<T>::MQPop:

			if (CMyQueue<T>::m_lstData.size() > 0) {
				pParam->Data = CMyQueue<T>::m_lstData.front();
				if ((m_base->*m_callback)(pParam->Data) == 0) {
					CMyQueue<T>::m_lstData.pop_front();
				}	
			}
			delete pParam;
			break;

		case CMyQueue<T>::MQSize:
			pParam->nOperator = CMyQueue<T>::m_lstData.size();
			if (pParam->hEvent != NULL)
				SetEvent(pParam->hEvent);
			break;

		case CMyQueue<T>::MQClear:
			CMyQueue<T>::m_lstData.clear();
			delete pParam;
			break;

		default:
			OutputDebugStringA("Unknown operator!\r\n");
			break;
		}
	}

private:
	ThreadFuncBase* m_base;
	MYCALLBACK m_callback;
	CMyThread m_thread;
};


typedef MySendQueue<std::vector<char>>::MYCALLBACK SENDCALLBACK;