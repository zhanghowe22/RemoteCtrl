

#pragma once
#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>

#pragma pack(push)
#pragma pack(1)

// 通讯包类
class CPacket {
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}

	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
		sHead = 0XFEFF;

		nLength = nSize + 4;

		sCmd = nCmd;

		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			strData.clear();
		}

		sSum = 0;
		for (int j = 0; j < strData.size(); j++) {
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}

	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}

	CPacket(const BYTE* pData, size_t& nSize) {
		size_t i = 0;
		for (; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0XFEFF) {
				sHead = *(WORD*)(pData + i); // 读取包头
				i += 2;
				break;
			}
		}

		if (i + 4 + 2 + 2 > nSize) { // 包数据可能不全，或者包头未能全部接收到
			nSize = 0;
			return;
		}

		nLength = *(DWORD*)(pData + i); // 读取长度
		i += 4;
		if (nLength + i > nSize) { // 包未完全接收到
			nSize = 0;
			return;
		}

		sCmd = *(WORD*)(pData + i); // 读取控制命令
		i += 2;

		if (nLength > 4) {
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}

		sSum = *(WORD*)(pData + i);
		i += 2;

		WORD sum = 0;
		for (int j = 0; j < strData.size(); j++) {
			sum += BYTE(strData[j]) & 0xFF;
		}

		if (sum == sSum) {
			nSize = i;
			return;
		}
		nSize = 0;
	}

	~CPacket() {

	}

	CPacket& operator=(const CPacket& pack) {
		if (this != &pack) {
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;
	}

	int Size() {
		return nLength + 6;
	}

	const char* Data() {
		strOut.resize(nLength + 6);

		BYTE* pData = (BYTE*)strOut.c_str();

		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;

		return strOut.c_str();
	}

public:
	WORD sHead;          // 固定位 FE FF
	DWORD nLength;       // 包长度（从控制命令开始，到校验和结束）
	WORD sCmd;           // 控制命令
	std::string strData; // 包数据
	WORD sSum;           // 和校验

	std::string strOut; // 整个包的数据
};

#pragma pack(pop)

typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction; // 点击、移动、双击
	WORD nButton; // 左键、右键、中键
	POINT ptXY; // 坐标
}MOUSEEV, * PMOUSEEV;

std::string GetErrorInfo(int wsaErrCode);

class CClientSocket
{
public:
	static CClientSocket* getInstance() {
		if (m_instance == NULL) { // 静态函数没有this指针，无法直接访问成员变量
			m_instance = new CClientSocket();
		}
		return m_instance;
	}

	bool InitSocket(int nIp, int nPort)
	{
		if (m_sock != INVALID_SOCKET) {
			CloseSocket();
		}

		m_sock = socket(PF_INET, SOCK_STREAM, 0);

		if (m_sock == -1)return false;

		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;

		// TRACE("addr: %08X nIp = %08X\r\n", inet_addr("127.0.0.1"), nIp);

		serv_adr.sin_addr.s_addr = htonl(nIp);
		serv_adr.sin_port = htons(nPort);

		int ret = connect(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr));

		if (serv_adr.sin_addr.s_addr == INADDR_NONE) {
			AfxMessageBox(_T("指定的IP地址不存在！！！"));
			
			return false;
		}

		if (ret == -1) {
			AfxMessageBox(_T("连接失败！！！"));
			TRACE("连接失败: %d %s\r\n", WSAGetLastError(), GetErrorInfo(WSAGetLastError()).c_str());
			return false;
		}

		return true;
	}

	#define BUFFER_SIZE 4096
	int DealCommand() {
		if (m_sock == -1) return -1;

		char* buffer = m_buffer.data(); // 4k的
		memset(buffer, 0, BUFFER_SIZE);

		size_t index = 0; // buffer缓存的索引

		while (true)
		{
			size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0) {
				return -1;
			}

			index += len;
			len = index;

			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0) {
				memmove(buffer, buffer + len, BUFFER_SIZE - len); // 将buffer中未解析的内容移到开头
				index -= len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}

	bool Send(const char* pData, size_t nSize) {
		if (m_sock == -1) return false;
		return send(m_sock, pData, nSize, 0) > 0;
	}

	bool Send(CPacket& pack) {
		TRACE("client m_sock = %d\r\n", m_sock);
		if (m_sock == -1) return false;
		return send(m_sock, pack.Data(), pack.Size(), 0) > 0;
	}

	bool GetFilePath(std::string& strPath) {
		if ((m_packet.sCmd == 2) || (m_packet.sCmd == 3) || (m_packet.sCmd == 4)) {
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}

	bool GetMouseEvent(MOUSEEV& mouse)
	{
		if (m_packet.sCmd == 5) {
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
			return true;
		}
		return false;
	}

	CPacket& GetPacket() {
		return m_packet;
	}

	void CloseSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}

private:
	std::vector<char> m_buffer;

	SOCKET m_sock;

	CPacket m_packet;

	CClientSocket() {
		if (!InitSockEnv()) {
			MessageBox(NULL, _T("无法初始化套接字环境,请检查网络设置"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
			exit(0);
		}

		m_buffer.resize(BUFFER_SIZE);
	}

	CClientSocket& operator=(const CClientSocket& ss) {}

	CClientSocket(const CClientSocket& ss) {
		m_sock = ss.m_sock;
	}

	~CClientSocket() {
		closesocket(m_sock);
		WSACleanup();
	}

	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			return FALSE;
		}
		return TRUE;
	}

	static void releaseInstance()
	{
		if (m_instance != NULL) {
			CClientSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}

	static CClientSocket* m_instance;

	class CHelper {
	public:
		CHelper() {
			CClientSocket::getInstance();
		}

		~CHelper() {
			CClientSocket::releaseInstance();
		}
	};

	static CHelper m_helper;
};

extern CClientSocket client;

