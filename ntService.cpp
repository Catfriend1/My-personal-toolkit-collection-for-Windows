// ntService.cpp: WinSuite's Windows NT Service Handler
//
// Thread-Safe: NO
//


//////////////////
// Includes		//
//////////////////
#include "stdafx.h"

#include "Modules/_modules.h"

// Import REGV_SYSLOG_LOG_ENABLE, REGV_SYSLOG_LOG_ALL, REGV_NETMSG_SERVER_PASSWORD
#include "Boot.h"															

#include "Winsuite_Server.h"

#include "NetMsg/NetMsg_Server.h"

// Import SYSLOG_MODE_LOG_ENABLE, SYSLOG_MODE_LOG_ALL
#include "Syslog/Syslog_Shared.h"											
#include "Syslog/Syslog_Server.h"

#include "ntService.h"


//////////////
// Consts	//
//////////////
const bool NTSERVICE_REACT_ON_PAUSECONTINUE					= false;
const bool NTSERVICE_REACT_ON_PRESHUTDOWN					= true;

const DWORD NTSERVICE_DEFAULT_TIMEOUT						= 20000;

const long NTSERVICE_WINSUITE_THREAD_RES_OK					= -105;
const long NTSERVICE_WINSUITE_THREAD_RES_FAIL				= -106;

const long NTSERVICE_NETMSG_THREAD_RES_OK					= -115;
const long NTSERVICE_NETMSG_THREAD_RES_FAIL					= -116;

const long NTSERVICE_SYSLOG_THREAD_RES_OK					= -107;
const long NTSERVICE_SYSLOG_THREAD_RES_FAIL					= -108;


//////////////
// Cache	//
//////////////
SERVICE_STATUS ntsvc_status;
SERVICE_STATUS_HANDLE ntsvc_ctrl;
string ntsvc_name;


//////////////////////////
// Cache: Service App	//
//////////////////////////
ThreadManager TVM_ServiceApp;
Winsuite_Server* mod_winsuite_server;
NetMsg_Server* mod_netmsg_server;
Syslog_Server* mod_syslog_server;


//////////////////////////////
// Thread ID Cache			//
//////////////////////////////
DWORD MODID_NTSVC_MAIN_THREAD = 0;
DWORD MODID_WINSUITE_SERVER = 0;
DWORD MODID_NETMSG_SERVER = 0;
DWORD MODID_SYSLOG_SERVER = 0;


//////////////////////////////////
// Functions: Service App		//
//////////////////////////////////
bool ServiceApp_Init ()
{
	//
	// Note: This function is called by the "ntsvc_ServiceMain"-Thread in order to do initialization.
	//		 Returns FALSE in order to signal an abnormal "ExitProcess()" should be issued.
	//

	// Init RAM.
	MODID_NTSVC_MAIN_THREAD = GetCurrentThreadId();

	// Init RAM.
	mod_winsuite_server = 0;
	mod_netmsg_server = 0;
	mod_syslog_server = 0;

	// Initialize Winsuite_Server.
	mod_winsuite_server = new Winsuite_Server();

	// Initialize NetMsg Server.
	if (REGV_NETMSG_SERVER == 1)
	{
		mod_netmsg_server = new NetMsg_Server();
		if (!mod_netmsg_server->Init(REGV_NETMSG_SERVER_PASSWORD))
		{
			// NetMsg initialization failed.
			REGV_NETMSG_SERVER = 0;
			delete mod_netmsg_server;
			mod_netmsg_server = 0;
		}
	}

	// Initialize Syslog Server.
	if (REGV_SYSLOG_ENABLE == 1)
	{
		mod_syslog_server = new Syslog_Server();
		if (!mod_syslog_server->Init(
					((REGV_SYSLOG_LOG_ENABLE == 1)	? SYSLOG_MODE_LOG_ENABLE : 0) |
					((REGV_SYSLOG_LOG_ALL == 1)		? SYSLOG_MODE_LOG_ALL : 0)
						))
		{
			// Syslog initialization failed.
			REGV_SYSLOG_ENABLE = 0;
			delete mod_syslog_server;
			mod_syslog_server = 0;
		}
	}
	
	// Return Success.
	return true;
}

void WINAPI ntsvc_ServiceMain (DWORD argc, LPTSTR* argv)
{
	//
	// Note: The function "ntsvc_ServiceMain" is externally called by Windows SCM
	//		 after setting state to "SERVICE_START_PENDING".
	//

	// Variables.
	bool bDebugMode = false;
	MSG msg;

	// Init RAM.
	// Note: "ntsvc_status" already has been fully initialized here.
	//		 This has been done by "ntsvc_InitFromWinMain".
	if (argc == -1)
	{
		bDebugMode = true;
		argc = 0;
	}


	//
	// 1. Register a new Service Control Handler during phase "SERVICE_START_PENDING".
	//
	if (!bDebugMode)
	{
		ntsvc_ctrl = RegisterServiceCtrlHandlerEx (&ntsvc_name[0], &ntsvc_ServiceControlEx, NULL);
		if (ntsvc_ctrl == 0)
		{ 
			// (!) FAILED to set our own SERVICE CONTROL HANDLER FUNCTION.
			// Terminate our application immediately.
			ExitProcess (1000);
			return; 
		}
	}


	//
	// 2. Perfrom ServiceApp Initialization.
	//
	if (!ServiceApp_Init())
	{
		// (!) INIT FAILED.
		// 2012-07-22: SetServiceStatus leads to a recursion crash in the SCM
		//				if "SERVICE_STOPPED" is issued in this phase.
		// 
		// 
		// ntsvc_status.dwWin32ExitCode = 1001;
		// ntsvc_status.dwCurrentState = SERVICE_STOPPED;
		// SetServiceStatus (ntsvc_ctrl, &ntsvc_status);

		// Terminate our application immediately.
		ExitProcess (1001);
		return;
	}


	//
	// (!) INIT SUCCEEDED.
	//
	// 3. Indicate State "SERVICE_RUNNING" and 
	//	  reset potentially set "WaitHint" and "CheckPoint" counters.
	//
	ntsvc_status.dwCheckPoint = 0;
	ntsvc_status.dwWaitHint = 0;
	ntsvc_status.dwCurrentState = SERVICE_RUNNING;
	if (!SetServiceStatus(ntsvc_ctrl, &ntsvc_status) && !bDebugMode) 
	{
		// FAILED to set "SERVICE_RUNNING" state.
		// Terminate our application immediately.
		ExitProcess (1002);
		return; 
	}


	//
	// Main Message Queue of Win32 Service Process.
	//

	// Create Winsuite Server Thread
	TVM_ServiceApp.NewThread (&ServiceApp_Winsuite_Thread_Server, 0);

	// Create NetMsg Server Thread
	if (REGV_NETMSG_SERVER == 1)
	{
		TVM_ServiceApp.NewThread (&ServiceApp_NetMsg_Thread_Server, 0);
	}

	// Create Syslog Server Thread
	if (REGV_SYSLOG_ENABLE == 1)
	{
		TVM_ServiceApp.NewThread (&ServiceApp_Syslog_Thread_Server, 0);
	}

	// ServiceApp Main Message Queue
	while (GetMessage(&msg, 0, 0, 0))
	{
		// Wait for WM_QUIT signal from "ServiceApp_Terminate".
	}

	//
	// Note: "ntsvc_ServiceMain" received a "WM_QUIT" signal from "ntsvc_ServiceControl".
	//

	//
	// Send all Service Worker Threads a WM_QUIT signal.
	//

	// Shutdown Winsuite_Server if necessary.
	if (MODID_WINSUITE_SERVER != 0) 
	{ 
		PostThreadMessage (MODID_WINSUITE_SERVER, WM_QUIT, 0, 0); 
	}

	// Shutdown NetMsg_Server if necessary.
	if (MODID_NETMSG_SERVER != 0) 
	{
		PostThreadMessage (MODID_NETMSG_SERVER, WM_QUIT, 0, 0); 
	}

	// Shutdown Syslog_Server if necessary.
	if (MODID_SYSLOG_SERVER != 0) 
	{
		PostThreadMessage (MODID_SYSLOG_SERVER, WM_QUIT, 0, 0); 
	}

	// Wait for the Module-Threading-System to shutdown.
	TVM_ServiceApp.AwaitShutdown();

	// Unload Winsuite_Server.
	if (mod_winsuite_server != 0)
	{
		delete mod_winsuite_server;
		mod_winsuite_server = 0;
	}

	// Unload NetMsg_Server.
	if (mod_netmsg_server != 0)
	{
		delete mod_netmsg_server;
		mod_netmsg_server = 0;
	}

	// Unload Syslog_Server.
	if (mod_syslog_server != 0)
	{
		delete mod_syslog_server;
		mod_syslog_server = 0;
	}

	// Indicate State "SERVICE_STOPPED" and return an exitcode.
	ntsvc_status.dwCheckPoint = 0;
	ntsvc_status.dwWaitHint = 0;
	ntsvc_status.dwWin32ExitCode = 0;
	ntsvc_status.dwCurrentState = SERVICE_STOPPED;
	SetServiceStatus (ntsvc_ctrl, &ntsvc_status);

	// Return from "ntSvcMain"-Thread after signalling "SERVICE_STOPPED".
	return;
}


//////////////////////////////////////
// Functions: ServiceApp Threads	//
//////////////////////////////////////
long ServiceApp_Winsuite_Thread_Server (long param)
{
	bool fail_success;
	
	// Assign global thread ID.
	MODID_WINSUITE_SERVER = GetCurrentThreadId();

	// Start main worker.
	fail_success = mod_winsuite_server->Main();

	// Remove Thread
	TVM_ServiceApp.RemoveThread();
	return (fail_success) ? NTSERVICE_WINSUITE_THREAD_RES_OK : NTSERVICE_WINSUITE_THREAD_RES_FAIL;
}

long ServiceApp_NetMsg_Thread_Server (long param)
{
	bool fail_success;
	
	// Assign global thread ID.
	MODID_NETMSG_SERVER = GetCurrentThreadId();

	// Start main worker.
	fail_success = mod_netmsg_server->Main();

	// Remove Thread
	TVM_ServiceApp.RemoveThread();
	return (fail_success) ? NTSERVICE_NETMSG_THREAD_RES_OK : NTSERVICE_NETMSG_THREAD_RES_FAIL;
}

long ServiceApp_Syslog_Thread_Server (long param)
{
	bool fail_success;
	
	// Assign global thread ID.
	MODID_SYSLOG_SERVER = GetCurrentThreadId();

	// Start main worker.
	fail_success = mod_syslog_server->Main();

	// Remove Thread
	TVM_ServiceApp.RemoveThread();
	return (fail_success) ? NTSERVICE_SYSLOG_THREAD_RES_OK : NTSERVICE_SYSLOG_THREAD_RES_FAIL;
}


//////////////////////////////////////////////////
// Functions: NT-Service Install / Uninstall	//
//////////////////////////////////////////////////
bool ntsvc_Install (string svcname, string file)
{
	// Open the SCM
	SC_HANDLE myscm = OpenSCManager (NULL, NULL, SC_MANAGER_CREATE_SERVICE); 
	if (myscm == 0) { return false; };
	
	// Create the new service
	SC_HANDLE newsvc = CreateService (myscm, &svcname[0], &svcname[0], 
										SERVICE_ALL_ACCESS, 
										SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS, 
										SERVICE_AUTO_START, 
										SERVICE_ERROR_NORMAL, 
										&file[0], NULL, NULL, NULL, NULL, NULL);
	if (newsvc == 0) { return false; };
	
	// (!) Success
	CloseServiceHandle (newsvc); 
	CloseServiceHandle (myscm);
	return true;
}

bool ntsvc_Uninstall (string svcname)
{
	// Open the SCM
	SC_HANDLE myscm = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS); 
	if (myscm == 0) { return false; };
	
	// Open the specified service
	SC_HANDLE thesvc = OpenService (myscm, &svcname[0], SERVICE_ALL_ACCESS);
	if (thesvc == 0) { return false; };
	
	// Delete the specified service
	if (!DeleteService(thesvc))
	{
		CloseServiceHandle (thesvc);
		CloseServiceHandle (myscm);
		return false;
	}
	
	// (!) Success
	CloseServiceHandle (thesvc);
	CloseServiceHandle (myscm);
	return true;
}


//////////////////////////////////////////////
// Functions: NT-Service Public Interface	//
//////////////////////////////////////////////
bool ntsvc_InitFromWinMain (string svcname)
{
	//
	// Note: This function is called by "WinMain" in order to prepare start of the "ntsvc_ServiceMain"-Thread.
	//

	// Variables.
	bool fail_success = false;

	// Init RAM.
	ntsvc_ctrl = 0;
	ntsvc_name = svcname;
	
	// Set Service Information.
	// Initialize "ntsvc_status" structure.
	ntsvc_status.dwServiceType = SERVICE_WIN32;
	ntsvc_status.dwCurrentState = SERVICE_START_PENDING;
	// COMPATIBILITY: Windows Vista or higher.
	ntsvc_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | 
										SERVICE_ACCEPT_SHUTDOWN | 
										(NTSERVICE_REACT_ON_PRESHUTDOWN * SERVICE_ACCEPT_PRESHUTDOWN) | 
										(SERVICE_ACCEPT_PAUSE_CONTINUE * NTSERVICE_REACT_ON_PAUSECONTINUE);
	
	ntsvc_status.dwWin32ExitCode = 0;
	ntsvc_status.dwServiceSpecificExitCode = 0;
	ntsvc_status.dwCheckPoint = 0;
	ntsvc_status.dwWaitHint = 0;

	// Build the Service Startup Table
	SERVICE_TABLE_ENTRY disptable[2];
	disptable[0].lpServiceName = &ntsvc_name[0];
	disptable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION) &ntsvc_ServiceMain;
	disptable[1].lpServiceName = 0;
	disptable[1].lpServiceProc = 0;
	
	// Start the "ntsvc_ServiceMain" function as a new thread.
	fail_success = (StartServiceCtrlDispatcher(&disptable[0]) != 0);

	// Return if the operation succeeded.
	return fail_success;	
}


//////////////////////////////////////////
// Functions: Service Control Handler	//
//////////////////////////////////////////
DWORD WINAPI ntsvc_ServiceControlEx (DWORD control, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
	//
	// Note: This function is externally called by Windows SCM.
	//		 "ntsvc_ServiceControlEx" supersedes "ntsvc_ServiceControl" and supports "SERVICE_CONTROL_PRESHUTDOWN".
	//

	// Act depending on new service state and register back consequences to the SCM.
	if ((control == SERVICE_CONTROL_STOP) || 
		(control == SERVICE_CONTROL_SHUTDOWN) || 
		((control == SERVICE_CONTROL_PRESHUTDOWN) && NTSERVICE_REACT_ON_PRESHUTDOWN)
		)
	{
		// Indicate State "SERVICE_STOP_PENDING".
		ntsvc_status.dwCheckPoint = 0;
		ntsvc_status.dwWaitHint = 0;
		ntsvc_status.dwCurrentState = SERVICE_STOP_PENDING;
		SetServiceStatus (ntsvc_ctrl, &ntsvc_status);

		// Set timeout counters to zero.
		ntsvc_SendProgressToSCM (NTSERVICE_DEFAULT_TIMEOUT, true);

		// Send "ntSvc_ServiceMain"-Thread a signal to shut down all worker threads.
		if (MODID_NTSVC_MAIN_THREAD != 0)
		{
			PostThreadMessage (MODID_NTSVC_MAIN_THREAD, WM_QUIT, 0, 0);
		}
		

		//
		// Note: This function has to be finished before "ntSvc_ServiceMain"-Thread completes.
		//		 Else, a 30 second timeout will occur in Windows SCM.
		//		 "ntSvc_ServiceMain"-Thread has to signal state "SERVICE_STOPPED" when it finished uninitialization.
		//
	}
	else if ((control == SERVICE_CONTROL_PAUSE) && NTSERVICE_REACT_ON_PAUSECONTINUE)
	{
		// Do some stuff.
		//

		// Indicate State "SERVICE_PAUSED".
		ntsvc_status.dwCurrentState = SERVICE_PAUSED;
		SetServiceStatus (ntsvc_ctrl, &ntsvc_status);
	}
	else if ((control == SERVICE_CONTROL_CONTINUE) && NTSERVICE_REACT_ON_PAUSECONTINUE)
	{
		// Do some stuff.
		//

		// Indicate State "SERVICE_RUNNING".
		ntsvc_status.dwCurrentState = SERVICE_RUNNING;
		SetServiceStatus (ntsvc_ctrl, &ntsvc_status);
	}

	// Return Success to the Windows SCM.
	return NO_ERROR;
}


//////////////////////////////////////////////////////////////////
// Functions: Thread Interface for telling shutdown progress	//
//////////////////////////////////////////////////////////////////
void WINAPI ntsvc_SendProgressToSCM (DWORD dwMaxTimeMSForNextStep, bool bResetCheckpointCounter)
{
	//
	// Note: This function is externally called by thread specific shutdown procedures.
	//		 It is meant to notify the SCM of current shutdown progress, so that it does not think our service may be hung.
	//		 On every call, this function increments the "dwCheckpoint"-counter if "bResetCheckpointCounter" is set to "false".
	//		 The first call to this function SHOULD CONTAIN A RESET.
	//

	// Static Variables.
	static SERVICE_STATUS service_status;

	// Variables.
	DWORD dwOldCheckpointCounter;

	// Backup or Reset checkpoint counter.
	dwOldCheckpointCounter = (bResetCheckpointCounter) ? 0 : service_status.dwCheckPoint;

	// Copy current "ntsvc_status".
	service_status = ntsvc_status;

	// Restore previous checkpoint counter and set the max wait time.
	service_status.dwCheckPoint = dwOldCheckpointCounter;
	service_status.dwWaitHint = dwMaxTimeMSForNextStep;

	// Increment the counter if no reset is issued to this function.
	if (!bResetCheckpointCounter)  { service_status.dwCheckPoint++; };

	// Now send the new "service_status" to Windows SCM.
	SetServiceStatus (ntsvc_ctrl, &service_status);

	// Return.
	return;
}

