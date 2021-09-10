// Process.cpp: Enhancing the Windows Process & Threading System.
//
// Thread-Safe:		YES
// 


//////////////////
// Includes		//
//////////////////
#include "stdafx.h"
#include "math.h"

// https://msdn.microsoft.com/en-us/library/ms683217(VS.85).aspx
#include "psapi.h"								// << GetProcessImageFileName

#include "_modules.h"


//////////////
// Consts	//
//////////////
const long wmaTask					= 1;
const long wmaText					= 2;
const long wmaFrontVisible			= 4;
const long wmaFull					= 8;

const int DOS_MAX					= 256;		// [256]


//////////////
// Types	//
//////////////
struct t_ThreadMsgBoxData
{
	HWND hwnd;
	string* pstrText;
	string* pstrTitle;
	long lngMsgBoxParams;
};


//////////////////////
// Global Cache		//
//////////////////////
ThreadManager TVM;
bool is_64_bit_os				= RunningUnder64BitOS();


//////////////////////
// Internal Cache	//
//////////////////////
DWORD TVM_LastThreadDown = 0;
CriticalSection THREAD_REMOVE_LOCK;


//////////////////////////
// OS Version Related	//
//////////////////////////
bool RunningUnder64BitOS()
{
	BOOL ret = FALSE;
	IsWow64Process (GetCurrentProcess(), &ret);
	return (ret == TRUE);
}

bool EnablePrivilege (string PrivilegeName, BOOL bEnable)
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tp;
	LUID luid;
	BOOL res;
	
	if (!OpenProcessToken(GetCurrentProcess(), 
							TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_READ, &hToken))
	{
		return false;
	}
	
	if (!LookupPrivilegeValue(NULL, &PrivilegeName[0], &luid))
	{
		return true;
	}
	
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = (bEnable) ? SE_PRIVILEGE_ENABLED : 0;
	res = AdjustTokenPrivileges(hToken, FALSE, &tp, 0, 0, 0);
	
	CloseHandle(hToken);
	return (res == 1);
}

TOKEN_ELEVATION_TYPE GetCurrentUsersElevation ()
{
	static bool check_already_done = false;
	static TOKEN_ELEVATION_TYPE elevationType;

	// Return values: { TokenElevationTypeDefault, TokenElevationTypeLimited, TokenElevationTypeFull}
	// Check if we already did that Verification
	if (check_already_done)
	{
		return elevationType; 
	}

	// Windows Vista/7.
	HANDLE hToken;
	DWORD dwSize;

	// Open current process.
	OpenProcessToken (GetCurrentProcess(), TOKEN_QUERY, &hToken);
	GetTokenInformation (hToken, TokenElevationType, &elevationType, sizeof(elevationType), &dwSize);

	// Close process token.
	if (hToken) 
	{ 
		CloseHandle(hToken); 
	}

	// This routine does not recognize domain administrators properly.
	if (IsUserAnAdmin() == TRUE)
	{
		elevationType = TokenElevationTypeFull;
	}

	// Return Elevation Type
	check_already_done = true;
	return elevationType;
}

bool IsRemoteDesktopSession ()
{
	return (GetSystemMetrics(SM_REMOTESESSION) != 0);
}

string GetCurrentUserSID ()
{
	HANDLE hToken;
	DWORD dwBufferSize;
	LPSTR sidbuf;
	string buffer, ret;

	// Reset return string
	ret = "";

	// Open the access token associated with the calling process
	hToken = 0;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken) == TRUE) 
	{
		// Get the size of the memory buffer needed for the SID
		dwBufferSize = 0;
		GetTokenInformation(hToken, TokenUser, NULL, 0, &dwBufferSize);
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			// Allocate buffer for user token data
			buffer.resize (dwBufferSize);

			// Retrieve the token information in a TOKEN_USER structure
			if (GetTokenInformation(hToken, TokenUser, (PTOKEN_USER) &buffer[0], dwBufferSize, &dwBufferSize) == TRUE) 
			{
				// Check if SID is valid
				if (IsValidSid(((PTOKEN_USER) &buffer[0])->User.Sid) == TRUE) 
				{
					if (ConvertSidToStringSid(((PTOKEN_USER) &buffer[0])->User.Sid, &sidbuf) == TRUE)
					{
						// Copy generated SID String to our return string
						ret = sidbuf;

						// Free Memory that has been temporarily allocated for the converted SID String
						LocalFree (sidbuf);
					}

				}
			}
		}

		// Close Process Handle
		CloseHandle (hToken);
	}

	// Return ret string
	return ret;
}


//////////////////////////////
// Thread/Process Create	//
//////////////////////////////
DWORD NewProcess (string exe, string cmdline)
{
	STARTUPINFO sti;
	PROCESS_INFORMATION pif;
	sti.lpReserved = 0;
	sti.lpDesktop = 0;
	sti.lpTitle = 0;
	sti.dwFlags = 0;
	sti.cbReserved2 = 0;
	sti.lpReserved2 = 0;
	sti.cb = sizeof(STARTUPINFO);

	// Create the Process
	if (CreateProcess (&exe[0], &CSR(" ", cmdline)[0], 
			0, 0, false, CREATE_NEW_PROCESS_GROUP | NORMAL_PRIORITY_CLASS, 0, 0, &sti, &pif))
	{
		return pif.dwProcessId;
	}
	return 0;
}

DWORD NewProcess16 (string exe, string cmdline)
{
	STARTUPINFO sti;
	PROCESS_INFORMATION pif;
	sti.lpReserved = 0;
	sti.lpDesktop = 0;
	sti.lpTitle = 0;
	sti.dwFlags = 0;
	sti.cbReserved2 = 0;
	sti.lpReserved2 = 0;
	sti.cb = sizeof(STARTUPINFO);
	if (CreateProcess (NULL, &CSR(exe, " ", cmdline)[0], 
			0, 0, false, CREATE_NEW_PROCESS_GROUP | NORMAL_PRIORITY_CLASS, 0, 0, &sti, &pif))
	{
		return pif.dwProcessId;
	}
	return 0;
}

HANDLE unsecNewThread (void* func, void* param, DWORD* thrid)
{
	return CreateThread (0, 0, (LPTHREAD_START_ROUTINE) func, param, 0, thrid);
}


//////////////////////////////
// Process Management		//
//////////////////////////////
DWORD TaskWaitKernel (DWORD procid, long ms)
{
	HANDLE hProcess = OpenProcess (SYNCHRONIZE, false, procid);
	DWORD dwExitCode = 0;

	if (hProcess != 0)
	{
		WaitForSingleObject (hProcess, ms);
		GetExitCodeProcess (hProcess, &dwExitCode);
		CloseHandle (hProcess);
	}

	// Return Process ExitCode.
	return dwExitCode;
}

bool KillProcess (DWORD procid, bool force)
{
	HANDLE handle;

	// Acquire Priviledge
	if (force) { EnablePrivilege (SE_DEBUG_NAME, true); };

	// Open the Process for purpose of terminating it
	handle = OpenProcess (PROCESS_TERMINATE, false, procid);

	// Release required Priviledge
	if (force) { EnablePrivilege (SE_DEBUG_NAME, false); };

	// Terminate the Process
	if (handle != 0)
	{
		TerminateProcess (handle, 1);
		CloseHandle (handle);
	}

	// Return if the Operation succeeded
	return (handle != 0);
}

bool TerminateExplorer ()
{
	HWND ewnd;
	DWORD procid;

	// Find Explorer.exe Window
	ewnd = ExplorerWindow();
	GetWindowThreadProcessId (ewnd, &procid);
	if ((ewnd != 0) && (procid != 0))
	{
		return KillProcess(procid);
	}
	else
	{
		return false;
	}
}

string GetCurrentUser ()
{
	static bool did_already_acquire = false;
	static string current_username;

	DWORD max_username_len = 255;
	long res;

	// Check if we already did the work
	if (did_already_acquire) { return current_username; };

	// Get Current Username
	current_username.resize (max_username_len);
	res = GetUserName (&current_username[0], &max_username_len);
	if (res != 0) { current_username.resize (max_username_len - 1); } else { current_username = ""; };
	
	// Return Current Username
	did_already_acquire = true;
	return current_username;
}

string GetEnvVariable (string varname)
{
	string buf;

	if (xlen(varname) != 0)
	{
		Cutquotes (&varname, '%');		// Cut the Variable Identifier
		buf.resize (DOS_MAX);
		long res = GetEnvironmentVariable (&varname[0], &buf[0], DOS_MAX);
		if (res <= DOS_MAX)				// Was buffer big enough?
		{
			buf.resize (res);
		}
		else
		{
			buf.resize (0);
		}
	}
	return buf;
}

void Cutquotes (string* obj, unsigned char quote)
{
	if ((*obj).length() > 0)
	{
		// Object length greater that zero
		if ((*obj)[0] == quote) { *obj = Right(*obj, (long) (*obj).length() - 1); };
		if ((*obj)[(long) (*obj).length()-1] == quote) { obj->resize ((long) (*obj).length() - 1); };
	}
}

string FormatMsg (DWORD errcode, long component)
{
	string buf;
	long res;
	
	// Read the Error Description from API
	buf.resize (256);
	res = FormatMessage (component, 0, errcode, 0, &buf[0], 256, 0);
	if ((res >= 1) && (res <= 256))
	{
		buf.resize (res);
		if (buf[xlen(buf)-1] == 10) { buf.resize(xlen(buf)-1); };
		if (buf[xlen(buf)-1] == 13) { buf.resize(xlen(buf)-1); };
		return buf;
	}
	else
	{
		return "Unresolved Error";
	}
}

HANDLE ThreadInfoBox (HWND hwnd, string strText, string strTitle, long lngMsgBoxParams)
{
	// Return Value: Handle of the newly created message box thread.
	// Variables.
	DWORD dwRetThreadID = 0;

	// Allocate structure with info for the new thread.
	t_ThreadMsgBoxData* tmbd = new t_ThreadMsgBoxData;
	tmbd->hwnd = hwnd;
	tmbd->pstrText = new string(strText);
	tmbd->pstrTitle = new string(strTitle);
	tmbd->lngMsgBoxParams = lngMsgBoxParams;

	// Create MsgBox Thread.
	return unsecNewThread(&ThreadInfoBox_Thread, (void*) tmbd, &dwRetThreadID);
}

long ThreadInfoBox_Thread (LONG_PTR param)
{
	t_ThreadMsgBoxData* tmbd = (t_ThreadMsgBoxData*) param;

	// Show Message Box UI.
	MessageBox (tmbd->hwnd, &(*tmbd->pstrText)[0], &(*tmbd->pstrTitle)[0], tmbd->lngMsgBoxParams);

	// Clear Memory;
	delete tmbd->pstrText;
	delete tmbd->pstrTitle;
	delete tmbd;
	
	// Terminate Thread.
	// TVM.RemoveThread();
	return -9000;
}


//////////////////////////////////////////////////////////////////////////////////////////////
// Class: ThreadManager START																//
//////////////////////////////////////////////////////////////////////////////////////////////
const long ThreadManager::THREAD_REMOVE_DELAY			= 750;

ThreadManager::ThreadManager ()
{
	MainThread = GetCurrentThreadId();		// Main Thread is creating THIS CLASS
	tmc_count = 0;
}

ThreadManager::~ThreadManager ()
{
	// Clear
	tmc.Redim (0);
}

HANDLE ThreadManager::NewThread (void* func, void* param, DWORD* ret_thrid)						// public
{
	HANDLE hThread;
	DWORD thrid;
	long u;
	
	// Enter Critical Section
	cs.Enter();
	
	// Start the new thread
	hThread = unsecNewThread (func, param, &thrid);		
	
	// Add new thread id to cache
	u = tmc.Ubound() + 1;
	tmc.Redim (u);
	tmc[u] = thrid;
	tmc_count = tmc.Ubound();
	
	// Leave Critical Section
	cs.Leave();
	
	// Save identifier if required by the optional parameter
	if (ret_thrid != 0)
	{
		*ret_thrid = thrid;
	}

	// Return HANDLE to the newly created Thread
	return hThread;
}

void ThreadManager::RemoveThread ()																// public
{
	DWORD current;
	long u;
	
	// SPECIAL CS: Acquire Global Thread Remove Lock
	THREAD_REMOVE_LOCK.Enter();
	
	// You cannot remove the Main Thread
	current = GetCurrentThreadId();
	if (current != MainThread)		
	{
		// Enter Critical Section
		cs.Enter();
		
		// Search for the current thread identifier
		u = tmc.Ubound();
		for (long i = 1; i <= u; i++)
		{
			if (current == tmc[i])
			{
				// This index represents the Current Thread
				tmc.Remove (i);
				tmc_count = tmc.Ubound();
				break;
			}
		}
		
		// Leave Critical Section
		cs.Leave();
	}
	
	// SPECIAL CS: Release Global Thread Remove Lock
	THREAD_REMOVE_LOCK.Leave();
}

void ThreadManager::AwaitShutdown ()															// public
{
	MSG msg;

	// Wait for all managed Threads to Shutdown
	while (tmc_count != 0)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE))
		{
			GetMessage (&msg, 0, 0, 0);
			TranslateMessage (&msg);
			DispatchMessage (&msg);
		}

		// Sleep some time so the managed Threads can use it to execute until they are finished
		Sleep (50);
	}
}

long ThreadManager::count ()																	// public
{
	// No CS, because: ATOMIC READ OPERATION
	return tmc_count;
}

DWORD ThreadManager::GetMainThread ()															// public
{
	// No CS, because: ATOMIC READ OPERATION
	return MainThread;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Class: ThreadManager END																	//
//////////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////////////////////
// Class: EnumProcs START																	//
//////////////////////////////////////////////////////////////////////////////////////////////
EnumProcs::EnumProcs ()
{
	// Init Class Cache.
	enum_done = false;

	// Init OS functions (only available in Windows 6.0 or higher)
	DYN_WINAPI_QueryFullProcessImageName = 0;
	DYN_WINAPI_QueryFullProcessImageName = (DYN_WINAPI_QUERYFULLPROCESSIMAGENAME) GetProcAddress (
										GetModuleHandle(TEXT("kernel32.dll")), 
										"QueryFullProcessImageNameA"
										);
}

EnumProcs::~EnumProcs ()
{
	// Clear RAM.
	Clear();

	// Release WinAPI Imports.
	DYN_WINAPI_QueryFullProcessImageName = 0;
}

void EnumProcs::Clear ()																				// public
{
	// Enter Critical Section
	cs.Enter();

	// Reset Cache
	ep.Redim (0);

	// Leave Critical Section
	cs.Leave();
}

void EnumProcs::Enum ()																					// public
{
	// Variables
	PROCESSENTRY32 proc;
	MODULEENTRY32 mod;
	HANDLE hprocsnap, hmodsnap;
	long u = 0;
	long procres, modres; 

	// 32/64-Bit Process Detail Enumeration
	HANDLE hTmpProcCheckHandle = 0;
	BOOL bIs_Wow64_Process = TRUE;
	string strProcFullFilename64;
	DWORD inout_dwResultLen = 0;

	// Enter Critical Section
	cs.Enter();
	
	// Initialize & Snap
	Clear();
	hprocsnap = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
	
	// Walk through PID List
	if (hprocsnap != 0)
	{
		proc.dwSize = sizeof(PROCESSENTRY32);
		procres = Process32First (hprocsnap, &proc);
		while (procres)
		{
			u++; ep.Redim (u);
			ep[u].procid = proc.th32ProcessID;
			ep[u].parentprocid = proc.th32ParentProcessID;

			// Windows XP, Vista, 7, 8, 10 and higher.
			// Assume we did not find any more information in the toolhelp snapshot.
			bool modsuccess = false;

			// Check if this process is running in 32-bit environment.
			bIs_Wow64_Process = FALSE;
			hTmpProcCheckHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, proc.th32ProcessID);
			if (hTmpProcCheckHandle != 0)
			{
				IsWow64Process (hTmpProcCheckHandle, &bIs_Wow64_Process);
				CloseHandle (hTmpProcCheckHandle);
			}
				

			//
			// We need to use different mechanisms to collect information on the process image we found running under "proc.th32ProcessID".
			//
			if (bIs_Wow64_Process == TRUE)
			{
				//
				// 32-Bit process found.
				//
				// For testing purposes only.
				// #ifdef _DEBUG
				// 	OutputDebugString (&CSR("EnumProcs::Enum: Found 32-Bit Process \"", proc.szExeFile, "\".\n")[0]);
				// #endif
				//

				//
				// Create toolhelp snapshot to get more information on this ProcID.
				//
				hmodsnap = CreateToolhelp32Snapshot (TH32CS_SNAPMODULE, proc.th32ProcessID);
				if (hmodsnap != INVALID_HANDLE_VALUE)
				{
					mod.dwSize = sizeof(MODULEENTRY32);
					modres = Module32First (hmodsnap, &mod);
					while (modres)
					{
						if (mod.th32ProcessID == proc.th32ProcessID)
						{
							ep[u].path = mod.szExePath;
							modsuccess = true;
							break;
						}
						modres = Module32Next (hmodsnap, &mod);
					}
					CloseHandle (hmodsnap);
				}
				else
				{
					// For testing purposes only.
					// #ifdef _DEBUG
					//	 OutputDebugString (&CSR("EnumProcs::Enum: CreateToolhelp32Snapshot failed for: ", proc.szExeFile, "\n")[0]);
					// #endif
				}


				//
				// END if (bIs_Wow64_Process == TRUE)
				//
			}
			else
			{
				//
				// 64-Bit process found.
				//
				// For testing purposes only.
				// #ifdef _DEBUG
				// 	OutputDebugString (&CSR("EnumProcs::Enum: Found 64-Bit Process \"", proc.szExeFile, "\".\n")[0]);
				// #endif

				//
				// Query full process image path and filename.
				// This only works in Windows 6.0 or higher.
				// 
				if (DYN_WINAPI_QueryFullProcessImageName != 0)
				{
					// PROCESS_QUERY_INFORMATION | PROCESS_VM_READ
					hTmpProcCheckHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, proc.th32ProcessID);
					if (hTmpProcCheckHandle != 0)
					{
						strProcFullFilename64.resize (MAX_PATH);
						inout_dwResultLen = (long) strProcFullFilename64.length();
						if (DYN_WINAPI_QueryFullProcessImageName(hTmpProcCheckHandle, 0, &strProcFullFilename64[0], &inout_dwResultLen) != 0)
						{
							// QueryFullProcessImageName succeeded.
							strProcFullFilename64.resize (inout_dwResultLen);
							// #ifdef _DEBUG
							// 	OutputDebugString (&CSR("EnumProcs::Enum: Process \"", proc.szExeFile, "\" image path is \"", strProcFullFilename64,"\".\n")[0]);
							// #endif
								
							// Return retrieved process fullfn and success.
							ep[u].path = strProcFullFilename64;
							modsuccess = true;

						}	
						CloseHandle (hTmpProcCheckHandle);
					}
				}
					
				// END if (bIs_Wow64_Process == TRUE)
			}


			//
			// Unsuccessful? - Go failsafe and return the EXE Image Filename for the EXE Image Path.
			//
			if (!modsuccess)
			{
				// #ifdef _DEBUG
				// 	OutputDebugString (&CSR("EnumProcs::Enum: !modsuccess for Process \"", proc.szExeFile, "\".\n")[0]);
				// #endif
				ep[u].path = proc.szExeFile;
			}
			
			ep[u].name = GetFilenameFromCall(CSR(Chr(34), ep[u].path, Chr(34)));
			procres = Process32Next (hprocsnap, &proc);
		}
		CloseHandle (hprocsnap);
	}
	
	// Indicate that at least one enumeration has been completed successfully within this class object
	enum_done = true;

	// Leave Critical Section
	cs.Leave();
}


// EnumProcs - Properties
long EnumProcs::Ubound ()																				// public
{
	long u;

	// Enter Critical Section
	cs.Enter();

	// Get Item Count
	u = ep.Ubound();

	// Leave Critical Section
	cs.Leave();

	// Return Item Count
	return u;
}

string EnumProcs::exename (long idx)																	// public
{
	string tmp_exename;

	// Enter Critical Section
	cs.Enter();

	// Get from Cache
	tmp_exename = ep[idx].name;

	// Leave Critical Section
	cs.Leave();

	// Return
	return tmp_exename;
}

string EnumProcs::exepath (long idx)																	// public
{
	string tmp_exepath;

	// Enter Critical Section
	cs.Enter();

	// Get from Cache
	tmp_exepath = ep[idx].path;

	// Leave Critical Section
	cs.Leave();

	// Return
	return tmp_exepath;
}

DWORD EnumProcs::procid	(long idx)																		// public
{
	DWORD tmp_procid;

	// Enter Critical Section
	cs.Enter();

	// Get from Cache
	tmp_procid = ep[idx].procid;

	// Leave Critical Section
	cs.Leave();

	// Return
	return tmp_procid;
}

DWORD EnumProcs::parentprocid (long idx)																// public
{
	DWORD tmp_parent_procid;

	// Enter Critical Section
	cs.Enter();

	// Get from Cache
	tmp_parent_procid = ep[idx].parentprocid;

	// Leave Critical Section
	cs.Leave();

	// Return
	return tmp_parent_procid;
}

bool EnumProcs::IsKernelProcess (DWORD check_procid)													// public
{
	long u, i;
	bool res = false;

	// Enter Critical Section
	cs.Enter();

	// Check if enumeration has been done
	if (!enum_done)
	{
		// Leave Critical Section before return
		cs.Leave();
		DebugOut ("Process->EnumProcs->IsKernelProcess: Missing enumeration before function call");
		return false;
	}
	
	// Search for the given process in the last enumeration
	u = ep.Ubound();
	for (i = 1; i <= u; i++)
	{
		if (ep[i].procid == check_procid)
		{
			// Given Process has been found
			// Windows XP/Vista/7
			res = (InStr(UCase("*[System Process]*System*"), UCase(ep[i].name)) != 0);
			break;
		}
	}

	// Leave Critical Section
	cs.Leave();

	// Return Result of Investigation of that process
	return res;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Class: EnumProcs END																		//
//////////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////////////////////
// Class: EnumApps START																	//
//////////////////////////////////////////////////////////////////////////////////////////////
EnumApps::EnumApps ()
{
}

EnumApps::~EnumApps ()
{
	Clear();
}

void EnumApps::Clear ()																						// public
{
	// Enter Critical Section
	cs.Enter();

	// Reset Cache
	ea.Redim (0);

	// Leave Critical Section
	cs.Leave();
}

void EnumApps::Enum (long EnumFlags)																		// public
{
	// Enter Critical Section
	cs.Enter();

	// Clear Cache first before starting to enumerate all processes
	Clear();

	// Prepare Callback Info Parameter
	wmtEnumInfo info;
	info.sar = &ea;
	info.flags = EnumFlags;
	EnumWindows (&EnumWindowsProc, (LPARAM) &info);

	// Leave Critical Section
	cs.Leave();
}

BOOL CALLBACK EnumApps::EnumWindowsProc (HWND hwnd, LPARAM tinfo)											// protected
{
	//
	// INFO: (!) UNCLEAR IF CS IS NEEDED HERE; CAREFUL!!!; ATTENTION!!!
	//
	string txt;
	DWORD procid;
	HWND owner;
	long flags, tlen;
	size_t exstyle;

	// Resolve 'info' first
	StructArray <WMTEA> *sar = ((wmtEnumInfo*) tinfo)->sar;

	// Enter Critical Section
	// sar->Enter();

	// Get Enum Flags
	flags = ((wmtEnumInfo*) tinfo)->flags;
	
	// Get Window Information
	tlen = GetWindowTextLength(hwnd);
	if (tlen != 0)		// Window Text existing?
	{
		tlen++; txt.resize (tlen);											// Prepare Buffer
		tlen = GetWindowText (hwnd, &txt[0], tlen); txt.resize (tlen);		// Get Window Text
	}
	
	owner = GetWindow (hwnd, GW_OWNER);					// Get the Owner of the Window
	exstyle = GetWindowLongPtr (hwnd, GWL_EXSTYLE);		// Get Extended Windowstyle
	
	if ((flags & wmaFull) || 
		((flags & wmaText) && (tlen != 0)) ||
		((flags & wmaFrontVisible) && (IsWindow(hwnd)) && (IsWindowVisible(hwnd)) &&
							  (((!(exstyle & WS_EX_TOOLWINDOW)) && (owner == 0)) || (owner != 0))
								) || 
		((flags & wmaTask) && (IsWindow(hwnd)) && (IsWindowVisible(hwnd)) &&
							  (GetAbsParent(hwnd) == hwnd) && (
									((!(exstyle & WS_EX_TOOLWINDOW)) && (owner == 0)) ||
									((exstyle & WS_EX_APPWINDOW) && (owner != 0)))
						 ))
	{
		GetWindowThreadProcessId (hwnd, &procid);			// Get Process belonging to Window
		long u = sar->Ubound() + 1;							// Add new entry to cache
		sar->Redim (u);
		(*sar)[u].hwnd = hwnd;
		(*sar)[u].procid = procid;
		(*sar)[u].name = txt;
	}

	// Leave Critical Section
	// sar->Leave();

	// Return "true" to the OPERATING SYSTEM, because we want to get more window information about other windows
	return true;
}

void EnumApps::Take (EnumApps* EA)																			// public
{
	long u;

	// Enter Critical Section
	cs.Enter();
	
	// Reset Cache first
	Clear();

	// Reserve enough space in the Local Cache to be ready for taking the data of the given foreign EnumApps Cache
	u = EA->Ubound();
	ea.Redim (u);
	
	// Copy all data from the foreign Cache to the local Cache
	for (long i = 1; i <= u; i++)
	{
		ea[i].hwnd = EA->hwnd(i);
		ea[i].procid = EA->procid(i);
		ea[i].name = EA->name(i);
	}

	// Leave Critical Section
	cs.Leave();
}

long EnumApps::Ubound ()																					// public
{
	long u;

	// Enter Critical Section
	cs.Enter();

	// Get Item Count
	u = ea.Ubound();

	// Leave Critical Section
	cs.Leave();

	// Return Item Count
	return u;
}

string EnumApps::name (long idx)																			// public
{
	string tmp_name;

	// Enter Critical Section
	cs.Enter();

	// Get from Cache
	tmp_name = ea[idx].name;

	// Leave Critical Section
	cs.Leave();

	// Return
	return tmp_name;
}

HWND EnumApps::hwnd	(long idx)																				// public
{
	HWND tmp_hwnd;

	// Enter Critical Section
	cs.Enter();

	// Get from Cache
	tmp_hwnd = ea[idx].hwnd;

	// Leave Critical Section
	cs.Leave();

	// Return
	return tmp_hwnd;
}

DWORD EnumApps::procid (long idx)																			// public
{
	DWORD tmp_procid;

	// Enter Critical Section
	cs.Enter();

	// Get from Cache
	tmp_procid = ea[idx].procid;

	// Leave Critical Section
	cs.Leave();

	// Return
	return tmp_procid;
}


//////////////////////////////////////////////////////////////////////////////////////////////
// Class: EnumApps END																		//
//////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////
// Class: AppDetect START																	//
//////////////////////////////////////////////////////////////////////////////////////////////
AppDetect::AppDetect ()	
{
}

AppDetect::~AppDetect ()
{
	Reset();
}

void AppDetect::Reset ()																				// public
{
	// Enter Critical Section
	cs.Enter();

	// Reset Cache
	E2.Clear ();

	// Leave Critical Section
	cs.Leave();
}

void AppDetect::Update (long EnumFlags)																	// public
{
	// Enter Critical Section
	cs.Enter();

	// Update
	E2.Take (&E1);
	E1.Enum (EnumFlags);

	// Leave Critical Section
	cs.Leave();
}

bool AppDetect::AppsChanged (EnumApps* EA)																// public
{
	long u, i, j;
	bool windowfound;
	bool changed = false;

	// Enter Critical Section
	cs.Enter();

	// Check if the calling thread contains a copy of the EnumApps-Cache
	if (EA != 0)
	{
		// Copy Appcache to the calling function
		EA->Take (&E1);
	}

	if (E1.Ubound() != E2.Ubound())
	{
		// Leave Critical Section before return
		cs.Leave();
		return true;
	}
	
	// Both have same Ubound -> compare entries
	u = E1.Ubound();
	for (i = 1; i <= u; i++)
	{
		windowfound = false;
		for (j = 1; j <= u; j++)
		{
			if (E1.hwnd(i) == E2.hwnd(j))
			{
				windowfound = true;
				if (E1.name(i) != E2.name(j))
				{
					changed = true;
					break;
				}
			}
		}
		if ((!windowfound) || (changed))
		{
			changed = true;
			break;
		}
	}

	// Leave Critical Section
	cs.Leave();

	// Return if there were changes in the AppDetector-Cache
	return changed;
}

bool AppDetect::AppStillExists (HWND hwnd, DWORD procid, string* title)									// public
{
	bool success = false;
	long u, i;

	// Enter Critical Section
	cs.Enter();

	// Look for the specified Application by HWND and PROCID, Return Title if required so
	u = E1.Ubound();
	for (i = 1; i <= u; i++)
	{
		if ((E1.hwnd(i) == hwnd) && (E1.procid(i) == procid))
		{
			if (title != 0)
			{
				(*title) = E1.name(i);
			}
			success = true;
			break;
		}
	}

	// Leave Critical Section
	cs.Leave();

	// Return if the Application still exists
	return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Class: AppDetect END																		//
//////////////////////////////////////////////////////////////////////////////////////////////





//////////////////////////////////////////////////////////////////////////////////////////////
// Class: Impersonate User START															//
//////////////////////////////////////////////////////////////////////////////////////////////
ImpersUser::ImpersUser()
{
	foreign_user_token = 0;
}

ImpersUser::~ImpersUser()
{
	// Log off if necessary
	Logoff();
}

bool ImpersUser::Logon(string username, string password, string domain)									// public
{
	bool fail_success = false;
	bool api_res;

	// Enter Critical Section
	cs.Enter();

	// Check if we have alreay done an impersonation within this class, if necessary Log off first
	Logoff();

	// Attempt to log on
	// LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT (without domain given)
	// LOGON32_LOGON_NEW_CREDENTIALS, LOGON32_PROVIDER_WINNT50 (with domain given)
	api_res = (LogonUser(&username[0], (xlen(domain) > 0) ? &domain[0] : NULL, &password[0], 
							LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &foreign_user_token) == TRUE);

	// Impersonate if Logon was successful
	if (api_res)
	{
		fail_success = (ImpersonateLoggedOnUser(foreign_user_token) == TRUE);
	}

	// Logoff again if the operation was unsuccessful
	if (!fail_success) { Logoff(); };

	// Leave Critical Section
	cs.Leave();

	// Return if the operation succeeded
	return fail_success;
}

bool ImpersUser::SetThreadToConsoleUsersContext()
{
	DWORD dwSessionId, dwExplorerSessId, dwExplorerLogonPid;
	HANDLE hProcess, hPToken, hSnap;
	PROCESSENTRY32 procEntry;
	bool fail_success = false;

	// Enter Critical Section
	cs.Enter();

	// Get the Active Desktop's Session ID
	dwSessionId = WTSGetActiveConsoleSessionId();

	// We seach for the current user's explorer process because it has the current user's token
	hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap != INVALID_HANDLE_VALUE)
	{ 
		procEntry.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(hSnap, &procEntry))
		{
			dwExplorerLogonPid = 0;
			do
			{
				if (_stricmp(procEntry.szExeFile, "explorer.exe") == 0)
				{
					if (ProcessIdToSessionId(procEntry.th32ProcessID, &dwExplorerSessId))
					{
						// Check if the currently found "Explorer.exe" belongs to the Console User Session
						if (dwExplorerSessId == dwSessionId)
						{
							dwExplorerLogonPid = procEntry.th32ProcessID;
							break;
						}
					}
				}
			} while (Process32Next(hSnap, &procEntry));

			// Check if we found the "Explorer.exe" process
			if (dwExplorerLogonPid != 0)
			{
				// Open Explorer Process
				hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, dwExplorerLogonPid);
				if (hProcess == 0)
				{
					DebugOut ("ImpersUser::SetThreadToConsoleUsersContext->Could not open process.");
				}
				else
				{
					if (!OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_DUPLICATE | 
												TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_SESSIONID | 
												TOKEN_READ | TOKEN_WRITE, &hPToken))
					{
						#ifdef _DEBUG
							DebugOut ("ImpersUser::SetThreadToConsoleUsersContext->Could not open process token.");
						#endif
					}
					else
					{
						// Impersonate Token
						ImpersonateLoggedOnUser (hPToken);
						if (GetLastError() == ERROR_SUCCESS)
						{
							// Store the impersonated user token in Cache
							foreign_user_token = hPToken;

							// Success
							fail_success = true;
						}
						else
						{
							#ifdef _DEBUG
								DebugOut ("ImpersUser::SetThreadToConsoleUsersContext->Could not impersonate active console session.");
							#endif
						}
					}
				}
			}
			else
			{
				#ifdef _DEBUG
					DebugOut ("ImpersUser::SetThreadToConsoleUsersContext->Could not find current console users's Explorer Process.");
				#endif
			}
		}
	}

	// Check if we had success, if not, Logoff
	if (!fail_success) { Logoff(); };

	// Leave Critical Section
	cs.Leave();

	// Return if the operation succeeded
	return fail_success;
}

void ImpersUser::Logoff()																				// public
{
	// Enter Critical Section
	cs.Enter();

	// Close Foreign User Token because it is no longer needed
	if (foreign_user_token != 0)
	{
		// Windows API Call
		RevertToSelf();

		// Close foreign token handle
		CloseHandle (foreign_user_token);
		foreign_user_token = 0;
	}

	// Leave Critical Section
	cs.Leave();
}

HANDLE ImpersUser::GetToken ()
{
	return foreign_user_token;
}
//////////////////////////////////////////////////////////////////////////////////////////////
// Class: Impersonate User END															//
//////////////////////////////////////////////////////////////////////////////////////////////


