// Wsock2.h: Declares everything needed to use the Wsock2.cpp module
//
// Thread-Safe: YES
//

//////////////////////
// Implementing		//
//////////////////////
//
// --> Include "WinSock2.h"
// --> Include "wininet.h"
//
// --> Link into "ws2_32.lib"
// --> Link into "Mpr.lib"
// --> Link into "wininet.lib", "rasapi32.lib"
//
// On Application Startup: CALL WS2_START() ONCE TO INITIALIZE WINSOCK2 MODULE OF WINDOWS API!
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __WSOCK2_H
#define __WSOCK2_H


	//////////////
	// Types	//
	//////////////
	class RNAENUM
	{
	public:
		DWORD hconn;
		string name;
	};


	//////////////
	// Cache	//
	//////////////
	extern HMODULE RNA_NT_LIB_DLL;


	//////////////////////////
	// C++ Socket Class		//
	//////////////////////////
	class csocket
	{
		private:
			// Consts
			static const long WSII_MAX_RECV;
			static const long MOD_PROTO_DESC_LEN;

			// Critical Section Object
			CriticalSection cs;

			// Cache
			SOCKET mysocket;
			string recv_cache;
			string recv_data;
			long recv_pos;
			long recv_end;
			bool i_have_taken_a_socket;

			// Init, Terminate, Constructors, Destructor
			void Init();

			// Internal TCP Protocol Handling for own layer
			bool HandleReceivedFragment (string, string*);

		public:
			// Init, Terminate, Constructors, Destructor
			csocket();
			csocket(SOCKET);
			~csocket();
			void Take (SOCKET);
		
			// Socket Creation & Shutdown
			bool Create();
			bool Close();
		
			// Socket Bind, Listen, Accept
			bool Bind (string, int);
			bool Listen ();
			bool Accept (SOCKET*);

			// Socket Connect
			bool Connect (string, int);
		
			// Socket Status
			bool readable ();
			bool connected ();
		
			// Socket Data Exchange
			bool raw_send (string);
			bool mod_send (string);
		
			bool raw_recv (string*, bool*);
			bool EnterReceiveLoop (string*, bool*);
		
			// User Interface
			SOCKET Get();
	};


	//////////////////////////
	// Helper Functions		//
	//////////////////////////
	bool WS2_Start ();

	long WS2_DetLngAddress (string);
	string WS2_DetIPAddress (string);

	bool WS2_TestMemory (long);


	//////////////////////////
	// Class: CommQueue		//
	//////////////////////////
	class CommQueue
	{
	private:
		// Critical Section Object
		CriticalSection cs;

		// Cache
		StructArray <string> queue;

	public:
		CommQueue ();
		~CommQueue ();
	
		// Functions: Interface
		void Clear ();
		void Add (string);
		string Get ();
		bool SearchAndRemove (unsigned char, string*);
	};


	//////////////////////////
	// Class: EnumNetwork	//
	//////////////////////////
	class WMTNET
	{
		public:
			string local, remote, prov, comment;
			DWORD type, disptype;
	};

	class EnumNetwork
	{
	private:
		// Critical Section Object
		CriticalSection cs;

		// Cache
		StructArray <WMTNET> wmnet;
	
		// Functions
		void EnumResource (NETRESOURCE*);

	public:
		EnumNetwork ();
		~EnumNetwork ();
	
		// Functions: Interface
		void Clear ();
		void Enum ();
	
		// Functions: Operators
		WMTNET& operator[] (long);
		long Ubound ();
	};



	//////////////////////
	// Network Info		//
	//////////////////////
	string GetComputerName ();


	//////////////////////////////
	// RAS Module Bootstrap		//
	//////////////////////////////
	bool RNA_NT_LoadModules ();
	void RNA_NT_UnloadModules ();


	//////////////////////////////////////////////
	// Functions: RNA Connect / Disconnect		//
	//////////////////////////////////////////////
	DWORD RNA_Connect (HWND, string, long = INTERNET_AUTODIAL_FORCE_ONLINE);
	bool RNA_Disconnect (DWORD);


	//////////////////////////////
	// Functions: WinInet		//
	//////////////////////////////
	bool WinInet_DownloadFile (string*, string, string* = NULL, DWORD = INTERNET_OPEN_TYPE_PRECONFIG);


#endif

