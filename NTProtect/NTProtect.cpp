// NTProtect.cpp: NT Protect provides a passworded LockDown-Function to the currently running Operating System
//
// Thread-Safe:	NO
//


//////////////////
// Includes		//
//////////////////
#include "stdafx.h"
#include "../resource.h"

#include "../Modules/_modules.h"

#include "vfw.h"
#include "vfwmsgs.h"
#include "evcode.h"

// needed for OS_ChangeVolume member function.
#include <mmdeviceapi.h>
#include "endpointvolume_custom.h"						// Warning: <endpointvolume.h> from microsoft is not compatible with winver 0x0501!

#include "../Boot.h"
#include "../vcomm.h"
#include "NTProtect.h"


//////////////////////////////
// Consts: Class Related	//
//////////////////////////////
const char NTProtect::CAPTION[]				= "NT Protect";
const char NTProtect::LOGFILE[]				= "ntprotect.log";
const char NTProtect::WNDCLASS[]			= "*WS_C32_ntp_locker";

const char NTProtect::REG_NTP_PW[]			= "NTP-Password";
const char NTProtect::REG_SHOW_LOGO[]		= "ShowLogo";
const char NTProtect::REG_SHOW_DESKTOP[]	= "ShowDesktop";
const char NTProtect::REG_WRITE_LOG[]		= "WriteLog";
const char NTProtect::REG_ALARM_SOUND[]		= "AlarmSound";

const long NTProtect::PWDLGRES_KEEP			= 0;
const long NTProtect::PWDLGRES_UNLOCK		= 1;

const long NTProtect::PWCDLGRES_NOACT		= 0;
const long NTProtect::PWCDLGRES_CHANGED		= 1;

const long NTProtect::MODE_DESKTOP			= 1;
const long NTProtect::MODE_LOGO				= 2;
const long NTProtect::MODE_WRITELOG			= 4;

const int  NTProtect::LOCKWND_TIMER_ID			= 1;
const long NTProtect::LOCKWND_TIMER_INTERVAL	= 1000;					// [1000]

const long NTProtect::NTSERVICE_TCP_PORT	= 17889;

const long NTProtect::LOCK_THREAD_RES		= -102;


//////////////////////////////////
// Functions: Init / Terminate	//
//////////////////////////////////
NTProtect::NTProtect()
{
	long regv_show_logo, regv_show_desktop, regv_write_log;

	// Init RAM
	locktime_ticks = 0;
	ntp_runmode = 0;
	ntp_lockthread = 0;
	ntp_log = INVALID_HANDLE_VALUE;
	ntp_alarmsound = "";
	regv_show_logo = 0; 
	regv_show_desktop = 0; 
	regv_write_log = 0;
	inited = false;

	// Get Shell Folder Path for Internal Resource File Extraction
	intfile_path = GetShellFolder(CSIDL_LOCAL_APPDATA);
	if (xlen(intfile_path) == 0)
	{
		DebugOut ("NTProtect::Initialize->Could not get ShellFolder path.");
	}

	// Init Registry
	reg.SetRootKey (HKEY_CURRENT_USER, "Software\\WinSuite\\NTProtect");
	reg.Init (REG_NTP_PW, "");
	reg.Init (REG_SHOW_LOGO, 1);
	reg.Init (REG_SHOW_DESKTOP, 0);
	reg.Init (REG_WRITE_LOG, 1);
	reg.Init (REG_ALARM_SOUND, "");

	// Read RunMode from Registry
	reg.Get (&regv_show_logo, REG_SHOW_LOGO);
	reg.Get (&regv_show_desktop, REG_SHOW_DESKTOP);
	reg.Get (&regv_write_log, REG_WRITE_LOG);

	if (regv_write_log == 1)		{ ntp_runmode |= MODE_WRITELOG; };
	if (regv_show_desktop == 1)		{ ntp_runmode |= MODE_DESKTOP; };
	if (regv_show_logo == 1)		{ if (!(ntp_runmode & MODE_DESKTOP)) { ntp_runmode |= MODE_LOGO; }; };

	// Get Alarm Sound if specified via parameter
	reg.Get(&ntp_alarmsound, REG_ALARM_SOUND);
	if (xlen(ntp_alarmsound) != 0)
	{
		// Alarm Sound was specified in the registry
		if (!FileExists(ntp_alarmsound))
		{
			InfoBox (0, LANG_EN ? CSR("Error: The file \"", ntp_alarmsound, "\" does not exist.") : 
							CSR("Fehler: Die Datei \"", ntp_alarmsound, "\" existiert nicht."), 
								CAPTION, MB_OK | MB_ICONEXCLAMATION);
			ntp_alarmsound = "";
		}
	}
}

NTProtect::~NTProtect()
{
	// Issue a Terminate command to ensure that the Locker Thread finishes its work before the class gets destroyed
	Terminate();
}

bool NTProtect::Init ()																							// public
{
	bool fail_success = true;

	// Store and Return if the operation succeeded
	inited = fail_success;
	return fail_success;
}

void NTProtect::Terminate ()																					// public
{
	if (ntp_lockthread != 0)
	{
		// Lock is currently engaged
		PostThreadMessage (ntp_lockthread, WM_QUIT, LOCK_THREAD_RES, 0);

		// Wait for Locker Thread to finish
		NTP_VM.AwaitShutdown();
	}

	// Uninit complete
	inited = false;
}



//////////////////////////////////
// Functions: Locker Thread		//
//////////////////////////////////
bool NTProtect::IsSystemLocked ()
{
	return (ntp_lockthread != 0);
}

bool NTProtect::StartLocking ()
{
	bool fail_success = false;

	// Check if we are inited
	if (inited)
	{ 
		// Check if lock is already engaged
		if (ntp_lockthread == 0)
		{
			fail_success = (NTP_VM.NewThread(&_ext_NTProtect_LockerThread, (void*) this, &ntp_lockthread) != 0);
		}
		else
		{
			fail_success = true;
		}
	}
	return fail_success;
}

long NTProtect::_ext_NTProtect_LockerThread (NTProtect* class_ptr)
{
	if (class_ptr != 0) { return class_ptr->LockerThread(); } else { return -1; };
}

long NTProtect::LockerThread ()
{
	MSG msg;
	string deskid;

	// Wait
	Sleep (300);

	// Backup current desktop context
	oldprocdesk = GetThreadDesktop(GetCurrentThreadId());

	// Create and set new desktop context
	// Generate Desktop ID
	deskid = CSR("ntpd", CStr((GetTickCount()/2)+NTP_VM.GetMainThread()));

	// Lock User Keyboard Shortcuts of Windows
	bool serviceLockCmdFailSuccess = true;
	#ifndef _DEBUG
	serviceLockCmdFailSuccess = SendCommandToService(CSR(Chr(VCOMM_NTPROTECT_LOCK), str2hex(CStr(locktime_ticks = GetTickCount()))));
	#endif

	// Create a new desktop for NTProtect
	ntp_desk = CreateDesktop(&deskid[0], NULL, NULL, 0, 
												DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW | 
												DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL  |
												DESKTOP_JOURNALPLAYBACK | DESKTOP_JOURNALRECORD | 
												DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS | 
												DESKTOP_SWITCHDESKTOP, NULL);
	if (ntp_desk == 0) { NTP_VM.RemoveThread(); ntp_lockthread = 0; return false; };

	// Assign NTProtect/WinSuite to this desktop
	#ifndef _DEBUG
		if (!SetThreadDesktop(ntp_desk)) { NTP_VM.RemoveThread(); ntp_lockthread = 0; return false; };
		if (!SwitchDesktop(ntp_desk)) { NTP_VM.RemoveThread(); ntp_lockthread = 0; return false; };
	#endif

	// Start Logging if necessary
	Log_Open ();
	
	// Get the time of this lock event
	GetLocalTime (&locktime);
	Log_Write (LANG_EN ? "System lock engaged." : "System gesperrt.");
	
	// Get the Screen Extents
	sw = VirtualScreenWidth();
	sh = VirtualScreenHeight();
	
	// Create the Locker Window
	NewWindowClass (WNDCLASS, &_ext_NTProtect_Callback_Lock_WndProc, myinst, CS_OWNDC, 0, 0, NULL, true);
	lwnd = CreateWindowEx (WS_EX_TOPMOST, WNDCLASS, "» NT Protect «", 
							WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 
							0, 0, sw, sh, 0, 0, myinst, 0);
	SetWindowLongPtr (lwnd, GWLP_USERDATA, (LONG_PTR) this);
	ldc = lwnd_mem.Create (lwnd, sw, sh);
	SetBkMode (ldc, TRANSPARENT);
	
	// Remove the Caption
	RemoveStyle (lwnd, WS_CAPTION);
	
	// Draw the Contents & Set Foreground
	FillRect2 (ldc, 0, 0, sw, sh, sc.BBLACK);
	Graph_InitContents();
	DeActivateWindow(ActivateWindow(lwnd));

	// Hide Mouse Cursor
	ShowCursor (false);

	// If DeskMode is active, make the window transparent
	if (ntp_runmode & MODE_DESKTOP)
	{
		HRGN crgn;
		crgn = CreateRectRgn (0, 0, sw, sh);
		SetWindowRgn (lwnd, crgn, true);
		DeleteObject (crgn);
	}

	// Run Co-Initialization & ActiveMovie Init
	if (xlen(ntp_alarmsound) != 0)
	{
		if (CoInitializeEx(0, COINIT_APARTMENTTHREADED) != 0)
		{
			MessageBox (0, "NTProtect::CoInitializeEx failed !", "", MB_OK | MB_SYSTEMMODAL | MB_ICONSTOP);
		}
		ntp_amovie = new ActiveMovie;
	}

	// Start Lock Window Timer.
	SetTimer (lwnd, LOCKWND_TIMER_ID, LOCKWND_TIMER_INTERVAL, NULL);

	if (!serviceLockCmdFailSuccess) {
		ShowCursor(true);
		InfoBox(0, LANG_EN ? "Could not completely perform a lock on your current windows session because the \"WinSuite\"-Service is not running." :
					"Achtung: Die aktuelle Windows-Sitzung konnte nicht vollständig gesperrt werden, weil der \"WinSuite\"-Dienst nicht gestartet ist.",
					CAPTION, MB_OK  | MB_SYSTEMMODAL | MB_ICONEXCLAMATION);
		ShowCursor(false);
	}

	// Thread Process Queue
	while (GetMessage(&msg, 0, 0, 0))
	{
		if (!IsDialogMessage(GetAbsParent(msg.hwnd), &msg))
		{
			TranslateMessage (&msg);
			DispatchMessage (&msg);
		}
	}

	// Stop Lock Window Timer.
	KillTimer (lwnd, LOCKWND_TIMER_ID);

	// Run ActiveMovie UnInit & Co-UnInitialization
	if (xlen(ntp_alarmsound) != 0)
	{
		delete ntp_amovie;
		CoUninitialize();
	}

	// Stop Logging if necessary
	Log_Write (LANG_EN ? "System unlocked." : "System entsperrt.");
	
	// Hide the Locker Window
	ShowWindow (lwnd, SW_HIDE);

	// Show Mouse Cursor
	ShowCursor (true);
	
	// Destroy the Locker Window
	lwnd_mem.Free();
	DestroyWindow (lwnd);
	UnregisterClass (WNDCLASS, myinst);

	// Reset to the default desktop
	#ifndef _DEBUG
		SetThreadDesktop (oldprocdesk);
		SwitchDesktop (oldprocdesk);
		CloseDesktop (ntp_desk);
	#endif

	// Unlock all User Input & System Keys
	if (!SendCommandToService (CSR(Chr(VCOMM_NTPROTECT_UNLOCK), str2hex(CStr(locktime_ticks+0x31875)))))
	{
		// InfoBox would not work here because it prevents the thread from exiting as the message queue is no longer active.
		#ifdef _DEBUG
		Log_Write (LANG_EN ? "Could not completely unlock your current windows session because the \"WinSuite\"-Service is not running." : 
								"Achtung: Die aktuelle Windows-Sitzung konnte nicht vollständig entsperrt werden, weil der \"WinSuite\"-Dienst nicht gestartet ist.");
		#endif
	}
	Log_Close();

	// (!) Success
	NTP_VM.RemoveThread();
	ntp_lockthread = 0;
	return LOCK_THREAD_RES;
}



//////////////////////////
// Graphical Design		//
//////////////////////////
void NTProtect::Graph_InitContents ()
{
	if (ntp_runmode & MODE_LOGO)
	{
		TransIcon (ldc, L_IDI_APPS, 30, 30, 32, 32);
		SetTextColor (ldc, RGB(0, 200, 90));
		FontOut (ldc, 80, 38, "NT Protect", FONT_MSSANS_BOLD, false);
	}
}

void NTProtect::Graph_UpdateContents ()
{
	//////////////////////////
	// Initial Color Set:	//
	// ------------------	//
	//						//
	// C1: 0, 160, 155		//
	// C2: 0, 92, 155		//
	//////////////////////////
	static unsigned char rch = 80, gch = 92, bch = 155;
	static int direction = 0;
	
	// Calculate Colors
	srand ((unsigned int) GetTickCount());
	int add = rand() % 10;
	if (direction == 0)
	{
		if ((bch + add) >= 230)
		{
			rch = 30 + rand() % 60;
			gch = 75 + rand() % 40;
			direction = 1;
		}
		else
		{
			bch += add;
		}
	}
	else
	{
		if ((bch - add) <= 40)
		{
			rch = 30 + rand() % 60;
			gch = 75 + rand() % 40;
			direction = 0;
		}
		else
		{
			bch -= add;
		}
	}
	
	// Update the Gradient
	DrawGradient (ldc, 4, 7, sw-4, (sh-9)/2, rch, 160, bch, 
											 rch, gch, bch, GRADIENT_FILL_RECT_V);
	
	DrawGradient (ldc, 4, (sh-9)/2, sw-4, sh-9, rch, gch, bch, 
												rch, 160, bch, GRADIENT_FILL_RECT_V);
	
	// Redraw the GroupBox
	DrawGroupBox (ldc, 4, 7, sw-8, sh-14, CAPTION);

	// Refresh the Locker Window
	lwnd_mem.Refresh();
}


//////////////////////
// Window Process	//
//////////////////////
LRESULT CALLBACK NTProtect::_ext_NTProtect_Callback_Lock_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	NTProtect* class_ptr = (NTProtect*) GetWindowLongPtr (hwnd, GWLP_USERDATA);
	if (class_ptr != 0)
	{
		return class_ptr->Lock_WndProc (hwnd, msg, wparam, lparam);
	}
	return DefWindowProc (hwnd, msg, wparam, lparam);
}

LRESULT CALLBACK NTProtect::Lock_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_PAINT)
	{
		if (!(ntp_runmode & MODE_DESKTOP))
		{
			lwnd_mem.Refresh();
			return false;
		}
	}
	else if (msg == WM_TIMER)
	{
		if (wparam == LOCKWND_TIMER_ID)
		{
			// Check if the lock is still intact.
			HDESK hActiveDesktop = OpenDesktop("Default", 0, false, DESKTOP_SWITCHDESKTOP);
#ifndef _DEBUG
			// !! RELEASE !!
			if (hActiveDesktop == 0)
			{
				// Lock broke detected. Probably the active desktop changed.
				Log_Write (LANG_EN ? "[ERROR] System lock broken, attempting to restore ..." : "[FEHLER] Systemsperre defekt, versuche Wiederherstellung ...");
			
				// Try to restore the lock.
				bool bResult = (SetThreadDesktop(ntp_desk) != 0);
				bResult = bResult && (SwitchDesktop(ntp_desk) != 0);
				if (!bResult)
				{
					Log_Write (LANG_EN ? "[ERROR] System lock broken because it could not be restored." : "[FEHLER] Systemsperre defekt, da die Wiederherstellung nicht moeglich war.");
				}
			}
			else
#else
			// !! DEBUG !!
			Log_Write (CStr((long) hActiveDesktop));
#endif

			// Close Active Desktop Test Handle.
			CloseDesktop (hActiveDesktop);
		}
	}
	else if ((msg == WM_LBUTTONDOWN) || 
				(msg == WM_MBUTTONDOWN) ||
				(msg == WM_RBUTTONDOWN) || 
				(msg == WM_KEYUP))
	{
		// Raise alarm if necessary
		if (ntp_alarmsound.length() != 0)
		{
			// Loop if necessary
			if (ntp_amovie->GetPosition() == ntp_amovie->GetLength())
			{
				ntp_amovie->Close();
				ntp_amovie->Open(ntp_alarmsound);
				ntp_amovie->SetVolume ((long) (((double) (100 - (100)) / 100) * VOLUME_SILENT));
				ntp_amovie->Play();
			}
		}

		// Act on Input
		if ((msg == WM_KEYUP) && (wparam == 112))		// <F1>
		{
			ShowCursor (true);
			Show_SystemInfo();
			ShowCursor (false);
		}
		else if ((msg == WM_KEYUP) && (wparam == 27))			// <ESC>
		{
			// (!) Do nothing
		}
		else if ((msg == WM_KEYUP) && (wparam == 175))			// <Volume Up>
		{
			OS_Volume_StepUp();
		}
		else if ((msg == WM_KEYUP) && (wparam == 174))			// <Volume Down>
		{
			OS_Volume_StepDown();
		}
		else if ((msg == WM_KEYUP) && (wparam == 173))			// <Mute>
		{
			OS_Volume_Mute();
		}
		else if ((msg == WM_KEYUP) && (wparam == 176))			// <Forward>
		{
			if (mod_systrayplayer != 0)
			{
				mod_systrayplayer->Command_Next();
			}
		}
		else if ((msg == WM_KEYUP) && (wparam == 177))			// <Backward, Replay current>
		{
			if (mod_systrayplayer != 0)
			{
				mod_systrayplayer->Command_Play(false);
			}
		}
		else if ((msg == WM_KEYUP) && (wparam == 178))			// <Stop>
		{
			if (mod_systrayplayer != 0)
			{
				mod_systrayplayer->Command_Stop();
			}
		}
		else if ((msg == WM_KEYUP) && (wparam == 179))			// <Play/Pause>
		{
			if (mod_systrayplayer != 0)
			{
				if (mod_systrayplayer->getPlayerState() == SYSTRAYPLAYER_STATE_PAUSED)
				{
					mod_systrayplayer->Command_Play(false);
				}
				else
				{
					mod_systrayplayer->Command_Pause();
				}
			}
		}
		else
		{
			// Show the cursor
			ShowCursor (true);

			// Perform the Password Checking Dialog
			if (DialogBoxParam (myinst, (LPCTSTR) IDD_PASS, lwnd, &_ext_NTProtect_Callback_EnterPW_WndProc, (LONG_PTR) this) == PWDLGRES_UNLOCK)
			{
				PostQuitMessage (LOCK_THREAD_RES);
			}

			// Hide the cursor
			ShowCursor (false);
		}
	}
	else if (msg == WM_CLOSE)
	{
		// Do not invoke WM_DESTROY.
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
			// Note: Session is being SURELY ended after return (this is the MainThread)
			TerminateVKernel (true);
			return false;
		}
	}

	// Call default window process.
	return DefWindowProc (hwnd, msg, wparam, lparam);
}


//////////////////////////////////
// Password Dialog Procedure	//
//////////////////////////////////
INT_PTR CALLBACK NTProtect::_ext_NTProtect_Callback_EnterPW_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static NTProtect* class_ptr = 0;
	if (class_ptr != 0)
	{
		return class_ptr->EnterPW_WndProc (hwnd, msg, wparam, lparam);
	}
	else
	{
		if (msg == WM_INITDIALOG)
		{
			class_ptr = (NTProtect*) lparam;
			return class_ptr->EnterPW_WndProc (hwnd, msg, wparam, lparam);
		}
	}
	return DefWindowProc (hwnd, msg, wparam, lparam);
}

INT_PTR CALLBACK NTProtect::EnterPW_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static bool log_failure_once = false;

	if (msg == WM_INITDIALOG)
	{
		RECT prc, myrc;

		// Set Texts
		ctlSetText (hwnd, LANG_EN ? "··· Enter Password ···" : "··· Passworteingabe ···");
		ctlSetText (dia(hwnd, IDL_LOGINPROMPT), 
							LANG_EN ? "This workstation has been locked. Please authenticate by entering your password:" : 
										"Dieser Computer ist gesperrt. Bitte geben Sie das Passwort ein:");
		ctlSetText (dia(hwnd, IDB_CHANGE), LANG_EN ? "Change" : "Ändern");
		ctlSetText (dia(hwnd, IDCANCEL), LANG_EN ? "Can&cel" : "Abbre&chen");
		
		// Layout the Dialog
		// 2012-10-26: Not compatible with multiple monitor systems: GetClientRect (GetParent(hwnd), &prc);
		prc.right = ScreenWidth();
		prc.bottom = ScreenHeight();
		GetClientRect (hwnd, &myrc);
		SetWindowPos (hwnd, 0, (prc.right / 2) - (myrc.right / 2), 
								(prc.bottom / 2) - (myrc.bottom / 2), 0, 0, SWP_NOSIZE);
		
		// Focus the Password Field
		SetFocus (dia(hwnd, IDC_PW));
		
		// Check the zero pwd
		EnterPW_WndProc (hwnd, WM_COMMAND, MAKEWPARAM(IDC_PW, EN_CHANGE), 0);
	}
	else if (msg == WM_COMMAND)
	{
		long id = LOWORD(wparam); long msg = HIWORD(wparam);
		
		// Buttons
		if (msg == BN_CLICKED)
		{
			if (id == IDOK)
			{
				if (IsPwCorrect(ctlGetText(dia(hwnd, IDC_PW))))
				{
					EndDialog (hwnd, PWDLGRES_UNLOCK);
				}
				else
				{
					// Log
					if (!log_failure_once)
					{
						log_failure_once = true;
						Log_Write (LANG_EN ? "(!) Authentication failed" : "(!) Anmeldung fehlgeschlagen");
					}
					
					// Act
					// InfoBox (hwnd, LANG_EN ? "Incorrect Password!" : "Falsches Passwort!", CAPTION, MB_OK | MB_ICONEXCLAMATION);
					ctlSetText (dia(hwnd, IDC_PW), "");
					SetFocus (dia(hwnd, IDC_PW));
				}
			}
			else if (id == IDB_CHANGE)
			{
				if (DialogBoxParam(myinst, (LPCTSTR) IDD_CHANGEPW, hwnd, &_ext_NTProtect_Callback_ChangePW_WndProc, (LONG_PTR) this)
							== PWCDLGRES_CHANGED)
				{
					// Log
					Log_Write (LANG_EN ? "(!) Password has been changed" : "(!) Das Passwort wurde geändert.");

					// Clear out currently filled in (old) password
					ctlSetText (dia(hwnd, IDC_PW), "");

					// Notify of change
					EnterPW_WndProc (hwnd, WM_COMMAND, MAKEWPARAM(IDC_PW, EN_CHANGE), 0);

					// Adjust focus
					SetFocus (dia(hwnd, IDC_PW));
				}
			}
			else if (id == IDCANCEL)
			{
				EndDialog (hwnd, PWDLGRES_KEEP);
			}
		}
		
		// Edit Box
		if ((id == IDC_PW) && (msg == EN_CHANGE))
		{
			if (IsPwCorrect(ctlGetText(dia(hwnd, IDC_PW))))
			{
				EnableWindow (dia(hwnd, IDOK), true);
				EnableWindow (dia(hwnd, IDB_CHANGE), true);
			}
			else
			{
				EnableWindow (dia(hwnd, IDB_CHANGE), false);
				EnableWindow (dia(hwnd, IDOK), true);
			}
		}
	}
	else if (msg == WM_CLOSE)
	{
		// End password dialog upon user request.
		EndDialog (hwnd, PWDLGRES_KEEP);
	}
	return false;
}

bool NTProtect::IsPwCorrect (string given_pwd)
{
	string pwcache;

	reg.Get (&pwcache, REG_NTP_PW);
	if (xlen(pwcache) < 0)
	{
		pwcache.resize(0);
	}

	return (hex2str(pwcache) == given_pwd);
}


//////////////////////////////////////////
// Change Password Dialog Procedure		//
//////////////////////////////////////////
INT_PTR CALLBACK NTProtect::_ext_NTProtect_Callback_ChangePW_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static NTProtect* class_ptr = 0;
	if (class_ptr != 0)
	{
		return class_ptr->ChangePW_WndProc (hwnd, msg, wparam, lparam);
	}
	else
	{
		if (msg == WM_INITDIALOG)
		{
			class_ptr = (NTProtect*) lparam;
			return class_ptr->ChangePW_WndProc (hwnd, msg, wparam, lparam);
		}
	}
	return DefWindowProc (hwnd, msg, wparam, lparam);
}

INT_PTR CALLBACK NTProtect::ChangePW_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_INITDIALOG)
	{
		// Set Texts
		ctlSetText (hwnd, LANG_EN ? "Change Password" : "Passwort ändern");
		ctlSetText (dia(hwnd, IDL_CHANGEPWPROMPT), LANG_EN ? "Please enter and verify your new desired password now:" : "Bitte geben Sie das neue Passwort in beide Zeilen ein:");
		ctlSetText (dia(hwnd, IDCANCEL), LANG_EN ? "Cancel" : "Abbrechen");

		// Set Focus
		SetFocus (dia(hwnd, IDC_NEWPW));
	}
	else if (msg == WM_COMMAND)
	{
		long id = LOWORD(wparam); long msg = HIWORD(wparam);
		
		// Buttons
		if (msg == BN_CLICKED)
		{
			if (id == IDOK)
			{
				if (ctlGetText(dia(hwnd, IDC_NEWPW)) == ctlGetText(dia(hwnd, IDC_VERNEWPW)))
				{
					reg.Set (str2hex(ctlGetText(dia(hwnd, IDC_NEWPW))), REG_NTP_PW);
					EndDialog (hwnd, PWCDLGRES_CHANGED);
				}
				else
				{
					InfoBox (hwnd, LANG_EN ? "Error: The two passwords you typed are different!" : "Fehler: Die zwei eingegebenen Passworte stimmen nicht überein!", CAPTION, MB_OK | MB_ICONSTOP);
					ctlSetText (dia(hwnd, IDC_NEWPW), "");
					ctlSetText (dia(hwnd, IDC_VERNEWPW), "");
					SetFocus (dia(hwnd, IDC_NEWPW));
				}
			}
			else if (id == IDCANCEL)
			{
				EndDialog (hwnd, PWCDLGRES_NOACT);
			}
		}
	}
	return false;
}


//////////////////////////////////
// System Information			//
//////////////////////////////////
void NTProtect::Show_SystemInfo ()
{
	// Get the system's running time
	SYSTEMTIME runtime;
	msecs2systime (GetTickCount(), &runtime);
	
	// Display
	MessageBox (lwnd, &CSR(
						CSR("Computer:\t", GetComputerName(), "\n"), 
						CSR(LANG_EN ? "\nCurrent user:\t" : "\nAktueller Benutzer:\t", GetCurrentUser()), 
						CSR(LANG_EN ? "\nSystem runtime:\t" : "\nSystemlaufzeit:\t", time(&runtime)), 
						CSR(LANG_EN ? "\nSystem locked:\t" : "\nSystem gesperrt:\t", date(&locktime), " @ ", _clock(&locktime))
									)[0], "System Information", MB_ICONINFORMATION | MB_OK | MB_SYSTEMMODAL);
}


//////////////////////////////////
// Generic Logging Functions	//
//////////////////////////////////
void NTProtect::Log_Open ()
{
	if (ntp_runmode & MODE_WRITELOG)
	{
		string logfolder = GetShellFolder(CSIDL_LOCAL_APPDATA);

		// Close an existing log handle
		if (ntp_log != INVALID_HANDLE_VALUE) { Log_Close(); };
		
		// Create or open the logfile
		ntp_log = CreateFile (&CSR(logfolder, "\\", LOGFILE)[0], 
								GENERIC_WRITE, FILE_SHARE_READ, 0, 
								OPEN_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, 0);
		if (ntp_log != INVALID_HANDLE_VALUE)
		{
			// Append all data
			SetFilePointer (ntp_log, GetFileSize(ntp_log, 0), 0, FILE_BEGIN);
			WriteFileData (ntp_log, "\r\n");
		}
		else
		{
			InfoBox (0, CSR("Error accessing logfile \"", LOGFILE, "\"."), CAPTION, MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
		}
	}
}

void NTProtect::Log_Write (string text)
{
	if ((ntp_runmode & MODE_WRITELOG) && (ntp_log != INVALID_HANDLE_VALUE))
	{
		WriteFileData (ntp_log, CSR(
									CSR("[", date(), " ", time(), "]"), 
									" ", text, "\r\n"));
	}
}

void NTProtect::Log_Close ()
{
	if ((ntp_runmode & MODE_WRITELOG) && (ntp_log != INVALID_HANDLE_VALUE))
	{
		CloseHandle (ntp_log);
		ntp_log = INVALID_HANDLE_VALUE;
	}
}


//////////////////////////////////////////
// Functions: NTService Communication	//
//////////////////////////////////////////
bool NTProtect::SendCommandToService(string ws_svc_command)
{
	bool fail_success = false;
	csocket client;
	
	// Initialize Client Socket
	if (!client.Create())
	{
		DebugOut ("Winsuite/NTProtect::SendCommandToService->Client cannot create socket.");
	}
	else
	{
		// Try to connect
		if (client.Connect("127.0.0.1", NTSERVICE_TCP_PORT))
		{
			// Authenticate
			if (client.mod_send(CSR(Chr(VCOMM_AUTH), AUTH_SIGNATURE)))
			{
				// Successfully sent data, now send our requested command to the NT Service
				fail_success = client.mod_send(ws_svc_command);
			}
		}
		else
		{
			// Error
		}

		// Transfer finished or No connection, so close that socket
		client.Close();
	}

	// Return if the operation succeeded
	return fail_success;
}


//////////////////////////////////////////////////
// Functions: Win32 API for Volume Management	//
//////////////////////////////////////////////////
bool NTProtect::OS_Volume_StepUp ()											// private
{
	long lngVolume = OS_GetVolume() + 5;
	if (lngVolume > 100) { lngVolume = 100; };
	return OS_SetVolume(lngVolume);
}

bool NTProtect::OS_Volume_StepDown ()										// private
{
	long lngVolume = OS_GetVolume() - 5;
	if (lngVolume < 0) { lngVolume = 0; };
	return OS_SetVolume(lngVolume);
}

bool NTProtect::OS_Volume_Mute ()											// private
{
	static long lngPrevVolume = 0;
	long lngCurVolume = OS_GetVolume();

	if (lngCurVolume > 0)
	{
		lngPrevVolume = lngCurVolume;
		return OS_SetVolume(0);
	}
	else
	{
		 return OS_SetVolume(lngPrevVolume);
	}
}

long NTProtect::OS_GetVolume ()												// private
{
	//
	// Return Value: OS_GetVolume() returns a value between 0 and 100 representing relative volume in percent.
	//

	// Variables.
	IMMDeviceEnumerator *deviceEnumerator = NULL;
	IMMDevice *defaultDevice = NULL;
	IAudioEndpointVolume *endpointVolume = NULL;
    HRESULT hResult = NULL;
	float fCurrentVolume = 0;
 
	// Initialize mmdev subsystem.
    CoInitialize (NULL);
    hResult = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, 
								__uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
    hResult = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
    deviceEnumerator->Release();
    deviceEnumerator = NULL;

	// Get current audio endpoint.
    hResult = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID *) &endpointVolume);
    defaultDevice->Release();
    defaultDevice = NULL;
 
	// Get audio endpoint master volume.
    // endpointVolume->GetMasterVolumeLevel(&fCurrentVolume);
    hResult = endpointVolume->GetMasterVolumeLevelScalar(&fCurrentVolume);
    
	// Release audio endpoint.
	endpointVolume->Release();
 
	// Uninitialize mmdev subsystem.
    CoUninitialize();
    return (long) (fCurrentVolume * 100);
}

bool NTProtect::OS_SetVolume (long lngVolumePercent)						// private
{
	//
	// Note: lngVolumePercent has to be between 0 and 100.
	//

	// Variables.
    IMMDeviceEnumerator *deviceEnumerator = NULL;
	IMMDevice *defaultDevice = NULL;
	IAudioEndpointVolume *endpointVolume = NULL;
    HRESULT hResult = NULL;
    double newVolume = ((double) lngVolumePercent / 100);
	float fCurrentVolume = 0;

	// Initialize mmdev subsystem.
	CoInitialize (NULL);
    hResult = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, 
								__uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
    hResult = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
    deviceEnumerator->Release();
    deviceEnumerator = NULL;
 
	// Get current audio endpoint.
    hResult = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID *) &endpointVolume);
    defaultDevice->Release();
    defaultDevice = NULL;
 
	// Set audio endpoint master volume.
	// hResult = endpointVolume->SetMasterVolumeLevel((float)newVolume, NULL);
	hResult = endpointVolume->SetMasterVolumeLevelScalar((float)newVolume, NULL);

	// Release audio endpoint.
    endpointVolume->Release();
 
	// Uninitialize mmdev subsystem.
    CoUninitialize();

	// Return Success.
    return true;
}

