// Syslog_Client.h: Defines everything needed to use the Syslog_Client.cpp module.
//
// Thread-Safe: NO
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __SYSLOG_CLIENT_H
#define __SYSLOG_CLIENT_H



	//////////////////////////////
	// Class: Syslog_Client		//
	//////////////////////////////
	class Syslog_Client
	{
	private:
		// Consts
		static const bool DEBUG_MODE;

		static const long SYSLOG_TCP_PORT;
		static const char SYSLOG_CAPTION[], CLS_HOSTWINDOW[];

		static const long TIMER_DELAY;
		static const long MSG_DISPTIME;

		static const long MAINMESSAGE_THREAD_RES;

		// Cache
		CriticalSection cs;
		bool log_general;
		bool log_capchanges;
		bool signal_client_shutdown;

		bool flag_sent_username_once;

		EnumApps* ea1;
		EnumApps* ea2;
		SystrayIcon* sysicon;
		HWND hostwnd;

		HICON L_IDI_BLUEEYE;
		HICON L_IDI_SLSTART;
		HICON L_IDI_SLSTOP;
		HICON L_IDI_CHECK;
		HICON L_IDI_LOGFILE;

		ThreadManager CLIENT_VM;
		csocket client;

		// Cache: Main Message Queue Thread Management
		ThreadManager SYSLOG_VM;
		DWORD MAINMESSAGE_THREAD_ID;

		// Functions: Resource Management
		void LoadResources ();
		void FreeResources ();

		// Functions: Main Thread
		long MainMessageQueue_Thread();
		void Timer_Event ();

		// Systray Menu Related
		void ShowSystrayMenu ();

		// (!) Thread Interface (needs CS protect at some locations)
		bool WriteLog (string);

	public:
		Syslog_Client();
		~Syslog_Client();

		// Callback
		LRESULT CALLBACK HostWndProc (HWND, UINT, WPARAM, LPARAM);
		long Client_Worker();
		long Logtimer();
		long FileView_And_Deletor();

	protected:
		// Functions: Callback
		static long _ext_Syslog_MainMessageQueue_Thread(Syslog_Client*);

		static LRESULT CALLBACK _ext_Syslog_Client_HostWndProc (HWND, UINT, WPARAM, LPARAM);

		static long _ext_Syslog_Client_Client_Worker (Syslog_Client*);
		static long _ext_Syslog_Client_Logtimer (Syslog_Client*);
		static long _ext_Syslog_Client_FileView_And_Deletor (Syslog_Client*);
	};


#endif
