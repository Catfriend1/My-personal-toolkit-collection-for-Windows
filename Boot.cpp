// Boot.cpp: WinSuite's Startup Module
//
// Thread-Safe: NO
//
//
// WinSuite Parameter Documentation:
// ---------------------------------
//
// /Service: Runs WinSuite as a NT Service and registers back to the SCM
// /Install: Installs the NT Service into the SCM
// /Uninstall: Uninstalls the NT Service from the SCM
// /Usermode: Start all usermode components according to the registry settings
// <no parameter>: Executes Internal WinSuite Setup Dialog
//
//



//////////////////
// Includes		//
//////////////////
#include "stdafx.h"
#include "resource.h"

#include "Modules/_modules.h"

#include "Autoruns/Autoruns.h"
#include "NTProtect/NTProtect.h"
#include "WindowPoser/WindowPoser.h"							// needed before "ProcView.h"
#include "ProcView/ProcView.h"									// needed for ProcView::IsDatePrintEnabled()
#include "SystrayPlayer/SystrayPlayer.h"						// needed for SystrayPlayer::IsShutdownAllowed()

#include "NetMsg/NetMsg_Server.h"
#include "NetMsg/NetMsg_Client.h"

#include "Syslog/Syslog_Shared.h"
#include "Syslog/Syslog_Client.h"

#include "NTService.h"

#include "Boot.h"


//////////////
// Consts	//
//////////////
const char WINSUITE_CAPTION[]				= "WinSuite";
const char WINSUITE_SVCNAME[]				= "WinSuite";
const char WNDCLASS_VK2[]					= "*WS_C32_MainWindow_VK2";

const char REG_WINSUITE[]					= "Software\\WinSuite";
const char REG_RUN[]						= "Software\\Microsoft\\Windows\\CurrentVersion\\Run";

const char REGR_AUTORUNS[]					= "Autoruns";
const char REGR_NTPROTECT[]					= "NTProtect";
const char REGR_PROCVIEW[]					= "ProcView";
const char REGR_SYSTRAYPLAYER[]				= "SystrayPlayer";

// Consts: Syslog
// Note: These registry settings will not be shown in setup and are turned off by default.
const char REGR_SYSLOG_ENABLE[]				= "ThreadManagerOn";
const char REGR_SYSLOG_LOG_ENABLE[]			= "MultiThreadOn";
const char REGR_SYSLOG_LOG_ALL[]			= "MultiThreadAll";

// Consts: NetMsg (Network Messenger)
// Note: These registry settings will note be shown in setup and are turned off by default.
const char REGR_NETMSG_SERVER[]				= "NetworkMessengerServer";
const char REGR_NETMSG_SERVER_PASSWORD[]	= "NetworkMessengerServerPassword";
const char REGR_NETMSG_SERVER_ADDRESS[]		= "NetworkMessengerServerAddress";
const char REGR_NETMSG_CLIENT[]				= "NetworkMessengerClient";


//////////////////////////
// Icons: Autoruns [3]	//
//////////////////////////
HICON L_IDI_REGTOOL, L_IDI_REGTOOL16, L_IDI_RESTART32;


//////////////////////////////
// Icons: NTProtect [1]		//
//////////////////////////////
HICON L_IDI_APPS;


//////////////////////////////
// Fonts: NTProtect [1]		//
//////////////////////////////
HFONT FONT_MSSANS_BOLD;


//////////////
// Globals	//
//////////////
HINSTANCE myinst;
HICON L_IDI_WINSUITE;
HWND vk2wnd;
bool terminated = false;

ARLauncher* mod_arlauncher;						// This function gets always executed on Usermode startup
Autoruns* mod_autoruns;
NTProtect* mod_ntprotect;
ProcView* mod_procview;
SystrayPlayer* mod_systrayplayer;
Syslog_Client* mod_syslog_client;
NetMsg_Client* mod_netmsg_client;

// Globals: Syslog
long REGV_SYSLOG_ENABLE = 0;
long REGV_SYSLOG_LOG_ENABLE = 0;
long REGV_SYSLOG_LOG_ALL = 0;

// Globals: NetMsg (Network Messenger)
long REGV_NETMSG_SERVER = 0;
string REGV_NETMSG_SERVER_PASSWORD = "";
string REGV_NETMSG_SERVER_ADDRESS = "";
long REGV_NETMSG_CLIENT = 0;


//////////////////////////////////
// Functions: Init / Terminate	//
//////////////////////////////////
int APIENTRY WinMain (HINSTANCE hinst, HINSTANCE hprevinst, LPSTR cmdline, int cmdshow)
{
	// Variables.
	HANDLE mymutex;
	RegObj reg_service (HKLM, REG_WINSUITE);

	// Init RAM.
	mymutex = 0;
	mod_autoruns = 0;
	mod_ntprotect = 0;
	mod_procview = 0;
	mod_systrayplayer = 0;
	mod_netmsg_client = 0;
	mod_syslog_client = 0;

	// Cache Instance.
	myinst = hinst;

	// Init Common Service Registry Settings
	#ifndef _DEBUG
	reg_service.Init (REGR_SYSLOG_ENABLE, 0);
	reg_service.Init (REGR_SYSLOG_LOG_ENABLE, 0);
	reg_service.Init (REGR_SYSLOG_LOG_ALL, 0);
	reg_service.Init (REGR_NETMSG_SERVER, 0);
	reg_service.Init (REGR_NETMSG_SERVER_PASSWORD, "");
	reg_service.Init (REGR_NETMSG_SERVER_ADDRESS, "");
	reg_service.Init (REGR_NETMSG_CLIENT, 0);
	#endif

	reg_service.Get (&REGV_SYSLOG_ENABLE, REGR_SYSLOG_ENABLE);
	reg_service.Get (&REGV_SYSLOG_LOG_ENABLE, REGR_SYSLOG_LOG_ENABLE);
	reg_service.Get (&REGV_SYSLOG_LOG_ALL, REGR_SYSLOG_LOG_ALL);
	reg_service.Get (&REGV_NETMSG_SERVER, REGR_NETMSG_SERVER);
	reg_service.Get (&REGV_NETMSG_SERVER_PASSWORD, REGR_NETMSG_SERVER_PASSWORD);
	reg_service.Get (&REGV_NETMSG_SERVER_ADDRESS, REGR_NETMSG_SERVER_ADDRESS);
	reg_service.Get (&REGV_NETMSG_CLIENT, REGR_NETMSG_CLIENT);


	//
	// DEBUG VECTOR
	//
	//
	#ifdef _DEBUG
		/*
		StructArray <DISPLAY_DEVICE> paDD;
		StructArray <DEVMODE> paDM;

		GetActiveDisplayDevices(&paDD, &paDM);

		for (long i = 1; i <= paDD.Ubound(); i++)
		{
			DebugOut (paDD[i].DeviceName);
			DebugOut (paDM[i].dmPelsWidth);
			DebugOut (paDM[i].dmPelsHeight);
		}
		return TRUE;
		*/
	#endif
	//
	#ifdef _DEBUG
		if (InStr(UCase(cmdline), UCase("/DebugService")) != 0)
		{
			DWORD temp_thread_id;

			// Start WinSock.
			if (!WS2_Start())
			{
				// Abort Startup if unsuccessful
				InfoBox (0, "Error: Could not start WSA Interface, Application halted.", WINSUITE_CAPTION, MB_OK | MB_SYSTEMMODAL | MB_ICONSTOP);
				return -1;
			}

			// Start service in debug context.
			unsecNewThread ((void*) &DebugHelper_ntService_Thread, NULL, &temp_thread_id);

			// We'll now continue with another command line parameter.
		}
	#endif


	// ------------------------------------
	// Act as specified in the Command Line
	// ------------------------------------
	if (InStr(UCase(cmdline), UCase("/Install")) != 0)
	{
		// Install the Service (NTService and Server Part)
		// Windows XP/Vista/7
		if (ntsvc_Install(WINSUITE_SVCNAME, CSR(GetAppExe(), " /Service")))
		{
			MessageBox (0, LANG_EN ? "WinSuite Service has been successfully installed." : 
										"Der Dienst der WinSuite wurde erfolgreich installiert.", 
											WINSUITE_CAPTION, MB_OK | MB_ICONINFORMATION);
		}
		else
		{
			MessageBox (0, LANG_EN ? "Failed to install WinSuite Service." : 
							"Der Dienst der WinSuite konnte nicht installiert werden.", 
								WINSUITE_CAPTION, MB_OK | MB_ICONSTOP);
		}

		// Install the Service (Client Part)
		regWrite (CSR(LCase(GetAppExe()), " /Usermode"), HKLM, REG_RUN, WINSUITE_SVCNAME);
	}
	else if (InStr(UCase(cmdline), UCase("/Uninstall")) != 0)
	{
		// Windows XP/Vista/7
		if (ntsvc_Uninstall(WINSUITE_SVCNAME))
		{
			MessageBox (0, LANG_EN ? "WinSuite Service has been successfully uninstalled." : 
										"Der WinSuite Dienst wurde erfolgreich deinstalliert.", 
											WINSUITE_CAPTION, MB_OK | MB_ICONINFORMATION);
		}
		else
		{
			MessageBox (0, LANG_EN ? "Failed to uninstall WinSuite Service." : 
										"Fehler beim Deinstallieren des WinSuite Dienstes.", 
											WINSUITE_CAPTION, MB_OK | MB_ICONSTOP);
		}

		// Uninstall the Service (Client Part)
		regDeleteValue (HKLM, REG_RUN, WINSUITE_SVCNAME);
	}
	else if (InStr(UCase(cmdline), UCase("/Service")) != 0)
	{
		// Start WinSock.
		if (!WS2_Start())
		{
			// Abort Startup if unsuccessful
			InfoBox (0, "Error: Could not start WSA Interface, Application halted.", WINSUITE_CAPTION, MB_OK | MB_SYSTEMMODAL | MB_ICONSTOP);
			return -1;
		}

		// Initialize the NT Service.
		// Note: This function tells the SCM to start "ntSvcMain()-Thread" asynchronously with state RUNNING.
		ntsvc_InitFromWinMain (WINSUITE_SVCNAME);

		// We'll now exit our WinMain-Thread.
		// This is okay because Windows SCM has already started the "ntSvcMain()-Thread" asynchronously with state RUNNING.
	}
	else if (InStr(UCase(cmdline), UCase("/Usermode")) != 0)
	{
		// Start Application in USERMODE.
		// Local Variables.
		MSG msg;
		HACCEL mainacc;
		RegObj reg (HKCU, REG_WINSUITE);

		long REGV_AUTORUNS;
		long REGV_NTPROTECT;
		long REGV_PROCVIEW;
		long REGV_SYSTRAYPLAYER;

		// Init Windows Dlls
		InitCommonControls();

		// Init Common User Registry Settings
		reg.Init (REGR_AUTORUNS, 0);					// Changed 2012-12-22: "Autoruns" component is turned off by default.
		reg.Init (REGR_NTPROTECT, 1);
		reg.Init (REGR_PROCVIEW, 1);
		reg.Init (REGR_SYSTRAYPLAYER, 1);

		reg.Get (&REGV_AUTORUNS, REGR_AUTORUNS);
		reg.Get (&REGV_NTPROTECT, REGR_NTPROTECT);
		reg.Get (&REGV_PROCVIEW, REGR_PROCVIEW);
		reg.Get (&REGV_SYSTRAYPLAYER, REGR_SYSTRAYPLAYER);

		// Run Co-Initialization
		if (CoInitializeEx(0, COINIT_APARTMENTTHREADED) != 0)
		{
			// Abort Startup
			InfoBox (0, "Call to system function CoInitializeEx failed.", WINSUITE_CAPTION, MB_OK | MB_SYSTEMMODAL | MB_ICONSTOP);
			return false;
		}

		// Start WinSock
		if (!WS2_Start())
		{
			// Abort Startup if unsuccessful
			InfoBox (0, "Error: Could not start WSA Interface, Application halted.", WINSUITE_CAPTION, MB_OK | MB_SYSTEMMODAL | MB_ICONSTOP);
			return -1;
		}


		//
		// Load Resources
		//
		// -- WinSuite [1] --
		L_IDI_WINSUITE = LoadIconRes (myinst, IDI_WINSUITE, 16, 16);
		
		// -- Autoruns [3] --
		L_IDI_REGTOOL = LoadIconRes (myinst, IDI_REGTOOL, 32, 32);
		L_IDI_REGTOOL16 = LoadIconRes (myinst, IDI_REGTOOL, 16, 16);
		L_IDI_RESTART32 = LoadIconRes (myinst, IDI_RESTART32, 32, 32);

		// -- NTProtect [2] --
		L_IDI_APPS = LoadIconRes (myinst, IDI_APPS, 32, 32);
		FONT_MSSANS_BOLD = CreateFont (16, 0, 0, 0, FW_BOLD, false, true, false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_LH_ANGLES, PROOF_QUALITY, 4, "Arial");
	

		//
		// Create VKernel2 Window
		//
		NewWindowClass (WNDCLASS_VK2, &VK2_WndProc, myinst);
		vk2wnd = CreateMainAppWindow (WNDCLASS_VK2, WINSUITE_CAPTION, 0, 0, 0, myinst);
		RegisterHotKey (vk2wnd, 1, MOD_WIN, 'Y');
		RegisterHotKey (vk2wnd, 2, MOD_WIN, VK_F12);
		RegisterHotKey (vk2wnd, 3, 0, VK_SCROLL);


		//
		// Fire up all applications specified in the registry
		//
		if (REGV_AUTORUNS == 1) { mod_autoruns = new Autoruns(); };
		if (REGV_NTPROTECT == 1)
		{
			mod_ntprotect = new NTProtect(); 
			if (!mod_ntprotect->Init()) 
			{ 
				delete mod_ntprotect; 
				mod_ntprotect = 0; 
			}
		}
		if (REGV_PROCVIEW == 1)			{ mod_procview = new ProcView(); };
		if (REGV_SYSTRAYPLAYER == 1)	{ mod_systrayplayer = new SystrayPlayer(); };
		if (REGV_NETMSG_CLIENT == 1)	{ mod_netmsg_client = new NetMsg_Client(); };
		if (REGV_SYSLOG_ENABLE == 1)	{ mod_syslog_client = new Syslog_Client(); };


		//
		// Always run the ARLauncher
		//
		mod_arlauncher = new ARLauncher();
		mod_arlauncher->Launcher_ExternExecute();


		//
		// Main Message Loop
		//
		mainacc = LoadAccelerators (myinst, (LPCTSTR) IDR_SYSTRAYPLAYER);
		while (GetMessage(&msg, 0, 0, 0))
		{
			if (!TranslateAccelerator(GetAbsParent(msg.hwnd), mainacc, &msg))
			{
				if (!IsDialogMessage(GetAbsParent(msg.hwnd), &msg))
				{
					TranslateMessage (&msg);
					DispatchMessage (&msg);
				}
			}
		}

		// Destory Accelerator Table
		DestroyAcceleratorTable (mainacc);
	
		// Wait for the termination of all components
		if (!terminated)
		{
			TerminateAll();
		}

		// Stop WinSock.
		WSACleanup();

		// Return Exit Code
		return (int) msg.wParam;
	}
	else
	{
		//
		// Note: Show Setup Dialog
		//

		// Variables.
		Setup* setup;
		MSG msg;

		// Initialize and Show Setup Dialog
		setup = new Setup();

		// Main Message Loop
		while (GetMessage(&msg, 0, 0, 0))
		{

			if (!IsDialogMessage(GetAbsParent(msg.hwnd), &msg))
			{
				TranslateMessage (&msg);
				DispatchMessage (&msg);
			}
		}

		// Terminate Setup Window Dialog
		delete setup;

		// Return Exit Code
		return (int) msg.wParam;
	}

	// Return Exit Code
	return 0;
}

long DebugHelper_ntService_Thread (long param)
{
	ntsvc_ServiceMain(-1, (LPTSTR*) NULL);
	return 99;
}

void TerminateAll ()
{
	//
	// Shutdown every single module that is currently loaded.
	//
	if (mod_autoruns != 0)			{ delete mod_autoruns; mod_autoruns = 0; };
	if (mod_ntprotect != 0)			
	{ 
		mod_ntprotect->Terminate(); 
		delete mod_ntprotect; 
		mod_ntprotect = 0; 
	}
	if (mod_procview != 0)			{ delete mod_procview; mod_procview = 0; };
	if (mod_systrayplayer != 0)		{ delete mod_systrayplayer; mod_systrayplayer = 0; };
	if (mod_arlauncher != 0)		{ delete mod_arlauncher; mod_arlauncher = 0; };
	if (mod_netmsg_client != 0)		{ delete mod_netmsg_client; mod_netmsg_client = 0; };
	if (mod_syslog_client != 0)		{ delete mod_syslog_client; mod_syslog_client = 0; };

	// First Threads, Then Memory
	TVM.AwaitShutdown ();

	// Unregister Hotkeys
	UnregisterHotKey (vk2wnd, 3);
	UnregisterHotKey (vk2wnd, 2);
	UnregisterHotKey (vk2wnd, 1);

	// Destroy VKernel2 Window
	DestroyWindow (vk2wnd);
	UnregisterClass(WNDCLASS_VK2, myinst);

	// Run Co-UnInitializiation
	CoUninitialize();


	// Free Resources
	// --------------
	//
	// -- Autoruns [3] --
	DestroyIcon (L_IDI_REGTOOL); DestroyIcon (L_IDI_REGTOOL16);
	DestroyIcon (L_IDI_RESTART32);
	
	// -- WinSuite [1] --
	DestroyIcon (L_IDI_WINSUITE);

	// -- NTProtect [2] --
	DestroyIcon (L_IDI_APPS);
	DeleteObject (FONT_MSSANS_BOLD);
	
	// Indicate that all components have terminated successfully
	terminated = true;
}

bool TerminateVKernel_Allowed ()
{
	// Note: This function gets called prior to "TerminateVKernel" during WM_QUERYENDSESSION or WM_ENDSESSION.
	bool allow_terminate = false;
	
	// Check if the system is currently locked by "NTProtect".
	if (mod_ntprotect != 0)
	{
		if (!mod_ntprotect->IsSystemLocked())
		{
			allow_terminate = true;
		}
		else
		{
			// (!) SHUTDOWN on LOCKED SYSTEM in progress.
			// Note: This is only granted if "SystrayPlayer" component permits it.
			if (mod_systrayplayer != 0)
			{
				allow_terminate = mod_systrayplayer->IsShutdownAllowed();
			}
			else
			{
				allow_terminate = false;
			}
		}
	}
	else
	{
		allow_terminate = true;
	}

	// Return result.
	return allow_terminate;
}

bool TerminateVKernel (bool force)
{
	DWORD MainThread = TVM.GetMainThread();

	// Check if the system is locked by "NTProtect" and should resist an application shutdown.
	if (!TerminateVKernel_Allowed())
	{ 
		// (!) Failed.
		return false; 
	}
	else
	{
		// Check if we are in the context of the main thread.
		if (GetCurrentThreadId() == MainThread)
		{
			if (force)
			{
				TerminateAll();
			}
			else
			{
				PostQuitMessage (0);
			}
		}
		else
		{
			PostThreadMessage (MainThread, WM_QUIT, 0, 0);
		}

		// (!) Success.
		return true;
	}
}


//////////////////////////
// VK2 Window Process	//
//////////////////////////
LRESULT CALLBACK VK2_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_CLOSE)
	{
		//
		// Note: This message is received if the user sits on the desktop hitting ALT+F4
		//		 when no other window of this process is visible at the same time.
		//
		//		 This application is not intended to be closed in release versions.
		//

		// Do not invoke WM_DESTROY.
		return false;					
	}
	else if (msg == WM_HOTKEY)
	{
		if ((wparam == 1) && (HIWORD(lparam) == 'Y'))
		{
			// (!) Check if NTProtect is loaded
			if (mod_ntprotect != 0)
			{
				if (!mod_ntprotect->StartLocking())
				{
					InfoBox (0, LANG_EN ? "NT Protect: An error ocurred while trying to lock the windows environment." : 
											"NT Protect: Beim sperren der Windowsoberfläche ist ein Fehler aufgetreten.", 
												WINSUITE_CAPTION, MB_OK | MB_ICONSTOP);
				}
			}
			else
			{
				InfoBox (0, LANG_EN ? "WinSuite: NT Protect component has been disabled by user settings." : 
										"WinSuite: NT Protect wurde in den Benutzereinstellungen deaktiviert.", 	
										WINSUITE_CAPTION, MB_OK | MB_ICONSTOP);
			}
		}
		else if ((wparam == 2) && (HIWORD(lparam) == VK_F12))
		{
			bool bOperationAllowed = false;

			// NTProtect may not be active when this operation is being performed.
			if (mod_ntprotect != 0)
			{
				if (!mod_ntprotect->IsSystemLocked())
				{
					// NTProtect has currently not locked the system.
					bOperationAllowed = true;
				}
			}
			else
			{
				// NTProtect is not loaded and thus may not be active.
				bOperationAllowed = true;
			}

			// Check if operation is allowed.
			if (bOperationAllowed)
			{
				TerminateExplorer();
				Sleep (500);

				//
				// Note: Call must exactly be "%WINDIR%\\Explorer.exe"
				//		 or it will fail to create a shell instance of explorer.
				//
				ShellExecute (0, CSR(GetWinDir(), "\\Explorer.exe"));
			}
		}
		else if ((wparam == 3) && (HIWORD(lparam) == VK_SCROLL))
		{
			// Local Variables.
			CSendKeys csendkeys;
			SYSTEMTIME sysdate;
			string date;
			static bool bReentrantLock = false;

			// ------------------------
			// DATEPRINT - Rollen-Taste
			// ------------------------

			// Check if dateprint component is enabled via procview.
			if (mod_procview != 0)
			{
				if (mod_procview->IsDatePrintEnabled() && !bReentrantLock)
				{
					// Loopback Protection.
					bReentrantLock = true;

					// Test code.
					string strWindowTitle;
					strWindowTitle.resize (256);
					HWND hwndFGW = GetForegroundWindow();
					long lngResult = GetWindowText (hwndFGW, &strWindowTitle[0], (long) strWindowTitle.length());
					if (lngResult > 0)
					{
						strWindowTitle.resize (lngResult);
					}

					// Get current timestamp.
					GetLocalTime (&sysdate);
					date = CSR(FillStr(CStr(sysdate.wYear), 2, '0'), "-", 
								FillStr(CStr(sysdate.wMonth), 2, '0'), "-", 
								FillStr(CStr(sysdate.wDay), 2, '0'));

					// Format timestamp according to current foreground application.
					if (
								(InStr(UCase(strWindowTitle), "EXCEL") > 0) || 
								(InStr(UCase(strWindowTitle), "LIBREOFFICE") > 0) ||
								(InStr(UCase(strWindowTitle), "POWERPOINT") > 0) || 
								(InStr(UCase(strWindowTitle), "REMOTE DESKTOP MANAGER") > 0) ||
								(InStr(UCase(strWindowTitle), "VISIO") > 0) || 
								(InStr(UCase(strWindowTitle), "VISUAL STUDIO") > 0) || 
								(InStr(UCase(strWindowTitle), "WORD") > 0)
							)
					{
							// Do nothing.
					}
					else
					{
						date += "_";
					}

					// Type current date into forefront application window.
					// InfoBox (0, date, CSR("Current Window Title: >", strWindowTitle, "<"), MB_OK);
					csendkeys.SendKeys (&date[0]);

					// Unregister Hotkey to avoid causing subsequent hotkey events with time dilatation.
					UnregisterHotKey (hwnd, 3);

					// Turn the SCROLL LOCK keyboard light off.
					keybd_event (VK_SCROLL, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
					keybd_event (VK_SCROLL, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
					keybd_event (VK_SCROLL, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
					keybd_event (VK_SCROLL, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);

					// Re-Register Hotkey.
					RegisterHotKey (hwnd, 3, 0, VK_SCROLL);

					// Loopback Protection.
					bReentrantLock = false;
				}
			}
		}
	}
	else if (msg == WM_ENDSESSION)
	{
		if (!TerminateVKernel_Allowed())
		{
			return false;
		}

		if (wparam == 1)
		{
			// Session is being SURELY ended after return (this is the MainThread)
			TerminateVKernel (true);
			return false;
		}
	}
	return DefWindowProc (hwnd, msg, wparam, lparam);
}



/////////////////////////////////////////////////////////////////////////
// SETUP VECTOR START HERE		/////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//////////////////////////////
// Setup: Init / Terminate	//
//////////////////////////////
Setup::Setup ()
{
	long REGV_AUTORUNS;
	long REGV_NTPROTECT;
	long REGV_PROCVIEW;
	long REGV_SYSTRAYPLAYER;

	// Init RAM
	reg = 0;
	setupwnd = 0;
	cnt_selected_components = 0;

	// Init Common Registry Settings
	reg = new RegObj(HKCU, REG_WINSUITE);
	reg_ntp = new RegObj(HKCU, CSR(REG_WINSUITE, "\\NTProtect"));
	reg_spl = new RegObj(HKCU, CSR(REG_WINSUITE, "\\SystrayPlayer"));

	reg->Init (REGR_AUTORUNS, 0);		// Changed 2012-12-22: "Autoruns" component is turned off by default.
	reg->Init (REGR_NTPROTECT, 0);
	reg->Init (REGR_PROCVIEW, 1);
	reg->Init (REGR_SYSTRAYPLAYER, 1);

	reg->Get (&REGV_AUTORUNS, REGR_AUTORUNS);
	reg->Get (&REGV_NTPROTECT, REGR_NTPROTECT);
	reg->Get (&REGV_PROCVIEW, REGR_PROCVIEW);
	reg->Get (&REGV_SYSTRAYPLAYER, REGR_SYSTRAYPLAYER);

	// Create Dialog Window
	setupwnd = CreateDialogParam (myinst, (LPCTSTR) IDD_SETUP, 0, &_ext_Setup_WndProc, (LPARAM) this);
	
	// Set Component States to Checkboxes
	chkSetState (dia(setupwnd, IDC_AUTORUNS), REGV_AUTORUNS);
	chkSetState (dia(setupwnd, IDC_NTPROTECT), REGV_NTPROTECT);
	chkSetState (dia(setupwnd, IDC_PROCVIEW), REGV_PROCVIEW);
	chkSetState (dia(setupwnd, IDC_SYSTRAYPLAYER), REGV_SYSTRAYPLAYER);

	// Set Custom Settings Checkboxes
	//
	// NTProtect
	//
	long ntp_showlogo, ntp_showdesk, ntp_writelog;
	string ntp_alarmsound;
	reg_ntp->Init ("ShowLogo", 1);
	reg_ntp->Init ("ShowDesktop", 0);
	reg_ntp->Init ("WriteLog", 1);
	reg_ntp->Init ("AlarmSound", "");
	reg_ntp->Get (&ntp_showlogo, "ShowLogo");
	reg_ntp->Get (&ntp_showdesk, "ShowDesktop");
	reg_ntp->Get (&ntp_writelog, "WriteLog");
	reg_ntp->Get (&ntp_alarmsound, "AlarmSound");
	chkSetState (dia(setupwnd, IDC_NTP_SHOWLOGO), ntp_showlogo);
	chkSetState (dia(setupwnd, IDC_NTP_SHOWDESK), ntp_showdesk);
	chkSetState (dia(setupwnd, IDC_NTP_WRITELOG), ntp_writelog);
	ctlSetText (dia(setupwnd, IDC_NTP_ALARMSOUND), ntp_alarmsound);
	//
	// Systray Player
	//
	string spl_ramdrive;
	reg_spl->Init ("PlayerRamdrive", "");
	reg_spl->Get (&spl_ramdrive, "PlayerRamdrive");
	ctlSetText (dia(setupwnd, IDC_SPL_RDDRIVE), spl_ramdrive);
	chkSetState (dia(setupwnd, IDC_SPL_USERD), (xlen(spl_ramdrive) == 0) ? 0 : 1);
	
	// Set Window Taskbar Icon and Show the Setup Dialog
	SendMessage (setupwnd, WM_SETICON, ICON_SMALL, (LPARAM) L_IDI_WINSUITE);
	ShowWindow (setupwnd, SW_SHOW);
	SetForegroundWindowEx (setupwnd);
}

Setup::~Setup()
{
	// Destroy Setup Dialog Window
	if (setupwnd != 0)
	{
		DestroyWindow (setupwnd);

		// Delete the Registry Object
		if (reg != 0) { delete reg; delete reg_ib; delete reg_ntp; delete reg_spl; };
	}
}


//////////////////////////////
// Setup: Window Process	//
//////////////////////////////
INT_PTR CALLBACK Setup::_ext_Setup_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)					// protected
{
	Setup* dex = (Setup*) GetWindowLongPtr (hwnd, GWLP_USERDATA);
	if (dex != 0) { return dex->WndProc (hwnd, msg, wparam, lparam); };
	if (msg == WM_INITDIALOG)
	{
		// lparam=InitParameter=ClassPointer
		SetWindowLongPtr (hwnd, GWLP_USERDATA, lparam);
		return _ext_Setup_WndProc (hwnd, msg, wparam, lparam);
	}
	return false;
}

INT_PTR CALLBACK Setup::WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)							// public
{
	if (msg == WM_INITDIALOG)
	{
		string desc_autoruns, desc_ntprotect, desc_systrayplayer, desc_procview;
		string desc_moni_phonenums, desc_drive_info, desc_welcome1;

		// Prepare Control Texts
		if (LANG_EN)
		{
			desc_autoruns = "... is a utility to protect your global and user specific autostart list. Whenever a program wants to change it, Autostart Manager will prompt you to authorise the change or roll back to the previous state. Additionally, this component has a built-in editor to help you easily modify your system's autostart entries.";
			desc_ntprotect = "... acts like a high secure screen saver. If you press the magic keyboard shortcut <WIN>+<Y> simultaneously, this component will lock your active Windows session down until you provide a self-defined password correctly.\n\nPlease Note: It is impossible to shutdown, unlock the session or change the currently working user without providing the correct password.";
			desc_systrayplayer = "... represents a multimedia file player which can run minimized to Windows' System Tray while performing playback of your custom play list. You can open its main window on demand at any time, e. g. for changing your play list and then lay it back to the Tray by closing the player window. Playback will, of course, then continue and no player window hinders you to do your work in the foreground. The Player can play back almost every file that Windows can handle by default. (mp3, wav, wma)";
			desc_procview = "... lets you know which programs and services run on your computer at any time. It provides a functionality similar to Windows Task Manager. If an application stops responding, just right-click on Process Viewer's System Tray Icon and select the faulty process in order to force it to terminate immediately. Double-clicking Process Viewer's System Tray Icon offers you a list of individually started processes on your System with detailed information about the windows created by every single process.";
			desc_drive_info = "Driver letter:";
			desc_welcome1 = "Please choose from all available WinSuite components the ones you wish to run during your Windows session:";
		}
		else
		{
			desc_autoruns = "... unterstützt Sie dabei, die Autostartliste für den aktuellen und alle Benutzer auf Änderungen zu überwachen und diese ggf. nach Ihren Wünschen anzupassen. Wenn ein Programm Autostarteinträge anlegt oder verändert, werden Sie jedes Mal um Genehmigung gefragt. Lehnen Sie ab, wird die Änderung rückgängig gemacht.";
			desc_ntprotect = "... verhält sich wie ein sicherer Bildschirmschoner. Sie betätigen die Tasten <WIN>+<Y> gleichzeitig, um Ihren Computer zu sperren. Die Sperre kann nur durch ein von Ihnen festgelegtes persönliches Passwort wieder aufgehoben werden. Während dieser Zeit kann sich auch kein weiterer Benutzer lokal an Ihrem Computer anmelden. (Ausnahme: Terminalserverbenutzer)";
			desc_systrayplayer = "... stellt einen Multimedia-Player dar, der minimiert im Windows Symbolbereich eine von Ihnen zusammen gestellte Playliste abspielen kann. Sie sehen das Hauptfenster des Players nur auf Wunsch, so dass er Sie, während Sie Musik hören, nicht bei der Arbeit stört. Er kann alle gängigen Formate wie z. B. \"mp3\", \"wav\" und \"wma\", die auch von Windows unterstützt werden, abspielen.";
			desc_procview = "... gibt Ihnen zu jedem Zeitpunkt einen Überblick, welche Programme und Dienste auf Ihrem Computer ausgeführt werden. Er funktioniert ähnlich wie der Windows Task Manager und bietet Ihnen auch die Möglichkeit, Tasks und Prozesse zu beenden, sofern Sie die entsprechende Berechtigungsstufe dazu in Windows haben.  Das Tool erlaubt Ihnen zusätzlich das schnelle und komfortable Beenden eines abgestürzten Programms, mit dem Sie bis zum Fehlerzeitpunkt gearbeitet haben.";
			desc_drive_info = "Laufwerk:";
			desc_welcome1 = "Bitte wählen Sie aus allen verfügbaren WinSuite Komponenten diejenigen aus, die gestartet werden sollen:";
		}

		// Set Control Texts
		ctlSetText (dia(hwnd, IDL_AUTORUNS), desc_autoruns);
		ctlSetText (dia(hwnd, IDL_NTPROTECT), desc_ntprotect);
		ctlSetText (dia(hwnd, IDL_SYSTRAYPLAYER), desc_systrayplayer);
		ctlSetText (dia(hwnd, IDL_PROCVIEW), desc_procview);

		ctlSetText (dia(hwnd, IDC_MONITOR_PHONENUMS), desc_moni_phonenums);
		ctlSetText (dia(hwnd, IDC_DRIVE_INFO), desc_drive_info);
		ctlSetText (dia(hwnd, IDC_WELCOME1), desc_welcome1);

		ctlSetText (dia(hwnd, IDOK), LANG_EN ? "&Save Settings and Exit Setup" : "&Speichern und beenden");
		ctlSetText (dia(hwnd, IDCANCEL), LANG_EN ? "&Cancel and Exit Setup" : "Nicht speichern und abbre&chen");

		// Return Init Complete
		return true;
	}
	else if (msg == WM_COMMAND)
	{
		long id = LOWORD(wparam);
		long msg = HIWORD(wparam);
		
		if ((id == IDOK) && (msg == BN_CLICKED))
		{
			// Count how many main components are selected, Reset counter first!
			cnt_selected_components = 0;


			// Write Component Checkbox States to Registry - Note: [State] 0 = unchecked / 1 = checked
			reg->Set (Count(chkGetState(dia(hwnd, IDC_AUTORUNS))), REGR_AUTORUNS);
			reg->Set (Count(chkGetState(dia(hwnd, IDC_NTPROTECT))), REGR_NTPROTECT);
			reg->Set (Count(chkGetState(dia(hwnd, IDC_PROCVIEW))), REGR_PROCVIEW);
			reg->Set (Count(chkGetState(dia(hwnd, IDC_SYSTRAYPLAYER))), REGR_SYSTRAYPLAYER);
			

			// Write Custom Settings Checkbox States to Registry - Note: [State] 0 = unchecked / 1 = checked
			//
			// NT Protect
			//
			reg_ntp->Set (chkGetState(dia(hwnd, IDC_NTP_SHOWLOGO)), "ShowLogo");
			reg_ntp->Set (chkGetState(dia(hwnd, IDC_NTP_SHOWDESK)), "ShowDesktop");
			reg_ntp->Set (chkGetState(dia(hwnd, IDC_NTP_WRITELOG)), "WriteLog");
			reg_ntp->Set (ctlGetText(dia(hwnd, IDC_NTP_ALARMSOUND)), "AlarmSound");
			//
			// Systray Player
			//
			reg_spl->Set ((chkGetState(dia(hwnd, IDC_SPL_USERD)) == 1) ? ctlGetText(dia(hwnd, IDC_SPL_RDDRIVE)) : "", "PlayerRamdrive");
			

			// -----------------------------------------------------------
			// Determine if we need to install or uninstall the NT Service
			// -----------------------------------------------------------
			//
			// This is what is already configured
			string autorun_value;
			bool service_prev_installed;
			service_prev_installed = (regRead(&autorun_value, HKLM, REG_RUN, WINSUITE_SVCNAME) != REG_NOVALUE);

			// We compare the currently requested state to what should be the new state
			if (cnt_selected_components == 0)
			{
				if (service_prev_installed)
				{
					// We need to uninstall
					ShellExecuteRunAs (0, CSR(Chr(34), GetAppExe(), Chr(34), " /Uninstall"));
				}
			}
			else
			{
				if (!service_prev_installed)
				{
					// We need to install
					ShellExecuteRunAs (0, CSR(Chr(34), GetAppExe(), Chr(34), " /Install"));
				}
			}


			//
			// ------------------------------------
			// Exit after everything has been saved
			// ------------------------------------
			TerminateVKernel (false);
		}
		else if ((id == IDC_SPL_RDDRIVE) && (msg == EN_CHANGE))
		{
			// User changed the Ramdrive Edit Control
			chkSetState (dia(hwnd, IDC_SPL_USERD), (xlen(ctlGetText(dia(hwnd, IDC_SPL_RDDRIVE))) != 0));
		}
		else if ((id == IDCANCEL) && (msg == BN_CLICKED))
		{
			// Do not save changes and exit
			TerminateVKernel (false);
		}
	}
	else if (msg == WM_CLOSE)
	{
		// Do not save changes and exit.
		TerminateVKernel (false);

		// Do not invoke WM_DESTROY
		return false;
	}
	else if (msg == WM_ENDSESSION)
	{
		if (!TerminateVKernel_Allowed())
		{
			return false;
		}

		if (wparam == 1)
		{
			// Session is being SURELY ended after return (this is the MainThread)
			TerminateVKernel (true);
			return false;
		}
	}
	return false;
}

long Setup::Count (long pass_selection_state_through)
{
	// This function is being called for every selected component of the setup window.
	// Counter is a global variable that required to be reset first.
	cnt_selected_components += pass_selection_state_through;
	return pass_selection_state_through;
}

