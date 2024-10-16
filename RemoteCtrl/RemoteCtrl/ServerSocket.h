#pragma once
#include "pch.h"
#include "framework.h"

// ͨѶ����
class CPacket {
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}

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
				sHead = *(WORD*)(pData + i); // ��ȡ��ͷ
				i += 2;
				break;
			}
		}

		if (i + 4 + 2 + 2 >= nSize) { // �����ݿ��ܲ�ȫ�����߰�ͷδ��ȫ�����յ�
			nSize = 0;
			return;
		}

		nLength = *(DWORD*)(pData + i); // ��ȡ����
		i += 4;
		if (nLength + i > nSize) { // ��δ��ȫ���յ�
			nSize = 0;
			return;
		}

		sCmd = *(WORD*)(pData + i); // ��ȡ��������
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
			sum += BYTE(strData[i]) & 0xFF;
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

public:
	WORD sHead;          // �̶�λ FE FF
	DWORD nLength;       // �����ȣ��ӿ������ʼ����У��ͽ�����
	WORD sCmd;           // ��������
	std::string strData; // ������
	WORD sSum;           // ��У��
};

class CServerSocket
{
public:
	static CServerSocket* getInstance() {
		if (m_instance == NULL) { // ��̬����û��thisָ�룬�޷�ֱ�ӷ��ʳ�Ա����
			m_instance = new CServerSocket();
		}
		return m_instance;
	}

	bool InitSocket()
	{
		if (m_sock == -1)return false;

		sockaddr_in serv_adr, client_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = INADDR_ANY;
		serv_adr.sin_port = htons(9527);

		if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
			return false;
		}

		if (listen(m_sock, 1) == -1) {
			return false;
		}

		return true;
	}

	bool AcceptClient() {
		sockaddr_in client_adr;
		int cli_sz = sizeof(client_adr);
		m_client = accept(m_sock, (sockaddr*)&client_adr, &cli_sz);
		if (m_client == -1) {
			return false;
		}
		return true;
	}

    #define BUFFER_SIZE 4096
	int DealCommand() {
		if (m_client == -1) return -1;

		char* buffer = new char[BUFFER_SIZE]; // 4k��
		memset(buffer, 0, BUFFER_SIZE);

		size_t index = 0; // buffer���������

		while (true)
		{
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0) {
				return -1;
			}

			index += len;
			len = index;

			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0) {
				memmove(buffer, buffer + len, BUFFER_SIZE - len); // ��buffer��δ�����������Ƶ���ͷ
				index -= len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}

	bool Send(const char* pData, size_t nSize) {
		if (m_client == -1) return false;
		return send(m_client, pData, nSize, 0) > 0;
	}

private:

	SOCKET m_sock;

	SOCKET m_client;

	CPacket m_packet;

	CServerSocket(){
		m_client = INVALID_SOCKET;

		if (!InitSockEnv()) {
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���,������������"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
	}

	CServerSocket& operator=(const CServerSocket& ss){}

	CServerSocket(const CServerSocket& ss) {
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	}

	~CServerSocket(){
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
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}

	static CServerSocket* m_instance;

	class CHelper {
	public:
		CHelper() {
			CServerSocket::getInstance();
		}

		~CHelper() {
			CServerSocket::releaseInstance();
		}
	};

	static CHelper m_helper;
};

extern CServerSocket server;

