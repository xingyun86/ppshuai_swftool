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
		this->SetWindowText(_T("SWFѹ����ѹ����"));
		GetDlgItem(IDC_STATIC_SOURCE).SetWindowText(_T("ѡ��ѹ��/��ѹ��SWF�ļ�(����ק)"));
		GetDlgItem(IDC_STATIC_DESTINATION).SetWindowText(_T("ѡ��Ҫ�����SWF�ļ�����"));
		GetDlgItem(IDC_EDIT_SOURCE).SetWindowText(_T(""));
		GetDlgItem(IDC_EDIT_DESTINATION).SetWindowText(_T(""));
		GetDlgItem(IDC_BUTTON_SOURCE).SetWindowText(_T("ѡ���ļ�"));
		GetDlgItem(IDC_BUTTON_DESTINATION).SetWindowText(_T("ѡ��Ŀ¼"));
		GetDlgItem(IDOK).SetWindowText(_T("ѹ��/��ѹ"));
		GetDlgItem(IDCANCEL).SetWindowText(_T("�ر����"));
		GetDlgItem(ID_APP_ABOUT).SetWindowText(_T("��������"));
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
	_TCHAR tzSourceFileName[MAX_PATH] = { 0 };	// �����ļ���
	_TCHAR tzDestinationFileName[MAX_PATH] = { 0 };	// ����ļ���

	GetDlgItem(IDC_EDIT_SOURCE).GetWindowText(tzSourceFileName, sizeof(tzSourceFileName) / sizeof(*tzSourceFileName));
	GetDlgItem(IDC_EDIT_DESTINATION).GetWindowText(tzDestinationFileName, sizeof(tzDestinationFileName) / sizeof(*tzDestinationFileName));
	if (!*tzSourceFileName)
	{
		this->MessageBox(_T("Դ�ļ����Ʋ���Ϊ��"), _T("�ļ�����ʾ"), MB_ICONWARNING);
		goto __LEAVE_CLEAN__;
	}
	if (!*tzDestinationFileName)
	{
		this->MessageBox(_T("Ŀ���ļ����Ʋ���Ϊ��"), _T("�ļ�����ʾ"), MB_ICONWARNING);
		goto __LEAVE_CLEAN__;
	}
	hFile = ::CreateFile(tzSourceFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		this->MessageBox(_T("�޷����ļ�"), _T("�ļ�����ʾ"), MB_ICONWARNING);
		goto __LEAVE_CLEAN__;
	}
	// ��ȡ�ļ�����
	if (!::GetFileSizeEx(hFile, &liFileSize))
	{
		this->MessageBox(_T("�޷���ȡ�ļ�����"), _T("�ļ�����ʾ"), MB_ICONWARNING);
		goto __LEAVE_CLEAN__;
	}
	// ����swfԴ�ļ�����
	pSourceData = (BYTE*)malloc(liFileSize.QuadPart * sizeof(BYTE));
	if (!pSourceData)
	{
		this->MessageBox(_T("Դ�����ڴ����ʧ��"), _T("�ڴ������ʾ"), MB_ICONWARNING);
		goto __LEAVE_CLEAN__;
	}
	// ��ȡswf�ļ�ͷ	
	if (!::ReadFile(hFile, pSourceData, liFileSize.QuadPart, &dwNumberOfBytesRead, NULL) && (dwNumberOfBytesRead != liFileSize.QuadPart))
	{
		this->MessageBox(_T("�޷���ȡԴ�ļ�����"), _T("�ļ�����ʾ"), MB_ICONWARNING);
		goto __LEAVE_CLEAN__;
	}
	::CloseHandle(hFile);
	hFile = NULL;

	// ��ȡ�ļ���Ϣ�е��ļ�����
	dwDestinationSize = *(DWORD*)(pSourceData + (SWF_HEADER_LEN / 2));
	// ����һ���Ƿ�Ϊ������swf�ļ�
	if (!((pSourceData[0] == 'C' || pSourceData[0] == 'F') && pSourceData[1] == 'W' && 
		pSourceData[2] == 'S') || (dwDestinationSize < dwNumberOfBytesRead))
	{
		this->MessageBox(_T("���ļ�����SWF�ļ�"), _T("�ļ�����ʾ"), MB_ICONWARNING);
		goto __LEAVE_CLEAN__;
	}
	// ����swfĿ���ļ�����
	pDestinationData = (BYTE*)malloc(dwDestinationSize * sizeof(BYTE));
	if (!pDestinationData)
	{
		this->MessageBox(_T("Ŀ�������ڴ����ʧ��"), _T("�ڴ������ʾ"), MB_ICONWARNING);
		goto __LEAVE_CLEAN__;
	}
	// �����ļ�ͷѡ��ѹ��/��ѹ
	bCompress = (pSourceData[0] == 'C');
	if (bCompress)
	{
		*pDestinationData = 'F';
		// ��ѹ�����޸��ļ�ͷ��Ϣ
		uncompress(pDestinationData + SWF_HEADER_LEN, &dwDestinationSize, pSourceData + SWF_HEADER_LEN, dwNumberOfBytesRead - SWF_HEADER_LEN);
	}
	else
	{
		*pDestinationData = 'C';
		// ѹ�������޸��ļ�ͷ��Ϣ
		compress(pDestinationData + SWF_HEADER_LEN, &dwDestinationSize, pSourceData + SWF_HEADER_LEN, dwNumberOfBytesRead - SWF_HEADER_LEN);
	}

	memcpy(pDestinationData + sizeof(CHAR), pSourceData + sizeof(CHAR), SWF_HEADER_LEN - sizeof(CHAR));

	// д���ļ�
	hFile = ::CreateFile(tzDestinationFileName, FILE_WRITE_ACCESS, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		this->MessageBox(_T("���·����Ч"), _T("�ļ�������ʾ"), MB_ICONWARNING);
		goto __LEAVE_CLEAN__;
	}

	if(!::WriteFile(hFile, pDestinationData, dwDestinationSize + SWF_HEADER_LEN, &dwNumberOfBytesWritten, NULL))
	{
		this->MessageBox(_T("�޷�д��Ŀ���ļ�����"), _T("�ļ�������ʾ"), MB_ICONWARNING);
		goto __LEAVE_CLEAN__;
	}

	::CloseHandle(hFile);
	hFile = NULL;

	this->MessageBox((bCompress ? _T("��ѹ�ɹ�") : _T("ѹ���ɹ�")), _T("�ļ���ʾ"), MB_ICONINFORMATION);

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
	_TCHAR tzOldPath[MAX_PATH];	// ���浱ǰ����Ŀ¼
	::GetCurrentDirectory(MAX_PATH, tzOldPath);

	_TCHAR tzSourceName[MAX_PATH] = { 0 };
	_TCHAR tzDestinationName[MAX_PATH] = { 0 };
	// �� �򿪶Ի���
	_TCHAR szFilter[] = _T("SWF Files (*.swf)\0*.swf\0\0");
	WTL::CFileDialog dlg(true, 0, 0, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST, szFilter, m_hWnd);
	if (dlg.DoModal() == IDOK)
	{
		lstrcpy(tzSourceName, dlg.m_szFileName);
		wsprintf(tzDestinationName, _T("%s-out.swf"), dlg.m_szFileName);

		//��ȡ�ļ���
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
	//���ѡ���Ŀ¼·��
	_TCHAR tzPath[MAX_PATH] = { 0 };
	_TCHAR tzFileName[MAX_PATH] = { 0 };
	_TCHAR tzSourceFileName[MAX_PATH] = { 0 };

	::GetCurrentDirectory(MAX_PATH, tzOldPath);
	
	::ZeroMemory(tzPath, sizeof(tzPath));

	bi.hwndOwner = m_hWnd;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = tzPath;
	bi.lpszTitle = _T("��ѡ����Ҫ�����Ŀ¼��");
	bi.ulFlags = 0;
	bi.lpfn = NULL;
	bi.lParam = 0;
	bi.iImage = 0;
	//����ѡ��Ŀ¼�Ի���
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
		this->MessageBox(_T("��Ч��Ŀ¼��������ѡ��"), _T("ѡ��Ŀ¼��ʾ"), MB_ICONWARNING);
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
