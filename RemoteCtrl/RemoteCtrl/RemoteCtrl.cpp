// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"
#include "CommonTool.h"
#include <conio.h>
#include "MyQueue.h"
#include <winsock2.h>
#include <mswsockdef.h>
#include "MyServer.h"

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

void iocp();

void udp_server();
void udp_client(bool isHost = true);

//int wmain(int argc, TCHAR* argv[]);
//int _tmain(int argc, TCHAR* argv[]);

void initsock() {
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
}

void clearsock() {
	WSACleanup();
}

int main(int argc, char* argv[])
{
	if (!CCommonTool::Init()) return 1;
	initsock();

	if (argc == 1) { 
		char wstrDir[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, wstrDir);
		STARTUPINFOA si;
		PROCESS_INFORMATION pi;
		memset(&si, 0, sizeof(si));
		memset(&pi, 0, sizeof(pi));
		string strCmd = argv[0];
		strCmd += " 1";
		BOOL bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, wstrDir, &si, &pi);
		if (bRet) {
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
			TRACE("进程ID:%d\r\n", pi.dwProcessId);
			TRACE("线程ID:%d\r\n", pi.dwThreadId);

			strCmd += " 2";
			bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, wstrDir, &si, &pi);
			if (bRet) {
				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
				TRACE("进程ID:%d\r\n", pi.dwProcessId);
				TRACE("线程ID:%d\r\n", pi.dwThreadId);
				udp_server(); // 服务器代码
			}
		}
	}
	else if (argc == 2) { // 主客户端
		udp_client();
	}
	else { // 从客户端
		udp_client(false);
	}

	clearsock();

	// iocp();

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

class COverlapped {
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator;
	char m_buffer[4096];
	COverlapped() {
		m_operator = 0;
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		memset(m_buffer, 0, sizeof(m_buffer));
	}
};

void iocp()
{
	CMyServer server;
	server.StartService();
	getchar();
}
/*
* 1 易用性
*	a. 简化参数
*	b. 类型适配（参数适配）
*	c. 流程简化
* 2 易移植性（高内聚、低耦合）
*	a. 
*/
#include "ENetwork.h"

int RecvFromCB(void* arg, const EBuffer& buffer, ESockaddrIn& addr) 
{
	EServer* server = (EServer*)arg;
	return server->Sendto(addr, buffer);
}

int SendToCB(void* arg, const ESockaddrIn& addr, int ret) 
{
	EServer* server = (EServer*)arg;
	printf("Sendto done! %d\r\n", server);
	return 0;
}

void udp_server()
{
	std::list<ESockaddrIn> lstClients;

	EServerParameter param(
		"127.0.0.1", 20000, ETYPE::ETypeUDP, NULL, NULL, NULL, RecvFromCB, SendToCB
	);

	EServer server(param);

	server.Invoke(&server);

	printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
	getchar();
	return;
}

void udp_client(bool isHost)
{
	Sleep(2000);

	sockaddr_in server, client;
	int len = sizeof(client);
	server.sin_family = AF_INET;
	server.sin_port = htons(20000);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");

	SOCKET sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) {
		printf("%s(%d):%s ERROR!!!\r\n", __FILE__, __LINE__, __FUNCTION__);
		return;
	}

	if (isHost) { // 主客户端代码
		printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
		EBuffer msg = "hello world!\n";
		int ret = sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&server, sizeof(server));
		printf("%s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
		if (ret > 0) {
			msg.resize(1024);
			memset((char*)msg.c_str(), 0, msg.size());
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
			printf("host %s(%d):%s ERROR(%d) ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
			if (ret > 0) {
				printf("%s(%d):%s - ip: %s port: %d\r\n", __FILE__, __LINE__, __FUNCTION__, inet_ntoa(client.sin_addr), ntohs(client.sin_port));
				printf("%s(%d):%s msg = %d\r\n", __FILE__, __LINE__, __FUNCTION__, msg.size());
			}
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
			printf("host %s(%d):%s ERROR(%d) ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
			if (ret > 0) {
				printf("%s(%d):%s - ip: %s port: %d\r\n", __FILE__, __LINE__, __FUNCTION__, inet_ntoa(client.sin_addr), ntohs(client.sin_port));
				printf("%s(%d):%s msg = %s\r\n", __FILE__, __LINE__, __FUNCTION__, msg.c_str());
			}
		}
	}	
	else { // 从客户端代码
		printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
		std::string msg = "hello world!\n";
		int ret = sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&server, sizeof(server));
		printf("%s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
		if (ret > 0) {
			msg.resize(1024);
			memset((char*)msg.c_str(), 0, msg.size());
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
			printf("client %s(%d):%s ERROR(%d) ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
			if (ret > 0) {
				sockaddr_in addr;
				memcpy(&addr, msg.c_str(), sizeof(addr));
				sockaddr_in* paddr = (sockaddr_in*)&addr;
				printf("%s(%d):%s - ip: %s port: %d\r\n", __FILE__, __LINE__, __FUNCTION__, inet_ntoa(client.sin_addr), ntohs(client.sin_port));
				printf("%s(%d):%s msg = %d\r\n", __FILE__, __LINE__, __FUNCTION__, msg.size());
				printf("%s(%d):%s - ip: %s port: %d\r\n", __FILE__, __LINE__, __FUNCTION__, inet_ntoa(paddr->sin_addr), ntohs(paddr->sin_port));
				msg = "hello, i am client!\r\n";
				ret = sendto(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)paddr, sizeof(sockaddr_in));
				printf("client %s(%d):%s ERROR(%d) ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
			}
		}
	}
	closesocket(sock);
}

/*
* 1. 思路：实现一个需求的过程
*	确定需求（阶段性的）、选定技术方案（依据技术点）、从框架开发到细节实现（从顶到底）、编译问题、
*	内存泄露（线程结束、exit函数）、bug排查与功能调试（日志、断点、线程、调用堆栈、内存、监视、局部变量、自动变量）、
*	压力测试（额外写代码）、功能上线
* 2 设计：易用性、移植性（可复用性）、安全性、稳定性（鲁棒性）、可扩展性
*/