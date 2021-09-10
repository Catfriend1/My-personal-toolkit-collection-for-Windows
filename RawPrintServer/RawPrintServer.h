// RawPrintServer.h: Declares everything needed to use the RawPrintServer.cpp module.
//
// Thread-Safe: NO
//



//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __RAWPRINTSERVER_H
#define __RAWPRINTSERVER_H



	//////////////////
	// Includes		//
	//////////////////
	// #include ...



	//////////////////////////////
	// Class: RawPrintServer	//
	//////////////////////////////
	class RawPrintServer
	{
	private:
		// Consts
		// static const bool DEBUG_MODE;

		// Types
		// static struct NetClient;

		// Critical Section Object
		CriticalSection cs;

		// Cache
		// string cache_locktime;

	
		// Cache: Network Server
		//csocket srv;
		//ThreadManager SERVER_VM;
		//StructArray <NetClient> netclients;
		//bool signal_server_shutdown;

	public:
		RawPrintServer();
		~RawPrintServer();

		// Functions: Class
		bool RawPrintServer::Run (string, int = 9100);

	protected:
		// static long _ext_RawPrintServer_Server_Worker (RawPrintServer*);
	};




#endif

