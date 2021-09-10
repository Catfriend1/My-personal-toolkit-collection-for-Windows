// NetMsg_Server.h: Defines everything needed to use the NetMsg_Server.cpp module.
//
// Thread-Safe: YES
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __NETMSG_SERVER_H
#define __NETMSG_SERVER_H


	//////////////////////////////
	// Class: NetMsg_Server		//
	//////////////////////////////
	class NetMsg_Server
	{
	private:
		// Consts
		static const long NETMSG_TCP_PORT;
		static const char NETMSG_CAPTION[];
		static const char SECTION_CLIENTS[];

		// static const long TIMER_DELAY;

		// Types
		struct NetClient;

		// Cache: Client Data Storage
		Inifile cds;
		string strServerPassword;

		// Cache: Server
		csocket srv;
		ThreadManager SERVER_VM;
		StructArray <NetClient> netclients;
		CriticalSection cs;
		bool signal_server_shutdown;

		// Functions

	public:
		NetMsg_Server();
		~NetMsg_Server();

		// Functions
		bool Init(string);				// Init (string strServerPassword)
		bool Main();

		// Callback
		long Server_Worker();

	protected:
		static long _ext_NetMsg_Server_Server_Worker (NetMsg_Server*);
	};


#endif
