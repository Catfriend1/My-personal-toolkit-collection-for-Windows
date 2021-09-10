// MegaPopup.cpp: Using an alternate Popup Menu
//
// Thread-Safe: YES
//

//////////////////
// Includes		//
//////////////////
#include "stdafx.h"
#include "math.h"

#include "_modules.h"


//////////////////////
// Consts			//
//////////////////////
// const int MPM_TOPSKIP		= 17;				// [18] [17]
// const int MPM_FONTSIZE		= 7;				// [8] [7]
// const int TS = MPM_TOPSKIP;

const int MPM_NOTHING		= -1;
const int MPM_ESC			= -2;
const int MPM_PREV			= -3;
const int MPM_SELECT		= -4;
const int MPM_THREADWAIT	= -5;


//////////////////////////////
// Consts: Align Flags		//
//////////////////////////////
const int MPM_TOPLEFT		= 1;				// [1]
const int MPM_RIGHTBOTTOM	= 2;				// [2]


//////////////////////////////
// Consts: Align Flags		//
//////////////////////////////
const int MPM_NOSCROLL		= 0;				// [0]
const int MPM_HSCROLL		= 1;				// [1]
const int MPM_VSCROLL		= 2;				// [2]


//////////////////////////////
// Consts: Item Flags		//
//////////////////////////////
const int MPM_DISABLED		= 1;				// [1]
const int MPM_FADED			= 2;				// [2]
const int MPM_HILITEBOX		= 4;				// [4]


//////////////////////
// Class Functions	//
//////////////////////
const char MegaPopup::WNDCLASS[]				= "***MegaPopup";
const char MegaPopup::SFONT_MSSANS[]			= "MS Sans Serif";
const char MegaPopup::SFONT_WEBDINGS[]			= "Webdings";
const int  MegaPopup::MAX_MENUITEMS				= 500;					// [500]

MegaPopup::MegaPopup (long vtopskip, long vfontsize)
{
	Init(vtopskip, vfontsize);
}

MegaPopup::~MegaPopup ()
{
	Clear();
	UnInitPens();
}

void MegaPopup::Init(long vtopskip, long vfontsize)													// private
{
	lock = false;
	scrolling = false;
	quoteMouse = false;
	parent = 0;
	jumpto = 0;
	lastmb = 0;
	
	// Configure Constants
	TS = vtopskip;
	MPM_FONTSIZE = vfontsize;
	
	// Initializing Cache
	tme.Redim (MAX_MENUITEMS); nextone = 1;
	for (long i = 0; i <= MAX_MENUITEMS; i++)
	{
		tme[i].used = false;
	}
	InitPens();
}


//////////////////////////
// Graphical Objects	//
//////////////////////////
void MegaPopup::InitPens ()																			// private
{
	FONT_MSSANS = CreateFont (MPM_FONTSIZE, 0, 0, 0, FW_NORMAL, false, false, false, 
					DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_LH_ANGLES, PROOF_QUALITY, 4, SFONT_MSSANS);
}

void MegaPopup::UnInitPens ()																		// private
{
	DeleteObject (FONT_MSSANS);
}


//////////////////////////////
// Add/Delete Entries		//
//////////////////////////////
void MegaPopup::Add (long index, string text, HICON icon, int flags)								// public
{
	bool IsSeparator = false;
	
	// Enter Critical Section
	cs.Enter();

	// Check for Seperator
	if (xlen(text) == 0)
	{
		IsSeparator = true;
	}
	
	// Autogenerate new index if necessary
	if (index == -1)
	{
		index = nextone;
	}	
	
	// Check if Index is valid
	if ((index >= 0) && (index <= MAX_MENUITEMS))
	{
		// Copy Data to User Interface Cache
		tme[index].text = text;
		tme[index].icon = icon;
		tme[index].flags = flags;
		nextone = index + 1;
		tme[index].used = true;
	}

	// Leave Critical Section
	cs.Leave();
}

void MegaPopup::Delete (long index)															// public
{
	// Enter Critical Section
	cs.Enter();
	
	// Check if Index is valid
	if ((index >= 0) && (index <= MAX_MENUITEMS))
	{
		tme[index].used = false;
	}

	// Leave Critical Section
	cs.Leave();
}

void MegaPopup::Clear ()																	// public
{
	// Enter Critical Section
	cs.Enter();

	// Delete all entries one by one
	for (long i = 0; i <= MAX_MENUITEMS; i++)
	{
		Delete (i);
	}
	nextone = 1;

	// Leave Critical Section
	cs.Leave();
}


//////////////////////////
// User Interface		//
//////////////////////////
long MegaPopup::EntryCount ()																// public
{
	long c = 0;

	// Enter Critical Section
	cs.Enter();

	// Count all indexes marked as USED
	for (long i = 0; i <= MAX_MENUITEMS; i++)
	{
		if (tme[i].used) { c++; };
	}

	// Leave Critical Section
	cs.Leave();

	// Return Entry Count
	return c;
}

string MegaPopup::LongestEntry ()															// public
{
	long l = 0, idx = 0;
	string ret;

	// Enter Critical Section
	cs.Enter();

	// Detect the longest entry
	for (long i = 0; i <= MAX_MENUITEMS; i++)
	{
		if ((tme[i].used) && (xlen(tme[i].text) > l))
		{
			l = xlen(tme[i].text);
			idx = i;
		}
	}
	ret = (idx != 0) ? tme[idx].text : "";

	// Leave Critical Section
	cs.Leave();

	// Return Entry
	return ret;
}

string MegaPopup::Entry (long index)														// public
{
	string ret;
	
	// Check if Index is valid
	if (!((index >= 0) && (index <= MAX_MENUITEMS))) { FatalAppExit (0, "MegaPopup::Entry->Invalid Index"); return ""; };
	
	// Enter Critical Section
	cs.Enter();

	// Fetch entry from Cache
	ret = tme[index].text;

	// Leave Critical Section
	cs.Leave();

	// Return Entry
	return ret;
}

long MegaPopup::Flags (long index)															// public
{
	long ret;

	// Check if Index is valid
	if (!((index >= 0) && (index <= MAX_MENUITEMS))) { FatalAppExit (0, "MegaPopup::Flags->Invalid Index"); return 0; };
	
	// Enter Critical Section
	cs.Enter();

	// Fetch entry flags from Cache
	ret = tme[index].flags;

	// Leave Critical Section
	cs.Leave();

	// Return Flags of Entry
	return ret;
}

long MegaPopup::GetWidth ()																	// public
{
	HDC tdc = DisplayDC();
	long width_of_longest_entry;

	// Enter Critical Section
	cs.Enter();

	// Determine Width of Longest Entry
	width_of_longest_entry = GetTextWidth(tdc, FONT_MSSANS, LongestEntry()) + 64;
	DeleteDC (tdc);

	// Leave Critical Section
	cs.Leave();

	// Return Menu Width
	return width_of_longest_entry;
}

long MegaPopup::GetHeight ()																// public
{
	long height;

	// CS
	cs.Enter();
	height = (EntryCount() * TS) + 7;
	cs.Leave();

	// Return Menu Height
	return height;
}


//////////////////////////
// Alternate Show		//
//////////////////////////
long MegaPopup::ShowAlternative (HWND hwnd, long x, long y, int align)						// public
{
	HMENU hmenu;
	MENUITEMINFO mii;
	HWND oldactwnd = 0;
	long flags, res;

	// Enter Critical Section
	cs.Enter();
	
	// Create MenuItems
	hmenu = CreatePopupMenu();
	for (long i = 0; i <= MAX_MENUITEMS; i++)
	{
		if (tme[i].used)
		{
			mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID;
			
			// Item Text
			if (xlen(tme[i].text) != 0)
			{
				mii.fType = MFT_STRING;
			}
			else
			{
				mii.fType = MFT_SEPARATOR;
			}
			mii.dwTypeData = &tme[i].text[0];
			mii.cch = xlen(tme[i].text);
			
			// Item State
			mii.fState = (MFS_ENABLED * (!Flag(tme[i].flags, MPM_DISABLED))) | 
							((MFS_DISABLED | MFS_GRAYED) * Flag(tme[i].flags, MPM_DISABLED));
			
			// Item data
			mii.wID = i;
			mii.hSubMenu = 0;
			mii.hbmpChecked = 0;
			mii.hbmpUnchecked = 0;
			mii.dwItemData = 0;
			mii.cbSize = sizeof(mii);
			
			InsertMenuItem (hmenu, 0, 0, &mii);
		}
	}
	
	// Set Menuflags
	flags = TPM_LEFTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD;
	if (align == MPM_RIGHTBOTTOM)
	{
		flags |= (TPM_HORNEGANIMATION | TPM_VERNEGANIMATION);
	}
	else
	{
		flags |= (TPM_HORPOSANIMATION | TPM_VERPOSANIMATION);
	}
	
	// Show up the Menu
	oldactwnd = ActivateWindow (hwnd);
	res = TrackPopupMenu(hmenu, flags, x, y, 0, hwnd, 0);
	DeActivateWindow (oldactwnd);
	DestroyMenu (hmenu);

	// Leave Critical Section
	cs.Leave();

	// Return Result
	return res;
}

long MegaPopup::ShowWindowMenu (HWND hwnd, int menuidx, MegaPopupFade* fade)						// public
{
	RECT rc;
	HMENU wndmenu = GetMenu(hwnd);
	long res;

	// Decrement Menu IDX
	menuidx--;
	
	// Enter Critical Section
	cs.Enter();

	// Show Window Menu
	GetMenuItemRect (hwnd, wndmenu, menuidx, &rc);
	HiliteMenuItem (hwnd, wndmenu, menuidx, MF_BYPOSITION | MF_HILITE);
	res = Show(rc.left, rc.bottom, MPM_TOPLEFT, MPM_VSCROLL, fade);
	HiliteMenuItem (hwnd, wndmenu, menuidx, MF_BYPOSITION | MF_UNHILITE);

	// Leave Critical Section
	cs.Leave();

	// Return Result
	return res;
}


//////////////////////////
// Main Popup			//
//////////////////////////
void MegaPopup::CalcSizeAlign (long x, long y, int align)											// private
{
	//
	// No CS needed, because of: PRIVATE FUNCTION
	//
	long sw = VirtualScreenWidth();
	long sh = VirtualScreenHeight();
	long u = mc.Ubound();
	
	me.w = GetWidth();				// Space for all entries and icons
	me.h = (u * TS) + 7;
	if (align & MPM_RIGHTBOTTOM)
	{
		me.x = x - me.w; me.y = y - me.h;
	}
	else
	{
		me.x = x; me.y = y;
	}
	
	if ((me.x + me.w) > sw) { me.x = sw - me.w; };		// (x)
	if (me.x < 0) { me.x = 0; };						// (^)
	if ((me.x + me.w) > sw) { me.x = 0; };				// (!)
	if (me.h > sh)				// FullScreen scroll needed?
	{
		fsb = true;
		scrollspeed = 50;					// [50]
		scrollrate = Round(0.25 * u);		// [0.25]
		// (I)  scrollspeed = 50;
		// (II) scrollrate = 0.25 * mc.Ubound();
		// (I)  scrollspeed = 0.25 * mc.Ubound();
		// (II) scrollrate = 0.8 * (me.h / (200 - scrollspeed)) + 1;
		if (scrollrate <= 0) {scrollrate = 10; };
		me.y = 0;
		me.h = sh;
	}
	else
	{
		fsb = false;
		if ((me.y + me.h) > sh) { me.y = sh - me.h; };	// (y)
		if (me.y < 0) { me.y = 0; };					// (^)
		if ((me.y + me.h) > sh) { me.y = 0; };			// (!)
	}
}

long MegaPopup::Show (long x, long y, int align, int seffect, MegaPopupFade* fade)						// public
{
	MSG msg;
	long u = 0;
	long menures = MPM_NOTHING;
	HWND oldactwnd = 0;

	// Enter Critical Section
	cs.Enter();
	
	// Initialize Fader if necessary
	if (fade == 0)
	{
		fx.callback = 0;		// (!) Item is NOT faded
	}
	else
	{
		fx.callback = fade->callback;
		fx.data = fade->data;
	}
	
	// Copy data from ItemCache
	mc.Clear();
	for (long i = 0; i <= MAX_MENUITEMS; i++)
	{
		if (tme[i].used)
		{
			u++; mc.Redim (u);
			mc[u].idx = i;
			mc[u].icon = tme[i].icon;
			mc[u].flags = tme[i].flags;
			mc[u].text = tme[i].text;
			
			// If it is a separator, no icon will be assigned
			if (xlen(mc[u].text) == 0) { mc[u].icon = 0; };
		}
	}
	
	// Calculate Size & Align of Popup Window
	CalcSizeAlign (x, y, align);
	
	// Register & Create Popup Window
	NewWindowClass (WNDCLASS, &Callback_WndProc, 0, CS_OWNDC, 0, 0, 0, true);
	fwnd = CreateWindowEx (WS_EX_TOPMOST | WS_EX_DLGMODALFRAME | WS_EX_TOOLWINDOW, 
				WNDCLASS, WNDCLASS, 
				WS_BORDER, 
				me.x, me.y, me.w, me.h, 0, 0, 0, 0);
	SetWindowLongPtr (fwnd, GWLP_USERDATA, (LONG_PTR) this);
	if (fwnd == 0)
	{
		InfoBox (0, "MegaPopup::CreateWindow->General Failure", "", MB_OK | MB_SYSTEMMODAL | MB_ICONSTOP);
		
		// Leave Critical Section before return
		cs.Leave();
		return MPM_ESC;
	}
	RemoveStyle (fwnd, WS_CAPTION);
	
	// Create Drawing Surface
	fdc = fwnd_mem.Create (fwnd, me.w, me.h);
	SetBkMode (fdc, TRANSPARENT);
	
	// First Draw = Init
	sel = 0;
	CalcCoords();
	RefreshAllItems();	
	
	// Perform Effect if necessary
	if (seffect != MPM_NOSCROLL)
	{
		oldactwnd = PerformEffect(seffect, align);
	}
	else
	{
		fwnd_mem.Refresh();
		oldactwnd = ActivateWindow (fwnd);
		ShowWindow (fwnd, SW_SHOW);
	}
	
	// Window Procedure
	threadres = MPM_THREADWAIT; threaddata = 0;
	while (GetMessage (&msg, 0, 0, 0))					// 2010-02-21: GetMessage put in While-Statement
	{
		if (msg.hwnd != fwnd)
		{
			if (msg.message == WM_NCLBUTTONDOWN) { threadres = MPM_ESC; break; };
		}
		DispatchMessage (&msg);
		if (threadres != MPM_THREADWAIT) { break; };	
	}
	
	// De-Activate the Window properly
	DeActivateWindow (oldactwnd);
	
	// Get Result
	if (threadres == MPM_SELECT)
	{
		menures = threaddata;
	}
	else
	{
		menures = threadres;
	}
	
	// Clean Up
	fwnd_mem.Free();
	DestroyWindow (fwnd);
	UnregisterClass (WNDCLASS, 0);

	// Leave Critical Section
	cs.Leave();

	// Return Result
	return menures;
}

long MegaPopup::GetButton ()																		// public
{
	// No CS because: THIS IS AN ATOMIC OPERATION
	return lastmb;
}


//////////////////////
// Blitting			//
//////////////////////
void MegaPopup::CalcCoords ()																		// private
{
	//
	// No CS needed, because of: PRIVATE FUNCTION
	//
	long u = mc.Ubound();

	mc[0].y = 0; mc[0].h = 0;
	for (long i = 1; i <= u; i++)
	{
		mc[i].x = 0;
		mc[i].y = mc[i-1].y + mc[i-1].h;		// mc[i].y = (i - 1) * TS;
		mc[i].w = me.w - 2;
		mc[i].h = TS;
	}
}

void MegaPopup::Scroll (long how)																	// private
{
	//
	// No CS needed, because of: PRIVATE FUNCTION
	//
	long u = mc.Ubound();

	for (long i = 1; i <= u; i++)
	{
		mc[i].y = mc[i].y + how;
	}
	RefreshAllItems ();
}

void MegaPopup::RedrawItem (long o)																	// private
{
	//
	// No CS needed, because of: PRIVATE FUNCTION
	//
	// Check if Index is Valid
	if ((o >= 1) && (o <= mc.Ubound()))
	{
		long ix = mc[o].x, iy = mc[o].y, iw = mc[o].w, ih = mc[o].h;
		long middle = (long) floor ((double) ih / 2);
		
		if (xlen(mc[o].text) != 0)
		{
			// (!) Item is NOT a Separator
			if (o != sel)
			{
				// (!) Item is NOT Selected
				FillRect2 (fdc, ix, iy, iw, ih, (HBRUSH) COLOR_WINDOW);
				if (!(mc[o].flags & MPM_DISABLED))
				{
					// (!) Item IS Activated
					SetTextColor (fdc, 0);
				}
				else
				{
					// (!) Item IS DeActivated
					SetTextColor (fdc, RGB(100, 100, 100));
				}
			}
			else
			{
				// (!) Item IS Selected
				DrawHilite(o);
				if (!(mc[o].flags & MPM_DISABLED))
				{
					// (!) Item IS Activated
					SetTextColor (fdc, RGB(255, 255, 255));
				}
				else
				{
					// (!) Item IS DeActivated
					SetTextColor (fdc, RGB(135, 135, 135));
				}
			}
			
			if (mc[o].icon != 0)
			{
				// (!) Icon HAS been Assigned
				if (!(mc[o].flags & MPM_DISABLED))
				{
					// (!) Item IS Activated
					TransIcon (fdc, mc[o].icon, ix + 4, iy + (middle - MPM_FONTSIZE), 16, 16);
				}
				else
				{
					// (!) Item IS DeActivated
					InvIcon (fdc, mc[o].icon, ix + 4, iy + (middle - MPM_FONTSIZE), 16, 15);
				}
			}
			
			FontOut (fdc, ix + 25, iy + (middle - MPM_FONTSIZE) + 1, mc[o].text, FONT_MSSANS);
			
			if (mc[o].flags & MPM_FADED)
			{
				// (!) Item IS being Faded
				if (o != sel)
				{
					// (!) Item is NOT Selected
					DrawFontChar (fdc, ix + iw - 20, iy + middle - 11, 52, SFONT_WEBDINGS, 
						20, 0);
				}
				else
				{
					// (!) Item IS Selected
					DrawFontChar (fdc, ix + iw - 20, iy + middle - 11, 52, SFONT_WEBDINGS, 
						20, RGB(255,255,255));
				}
			}
		}
		else
		{
			// (!) Item IS a Separator
			FillRect2 (fdc, ix, iy, iw, ih, (HBRUSH) COLOR_WINDOW);
			Line2 (fdc, ix + 3, iy + middle - 1, 
						ix + iw - 6, iy + middle - 1, sc.PGREY);
			Line2 (fdc, ix + 3, iy + middle, 
						ix + iw - 6, iy + middle, sc.PWHITE);
		}
		
		/*
		if (xlen(mc[o].text) != 0)			// Is Separator?
		{
			if (mc[o].icon != 0)
			{
				TransIcon (fdc, mc[o].icon, mc[o].x + 4, mc[o].y + 1, 16, 16);
			}
			FontOut (fdc, mc[o].x + 25, mc[o].y + (long) floor(mc[o].h / 4) - 2, mc[o].text, FONT_MSSANS);
			// TextOutEx (fdc, mc[o].x + 25, mc[o].y + floor(mc[o].h / 4) - 2, mc[o].text);
		}
		*/
	}
}

void MegaPopup::DrawHilite (long o)																	// private
{
	//
	// No CS needed, because of: PRIVATE FUNCTION
	//
	if (!(mc[o].flags & MPM_HILITEBOX))
	{
		// Normal Hilite
		FillRect2 (fdc, mc[o].x, mc[o].y, mc[o].w, mc[o].h, sc.BBLUE);
	}
	else
	{
		// Hilite Box
		FillRect2 (fdc, mc[o].x + 23, mc[o].y, mc[o].w, mc[o].h, sc.BBLUE);
		DrawFrame3D (fdc, mc[o].x + 1, mc[o].y - 1, 21, TS+1);
	}
}

void MegaPopup::DrawScrollArrows ()																	// private
{
	//
	// No CS needed, because of: PRIVATE FUNCTION
	//
	// Draw Scroll Arrows if necessary
	if (fsb)
	{
		long arrowcolor;
		FillRect2 (fdc, me.w - 34, -1, 16, 14, (HBRUSH) COLOR_WINDOW);
		FillRect2 (fdc, me.w - 34, me.h - 20, 16, 14, (HBRUSH) COLOR_WINDOW);
		if (!scrolling)
		{
			arrowcolor = RGB(0, 0, 255);
			DrawFrame3D (fdc, me.w - 34, -1, 16, 14);
			DrawFrame3D (fdc, me.w - 34, me.h - 20, 16, 14);	
		}
		else
		{
			arrowcolor = RGB(255, 0, 0);
			DrawInvFrame3D (fdc, me.w - 34, -1, 16, 14, true);
			DrawInvFrame3D (fdc, me.w - 34, me.h - 20, 16, 14, true);
		}
		DrawFontChar (fdc, me.w - 33, -5, 53, SFONT_WEBDINGS, 20, arrowcolor);
		DrawFontChar (fdc, me.w - 33, me.h - 23, 54, SFONT_WEBDINGS, 20, arrowcolor);
	}
}

void MegaPopup::RefreshAllItems ()																	// private
{
	//
	// No CS needed, because of: PRIVATE FUNCTION
	//
	FillRect2 (fdc, 0, 0, me.w, me.h, (HBRUSH) COLOR_WINDOW);
	for (long i = 1; i <= mc.Ubound(); i++)
	{
		RedrawItem(i);
	}
	DrawScrollArrows();
	fwnd_mem.Refresh();
}


//////////////////////
// Window Process	//
//////////////////////
LRESULT CALLBACK MegaPopup::Callback_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)	// protected
{
	MegaPopup* mpm = (MegaPopup*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if (mpm != 0)
	{
		return mpm->WndProc (hwnd, msg, wparam, lparam);
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

LRESULT MegaPopup::WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)							// public
{
	//
	// No CS: RISK OF DEADLOCK within a window process
	//
	if (!lock)
	{
		lock = true;
		if (msg == WM_KILLFOCUS)
		{
			// Focused Window Is Parent Window? (Determine by Class!)
			if (GetWindowClass((HWND) wparam) == WNDCLASS)
			{
				JumpPrevious ((HWND) wparam);
			}
			else
			{
				KeyDown (27);		// Simulate <ESC>
			}
		}
		else if (msg == WM_KEYDOWN)
		{
			if (wparam == 13) { lastmb = VK_LBUTTON; };
			KeyDown ((long) wparam);
		}
		else if (msg == WM_LBUTTONUP)
		{
			MouseUp (VK_LBUTTON, LOWORD(lparam), HIWORD(lparam));
		}
		else if (msg == WM_RBUTTONUP)
		{
			MouseUp (VK_RBUTTON, LOWORD(lparam), HIWORD(lparam));
		}
		else if (msg == WM_MOUSEMOVE)
		{
			MouseMove ((long) wparam, LOWORD(lparam), HIWORD(lparam));
		}
		lock = false;
	}
	
	if (msg == WM_PAINT) { fwnd_mem.Refresh(); return false; };
	return DefWindowProc (hwnd, msg, wparam, lparam);
}

void MegaPopup::KeyDown (long key)																	// private
{
	//
	// No CS needed, because of: PRIVATE FUNCTION
	//
	if (key == 37)			// <Left>
	{
		JumpPrevious (parent);
	}
	else if (key == 39)		// <Right>
	{
		KeyDown (13);					// Simulate <RETURN>
	}
	else if (key == 38)		// <Up>
	{
		ChangeSelection (SearchBackward(sel));
		if (fsb) { KeyboardScroll(); };
	}
	else if (key == 40)		// <Down>
	{
		ChangeSelection (SearchForward(sel));
		if (fsb) { KeyboardScroll(); };
	}
	else if (key == 13)		// <RETURN>
	{
		if (sel != 0)
		{
			if (!(mc[sel].flags & MPM_DISABLED))
			{
				// (!) Item IS Activated
				if (!(mc[sel].flags & MPM_FADED))
				{
					// (!) Item is NOT Faded
					threaddata = mc[sel].idx;
					threadres = MPM_SELECT;
					RfMsgQueue (fwnd);
				}
				else
				{
					// (!) Item IS Faded
					if (fx.callback != 0)
					{
						fx.x = me.x + mc[sel].x + mc[sel].w - 3;
						fx.y = me.y + mc[sel].y;
						
						// Create child popup-menu and [initialize/start] it by callback
						MegaPopup* mpmchild = new MegaPopup (TS, MPM_FONTSIZE);
						mpmchild->parent = fwnd;
						long res = fx.callback (this, mpmchild, mc[sel].idx, fx.x, fx.y, 
										fx.data);		// Translate & Call
						
						// Restore Focus and Act
						SetActiveWindow (fwnd);
						if (res == MPM_PREV)
						{
							if (mpmchild->jumpto == fwnd)
							{
								// This menu is the next active one
								quoteMouse = true;
							}
							else
							{
								// We need to go back a little bit more
								JumpPrevious (mpmchild->jumpto);
							}
						}
						else if (res == MPM_NOTHING) {}
						else if (res == MPM_ESC)
						{
							threadres = MPM_ESC;
							RfMsgQueue (fwnd);
						}
						else
						{
							threaddata = mc[sel].idx;
							threadres = MPM_SELECT;
							RfMsgQueue (fwnd);
						}
						delete mpmchild;
					}
					else
					{
						FatalAppExit (0, "MegaPopup::KeyDown->No Callback Function");
					}
				}
			}
			else
			{
				// (!) Item IS DeActivated
				beep();
			}
		}
	}
	else if (key == 27)		// <ESC>
	{
		threadres = MPM_ESC;
		RfMsgQueue (fwnd);
	}
}

void MegaPopup::JumpPrevious (HWND hwnd)															// private
{
	//
	// No CS needed, because of: PRIVATE FUNCTION
	//
	jumpto = hwnd;
	threadres = MPM_PREV;
	RfMsgQueue (fwnd);
}

void MegaPopup::KeyboardScroll ()																	// private
{
	//
	// No CS needed, because of: PRIVATE FUNCTION
	//
	if ((sel >= 1) && (sel <= mc.Ubound()))				// Index Valid?
	{
		if (mc[sel].y <= (2 * TS))						// Selection at WindowEdge?
		{
			Scroll (-mc[sel].y + (2 * TS));
		}
		else if (mc[sel].y >= me.h - (2 * TS))
		{
			Scroll (-mc[sel].y + me.h - (2 * TS));
		}
	}
}

void MegaPopup::MouseMove (long shift, long x, long y)												// private
{
	//
	// No CS needed, because of: PRIVATE FUNCTION
	//
	long u = mc.Ubound();
	long newsel = 0;
	
	// Mouse at WindowEdge?
	if (fsb)
	{
		POINT pt; DWORD t;
		if (y <= 10)
		{
			scrolling = true;
			while (mc[1].y < 0)
			{
				GetCursorPos (&pt);
				if (ScreenToClient (fwnd, &pt) == 0)		// Cursor left the window
				{
					break;
				}
				else if (pt.y > 10)
				{
					break;
				}
				t = GetTickCount() + scrollspeed;
				Scroll (+scrollrate);
				while (GetTickCount() < t);
			}
			scrolling = false;
		}
		else if (y >= me.h - 15)
		{
			scrolling = true;
			while (mc[u].y + mc[u].h > me.h - 5)
			{
				GetCursorPos (&pt);
				if (ScreenToClient (fwnd, &pt) == 0)		// Cursor left the window
				{
					break;
				}
				else if (pt.y < me.h - 15)
				{
					break;
				}
				t = GetTickCount() + scrollspeed;
				Scroll (-scrollrate);
				while (GetTickCount() < t);
			}
			scrolling = false;
		}
	}
	
	// Determine Selection
	for (long i = 1; i <= u; i++)
	{
		if (PointInRect(x, y, mc[i].x, mc[i].y, mc[i].x + mc[i].w, mc[i].y + mc[i].h))
		{
			newsel = i;
			break;			// <----  Remove for Item Rectangle debug!
		}
	}
	
	// Apply if necessary
	if (newsel != 0)
	{
		if (xlen(mc[newsel].text) == 0) { newsel = 0; };			// Separator ^= 0 ("")
	}
	if (sel != newsel) { ChangeSelection(newsel); };
}

void MegaPopup::MouseUp (long button, long x, long y)											// private
{
	//
	// No CS needed, because of: PRIVATE FUNCTION
	//
	if (!quoteMouse)
	{
		lastmb = button;
		KeyDown (13);		// Simulate <RETURN>
	}
	else
	{
		quoteMouse = false;
	}
}


//////////////////////////
// Item Selection		//
//////////////////////////
long MegaPopup::SearchForward (long o)															// private
{
	//
	// No CS needed, because of: PRIVATE FUNCTION
	//
	long u = mc.Ubound();
	long res = 1;
	for (long i = o + 1; i <= u; i++)
	{
		if (xlen(mc[i].text) != 0)			// It is not a Separator
		{
			res = i; break;
		}
	}
	return res;
}

long MegaPopup::SearchBackward (long o)															// private
{
	//
	// No CS needed, because of: PRIVATE FUNCTION
	//
	long u = mc.Ubound();
	long res = u;
	for (long i = o - 1; i >= 1; i--)
	{
		if (xlen(mc[i].text) != 0)			// It is not a Separator
		{
			res = i; break;
		}
	}
	return res;
}

void MegaPopup::ChangeSelection (long newsel)													// private
{
	//
	// No CS needed, because of: PRIVATE FUNCTION
	//
	long oldsel = sel;
	sel = newsel;
	RedrawItem (oldsel); RedrawItem (sel);
	DrawScrollArrows();
	fwnd_mem.Refresh();
}


//////////////////////////
// Window Subsystem		//
//////////////////////////
string MegaPopup::GetWindowClass (HWND hwnd)													// private
{
	//
	// No CS needed, because of: PRIVATE FUNCTION
	//
	string buf; buf.resize (128);
	long res = GetClassName (hwnd, &buf[0], xlen(buf));
	buf.resize (res);
	return buf;
}

HWND MegaPopup::PerformEffect (int seffect, int align)											// private
{
	//
	// No CS needed, because of: PRIVATE FUNCTION
	//
	HWND oldactwnd = 0;
	long ty, nx, ny;
	long MPM_FX_START = 20;
	
	// Start the Scrolling Effect
	for (long tx = MPM_FX_START; tx <= me.w; tx += 16)
	{
		// Determine new size & origin
		ty = (long) (((double) tx / me.w) * me.h);
		if (seffect & MPM_HSCROLL) { nx = tx; } else { nx = me.w; };
		if (seffect & MPM_VSCROLL) { ny = ty; } else { ny = me.h; };
		if (align == MPM_RIGHTBOTTOM)
		{
			SetWindowPos (fwnd, 0, me.x + me.w - nx, me.y + me.h - ny, nx, ny, 0);
		}
		else
		{
			SetWindowOrgEx (fdc, nx - me.w, ny - me.h, 0);
			SetWindowPos (fwnd, 0, 0, 0, nx, ny, SWP_NOMOVE);
		}
		
		// Begin to Visualize after the first effect has been drawn
		if (tx == MPM_FX_START)
		{
			oldactwnd = ActivateWindow (fwnd);
			ShowWindow (fwnd, SW_SHOW);
		}
		
		// Refresh & Wait
		fwnd_mem.Refresh();
		Sleep (10);					// <-> while (GetTickCount() < ticks + 10);
	}
	
	// End the Scrolling Effect
	SetWindowOrgEx (fdc, 0, 0, 0);
	SetWindowPos (fwnd, 0, me.x, me.y, me.w, me.h, 0);
	fwnd_mem.Refresh();
	return oldactwnd;
}


