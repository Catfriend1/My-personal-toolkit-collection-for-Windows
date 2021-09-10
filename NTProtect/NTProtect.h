// NTProtect.h: Declares everything needed to use the NTProtect.cpp module
//
// Thread-Safe: NO
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __NTPROTECT_H
#define __NTPROTECT_H


	//////////////////////////
	// Class: NTProtect		//
	//////////////////////////
	class NTProtect
	{
	private:
		// Consts
		static const char CAPTION[];
		static const char LOGFILE[];
		static const char WNDCLASS[];

		static const char REG_NTP_PW[];
		static const char REG_SHOW_LOGO[], REG_SHOW_DESKTOP[], REG_WRITE_LOG[], REG_ALARM_SOUND[];

		static const long PWDLGRES_KEEP;
		static const long PWDLGRES_UNLOCK;

		static const long PWCDLGRES_NOACT;
		static const long PWCDLGRES_CHANGED;

		static const long MODE_DESKTOP;
		static const long MODE_LOGO;
		static const long MODE_WRITELOG;

		static const int  LOCKWND_TIMER_ID;
		static const long LOCKWND_TIMER_INTERVAL;

		static const long NTSERVICE_TCP_PORT;
		static const long LOCK_THREAD_RES;

		// Critical Section Object
		CriticalSection cs;

		// Cache
		string intfile_path;

		long ntp_runmode;
		bool inited;

		HDESK oldprocdesk, ntp_desk;

		DWORD ntp_lockthread;
		HANDLE ntp_log;

		string ntp_alarmsound;
		ActiveMovie* ntp_amovie;
		RegObj reg;
		ThreadManager NTP_VM;

		SYSTEMTIME locktime;
		DWORD locktime_ticks;
		HWND lwnd;
		HDC ldc;
		MemoryDC lwnd_mem;
		long sw, sh;

		// Functions: Graphical Design
		void Graph_InitContents();
		void Graph_UpdateContents();

		// Functions: Password Checking
		bool IsPwCorrect (string);

		// Functions: Log
		void Log_Open();
		void Log_Write (string);
		void Log_Close();

		// Functions: Win32 API for Volume Management
		bool OS_Volume_StepUp ();
		bool OS_Volume_StepDown ();
		bool OS_Volume_Mute ();
		long OS_GetVolume ();
		bool OS_SetVolume (long);

	public:
		// Constructor/Destructor
		NTProtect();
		~NTProtect();

		// Functions: Init/Terminate, Interface
		bool Init();
		void Terminate();
		bool IsSystemLocked();
		bool StartLocking();

		// Functions: System Info
		void Show_SystemInfo();

		// Functions: NTService Communication
		bool SendCommandToService(string);

		// Functions: Callback
		long LockerThread ();

		LRESULT CALLBACK Lock_WndProc (HWND, UINT, WPARAM, LPARAM);
		INT_PTR CALLBACK EnterPW_WndProc (HWND, UINT, WPARAM, LPARAM);
		INT_PTR CALLBACK ChangePW_WndProc (HWND, UINT, WPARAM, LPARAM);

	protected:
		// Functions: Callback
		static long _ext_NTProtect_LockerThread(NTProtect*);

		static LRESULT CALLBACK _ext_NTProtect_Callback_Lock_WndProc (HWND, UINT, WPARAM, LPARAM);
		static INT_PTR CALLBACK _ext_NTProtect_Callback_EnterPW_WndProc (HWND, UINT, WPARAM, LPARAM);
		static INT_PTR CALLBACK _ext_NTProtect_Callback_ChangePW_WndProc (HWND, UINT, WPARAM, LPARAM);
	};


#endif
