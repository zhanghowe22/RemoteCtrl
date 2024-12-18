// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"
#include "CommonTool.h"
#include <conio.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//#define INVOKE_PATH _T("C:\\Windows\\SysWOW64\\RemoteCtrl.exe")
#define INVOKE_PATH _T("C:\\Users\zhangjiahao\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe")

// 唯一的应用程序对象
CWinApp theApp;
using namespace std;

bool ChooseAutoInvoke(const CString& strPath) {
	TCHAR wcsSystem[MAX_PATH] = _T("");
	if (PathFileExists(strPath)) {
		return true;
	}

	CString strInfo = _T("该程序只允许用于合法的用途！\n");
	strInfo += _T("继续运行该程序，将使得这台机器处于被监控的状态！\n");
	strInfo += _T("如果你不想这样，请按取消按钮，退出程序。\n");
	strInfo += _T("按下 “是” 按钮，该程序将被复制到你的机器上，并随系统启动而自动运行！\n");
	strInfo += _T("按下 “否” 按钮，程序只运行一次，不会再系统留下任何东西！\n");
	int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
	if (ret == IDYES) {
		// WriteRegisterTable(strPath);
		if (!CCommonTool::WriteStartUpDir(strPath)) {
			MessageBox(NULL, _T("复制文件失败，是否权限不足？\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
	}
	else if (ret == IDCANCEL) {
		return false;
	}
	return true;
}

#define IOCP_LIST_EMPTY 0
#define IOCP_LIST_PUSH 1
#define IOCP_LIST_POP 2

enum {
	IocpListEmpty,
	IocpListPush,
	IocpListPop
};


typedef struct IocpParam {
	int nOperator; // 操作
	std::string strData; // 数据
	_beginthread_proc_type cbFunc; // 回调

	IocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL) {
		nOperator = op;
		strData = sData;
		cbFunc = cb;
	}

	IocpParam() {
		nOperator = -1;
	}

}IOCP_PARAM;

void threadQueueEntry(HANDLE hIOCP)
{
	std::list<std::string> lsString;
	DWORD dwTransferred = 0;
	ULONG_PTR completionKey = 0;
	OVERLAPPED* pOverlAppend = NULL;

	while (GetQueuedCompletionStatus(hIOCP, &dwTransferred, &completionKey, &pOverlAppend, INFINITE)) {
		if ((dwTransferred == 0) || (completionKey == NULL)) {
			printf("Thread is prepare to exit!\r\n");
			break;
		}
		IOCP_PARAM* pParam = (IOCP_PARAM*)completionKey;
		if (pParam->nOperator == IocpListPush) {
			lsString.push_back(pParam->strData);
		}
		else if (pParam->nOperator == IocpListPop) {
			std::string* pStr = NULL;
			if (lsString.size() > 0) {
				pStr = new std::string(lsString.front());
				lsString.pop_front();
			}
			if (pParam->cbFunc) {
				pParam->cbFunc(pStr);
			}
		}
		else if (pParam->nOperator == IocpListEmpty) {
			lsString.clear();
		}

		delete pParam;
	}

	_endthread();
}

void func(void* arg)
{
	std::string* pstr = (std::string*)arg;
	if (pstr != NULL) {
		printf("Pop from list: %s \r\n", pstr->c_str());
		delete pstr;
	}
	else {
		printf("List is empty, no data!\r\n");
	}
}

int main()
{
	if (!CCommonTool::Init()) return 1;

	printf("press any key to exit ...\r\n");

	HANDLE hIOCP = INVALID_HANDLE_VALUE; // IO Completion Port

	hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1); // epoll的区别点1

	HANDLE hThread = (HANDLE)_beginthread(threadQueueEntry, 0, hIOCP);

	ULONGLONG tick = GetTickCount64();

	while (_kbhit() != 0) { // 完成端口 把请求与实现分离开了
		if (GetTickCount64() - tick > 1300) {
			PostQueuedCompletionStatus(hIOCP, sizeof(IOCP_PARAM), (ULONG_PTR)new IOCP_PARAM(IocpListPop, "hello world"), NULL);
			tick = GetTickCount64();
		}

		if (GetTickCount64() - tick > 2000) {
			PostQueuedCompletionStatus(hIOCP, sizeof(IOCP_PARAM), (ULONG_PTR)new IOCP_PARAM(IocpListPush, "hello world"), NULL);
			tick = GetTickCount64();
		}

		Sleep(1);
	}

	if (hIOCP != NULL) {
		PostQueuedCompletionStatus(hIOCP, 0, NULL, NULL);
		WaitForSingleObject(hThread, INFINITE);
	}

	CloseHandle(hIOCP);

	printf("exit done!\r\n");
	::exit(0);

	/*
	if (CCommonTool::IsAdmin()) {
		if (!CCommonTool::Init()) return 1;
		if (ChooseAutoInvoke(INVOKE_PATH)) {
			CCommand cmd;
			int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);
			switch (ret)
			{
			case -1:
				MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态！"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
				break;

			case -2:
				MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
				break;

			default:
				break;
			}
		}
	}
	else {
		if (!CCommonTool::RunAsAdmin()) {
			CCommonTool::ShowError();
			return 1;
		}
	}
	*/
	return 0;
}
