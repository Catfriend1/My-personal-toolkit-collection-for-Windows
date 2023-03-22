// Syslog_Client.cpp
//
// Performs a connection to the NTService via the TCP Protocol
// The Server controls the INI Logfile in the \System32 directory and receives data from this interactive user client
// 
// Thread-Safe: NO
//



//////////////////
// Includes		//
//////////////////
#include "stdafx.h"
#include "../resource.h"

#include "../Modules/_modules.h"
#include "../Boot.h"

#include "Syslog_Shared.h"
#include "Syslog_vcomm.h"
#include "Syslog_Client.h"



//////////////////////
// Class: Constants	//
//////////////////////
const bool Syslog_Client::DEBUG_MODE						= false;

const long Syslog_Client::SYSLOG_TCP_PORT					= 17888;
const char Syslog_Client::SYSLOG_CAPTION[]					= "Syslog";
const char Syslog_Client::CLS_HOSTWINDOW[]					= "Syslog_HostWndClass";
const char Syslog_Client::LOCK_KEY							= 'N';

const long Syslog_Client::TIMER_DELAY						= 1000;
const long Syslog_Client::MSG_DISPTIME						= 12;

const long Syslog_Client::MAINMESSAGE_THREAD_RES			= -109;


//////////////
// Types	//
//////////////



//////////////////////////////////////
// Class: Constructor, Destructor	//
//////////////////////////////////////
Syslog_Client::Syslog_Client ()
{
	// Init RAM.
	MAINMESSAGE_THREAD_ID = 0;
	log_general = false;
	log_capchanges = false;
	signal_client_shutdown = false;
	flag_sent_username_once = false;
	sysicon = 0;

	// Load Resources.
	LoadResources();

	// Register UI Objects
	NewWindowClass (CLS_HOSTWINDOW, &_ext_Syslog_Client_HostWndProc, myinst);

	// Add Systray Icon
	sysicon = new SystrayIcon;

	// Initialize Application Surveillance
	ea2 = new EnumApps;
	ea2->Enum();

	// Spawn Main Message Queue Thread.
	SYSLOG_VM.NewThread (&_ext_Syslog_MainMessageQueue_Thread, (void*) this, &MAINMESSAGE_THREAD_ID);
	
	// Return Success.
	// return true;
}

Syslog_Client::~Syslog_Client ()
{
	// Terminate Main Message Queue Thread.
	if (MAINMESSAGE_THREAD_ID != 0)
	{
		PostThreadMessage (MAINMESSAGE_THREAD_ID, WM_QUIT, MAINMESSAGE_THREAD_RES, 0);

		// Wait for Main Message Queue Thread to finish.
		SYSLOG_VM.AwaitShutdown();
	}

	// Remove Systray Icon
	if (sysicon != 0) 
	{ 
		sysicon->Hide();
		delete sysicon; 
	}

	// Destroy Window Objects
	DestroyWindow (hostwnd);
	
	// Unregister Window Class
	UnregisterClass (CLS_HOSTWINDOW, myinst);

	// Free Resources
	FreeResources();
}


//////////////////////////////////
// Class: Resource Management	//
//////////////////////////////////
void Syslog_Client::LoadResources ()
{
	L_IDI_BLUEEYE = LoadIconRes (myinst, IDI_BLUEEYE, 16, 16);
	L_IDI_SLSTART = LoadIconRes (myinst, IDI_SLSTART, 16, 16);
	L_IDI_SLSTOP = LoadIconRes (myinst, IDI_SLSTOP, 16, 16);
	L_IDI_CHECK = LoadIconRes (myinst, IDI_CHECK, 16, 16);
	L_IDI_LOGFILE = LoadIconRes (myinst, IDI_LOGFILE, 16, 16);
}

void Syslog_Client::FreeResources ()
{
	DestroyIcon (L_IDI_BLUEEYE);
	DestroyIcon (L_IDI_SLSTART);
	DestroyIcon (L_IDI_SLSTOP);
	DestroyIcon (L_IDI_CHECK);
	DestroyIcon (L_IDI_LOGFILE);
}


//////////////////////////////
// Functions: Main Thread	//
//////////////////////////////
// protected
long Syslog_Client::_ext_Syslog_MainMessageQueue_Thread (Syslog_Client* class_ptr)
{
	if (class_ptr != 0) 
	{ 
		return class_ptr->MainMessageQueue_Thread(); 
	} 
	else 
	{ 
		return -1; 
	}
}

// private
long Syslog_Client::MainMessageQueue_Thread ()
{
	MSG msg;

	// Create Host & Tooltip Window
	hostwnd = CreateNullWindow ("Host Window", 0, CLS_HOSTWINDOW);
	SetWindowLongPtr (hostwnd, GWLP_USERDATA, (LONG_PTR) this);
	sysicon->SetData (L_IDI_BLUEEYE, SYSLOG_CAPTION, hostwnd, 0);

	// Register Hotkey "Win+LOCK_KEY"
	RegisterHotKey (hostwnd, 1, MOD_WIN, LOCK_KEY);

	// Start NetClient Worker Thread
	CLIENT_VM.NewThread (&_ext_Syslog_Client_Client_Worker, (void*) this);
	CLIENT_VM.NewThread (&_ext_Syslog_Client_Logtimer, (void*) this);

	// Message Loop
	while (true)
	{
		// Check if we got a shutdown signal
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				// Abort Message Loop
				break;
			}
			else if (!IsDialogMessage(GetAbsParent(msg.hwnd), &msg))
			{
				TranslateMessage (&msg);
				DispatchMessage (&msg);
			}
		}
		
		// Set the Client Thread Idle for a time
		Sleep (50);
	}

	// Signal all CLIENT_VM's a shutdown
	signal_client_shutdown = true;


	//
	// Note: If the client application is shutting down (due to a windows logoff event),
	//		 notify the server of this.
	if (signal_client_shutdown && client.connected())
	{
		if (!client.mod_send(Chr(SLG_VCOMM_NOTIFY_LOGOFF))) 
		{ 
			// FAILED. (unhandled error)
		}
	}

	// Client Receive Loop is blocked by call to WS2::recv. Close the connection forcefully to interrupt.
	client.Close();

	// Unregister Hotkeys.
	UnregisterHotKey (hostwnd, 1);

	// Shutdown NetClient Workers.
	CLIENT_VM.AwaitShutdown();

	// Remove Thread.
	SYSLOG_VM.RemoveThread();

	// (!) Success.
	return (long) msg.wParam;
}

long Syslog_Client::_ext_Syslog_Client_Client_Worker (Syslog_Client* ptr_Syslog_Client_Class)
{
	return ptr_Syslog_Client_Class->Client_Worker();
}

long Syslog_Client::Client_Worker()
{
	while (!signal_client_shutdown)
	{
		// Initialize Client Socket
		if (!client.Create())
		{
			DebugOut ("Syslog Client Failure: Cannot create socket.");
			CLIENT_VM.RemoveThread();
			return 1005;
		}
		
		// Try to connect
		if (client.Connect("127.0.0.1", SYSLOG_TCP_PORT))
		{
			string data;
			unsigned char id;
			bool closed = false;
			
			// Authenticate
			if (!client.mod_send(CSR(Chr(SLG_VCOMM_AUTH), SLG_AUTH_SIGNATURE))) { break; };
			
			// Acquire CFS
			if (!flag_sent_username_once)
			{
				if (client.mod_send(CSR(Chr(SLG_VCOMM_NOTIFY_LOGON), GetCurrentUser())))
				{
					// Success
					flag_sent_username_once = true;
				}
				else
				{
					break;
				}
			}
			
			// CLIENT WORKERS RECEIVE LOOP
			while (!signal_client_shutdown)
			{
				if (client.EnterReceiveLoop(&data, &closed))
				{
					// (!) Server closed the connection
					if (closed) 
					{ 
						break; 
					}
					
					// Get ID
					if (xlen(data) >= 1) { id = data[0]; } else { id = 0; };
					
					// ------------------------------   ACT   -------------------------------------
					if (id == SLG_VCOMM_SPAWN_SETTINGS)
					{
						 string tmp = Right(data, xlen(data)-1);
						 if (xlen(tmp) >= 2)
						 {
							log_general = (tmp[0] == 1);
							log_capchanges = (tmp[1] == 1);
						 }
					}
					else if (id == SLG_VCOMM_SHOW_USERINFO)
					{
						string title, msg;
						string tmp;
						bool set_tooltip;

						if ((xlen(data) - 1) > 1)
						{
							set_tooltip = (data[1] == 1);
							tmp = Right(data, xlen(data)-2);
							long idx = InStr(tmp, "|");
							if (idx > 0)
							{
								title = Left(tmp, idx - 1);
								msg = Right(tmp, xlen(tmp) - idx);

								cs.Enter();
								if (set_tooltip)
								{
									sysicon->SetData(0, CSR(title, " ", msg));
									sysicon->Modify();
								}
								sysicon->ShowDelayed (MSG_DISPTIME);
								sysicon->ShowBalloon (title, msg, MSG_DISPTIME, NIIF_INFO);
								cs.Leave();
							}
						}
					}
					else if (id == SLG_VCOMM_RECV_LOGDATA)
					{
						// Check if the shared buffer contains data
						if (xlen(data) > 1)
						{
							string tmpfn = CSR(GetTempFolder(), "\\System.log");
							HANDLE hfile = CreateFile (&tmpfn[0], GENERIC_WRITE, 0, NULL, 
														CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, NULL);
							if (hfile != 0)
							{
								WriteFileData (hfile, Right(data, xlen(data) - 1));
								CloseHandle (hfile);

								// Execute Notepad to Display the content
								CLIENT_VM.NewThread (&_ext_Syslog_Client_FileView_And_Deletor, (void*) this);
							}
						}
					}
					else
					{
						// -- Unknown packet --
						#ifdef _DEBUG
							closed = true;
							DebugOut ("Syslog Client Error: Invalid Network Message received from server");
							break;
						#endif
					}
				}
			}
		}

		// Transfer finished or no connection, so close that socket and retry
		client.Close();

		// Wait some time before attempting to reconnect to the service
		if (!signal_client_shutdown) 
		{ 
			Sleep(10000); 
		}
	}

	// Remove Thread
	CLIENT_VM.RemoveThread();
	return 5;
}

long Syslog_Client::_ext_Syslog_Client_Logtimer (Syslog_Client* ptr_Syslog_Client_Class)
{
	return ptr_Syslog_Client_Class->Logtimer();
}

long Syslog_Client::Logtimer ()
{
	MSG msg;
	UINT_PTR hLogTimer;

	// Set Logtimer.
	hLogTimer = SetTimer(0, 0, TIMER_DELAY, NULL);

	// Message Loop
	while (!signal_client_shutdown)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_TIMER)
			{
				// Refresh Systray Icon
				cs.Enter();
				sysicon->Modify();
				cs.Leave();

				// Application Surveillance
				if (log_general)
				{
					long a, b, u_a, u_b;
					bool is_newwnd, is_wnddown;

					// -2- exists, -1- does not exist;
					ea1 = new EnumApps;
					ea1->Enum (wmaFrontVisible);

					// Compare Windows
					u_a = ea1->Ubound(); u_b = ea2->Ubound();
					for (a = 1; a <= u_a; a++)
					{
						is_newwnd = true;
						for (b = 1; b <= u_b; b++)
						{
							if (ea1->hwnd(a) == ea2->hwnd(b))
							{
								if (log_capchanges)
								{
									if (ea1->name(a) != ea2->name(b))
									{
										// (!) Window caption changed
										string temp = CSR(Chr(34), ea2->name(b), Chr(34));
										if (xlen(temp) < 28) { temp += ChrString(28 - xlen(temp), 32); };
										WriteLog (CSR(CSR("  [~]  ", temp, " -> "), Chr(34), ea1->name(a), Chr(34)));
									}
								}
								is_newwnd = false;
								break;
							}
						}
						if (is_newwnd && (xlen(ea1->name(a)) != 0))
						{
							// (!) A New Window has been created
							WriteLog (CSR(" [+]  ", Chr(34), ea1->name(a), Chr(34)));
						}
					}
					
					for (b = 1; b <= u_b; b++)
					{
						is_wnddown = true;
						for (a = 1; a <= u_a; a++)
						{
							if (ea1->hwnd(a) == ea2->hwnd(b))
							{
								is_wnddown = false;
								break;
							}
						}
						if (is_wnddown && (xlen(ea2->name(b)) != 0))
						{
							// (!) A Window has been closed
							WriteLog (CSR(" [-]  ", Chr(34), ea2->name(b), Chr(34)));
						}
					}

					// Delete -2-, Move -1- to -2-;
					delete ea2;
					ea2 = ea1;
				}

			}
			else if (msg.message == WM_QUIT)
			{
				break;
			}
		}

		// Sleep
		Sleep (50);
	}

	// Kill Logtimer.
	KillTimer (0, hLogTimer);

	// Remove Thread.
	CLIENT_VM.RemoveThread();
	return 8;
}



//////////////////////
// Window Process	//
//////////////////////
LRESULT CALLBACK Syslog_Client::_ext_Syslog_Client_HostWndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	Syslog_Client* cbclass = (Syslog_Client*) GetWindowLongPtr (hwnd, GWLP_USERDATA);
	if (cbclass != 0)
	{
		return cbclass->HostWndProc (hwnd, msg, wparam, lparam);
	}
	else
	{
		return DefWindowProc (hwnd, msg, wparam, lparam);
	}
}

LRESULT CALLBACK Syslog_Client::HostWndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_CLOSE)
	{
		// Do not invoke WM_DESTROY.
		// The client may not be externally closed.
		return false;
	}
	else if (msg == WM_QUERYENDSESSION)
	{
		if (!TerminateVKernel_Allowed())
		{
			return false;
		}
		else
		{
			// It's ok to terminate Windows Session
			return true;
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
			// Session is being SURELY ended after return (this is the MainThread).
			TerminateVKernel (true);
			return false;
		}
	}
	else if (msg == WM_HOTKEY)
	{
		if ((wparam == 1) && (HIWORD(lparam) == LOCK_KEY))
		{
			sysicon->AbortDelayTimer();
			// -- Bad thing -- sysicon->SetData (0, SYSLOG_CAPTION);
			if (!sysicon->Show())
			{
				// (!) Icon could not be shown because it was already visible
				sysicon->Hide ();
			}
		}
	}
	else if (msg == WM_LBUTTONUP)
	{
		if (lparam == WM_RBUTTONUP)
		{
			// Open Popup Menu.
			ShowSystrayMenu();
		}
	}
	return DefWindowProc (hwnd, msg, wparam, lparam);
}



//////////////////////////////
// Systray Menu Related		//
//////////////////////////////
void Syslog_Client::ShowSystrayMenu ()
{
	// Variables.
	MegaPopup spm;
	POINT pt;
	long hv;
	bool bUserIsAdminMember;

	// Check current users elevation first.
	bUserIsAdminMember = ((GetCurrentUsersElevation() == TokenElevationTypeLimited) ||
								(GetCurrentUsersElevation() == TokenElevationTypeFull));
	
	// Prepare Popup Menu.
	spm.Add (1, (LANG_EN ? "View System Log" : "Log anzeigen"), L_IDI_LOGFILE, MPM_HILITEBOX);
	spm.Add (2);
	
	// Add admin-only menu commands.
	if (bUserIsAdminMember)
	{
		spm.Add (3, (LANG_EN ? "Enable System Logging" : "Log aktivieren"), 
										L_IDI_SLSTART, MPM_HILITEBOX | (MPM_DISABLED*log_general));
		spm.Add (4, (LANG_EN ? "Disable System Logging" : "Log deaktivieren"), 
										L_IDI_SLSTOP, MPM_HILITEBOX | (MPM_DISABLED*(!log_general)));
		spm.Add (5);
		spm.Add (6, (LANG_EN ? "Log caption changes" : "Logge Titelverlauf"), 
										(HICON) ((LONG_PTR) L_IDI_CHECK*log_capchanges), 
											(MPM_HILITEBOX*log_general) | 
											(MPM_DISABLED*(!log_general)));
		spm.Add (7);
	}
	
	// Show Popup Menu.
	GetCursorPos (&pt);
	sysicon->AbortDelayTimer();
	hv = spm.Show (pt.x, pt.y, MPM_RIGHTBOTTOM, MPM_VSCROLL);
	
	// Act depending on menu entry selection.
	if (hv == 1)
	{
		if (!client.mod_send(Chr(SLG_VCOMM_ACQUIRE_LOGDATA)))
		{
			MessageBox (0, 
							(LANG_EN ? "Sorry, cannot load the system log due to an internal error." : 
										"Entschuldigung, wegen eines internen Fehlers konnte das Log nicht geladen werden."), 
							SYSLOG_CAPTION, MB_OK | MB_ICONSTOP);
		}

	}
	else if (hv == 3)
	{
		// Only Notify Server of change and THEN wait for his response
		if (!client.mod_send(CSR(Chr(SLG_VCOMM_SET_LOGGING), Chr(1))))
		{
			MessageBox (0, 
							(LANG_EN ? "Sorry, cannot execute requested command due to an internal error." : 
										"Entschuldigung, wegen eines internen Fehlers konnte der angeforderte Befehl nicht ausgeführt werden."), 
							SYSLOG_CAPTION, MB_OK | MB_ICONSTOP);
		}
	}
	else if (hv == 4)
	{
		// Only Notify Server of change and THEN wait for his response
		if (!client.mod_send(CSR(Chr(SLG_VCOMM_SET_LOGGING), Chr(0)))) 
		{
			MessageBox (0, 
							(LANG_EN ? "Sorry, cannot execute requested command due to an internal error." : 
										"Entschuldigung, wegen eines internen Fehlers konnte der angeforderte Befehl nicht ausgeführt werden."), 
							SYSLOG_CAPTION, MB_OK | MB_ICONSTOP);
		}
	}
	else if (hv == 6)
	{
		// Only Notify Server of change and THEN wait for his response
		if (!client.mod_send(CSR(Chr(SLG_VCOMM_SET_LOGCAPCHANGES), Chr(!log_capchanges)))) 
		{
			MessageBox (0, 
							(LANG_EN ? "Sorry, cannot execute requested command due to an internal error." : 
										"Entschuldigung, wegen eines internen Fehlers konnte der angeforderte Befehl nicht ausgeführt werden."), 
							SYSLOG_CAPTION, MB_OK | MB_ICONSTOP);
		}
	}
}


//////////////////////////
// (!) Thread Interface //
//////////////////////////
bool Syslog_Client::WriteLog (string out)
{
	//
	// (!) Careful: Calling this function from the CLIENT_VM Thread can lead to DEADLOCK
	//
	while (!client.connected() && !signal_client_shutdown) 
	{ 
		Sleep (50); 
	}

	// Return if the operation succeeded.
	return (client.mod_send(CSR(Chr(SLG_VCOMM_LOGTHIS), out)));
}

long Syslog_Client::_ext_Syslog_Client_FileView_And_Deletor (Syslog_Client* ptr_Syslog_Client_Class)
{
	return ptr_Syslog_Client_Class->FileView_And_Deletor();
}

long Syslog_Client::FileView_And_Deletor ()
{
	string tmpfn = CSR(GetTempFolder(), "\\System.log");
	string mypath, myfile, myparam;
	DWORD procid;

	// Try to use "Notepad2" - if present.
	ShellExResolve (CSR("notepad2.exe ", tmpfn), &mypath, &myfile, &myparam);
	if (UCase(mypath) == UCase(myfile))
	{
		// "Notepad2" not present - Fallback to "notepad.exe".
		ShellExResolve (CSR("notepad.exe ", tmpfn), &mypath, &myfile, &myparam);
	}

	// Call Notepad Logviewer and wait for its termination.
	procid = NewProcess16 (CSR(mypath, "\\", myfile), myparam);
	TaskWaitKernel (procid, INFINITE);
	DeleteFile (&tmpfn[0]);

	// Terminate Thread
	CLIENT_VM.RemoveThread();
	return 7;
}

