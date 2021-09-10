// SystrayPlayer.cpp: SystrayPlayer's Main Module
//
// Thread-Safe:	PARTLY
//


//////////////////
// Includes		//
//////////////////
#include "stdafx.h"
#include "../resource.h"

#include "math.h"
#include "vfw.h"
#include "vfwmsgs.h"
#include "evcode.h"

#include "../Modules/_modules.h"

#include "../Boot.h"
#include "SystrayPlayer.h"


//////////////////
// Consts		//
//////////////////
const char SystrayPlayer::CAPTION[]					= "Systray Player";
const char SystrayPlayer::TRAY_CAPTION[]			= "» Systray Player";
const char SystrayPlayer::WNDCLASS[]				= "*WS_C32_SystrayPlayer";
const long SystrayPlayer::SYSICON_TIMER_INTERVAL	= 5000;							// [5000]
const int  SystrayPlayer::POSBAR_TIMER_ID			= 1;							// [1]
const int  SystrayPlayer::SYSICON_TIMER_ID			= 2;							// [2]

const long SystrayPlayer::VFW_E_FILEMISSING			= 0x80070002;

const char SystrayPlayer::REG_WINDOW_X[]			= "PlayerWindowX";
const char SystrayPlayer::REG_WINDOW_Y[]			= "PlayerWindowY";
const char SystrayPlayer::REG_VOLUME[]				= "PlayerVolume";
const char SystrayPlayer::REG_REPEAT[]				= "PlayerAutoRepeat";
const char SystrayPlayer::REG_LISTCOUNT[]			= "PlayerPlaylistCount";
const char SystrayPlayer::REG_RAMDRIVE[]			= "PlayerRamdrive";
const char SystrayPlayer::REG_LASTPLAYINDEX[]		= "PlayerLastPlayIndex";
const char SystrayPlayer::REG_LASTPLAYSTATE[]		= "PlayerLastPlayState";
const char SystrayPlayer::REG_RANDOMMODE[]			= "PlayerRandomMode";
const char SystrayPlayer::REG_PLAYLIST_PREFIX[]		= "PL_";						// Keep this the same across program releases.

const long SystrayPlayer::DEF_WINDOW_X				= 190;							// [190]
const long SystrayPlayer::DEF_WINDOW_Y				= 130;							// [130]
const long SystrayPlayer::WINDOW_WIDTH				= 412;							// [412]
const long SystrayPlayer::WINDOW_HEIGHT				= 338;							// [338]

const long SystrayPlayer::MODE_NORMAL				= 0;
const long SystrayPlayer::MODE_RAMDISK				= 1;

const long SYSTRAYPLAYER_STATE_STOP					= 0;
const long SYSTRAYPLAYER_STATE_PAUSED				= 1;
const long SYSTRAYPLAYER_STATE_PLAYING				= 2;


//////////////////////////////////////////////////
// Class: SystrayPlayer - Init / Terminate		//
//////////////////////////////////////////////////
SystrayPlayer::SystrayPlayer ()
{
	// Variables.
	long initx;
	long inity;
	long initrandom;			// Temporary Variable for fetching last state of random checkbox.
	long initrepeat;			// Temporary Variable for fetching last state of repeat checkbox.
	long initplaystate;			// Temporary Variable for fetching last state of playback.
	long u;
	string file;

	// Init RAM.
	bShutdownAllowed = false;
	ctrlinited = false;
	window_visible = false;

	// Init Startup Player State.
	mentry.Clear();
	pl_state = SYSTRAYPLAYER_STATE_STOP;
	pl_curfile = 0;
	pl_curloop = 0;
	pl_curspeed = 1;

	// Init Multimedia File Extension Filter for Shell OpenDialog.
	CDLG_FILTER_MULTIMEDIA_FILES = CSR(CreateFilter(LANG_EN ? "MP3 Compressed Audio" : "MP3-Audio", "*.mp3"),
									   CreateFilter(LANG_EN ? "Wave Audio" : "Wave-Audio", "*.wav"), 
									   CreateFilter(LANG_EN ? "MIDI Audio" : "MIDI-Instrumentale", "*.mid"), 
									   CreateFilter(LANG_EN ? "Windows Media Audio" : "Windows Media Audio", "*.wma"));
	CDLG_FILTER_MULTIMEDIA_FILES += CSR(CreateFilter(LANG_EN ? "All Multimedia Files" : "Alle Multimedia Dateien", "*.mp3;*.wav;*.mid;*.wma"), 
										CreateFilter(LANG_EN ? "All Files" : "Alle Dateien", "*.*"));

	// Initialize Keys & Values
	reg.SetRootKey (HKEY_CURRENT_USER, "Software\\WinSuite\\SystrayPlayer");
	reg.Init (REG_WINDOW_X, DEF_WINDOW_X);
	reg.Init (REG_WINDOW_Y, DEF_WINDOW_Y);
	reg.Init (REG_VOLUME, 100);
	reg.Init (REG_REPEAT, 0);
	reg.Init (REG_LISTCOUNT, 0);
	reg.Init (REG_RAMDRIVE, "");
	reg.Init (REG_RANDOMMODE, 0);
	reg.Init (REG_LASTPLAYINDEX, 0);
	reg.Init (REG_LASTPLAYSTATE, SYSTRAYPLAYER_STATE_STOP);
	
	// Verify Window Coordinates
	reg.Get (&initx, REG_WINDOW_X);
	reg.Get (&inity, REG_WINDOW_Y);
	if (!CheckWindowCoords(0, initx, inity))
	{
		// Coords are invalid, so save the default coords
		reg.Set (DEF_WINDOW_X, REG_WINDOW_X);
		reg.Set (DEF_WINDOW_Y, REG_WINDOW_Y);

		// Update RAM
		initx = DEF_WINDOW_X;
		inity = DEF_WINDOW_Y;
	}

	// Load Resources.
	LoadResources();

	// Determine Runmode
	runmode = MODE_NORMAL;
	reg.Get (&ramdisk, REG_RAMDRIVE);
	if (ramdisk.length() != 0)
	{
		if (InitRamdisk())
		{
			// (!) Ramdisk Init succeeded
			runmode |= MODE_RAMDISK;
		}
	}

	// Create the Main Window,
	NewWindowClass (WNDCLASS, &_ext_SystrayPlayer_WndProc, myinst, 0, 0, L_IDI_AMOVIE, NULL, true);
	pwnd = CreateWindowEx (WS_EX_APPWINDOW | WS_EX_CONTROLPARENT, WNDCLASS, CAPTION, 
							WS_CAPTION | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | 
							WS_SYSMENU | WS_MINIMIZEBOX, 
							initx, inity, WINDOW_WIDTH, WINDOW_HEIGHT + (GetWindowCaptionHeight() - 18), 
							0, 0, myinst, 0);
	SetWindowLongPtr (pwnd, GWLP_USERDATA, (LONG_PTR) this);
	pdc = pwnd_mem.Create (pwnd, WINDOW_WIDTH, WINDOW_HEIGHT);
	SetBkMode (pdc, TRANSPARENT);
	
	// Add the Listbox
	FontOut (pdc, 8, 9, LANG_EN ? "Drag and Drop your Music files here:" : "Legen Sie hier Ihre Musikdateien ab:", sfnt.MSSANS);
	lb = CreateWindowEx (WS_EX_CLIENTEDGE | WS_EX_ACCEPTFILES, "LISTBOX", "mp3list", 
							WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | 
							LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT | LBS_NOTIFY, 
							7, 25, 296, 226, pwnd, 0, myinst, 0);
	ctlSetFont (lb, sfnt.MSSANS);
	
	// Add BitmapButtons
	cmdadd = new BitmapButton (pwnd, 309, 25, 91, 26, L_IDI_ADDFILE, sfnt.MSSANS, LANG_EN ? "&Add File ..." : "Hinzufügen...");
	cmdremove = new BitmapButton (pwnd, 309, 56, 91, 26, L_IDI_REMOVE, sfnt.MSSANS, LANG_EN ? "&Remove File" : "Entfernen...");
	cmdplay = new BitmapButton (pwnd, 309, 110, 91, 26, L_IDI_PLAY, sfnt.MSSANS, "&Play");
	cmdpause = new BitmapButton (pwnd, 309, 141, 91, 26, L_IDI_PLAYPAUSE, sfnt.MSSANS, "Pause");
	cmdstop = new BitmapButton (pwnd, 309, 172, 91, 26, L_IDI_STOP, sfnt.MSSANS, "&Stop");
	cmdnextfile = new BitmapButton (pwnd, 309, 225, 91, 26, L_IDI_NEXT, sfnt.MSSANS, LANG_EN ? "&Next file" : "&Nächst. Lied");
#ifdef SYSTRAYPLAYER_CANEXIT
	cmdexit = new BitmapButton (pwnd, 309, 225, 91, 26, L_IDI_CLOSE, sfnt.MSSANS, "E&xit Player");
#endif

	cmdremove->Enable (false);
	cmdpause->Enable (false);
	cmdstop->Enable (false);
	cmdnextfile->Enable (false);
	

	//
	// Add Trackbar: Position Slider
	//
	trkpos = CreateWindowEx (WS_EX_DLGMODALFRAME, TRACKBAR_CLASS, "Position", 
								WS_CHILD | WS_VISIBLE | WS_TABSTOP | 
								TBS_HORZ | TBS_FIXEDLENGTH, 
								7, 257, 355, 29, pwnd, 0, myinst, 0);
	trkSetRange (trkpos, 0, 100, false);
	trkSetPos (trkpos, 0);
	

	//
	// Add Trackbar: Volume Slider
	//
	reg.Get (&pl_volume, REG_VOLUME);
	trkvol = CreateWindowEx (0, TRACKBAR_CLASS, "Volume", 
								WS_CHILD | WS_VISIBLE | WS_TABSTOP | 
								TBS_VERT | TBS_FIXEDLENGTH | 
								TBS_BOTH | TBS_NOTICKS | TBS_TOOLTIPS, 
								371, 253, 19, 40, pwnd, 0, myinst, 0);
	trkSetRange (trkvol, -100, 0, false);
	trkSetThumbsize (trkvol, 15);
	trkSetPos (trkvol, -pl_volume);
	

	//
	// GUI: Add Checkboxes and Edit Controls
	//
	// 1. Random Mode
	//
	chkrandom = CreateWindowEx (0, "BUTTON", LANG_EN ? "Random order" : "Zufällig", 
								WS_CHILD | WS_VISIBLE | WS_TABSTOP |  
								BS_AUTOCHECKBOX, 
								7, 292, 86, 17, pwnd, 0, myinst, 0);
	ctlSetFont (chkrandom, sfnt.MSSANS);
	//
	// 2. Repeat Mode
	//
	chkrepeat = CreateWindowEx (0, "BUTTON", LANG_EN ? "Auto Repeat:" : "Wiederholen:", 
								WS_CHILD | WS_VISIBLE | WS_TABSTOP |  
								BS_AUTOCHECKBOX, 
								20 + 90, 292, 86, 17, pwnd, 0, myinst, 0);
	ctlSetFont (chkrepeat, sfnt.MSSANS);
	
	txtrepeat = CreateWindowEx (WS_EX_CLIENTEDGE, "EDIT", "0", 
								WS_CHILD | WS_VISIBLE | WS_TABSTOP | 
								ES_NUMBER,
								107 + 90, 291, 23, 18, pwnd, 0, myinst, 0);
	ctlSetFont (txtrepeat, sfnt.MSSANS);
	EnableWindow (txtrepeat, false);
	FontOut (pdc, 135 + 90, 293, LANG_EN ? "times" : "Mal", sfnt.MSSANS);
	//
	// 3. Auto-Shutdown Mode
	//
	chkshutdown = CreateWindowEx (0, "BUTTON", LANG_EN ? "Shutdown afterwards" : "Danach herunterfahren", 
									WS_CHILD | WS_VISIBLE | WS_TABSTOP | 
									BS_AUTOCHECKBOX, 
									270, 292, 190, 17, pwnd, 0, myinst, 0);
	ctlSetFont (chkshutdown, sfnt.MSSANS);
	

	//
	// Create ActiveMovie Control
	//
	spl_amovie = new ActiveMovie;
	
	// Create the "Speed" & the "Clear List" Button
	cmdspeed = CreateButton (pwnd, LANG_EN ? "Speed..." : "Geschw.", 
								201, 7, 48, 17, myinst);
	ctlSetFont (cmdspeed, sfnt.MSSANS);

	cmdclear = CreateButton (pwnd, LANG_EN ? "C&lear" : "&Leeren", 
								253, 7, 48, 17, myinst);
	ctlSetFont (cmdclear, sfnt.MSSANS);
	

	// ----------------------------------------
	// (!) ALL CONTROLS ARE NOW INITIALIZED (!)
	// ----------------------------------------
	ctrlinited = true;


	//
	// Add the Systray Icon
	//
	sysicon.SetData (L_IDI_AMOVIE, TRAY_CAPTION, pwnd, 1);
	sysicon.Show();

	// Add Sysicon Refresh Timer
	SetTimer (pwnd, SYSICON_TIMER_ID, SYSICON_TIMER_INTERVAL, NULL);

	// Update GUI: Last state of "Repeat Mode".
	reg.Get (&initrepeat, REG_REPEAT);
	if (initrepeat == 1)
	{
		chkSetState (chkrepeat, BST_CHECKED);
		WndProc (pwnd, WM_COMMAND, MAKEWPARAM(0, BN_CLICKED), (LPARAM) chkrepeat);
	}

	// Update GUI: Last state of "Random Mode".
	reg.Get (&initrandom, REG_RANDOMMODE);
	if (initrandom == 1)
	{
		chkSetState (chkrandom, BST_CHECKED);
		WndProc (pwnd, WM_COMMAND, MAKEWPARAM(0, BN_CLICKED), (LPARAM) chkrandom);
	}
	

	// ------------------------------------------
	// Read the last playlist stored in registry.
	// ------------------------------------------
	//
	// 1. Read last-stored playlist from registry.
	mentry.Clear();
	reg.Get (&u, REG_LISTCOUNT);
	for (long i = 1; i <= u; i++)
	{
		// Make sure "file" is empty if an entry does not exist.
		file = "";

		// Read and add one file of the stored playlist
		if (reg.Get(&file, CSR(REG_PLAYLIST_PREFIX, FillStr(CStr(i), 4, '0'))) != REG_NOVALUE)
		{
			if (file.length() > 0)
			{
				AddNewFile (file);
			}
		}
	}
	//
	// 2. Read file that was playing at the time the player was shut down last time.
	//
	reg.Get (&pl_curfile, REG_LASTPLAYINDEX);
	if (pl_curfile > 0)
	{
		// Check if file index lies between valid array bounds.
		if ((pl_curfile >= 0) && (pl_curfile <= mentry.Ubound()))
		{
			// Auto-select and play the last stored active file.
			lbSelectSingle (lb, pl_curfile - 1);				// ListBox Control is zero-based.
		}
		else
		{
			// FIX invalid index.
			pl_curfile = 0;
		}
	}
	//
	// 3. Load last player state from registry.
	//
	reg.Get (&initplaystate, REG_LASTPLAYSTATE);
	if (initplaystate == SYSTRAYPLAYER_STATE_PLAYING)
	{
		// Auto-start playback.
		Command_Play (false);
	}

	return;
}

SystrayPlayer::~SystrayPlayer ()
{
	// Uninitialize
	// ------------

	// Save current player settings and playback status.
	SavePlaylist();

	// Stop playback.
	Command_Stop();

	// Kill Sysicon Timer
	KillTimer (pwnd, SYSICON_TIMER_ID);
	
	// Hide MainWindow & Unload Controls
	Window_Hide();
	if (ctrlinited)
	{
		// Destroy all child control windows
		DestroyWindow (lb);
		DestroyWindow (cmdclear);
		DestroyWindow (cmdspeed);
		DestroyWindow (trkpos);
		DestroyWindow (trkvol);
		DestroyWindow (chkrandom);
		DestroyWindow (chkrepeat);
		DestroyWindow (chkshutdown);
		DestroyWindow (txtrepeat);

		// Delete all Comctls
		delete cmdadd;
		delete cmdremove;
		delete cmdplay;
		delete cmdpause;
		delete cmdstop;
		delete cmdnextfile;
#ifdef SYSTRAYPLAYER_CANEXIT
		delete cmdexit;
#endif
		delete spl_amovie;

		// Status: Uninited
		ctrlinited = false;
	}
	
	// Destroy DC & Window
	pwnd_mem.Free();
	DestroyWindow (pwnd);

	// Unregister Class
	UnregisterClass (WNDCLASS, myinst);

	// Remove Systray Icon
	sysicon.Hide();

	// Free Resources.
	FreeResources();
}


//////////////////////////////////
// Class: Resource Management	//
//////////////////////////////////
void SystrayPlayer::LoadResources ()
{
	L_IDI_EXECUTE = LoadIconRes (myinst, IDI_EXECUTE, 16, 16);
	L_IDI_AMOVIE = LoadIconRes (myinst, IDI_AMOVIE16, 16, 16);
	L_IDI_POWER = LoadIconRes (myinst, IDI_POWER, 16, 16);
	L_IDI_DISK = LoadIconRes (myinst, IDI_DISK, 16, 16);
	L_IDI_ADDFILE = LoadIconRes (myinst, IDI_ADDFILE, 16, 16);
	L_IDI_REMOVE = LoadIconRes (myinst, IDI_REMOVE, 16, 16);
	L_IDI_PLAY = LoadIconRes (myinst, IDI_PLAY, 16, 16);
	L_IDI_PLAYPAUSE = LoadIconRes (myinst, IDI_PLAYPAUSE, 16, 16);
	L_IDI_STOP = LoadIconRes (myinst, IDI_STOP, 16, 16);
	L_IDI_NEXT = LoadIconRes (myinst, IDI_NEXTFILE, 16, 16);
	L_IDI_RESTORE = LoadIconRes (myinst, IDI_RESTORE, 16, 16);
}

void SystrayPlayer::FreeResources ()
{
	DestroyIcon (L_IDI_EXECUTE);
	DestroyIcon (L_IDI_AMOVIE);
	DestroyIcon (L_IDI_POWER);
	DestroyIcon (L_IDI_DISK);
	DestroyIcon (L_IDI_ADDFILE);
	DestroyIcon (L_IDI_REMOVE);
	DestroyIcon (L_IDI_PLAY); 
	DestroyIcon (L_IDI_PLAYPAUSE);
	DestroyIcon (L_IDI_STOP);
	DestroyIcon (L_IDI_NEXT); 
	DestroyIcon (L_IDI_RESTORE);
}


//////////////////////////
// Ramdisk Management	//
//////////////////////////
bool SystrayPlayer::InitRamdisk ()																		// private
{
	ULARGE_INTEGER freespace;
	
	// Check the drive type
	UINT drivetype = GetDriveType(&ramdisk[0]);
	if ((drivetype != DRIVE_RAMDISK) && (drivetype != DRIVE_REMOTE) && (drivetype != DRIVE_FIXED))
	{
		InfoBox (0, LANG_EN ? "Error:\nThe drive you specified is not a valid ramdisk." : 
								"Fehler:\nDas von Ihnen angegebene Laufwerk ist keine gültige Ramdisk und kann nicht verwendet werden.",
							CAPTION, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	
	// Check if there is at least 1 MB free
	if (GetDiskFreeSpaceEx(&ramdisk[0], NULL, NULL, &freespace))
	{
		if (freespace.QuadPart < (1024*1024))
		{
			EnumDir edt;
			string file;
			long u;
			int res;
			
			edt.Enum (CSR(ramdisk, "\\"), "*.*", FS_NAME);
			u = edt.Ubound();
			
			for (long i = 1; i <= u; i++)
			{
				if (!(edt[i].attrib & FILE_ATTRIBUTE_DIRECTORY))
				{
					file = CSR(UCase(ramdisk), "\\", edt[i].file);
					res = InfoBox(0, LANG_EN ? CSR("-- Warning --\n\nThere is not enough free space on your Ramdisk ", UCase(ramdisk), " .\n\nDo you want to delete the following file:\n", file, " ?") : 
												CSR("-- Warnung --\n\nAuf Ihrer Ramdisk ", UCase(ramdisk), " befindet sich nicht genug freier Speicher.\n\nMöchten Sie die folgende Datei löschen:\n", file, " ?"), 
												CAPTION, MB_YESNOCANCEL | MB_DEFBUTTON2 | MB_ICONEXCLAMATION);
					if (res == IDYES)
					{
						EnsureDeleteFile (file);
					}
					else if (res == IDCANCEL)
					{
						return false;
					}
				}
			}
		}
	}
	
	// Ok, Player can use the ramdisk now.
	return true;
}


//////////////////
// Show & Hide	//
//////////////////
void SystrayPlayer::Window_Show()
{
	ShowWindow (pwnd, SW_SHOW);
	if (StyleExists(pwnd, WS_MINIMIZE))
	{
		ShowWindow (pwnd, SW_RESTORE);
	}
	SetForegroundWindowEx (pwnd);
	window_visible = true;
}

void SystrayPlayer::Window_Hide()
{
	SendMessage (pwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
	ShowWindow (pwnd, SW_HIDE);
	window_visible = false;
}

void SystrayPlayer::Window_Toggle()
{
	if (!StyleExists(pwnd, WS_VISIBLE))
	{
		Window_Show();
	}
	else
	{
		Window_Hide();
	}
}


//////////////////////
// Window Process	//
//////////////////////
LRESULT CALLBACK SystrayPlayer::_ext_SystrayPlayer_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	SystrayPlayer* class_ptr = (SystrayPlayer*) GetWindowLongPtr (hwnd, GWLP_USERDATA);
	if (class_ptr != 0)
	{
		return class_ptr->WndProc (hwnd, msg, wparam, lparam);
	}
	return DefWindowProc (hwnd, msg, wparam, lparam);
}

LRESULT CALLBACK SystrayPlayer::WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	double cur_media_time;

	if (msg == WM_PAINT)
	{
		if (ctrlinited)
		{
			pwnd_mem.Refresh();
		}
		return false;
	}
	else if (msg == WM_TIMER)
	{
		if (ctrlinited)
		{
			if (wparam == POSBAR_TIMER_ID)
			{
				if (window_visible)
				{
					cur_media_time = spl_amovie->GetPosition();

					// Update Positioning Bar
					trkSetPos (trkpos, (long) ((cur_media_time * 100) / spl_amovie->GetLength()));

					// Update Window Caption
					ctlSetText (pwnd, CSR(CAPTION, " [", MediaTime(cur_media_time), "]"));
				}
			}
			else if (wparam == SYSICON_TIMER_ID)
			{
				// Tray Icon Timer
				sysicon.Show();
			}
		}
	}
	else if (msg == WM_HSCROLL)
	{
		if (ctrlinited && ((HWND) lparam == trkpos))
		{
			long code = LOWORD(wparam);
			if ((code == TB_THUMBTRACK) || (code == TB_PAGEUP) || (code == TB_PAGEDOWN) || 
				(code == TB_LINEUP) || (code == TB_LINEDOWN) || (code == TB_TOP) || 
				(code == TB_BOTTOM))
			{
				spl_amovie->SetPosition (((double) trkGetPos(trkpos) / 100) * spl_amovie->GetLength());
			}
		}
	}
	else if (msg == WM_VSCROLL)
	{
		if (ctrlinited && ((HWND) lparam == trkvol))
		{
			long code = LOWORD(wparam);
			if ((code == TB_THUMBTRACK) || (code == TB_PAGEUP) || (code == TB_PAGEDOWN) || 
				(code == TB_LINEUP) || (code == TB_LINEDOWN) || (code == TB_TOP) || 
				(code == TB_BOTTOM))
			{
				pl_volume = -trkGetPos(trkvol);
				spl_amovie->SetVolume ((long) (((double) (100 - pl_volume) / 100) * VOLUME_SILENT));
			}
		}
	}
	else if (msg == WM_COMMAND)
	{
		if (ctrlinited)
		{
			if (HIWORD(wparam) == 1)							// Accelerator used
			{
				int accid = LOWORD(wparam);
				if (accid == IDM_ADD)			{ cmdadd->ButtonClick(); }
				else if (accid == IDM_REMOVE)	{ cmdremove->ButtonClick(); }
				else if (accid == IDM_PLAY)
				{
					if (pl_state == SYSTRAYPLAYER_STATE_PLAYING)
					{
						cmdpause->ButtonClick();
					}
					else if ((pl_state == SYSTRAYPLAYER_STATE_PAUSED) || (pl_state == SYSTRAYPLAYER_STATE_STOP))
					{
						cmdplay->ButtonClick();
					}
				}
				else if (accid == IDM_STOP)		{ cmdstop->ButtonClick(); }
				else if (accid == IDM_NEXTFILE)	{ cmdnextfile->ButtonClick(); }
				else if (accid == IDM_EXIT)		{ cmdexit->ButtonClick(); }
			}
			else if (cmdadd->clicked(wparam, lparam))		{ Command_Add(); cmdadd->DrawMe(false); }
			else if (cmdremove->clicked(wparam, lparam))	{ Command_Remove(); cmdremove->DrawMe(false); }
			else if (cmdplay->clicked(wparam, lparam))		{ Command_Play (false); cmdplay->DrawMe(false); }
			else if (cmdpause->clicked(wparam, lparam))		{ Command_Pause(); cmdpause->DrawMe(false); }
			else if (cmdstop->clicked(wparam, lparam))		{ Command_Stop(); cmdstop->DrawMe(false); }
			else if (cmdnextfile->clicked(wparam, lparam))	{ Command_Next(); cmdnextfile->DrawMe(false); }
#ifdef SYSTRAYPLAYER_CANEXIT
			else if (cmdexit->clicked(wparam, lparam))		{ Command_Stop(); TerminateVKernel (false); }
#endif
			
			if (((HWND) lparam == cmdclear) && (HIWORD(wparam) == BN_CLICKED))
			{
				// Stop playback first before attempting to clear playlist.
				Command_Stop();

				// GUI: Disable "remove file" button.
				cmdremove->Enable (false);

				// Clear playlist cache and listbox.
				mentry.Clear();
				lbClear (lb);
			}
			else if (((HWND) lparam == cmdspeed) && (HIWORD(wparam) == BN_CLICKED))
			{
				MegaPopup spsel (13, 6);
				RECT btnrc;
				long hv, s;
				
				// Create menu entries
				for (s = 70; s <= 150; s += 5)
				{
					spsel.Add (((150 - s) / 5) + 1, CSR(CStr(s), " %"));
				}
				
				// Show up the menu
				GetWindowRect (cmdspeed, &btnrc);
				hv = spsel.Show (btnrc.right - 2, btnrc.bottom - 2, 
									MPM_TOPLEFT, MPM_HSCROLL | MPM_VSCROLL);
				
				// Act
				if (hv >= 1)
				{
					pl_curspeed = (double) (150 - ((hv - 1) * 5)) / 100;
					spl_amovie->SetRate (pl_curspeed);
				}
			}
			else if (((HWND) lparam == lb) && (HIWORD(wparam) == LBN_SELCHANGE))
			{
				if (lbSelectedItem(lb) != LB_ERR)
				{
					cmdremove->Enable (true);
				}
				else
				{
					cmdremove->Enable (false);
				}
			}
			else if (((HWND) lparam == chkrepeat) && (HIWORD(wparam) == BN_CLICKED))
			{
				long newstate = chkGetState(chkrepeat);
				EnableWindow (txtrepeat, (newstate == BST_CHECKED));
				if (newstate == BST_CHECKED)
				{
					// Decide if "Shutdown" is available
					EnableWindow (chkshutdown, (CLong(ctlGetText(txtrepeat)) != 0));
				}
				else
				{
					// Shutdown is available
					EnableWindow (chkshutdown, true);
				}
			}
			else if (((HWND) lparam == txtrepeat) && (HIWORD(wparam) == EN_CHANGE))
			{
				EnableWindow (chkshutdown, (CLong(ctlGetText(txtrepeat)) != 0));
			}
		}
	}
	else if (msg == WM_DROPFILES)
	{
		HDROP cur = (HDROP) wparam;
		string buf;
		
		long u = DragQueryFile (cur, 0xFFFFFFFF, NULL, 0) - 1;
		for (long i = 0; i <= u; i++)
		{
			// Retrieve filename
			buf.resize (DragQueryFile(cur, i, NULL, 0) + 1);
			DragQueryFile (cur, i, &buf[0], xlen(buf));
			
			// Add file to Cache & List
			AddNewFile (buf);
		}
		
		// Finish the Drag Operation
		DragFinish (cur);
	}
	else if (msg == MCIWNDM_NOTIFYMODE)
	{
		long evcode;
		LONG_PTR param1, param2;
		
		// Handle the DirectShow Event Queue
		while (spl_amovie->GetEvent(&evcode, &param1, &param2) == S_OK)
		{
			if (evcode == EC_COMPLETE)
			{
				if (spl_amovie->GetPosition() == spl_amovie->GetLength())
				{
					PlayNextFile();
				}
			}
			spl_amovie->FreeEvent (evcode, (long) param1, (long) param2);
		}
	}
	else if (msg == WM_MOVE)
	{
		if (!StyleExists(pwnd, WS_MINIMIZE))
		{
			RECT newrc;
			
			// Get new window coordinates.
			GetWindowRect (pwnd, &newrc);

			// Save if the new coords are valid.
			if (CheckWindowCoords(pwnd, newrc.left, newrc.top))
			{
				reg.Set (newrc.left, REG_WINDOW_X);
				reg.Set (newrc.top, REG_WINDOW_Y);
			}
		}
	}
	else if (msg == WM_CLOSE)
	{
		// Hide the player window.
		Window_Hide();

		// Do not invoke WM_DESTROY.
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
			if (wparam == 1)
			{
				// Session is being SURELY ended after return (this is the MainThread)
				TerminateVKernel (true);
				return false;
			}
		}
	}
	else if (msg == WM_LBUTTONUP)
	{
		if (wparam == 1)
		{
			static bool lock = false;
			if (!lock)
			{
				// Lock
				lock = true;

				// Act
				if (lparam == WM_RBUTTONUP)
				{
					MegaPopup spm(19, 8);
					MegaPopupFade fade;
					POINT pt;
					long hv;
					

					//
					// Create the Menu
					//
					spm.Clear();
					spm.Add (1, LANG_EN ? "Show Player" : "Player anzeigen", L_IDI_RESTORE, MPM_HILITEBOX);
					
					#ifdef _DEBUG
						spm.Add (2, LANG_EN ? "CPU Priority" : "CPU Priorität", L_IDI_EXECUTE, MPM_HILITEBOX | MPM_FADED);
					#endif
					
					spm.Add (3, LANG_EN ? "Save Playlist" : "Playliste speichern", L_IDI_DISK, MPM_HILITEBOX);
					spm.Add (4, "", 0, 0);
					if (ctrlinited)
					{
						if (IsWindowEnabled(cmdstop->hwnd()))
						{
							spm.Add (5, LANG_EN ? "Next file" : "Nächstes Lied", L_IDI_NEXT, MPM_HILITEBOX);
							spm.Add (6, "", 0, 0);
						}
						spm.Add (7, "Play", L_IDI_PLAY, MPM_HILITEBOX | MPM_DISABLED*!IsWindowEnabled(cmdplay->hwnd()));
						spm.Add (8, "Pause", L_IDI_PLAYPAUSE, MPM_HILITEBOX | MPM_DISABLED*!IsWindowEnabled(cmdpause->hwnd()));
						spm.Add (9, "Stop", L_IDI_STOP, MPM_HILITEBOX | MPM_DISABLED*!IsWindowEnabled(cmdstop->hwnd()));
					}

					
					//
					// Show Up the Menu
					//
					GetCursorPos (&pt);
					fade.callback = &_ext_SystrayPlayer_TrayMenuFader;
					fade.data = (long*) this;
					hv = spm.Show (pt.x, pt.y, MPM_RIGHTBOTTOM, MPM_NOSCROLL, &fade);
					
					// Act
					if (hv == 1)
					{
						Window_Show();
					}
					else if (hv == 3)
					{
						SavePlaylist();
					}
					else if (hv == 5)
					{
						Command_Next();
					}
					else if (hv == 7)
					{
						Command_Play (false);
					}
					else if (hv == 8)
					{
						Command_Pause();
					}
					else if (hv == 9)
					{
						Command_Stop();
					}
				}
				else if (lparam == WM_LBUTTONDBLCLK)
				{
					Window_Toggle();
				}

				// Unlock
				lock = false;
			}
		}
	}
	return DefWindowProc (hwnd, msg, wparam, lparam);
}

long SystrayPlayer::_ext_SystrayPlayer_TrayMenuFader (MegaPopup* par, MegaPopup* chi, long entry, long x, long y, long* data)
{
	long hv;
	
	// Create and show the child menu
	chi->Add (1, "Normal", ((SystrayPlayer*) data)->L_IDI_POWER, 0);
	chi->Add (2, "Idle", ((SystrayPlayer*) data)->L_IDI_POWER, 0);
	hv = chi->Show (x, y, MPM_TOPLEFT, MPM_HSCROLL);
	
	// Act
	if (hv == 1)
	{
		SetPriorityClass (GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
	}
	else if (hv == 2)
	{
		SetPriorityClass (GetCurrentProcess(), IDLE_PRIORITY_CLASS);
	}
	
	return hv;
}


//////////////////////////////////
// Player Control Functions		//
//////////////////////////////////
long SystrayPlayer::getPlayerState ()
{
	// NTProtect External Interface.
	return pl_state;
}

void SystrayPlayer::PlayNextFile ()
{
	long res;
	long i;
	long u;

	// (!) SECURITY: Reset Shutdown Allowed flag.
	bShutdownAllowed = false;
	
	// Handle next file to play.
	if (pl_state == SYSTRAYPLAYER_STATE_PLAYING)
	{
		//
		// Close finished file by closing ActiveMovie Player and kill the POSBAR timer.
		//
		my_amovie_close (pwnd);
		KillTimer (pwnd, POSBAR_TIMER_ID);
		

		//
		// If the last file played back has a valid index, mark it as done.
		//
		if ((pl_curfile > 0) && (pl_curfile <= mentry.Ubound()))
		{
			// Last playback index is valid, mark the file as done.
			mentry[pl_curfile].bRandomHasPlayed = true;
		}


		//
		// Any file left for playback?
		// What is the next file for playback?
		// --> Function "DecideNextFileToPlay" will return FALSE if no file is left.
		// --> If any file should be played next, the function will return TRUE and set "pl_curfile".
		//
		if (DecideNextFileToPlay(&pl_curfile,    (chkGetState(chkrandom) == BST_CHECKED)	))
		{
			// Open ActiveMovie Player again.
			res = my_amovie_open (mentry[pl_curfile].strFile, pwnd, MCIWNDM_NOTIFYMODE);
			if (res == S_OK)
			{
				// Open file succeeded
				spl_amovie->Play();
				spl_amovie->SetVolume ((long) (((double) (100 - pl_volume) / 100) * VOLUME_SILENT));
				spl_amovie->SetRate (pl_curspeed);
				
				ChangePlayerState (SYSTRAYPLAYER_STATE_PLAYING);
				lbSelectSingle (lb, pl_curfile - 1);		// ListBox uses zero-based index.
				cmdremove->Enable (true);
				
				// Set the Positioning Timer
				trkSetPos (trkpos, 0);
				SetTimer (pwnd, POSBAR_TIMER_ID, 1000, NULL);
			}
			else if ((res == VFW_E_INVALID_FILE_FORMAT) || 
						(res == VFW_E_UNKNOWN_FILE_TYPE) || 
						(res == VFW_E_UNSUPPORTED_STREAM) || 
						(res == VFW_E_NOT_FOUND) ||
						(res == VFW_E_FILEMISSING))
			{
				// Error: File not found or Invalid file type (296)
				// -> Automatically plays next file because the current file is being removed
				// <old> PlayNextFile();
				lbSelectSingle (lb, pl_curfile - 1);
				Command_Remove();
			}
		}
		else
		{
			//
			// ---------------------------------------------------------------
			// EVENT: We reached the end of the playlist ("PlaybackFinished").
			// ---------------------------------------------------------------
			//
			// Stop playback.
			Command_Stop();

			// Enter CS.
			cs.Enter();

			// Reset random markers for all files in playlist cache (mentry).
			u = mentry.Ubound();
			for (i = 1; i <= u; i++)
			{
				mentry[i].bRandomHasPlayed = false;				// Indicate file has not been played back yet in random-mode.
			}

			// Leave CS.
			cs.Leave();
			
			// (!) SECURITY: Reset Shutdown Allowed flag.
			bShutdownAllowed = false;

			// Handle finished playback of playlist.
			if (chkGetState(chkrepeat) == BST_CHECKED)
			{
				//
				// User selected: REPEAT MODE ON
				//
				long repcount = CLong(ctlGetText(txtrepeat));

				// "pl_curloop": Indicates how often the list had been played back.
				pl_curloop++;

				// Check if desired "repeat count" has been reached.
				if (repcount == 0)
				{
					// User selected: INFINITE LOOP
					// Start playback over again.
					Command_Play (false);
				}
				else if (pl_curloop < (repcount + 1))
				{
					// User selected: LOOP <repcount> TIMES.
					// We are not finished with <repcount> times yet, so start over again.
					Command_Play (true);
				}
				else if (pl_curloop >= (repcount + 1))
				{
					// User select: LOOP <repcount> TIMES.
					// We are done and finished playback for the last time.
					//
					// Is Shutdown Active ?
					if ((IsWindowEnabled(chkshutdown)) && (chkGetState(chkshutdown) == BST_CHECKED))
					{
						// SECURITY: Set ShutdownAllowed Flag to allow normal termination of NTProtect if it is active.
						bShutdownAllowed = true;

						// Shutdown windows.
						ShellExecute (0, CSR(GetSystemDir32(), "\\shutdown.exe -s -f -t 0"));
					}
				}
			}
			else if ((IsWindowEnabled(chkshutdown)) && (chkGetState(chkshutdown) == BST_CHECKED))
			{
				//
				// User selected: REPEAT MODE OFF
				//

				// SECURITY: Set ShutdownAllowed Flag to allow normal termination of NTProtect if it is active.
				bShutdownAllowed = true;

				// Shutdown windows.
				ShellExecute (0, CSR(GetSystemDir32(), "\\shutdown.exe -s -f -t 0"));
			}
		
		}
	}
}

// private
bool SystrayPlayer::DecideNextFileToPlay (long* in_out_lngCacheIndex, bool bRandomModeOn)
{
	//
	// Return Values: Returns ...
	//
	//			* TRUE if "retNextCacheIndex" contains the index of the next file to play.
	//			* FALSE if there is no next file to play.
	//
	// Variables.
	long i;
	long u;
	bool bAllFilesPlayed;
	SYSTEMTIME systime;


	//
	// Check playback mode.
	//
	if (bRandomModeOn)
	{
		// 
		// User selected: RANDOM PLAYBACK-MODE.
		//
		// Algorithm for next-file-decision:
		//
		// 1. Check if any file has not been played yet. If all files had played already, abort.
		// 2. Seek randomly for the next-file-to-play and return it.
		//

		// Assume all files have played yet.
		bAllFilesPlayed = true;

		// Enter CS.
		cs.Enter();

		// Step 1: Check if all files have played yet. (Also triggers, when playlist is empty!)
		u = mentry.Ubound();
		for (i = 1; i <= u; i++)
		{
			if (!mentry[i].bRandomHasPlayed)
			{
				// One file has not been played back yet.
				bAllFilesPlayed = false;
				break;
			}
		}

		// Leave CS.
		cs.Leave();

		// Abort if all files had been played back and no one is left.
		if (bAllFilesPlayed)
		{
			// Another playback loop just finished and there is no file left.
			return false;
		}
		else
		{
			// Set seed for random number generation.
			GetSystemTime (&systime);
			srand ((systime.wHour * 100000) + (systime.wMinute * 1000) + (systime.wMilliseconds));

			// Enter CS.
			cs.Enter();

			// Create random numbers until a not-played-back file is found in the playlist cache (mentry).
			do
			{
				//
				// Obtain random number between a and b:
				//		a + (rand() % ( b - a + 1 ));
				//
				*in_out_lngCacheIndex = (1 + rand() % (mentry.Ubound() - 1 + 1));
				if ((*in_out_lngCacheIndex <= 0) || (*in_out_lngCacheIndex > mentry.Ubound()))
				{
					// Fix invalid random number results.
					// This should never be necessary, but it's still here.
					*in_out_lngCacheIndex = 1;		
				}
			} while (mentry[*in_out_lngCacheIndex].bRandomHasPlayed);

			// Leave CS.
			cs.Leave();

			// "in_out_lngCacheIndex" now contains next-file-to-play.
			return true;
		}
	}
	else
	{
		// 
		// User selected: NORMAL, LINEAR PLAYBACK-MODE.
		//
		if ((*in_out_lngCacheIndex + 1) <= mentry.Ubound())
		{
			// Advance to next file.
			(*in_out_lngCacheIndex)++;
			return true;
		}
		else
		{
			// Another playback loop just finished and there is no file left.
			return false;
		}
	}
}

void SystrayPlayer::Command_Play (bool noresetcount)									// public
{
	long idx;

	// (!) SECURITY: Reset Shutdown Allowed flag.
	bShutdownAllowed = false;

	// Get the current Player State
	if (pl_state == SYSTRAYPLAYER_STATE_STOP)
	{
		if (mentry.Ubound() > 0)	
		{
			// Reset counted Playlist Loops only if necessary.
			if (!noresetcount)
			{
				pl_curloop = 0;
			}

			// (?) Is any file selected for starting or should we use the first file
			idx = lbSelectedItem(lb);
			if (idx != LB_ERR)
			{
				// Use the file selected by the user to begin
				pl_curfile = idx;
			}
			else
			{
				// Use the first file from the play list
				pl_curfile = 0;
			}
			ChangePlayerState (SYSTRAYPLAYER_STATE_PLAYING);
			PlayNextFile();
		}
	}
	else if (pl_state == SYSTRAYPLAYER_STATE_PAUSED)
	{
		if (spl_amovie->GetPosition() != spl_amovie->GetLength())
		{
			// No EOF - simply resume
			spl_amovie->Play();
			ChangePlayerState (SYSTRAYPLAYER_STATE_PLAYING);
		}
		else
		{
			// The EOF has already been reached, resume does not work!
			ChangePlayerState (SYSTRAYPLAYER_STATE_PLAYING);
			PlayNextFile();
		}
	}
	else if (pl_state == SYSTRAYPLAYER_STATE_PLAYING)
	{
		idx = lbSelectedItem(lb);
		if (idx != LB_ERR)
		{
			idx++;
			if (pl_curfile == idx)
			{
				spl_amovie->Home();
				spl_amovie->Play();
			}
			else
			{
				pl_curfile = idx - 1;
				PlayNextFile();
			}
		}
	}
}

void SystrayPlayer::Command_Pause ()											// public
{
	// (!) SECURITY: Reset Shutdown Allowed flag.
	bShutdownAllowed = false;

	// Handle pausing the player.
	if (pl_state == SYSTRAYPLAYER_STATE_PLAYING)
	{
		ChangePlayerState (SYSTRAYPLAYER_STATE_PAUSED);
		spl_amovie->Pause();
	}
}									// public

void SystrayPlayer::Command_Stop ()
{
	// Note: Do not reset Shutdown Allowed Flag here.

	// Handle stopping the player.
	if ((pl_state == SYSTRAYPLAYER_STATE_PLAYING) || (pl_state == SYSTRAYPLAYER_STATE_PAUSED))
	{
		// Change Player State and close ActiveMovie Player.
		ChangePlayerState (SYSTRAYPLAYER_STATE_STOP);
		spl_amovie->Stop();
		my_amovie_close (pwnd);
		
		// Stop Positioning Timer.
		KillTimer (pwnd, POSBAR_TIMER_ID);
		ctlSetText (pwnd, CAPTION);
		
		// Update GUI and player vars:
		//		* Remove current playback selection and 
		//		* Reset the current filename pointer to beginning of playlist.
		pl_curfile = 0;
		lbSelectSingle (lb, -1);
	}
}									// public

void SystrayPlayer::Command_Next ()
{
	// (!) SECURITY: Reset Shutdown Allowed flag.
	bShutdownAllowed = false;

	// Handle play next file.
	if (pl_state == SYSTRAYPLAYER_STATE_PLAYING)
	{
		PlayNextFile();
	}
	else if (pl_state == SYSTRAYPLAYER_STATE_PAUSED)
	{
		Command_Play (false);		// false is unnecessary because reset function is not reached
		PlayNextFile();
	}
}									// public

// public
bool SystrayPlayer::IsShutdownAllowed ()
{
	return bShutdownAllowed;
}

void SystrayPlayer::ChangePlayerState (long newstate)
{
	if (newstate == SYSTRAYPLAYER_STATE_PLAYING)
	{
		cmdpause->SetCaption ("&Pause");
		cmdplay->SetCaption ("Play");
		cmdpause->Enable (true);
		cmdstop->Enable (true);
		cmdnextfile->Enable (true);
	}
	else if (newstate == SYSTRAYPLAYER_STATE_PAUSED)
	{
		cmdpause->SetCaption ("Pause");
		cmdplay->SetCaption ("&Play");
		cmdpause->Enable (false);
		cmdnextfile->Enable (true);
	}
	else if (newstate == SYSTRAYPLAYER_STATE_STOP)
	{
		cmdpause->SetCaption ("Pause");
		cmdplay->SetCaption ("&Play");
		cmdpause->Enable (false);
		cmdstop->Enable (false);
		cmdnextfile->Enable (false);
	}
	pl_state = newstate;
}


//////////////////////////
// User Interface		//
//////////////////////////
void SystrayPlayer::AddNewFile (string strFile)
{
	MusicObj moAdd;

	// Check if we got a new FILE.
	if (!IsDirectory(strFile))
	{
		moAdd.strFile = strFile;
		moAdd.strName = GetFilenameFromCall(CSR("\"", strFile, "\""));
		moAdd.bRandomHasPlayed = false;										// File did not play in random-mode yet.
		
		// Append to cache (mentry).
		mentry.Append(moAdd);

		// Append to ListBox (GUI).
		lbAdd (lb, moAdd.strName);
	}
}

void SystrayPlayer::Command_Add()
{
	string file = cdlgOpenFile (pwnd, LANG_EN ? "Add Multimedia File ..." : "Multimediadatei hinzufügen ...", "\\", 
									CDLG_FILTER_MULTIMEDIA_FILES, 5);
	if (file.length() != 0)
	{
		if (FileExists(file))
		{
			AddNewFile (file);
		}
		else
		{
			InfoBox (pwnd, "Error: File does not exist !", "Unable to add file", MB_OK | MB_ICONSTOP);
		}
	}
}

void SystrayPlayer::Command_Remove()
{
	long idx = lbSelectedItem(lb);
	bool playnext = false;
	
	// Index Valid?
	if (idx != LB_ERR)
	{
		idx++;
		
		if (pl_state == SYSTRAYPLAYER_STATE_PLAYING)
		{
			if (idx < pl_curfile)			
			{
				// Selected Item is BEFORE the Current Item
				pl_curfile--;
			}
			else if (idx == pl_curfile)		
			{
				// Selected Item IS the Current Item
				pl_curfile--;
				playnext = true;
			}
		}
		else if (pl_state == SYSTRAYPLAYER_STATE_PAUSED)
		{
			Command_Stop();
		}
		
		// ------------------------------------------------------------
		// Remove currently selected item from Playlist Cache (mentry).
		// ------------------------------------------------------------
		if ((idx >= 0) && (idx <= mentry.Ubound()))
		{
			mentry.Remove (idx);
			lbRemove (lb, idx - 1);		// (!) Zero based

			// Disable "cmdRemove" button if no selected item is left for removal.
			if (lbSelectedItem(lb) == LB_ERR)
			{
				cmdremove->Enable (false);
			}
		}


		//
		// If marker is set (because we deleted the currently playing file), continue with playing the next file from the list.
		//
		if (playnext)
		{
			PlayNextFile();
		}
	}
}


//////////////////////////////
// Additional Functions		//
//////////////////////////////
string SystrayPlayer::MediaTime (double pos)
{
	double mins, hours;
	string ss, mm, hh;
	if (pos < 0) { pos = 0; };
	
	hours = floor(pos / 3600);
	mins = floor((pos - (hours * 3600)) / 60);
	
	ss = FillStr(CStr((long) ((pos - (hours * 3600)) - (mins * 60))), 2, '0');
	mm = FillStr(CStr((long) mins), 2, '0');
	hh = CStr((long) hours);
	
	// Check if we have reached an hour, so we need the HOURS value in front of everything
	if (hours < 1)
	{
		return CSR(mm, ":", ss);
	}
	else
	{
		return CSR(hh, ":", mm, ":", ss);
	}
}


//////////////////////////
// Playlist Storage		//
//////////////////////////
void SystrayPlayer::SavePlaylist ()
{
	//
	// Purpose: This function immediately stores current playlist and playback condition to the registry.
	//

	// Variables.
	long i, u;
	
	// First, clear all stored entries of previous playlist snapshot.
	reg.Get (&u, REG_LISTCOUNT);
	for (i = 1; i <= u; i++)
	{
		reg.Delete (CSR(REG_PLAYLIST_PREFIX, FillStr(CStr(i), 4, '0')));
	}

	// Enter CS.
	cs.Enter();
	
	// Save all playlist entries from memory to registry.
	u = mentry.Ubound();
	reg.Set (u, REG_LISTCOUNT);
	for (i = 1; i <= u; i++)
	{
		reg.Set (mentry[i].strFile, CSR(REG_PLAYLIST_PREFIX, FillStr(CStr(i), 4, '0')));
	}

	// Leave CS.
	cs.Leave();


	//
	// Save index of file that is currently playing.
	//
	reg.Set (pl_curfile, REG_LASTPLAYINDEX);


	//
	// Save current playback settings:
	//
	//			* state
	//			* volume
	//			* random mode
	//			* repeat count
	//
	reg.Set (pl_state, REG_LASTPLAYSTATE);
	reg.Set (pl_volume, REG_VOLUME);
	reg.Set ((chkGetState(chkrandom) == BST_CHECKED), REG_RANDOMMODE);
	reg.Set ((chkGetState(chkrepeat) == BST_CHECKED), REG_REPEAT);

	return;
}


//////////////////////////
// Ramdisk Support		//
//////////////////////////
long SystrayPlayer::my_amovie_open (string file, HWND notifywnd, long message, long param)
{
	// Load the file into the Ramdisk if necessary
	if (runmode & MODE_RAMDISK)
	{
		string newfile = CSR(ramdisk, "\\", GetFilenameFromCall(CSR(Chr(34), file, Chr(34))));
		if (CopyFile(&file[0], &newfile[0], true))
		{
			file = newfile;
			rd_lastfile = newfile;
		}
	}
	
	// Complete the ActiveMovie->Open request
	return spl_amovie->Open (file, notifywnd, message, param);
}

void SystrayPlayer::my_amovie_close (HWND notifywnd)
{
	// Complete the ActiveMovie->Close request
	spl_amovie->Close (notifywnd);
	
	// Delete the file from the Ramdisk if necessary
	if (runmode & MODE_RAMDISK)
	{
		if (FileExists(rd_lastfile) && (xlen(rd_lastfile) != 0))
		{
			EnsureDeleteFile (rd_lastfile);
		}
	}
}


