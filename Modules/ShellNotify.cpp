// ShellNotify.cpp: System Tray Icon Management Module
//
// Thread-Safe: YES
//


//////////////////
// Includes		//
//////////////////
#include "stdafx.h"

#include "_modules.h"


//////////////
// Consts	//
//////////////
const char SystrayIcon::WNDCLASS[]				=	 "*SysIconSrvCL";


//////////////////////////
// Init / Terminate		//
//////////////////////////
SystrayIcon::SystrayIcon ()
{
	// Initialize Local Cache
	servwnd = 0;
	icon_visible = FALSE;

	// Create Server Window
	NewWindowClass (WNDCLASS, &Callback_WndProc, 0);
	servwnd = CreateWindowEx (0, WNDCLASS, "* Icon Callback Server", 0, 0, 0, 0, 0, 0, 0, 0, 0);
	SetWindowLongPtr (servwnd, GWLP_USERDATA, (LONG_PTR) this);
	
	// Initialize NOTIFYICONDATA
	nid.uID = 0;
	nid.hIcon = 0;
	nid.hWnd = 0;
	nid.uCallbackMessage = WM_LBUTTONUP;
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
	nid.dwInfoFlags = 0;
	nid.uVersion = NOTIFYICON_VERSION;
	nid.cbSize = sizeof(nid);
	SetData (0, Chr(0), 0, 0);		// (!) Set start tooltip to nothing
}

SystrayIcon::~SystrayIcon ()
{
	Hide();

	if (servwnd == 0)
	{
		DestroyWindow (servwnd);
		UnregisterClass (WNDCLASS, 0);
		servwnd = 0;
	}
}


//////////////////////
// User Interface	//
//////////////////////
void SystrayIcon::SetData (HICON icon, string tooltip, HWND hwnd, long index)						// public
{
	// Enter Critical Section
	cs.Enter();

	// Check which parameters have been given to this function and adjust the Cache accordingly
	if (index != -1)	{ nid.uID = index; };
	if (icon != 0)		{ nid.hIcon = icon; };
	if (hwnd != 0)		{ nid.hWnd = hwnd; };
	if (xlen(tooltip) != 0)
	{
		String2Buffer (tooltip, &nid.szTip[0], 63);
		nid.szTip[63] = '\0';			// (!) Last byte must be null character
	}

	// Leave Critical Section
	cs.Leave();
}

BOOL SystrayIcon::Show ()																			// public
{
	BOOL res;
	if (!((nid.hWnd != 0) && (nid.hIcon != 0))) { return FALSE; };

	// CS
	cs.Enter();
	icon_visible = TRUE;
	res = Shell_NotifyIcon (NIM_ADD, &nid);
	cs.Leave();

	// Return if the operation succeeded
	return res;
}

void SystrayIcon::Modify ()																			// public
{
	if (!((nid.hWnd != 0) && (nid.hIcon != 0))) { return; };
	if (!icon_visible) { return; };

	// CS
	cs.Enter();
	Shell_NotifyIcon (NIM_MODIFY, &nid);
	cs.Leave();
}

void SystrayIcon::Hide ()																			// public
{
	if (!((nid.hWnd != 0) && (nid.hIcon != 0))) { return; };
	if (!icon_visible) { return; };

	// CS
	cs.Enter();
	icon_visible = FALSE;
	Shell_NotifyIcon (NIM_DELETE, &nid);
	cs.Leave();
}


//////////////////////////
// Special Features		//
//////////////////////////
void SystrayIcon::ShowDelayed (long secs)															// public
{
	if (!((nid.hWnd != 0) && (nid.hIcon != 0))) { return; };
	
	// Enter Critical Section
	cs.Enter();

	// Show the Icon until the Timer calls back to take it down again
	Show();
	SetTimer (servwnd, 0, secs*1000, 0);

	// Leave Critical Section
	cs.Leave();
}

void SystrayIcon::AbortDelayTimer ()																// public
{
	// CS
	cs.Enter();
	KillTimer (servwnd, 0);
	cs.Leave();
}

void SystrayIcon::ShowBalloon (string btitle, string btext, long secs, long icon)					// public
{
	if (!((nid.hWnd != 0) && (nid.hIcon != 0))) { return; };
	
	// Enter Critical Section
	cs.Enter();

	// Show Balloon
	String2Buffer (btitle, &nid.szInfoTitle[0], 63);
	nid.szInfoTitle[63] = '\0';			// (!) Last byte must be null character
	
	String2Buffer (btext, &nid.szInfo[0], 254);
	nid.szInfo[254] = '\0';				// (!) Last byte must be null character
	
	nid.uTimeout = secs*1000;
	nid.dwInfoFlags = icon;
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_INFO;
	if (!icon_visible)
	{
		Show();
	}
	else
	{
		Modify();
	}
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;

	// Leave Critical Section
	cs.Leave();
}


//////////////////////
// Window Process	//
//////////////////////
LRESULT CALLBACK SystrayIcon::Callback_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)	// protected
{
	SystrayIcon* sti = (SystrayIcon*) GetWindowLongPtr (hwnd, GWLP_USERDATA);
	if (sti != 0)
	{
		return sti->WndProc (hwnd, msg, wparam, lparam);
	}
	return DefWindowProc (hwnd, msg, wparam, lparam);
}

LRESULT SystrayIcon::WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)						// public
{
	// No CS in Window Process, Risk of Deadlocks!
	if (msg == WM_TIMER)
	{
		AbortDelayTimer();
		Hide();
	}
	else if (msg == WM_CLOSE)
	{
		// Do not invoke WM_DESTROY for this window.
		return false;
	}
	return DefWindowProc (hwnd, msg, wparam, lparam);
}


