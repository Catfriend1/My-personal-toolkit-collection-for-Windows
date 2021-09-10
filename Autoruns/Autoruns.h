// Autoruns.h: Declares everything needed to use the Autoruns.cpp module
// 
// Thread-Safe: NO
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __AUTORUNS_H
#define __AUTORUNS_H


	//////////////////////////
	// Class: Autoruns		//
	//////////////////////////
	class Autoruns
	{
	private:
		// Consts
		static const char CAPTION[], TRAY_CAPTION[], WNDCLASS_EDITOR[];
		static const long GUARD_TIMER_INTERVAL;
		static const char REG_WINDOW_X[], REG_WINDOW_Y[];
		static const long EDITOR_DEF_WND_X, EDITOR_DEF_WND_Y, EDITOR_WND_WIDTH, EDITOR_WND_HEIGHT;
		static const long MYTV_INVALID, MYTV_GLOBAL, MYTV_LOCAL;
		static const char REG_AUTORUN_BASE[], REG_WINLOGON[];

		// Consts: Guard
		static const char GUARD_CAPTION[];
		static const long ATTRIBUTE_INI_LOCKED, ATTRIBUTE_INI_UNLOCKED;
		static const long DIA_DEFWIDTH, DIA_DEFHEIGHT, DIA_DETAILMODE;
		static const long DIA_ACCEPT, DIA_RESTORE;

		// Consts: Refind
		static const int RF_OK, RF_ADDED, RF_CHANGED, RF_DELETED;

		// Types
		struct GuardWndData
		{
			int action;
			HKEY mainkey;
			string section;
			string name;
			string value;
			string prev;
			Autoruns* class_ptr;
		};

		// Critical Section Object
		CriticalSection cs;

		// Cache: Local Classes
		SystrayIcon sysicon;
		RegObj reg;
		StdFonts sfnt;
	
		// Cache: Editor
		HWND ewnd;
		HWND ar_tvc, lvname, lvstr, cmdadd, cmddel, chkassis;
		HWND txtshell, txtwinload, txtwinrun, cmdapply, cmdupdate;
		HDC edc;
		MemoryDC ewnd_mem;
		string readable_curuser;
		bool ctrlinited;

		// Cache: Guard
		HANDLE arfile;
		string ini_file;
		Inifile ini;
		bool guard_inited;

		// Functions: Window Visibility
		void Window_Show();
		void Window_Hide();

		// Functions: Editor Window Process
		LRESULT CALLBACK Editor_WndProc (HWND, UINT, WPARAM, LPARAM);
		void Editor_EntrySelected (long);

		// Functions:  Editor
		void Editor_FillTreeView ();
		string Editor_DetCurrentUser ();

		void Editor_RefreshEntries ();
		HKEY Editor_DecideHKey (LRESULT);

		// Functions: Editor User Interface
		void Editor_Addreg ();
		void Editor_Delreg ();
		void Editor_ChkAssist (long);

		void Editor_Refresh ();
		void Editor_Apply ();

		// Functions: Guard Init/Terminate
		void Guard_Start ();
		void Guard_Stop ();

		// Functions:  Guard Timer
		void CheckRegSection (HKEY, string);

		// Functions: INI's File Operations
		bool Guard_LockINI ();
		void Guard_UnlockINI ();

		void Guard_InitINI ();
		void Guard_FlushINI ();

		string Reg2Cache (HKEY, string);

		// Functions: Autostart Warning Dialog
		INT_PTR CALLBACK Guard_WndProc (HWND, UINT, WPARAM, LPARAM);;

		// Functions: External Access to the Cache
		void Guard_Direct_Write (HKEY, string, string, string);
		void Guard_Direct_Delete (HKEY , string, string);

	public:
		// Functions: Constructor, Destructor
		Autoruns();
		~Autoruns();

	protected:
		// Functions: Callback
		static LRESULT CALLBACK _ext_Autoruns_Editor_WndProc (HWND, UINT, WPARAM, LPARAM);
		static INT_PTR CALLBACK _ext_Autoruns_Guard_WndProc (HWND, UINT, WPARAM, LPARAM);
	};







	//////////////////////////////////////////////
	// Class: Autoruns Launcher Component		//
	//////////////////////////////////////////////
	class ARLauncher
	{
	private:
		// Consts
		static const char LAUNCHER_CAPTION[], WNDCLASS_LAUNCHER[];
		static const long LAUNCHER_WND_X, LAUNCHER_WND_Y, LAUNCHER_WND_WIDTH, LAUNCHER_WND_HEIGHT;
		static const char REG_AUTORUN_BASE[];

		// Critical Section Object
		CriticalSection cs;

		// Cache
		ThreadManager LAUNCHER_VM;
		StdFonts sfnt;
		IconBox* iconbox;
		HWND awnd;
		HDC adc;
		MemoryDC awnd_mem;
		bool ctrlinited, termsignal;

		// Functions: Launcher
		long Launcher_WorkerThread ();
		long Launcher_ReturnCountOfAssistedARS (HKEY, string);
		void Launcher_LaunchPrograms (HKEY, string);

		LRESULT CALLBACK Launcher_WndProc (HWND, UINT, WPARAM, LPARAM);
		void Launcher_ShowStatus (string, string);

	public:
		// Functions: Constructor, Destructor
		ARLauncher();
		~ARLauncher();

		// Functions: Launcher Interface
		bool Launcher_ExternExecute ();

		// Functions: Callback

	protected:
		static long _ext_ARLauncher_WorkerThread (ARLauncher*);
		static LRESULT CALLBACK _ext_ARLauncher_WndProc (HWND, UINT, WPARAM, LPARAM);
	};



#endif
