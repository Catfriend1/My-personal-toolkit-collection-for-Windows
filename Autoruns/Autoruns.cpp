// Autoruns.cpp: Autostart Manager's Main Module
//
// Thread-Safe: NO
//


//////////////////
// Includes		//
//////////////////
#include "stdafx.h"
#include "../resource.h"

#include "../Modules/_modules.h"

#include "../Boot.h"			// <= Icons, myinst
#include "Autoruns.h"


//////////////
// Consts	//
//////////////
const char Autoruns::CAPTION[]					= "Autostart Manager";
const char Autoruns::TRAY_CAPTION[]				= "Autostart Manager";
const char Autoruns::WNDCLASS_EDITOR[]			= "*WS_C32_Autoruns_Editor";

const long Autoruns::GUARD_TIMER_INTERVAL		= 10000;						// [1000]

const char Autoruns::REG_WINDOW_X[]				= "Autoruns_WindowX";
const char Autoruns::REG_WINDOW_Y[]				= "Autoruns_WindowY";

const long Autoruns::EDITOR_DEF_WND_X			= 190;
const long Autoruns::EDITOR_DEF_WND_Y			= 130;
const long Autoruns::EDITOR_WND_WIDTH			= 605;							// [605]
const long Autoruns::EDITOR_WND_HEIGHT			= 422;							// [422]

const long Autoruns::MYTV_INVALID				= 0;							// [0]
const long Autoruns::MYTV_GLOBAL				= 1;							// [1]
const long Autoruns::MYTV_LOCAL					= 2;							// [2]

const char Autoruns::REG_AUTORUN_BASE[]			= "Software\\Microsoft\\Windows\\CurrentVersion\\";
const char Autoruns::REG_WINLOGON[]				= "Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";


//////////////////////
// Consts: Guard	//
//////////////////////
const char Autoruns::GUARD_CAPTION[]			= 
													#ifdef _LANG_EN 
														"Autostart Manager - Guard Warning";
													#else 
														"Autostart Manager - Wächterwarnung";
													#endif;

const long Autoruns::ATTRIBUTE_INI_LOCKED		= FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
const long Autoruns::ATTRIBUTE_INI_UNLOCKED		= FILE_ATTRIBUTE_ARCHIVE;

const long Autoruns::DIA_DEFWIDTH				= 417;							// [417]
const long Autoruns::DIA_DEFHEIGHT				= 236;							// [236]
const long Autoruns::DIA_DETAILMODE				= 365;							// [365]

const long Autoruns::DIA_ACCEPT					= 7;
const long Autoruns::DIA_RESTORE				= 8;


//////////////////////
// Consts: Refind	//
//////////////////////
const int Autoruns::RF_OK						= 0;
const int Autoruns::RF_ADDED					= 1;
const int Autoruns::RF_CHANGED					= 2;
const int Autoruns::RF_DELETED					= 3;


//////////////////////////////////////////
// Class: Autoruns - Init / Terminate	//
//////////////////////////////////////////
Autoruns::Autoruns()
{
	long initx, inity;
	string SHF_LOCAL_APPDATA;

	// Init RAM
	ctrlinited = false;
	guard_inited = false;
	arfile = INVALID_HANDLE_VALUE;

	// Init Common Info Variables
	readable_curuser = (xlen(GetCurrentUser()) == 0) ? "Standard" : GetCurrentUser();
	reg.SetRootKey (HKEY_CURRENT_USER, "Software\\WinSuite\\Autoruns");

	// Get Shell Folder and build INI File FullFN
	SHF_LOCAL_APPDATA = GetShellFolder(CSIDL_LOCAL_APPDATA);
	if (xlen(SHF_LOCAL_APPDATA) == 0)
	{
		DebugOut ("Autostart Manager: Error retrieving shell folder path for autostart analytics file.");
		return;
	}
	ini_file = CSR(SHF_LOCAL_APPDATA, "\\WinSuite_AutostartManager.ini");

	// Init Windows Autostart keys
	regEnsureKeyExist (HKLM, CSR(REG_AUTORUN_BASE, "Run"));
	regEnsureKeyExist (HKLM, CSR(REG_AUTORUN_BASE, "RunOnce"));
	regEnsureKeyExist (HKLM, CSR(REG_AUTORUN_BASE, "RunServices"));
	regEnsureKeyExist (HKLM, CSR(REG_AUTORUN_BASE, "RunServicesOnce"));

	regEnsureKeyExist (HKCU, CSR(REG_AUTORUN_BASE, "Run"));
	regEnsureKeyExist (HKCU, CSR(REG_AUTORUN_BASE, "RunOnce"));
	
	// Init my own RegKey
	reg.Init (REG_WINDOW_X, EDITOR_DEF_WND_X);
	reg.Init (REG_WINDOW_Y, EDITOR_DEF_WND_Y);
	
	// Verify Window Coordinates
	reg.Get (&initx, REG_WINDOW_X);
	reg.Get (&inity, REG_WINDOW_Y);
	if (!CheckWindowCoords(0, initx, inity))
	{
		reg.Set (EDITOR_DEF_WND_X, REG_WINDOW_X);
		reg.Set (EDITOR_DEF_WND_Y, REG_WINDOW_Y);
		initx = EDITOR_DEF_WND_X;
		inity = EDITOR_DEF_WND_Y;
	}
	
	// Create the Editor Window
	NewWindowClass (WNDCLASS_EDITOR, &_ext_Autoruns_Editor_WndProc, myinst, 0, L_IDI_REGTOOL, L_IDI_REGTOOL16, NULL, true);
	ewnd = CreateWindowEx (WS_EX_APPWINDOW | WS_EX_CONTROLPARENT, WNDCLASS_EDITOR, CAPTION, 
							WS_CAPTION | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | 
							WS_SYSMENU | WS_MINIMIZEBOX, 
							initx, inity, EDITOR_WND_WIDTH, EDITOR_WND_HEIGHT, 
							0, 0, myinst, 0);
	SetWindowLongPtr (ewnd, GWLP_USERDATA, (LONG_PTR) this);
	edc = ewnd_mem.Create (ewnd, EDITOR_WND_WIDTH, EDITOR_WND_HEIGHT);
	SetBkMode (edc, TRANSPARENT);
	
	// Draw the Group Boxes
	DrawGroupBox (edc, 16, 16, 566, 284, LANG_EN ? "Registry Autoruns" : "Autostartprogramme");
	DrawGroupBox (edc, 16, 312, 566, 75, "Winlogon Info");
	
	// Add the TreeView Control
	ar_tvc = CreateWindowEx (WS_EX_CLIENTEDGE, WC_TREEVIEW, NULL, 
							WS_VISIBLE | WS_CHILD | WS_TABSTOP | 
							TVS_HASLINES | TVS_HASBUTTONS | TVS_SHOWSELALWAYS, 
							35, 61, 136, 213, ewnd, 0, myinst, 0);
	tvSetIndent (ar_tvc, 32);
	
	// Add the two ListViews
	lvname = CreateWindowEx (WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL, 
								WS_VISIBLE | WS_CHILD | 
								LVS_EDITLABELS | LVS_REPORT | WS_TABSTOP | 
								LVS_SHOWSELALWAYS | LVS_SINGLESEL, 
								172, 61, 122, 213, ewnd, 0, myinst, 0);
	
	lvstr = CreateWindowEx (WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL, 
								WS_VISIBLE | WS_CHILD | 
								LVS_EDITLABELS | LVS_REPORT | WS_TABSTOP | 
								LVS_SHOWSELALWAYS | LVS_SINGLESEL, 
								294, 61, 273, 213, ewnd, 0, myinst, 0);
	
	// Add "Add", "Delete" and "Refresh" Button
	cmdupdate = CreateButton (ewnd, LANG_EN ? "&Refresh" : "Aktualisieren", 302, 28, 80, 23, myinst);
	cmdadd = CreateButton (ewnd, LANG_EN ? "&Add Entry" : "Neuer Eintrag", 386, 28, 80, 23, myinst);
	cmddel = CreateButton (ewnd, LANG_EN ? "&Delete Entry" : "Eintrag löschen", 470, 28, 80, 23, myinst);
	ctlSetFont (cmdupdate, sfnt.MSSANS);
	ctlSetFont (cmdadd, sfnt.MSSANS);
	ctlSetFont (cmddel, sfnt.MSSANS);

	// Add Checkbox for "Assisted Autorun"
	chkassis = CreateWindowEx (0, "BUTTON", LANG_EN ? "Assisted Autorun" : "Autostart Assistent benutzen", 
								WS_CHILD | WS_VISIBLE | WS_TABSTOP |  
								BS_AUTOCHECKBOX, 
								297, 277, 240, 17, ewnd, 0, myinst, 0);
	ctlSetFont (chkassis, sfnt.MSSANS);
	
	// Set the Column Headers
	lvAddColumn (lvname, 1, "Name", LVCFMT_LEFT, 118);
	lvAddColumn (lvstr, 1, "Value", LVCFMT_LEFT, 269);
	
	// Init Shell Editor
	FontOut (edc, 42, 334, "Windows Shell:", sfnt.MSSANS);
	txtshell = CreateEditbox (ewnd, "", 122, 332, 150, myinst);
	ctlSetFont (txtshell, sfnt.MSSANS);
	
	// Init WIN.INI/SYSTEM.INI Editor
	txtwinload = 0;
	txtwinrun = 0;
	
	// Add "Apply" Button
	cmdapply = CreateButton (ewnd, LANG_EN ? "Apply" : "Übernehmen", 42, 355, LANG_EN ? 80 : 90, 23, myinst);
	ctlSetFont (cmdapply, sfnt.MSSANS);
	
	// ------------------------------------
	// (!) All controls are initialized now
	// ------------------------------------
	ctrlinited = true;

	// Add the Systray Icon
	sysicon.SetData (L_IDI_REGTOOL16, CSR(Chr(187), " ", CAPTION), ewnd, 1);
	sysicon.Show();
	
	// Draw the Textlabels
	FontOut (edc, 35, 38, LANG_EN ? "Click the required key below to edit its contents:" : "Bitte gewünschten Schlüssel zur Bearbeitung wählen:", sfnt.MSSANS);
	
	// Fill the Editor's TreeView
	{
		TVINS tvi;
		HTREEITEM node_all, node_cur;
		
		// Clear the TreeView
		tvClear (ar_tvc);
		
		// Add the "All Users" Section
		tvi.item.lParam = MYTV_INVALID;
		node_all = tvAdd (ar_tvc, TVI_ROOT, TVI_LAST, 1, "All Users", 0, &tvi, TVIF_PARAM);
		
		tvi.item.lParam = MYTV_GLOBAL;
		tvSelect (ar_tvc, tvAdd(ar_tvc, node_all, TVI_LAST, 0, "Run", 0, &tvi, TVIF_PARAM));		// Set "All Users"->"Run" as Default Selection
		tvAdd (ar_tvc, node_all, TVI_LAST, 0, "RunOnce", 0, &tvi, TVIF_PARAM);
		tvExpand (ar_tvc, node_all, TVE_EXPAND);
		
		// Add the "Current User" Section
		tvi.item.lParam = MYTV_INVALID;
		node_cur = tvAdd (ar_tvc, TVI_ROOT, TVI_LAST, 1, readable_curuser, 0, &tvi, TVIF_PARAM);
		
		tvi.item.lParam = MYTV_LOCAL;
		tvAdd (ar_tvc, node_cur, TVI_LAST, 0, "Run", 0, &tvi, TVIF_PARAM);
		tvAdd (ar_tvc, node_cur, TVI_LAST, 0, "RunOnce", 0, &tvi, TVIF_PARAM);
		tvExpand (ar_tvc, node_cur, TVE_EXPAND);
	}
	
	// Update the Window Content (especially Win/System.ini)
	Editor_Refresh();
	
	// INI already exists ?
	if (!FileExists(ini_file))
	{
		HANDLE crf = CreateFile(&ini_file[0], GENERIC_WRITE, 0, 
									NULL, CREATE_NEW, ATTRIBUTE_INI_LOCKED, 0);
		if (crf != INVALID_HANDLE_VALUE)
		{
			CloseHandle (crf);
			
			// (!) WARNING: INI has been re-created
			InfoBox (0, LANG_EN ? "(!) Guard file has been recreated\n\nWARNING:\n\nThe reason for this could be another program trying to attack your Autostart List by deleting the file.\n\nBut if you are starting Autostart Manager for the first time, this message is only meant to inform you to check the following entries carefully." : 
								  "(!) Die Autostart-Überwachungsdatei wurde neu erstellt.\n\nWARNUNG:\n\nDer Grund dafür könnte ein anderes Programm sein, dass durch ein Zurücksetzen dieser Datei versucht, den Autostart-Schutz zu umgehen.\n\nWenn Sie Autostart Manager das erste Mal starten, ignorieren Sie diese Meldung und überprüfen die anschließend erscheinenden Meldungen genauestens.", 
								GUARD_CAPTION, MB_OK | MB_ICONEXCLAMATION);
		}
		else
		{
			// (!) WARNING: Unable to create INI-file
			InfoBox (0, LANG_EN ? "(!) Unable to create guard file.\n\nWARNING: Your Autostart Protection is currently disabled!" :
									"(!) Die Autostart-Überwachungsdatei konnte nicht erstellt oder geöffnet werden.\n\nWARNUNG: Die Autostart-Überwachung ist momentan nicht aktiv.", 
									GUARD_CAPTION, MB_OK | MB_ICONEXCLAMATION);
		}
	}
	
	// Cache, Lock & Initialize "Autoruns.ini"
	if (ini.Open(ini_file, true))
	{
		if (Guard_LockINI())
		{
			// Initialize Cached INI Content
			ini.AddSection (Reg2Cache(HKLM, "Run"));
			ini.AddSection (Reg2Cache(HKLM, "RunOnce"));
			ini.AddSection (Reg2Cache(HKLM, "RunServices"));
			ini.AddSection (Reg2Cache(HKLM, "RunServicesOnce"));
			ini.AddSection (Reg2Cache(HKCU, "Run"));
			ini.AddSection (Reg2Cache(HKCU, "RunOnce"));
			
			// Signal "Guard startup successful"
			guard_inited = true;

			// Start the Guard Timer
			SetTimer (ewnd, 1, GUARD_TIMER_INTERVAL, NULL);
		}
	}

	// Check if the Guard inited properly
	if (!guard_inited)
	{
		InfoBox (0, LANG_EN ? "Warning: Due to another program stopping it to function properly, the Autostart Guard is inactive." : 
								"Warnung: Die Autostartüberwachung ist momentan aufgrund des Einflusses eines Fremdprogramms nicht funktionstüchtig.", 
									GUARD_CAPTION, MB_OK | MB_ICONSTOP);
	}
	
	// Init Success
}

Autoruns::~Autoruns ()
{
	// Kill the Timer
	KillTimer (ewnd, 1);

	// Unlock INI file
	Guard_UnlockINI();
	
	// Signal Guard to Stop
	guard_inited = false;

	// Hide Editor Window
	Window_Hide();

	// Unload Controls
	if (ctrlinited)
	{
		DestroyWindow (ar_tvc);
		DestroyWindow (lvname);
		DestroyWindow (lvstr);
		DestroyWindow (cmdadd);
		DestroyWindow (cmddel);
		
		DestroyWindow (txtshell);
		if (txtwinload != 0) { DestroyWindow (txtwinload); txtwinload = 0; };
		if (txtwinrun != 0) { DestroyWindow (txtwinrun); txtwinrun = 0; };
		DestroyWindow (cmdapply);
		DestroyWindow (cmdupdate);
		DestroyWindow (chkassis);

		// All Controls Uninited
		ctrlinited = false;
	}
	
	// Terminate
	ewnd_mem.Free();
	DestroyWindow (ewnd);
	UnregisterClass (WNDCLASS_EDITOR, myinst);
	
	// Remove Systray Icon
	sysicon.Hide();
	
	// UnInit completed
}


//////////////////
// Show & Hide	//
//////////////////
void Autoruns::Window_Show()
{
	ShowWindow (ewnd, SW_SHOW);
	if (StyleExists(ewnd, WS_MINIMIZE))
	{
		ShowWindow (ewnd, SW_RESTORE);
	}
	SetForegroundWindowEx (ewnd);
}

void Autoruns::Window_Hide()
{
	SendMessage (ewnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
	ShowWindow (ewnd, SW_HIDE);
}


//////////////////////
// Window Process	//
//////////////////////
LRESULT CALLBACK Autoruns::_ext_Autoruns_Editor_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	Autoruns* class_ptr = (Autoruns*) GetWindowLongPtr (hwnd, GWLP_USERDATA);
	if (class_ptr != 0)
	{
		return class_ptr->Editor_WndProc (hwnd, msg, wparam, lparam);
	}
	return DefWindowProc (hwnd, msg, wparam, lparam);
}


LRESULT CALLBACK Autoruns::Editor_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_PAINT)
	{
		ewnd_mem.Refresh();
		return false;
	}
	else if (msg == WM_TIMER)
	{
		static bool lock = false;

		// (!) LOOPBACK PROTECTED
		if (!lock)
		{
			lock = true;
			if (guard_inited)
			{
				CheckRegSection (HKLM, "Run");
				CheckRegSection (HKLM, "RunOnce");
				CheckRegSection (HKCU, "Run");
				CheckRegSection (HKCU, "RunOnce");
			}
			lock = false;
		}

		// Refresh the Systray Icon
		sysicon.Show();
	}
	else if (msg == WM_NOTIFY)
	{
		if (ctrlinited)
		{
			NMHDR* nmhdr = (NMHDR*) lparam;
			
			// Tree-View
			// ---------
			if ((nmhdr->hwndFrom == ar_tvc) && (nmhdr->code == TVN_SELCHANGED))
			{
				Editor_RefreshEntries();
			}
			
			// ListView: NAME
			// --------------
			if ((nmhdr->hwndFrom == lvname) && (nmhdr->code == LVN_ENDLABELEDIT))
			{
				LV_DISPINFO* lvd = (LV_DISPINFO*) lparam;
				long itemidx = lvd->item.iItem;
				
				// Get Registry Section
				LRESULT regsec;
				string curnode = tvGetItemText(ar_tvc, tvSelectedItem(ar_tvc), 128, &regsec);
				HKEY mainkey = Editor_DecideHKey(regsec);
				
				if (lvd->item.pszText != 0)		// (!) AVOID NULL POINTER
				{
					if (((string) lvd->item.pszText).length() != 0)
					{
						UACRegEdit uacreg;
						string value, oldname ;

						value = lvGetItemText(lvstr, 0, itemidx);
						oldname = lvGetItemText(lvname, 0, itemidx);
						
						// Delete "curname" (to be read from lvname#itemidx)
						uacreg.DeleteValue (mainkey, CSR(REG_AUTORUN_BASE, curnode), oldname);
						
						// Create "newname" with "oldvalue" (to be read from lvstr#itemidx)
						uacreg.Write  (value, mainkey, CSR(REG_AUTORUN_BASE, curnode), lvd->item.pszText);
						
						// Commit Changes to Registry
						if (uacreg.CommitChangesToRegistry())
						{
							// Bypass the Guard if the write operation succeeded
							Guard_Direct_Delete (mainkey, curnode, oldname);
							Guard_Direct_Write (mainkey, curnode, lvd->item.pszText, value);
						}
						return true;
					}
					else
					{
						return false;	// (!) New NAME is invalid
					}
				}
			}
			else if ((nmhdr->hwndFrom == lvname) && (nmhdr->code == LVN_ITEMCHANGED))
			{
				NM_LISTVIEW* nml = (NM_LISTVIEW*) lparam;
				if ((!(nml->uOldState & LVIS_SELECTED)) && (nml->uNewState & LVIS_SELECTED))
				{
					// (!) Item is now selected
					lvSelect (lvstr, nml->iItem, true);
					lvSetItemFocus (lvstr, nml->iItem);
					Editor_EntrySelected (nml->iItem);
				}
			}
			
			// ListView: STR
			// -------------
			if ((nmhdr->hwndFrom == lvstr) && (nmhdr->code == LVN_ENDLABELEDIT))
			{
				LV_DISPINFO* lvd = (LV_DISPINFO*) lparam;
				long itemidx = lvd->item.iItem;
				
				// Get Registry Section
				LRESULT regsec;
				string curnode = tvGetItemText(ar_tvc, tvSelectedItem(ar_tvc), 128, &regsec);
				HKEY mainkey = Editor_DecideHKey(regsec);
				
				// Flush registry with new string
				if (lvd->item.pszText != 0)		// (!) AVOID NULL POINTER
				{
					UACRegEdit uacreg;
					string name;
					
					name = lvGetItemText(lvname, 0, itemidx);
					uacreg.Write (lvd->item.pszText, mainkey, CSR(REG_AUTORUN_BASE, curnode), name);

					// Commit Changes to Registry
					if (uacreg.CommitChangesToRegistry())
					{
						// Bypass the Guard if the write operation succeeded
						Guard_Direct_Write (mainkey, curnode, name, lvd->item.pszText);
					}
				}
				return true;
			}
			else if ((nmhdr->hwndFrom == lvstr) && (nmhdr->code == LVN_ITEMCHANGED))
			{
				NM_LISTVIEW* nml = (NM_LISTVIEW*) lparam;
				if ((!(nml->uOldState & LVIS_SELECTED)) && (nml->uNewState & LVIS_SELECTED))
				{
					// (!) Item is now selected
					lvSelect (lvname, nml->iItem, true);
					lvSetItemFocus (lvname, nml->iItem);
					Editor_EntrySelected (nml->iItem);
				}
			}
		}
	}
	else if (msg == WM_COMMAND)
	{
		long code = HIWORD(wparam);
		if (((HWND) lparam == cmdadd) && (code == BN_CLICKED))
		{
			Editor_Addreg();
		}
		else if (((HWND) lparam == cmddel) && (code == BN_CLICKED))
		{
			Editor_Delreg();
		}
		else if (((HWND) lparam == cmdupdate) && (code == BN_CLICKED))
		{
			Editor_Refresh();
		}
		else if (((HWND) lparam == cmdapply) && (code == BN_CLICKED))
		{
			Editor_Apply();
		}
		else if (((HWND) lparam == chkassis) && (code == BN_CLICKED))
		{
			Editor_ChkAssist (chkGetState(chkassis));
		}
	}
	else if (msg == WM_CLOSE)
	{
		Window_Hide();

		// Do not invoke WM_DESTROY
		return false;
	}
	else if (msg == WM_MOVE)
	{
		if (!StyleExists(ewnd, WS_MINIMIZE))
		{
			RECT newrc;
			GetWindowRect (ewnd, &newrc);
			
			// Save if the new coords are valid
			if (CheckWindowCoords(ewnd, newrc.left, newrc.top))
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
			// Session is being SURELY ended after return (this is the MainThread)
			TerminateVKernel (true);
			return false;
		}
	}
	else if (msg == WM_LBUTTONUP)
	{
		// Received message from sysicon.
		if (wparam == 1)
		{
			// Message from our Systray Icon
			static bool lock = false;
			if (!lock)
			{
				// Lock
				lock = true;

				// Act
				if (lparam == WM_LBUTTONDBLCLK)
				{
					// Toggle Window Visibility
					if (!StyleExists(ewnd, WS_VISIBLE))
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

void Autoruns::Editor_EntrySelected (long itemidx)
{
	LRESULT regsec;
	HTREEITEM curitem = tvSelectedItem(ar_tvc);
	string text = tvGetItemText (ar_tvc, curitem, 128, &regsec);

	// Display current State of the Option
	chkSetState (chkassis, (Left(lvGetItemText(lvstr, 0, itemidx), 1) == "-"));

	// Enable the window if this option can be edited for the current TreeView
	EnableWindow (chkassis, (((regsec == MYTV_GLOBAL) || (regsec == MYTV_LOCAL)) && (text == "Run")));
}


//////////////////////////
// Editor Interface		//
//////////////////////////
void Autoruns::Editor_RefreshEntries ()
{
	string text;
	LRESULT regsec;
	HTREEITEM curitem = tvSelectedItem(ar_tvc);
	if (curitem == 0) { return; };

	// Disable "Assisted Autorun" Checkbox
	EnableWindow (chkassis, false);
	chkSetState (chkassis, 0);
	
	// Get Text & Param
	text = tvGetItemText (ar_tvc, curitem, 128, &regsec);	
	
	// (?) Valid Subitem
	if ((regsec == MYTV_GLOBAL) || (regsec == MYTV_LOCAL))
	{
		RegDir rg;
		HKEY mainkey = Editor_DecideHKey(regsec);
		LVI lvi;
		string value;
		long u;
		
		// Enum the current key
		rg.Enum (mainkey, CSR(REG_AUTORUN_BASE, text));
		
		// Clear LVs
		lvClear (lvname);
		lvClear (lvstr);
		
		// Add all entries
		u = rg.Ubound();
		for (long i = 1; i <= u; i++)
		{
			if (rg.type(i) == REG_SZ)
			{
				lvAdd (lvname, 0, i, rg[i], 0, &lvi, 0);
				
				regRead (&value, mainkey, CSR(REG_AUTORUN_BASE, text), rg[i]);
				lvAdd (lvstr, 0, i, value, 0, &lvi, 0);
			}
		}
	}
	else
	{
		// Clear LVs, no valid TreeItem selected
		lvClear (lvname);
		lvClear (lvstr);
	}
}

HKEY Autoruns::Editor_DecideHKey (LRESULT nodeparam)
{
	if (nodeparam == MYTV_LOCAL)
	{
		return HKCU;
	}
	return HKLM;
}


//////////////////////
// User Interface	//
//////////////////////
void Autoruns::Editor_Addreg ()
{
	static long eidx = 0;
	LRESULT param;
	string curnode = tvGetItemText(ar_tvc, tvSelectedItem(ar_tvc), 128, &param);
	LVI lvi;
	
	if ((xlen(curnode) != 0) && (param != MYTV_INVALID))
	{
		eidx++;
		lvAdd (lvname, 0, lvCount(lvname), CSR("New Entry #", CStr(eidx)), LVIS_FOCUSED | LVIS_SELECTED, &lvi, 0);
		lvAdd (lvstr, 0, lvCount(lvstr), "", LVIS_FOCUSED | LVIS_SELECTED, &lvi, 0);
	}
	else
	{
		InfoBox (ewnd, LANG_EN ? "You cannot add an entry here!" : "Sie können an dieser Stelle keinen Eintrag hinzufügen!", CAPTION, MB_OK | MB_ICONSTOP);
	}
}

void Autoruns::Editor_Delreg ()
{
	LPARAM param;
	long selitem;
	string name;
	string curnode = tvGetItemText(ar_tvc, tvSelectedItem(ar_tvc), 128, &param);
	
	if ((xlen(curnode) != 0) && (param != MYTV_INVALID))	// Node Valid?
	{
		selitem = lvSelectedItem(lvname);
		if (selitem != -1)		// Any item selected ?
		{
			SetFocus (lvname);
			name = lvGetItemText (lvname, 0, selitem);
			
			// User must confirm deletion
			if (InfoBox(ewnd, CSR(LANG_EN ? "Do you really want to delete this entry:\n\nName: \"" : "Möchten Sie diesen Eintrag wirklich löschen:\n\nName: \"", 
										name, "\"\nValue: \"", lvGetItemText(lvstr, 0, selitem), "\""), 
										LANG_EN ? "Delete Entry" : "Eintrag löschen", MB_YESNO | MB_ICONQUESTION) == IDYES)
			{
				UACRegEdit uacreg;
				HKEY mainkey = Editor_DecideHKey(param);

				// Remove from Registry
				uacreg.DeleteValue (mainkey, CSR(REG_AUTORUN_BASE, curnode), name);

				// Commit Changes to the Registry
				if (uacreg.CommitChangesToRegistry())
				{
					// Bypass the Guard if the write operation succeeded
					Guard_Direct_Delete (mainkey, curnode, name);

					// Remove from ListViews
					lvRemove (lvname, selitem);
					lvRemove (lvstr, selitem);
				}
			}
		}
		else
		{
			InfoBox (ewnd, LANG_EN ? "Please select an item first!" : "Bitte wählen Sie zuerst einen Eintrag aus!", CAPTION, MB_OK | MB_ICONSTOP);
		}
	}
	else
	{
		InfoBox (ewnd, LANG_EN ? "You cannot delete an entry here!" : "Sie können an dieser Stelle keinen Eintrag löschen!", CAPTION, MB_OK | MB_ICONSTOP);
	}
}

void Autoruns::Editor_ChkAssist (long state)
{
	long itemidx = lvSelectedItem(lvstr);
	string name, data; 
	
	// Check if an item is currently selected
	if (itemidx < 0) { itemidx = lvSelectedItem(lvname); };
	if (itemidx < 0) { InfoBox(ewnd, LANG_EN ? "Please select an item first!" : "Bitte wählen Sie zuerst einen Eintrag aus!", CAPTION, MB_OK | MB_ICONEXCLAMATION); return; };

	// Get Item Text
	data = lvGetItemText(lvstr, 0, itemidx);

	// (!) SECURITY CHECK: Entries containing the WinSuite-Executable may not be selected for "Assisted Autorun"
	if (InStr(UCase(data), UCase(GetAppExe())) > 0)
	{
		MessageBox (ewnd, LANG_EN ? "Error: You can't startup WinSuite by assisted autorun!" :
									"Fehler: WinSuite kann mit dem Autostart-Assistenten nicht ausgeführt werden!", CAPTION, MB_OK | MB_ICONSTOP);
		chkSetState (chkassis, 0);
		return;
	}

	// Check State: 0 = "Do not add a minus for assisted autorun" / 1 = "Add minus"
	if (xlen(data) > 0)
	{
		if (state == 0)
		{
			// Check for minus and remove if existing
			if (Left(data, 1) == "-") { data = Right(data, xlen(data) - 1); };
		}
		else
		{
			// Check for minus and if non existing, add one
			if (Left(data, 1) != "-") { data = CSR("-", data); };
		}

		// Set the new content
		lvSetItemText (lvstr, 0, itemidx, data);

		// Notify the ListView of the change
		NMLVDISPINFO nmlvd;
		nmlvd.hdr.hwndFrom = lvstr;
		nmlvd.hdr.code = LVN_ENDLABELEDIT;
		nmlvd.item.iItem = itemidx;
		nmlvd.item.pszText = &data[0];
		nmlvd.item.cchTextMax = xlen(data);
		SendMessage (ewnd, WM_NOTIFY, 0, (LPARAM) &nmlvd);
	}
}

void Autoruns::Editor_Refresh ()
{
	Inifile ini;
	string res;
	string win_dir = GetWinDir();
	
	// Refresh the ListViews
	Editor_RefreshEntries();
	
	// Update Additional Shell/INI Information
	// Windows XP/Vista/7
	regRead(&res, HKEY_LOCAL_MACHINE, REG_WINLOGON, "Shell");
	ctlSetText (txtshell, res);
}

void Autoruns::Editor_Apply ()
{
	bool fail_success = false;
	Inifile ini;
	string win_dir = GetWinDir();
	
	UACRegEdit uacreg;

	// Windows XP/Vista/7
	uacreg.Write(ctlGetText(txtshell), HKEY_LOCAL_MACHINE, REG_WINLOGON, "Shell");

	// Commit Changes to the Registry
	fail_success = uacreg.CommitChangesToRegistry();

	// Message Out
	if (fail_success)
	{
		InfoBox (ewnd, LANG_EN ? "Changes have been saved successfully." : "Ihre Änderungen wurden erfolgreich gespeichert.", CAPTION, MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		InfoBox (ewnd, LANG_EN ? "Error saving changes. Probably you do not have sufficient privileges to complete this operation." : "Fehler beim Speichern der Änderungen. Möglicherweise besitzen Sie nicht die erforderlichen Berechtigungen.", CAPTION, MB_OK | MB_ICONSTOP);
	}
}


//////////////////////
// Guard Timer		//
//////////////////////
void Autoruns::CheckRegSection (HKEY mainkey, string section)
{
	RegDir rd;
	StructArray <string> idir;
	INT_PTR res;
	long i, u;
	string user, subkey, inisec;
	string regvalue, inivalue;
	
	// Find required sections first
	subkey = CSR(REG_AUTORUN_BASE, section);
	inisec = Reg2Cache(mainkey, section);
	
	// Enum the subkey
	rd.Enum (mainkey, subkey);
	u = rd.Ubound();
	
	// Try to refind all entries in the Cache
	for (i = 1; i <= u; i++)
	{
		if (rd.type(i) == REG_SZ)
		{
			if (ini.FindEntry(inisec, rd[i]) == 0)
			{
				// (!) Entry has been added
				regRead (&regvalue, mainkey, subkey, rd[i]);
				
				// Initialize WarnDialog
				GuardWndData tdia;
				tdia.action = RF_ADDED;
				tdia.mainkey = mainkey;
				tdia.section = section;
				tdia.name = rd[i];
				tdia.value = regvalue;
				tdia.class_ptr = this;
				
				// Start Guard Warning Dialog
				// NOTE: THIS CALL WAITS UNTIL THE USER SELECTS AN OPTION BEFORE CONTINUING
				res = DialogBoxParam(myinst, (LPCTSTR) IDD_ARWARN, 0, &_ext_Autoruns_Guard_WndProc, (LPARAM) &tdia);
				
				// Act
				if (res == DIA_ACCEPT)
				{
					ini.WriteEntry (inisec, rd[i], regvalue);
					Guard_FlushINI();
					
					// Refresh the Editor Window
					Editor_Refresh();
				}
				else if (res == DIA_RESTORE)
				{
					UACRegEdit uacreg;

					uacreg.DeleteValue (mainkey, subkey, rd[i]);
					uacreg.CommitChangesToRegistry();
				}
			}
			else
			{
				regRead (&regvalue, mainkey, subkey, rd[i]);
				inivalue = ini.GetEntry(inisec, rd[i]);
				if (regvalue == inivalue)
				{
					// (!) Entry matches
				}
				else
				{
					// (!) Entry changed
					// Initialize WarnDialog
					GuardWndData tdia;
					tdia.action = RF_CHANGED;
					tdia.mainkey = mainkey;
					tdia.section = section;
					tdia.name = rd[i];
					tdia.value = regvalue;
					tdia.prev = inivalue;
					tdia.class_ptr = this;
					
					// Start Guard Warning Dialog
					// NOTE: THIS CALL WAITS UNTIL THE USER SELECTS AN OPTION BEFORE CONTINUING
					res = DialogBoxParam(myinst, (LPCTSTR) IDD_ARWARN, 0, &_ext_Autoruns_Guard_WndProc, (LPARAM) &tdia);
					
					// Act
					if (res == DIA_ACCEPT)
					{
						ini.WriteEntry (inisec, rd[i], regvalue);
						Guard_FlushINI();
						
						// Refresh the Editor Window
						Editor_Refresh();
					}
					else if (res == DIA_RESTORE)
					{
						UACRegEdit uacreg;

						uacreg.Write (inivalue, mainkey, subkey, rd[i]);
						uacreg.CommitChangesToRegistry();
					}
				}
			}
		}
	}
	
	// Enum the INI-Cache
	ini.EnumEntries (inisec, &idir);
	u = idir.Ubound();
	
	// Try to refind all entries in the Reinistry
	for (i = 1; i <= u; i++)
	{
		if (regValueExistType(mainkey, subkey, idir[i]) != REG_SZ)
		{
			// (!) Entry deleted
			inivalue = ini.GetEntry(inisec, idir[i]);
			
			// Initialize the Guard Dialog
			GuardWndData tdia;
			tdia.action = RF_DELETED;
			tdia.mainkey = mainkey;
			tdia.section = section;
			tdia.name = idir[i];
			tdia.prev = inivalue;
			tdia.class_ptr = this;
			
			// Start Guard Warning Dialog
			// NOTE: THIS CALL WAITS UNTIL THE USER SELECTS AN OPTION BEFORE CONTINUING
			res = DialogBoxParam(myinst, (LPCTSTR) IDD_ARWARN, 0, &_ext_Autoruns_Guard_WndProc, (LPARAM) &tdia);
			
			// Act
			if (res == DIA_ACCEPT)
			{
				ini.DeleteEntry (inisec, idir[i]);
				Guard_FlushINI();
				
				// Refresh the Editor Window
				Editor_Refresh();
			}
			else if (res == DIA_RESTORE)
			{
				UACRegEdit uacreg;

				uacreg.Write (inivalue, mainkey, subkey, idir[i]);
				uacreg.CommitChangesToRegistry();
			}
		}
	}
}


//////////////////////////////
// INI's File Operations	//
//////////////////////////////
bool Autoruns::Guard_LockINI ()
{
	bool fail_success = false;

	// Ensure the right attributes (no readonly!)
	SetFileAttributes (&ini_file[0], ATTRIBUTE_INI_LOCKED);
	
	// Pseudo-Open the file and disallow sharing
	arfile = CreateFile (&ini_file[0], GENERIC_READ | GENERIC_WRITE, 0, 
							NULL, OPEN_ALWAYS, ATTRIBUTE_INI_LOCKED, 0);
	if (arfile == INVALID_HANDLE_VALUE)
	{
		InfoBox (0, LANG_EN ? "Unable to lock guard file. Maybe it is in use by another application that might currently try to bypass the Autostart Observation!" :
									"Die Autostartüberwachung konnte nicht initialisiert werden. Möglicherweise versucht ein anderes Programm, die Autostartüberwachung zu umgehen!", 
										GUARD_CAPTION, MB_OK | MB_ICONEXCLAMATION);
	}
	
	// Return if the operation succeeded
	fail_success = (arfile != INVALID_HANDLE_VALUE);
	return fail_success;
}

void Autoruns::Guard_UnlockINI ()
{
	if (arfile != INVALID_HANDLE_VALUE)
	{
		CloseHandle (arfile);
		arfile = INVALID_HANDLE_VALUE;		// (!) Never close it again
	}
}

void Autoruns::Guard_FlushINI ()
{
	// Ensure the right attributes (no readonly!)
	SetFileAttributes (&ini_file[0], ATTRIBUTE_INI_LOCKED);
	
	// Turn off lock, save and turn it on again
	Guard_UnlockINI();
	SetFileAttributes (&ini_file[0], ATTRIBUTE_INI_UNLOCKED);
	ini.Save (ini_file, true);
	SetFileAttributes (&ini_file[0], ATTRIBUTE_INI_LOCKED);
	Guard_LockINI();
}

string Autoruns::Reg2Cache (HKEY mainkey, string section)
{
	string user;
	if (mainkey == HKCU)
	{
		user = GetCurrentUser();
		if (xlen(user) == 0)
		{
			user = "-:Anonymous:-";
		}
	}
	else
	{
		user = "-:Global:-";
	}
	return CSR(user, "\\", section);
}


//////////////////////////////////
// Autostart Warning Dialog		//
//////////////////////////////////
INT_PTR CALLBACK Autoruns::_ext_Autoruns_Guard_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	GuardWndData* gwd = (GuardWndData*) GetWindowLongPtr (hwnd, GWLP_USERDATA);
	if (gwd != 0) { return ((Autoruns*) (gwd->class_ptr))->Guard_WndProc (hwnd, msg, wparam, lparam); };
	if (msg == WM_INITDIALOG)
	{
		// Store GuardWndData* in DIALOG'S USERDATA
		SetWindowLongPtr (hwnd, GWLP_USERDATA, lparam);
		return _ext_Autoruns_Guard_WndProc (hwnd, msg, wparam, lparam);
	}
	return false;
}

INT_PTR CALLBACK Autoruns::Guard_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	GuardWndData* gwd = (GuardWndData*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
	string text_surface;

	if (msg == WM_INITDIALOG)
	{
		// Repositon Dialog Window (Timer Call)
		SetTimer (hwnd, 1, 50, NULL);

		// Set Texts
		ctlSetText (hwnd, GUARD_CAPTION);
		ctlSetText (dia(hwnd, IDC_WARNING), LANG_EN ? "Warning:\n¯¯¯¯¯¯¯¯" : "Warnung:\n¯¯¯¯¯¯¯¯");
		ctlSetText (dia(hwnd, IDB_ACCEPT), LANG_EN ? "&Accept Change" : "&Akzeptieren");
		ctlSetText (dia(hwnd, IDB_RESTORE), LANG_EN ? "&Restore Entry" : "&Rückgängig");
		text_surface = ((gwd->mainkey == HKCU) ? (LANG_EN ? "User Autostarts" : "Benutzer-Autostarts") : (LANG_EN ? "Global Autostarts" : "Globale-Autostarts"));
		
		// Initialize Content
		if (gwd->action == RF_ADDED)
		{
			if (LANG_EN)
			{
				ctlSetText (dia(hwnd, IDS_EVENTMSG), CSR(
							CSR("Autostart value has been added.\n\nLevel:\t",
									text_surface, " -> ", gwd->section, "\n\nEntry:\t"), 
							CSR(Chr(34), gwd->name, Chr(34), "\nValue:\t"), 
							CSR(Chr(34), gwd->value, Chr(34))));
			}
			else
			{
				ctlSetText (dia(hwnd, IDS_EVENTMSG), CSR(
							CSR("Es wurde ein Autostart-Eintrag hinzugefügt.\n\nLevel:\t",
									text_surface, " -> ", gwd->section, "\n\nEintrag:\t"), 
							CSR(Chr(34), gwd->name, Chr(34), "\nWert:\t"), 
							CSR(Chr(34), gwd->value, Chr(34))));
			}
		}
		else if (gwd->action == RF_CHANGED)
		{
			if (LANG_EN)
			{
				ctlSetText (dia(hwnd, IDS_EVENTMSG), CSR(
							CSR("Autostart value has been changed.\n\nLevel:\t",
									text_surface, " -> ", gwd->section, "\nEntry:\t"), 
							CSR(Chr(34), gwd->name, Chr(34), "\n\nCurrent:\t\t"), 
							CSR(Chr(34), gwd->value, Chr(34), "\nPrevious:\t"), 
							CSR(Chr(34), gwd->prev, Chr(34))));
			}
			else
			{
				ctlSetText (dia(hwnd, IDS_EVENTMSG), CSR(
							CSR("Ein Autostart-Eintrag wurde verändert.\n\nLevel:\t",
									text_surface, " -> ", gwd->section, "\nEintrag:\t"), 
							CSR(Chr(34), gwd->name, Chr(34), "\n\nNeuer Wert:\t"), 
							CSR(Chr(34), gwd->value, Chr(34), "\nVorheriger Wert:\t"), 
							CSR(Chr(34), gwd->prev, Chr(34))));
			}
		}
		else if (gwd->action == RF_DELETED)
		{
			if (LANG_EN)
			{
				ctlSetText (dia(hwnd, IDS_EVENTMSG), CSR(
							CSR("Autostart value has been deleted.\n\nLevel:\t",
									text_surface, " -> ", gwd->section, "\n\nEntry:\t"), 
							CSR(Chr(34), gwd->name, Chr(34), "\nWas:\t"), 
							CSR(Chr(34), gwd->prev, Chr(34))));
			}
			else
			{
				ctlSetText (dia(hwnd, IDS_EVENTMSG), CSR(
							CSR("Ein Autostart-Eintrag wurde gelöscht.\n\nLevel:\t",
									text_surface, " -> ", gwd->section, "\n\nEintrag:\t\t"), 
							CSR(Chr(34), gwd->name, Chr(34), "\nVorheriger Wert:\t"), 
							CSR(Chr(34), gwd->prev, Chr(34))));
			}
		}
		
		// Show the Guard Warning Dialog
		SetWindowPos (hwnd, 0, 30, 30, DIA_DEFWIDTH, DIA_DEFHEIGHT, 0);
		PostMessage (hwnd, WM_SETICON, ICON_SMALL, (LPARAM) L_IDI_REGTOOL16);
		ShowWindow (hwnd, SW_SHOW);
		return true;
	}
	else if (msg == WM_TIMER)
	{
		RECT wa;

		// Repositon Dialog Window (Timer Call)
		SystemParametersInfo (SPI_GETWORKAREA, 0, &wa, 0);
		SetWindowPos (hwnd, 0, wa.left + 15, wa.top + 15, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
		KillTimer (hwnd, 1);
	}
	else if (msg == WM_COMMAND)
	{
		long id = LOWORD(wparam);
		long code = HIWORD(wparam);
		
		if ((id == IDB_ACCEPT) && (code == BN_CLICKED))
		{
			EndDialog (hwnd, DIA_ACCEPT);
		}
		else if ((id == IDB_RESTORE) && (code == BN_CLICKED))
		{
			EndDialog (hwnd, DIA_RESTORE);
		}
		else if ((id == IDB_DETAILS) && (code == BN_CLICKED))
		{
			// Display Details
			if (gwd != 0)
			{
				if (gwd->action == RF_ADDED)
				{
					ctlSetText (dia(hwnd, IDS_DETAILS), LANG_EN ? 
						/* EN */ "An entry which results in a new program being auto-started has been added. This is normal if you install a new program which needs to be started automatically. If you added it yourself just click \"Accept\" or undo the changes by clicking \"Restore\"." : 
						/* DE */ "Ein hinzugefügter Eintrag bedeutet, dass beim Anmelden eines Benutzers automatisch ein weiteres Programm gestartet wird. Wenn Sie gerade ein neues Programm installieren, dessen Autostart Sie wünschen, \"Akzeptieren\" Sie diese Änderung. Wenn Sie das Hinzufügen \"Rückgängig\" machen, wird das Programm nicht automatisch starten und keine Leistung des Systems permanent in Anspruch nehmen. Befindet sich die Änderung unter \"RunOnce\", akzeptieren Sie diese, wenn Sie gerade ein Setupprogramm ausführen."
						);
				}
				else if (gwd->action == RF_CHANGED)
				{
					ctlSetText (dia(hwnd, IDS_DETAILS), LANG_EN ? 
						/* EN */ "An entry has been modified probably to auto-start another program than it did oriininally. Maybe that a setup program changed it. If you modified the entry yourself just click \"Accept\" or undo the changes by clicking \"Restore\"." : 
						/* DE */ "Ein Eintrag wurde dahingehend verändert, dass er ein anderes Programm automatisch nach dem Anmelden starten lässt als es ursprünglich eingestellt war. Dies sollte nur akzeptiert werden, wenn Sie in einem gewollt automatisch startenden Programm Änderungen an der Startvariante vorgenommen haben. Klicken Sie andernfalls auf \"Rückgängig\"."
					);
				}
				else if (gwd->action == RF_DELETED)
				{
					ctlSetText (dia(hwnd, IDS_DETAILS), LANG_EN ? 
						/* EN */ "A deleted entry results in a faster loading of Windows. If a windows component had been removed, you should really restore this entry AT ONCE by clicking 'Restore'. Please note that you should not restore any temporarily used entries, e.g. found in the \"RunOnce\" section!" : 
						/* DE */ "Ein gelöschter Eintrag kann den Startvorgang von Windows u.U. beschleunigen. Wurde allerdings eine für den Windowsbetrieb benötigte Systemkomponente entfernt, sollten Sie den Löschvorgang unbedingt rückgängig machen. Wenn es sich um den Abschnitt \"RunOnce\" handelt, sollten Sie den Vorgang i.d.R. akzeptieren, da dies Setupprogramme zur korrekten Funktion benötigen."
						);
				}
				
				// Disable "Details" Button and Resize Dialog
				ctlSetText (dia(hwnd, IDB_DETAILS), "Details <<");
				EnableWindow (dia(hwnd, IDB_DETAILS), false);
				SetWindowPos (hwnd, 0, 0, 0, DIA_DEFWIDTH, DIA_DETAILMODE, SWP_NOMOVE);
			}
		}
	}
	return false;
}


//////////////////////////////////
// Extern Access to the Cache	//
//////////////////////////////////
void Autoruns::Guard_Direct_Write (HKEY mainkey, string section, string name, string value)
{
	if (guard_inited)
	{
		ini.WriteEntry (Reg2Cache(mainkey, section), name, value);
		Guard_FlushINI();
	}
}

void Autoruns::Guard_Direct_Delete (HKEY mainkey, string section, string name)
{
	if (guard_inited)
	{
		ini.DeleteEntry (Reg2Cache(mainkey, section), name);
		Guard_FlushINI();
	}
}







//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////
// AUTORUNS LAUNCHER CLASS BEGINS HERE			//
//////////////////////////////////////////////////


//////////////////////////
// Consts: Launcher		//
//////////////////////////
const char ARLauncher::LAUNCHER_CAPTION[]			= "System Autorun";
const char ARLauncher::WNDCLASS_LAUNCHER[]			= "*C32_Autoruns_Launcher";

const long ARLauncher::LAUNCHER_WND_X				= 70;											// [70]
const long ARLauncher::LAUNCHER_WND_Y				= 240;											// [55]
const long ARLauncher::LAUNCHER_WND_WIDTH			= 310;											// [310]
const long ARLauncher::LAUNCHER_WND_HEIGHT			= 90;											// [90]

const char ARLauncher::REG_AUTORUN_BASE[]			= "Software\\Microsoft\\Windows\\CurrentVersion\\";


//////////////////////////////////
// Functions: Init / Terminate	//
//////////////////////////////////
ARLauncher::ARLauncher ()
{
	// Init RAM
	ctrlinited = false;
	termsignal = false;
}

ARLauncher::~ARLauncher ()
{
	// Wait for Launcher Thread to finish
	LAUNCHER_VM.AwaitShutdown();
}

bool ARLauncher::Launcher_ExternExecute ()																// public
{
	long cnt_all_assisted_ars;

	// Init RAM
	termsignal = false;

	// First count if there are any "Assisted Autostarts" to perform
	cnt_all_assisted_ars = Launcher_ReturnCountOfAssistedARS(HKEY_LOCAL_MACHINE, CSR(REG_AUTORUN_BASE, "Run"));
	cnt_all_assisted_ars += Launcher_ReturnCountOfAssistedARS(HKEY_CURRENT_USER, CSR(REG_AUTORUN_BASE, "Run"));

	// Check if we have work to do
	if (cnt_all_assisted_ars == 0)
	{
		// (!) We have no work to do
		//
		// Exit External Call to this function successfully
		return true;
	}
	
	// Create the Service Window
	NewWindowClass (WNDCLASS_LAUNCHER, &_ext_ARLauncher_WndProc, myinst, 0, 0, 0, NULL, true);
	awnd = CreateWindowEx (WS_EX_APPWINDOW, WNDCLASS_LAUNCHER, LANG_EN ? "Performing Autorun ..." : "Verarbeite Autostarts ...", 
							WS_CAPTION | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 
							LAUNCHER_WND_X, LAUNCHER_WND_Y, LAUNCHER_WND_WIDTH, LAUNCHER_WND_HEIGHT, 
							0, 0, myinst, 0);
	adc = awnd_mem.Create (awnd, LAUNCHER_WND_WIDTH, LAUNCHER_WND_HEIGHT);
	SetBkMode (adc, TRANSPARENT);
	
	// Create the IconBox
	iconbox = new IconBox (awnd, 23, 16, 32, 32, L_IDI_RESTART32);
	ctrlinited = true;
	
	// Show the Service Window
	PostMessage (awnd, WM_SETICON, ICON_SMALL, (LPARAM) L_IDI_RESTART32);
	ShowWindow (awnd, SW_SHOW);

	// Inform the User that we are going to startup all autoruns that are assisted
	Launcher_ShowStatus (LANG_EN ? "Awaiting System startup ..." : "Warten auf Autostarts ...", "");

	// Start Worker Thread
	LAUNCHER_VM.NewThread (&_ext_ARLauncher_WorkerThread, this);

	// Return Success
	return true;
}

long ARLauncher::_ext_ARLauncher_WorkerThread (ARLauncher* class_ptr)								// protected
{
	if (class_ptr != 0) { return class_ptr->Launcher_WorkerThread(); } else { return -1; };
}

long ARLauncher::Launcher_WorkerThread ()
{
	// Await System Startup
	Sleep (2500);

	// Start the Launcher Chain for each registry branch
	Launcher_LaunchPrograms(HKEY_LOCAL_MACHINE, CSR(REG_AUTORUN_BASE, "Run"));
	Launcher_LaunchPrograms(HKEY_CURRENT_USER, CSR(REG_AUTORUN_BASE, "Run"));
	
	// Initiate Shutdown
	Launcher_ShowStatus (LANG_EN ? "Startup completed." : "Systemstart abgeschlossen.", "");
	termsignal = true;
	PostMessage (awnd, WM_CLOSE, 0, 0);

	// Thread Shutdown
	LAUNCHER_VM.RemoveThread();
	return -8;
}

long ARLauncher::Launcher_ReturnCountOfAssistedARS (HKEY root, string subkey)
{
	RegDir rd;
	long i, u, cnt;
	string call;

	// Set Counter to Zero
	cnt = 0;

	// Enum Autorun Registry Key and look for "Assisted Autorun"-entries
	rd.Enum (root, subkey);
	u = rd.Ubound();
	for (i = 1; i <= u; i++)
	{
		regRead(&call, root, subkey, rd[i]);

		// Got an Autorun entry, now check if it is "Assisted Autorun" ("-"-Mark)
		if (xlen(call) > 0)
		{
			if (Left(call, 1) == "-")
			{
				// (!) It is an assisted autorun entry, so count it
				cnt++;
			}
		}
	}

	// Return count of assisted autostarts that have been found in the given autorun section
	return cnt;
}

void ARLauncher::Launcher_LaunchPrograms (HKEY root, string subkey)										// private
{
	RegDir rd;
	long i, u, res;
	string call;

	// Enum Autorun Registry Key and look for "Assisted Autorun"-entries
	rd.Enum (root, subkey);
	u = rd.Ubound();
	for (i = 1; i <= u; i++)
	{
		regRead(&call, root, subkey, rd[i]);

		// Got an Autorun entry, now check if it is "Assisted Autorun" ("-"-Mark)
		if (xlen(call) > 0)
		{
			if (Left(call, 1) == "-")
			{
				call = Right(call, xlen(call) - 1);

				// (!) Found an "Assisted Autorun" Item
				Launcher_ShowStatus (call, "");
				
				// Launch current program
				if ((LONG_PTR) ShellExecute(0, call) > 32)
				{
					// (!) Success, wait for next program to execute
					Sleep (4000);
				}
				else
				{
					// (!) Error occured, do not wait for next program execution
					res = InfoBox (awnd, LANG_EN ? CSR("The program with the identifier \"", rd[i], "\" failed to start. Do you want to remove it from system startup?") : 
													CSR("Das Programm mit der Kennung \"", rd[i], "\" konnte nicht gestartet werden. Möchten Sie es aus dem Autostart entfernen?"), 
													LAUNCHER_CAPTION, MB_YESNO | MB_ICONEXCLAMATION);
					if (res == IDYES)
					{
						UACRegEdit uacreg;

						// Delete this Autorun entry
						uacreg.DeleteValue (root, subkey, rd[i]);
						uacreg.CommitChangesToRegistry ();
					}
				}
			}
		}
	}
}


//////////////////////////////////////////
// Functions: Launcher Window Process	//
//////////////////////////////////////////
LRESULT CALLBACK ARLauncher::_ext_ARLauncher_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)	// protected
{
	ARLauncher* class_ptr = (ARLauncher*) GetWindowLongPtr (hwnd, GWLP_USERDATA);
	if (class_ptr != 0)
	{
		return class_ptr->Launcher_WndProc (hwnd, msg, wparam, lparam);
	}
	return DefWindowProc (hwnd, msg, wparam, lparam);
}

LRESULT CALLBACK ARLauncher::Launcher_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)					// public
{
	if (msg == WM_PAINT)
	{
		awnd_mem.Refresh();
		return false;
	}
	else if (msg == WM_CLOSE)
	{
		if (termsignal)
		{
			// Terminate Launcher
			// ------------------
			//
			// Hide & Unload controls
			ShowWindow (awnd, SW_HIDE);
			if (ctrlinited)
			{
				delete iconbox;
				ctrlinited = false;
			}
			
			// Terminate
			awnd_mem.Free();
			DestroyWindow (awnd);
			UnregisterClass (WNDCLASS_LAUNCHER, myinst);
		}
		return false;				// Do not invoke WM_DESTROY
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

void ARLauncher::Launcher_ShowStatus (string lone, string ltwo)													// private
{
	FillRect2 (adc, 0, 0, LAUNCHER_WND_WIDTH, LAUNCHER_WND_HEIGHT, (HBRUSH) COLOR_WINDOW);
	FontOut (adc, 67, 17, lone, sfnt.MSSANS);
	FontOut (adc, 67, 35, ltwo, sfnt.MSSANS);
	awnd_mem.Refresh();
}


