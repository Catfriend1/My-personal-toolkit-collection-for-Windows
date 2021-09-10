// NetMsg_Client.h: Defines everything needed to use the NetMsg_Client.cpp module.
//
// Thread-Safe: YES
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __NETMSG_CLIENT_H
#define __NETMSG_CLIENT_H



	//////////////////////////////
	// Class: NetMsg_Client		//
	//////////////////////////////
	class NetMsg_Client
	{
	private:
		// Consts
		static const long NETMSG_TCP_PORT;
		static const char NETMSG_CAPTION[], CLS_HOSTWINDOW[];
		static const long MAX_LISTBOX_ITEMS;
		static const long CONNECTION_TIMEOUT;

		static const int  SYSICON_TIMER_ID;
		static const long SYSICON_TIMER_INTERVAL;
		static const int  LISTUPDATE_TIMER_ID;
		static const long LISTUPDATE_TIMER_INTERVAL;

		static const long ENTERPWRES_STAYLOCKED;
		static const long ENTERPWRES_UNLOCK;

		static const long MAINMESSAGE_THREAD_RES;

		// Consts: CDS - Client Data Storage
		static const char SECTION_CLIENTS[];

		// Cache
		CriticalSection cs;
		bool signal_client_shutdown;
		bool signal_cds_update;

		SystrayIcon* sysicon;
		HWND hostwnd;

		HICON L_IDI_NETMSG_NOMSG;
		HICON L_IDI_NETMSG_NEWMSG;
		HICON L_IDI_NETMSG_CONNECTING;
		HICON L_IDI_NETMSG_DISCONNECTED;

		// Cache: Network Management
		ThreadManager CLIENT_VM;
		CommQueue RespQueue;
		csocket client;
		Inifile cds;					// Client Data Store

		// Cache: Main Message Queue Thread Management
		ThreadManager NETMSG_VM;
		DWORD MAINMESSAGE_THREAD_ID;

		// Functions: Resource Management
		void LoadResources ();
		void FreeResources ();

		// Functions: Main Thread
		long MainMessageQueue_Thread();

		// Functions: Password Checking Dialog
		bool EnterPW_IsPWCorrect (string);

		// Functions: Systray Menu Related
		void ShowSystrayMenu ();

		// Functions: Thread Interface
		bool UI_Send_Command (string);

	public:
		NetMsg_Client();
		~NetMsg_Client();

		// Callback
		LRESULT CALLBACK HostWndProc (HWND, UINT, WPARAM, LPARAM);						// Host Window
		INT_PTR CALLBACK EnterPW_WndProc (HWND, UINT, WPARAM, LPARAM);		// Password Checking Dialog
		INT_PTR CALLBACK MainDlg_WndProc (HWND, UINT, WPARAM, LPARAM);						// NetMsg Main Window Dialog

		long Client_Worker();

	protected:
		// Functions: Callback
		static long _ext_NetMsg_MainMessageQueue_Thread(NetMsg_Client*);

		static LRESULT CALLBACK _ext_NetMsg_Client_HostWndProc (HWND, UINT, WPARAM, LPARAM);
		static INT_PTR CALLBACK _ext_NetMsg_Client_Callback_EnterPW_WndProc (HWND, UINT, WPARAM, LPARAM);
		static INT_PTR CALLBACK _ext_NetMsg_Client_MainDlg_WndProc (HWND, UINT, WPARAM, LPARAM);

		static long _ext_NetMsg_Client_Client_Worker (NetMsg_Client*);
	};


#endif
