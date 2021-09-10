// Comctls.cpp: Additional Common Controls
// Version: 2010-02-20
//
// Thread-Safe: YES
//


//////////////////
// Includes		//
//////////////////
#include "stdafx.h"

#include "_modules.h"


/////////////////////////////////////////////////////////////////////////////////////
// Class: BitmapButton		//				-START-
/////////////////////////////////////////////////////////////////////////////////////
const char BitmapButton::WNDCLASS[]					=		"C32BitmapButton";

BitmapButton::BitmapButton (HWND xparent, long x, long y, long w, long h, 
							HICON xicon, HFONT xfont, string caption, bool xaltselmode)
{
	// General Init
	focused = false;
	selected = false;
	active = false;
	parent = xparent; me.x = x; me.y = y; me.w = w; me.h = h; icon = xicon;
	altselmode = xaltselmode;
	
	// Register Window Class if necessary
	NewWindowClass (WNDCLASS, &Callback_WndProc, 0, CS_OWNDC, 0, 0, NULL, true);
	
	// Create Child Window
	bwnd = CreateWindowEx (0, 
				WNDCLASS, "(___) BitmapButton", 
				WS_CHILD | WS_VISIBLE | WS_TABSTOP, 
				me.x, me.y, me.w, me.h, parent, 0, 0, 0);
	SetWindowLongPtr (bwnd, GWLP_USERDATA, (LONG_PTR) this);
	
	// Create Blitting Surface
	bdc = bwnd_mem.Create (bwnd, me.w, me.h);
	SetBkMode (bdc, TRANSPARENT);
	
	// First Draw
	SetFont (xfont);
	SetCaption (caption);
}

BitmapButton::~BitmapButton ()
{
	bwnd_mem.Free();
	DestroyWindow (bwnd);
	UnregisterClass (WNDCLASS, 0);
}

void BitmapButton::DrawMe (bool newsel)																			// public
{
	// Enter Critical Section
	cs.Enter();
	
	// Draw the Button in the requested state
	selected = newsel;
	if (!selected)
	{
		FillRect2 (bdc, 0, 0, me.w, me.h, (HBRUSH) COLOR_WINDOW);
		DrawFrame3D (bdc, 0, 0, me.w, me.h);
		if (IsWindowEnabled(bwnd))
		{
			TransIcon (bdc, icon, 5, icony + 3, 16, 16);
			SetTextColor (bdc, RGB(0, 0, 0));
			FontOut (bdc, 24, fonty + 3, text, font, true);
		}
		else
		{
			InvIcon (bdc, icon, 5, icony + 3, 16, 16);
			GreyFontOut (bdc, 24, fonty + 3, text, font, true);
		}
	}
	else
	{
		FillRect2 (bdc, 0, 0, me.w, me.h, (HBRUSH) COLOR_WINDOW);
		TransIcon (bdc, icon, 5 + 1, icony + 3 + 1, 16, 16);
		SetTextColor (bdc, RGB(0, 0, 0));
		FontOut (bdc, 24 + 1, fonty + 3 + 1, text, font, true);
		if (!altselmode)
		{
			DrawInvFrame3D (bdc, 0, 0, me.w, me.h, false);
		}
		else
		{
			DrawBoldFrame3D (bdc, 0, 0, me.w, me.h);
		}
	}
	if (((focused && !altselmode) || (altselmode && focused && !selected)) && 
		(IsWindowEnabled(bwnd)))
	{
		DrawSurround (bdc, 4, 4, me.w - 8, me.h - 8, RGB(50, 50, 50));
	}
	bwnd_mem.Refresh();

	// Leave Critical Section
	cs.Leave();
}

// BitmapButton: Window Process
LRESULT CALLBACK BitmapButton::Callback_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)				// protected
{
	BitmapButton* bb = (BitmapButton*) GetWindowLongPtr (hwnd, GWLP_USERDATA);
	if (bb != 0)
	{
		return bb->WndProc (hwnd, msg, wparam, lparam);
	}
	return DefWindowProc (hwnd, msg, wparam, lparam);
}

LRESULT BitmapButton::WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)									// public
{
	// No CS for the Window Process
	// Risk of Deadlocks here...!

	if (msg == WM_PAINT)
	{
		bwnd_mem.Refresh();
		return false;
	}
	else if (msg == WM_ENABLE)
	{
		if (wparam == 0)
		{
			DrawMe (false);
		}
		else
		{
			DrawMe (false);
		}
	}
	else if (msg == WM_CAPTURECHANGED)
	{
		DrawMe (false);
	}
	
	// Button Routine
 	if (IsWindowEnabled(bwnd))
	{
		if (msg == WM_SETFOCUS)
		{
			focused = true;
			DrawMe (false);
		}
		else if (msg == WM_KILLFOCUS)
		{
			active = false;
			if (GetCapture() == bwnd)
			{
				ReleaseCapture();
			}

			focused = false;
			DrawMe (false);
		}
		else if ((msg == WM_MOUSEMOVE) && active)
		{
			if (wparam == MK_LBUTTON)
			{
				// Type BOOL - if inbound, DrawMe (true);
				DrawMe (PointInRect(LOWORD(lparam), HIWORD(lparam), 0, 0, me.w, me.h));
			}
		}
		else if (msg == WM_KEYDOWN)
		{
			// (!) Return does not cause WM_KEYDOWN
 			if (wparam == VK_SPACE)
			{
				active = true;
				DrawMe (true);
			}
		}
		else if (msg == WM_LBUTTONDOWN)
		{
			md.x = LOWORD(lparam); md.y = HIWORD(lparam);
			active = true;
			
			SetFocus (bwnd);
			DrawMe (true);
			SetCapture (bwnd);
		}
		else if (msg == WM_KEYUP)
		{
			if (((wparam == VK_SPACE) && active) ||
				(wparam == VK_RETURN))
			{
				active = false;
				ButtonClick();
			}
		}
		else if ((msg == WM_LBUTTONUP) && active)
		{
			active = false;
			ReleaseCapture();
			if (PointInRect(LOWORD(lparam), HIWORD(lparam), 0, 0, me.w, me.h))
			{
				ButtonClick();
			}
			else
			{
				DrawMe (false);
			}
		}
		else if ((msg == WM_RBUTTONDOWN) || (msg == WM_RBUTTONUP))						// Notify Parent Window
		{
			SendMessage (parent, WM_COMMAND, msg, (LPARAM) bwnd);
		}
	}

	return DefWindowProc (hwnd, msg, wparam, lparam);
}

// BitmapButton: Events
void BitmapButton::ButtonClick ()														// public
{
	// Enter Critical Section
	cs.Enter();

	// Simulate Click Event
	if (IsWindowEnabled(bwnd))
	{
		SendMessage (parent, WM_COMMAND, WM_LBUTTONUP, (LPARAM) bwnd);
	}

	// Leave Critical Section
	cs.Leave();
}

bool BitmapButton::clicked (WPARAM wparam, LPARAM lparam)								// public
{
	// No CS
	return ((wparam == WM_LBUTTONUP) && ((HWND) lparam == bwnd));
}

// BitmapButton: Properties
void BitmapButton::SetCaption (string caption)											// public
{
	// Enter Critical Section
	cs.Enter();

	// Change Caption Text and Redraw the Button
	text = caption;
	DrawMe (selected);

	// Leave Critical Section
	cs.Leave();
}

void BitmapButton::SetFont (HFONT newfont)												// public
{
	string teststr = " ";
	SIZE sz;
	
	// Enter Critical Section
	cs.Enter();

	// Calculate new font and icon position
	font = newfont;
	SelectObject (bdc, font);
	GetTextExtentPoint32 (bdc, &teststr[0], xlen(teststr), &sz);
	icony = ((me.h - 6) / 2) - 8;
	fonty = (me.h - 6 - sz.cy) / 2;
	
	// Redraw the Button
	DrawMe (selected);

	// Leave Critical Section
	cs.Leave();
}

void BitmapButton::Enable (bool state)													// public
{
	// No CS
	EnableWindow (bwnd, state);
}

HWND BitmapButton::hwnd ()																// public
{
	// No CS
	return bwnd;
}

/////////////////////////////////////////////////////////////////////////////////////
// Class: BitmapButton		//				-END-
/////////////////////////////////////////////////////////////////////////////////////








/////////////////////////////////////////////////////////////////////////////////////
// Class: IconBox		//					-START-
/////////////////////////////////////////////////////////////////////////////////////
const char IconBox::WNDCLASS[]					=	"C32IconBox";

IconBox::IconBox (HWND xparent, long x, long y, long w, long h, HICON icon)
{
	// General Init
	lock = false;
	parent = xparent; me.x = 0; me.y = 0; me.w = w; me.h = h;
	
	// Register Window Class if necessary
	NewWindowClass (WNDCLASS, &Callback_WndProc, 0, CS_OWNDC, 0, 0, NULL, true);
	
	// Create Child Window
	iwnd = CreateWindowEx (0, 
				WNDCLASS, "(___) IconBox", 
				WS_CHILD | WS_VISIBLE, 
				x, y, me.w, me.h, parent, 0, 0, 0);
	SetWindowLongPtr (iwnd, GWLP_USERDATA, (LONG_PTR) this);
	
	// Create Blitting Surface
	idc = iwnd_mem.Create (iwnd, me.w, me.h);
	SetBkMode (idc, TRANSPARENT);
	
	// First Draw
	if (icon != 0) { DrawIcon (icon); };
}

IconBox::~IconBox ()
{
	iwnd_mem.Free();
	DestroyWindow (iwnd);
	UnregisterClass (WNDCLASS, 0);
}

void IconBox::DrawIcon (HICON icon)
{
	// Enter Critical Section
	cs.Enter();

	// Redraw the Icon
	FillRect2 (idc, 0, 0, me.w, me.h, (HBRUSH) COLOR_WINDOW);
	TransIcon (idc, icon, 0, 0, me.w, me.h);
	iwnd_mem.Refresh();

	// Leave Critical Section
	cs.Leave();
}

LRESULT CALLBACK IconBox::Callback_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)			// protected
{
	// No CS
	IconBox* ib = (IconBox*) GetWindowLongPtr (hwnd, GWLP_USERDATA);
	if (ib != 0)
	{
		return ib->WndProc (hwnd, msg, wparam, lparam);
	}
	return DefWindowProc (hwnd, msg, wparam, lparam);
}

LRESULT IconBox::WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)								// public
{
	// No CS for Window Process, Risk of Deadlock!
	if (msg == WM_PAINT)
	{
		iwnd_mem.Refresh();
		return false;
	}
	return DefWindowProc (hwnd, msg, wparam, lparam);
}

HWND IconBox::hwnd ()																					// public
{
	// No CS
	return iwnd;
}

/////////////////////////////////////////////////////////////////////////////////////
// Class: IconBox		//					-END-
/////////////////////////////////////////////////////////////////////////////////////








/////////////////////////////////////////////////////////////////////////////////////
// Class: 3D Line Window	//				-START-
/////////////////////////////////////////////////////////////////////////////////////
const char LineWindow::WNDCLASS[] = "C32LineWindow3D";

LineWindow::LineWindow (HWND xparent, long x, long y, long len, int xmode)
{
	// General Init
	lock = false;
	parent = xparent; mode = xmode; me.x = 0; me.y = 0;
	if (mode == 1)			// Horizontal
	{
		me.w = len;
		me.h = 2;
	}
	else if (mode == 2)		// Vertical
	{
		me.w = 2;
		me.h = len;
	}

	// Register Window Class if necessary
	NewWindowClass (WNDCLASS, &Callback_WndProc, 0, CS_OWNDC, 0, 0, NULL, true);
	
	// Create Child Window
	lwnd = CreateWindowEx (0, 
				WNDCLASS, "(___) LineWindow3D", 
				WS_CHILD | WS_VISIBLE, 
				x, y, me.w, me.h, parent, 0, 0, 0);
	SetWindowLongPtr (lwnd, GWLP_USERDATA, (LONG_PTR) this);
	
	// Create Blitting Surface
	ldc = lwnd_mem.Create (lwnd, me.w, me.h);
	
	// First Draw
	DrawMe();
}

LineWindow::~LineWindow ()
{
	lwnd_mem.Free();
	DestroyWindow (lwnd);
	UnregisterClass (WNDCLASS, 0);
}

void LineWindow::DrawMe ()																				// public
{
	// Enter Critical Section
	cs.Enter();

	// Redraw the Line
	if (mode == 1)
	{
		// Horizontal Align
		Line2 (ldc, 0, 0, me.w, 0, sc.PGREY);
		Line2 (ldc, 0, 1, me.w, 1, sc.PWHITE);
	}
	else if (mode == 2)
	{
		// Vertical Align
		DownLine (ldc, 0, 0, me.h);
	}
	lwnd_mem.Refresh();

	// Leave Critical Section
	cs.Leave();
}

LRESULT CALLBACK LineWindow::Callback_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)		// protected
{
	// No CS
	LineWindow* lin = (LineWindow*) GetWindowLongPtr (hwnd, GWLP_USERDATA);
	if (lin != 0)
	{
		return lin->WndProc (hwnd, msg, wparam, lparam);
	}
	return DefWindowProc (hwnd, msg, wparam, lparam);
}

LRESULT LineWindow::WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)							// public
{
	// No CS for Window Process, Risk of Deadlock!
	if (msg == WM_PAINT)
	{
		lwnd_mem.Refresh();
		return false;
	}
	return DefWindowProc (hwnd, msg, wparam, lparam);
}

HWND LineWindow::hwnd ()																				// public
{
	// No CS
	return lwnd;
}

/////////////////////////////////////////////////////////////////////////////////////
// Class: 3D Line Window	//				-END-
/////////////////////////////////////////////////////////////////////////////////////


