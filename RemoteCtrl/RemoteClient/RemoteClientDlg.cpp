
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "WatchDialog.h"
#include "ClientController.h"

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
public:

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
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
	ON_NOTIFY(NM_THEMECHANGED, IDC_IPADDRESS_SERV, &CRemoteClientDlg::OnNMThemeChangedIpaddressServ)
	ON_EN_CHANGE(IDC_EDIT_PORT, &CRemoteClientDlg::OnEnChangeEditPort)
	ON_MESSAGE(WM_SEND_PACK_ACK, &CRemoteClientDlg::OnSendPackAck)
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

	InitUIData();

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
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1981);
}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	std::list<CPacket> lstPackets;
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1, true, NULL, 0); // 查看磁盘分区

	if (ret == 0) {
		AfxMessageBox(_T("命令处理失败！！！"));
		return;
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

	DeleteTreeChildrenItem(HTreeSelected);
	m_List.DeleteAllItems();

	// 点到节点上时，拼接完整路径
	CString strPath = GetPath(HTreeSelected);

	// 将路径发送到受控端
	std::list<CPacket> lstPackets;
	int nCmd = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false,
		(BYTE*)(LPCTSTR)strPath, strPath.GetLength(), (WPARAM)HTreeSelected);
}

void CRemoteClientDlg::LoadFileCurrent()
{
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hTree);

	m_List.DeleteAllItems();

	// 将路径发送到受控端
	int nCmd = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false, (BYTE*)(LPCTSTR)strPath);

	// 接收受控端返回的信息
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();

	while (pInfo->hasNext)
	{
		TRACE("[%s] isDir %d\r\n", pInfo->szFileName, pInfo->isDirectory);
		if (pInfo->isDirectory) {
			m_List.InsertItem(0, pInfo->szFileName);
		}

		int cmd = CClientController::getInstance()->DealCommand();

		if (cmd < 0) {
			TRACE("Deal Command failed %d!!!\r\n", cmd);
			break;
		}

		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}

	// CClientController::getInstance()->CloseSocket();
}

void CRemoteClientDlg::Str2Tree(const std::string& drivers, CTreeCtrl& tree)
{
	std::string dr; // 临时变量
	tree.DeleteAllItems();

	for (size_t i = 0; i < drivers.size(); i++) {
		if (drivers[i] == ',') {
			if (!dr.empty()) {
				dr += ":";
				HTREEITEM hTemp = tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
				tree.InsertItem("", hTemp, TVI_LAST);
				dr.clear();
			}
		}
		else {
			dr += drivers[i];
		}
	}

	// 处理最后一个驱动
	if (!dr.empty()) {
		dr += ":";
		HTREEITEM hTemp = tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
		tree.InsertItem("", hTemp, TVI_LAST);
	}
}

void CRemoteClientDlg::UpdateFileInfo(const FILEINFO& finfo, HTREEITEM hParent)
{
	TRACE("HasNext %d isdirectory %d %s\r\n", finfo.hasNext, finfo.isDirectory, finfo.szFileName);
	if (!finfo.hasNext) return;
	if (finfo.isDirectory) {
		CString fileName(finfo.szFileName);
		if (fileName == "." || fileName == "..") return;
		TRACE("hselected %08X\r\n", hParent);
		HTREEITEM hTemp = m_Tree.InsertItem(finfo.szFileName, static_cast<HTREEITEM>(hParent), TVI_LAST);
		m_Tree.InsertItem("", hTemp, TVI_LAST);
		m_Tree.Expand(hParent, TVE_EXPAND);
	}
	else {
		m_List.InsertItem(0, finfo.szFileName);
	}
}

void CRemoteClientDlg::UpdateDownloadFile(const std::string& strData, FILE* pFile)
{
	static LONGLONG length = 0, index = 0;
	if (length == 0) {
		length = *(long long*)strData.c_str();
		if (length == 0) {
			AfxMessageBox("文件长度为0或者无法读取文件!!!");
			CClientController::getInstance()->DownloadEnd();
		}
	}
	else if (length > 0 && index >= length) {
		fclose(pFile);
		length = 0;
		index = 0;
		CClientController::getInstance()->DownloadEnd();
	}
	else {
		fwrite(strData.c_str(), 1, strData.size(), pFile);
		index += strData.size();
		if (index >= length) {
			fclose(pFile);
			length = 0;
			index = 0;
			CClientController::getInstance()->DownloadEnd();
		}
	}
}

void CRemoteClientDlg::InitUIData()
{
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标
	UpdateData();
	m_server_address = 0x7F000001; // 0x7F000001对应127.0.0.1
	m_nPort = _T("9527");
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));
	UpdateData(FALSE);
	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);
}

void CRemoteClientDlg::DealCommand(WORD nCmd, const std::string& strData, LPARAM lParam)
{
	switch (nCmd) {

	case 1: // 获取驱动信息
		Str2Tree(strData, m_Tree);
		break;
	case 2: // 获取文件信息
		UpdateFileInfo(*(PFILEINFO)strData.c_str(), (HTREEITEM)lParam);
		break;
	case 3:
		MessageBox("打开文件完成!", "操作完成", MB_ICONINFORMATION);
		break;
	case 4: // 下载文件
		UpdateDownloadFile(strData, (FILE*)lParam);
		break;
	case 9:
		MessageBox("删除文件完成!", "操作完成", MB_ICONINFORMATION);
		break;
	case 1981:
		MessageBox("连接测试成功!", "连接成功", MB_ICONINFORMATION);
		break;
	default:
		TRACE("Unknow data received! %d \r\n", nCmd);
		break;
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
	int nListSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nListSelected, 0);

	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	strFile = GetPath(hSelected) + strFile;

	int ret = CClientController::getInstance()->DownFile(strFile);

	if (ret != 0) {
		MessageBox(_T("下载失败！"));
		TRACE("下载失败 ret = %d\r\n", ret);
	}
}

void CRemoteClientDlg::OnDeleteFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);

	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);

	strFile = strPath + strFile;
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 9, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());

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

	strFile = strPath + +"\\" + strFile;

	TRACE("Run file name is:%s\r\n", strFile);
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 3, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());

	if (ret < 0) {
		AfxMessageBox("打开文件执行命令失败！！！");
		return;
	}

}

void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	CClientController::getInstance()->StartWatchScreen();
}


void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnTimer(nIDEvent);
}


void CRemoteClientDlg::OnNMThemeChangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS nIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	*pResult = 0;
	UpdateData();
	CClientController* pController = CClientController::getInstance();

	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));
}


void CRemoteClientDlg::OnEnChangeEditPort()
{
	UpdateData();
	CClientController* pController = CClientController::getInstance();

	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));
}

LRESULT CRemoteClientDlg::OnSendPackAck(WPARAM wParam, LPARAM lParam)
{
	if ((lParam == -1) || (lParam == -2)) {
		// TODO: 错误处理
		TRACE("Socket is error! %d\r\n", lParam);
	}
	else if (lParam == 1) {
		// 对方关闭了套接字
		TRACE("Socket is closed!\r\n");
	}
	else {
		if (wParam != NULL) {
			CPacket head = *(CPacket*)wParam;
			delete (CPacket*)wParam;
			DealCommand(head.sCmd, head.strData, lParam);
		}
	}
	return 0;
}
