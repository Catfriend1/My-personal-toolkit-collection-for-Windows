// ProcView.cpp: Process Viewer's Main Module
//
// Thread-Safe: NO
//


//////////////////
// Includes		//
//////////////////
#include "stdafx.h"
#include "../resource.h"

#include "../Modules/_modules.h"
#include "../Modules/Ext/ShellExDlg.h"

#include "../WindowPoser/WindowPoser.h"

#include "../Boot.h"
#include "ProcView.h"


//////////////
// Consts	//
//////////////
const char ProcView::CAPTION[]					= "Process Manager";
const char ProcView::WNDCLASS[]					= "*WS_C32_ProcView_Window";

const char ProcView::REG_WINDOW_X[]				= "ProcView_WindowX";
const char ProcView::REG_WINDOW_Y[]				= "ProcView_WindowY";

const char ProcView::REG_WINDOWPOSER[]			= "WindowPoser";
const char ProcView::REG_DATEPRINT[]			= "DatePrint";

const char ProcView::REG_HOTKEY_TIP[]			= "Hotkey_Tip_Shown";
const long ProcView::DEF_HOTKEY_TIP				= 0;

const long ProcView::DEF_WINDOW_X				= 190;
const long ProcView::DEF_WINDOW_Y				= 130;
const long ProcView::DEF_WINDOW_WIDTH			= 433;
const long ProcView::DEF_WINDOW_HEIGHT			= 640;

const long ProcView::WINDOW_MIN_WIDTH			= 400;					// [350]
const long ProcView::WINDOW_MIN_HEIGHT			= 291;					// [291]

const int  ProcView::PROCWND_TIMER_ID			= 1;
const int  ProcView::SYSICON_TIMER_ID			= 2;
const long ProcView::PROCWND_TIMER_INTERVAL		= 2000;					// [2000]
const long ProcView::SYSICON_TIMER_INTERVAL		= 5000;					// [5000]

const long ProcView::StdMenuItems				= 5;
const long ProcView::StartKernel				= StdMenuItems + 1;			// [StdMenuItems + 1]
const long ProcView::StartSystem				= StartKernel + 41;			// [StartKernel + 41]
const long ProcView::StartOther					= StartSystem + 81;			// [StartSystem + 81]

const char ProcView::plist_kernel_w10[]			= "*[System Process]*System*SVCHOST.EXE*MEMORY COMPRESSION*Registry*";
const char ProcView::plist_system_w10[]			= "*SMSS.EXE*CSRSS.EXE*WINLOGON.EXE*SERVICES.EXE*LSASS.EXE*EXPLORER.EXE*SPOOLSV.EXE*ALG.EXE*CTFMON.EXE*AUDIODG.EXE*DWM.EXE*TASKHOST.EXE*LSM.EXE*WININIT.EXE*SEARCHFILTERHOST.EXE*SEARCHPROTOCOLHOST.EXE*SEARCHINDEXER.EXE*SPPSVC.EXE*VDS.EXE*VSSVC.EXE*PRINTISOLATIONHOST.EXE*WUAUCLT.EXE*MSCORSVW.EXE*OSPPSVC.EXE*CONHOST.EXE*DLLHOST.EXE*MSMPENG.EXE*MSSECES.EXE*SHELLEXPERIENCEHOST.EXE*SEARCHUI.EXE*SMSVCHOST.EXE*PRESENTATIONFONTCACHE.EXE*MYSQLD.EXE*OfficeClickToRun.exe*WmiPrvSE.exe*NisSrv.exe*WindowsInternal.ComposableShell.Experiences.TextInput.InputApp.exe*StartMenuExperienceHost.exe*MpCopyAccelerator.exe*";



//////////////////////////////////////////
// Consts: Third-party software info	//
//////////////////////////////////////////
// 1) Sophos AntiVirus
const char ProcView::REG_SOPHOS_SAVSERVICE_APPLICATION[]			= "SOFTWARE\\Sophos\\SAVService\\Application";										// Wow6432Node HKLM
const char ProcView::REGN_SOPHOS_SAVSERVICE_PATH[]					= "Path";

const char ProcView::REG_SOPHOS_WEBINTELLIGENCE_APPLICATION[]		= "SOFTWARE\\Sophos\\Web Intelligence";												// Wow6432Node HKLM
const char ProcView::REGN_SOPHOS_WEBINTELLIGENCE_LOCATION[]			= "Location";

const char ProcView::REG_SOPHOS_MCS[]								= "SOFTWARE\\Sophos\\Remote Management System\\ManagementAgent\\Adapters\\MCS";		// Wow6432Node HKLM
const char ProcView::REGN_SOPHOS_MCS_ADAPTER[]						= "DllPath";

const char ProcView::REG_SOPHOS_ALC[]								= "SOFTWARE\\Sophos\\Remote Management System\\ManagementAgent\\Adapters\\ALC";		// Wow6432Node HKLM
const char ProcView::REGN_SOPHOS_ALC_ADAPTER[]						= "DLLPath";


// 2) Realtek Audio Control Panel
const char ProcView::REG_REALTEK_AUDIO_GUIINFO[]					= "SOFTWARE\\Realtek\\Audio\\GUI_INFORMATION";										// 64-Bit HKLM
const char ProcView::REGN_REALTEK_AUDIO_PATH[]						= "CplDirectory";



//////////////////////////////
// Class: Init / Terminate	//
//////////////////////////////
ProcView::ProcView()
{
	long initx, inity;

	// Init RAM.
	ctrlinited = false;
	TRAY_CAPTION = CSR(Chr(187), " Process Manager");
	my_exefullfn = GetFilenameFromCall(CSR(Chr(34), GetAppExe(), Chr(34)));
	my_procid = GetCurrentProcessId();
	mod_windowposer = 0;

	DYN_WINAPI_Wow64DisableFsRedirection = 0;
	DYN_WINAPI_Wow64RevertFsRedirection = 0;

	// Load Filterlist according to currently loaded OS
	os_filter_kernel = plist_kernel_w10;
	os_filter_system = plist_system_w10;

	// Get System Root.
	if (is_64_bit_os == true)
	{
		// For 64-Bit OS only.

		// Get WinAPI.
		DYN_WINAPI_Wow64DisableFsRedirection = (DYN_WINAPI_WOW64DISABLEFSREDIRECTION) GetProcAddress (
										GetModuleHandle(TEXT("kernel32.dll")), 
										"Wow64DisableWow64FsRedirection"
										);
		DYN_WINAPI_Wow64RevertFsRedirection = (DYN_WINAPI_WOW64REVERTFSREDIRECTION) GetProcAddress (
										GetModuleHandle(TEXT("kernel32.dll")), 
										"Wow64RevertWow64FsRedirection"
										);

		// Get "Windows\\SysWow64".
		os_system_dir = GetSystemDirWow64();
	}
	else
	{
		// Get "Windows\\System32".
		os_system_dir = GetSystemDir32();
	}

	// Load Resources.
	LoadResources();
	
	// Initialize Registry Settings.
	reg.SetRootKey (HKEY_CURRENT_USER, "Software\\WinSuite\\ProcView");
	reg.Init (REG_WINDOW_X, DEF_WINDOW_X);
	reg.Init (REG_WINDOW_Y, DEF_WINDOW_Y);
	reg.Init (REG_HOTKEY_TIP, DEF_HOTKEY_TIP);
	reg.Init (REG_WINDOWPOSER, 0);
	reg.Init (REG_DATEPRINT, 0);
	
	// Read Registry Settings.
	reg.Get (&initx, REG_WINDOW_X);
	reg.Get (&inity, REG_WINDOW_Y);
	reg.Get (&REGV_WINDOWPOSER, REG_WINDOWPOSER);
	reg.Get (&REGV_DATEPRINT, REG_DATEPRINT);

	// Get third-party installation paths.

	// Realtek Audio Control Panel
	if (is_64_bit_os)
	{
		regRead64 (&REGV_REALTEK_AUDIO_PATH, HKLM, REG_REALTEK_AUDIO_GUIINFO, REGN_REALTEK_AUDIO_PATH);
	}
	else
	{
		reg.Get (&REGV_REALTEK_AUDIO_PATH, REGN_REALTEK_AUDIO_PATH, HKLM, REG_REALTEK_AUDIO_GUIINFO);
	}
	Cutb (&REGV_REALTEK_AUDIO_PATH);
	


	// Verify Window Coordinates.
	if (!CheckWindowCoords(0, initx, inity))
	{
		reg.Set (DEF_WINDOW_X, REG_WINDOW_X);
		reg.Set (DEF_WINDOW_Y, REG_WINDOW_Y);
		initx = DEF_WINDOW_X;
		inity = DEF_WINDOW_Y;
	}

	// Restore last Window Poser State.
	SetWindowPoserOnOff ((REGV_WINDOWPOSER == 1));
	
	// Reset Internals
	apd.Reset();
	
	// Create the Main Window
	NewWindowClass (WNDCLASS, &_ext_ProcView_WndProc, myinst, 0, 0, L_IDI_PROCVIEW, (LPCTSTR) IDR_PROCVIEW, false);
	mwnd = CreateWindowEx (WS_EX_APPWINDOW | WS_EX_CONTROLPARENT, 
							WNDCLASS, CAPTION, 
							WS_CAPTION | WS_SIZEBOX | WS_CLIPCHILDREN | 
							WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, 
							initx, inity, DEF_WINDOW_WIDTH, DEF_WINDOW_HEIGHT, 
							0, 0, myinst, 0);
	SetWindowLongPtr (mwnd, GWLP_USERDATA, (LONG_PTR) this);
	
	// Add the TreeView Control
	htreeview = CreateWindowEx (WS_EX_CLIENTEDGE, WC_TREEVIEW, NULL, 
							WS_VISIBLE | WS_CHILD | WS_TABSTOP | 
							TVS_HASLINES | TVS_HASBUTTONS | TVS_SHOWSELALWAYS, 
							0, 0, 0, 0, mwnd, 0, myinst, 0);
	tvSetIndent (htreeview, 20);
	
	// Create Image List
	ilist = ImageList_Create (16, 16, ILC_MASK | ILC_COLOR16, 0, 0);
	tvSetImageList (htreeview, ilist);
	
	// ------------------------------------
	// (!) All controls are now initialized
	// ------------------------------------
	ctrlinited = true;

	// Add the Systray Icon
	sysicon.SetData (L_IDI_PROCVIEW, TRAY_CAPTION, mwnd, 1);
	sysicon.Show();
	SetTimer (mwnd, SYSICON_TIMER_ID, SYSICON_TIMER_INTERVAL, NULL);
	
	// Init Success
}

ProcView::~ProcView()
{
	// Stop Timers
	KillTimer (mwnd, SYSICON_TIMER_ID);

	// Hide & Unload Controls
	Window_Hide();
	if (ctrlinited)
	{
		tvClear (htreeview);
		ImageList_RemoveAll (ilist);
		ImageList_Destroy (ilist);
		DestroyWindow (htreeview);

		// Inidicate that all controls have been unloaded
		ctrlinited = false;
	}
	
	// Terminate
	DestroyWindow (mwnd);
	UnregisterClass (WNDCLASS, myinst);
	
	// Remove Systray Icon
	sysicon.Hide();

	// Free Resources
	FreeResources();

	// Release WinAPI Imports.
	DYN_WINAPI_Wow64DisableFsRedirection = 0;
	DYN_WINAPI_Wow64RevertFsRedirection = 0;
}

void ProcView::Window_Show()
{
	// Start Updating
	SetTimer (mwnd, PROCWND_TIMER_ID, PROCWND_TIMER_INTERVAL, NULL);
	SendMessage (mwnd, WM_TIMER, PROCWND_TIMER_ID, 0);					// Forces a first refresh
	
	// Show Window
	ShowWindow (mwnd, SW_SHOW);
	if (StyleExists(mwnd, WS_MINIMIZE))
	{
		ShowWindow (mwnd, SW_RESTORE);
	}
	SetForegroundWindowEx (mwnd);
}

void ProcView::Window_Hide()
{
	// Stop Updating
	KillTimer (mwnd, 0);
	
	// Hide Window
	SendMessage (mwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
	ShowWindow (mwnd, SW_HIDE);
}


//////////////////////////////////
// Class: Resource Management	//
//////////////////////////////////
void ProcView::LoadResources ()
{
	L_IDI_CLOSE = LoadIconRes (myinst, IDI_CLOSE, 16, 16);
	L_IDI_EXECUTE = LoadIconRes (myinst, IDI_EXECUTE, 16, 16);
	L_IDI_EXPLORER = LoadIconRes (myinst, IDI_EXPLORER, 16, 16);
	L_IDI_MAX = LoadIconRes (myinst, IDI_MAX, 16, 16);
	L_IDI_PROCVIEW = LoadIconRes (myinst, IDI_PROCVIEW, 16, 16);
	L_IDI_SOFTWAREBIG = LoadIconRes (myinst, IDI_SOFTWAREBIG, 32, 32);
	L_IDI_UNKNOWN = LoadIconRes (myinst, IDI_UNKNOWN, 16, 16);
	L_IDI_CHECK = LoadIconRes (myinst, IDI_CHECK, 16, 16);
	L_IDI_PAPER = LoadIconRes (myinst, IDI_PAPER, 16, 16);
}

void ProcView::FreeResources ()
{
	DestroyIcon (L_IDI_CLOSE);
	DestroyIcon (L_IDI_EXECUTE);
	DestroyIcon (L_IDI_EXPLORER);
	DestroyIcon (L_IDI_MAX);
	DestroyIcon (L_IDI_PROCVIEW);
	DestroyIcon (L_IDI_SOFTWAREBIG);
	DestroyIcon (L_IDI_UNKNOWN);
	DestroyIcon (L_IDI_CHECK);
	DestroyIcon (L_IDI_PAPER);
}


//////////////////////
// Window Process	//
//////////////////////
LRESULT CALLBACK ProcView::_ext_ProcView_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	ProcView* class_ptr = (ProcView*) GetWindowLongPtr (hwnd, GWLP_USERDATA);
	if (class_ptr != 0)
	{
		return class_ptr->WndProc (hwnd, msg, wparam, lparam);
	}
	return DefWindowProc (hwnd, msg, wparam, lparam);
}

LRESULT CALLBACK ProcView::WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_SIZING)
	{
		// Keep minimum size
		RECT* rc = (RECT*) lparam;
		if ((rc->right - rc->left) < WINDOW_MIN_WIDTH) { rc->right = rc->left + WINDOW_MIN_WIDTH; };
		if ((rc->bottom - rc->top) < WINDOW_MIN_HEIGHT) { rc->bottom = rc->top + WINDOW_MIN_HEIGHT; };
	}
	else if (msg == WM_SIZE)
	{
		// Adjust the TreeView Control's size
		RECT rc;
		GetClientRect (mwnd, &rc);
		SetWindowPos (htreeview, 0, 0, 0, rc.right, rc.bottom, SWP_NOMOVE);
	}
	else if (msg == WM_TIMER)
	{
		if (wparam == SYSICON_TIMER_ID)
		{
			sysicon.Show();
		}
		else if (wparam == PROCWND_TIMER_ID)
		{
			static bool lock = false;

			// Avoid Loopback
			if (!lock)
			{
				lock = true;

				{
					EnumApps ea;

					// Detect if any windows changed
					apd.Update (wmaText);
					if (apd.AppsChanged(&ea))
					{
						EnumProcs ep;
						TVINS tvi;
						HTREEITEM node_win, node_proc, subwindow, previtem;
						HICON exeicon;
						long idx = 0, proc_counter = 0;			// Parameter ^= Cache Entry
						long windowcount;
						long pi, ai, pu, au;
						bool waslastnode;
						string exepath, vpath, vfile, vparams;
						
						// Save previous selection
						HWND prevhwnd;
						DWORD prevprocid;
						previtem = tvSelectedItem(htreeview);
						if (previtem != 0)
						{
							TVI data;
							tvGetItem (htreeview, previtem, TVIF_PARAM, &data);
							if ((data.lParam > 0) && (data.lParam <= pvtc.Ubound()))
							{
								prevhwnd = pvtc[(long) data.lParam].hwnd;
								prevprocid = pvtc[(long) data.lParam].procid;
							}
							else
							{
								// Index Invalid -> No refind actions
								previtem = 0;
							}
						}
						
						// Prepare TreeView & ImageList & Cache
						pvtc.Clear();
						tvClear (htreeview);
						ImageList_RemoveAll (ilist);
						
						// Create the Main Node
						ImageList_AddIcon (ilist, L_IDI_PROCVIEW);		// Index = 0
						ImageList_AddIcon (ilist, L_IDI_MAX);			// Index = 1
						tvi.item.iImage = 0; tvi.item.iSelectedImage = 0;
						
						tvi.item.lParam = -1;
						node_win = tvAdd (htreeview, TVI_ROOT, TVI_LAST, 1, "Windows", 0, 
											&tvi, TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE);
						
						// Parse Process List
						ep.Enum();
						pu = ep.Ubound();
						au = ea.Ubound();
						// for (pi = 1; pi <= pu; pi++)
						for (pi = pu; pi >= 1; pi--)
						{
							// Only display processes that do not belong to the Windows Kernel/System Layer
							/*
							if (ExistsInList(ep.exename(pi), os_filter_kernel))
							{
								continue;
							}
							else if (ExistsInList(ep.exename(pi), os_filter_system) && 
								(UCase(ep.exename(pi)) != "EXPLORER.EXE"))
							{
								continue;
							};
							*/

							// Take out our own Application Process (ModuleName & ProcID must match!)
							if ((UCase(ep.exename(pi)) == UCase(my_exefullfn)) && (ep.procid(pi) == my_procid)) { continue; };

							// Add to Cache
							idx++;
							pvtc.Redim (idx);
							pvtc[idx].hwnd = 0;					// (!) This is a process
							pvtc[idx].procid = ep.procid(pi);
							
							// Add corresponding Icon to Imagelist
							exepath = ep.exepath(pi); 
							ShellExResolve (CSR(Chr(34), exepath, Chr(34)), &vpath, &vfile, &vparams);
							exeicon = GetFileIcon(CSR(vpath, "\\", vfile), false);
							if (exeicon != 0)
							{
								if (ImageList_AddIcon(ilist, exeicon) != -1)
								{
									DestroyIcon (exeicon);
								}
								else
								{
									ImageList_AddIcon (ilist, L_IDI_UNKNOWN);
								}
							}
							else
							{
								// Could not get exe image icon
								ImageList_AddIcon (ilist, L_IDI_UNKNOWN);
							}
							
							// Add to TreeView
							proc_counter++;
							tvi.item.iImage = proc_counter+1;
							tvi.item.iSelectedImage = proc_counter+1;
							
							tvi.item.lParam = idx;
							node_proc = tvAdd (htreeview, node_win, TVI_LAST, 1, ep.exename(pi), 0, 
													&tvi, TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE);
							
							// Was it last selected? [-I-]
							waslastnode = false;
							if (previtem != 0)
							{
								if ((prevhwnd == 0) && (pvtc[idx].procid == prevprocid))
								{
									tvSelect (htreeview, node_proc, TVGN_CARET);
									waslastnode = true;
								}
							}
							
							// Find associated child windows
							windowcount = 0;
							for (ai = 1; ai <= au; ai++)
							{
								if (ep.procid(pi) == ea.procid(ai))
								{
									// Add to Cache
									windowcount++;
									idx++;
									pvtc.Redim (idx);
									pvtc[idx].hwnd = ea.hwnd(ai);
									pvtc[idx].procid = ea.procid(ai);
									
									// Add to TreeView
									tvi.item.lParam = idx;
									tvi.item.iImage = 1;
									tvi.item.iSelectedImage = 1;
									subwindow = tvAdd (htreeview, node_proc, TVI_SORT, 0, ea.name(ai), 0, 
														&tvi, TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE);
									
									// Was it last selected? [-II-]
									if (previtem != 0)
									{
										if ((pvtc[idx].hwnd == prevhwnd) && (pvtc[idx].procid == prevprocid))
										{
											tvExpand (htreeview, node_proc, TVE_EXPAND);
											tvSelect (htreeview, subwindow, TVGN_CARET);
										}
									}
								}
							}
							
							// If no windows had been found, disable the child feature
							if (windowcount == 0)
							{
								TVI cpn;
								cpn.cChildren = 0;
								tvSetItem (htreeview, node_proc, TVIF_CHILDREN, &cpn);
							}
							else
							{
								if (waslastnode)
								{
									tvExpand (htreeview, node_proc, TVE_EXPAND);
								}
							}
						}
						tvExpand (htreeview, node_win, TVE_EXPAND);
					}
				}

				// Unlock
				lock = false;
			}
		}
	}
	else if (msg == WM_COMMAND)
	{
		long item = LOWORD(wparam);
		if (item == ID_SENDCLOSE)
		{
			HTREEITEM curitem = tvSelectedItem(htreeview);
			TVI ci;
			string buf; buf.resize (512);
			long idx, u = pvtc.Ubound();
			
			if (curitem != 0)
			{
				ci.cchTextMax = xlen(buf);
				ci.pszText = &buf[0];
				tvGetItem (htreeview, curitem, TVIF_PARAM | TVIF_TEXT, &ci);
				idx = (long) ci.lParam;
				
				if ((idx > 0) && (idx <= u))
				{
					HWND hwnd = pvtc[idx].hwnd;
					DWORD procid = pvtc[idx].procid;
					if (hwnd != 0)
					{
						if (InfoBox(mwnd, LANG_EN ? "Do you really want to close this window by \"notify\"?" : 
													"Möchten Sie diesem Fenster wirklich ein Schließungssignal senden?", 
														buf, MB_YESNO | MB_ICONQUESTION) == IDYES)
						{
							ExitAppWindow (hwnd);
						}
					}
					else
					{
						InfoBox (mwnd, LANG_EN ? "Error:\nYou cannot close a process by sending the close message. Use \"Terminate Process\" instead." : 
												"Fehler:\nSie können keinen Prozess durch das Senden eines Schließungssignals beenden. Benutzen Sie stattdessen \"Terminate Process\".", 
													buf, MB_OK | MB_ICONSTOP);
					}
				}
			}
		}
		else if (item == ID_TERMINATE)
		{
			HTREEITEM curitem = tvSelectedItem(htreeview);
			TVI ci;
			string buf; buf.resize (512);
			long idx, u = pvtc.Ubound();
			
			if (curitem != 0)
			{
			
				ci.cchTextMax = xlen(buf);
				ci.pszText = &buf[0];
				tvGetItem (htreeview, curitem, TVIF_PARAM | TVIF_TEXT, &ci);
				idx = (long) ci.lParam;
				
				if ((idx > 0) && (idx <= u))
				{
					HWND hwnd = pvtc[idx].hwnd;
					DWORD procid = pvtc[idx].procid;
					string exename;
					
					if (hwnd != 0)
					{
						// This is a child window, so find out the Parent Process
						ci.cchTextMax = xlen(buf);
						ci.pszText = &buf[0];
						tvGetItem (htreeview, tvGetParent(htreeview, curitem), TVIF_PARAM | TVIF_TEXT, &ci);
					}
					exename = &buf[0];
					
					if (InfoBox(mwnd, LANG_EN ? "Do you really want to terminate this process?" :
													"Möchten Sie diesen Prozess wirklich beenden?", 
											exename, MB_YESNO | MB_ICONQUESTION) == IDYES)
					{
						KillProcessCheck (procid);
					}
				}
			}
		}
		else if (item == ID_SHX)
		{
			ShellExDialog();
		}
		else if (item == ID_HIDEWINDOW)
		{
			Window_Hide();
		}
	}
	else if (msg == WM_CLOSE)
	{
		Window_Hide();

		// Do not invoke WM_DESTROY.
		return false;			
	}
	else if (msg == WM_MOVE)
	{
		if (!StyleExists(mwnd, WS_MINIMIZE))
		{
			RECT newrc;
			GetWindowRect (mwnd, &newrc);
			
			// Save if the new coords are valid
			if (CheckWindowCoords(mwnd, newrc.left, newrc.top))
			{
				reg.Set (newrc.left, REG_WINDOW_X);
				reg.Set (newrc.top, REG_WINDOW_Y);
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
			// Session is being SURELY ended after return (this is the MainThread).
			TerminateVKernel (true);
			return false;
		}
	}
	else if (msg == WM_LBUTTONUP)
	{
		// Icon Process
		if (wparam == 1)
		{
			// First Iccn
			static bool lock = false;
			if (!lock)
			{
				// Lock
				lock = true;

				// Act
				if (lparam == WM_RBUTTONUP)
				{
					StructArray <PIDCACHE> pidc;
					EnumProcs ep;
					MegaPopup pm(17, 7);
					MegaPopupFade fade;
					string vpath, vfile, vparams, strIconSrcFullFN;
					HICON image_icon;
					PVOID pOldState = NULL;

					string os64_system32_dir;
					long BaseKernel, BaseSystem, BaseOther;
					long* CurrentList_Base;
					long u, i;
					

					// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
					// 64-Bit Windows only: [!] Disable windows file system redirection.
					// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
					if (is_64_bit_os && (DYN_WINAPI_Wow64DisableFsRedirection != 0))
					{
						DYN_WINAPI_Wow64DisableFsRedirection (&pOldState);
						os64_system32_dir = GetSystemDir32();
					}
		

					//
					// Create Standard Menu
					//
					pm.Add (1, LANG_EN ? "Process Overview" : "Prozessübersicht", L_IDI_PROCVIEW, 0);
					pm.Add (2, LANG_EN ? "Start program" : "Programm ausführen", L_IDI_EXECUTE, 0);
					pm.Add (3, LANG_EN ? "Extras" : "Extras", L_IDI_PAPER, MPM_HILITEBOX | MPM_FADED);
					pm.Add (4, LANG_EN ? "Setup WinSuite" : "WinSuite anpassen", L_IDI_REGTOOL16, 0);
					
					if (!REGV_SYSLOG_ENABLE)
					{
						pm.Add (5, LANG_EN ? "Exit WinSuite" : "WinSuite beenden", L_IDI_CLOSE, 0);
					}
					else
					{
						// Only allow exiting this application with syslog active when compiled in DEBUG MODE.
						#ifdef _DEBUG
							pm.Add (5, LANG_EN ? "Exit WinSuite" : "WinSuite beenden", L_IDI_CLOSE, 0);
						#endif
					}


					//
					// Insert Category Separators
					//
					pm.Add (StartKernel, "", 0, 0);			
					pm.Add (StartSystem, "", 0, 0);
					pm.Add (StartOther, "", 0, 0);

					// Create Categories, 
					BaseKernel = StartKernel + 1;
					BaseSystem = StartSystem + 1;
					BaseOther = StartOther + 1;

					// Enum & Assume all under "Other Category" and reserve enough memory
					ep.Enum ();	
					u = ep.Ubound();
					pidc.Redim (BaseOther + u);

					// Zero out all HICON references so we can destroy Icon Handles after the MPM Popup
					for (long j = 1; j <= (BaseOther + u); j++)
					{
						pidc[j].hicon = 0;
					}
					
					// Fill in the Data.
					for (i = 1; i <= u; i++)
					{
						// Hide modules from popup menu list.
						if ((UCase(ep.exename(i)) == UCase(my_exefullfn)) && (ep.procid(i) == my_procid))
						{
							// (!) Take out our own Application Process (ModuleName & ProcID must match!)
							// Continue with next FOR-Element.
							continue;
						}
						else if (FileExists(CSR(os_system_dir, "\\", ep.exename(i))) == true)
						{
							// (!) Do not display components listed in windows system root.
							// Continue with next FOR-Element.
							// InfoBox (0, CSR(os_system_dir, "\\", ep.exename(i)), "Test - windows_root", MB_OK);
							continue;
						}
						
						// For 64-bit Systems only.
						if (is_64_bit_os) 
						{
							if (FileExists(CSR(os64_system32_dir, "\\", ep.exename(i))) == true)
							{
								// Continue with next FOR-Element.
								// InfoBox (0, CSR(os64_system32_dir, "\\", ep.exename(i)), "Test", MB_OK);
								continue;
							}
						}
						
						// Realtek Audio Control Panel
						if (REGV_REALTEK_AUDIO_PATH.length() > 0)
						{
							// Realtek Audio Control Panel installed.
							if (FileExists(CSR(REGV_REALTEK_AUDIO_PATH, "\\", ep.exename(i))) == true)
							{
								// Continue with next FOR-Element.
								continue;
							}
						}

						// Variable default: "Icon load unsuccessful."
						image_icon = 0;

						// Load EXE Image Icon
						ShellExResolve(CSR(Chr(34), ep.exepath(i), Chr(34)), &vpath, &vfile, &vparams);
						if (UCase(vpath) == UCase(vfile))
						{
							// The EXE image path could not be retrieved.
							// This causes "vpath == vfile".
							// InfoBox (0, CSR(CSR("vpath = ", vpath, " / vfile = / ", vfile), "/ ep.exepath(i) = ", ep.exepath(i)), "Test - image_icon", MB_OK);
						}
						else
						{
							strIconSrcFullFN = CSR(vpath, "\\", vfile);
							image_icon = GetFileIcon(strIconSrcFullFN, false);		// Do NOT use continue after this line because DestroyIcon would be missing causing memory leak.

							// For testing purposes only.
							// InfoBox (0, CSR("strIconSrcFullFN = ", strIconSrcFullFN, " / ep.exepath(i) = ", ep.exepath(i)), "Test - image_icon", MB_OK);
						}

						// Did we retrieve a valid icon handle of the icon loaded above?
						if (image_icon == 0)
						{
							// Icon could not be loaded from file.
							image_icon = L_IDI_UNKNOWN;

							// For testing purposes only.
							// InfoBox (0, CSR("strIconSrcFullFN = ", strIconSrcFullFN, " / ep.exepath(i) = ", ep.exepath(i)), "Test - image_icon", MB_OK);
						}

						// Select appropriate List from "Kernel", "System" or "Other" for the enumed process
						CurrentList_Base = (ExistsInList(ep.exename(i), os_filter_kernel)) ? &BaseKernel : 
												((ExistsInList(ep.exename(i), os_filter_system)) ? &BaseSystem : &BaseOther);

						// Add Process to appropriate List.
						pm.Add (*CurrentList_Base, ep.exename(i), image_icon, 0);
						pidc[*CurrentList_Base].procid = ep.procid(i);
						pidc[*CurrentList_Base].exename = ep.exename(i);
						pidc[*CurrentList_Base].hicon = image_icon;						// Icon needs to be destroyed!!!
						(*CurrentList_Base)++;
					}


					// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
					// 64-Bit Windows only: [!] Revert windows file system redirection to original state.
					// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
					if (is_64_bit_os && (DYN_WINAPI_Wow64RevertFsRedirection != 0))
					{
						DYN_WINAPI_Wow64RevertFsRedirection (&pOldState);
					}

					
					// Show Up the Menu
					POINT pt;
					long hv;
					GetCursorPos (&pt);
					fade.callback = &_ext_ProcView_TrayMenuFader;
					fade.data = (long*) this;
					hv = pm.Show (pt.x, pt.y, MPM_RIGHTBOTTOM, MPM_NOSCROLL, &fade);

					// Destroy all Prepared Icons (except L_IDI_UNKNOWN!)
					u = pidc.Ubound();
					for (long k = 1; k <= u; k++)
					{
						if ((pidc[k].hicon != 0) && (pidc[k].hicon != L_IDI_UNKNOWN))
						{
							DestroyIcon (pidc[k].hicon);
						}
					}
					
					// Act
					if (hv == 1)
					{
						// Show ProcView's Main Window.
						Window_Show();
					}
					else if (hv == 2)
					{
						// Open Start Application Dialog.
						ShellExDialog();
					}
					else if (hv == 4)
					{
						// Run WinSuite Setup Application.
						ShellExecute (0, CSR(Chr(34), GetAppExe(), Chr(34))); 
					}
					else if (hv == 5)
					{
						// Exit WinSuite Application.
						if (InfoBox(0, LANG_EN ? "Are you sure you really want to exit WinSuite?" :
													"Sind Sie sicher, dass Sie WinSuite beenden möchten?", 
													"WinSuite", MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
						{
							TerminateVKernel (false);
						}
					}
					else
					{
						string msg;
						long res;
						if ((hv >= StartKernel) && (hv < StartSystem))
						{
							msg = LANG_EN ? "Do you really want to terminate this main process of Windows?\n\nNote: An instant windows halt could be the result!" : "Möchten Sie diesen Prozess den Windows-Kernsystems wirklich beenden?\n\nAchtung: Systemstillstand kann die Folge sein!";
							res = InfoBox (0, msg, pidc[hv].exename, MB_YESNO | MB_ICONERROR | MB_SYSTEMMODAL);
							if (res == IDYES)
							{
								KillProcessCheck (pidc[hv].procid);
							}
						}
						else if ((hv >= StartSystem) && (hv < StartOther))
						{
							msg = LANG_EN ? "Do you really want to terminate this process?\n\nNote: This process belongs to Windows and could be needed for Windows to work properly!" : "Möchten Sie diesen Prozess wirklich beenden?\n\nAchtung: Dieser Prozess gehört zum Windows-System und wird in der Regel für den ordnungsgemäßen Betrieb des Systems benötigt!";
							res = InfoBox (0, msg, pidc[hv].exename, MB_YESNO | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
							if (res == IDYES)
							{
								KillProcessCheck (pidc[hv].procid);
							}
						}
						else if (hv >= StartOther)
						{
							msg = LANG_EN ? "Do you really want to terminate this process?" : "Möchten Sie diesen Prozess wirklich beenden?";
							res = InfoBox (0, msg, pidc[hv].exename, MB_YESNO | MB_ICONQUESTION | MB_SYSTEMMODAL);
							if (res == IDYES)
							{
								KillProcessCheck (pidc[hv].procid);
							}
						}
					}
				}
				else if (lparam == WM_MBUTTONUP)
				{
					ShellExDialog();
				}
				else if (lparam == WM_LBUTTONDBLCLK)
				{
					// Toggle Window Visibility
					if (!StyleExists(mwnd, WS_VISIBLE))
					{
						Window_Show();
					}
					else
					{
						Window_Hide();
					}
				}

				// Unlock
				lock = false;
			}
		}
	}
	return DefWindowProc (hwnd, msg, wparam, lparam);
}

long ProcView::_ext_ProcView_TrayMenuFader (MegaPopup* par, MegaPopup* chi, long entry, long x, long y, long* data)
{
	ProcView* i_procview = (ProcView*) data;
	long hv = 0;

	// Check if class self-reference is valid.
	if (i_procview != 0)
	{
		// Create and show the child menu.
		chi->Add (1, LANG_EN ? "Window Poser" : "Fensterwächter", 
						(HICON) ((LONG_PTR) i_procview->L_IDI_CHECK * i_procview->REGV_WINDOWPOSER), 
						MPM_HILITEBOX 
							);
		chi->Add (2, LANG_EN ? "Date Print (SCROLL LOCK)" : "Datumsausgabe (ROLLEN)", 
						(HICON) ((LONG_PTR) i_procview->L_IDI_CHECK * i_procview->REGV_DATEPRINT),
						MPM_HILITEBOX 
							);
		chi->Add (3, "", 0, 0);
		chi->Add (4, LANG_EN ? "Recall Explorer" : "Explorer neustarten", i_procview->L_IDI_EXPLORER, 0);
	
		hv = chi->Show (x, y, MPM_TOPLEFT, MPM_HSCROLL);
	
		// Act
		if (hv == 1)
		{
			// Window Poser: Toggle Checkbox
			i_procview->SetWindowPoserOnOff (!i_procview->REGV_WINDOWPOSER);
		}
		else if (hv == 2)
		{
			// Toggle Component State
			// Note: Component State is externally acquired via public class interface by "VK2Wnd_WndProc" (Boot.cpp).
			i_procview->REGV_DATEPRINT = ((i_procview->REGV_DATEPRINT == 1) ? 0 : 1);
			i_procview->reg.Set (i_procview->REGV_DATEPRINT, i_procview->REG_DATEPRINT);
		}
		else if (hv == 4)
		{
			long lngTipShown = DEF_HOTKEY_TIP;

			// Terminate Explorer Process.
			TerminateExplorer();

			// Wait a bit before attempting to restart explorer.exe.
			Sleep (250);
			ShellExecute (0, CSR(GetWinDir(), "\\Explorer.exe"));

			// Show F12 hotkey tip once.
			i_procview->reg.Get (&lngTipShown, i_procview->REG_HOTKEY_TIP);
			if (lngTipShown == 0)
			{
				InfoBox (0, LANG_EN ? "This option is also available pressing the WIN + F12 key simultaneously." :
										"Sie können diesen Befehl auch ausführen, wenn Sie die Tastenkombination WIN + F12 benutzen.", 
										"WinSuite", MB_OK);
				i_procview->reg.Set (1, i_procview->REG_HOTKEY_TIP);
			}
		}
	}
	
	return hv;
}



//////////////////////////////
// Helper Functions			//
//////////////////////////////
bool ProcView::ExistsInList (string search, string list)
{
	//
	// For testing purposes only.
	//
	// InfoBox (0, CSR(list, "-", UCase(CSR("*", search, "*")), "-", CStr(InStr(UCase(list), UCase(CSR("*", search, "*"))))), "Test", MB_OK);
	//
	return (InStr(UCase(list), UCase(CSR("*", search, "*"))) != 0);
}

void ProcView::KillProcessCheck (DWORD procid)
{
	if (!KillProcess(procid, true))
	{
		InfoBox (0, LANG_EN ? "Could not kill process. Maybe your level of security elevation is not sufficient for this action." : 
								"Der Prozess konnte nicht beendet werden. Eventuell haben Sie nicht die ausreichende Berechtigung, um den Prozess beenden zu können."
								, TRAY_CAPTION, MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
	}
}

void ProcView::SetWindowPoserOnOff (bool bEnable)
{
	// State changed.
	if (bEnable)
	{
		// Turn Window Poser on.
		REGV_WINDOWPOSER = 1;
		mod_windowposer = new WindowPoser();
	}
	else
	{
		// Turn Window Poser off.
		if (mod_windowposer != 0)		
		{ 
			delete mod_windowposer; 
			mod_windowposer = 0; 
		}
		REGV_WINDOWPOSER = 0;
	}

	// Save new state in registry settings.
	reg.Set (REGV_WINDOWPOSER, REG_WINDOWPOSER);
}

bool ProcView::IsDatePrintEnabled ()
{
	return (REGV_DATEPRINT == 1);
}

