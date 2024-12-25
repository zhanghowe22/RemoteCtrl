#pragma once
#include "pch.h"
#include <Windows.h>
#include <atomic>
#include <vector>
#include <mutex>

class ThreadFuncBase {};

typedef int(ThreadFuncBase::* FUNCTYPE);

class ThreadWorker
{
public:
	ThreadWorker() :thiz(NULL), func(NULL) {}

	ThreadWorker(ThreadFuncBase* obj, FUNCTYPE f) : thiz(obj), func(f) {}

	ThreadWorker(const ThreadWorker& worker) {
		thiz = worker.thiz;
		func = worker.func;
	}

	ThreadWorker& operator=(const ThreadWorker& worker) {
		if (this != &worker) {
			thiz = worker.thiz;
			func = worker.func;
		}
		return *this;
	}

	int operator()() {
		if (IsValid()) 
		{
			return (thiz->*func);
		}
		return -1;
	}

	bool IsValid() {
		return (thiz != NULL) && (func != NULL);
	}

private:
	ThreadFuncBase* thiz;

	FUNCTYPE func;
};

class CMyThread
{
public:
	CMyThread() {
		m_hThread = NULL;
	}

	~CMyThread() {
		Stop();
	}
	// true: 成功； false: 失败
	bool Start() {
		m_bStatus = true;
		m_hThread = (HANDLE)_beginthread(&CMyThread::ThreadEntry, 0, this);
		if (!IsVaild()) {
			m_bStatus = false;
		}
		return m_bStatus;
	}

	bool IsVaild() { // true: 线程有效； false: 线程异常或已经终止
		if ((m_hThread == NULL) || (m_hThread == INVALID_HANDLE_VALUE)) return false;
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
	}

	bool Stop() {
		if (!m_bStatus) return true;
		m_bStatus = false;
		return WaitForSingleObject(m_hThread, INFINITE) == WAIT_OBJECT_0;
	}

	void UpdateWorker(const ::ThreadWorker& worker = ::ThreadWorker()) {
		m_worker.store(worker);
	}

	// true 表示空闲；false 表示已经分配了工作
	bool IsIdle() { 
		return !m_worker.load().IsValid();
	}

private:
	static void ThreadEntry(void* arg) {
		CMyThread* thiz = (CMyThread*)arg;
		if (thiz) {
			thiz->ThreadWorker();
		}
		_endthread();
	}

	void ThreadWorker() {
		while (m_bStatus) {
			::ThreadWorker worker = m_worker.load();
			if (worker.IsValid()) {
				int ret = worker();
				if (ret != 0) {
					CString str;
					str.Format(_T("Thread found warning code %d\r\n"), ret);
					OutputDebugString(str);
				}
				if (ret < 0) {
					m_worker.store(::ThreadWorker());
				}
			}
			else {
				Sleep(1);
			}
		}
	}

private:
	HANDLE m_hThread;
	bool m_bStatus; // true: 线程正在运行； false: 线程将要关闭
	std::atomic<::ThreadWorker> m_worker;
};

// 线程池类
class MyThreadPool {
public:
	MyThreadPool() {}

	~MyThreadPool() {
		Stop();
		m_threads.clear();
	}

	MyThreadPool(size_t size) {
		m_threads.resize(size);
	}

	bool Invoke() {
		bool ret = true;
		for (size_t i = 0; i < m_threads.size(); i++) {
			if (m_threads[i].Start() == false) {
				ret = false;
				break;
			}
		}

		if (!ret) {
			for (size_t i = 0; i < m_threads.size(); i++) {
				m_threads[i].Stop();
			}
		}
		return ret;
	}

	void Stop() {
		for (size_t i = 0; i < m_threads.size(); i++) {
			m_threads[i].Stop();
		}
	}

	// 返回-1 表示分配失败，所有线程都在忙；大宇等于0，表示第n个线程分配来做这个事情
	int DispatchWorker(const ThreadWorker& worker) {
		int index = -1;
		m_lock.lock();
		for (size_t i = 0; i < m_threads.size(); i++) {
			if (m_threads[i].IsIdle()) {
				m_threads[i].UpdateWorker(worker);
				index = i;
				break;
			}
		}
		m_lock.unlock();
		return index;
	}

	bool CheckThreadValid(size_t index) {
		if (index < m_threads.size())
		{
			return m_threads[index].IsVaild();
		}
		return false;
	}

public:
	std::mutex m_lock;
	std::vector<CMyThread> m_threads;
};

