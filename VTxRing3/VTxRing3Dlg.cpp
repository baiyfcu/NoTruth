
// VTxRing3Dlg.cpp : implementation file
//

#include "stdafx.h"
#include "VTxRing3.h"
#include "VTxRing3Dlg.h"
#include "afxdialogex.h"
#include "cDrvCtrl.h"
#include "IOCTL.h"	
#include "tlhelp32.h"
#include "Hook.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include <winternl.h>

typedef NTSTATUS(*pMyNtCreateThread)(
	OUT PHANDLE ThreadHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	IN HANDLE ProcessHandle,
	OUT PVOID ClientId,
	IN PCONTEXT ThreadContext,
	IN PVOID InitialTeb,
	IN BOOLEAN CreateSuspended);

#define DRV_PATH		"C:\\NoTruth.sys"
#define SERVICE_NAME	"NoTruthtest5"
#define DISPLAY_NAME	SERVICE_NAME
HOOKOBJ* g_HookObj = NULL;
pMyNtCreateThread g_NtCreateThread = NULL;
#pragma comment(lib,"ntdll.lib") // Need to link with ntdll.lib import library. You can find the ntdll.lib from the Windows DDK.
 
typedef struct _TRANSFER_IOCTL
{
	ULONG64 ProcID;
	ULONG64 HiddenType;
	ULONG64 Address;
}TRANSFERIOCTL, *PTRANSFERIOCTL;
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
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


// CVTxRing3Dlg dialog



CVTxRing3Dlg::CVTxRing3Dlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_VTXRING3_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CVTxRing3Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CVTxRing3Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CVTxRing3Dlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CVTxRing3Dlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON1, &CVTxRing3Dlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CVTxRing3Dlg::OnBnClickedButton2)
END_MESSAGE_MAP()


// CVTxRing3Dlg message handlers

BOOL CVTxRing3Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
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

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CVTxRing3Dlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CVTxRing3Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CVTxRing3Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

DWORD FindProcessId(WCHAR*processname)
{
	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;
	DWORD result = NULL;

	// Take a snapshot of all processes in the system.
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hProcessSnap) return(FALSE);

	pe32.dwSize = sizeof(PROCESSENTRY32); // <----- IMPORTANT

										  // Retrieve information about the first process,
										  // and exit if unsuccessful
	if (!Process32First(hProcessSnap, &pe32))
	{
		CloseHandle(hProcessSnap);          // clean the snapshot object
	//	OutputDebugStringA("!!! Failed to gather information on system processes! \n");
		return(NULL);
	}

	do
	{
//		OutputDebugStringA("Checking process\n");
		if (0 == _wcsicmp(processname, pe32.szExeFile))
		{
			result = pe32.th32ProcessID;
			break;
		}
	} while (Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);

	return result;
}
cDrvCtrl drv;
//--------------------------------------------------------------------------------------------------------------------//
BOOL WipeCopyOnWrite(
	_In_ HANDLE				 handle, 
	_In_ TRANSFERIOCTL TransferData
)
{
	BOOL				ret = FALSE;
	CString						err;
	UCHAR			 	  value = 0;
	ULONG			 oldProtect = 0;

	do {
		// Start Wipe
		if (!VirtualProtectEx(handle, (LPVOID)TransferData.Address, sizeof(value), PAGE_EXECUTE_WRITECOPY, &oldProtect))
		{
			err.Format(L"VirtualProtect1 - LastError: %d \r\n", GetLastError());
			OutputDebugString(err);
			break;
		}
		//Read an original value
		if (!ReadProcessMemory(handle, (LPVOID)TransferData.Address, &value, sizeof(value), NULL))
		{
			err.Format(L"ReadProcessMemory - LastError: %d \r\n", GetLastError());
			OutputDebugString(err);
			break;
		}

		//Wipe a Copy on Write, write a value, System will create a page for me
		if (!WriteProcessMemory(handle, (PVOID)TransferData.Address, &value, 1, NULL))
		{
			err.Format(L"WriteProcessMemory  - LastError: %d \r\n", GetLastError()); 
			OutputDebugString(err);
			VirtualProtectEx(handle, (LPVOID)TransferData.Address, sizeof(value), oldProtect, NULL);
			break;
		}

		// Stop Wipe
		if (!VirtualProtectEx(handle, (LPVOID)TransferData.Address, sizeof(value), oldProtect, &oldProtect))
		{ 
			err.Format(L"VirtualProtectEx  - LastError: %d \r\n", GetLastError());
			OutputDebugString(err);
			break; 
		}

		err.Format(L"[NOTEPAD]ProcID: %x Address1: %X oldValue : %X \r\n", TransferData.ProcID, TransferData.Address, value);
		OutputDebugString(err);

		ret = TRUE;

	} while (false);

	return ret;
}

 
template <typename T>
static T  FindOrignal(T handler)
{
	return reinterpret_cast<T>(g_HookObj->JmpToOrg);
}

extern "C" {
	//-------------------------------------//
	ULONG Test1()
	{
		OutputDebugStringA("Test1");
		return 0;
	}
	//-------------------------------------//

	ULONG Test2()
	{
		OutputDebugStringA("Test2");
		const auto Original = FindOrignal(Test1);
		return Original();
	}

} 

NTSTATUS 
MyNtCreateThread(
	OUT PHANDLE ThreadHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	IN HANDLE ProcessHandle,
	OUT PVOID ClientId,
	IN PCONTEXT ThreadContext,
	IN PVOID InitialTeb,
	IN BOOLEAN CreateSuspended)
{
	CString str; 
	OutputDebugString(L"Test my thread hook \r\n");
	const auto Original = FindOrignal(MyNtCreateThread);
	const auto status = Original(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, ClientId, ThreadContext, InitialTeb, CreateSuspended); 
	str.Format(L"Status: %x Err: %x \r\n", status, GetLastError());
	OutputDebugString(str); 
	return status;
}

DWORD WINAPI ExecuteThread(PVOID Param)
{
	CString str;
	ULONG  Hash;
	while (1)
	{
		ULONG value = 0;
		value = g_NtCreateThread(0, 0, 0, 0, 0, 0, 0, 0);;
		str.Format(L"Return Value : %x \r\n", value);
		OutputDebugString(str);
		Sleep(1000);
	}
	return 0;
}
DWORD WINAPI CheckSumThread(PVOID Param)
{
	CString str; 
	ULONG  Hash;
 
	while (1)
	{
		ULONG value = 0;
		value = *(PULONG)g_NtCreateThread;
		str.Format(L"value: %x \r\n", value); 
		OutputDebugString(str);
		Sleep(1000);
	}
	return 0;
}
 
//-------------------------------------//
void CVTxRing3Dlg::OnBnClickedOk()
{ 
	ULONG OutBuffer, RetBytes = 0;
	TRANSFERIOCTL transferData2 = { 0 };
	DWORD					pid = (DWORD)GetCurrentProcessId();
	HANDLE				 handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
 

	g_NtCreateThread = (pMyNtCreateThread)GetProcAddress(LoadLibraryA("ntdll.dll"), "NtCreateThread");


	PVOID NtCreateFile   = (PVOID)GetProcAddress(LoadLibraryA("ntdll.dll"), "NtCreateFile");

	CString err;

	transferData2.ProcID = GetCurrentProcessId();
	transferData2.HiddenType = 0x0;
	transferData2.Address = (ULONG64)g_NtCreateThread;
	 
	//Create Service
	if (!drv.Install(DRV_PATH, SERVICE_NAME, DISPLAY_NAME))
	{
		OutputDebugStringA("Change Page A222\r\n");
		CloseHandle(handle);
		return;
	} 

	//Start Service
	if (!drv.Start(SERVICE_NAME))
	{
		OutputDebugStringA("Change Page 333Attribute to Writable \r\n");

		drv.Remove(SERVICE_NAME);
		CloseHandle(handle);
		return;
	}
  
	OutputDebugStringA("Change Page Attribute to Writable \r\n");
 
	if (!WipeCopyOnWrite(handle, transferData2))
	{
		drv.Stop(SERVICE_NAME);
		drv.Remove(SERVICE_NAME);
		CloseHandle(handle);
		return;
	}
	
	OutputDebugStringA("Wiped Copy-On-Write \r\n"); 
	OutputDebugStringA("Change Page Attribute to Original \r\n");


	if (!drv.IoControl("\\\\.\\NoTruth",IOCTL_HIDE_ADD, &transferData2, sizeof(TRANSFERIOCTL), &OutBuffer, sizeof(ULONG), &RetBytes))
	{
		drv.Stop(SERVICE_NAME);
		drv.Remove(SERVICE_NAME);
		CloseHandle(handle);
		return;
		AfxMessageBox(L"Cannot IOCTL device \r\n");
	}

	if (!drv.IoControl("\\\\.\\NoTruth",IOCTL_HIDE_START, NULL, 0, NULL, 0, &RetBytes))
	{
		drv.Stop(SERVICE_NAME);
		drv.Remove(SERVICE_NAME);
		CloseHandle(handle);
		return;
		AfxMessageBox(L"Cannot IOCTL device \r\n");
	}
	 
 	CloseHandle(handle);

	CString str;
	ULONG value = 0;
	value = *(PULONG)g_NtCreateThread;
	str.Format(L"value: %x \r\n", value);
	OutputDebugString(str);
  
	SetupInlineHook_X64(&g_HookObj, g_NtCreateThread, MyNtCreateThread);
	 
	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)CheckSumThread, 0, 0, 0);
	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ExecuteThread, 0, 0, 0); 

	OutputDebugStringA("Successfully Hide \r\n"); 
	 

}


void CVTxRing3Dlg::OnBnClickedCancel()
{
	CDialog::OnCancel(); 
	drv.Stop(SERVICE_NAME);
	drv.Remove(SERVICE_NAME);
}


void CVTxRing3Dlg::OnBnClickedButton1()
{
}

void CVTxRing3Dlg::OnBnClickedButton2()
{
	// TODO: Add your control notification handler code here
	drv.Stop(SERVICE_NAME);
	drv.Remove(SERVICE_NAME);
}
