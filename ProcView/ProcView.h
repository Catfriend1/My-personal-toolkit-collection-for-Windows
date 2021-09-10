// ProcView.h: Declares everything needed to use the ProcView.cpp module
//
// Prerequisites: "WindowPoser/WindowPoser.h"
//
// Thread-Safe: NO
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __PROCVIEW_H
#define __PROCVIEW_H


	//////////////////////////
	// Class: ProcView		//
	//////////////////////////
	class ProcView
	{
	private:
		// Imports:
		typedef BOOL (WINAPI*DYN_WINAPI_WOW64DISABLEFSREDIRECTION)	(PVOID*	OldValue);
		typedef BOOL (WINAPI*DYN_WINAPI_WOW64REVERTFSREDIRECTION)	(PVOID	OldValue);

		// Type: Cachetype for the Systray Menu
		class PIDCACHE
		{
			public:
				string exename;
				DWORD procid;
				HICON hicon;
		};

		// Type: Cachetype for Process Viewer's Overview Window
		class pvTreeCache
		{
			public:
				HWND	hwnd;
				DWORD	procid;
		};

		// Consts
		static const char CAPTION[];
		static const char WNDCLASS[];

		static const char REG_WINDOW_X[], REG_WINDOW_Y[], REG_WINDOWPOSER[], REG_DATEPRINT[], REG_HOTKEY_TIP[];
		static const long DEF_WINDOW_X, DEF_WINDOW_Y, DEF_WINDOW_WIDTH, DEF_WINDOW_HEIGHT, DEF_HOTKEY_TIP;
		static const long WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT;

		static const int  PROCWND_TIMER_ID, SYSICON_TIMER_ID;
		static const long PROCWND_TIMER_INTERVAL, SYSICON_TIMER_INTERVAL;

		static const long StdMenuItems, StartKernel, StartSystem, StartOther;
		static const char plist_kernel_w10[], plist_system_w10[];

		// Consts: Third-party app info.
		//		1) Sophos AntiVirus
		static const char REG_SOPHOS_SAVSERVICE_APPLICATION[];
		static const char REGN_SOPHOS_SAVSERVICE_PATH[];
		
		static const char REG_SOPHOS_WEBINTELLIGENCE_APPLICATION[];
		static const char REGN_SOPHOS_WEBINTELLIGENCE_LOCATION[];

		static const char REG_SOPHOS_MCS[];
		static const char REGN_SOPHOS_MCS_ADAPTER[];

		static const char REG_SOPHOS_ALC[];
		static const char REGN_SOPHOS_ALC_ADAPTER[];

		//		2) Realtek Audio Control Panel
		static const char REG_REALTEK_AUDIO_GUIINFO[];
		static const char REGN_REALTEK_AUDIO_PATH[];

		// Critical Section Object
		CriticalSection cs;

		// WinAPI Import Cache.
		DYN_WINAPI_WOW64DISABLEFSREDIRECTION	DYN_WINAPI_Wow64DisableFsRedirection;
		DYN_WINAPI_WOW64REVERTFSREDIRECTION		DYN_WINAPI_Wow64RevertFsRedirection;

		// Cache
		SystrayIcon sysicon;
		RegObj reg;
		AppDetect apd;

		HWND mwnd;
		HWND htreeview;
		HIMAGELIST ilist;
		StructArray <pvTreeCache> pvtc;
		string TRAY_CAPTION;
		string os_filter_kernel, os_filter_system;
		string my_exefullfn;
		DWORD my_procid;
		bool ctrlinited;

		// Cache: Third-party installation info
		string REGV_SOPHOS_SAVSERVICE_PATH;
		string REGV_SOPHOS_WEBINTELLIGENCE_PATH;
		string REGV_SOPHOS_WEBCONTROL_PATH;

		string REGV_SOPHOS_MCS_ADAPTER;			// DLL-file
		string SOPHOS_MCS_PATH;					// => Folder.

		string REGV_SOPHOS_ALC_ADAPTER;			// DLL-file
		string SOPHOS_ALC_PATH;					// => Folder.

		string REGV_REALTEK_AUDIO_PATH;

		// Cache: Resource Management
		HICON L_IDI_CLOSE;
		HICON L_IDI_EXECUTE;
		HICON L_IDI_EXPLORER;
		HICON L_IDI_MAX;
		HICON L_IDI_PROCVIEW;
		HICON L_IDI_SOFTWAREBIG;
		HICON L_IDI_UNKNOWN;
		HICON L_IDI_CHECK;
		HICON L_IDI_PAPER;

		// Cache: Registry Settings
		long REGV_WINDOWPOSER;						// Indicates if component is turned on.
		long REGV_DATEPRINT;						// Indicates if component is turned on.

		// Cache: Sub components
		WindowPoser* mod_windowposer;

		// Cache: Path Vars
		string os_system_dir;

		// Functions: Resource Management
		void LoadResources ();
		void FreeResources ();

		// Functions: Helpers
		bool ExistsInList (string, string);
		void KillProcessCheck (DWORD);
		void SetWindowPoserOnOff (bool);

		// Functions: Window Visibility
		void Window_Show ();
		void Window_Hide ();

	public:
		// Functions: Constructor, Destructor
		ProcView();
		~ProcView();

		// Functions: External Class Interface for Hotkey handler in "VK2Wnd_Proc" (Boot.cpp).
		bool IsDatePrintEnabled();

		// Functions: Callback
		LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);

	protected:
		// Functions: Callback
		static LRESULT CALLBACK _ext_ProcView_WndProc (HWND, UINT, WPARAM, LPARAM);

		static long _ext_ProcView_TrayMenuFader (MegaPopup*, MegaPopup*, long, long, long, long*);
	};


#endif
