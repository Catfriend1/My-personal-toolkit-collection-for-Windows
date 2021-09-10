// Syslog_Server.h: Defines everything needed to use the Syslog_Server.cpp module.
//
// Thread-Safe:		NO
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __SYSLOG_SERVER_H
#define __SYSLOG_SERVER_H


	//////////////////////////////
	// Class: Syslog_Server		//
	//////////////////////////////
	class Syslog_Server
	{
	private:
		// Consts
		static const bool DEBUG_MODE;
		static const long SYSLOG_TCP_PORT;
		static const char SYSLOG_CAPTION[];

		static const long SLINI_ATTRIB_LOCK, SLINI_ATTRIB_UNLOCK;
		static const long TIMER_DELAY;
		static const char MSG_WINSTART[], MSG_WINSHUT[];
		static const char LOG_PROC_WHITELIST_BY_PROCNAME_ONLY[];

		// Types
		struct NetClient;

		// Cache: Process Filter and Whitelist
		string LOG_PROC_WHITELIST_WIN10;

		// Cache
		Inifile log;
		EnumProcs* ep1;
		EnumProcs* ep2;

		bool log_general;
		bool log_capchanges;

		string lastsess_date, lastsess_time;

		HANDLE slfile;
		string slini;

		// Cache: Server
		csocket srv;
		ThreadManager SERVER_VM;
		StructArray <NetClient> netclients;
		CriticalSection cs;
		bool signal_server_shutdown;

		// Cache: One-time-logged security warnings
		bool once_net_auth_failed;
		bool once_invalid_packet_received;

		// Functions
		void Timer_Event ();
		bool SnapShot_ProcessFiltered(string);

		// Log File Operations
		bool LockINI ();
		void UnlockINI ();

		// (!) Thread Interface (CS protected)
		void WriteLog (string, bool, bool = false);

		void Switch_LogGeneral (bool);

	public:
		Syslog_Server();
		~Syslog_Server();

		// Functions
		bool Init(long);
		bool Main();

		// Callback
		long Server_Worker();

	protected:
		static long _ext_Syslog_Server_Server_Worker (Syslog_Server*);
	};


#endif
