// Process.h: Declares everything needed to use the Process.cpp module
//
// Thread-Safe: YES
//
// Requires "define _WIN32_WINNT 0x0501" for "Impersonation Class".
// Requires "define _WIN32_WINNT 0x0501" for "IsWow64Process".
//
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __PROCESS_H
#define __PROCESS_H


	//////////////
	// Consts	//
	//////////////
	extern const long wmaTask, wmaText, wmaFrontVisible, wmaFull;

	extern const int DOS_MAX;


	//////////////
	// Globals	//
	//////////////
	extern bool is_64_bit_os;
	extern class ThreadManager TVM;


	//////////////////////////
	// OS Version Related	//
	//////////////////////////
	bool RunningUnder64BitOS ();

	bool EnablePrivilege (string, BOOL);

	TOKEN_ELEVATION_TYPE GetCurrentUsersElevation ();
	bool IsRemoteDesktopSession ();

	string GetCurrentUserSID ();


	//////////////////////////////
	// Thread/Process Create	//
	//////////////////////////////
	DWORD NewProcess (string, string);
	DWORD NewProcess16 (string, string);
	HANDLE unsecNewThread (void*, void*, DWORD*);


	//////////////////////////////
	// Process Management		//
	//////////////////////////////
	DWORD TaskWaitKernel (DWORD, long);
	bool KillProcess (DWORD, bool = false);
	bool TerminateExplorer ();


	//////////////////////////
	// Standard Functions	//
	//////////////////////////
	string GetCurrentUser ();
	string GetEnvVariable (string);
	void Cutquotes (string*, unsigned char = 34);		// 34 ^= '"');
	string FormatMsg (DWORD, long = FORMAT_MESSAGE_FROM_SYSTEM);
	HANDLE ThreadInfoBox (HWND, string, string = "", long = MB_OK);
	long ThreadInfoBox_Thread (LONG_PTR);


	//////////////////////////////
	// Class: ThreadManager		//
	//////////////////////////////
	class ThreadManager
	{
	private:
		// Consts
		static const long THREAD_REMOVE_DELAY;

		// Critical Section Object
		CriticalSection cs;

		// Cache
		StructArray <DWORD> tmc;
		DWORD MainThread;
		long tmc_count;

	public:
		ThreadManager ();
		~ThreadManager ();
	
		// Functions: Interface
		HANDLE NewThread (void*, void*, DWORD* = 0);
		void RemoveThread ();
		void AwaitShutdown ();
	
		long count ();
		DWORD GetMainThread ();
	};


	//////////////////////////
	// Class: EnumProcs		//
	//////////////////////////
	class EnumProcs
	{
	private:
		// Imports:
		typedef BOOL (WINAPI*DYN_WINAPI_QUERYFULLPROCESSIMAGENAME)	(_In_ HANDLE hProcess, _In_ DWORD dwFlags, _Out_ LPTSTR lpExeName, _Inout_ PDWORD lpdwSize);

		// Types
		class WMTEP
		{
		public:
			string name, path;
			DWORD procid, parentprocid;
		};

		// Critical Section Object
		CriticalSection cs;

		// WinAPI Import Cache.
		DYN_WINAPI_QUERYFULLPROCESSIMAGENAME	DYN_WINAPI_QueryFullProcessImageName;

		// Cache
		StructArray <WMTEP> ep;
		bool enum_done;

	public:
		EnumProcs ();
		~EnumProcs ();

		// Functions: Interface
		void Clear ();
		void Enum ();
	
		// Functions: Virtual Operators
		long Ubound ();
		string exename (long);
		string exepath (long);
		DWORD procid (long);
		DWORD parentprocid (long);

		// Special functions, available after at least one enumeration
		bool IsKernelProcess (DWORD);
	};


	//////////////////////////
	// Class: EnumApps		//
	//////////////////////////
	class EnumApps
	{
	private:
		//  Types
		class WMTEA
		{
		public:
			HWND hwnd;
			DWORD procid;
			string name;
		};

		struct wmtEnumInfo
		{
			StructArray <WMTEA> *sar;
			long flags;
		};

		// Cache
		StructArray <WMTEA> ea;

	public:
		// Critical Section Object
		CriticalSection cs;

		// Functions: Constructor/Destructor Blocks
		EnumApps ();
		~EnumApps ();

		// Functions: Interface
		void Clear	();
		void Enum (long = wmaTask);
		void Take (EnumApps*);
	
		// Functions: Operators
		long Ubound ();
		string name (long);
		HWND hwnd (long);
		DWORD procid (long);

	protected:
		static BOOL CALLBACK EnumWindowsProc (HWND, LPARAM);
	};


	//////////////////////////
	// Class: AppDetect		//
	//////////////////////////
	class AppDetect
	{
	private:
		// Critical Section Object
		CriticalSection cs;

		// Cache
		EnumApps E1, E2;

	public:
		AppDetect ();
		~AppDetect ();
	
		// Functions: Interface
		void Reset ();
		void Update (long = wmaTask);
		bool AppsChanged (EnumApps*);
		bool AppStillExists (HWND, DWORD, string* = 0);
	};


	//////////////////////////////////
	// Class: Impersonate User		//
	//////////////////////////////////
	class ImpersUser
	{
	private:
		// Critical Section Object
		CriticalSection cs;

		// Cache
		HANDLE foreign_user_token;

	public:
		ImpersUser();
		~ImpersUser();

		// Functions: To Start Impersonating the current Thread
		bool Logon(string, string, string = "");
		bool SetThreadToConsoleUsersContext();

		// Functions: To Stop Running Impersonating with the current Thread
		void Logoff();

		// Functions: Cache Interface
		HANDLE GetToken();
	};

#endif
