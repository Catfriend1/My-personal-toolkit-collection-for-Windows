// Winsuite_Server.h: Declares everything needed to use the Winsuite_Server.cpp module.
//
// Thread-Safe: YES
//



//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __WINSUITE_SERVER_H
#define __WINSUITE_SERVER_H



	//////////////////
	// Includes		//
	//////////////////



	//////////////////////////////
	// Class: Winsuite_Server	//
	//////////////////////////////
	class Winsuite_Server
	{
	private:
		// Consts
		static const bool DEBUG_MODE;

		static const long TCP_PORT;
		static const char CAPTION[];

		static const char REG_POLICIES[];

		// Types
		struct NetClient;

		// Critical Section Object
		CriticalSection cs;

		// Cache
		string cache_locktime;

		// Cache: VBOXIPC_Server
		bool vboxipc_enabled;
		bool bIndicateFastTimer;										// If set to TRUE, timer will go faster.
	
		// Cache: Network Server
		csocket srv;
		ThreadManager SERVER_VM;
		StructArray <NetClient> netclients;
		bool signal_server_shutdown;

		// Server Functions: NTProtect Related
		void NTProtect_SetPolicies (bool);


	public:
		Winsuite_Server();
		~Winsuite_Server();

		// Functions: Thread Interface
		bool Main();

		// Functions: Callback
		long Server_Worker();

	protected:
		static long _ext_Winsuite_Server_Server_Worker (Winsuite_Server*);
	};




#endif
