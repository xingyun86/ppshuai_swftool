// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "MainDlg.h"

BOOL CMainDlg::PreTranslateMessage(MSG* pMsg)
{
	return CWindow::IsDialogMessage(pMsg);
}

BOOL CMainDlg::OnIdle()
{
	UIUpdateChildWindows();
	return FALSE;
}

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	UIAddChildWindowContainer(m_hWnd);
	{
		this->SetWindowText(_T("SWF压缩解压工具"));
		GetDlgItem(IDC_STATIC_SOURCE).SetWindowText(_T("选择压缩/解压的SWF文件(可拖拽)"));
		GetDlgItem(IDC_STATIC_DESTINATION).SetWindowText(_T("选择要输出的SWF文件名称"));
		GetDlgItem(IDC_EDIT_SOURCE).SetWindowText(_T(""));
		GetDlgItem(IDC_EDIT_DESTINATION).SetWindowText(_T(""));
		GetDlgItem(IDC_BUTTON_SOURCE).SetWindowText(_T("选择文件"));
		GetDlgItem(IDC_BUTTON_DESTINATION).SetWindowText(_T("选择目录"));
		GetDlgItem(IDOK).SetWindowText(_T("压缩/解压"));
		GetDlgItem(IDCANCEL).SetWindowText(_T("关闭软件"));
		GetDlgItem(ID_APP_ABOUT).SetWindowText(_T("关于我们"));
		RegisterDropFilesEvent(m_hWnd);
	}

	return TRUE;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	// if UI is the last thread, no need to wait
	if(_Module.GetLockCount() == 1)
	{
		_Module.m_dwTimeOut = 0L;
		_Module.m_dwPause = 0L;
	}
	_Module.Unlock();

	return 0;
}

LRESULT CMainDlg::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CMainDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add validation code 
	//CloseDialog(wID);
#define SWF_HEADER_LEN 8
	HANDLE hFile = NULL;
	bool bCompress = false;
	BYTE* pSourceData = NULL;
	BYTE* pDestinationData = NULL;
	DWORD dwNumberOfBytesRead = 0;
	DWORD dwNumberOfBytesWritten = 0;
	DWORD dwDestinationSize = (0L);
	LARGE_INTEGER liFileSize = { 0L, 0L };
	_TCHAR tzSourceFileName[MAX_PATH] = { 0 };	// 输入文件名
	_TCHAR tzDestinationFileName[MAX_PATH] = { 0 };	// 输出文件名

	GetDlgItem(IDC_EDIT_SOURCE).GetWindowText(tzSourceFileName, sizeof(tzSourceFileName) / sizeof(*tzSourceFileName));
	GetDlgItem(IDC_EDIT_DESTINATION).GetWindowText(tzDestinationFileName, sizeof(tzDestinationFileName) / sizeof(*tzDestinationFileName));
	if (!*tzSourceFileName)
	{
		this->MessageBox(_T("源文件名称不能为空"), _T("文件打开提示"), MB_ICONWARNING);
		goto __LEAVE_CLEAN__;
	}
	if (!*tzDestinationFileName)
	{
		this->MessageBox(_T("目标文件名称不能为空"), _T("文件打开提示"), MB_ICONWARNING);
		goto __LEAVE_CLEAN__;
	}
	hFile = ::CreateFile(tzSourceFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		this->MessageBox(_T("无法打开文件"), _T("文件打开提示"), MB_ICONWARNING);
		goto __LEAVE_CLEAN__;
	}
	// 获取文件长度
	if (!::GetFileSizeEx(hFile, &liFileSize))
	{
		this->MessageBox(_T("无法获取文件长度"), _T("文件打开提示"), MB_ICONWARNING);
		goto __LEAVE_CLEAN__;
	}
	// 创建swf源文件缓存
	pSourceData = (BYTE*)malloc(liFileSize.QuadPart * sizeof(BYTE));
	if (!pSourceData)
	{
		this->MessageBox(_T("源数据内存分配失败"), _T("内存分配提示"), MB_ICONWARNING);
		goto __LEAVE_CLEAN__;
	}
	// 读取swf文件头	
	if (!::ReadFile(hFile, pSourceData, liFileSize.QuadPart, &dwNumberOfBytesRead, NULL) && (dwNumberOfBytesRead != liFileSize.QuadPart))
	{
		this->MessageBox(_T("无法读取源文件数据"), _T("文件打开提示"), MB_ICONWARNING);
		goto __LEAVE_CLEAN__;
	}
	::CloseHandle(hFile);
	hFile = NULL;

	// 获取文件信息中的文件长度
	dwDestinationSize = *(DWORD*)(pSourceData + (SWF_HEADER_LEN / 2));
	// 检验一下是否为正常的swf文件
	if (!((pSourceData[0] == 'C' || pSourceData[0] == 'F') && pSourceData[1] == 'W' && 
		pSourceData[2] == 'S') || (dwDestinationSize < dwNumberOfBytesRead))
	{
		this->MessageBox(_T("打开文件并非SWF文件"), _T("文件打开提示"), MB_ICONWARNING);
		goto __LEAVE_CLEAN__;
	}
	// 创建swf目标文件缓存
	pDestinationData = (BYTE*)malloc(dwDestinationSize * sizeof(BYTE));
	if (!pDestinationData)
	{
		this->MessageBox(_T("目标数据内存分配失败"), _T("内存分配提示"), MB_ICONWARNING);
		goto __LEAVE_CLEAN__;
	}
	// 根据文件头选择压缩/解压
	bCompress = (pSourceData[0] == 'C');
	if (bCompress)
	{
		*pDestinationData = 'F';
		// 解压，并修改文件头信息
		uncompress(pDestinationData + SWF_HEADER_LEN, &dwDestinationSize, pSourceData + SWF_HEADER_LEN, dwNumberOfBytesRead - SWF_HEADER_LEN);
	}
	else
	{
		*pDestinationData = 'C';
		// 压缩，并修改文件头信息
		compress(pDestinationData + SWF_HEADER_LEN, &dwDestinationSize, pSourceData + SWF_HEADER_LEN, dwNumberOfBytesRead - SWF_HEADER_LEN);
	}

	memcpy(pDestinationData + sizeof(CHAR), pSourceData + sizeof(CHAR), SWF_HEADER_LEN - sizeof(CHAR));

	// 写入文件
	hFile = ::CreateFile(tzDestinationFileName, FILE_WRITE_ACCESS, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		this->MessageBox(_T("输出路径无效"), _T("文件保存提示"), MB_ICONWARNING);
		goto __LEAVE_CLEAN__;
	}

	if(!::WriteFile(hFile, pDestinationData, dwDestinationSize + SWF_HEADER_LEN, &dwNumberOfBytesWritten, NULL))
	{
		this->MessageBox(_T("无法写入目标文件数据"), _T("文件保存提示"), MB_ICONWARNING);
		goto __LEAVE_CLEAN__;
	}

	::CloseHandle(hFile);
	hFile = NULL;

	this->MessageBox((bCompress ? _T("解压成功") : _T("压缩成功")), _T("文件提示"), MB_ICONINFORMATION);

__LEAVE_CLEAN__:

	if (hFile)
	{
		::CloseHandle(hFile);
		hFile = NULL;
	}
	if (pSourceData)
	{
		free(pSourceData);
	}
	if (pDestinationData)
	{
		free(pDestinationData);
	}

	return 0;
}

LRESULT CMainDlg::OnSourceBrowser(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	_TCHAR tzOldPath[MAX_PATH];	// 保存当前工作目录
	::GetCurrentDirectory(MAX_PATH, tzOldPath);

	_TCHAR tzSourceName[MAX_PATH] = { 0 };
	_TCHAR tzDestinationName[MAX_PATH] = { 0 };
	// 打开 打开对话框
	_TCHAR szFilter[] = _T("SWF Files (*.swf)\0*.swf\0\0");
	WTL::CFileDialog dlg(true, 0, 0, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST, szFilter, m_hWnd);
	if (dlg.DoModal() == IDOK)
	{
		lstrcpy(tzSourceName, dlg.m_szFileName);
		wsprintf(tzDestinationName, _T("%s-out.swf"), dlg.m_szFileName);

		//获取文件名
		GetDlgItem(IDC_EDIT_SOURCE).SetWindowText(tzSourceName);
		GetDlgItem(IDC_EDIT_DESTINATION).SetWindowText(tzDestinationName);
	}

	::SetCurrentDirectory(tzOldPath);
	return 0;
}

LRESULT CMainDlg::OnDestinationBrowser(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	//CloseDialog(wID);
	BROWSEINFO bi = { 0 };
	LPITEMIDLIST lpItemIdlList = NULL;
	_TCHAR tzOldPath[MAX_PATH] = { 0 };
	//存放选择的目录路径
	_TCHAR tzPath[MAX_PATH] = { 0 };
	_TCHAR tzFileName[MAX_PATH] = { 0 };
	_TCHAR tzSourceFileName[MAX_PATH] = { 0 };

	::GetCurrentDirectory(MAX_PATH, tzOldPath);
	
	::ZeroMemory(tzPath, sizeof(tzPath));

	bi.hwndOwner = m_hWnd;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = tzPath;
	bi.lpszTitle = _T("请选择需要打包的目录：");
	bi.ulFlags = 0;
	bi.lpfn = NULL;
	bi.lParam = 0;
	bi.iImage = 0;
	//弹出选择目录对话框
	lpItemIdlList = ::SHBrowseForFolder(&bi);
	
	if (lpItemIdlList && ::SHGetPathFromIDList(lpItemIdlList, tzPath))
	{
		GetDlgItem(IDC_EDIT_SOURCE).GetWindowText(tzSourceFileName, sizeof(tzSourceFileName) / sizeof(*tzSourceFileName));
		lstrcpy(_tcsrchr(tzSourceFileName, _T('\\')) + sizeof(char), m_tzFileName);
		wsprintf(tzFileName, _T("%s\\%s-out.swf"), tzPath, m_tzFileName);
		GetDlgItem(IDC_EDIT_DESTINATION).SetWindowText(tzFileName);
	}
	else
	{
		this->MessageBox(_T("无效的目录，请重新选择"), _T("选择目录提示"), MB_ICONWARNING);
	}

	::SetCurrentDirectory(tzOldPath);
	return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CloseDialog(wID);
	return 0;
}

void CMainDlg::CloseDialog(int nVal)
{
	DestroyWindow();
	::PostQuitMessage(nVal);
}
