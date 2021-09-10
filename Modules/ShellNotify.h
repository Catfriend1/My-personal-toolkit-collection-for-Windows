// ShellNotify.h: Declares everything needed to use the ShellNotify.cpp module
// 
// Prerequisites: "shellapi.h"
//
// Thread-Safe: YES
//



//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __SHELLNOTIFY_H
#define __SHELLNOTIFY_H


	//////////////////////////
	// Class: SystrayIcon	//
	//////////////////////////
	class SystrayIcon
	{
	private:
		// Consts
		static const char WNDCLASS[];

		// Critical Section Object
		CriticalSection cs;

		// Cache
		NOTIFYICONDATA nid;
		HWND servwnd;
		BOOL icon_visible;

	public:
		// Init / Terminate
		SystrayIcon();
		~SystrayIcon();
	
		// User Interface
		void SetData (HICON = 0, string = "", HWND = 0, long = -1);
	
		BOOL Show ();
		void Modify ();
		void Hide ();
		BOOL Refresh ();
	
		// Special Features
		void ShowDelayed (long = 12);
		void AbortDelayTimer ();
		void ShowBalloon (string, string, long = 12, long = NIIF_NONE);
	
		// Window Process
		LRESULT WndProc (HWND, UINT, WPARAM, LPARAM);

	protected:
		static LRESULT CALLBACK Callback_WndProc (HWND, UINT, WPARAM, LPARAM);
	};


#endif
