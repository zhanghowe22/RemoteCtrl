
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "ClientSocket.h"
#include "WatchDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_server_address(0)
	, m_nPort(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}

int CRemoteClientDlg::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData /*= NULL*/, size_t nLength /*= 0*/)
{
	UpdateData();
	CClientSocket* pClient = CClientSocket::getInstance();
	bool ret = pClient->InitSocket(m_server_address, atoi((LPCTSTR)m_nPort));
	if (!ret) {
		AfxMessageBox(_T("网络初始化失败！"));
		return -1;
	}

	CPacket pack(nCmd, pData, nLength);

	ret = pClient->Send(pack);
	TRACE("Send ret %d\r\n", ret);

	int cmd = pClient->DealCommand();

	TRACE("ack:%d\r\n", cmd);

	if(bAutoClose)
		pClient->CloseSocket();

	return cmd;
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
	ON_MESSAGE(WM_SEND_PACKET, &CRemoteClientDlg::OnSendPakcet) // ③ 在消息映射表注册消息
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	UpdateData();
	m_server_address = 0x7F000001; // 0x7F000001对应127.0.0.1
	m_nPort = _T("9527");
	UpdateData(FALSE);
	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);
	m_isFull = false;
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CRemoteClientDlg::OnBnClickedBtnTest()
{
	SendCommandPacket(1981);
}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	int ret = SendCommandPacket(1); // 查看磁盘分区

	if (ret == -1) {
		AfxMessageBox(_T("命令处理失败！！！"));
		return;
	}

	CClientSocket* pClient = CClientSocket::getInstance();
	std::string drivers = pClient->GetPacket().strData;
	std::string dr; // 临时变量
	m_Tree.DeleteAllItems();
	HTREEITEM hTemp;

	for (size_t i = 0; i <= drivers.size(); i++) {
		if (i == drivers.size() || drivers[i] == ',') {
			dr += ":";
			hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
			m_Tree.InsertItem("", hTemp, TVI_LAST);
			dr.clear();
		}
		else {
			dr += drivers[i];
		}
	}
}


CString CRemoteClientDlg::GetPath(HTREEITEM hTree)
{
	CString strRet;

	// 遍历父节点并逐层插入节点文本
	while (hTree != nullptr) {
		CString strTmp = m_Tree.GetItemText(hTree);
		strRet.Insert(0, strTmp + '\\');
		hTree = m_Tree.GetParentItem(hTree);
	}

	return strRet;
}

void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree)
{
	HTREEITEM hSub = NULL;

	do
	{
		hSub = m_Tree.GetChildItem(hTree);
		if (hSub != NULL) m_Tree.DeleteItem(hSub);
	} while (hSub);
}

void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_Tree.ScreenToClient(&ptMouse);

	// 判断双击是否点击在节点上
	HTREEITEM HTreeSelected = m_Tree.HitTest(ptMouse, 0);
	if (HTreeSelected == NULL) {
		return;
	}

	if (m_Tree.GetChildItem(HTreeSelected) == NULL)
		return;

	DeleteTreeChildrenItem(HTreeSelected);
	m_List.DeleteAllItems();

	// 点到节点上时，拼接完整路径
	CString strPath = GetPath(HTreeSelected);

	// 将路径发送到受控端
	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());

	// 接收受控端返回的信息
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();

	CClientSocket* pClient = CClientSocket::getInstance();
	while (pInfo->hasNext)
	{
		TRACE("[%s] isDir %d\r\n", pInfo->szFileName, pInfo->isDirectory);
		if (pInfo->isDirectory) {
			if (CString(pInfo->szFileName) == "." || CString(pInfo->szFileName) == "..") {
				int cmd = pClient->DealCommand();
				TRACE("ack:%d\r\n", cmd);
				if (cmd < 0) break;
				pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
				continue;
			}
			HTREEITEM hTemp = m_Tree.InsertItem(pInfo->szFileName, HTreeSelected, TVI_LAST);
			TRACE("Client recv dir name is: %s\r\n", pInfo->szFileName);
			m_Tree.InsertItem("", hTemp, TVI_LAST);
		}
		else {
			m_List.InsertItem(0, pInfo->szFileName);
			TRACE("Have next [%d]\r\n", pInfo->hasNext);
			TRACE("Client recv file name is: %s\r\n", pInfo->szFileName);
		}

		int cmd = pClient->DealCommand();

		if (cmd < 0) {
			TRACE("Deal Command failed %d!!!\r\n", cmd);
			break;
		}

		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}

	pClient->CloseSocket();
}

void CRemoteClientDlg::LoadFileCurrent()
{
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hTree);

	m_List.DeleteAllItems();

	// 将路径发送到受控端
	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());

	// 接收受控端返回的信息
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();

	CClientSocket* pClient = CClientSocket::getInstance();
	while (pInfo->hasNext)
	{
		TRACE("[%s] isDir %d\r\n", pInfo->szFileName, pInfo->isDirectory);
		if (pInfo->isDirectory) {
			m_List.InsertItem(0, pInfo->szFileName);
		}

		int cmd = pClient->DealCommand();

		if (cmd < 0) {
			TRACE("Deal Command failed %d!!!\r\n", cmd);
			break;
		}

		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}

	pClient->CloseSocket();
}

// 文件下载的处理线程
void CRemoteClientDlg::threadEntryForDownFile(void* arg)
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
	thiz->threadDownFile();
	_endthread();
}

void CRemoteClientDlg::threadDownFile()
{
	int nListSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nListSelected, 0);

	// 文件选择弹窗
	CFileDialog dlg(false, "*",
		strFile, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		NULL, this);

	if (dlg.DoModal() == IDOK) {
		FILE* pFile = fopen(dlg.GetPathName(), "wb+");
		if (pFile == NULL) {
			AfxMessageBox("本地没有权限保存该文件，或者文件无法创建！！！");
			m_dlgStatus.ShowWindow(SW_HIDE);
			EndWaitCursor();
			return;
		}

		HTREEITEM hSelected = m_Tree.GetSelectedItem();
		strFile = GetPath(hSelected) + strFile;
		CClientSocket* pClient = CClientSocket::getInstance();
		TRACE("%s\r\n", LPCSTR(strFile));

		do
		{
			// int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
			int ret = SendMessage(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)(LPCSTR)strFile);
			if (ret < 0) {
				AfxMessageBox("执行下载命令失败!!!");
				TRACE("执行下载命令失败：ret = %d \r\n", ret);
				break;
			}

			long long nLength = *(long long*)pClient->GetPacket().strData.c_str();
			if (nLength == 0) {
				AfxMessageBox("文件长度为0或者无法读取文件！！！");
				break;
			}
			long long nCount = 0;
			while (nCount < nLength) {
				ret = pClient->DealCommand();
				if (ret < 0) {
					AfxMessageBox("传输失败!!!");
					TRACE("传输失败：ret = %d \r\n", ret);
					break;
				}

				fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
				nCount += pClient->GetPacket().strData.size();
			}

		} while (false);

		fclose(pFile);
		pClient->CloseSocket();
	}
	m_dlgStatus.ShowWindow(SW_HIDE);
	EndWaitCursor();
	MessageBox(_T("下载完成！！！"), _T("完成"));
}

void CRemoteClientDlg::threadEntryForWatchData(void* arg)
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
	thiz->threadWatchData();
	_endthread();
}

void CRemoteClientDlg::threadWatchData()
{
	CClientSocket* pClient = NULL;
	do {
		pClient = CClientSocket::getInstance();
	} while (pClient == NULL);
	for (;;) { // while(true)
		CPacket pack(6, NULL, 0);
		bool ret = pClient->Send(pack);
		
		if (ret) {
			int cmd = pClient->DealCommand(); // 拿数据
			if (cmd == 6) {
				if (m_isFull == false) { // 更新数据到缓存
					BYTE* pData = (BYTE*)pClient->GetPacket().strData.c_str();

					HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
					if (hMem == NULL) {
						TRACE("内存不足了！");
						Sleep(1);
						continue;
					}

					IStream* pStream = NULL;

					HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
					if (hRet == S_OK) {
						ULONG length = 0;
						pStream->Write(pData, pClient->GetPacket().strData.size(), &length);
						LARGE_INTEGER bg = { 0 };
						pStream->Seek(bg, STREAM_SEEK_SET, NULL);
						m_image.Load(pStream);
						m_isFull = true;
					}
				}
			}
		}
		else {
			Sleep(1);
		}
	}
}

// 双击
void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;

	LoadFileInfo();
}

// 单击
void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;

	LoadFileInfo();
}


void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	CPoint ptMouse, ptList;
	GetCursorPos(&ptMouse);

	ptList = ptMouse;
	m_List.ScreenToClient(&ptList);

	int ListSelected = m_List.HitTest(ptList);
	if (ListSelected < 0) return;

	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);
	CMenu* pPupup = menu.GetSubMenu(0);
	if (pPupup != NULL) {
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTALIGN, ptMouse.x, ptMouse.y, this);
	}
}

void CRemoteClientDlg::OnDownloadFile()
{
	// 添加线程函数用来处理文件接收，防止接收大文件时主线程被阻塞
	_beginthread(CRemoteClientDlg::threadEntryForDownFile, 0, this);
	// 设置等待光标
	BeginWaitCursor();
	m_dlgStatus.m_info.SetWindowText(_T("命令正在执行中！"));
	m_dlgStatus.ShowWindow(SW_SHOW);
	// 居中
	m_dlgStatus.CenterWindow(this);
	m_dlgStatus.SetActiveWindow();
}

void CRemoteClientDlg::OnDeleteFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);

	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);

	strFile = strPath + strFile;
	int ret = SendCommandPacket(9, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());

	if (ret < 0) {
		AfxMessageBox("删除文件执行命令失败！！！");
		return;
	}

	// 删除后刷新界面
	LoadFileCurrent();
}


void CRemoteClientDlg::OnRunFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);

	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);

	strFile = strPath + + "\\" +strFile;

	TRACE("Run file name is:%s\r\n", strFile);
	int ret = SendCommandPacket(3, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());

	if (ret < 0) {
		AfxMessageBox("打开文件执行命令失败！！！");
		return;
	}

}

LRESULT CRemoteClientDlg::OnSendPakcet(WPARAM wParam, LPARAM lParam)
{
	// ④ 实现消息响应函数
	CString strFile = (LPCSTR)lParam;
	int ret = SendCommandPacket(wParam >> 1, wParam & 1, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	return ret;
}


void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	_beginthread(CRemoteClientDlg::threadEntryForWatchData, 0, this);
	CWatchDialog dlg(this);
	dlg.DoModal();
}


void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnTimer(nIDEvent);
}
