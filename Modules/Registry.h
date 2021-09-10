// Registry.h: Declares everything needed to use the Registry.cpp module
// 
// Thread-Safe: YES
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __REGISTRY_H
#define __REGISTRY_H


	//////////////////////
	// Declarations		//
	//////////////////////
	extern const HKEY HKCR, HKCU, HKLM;
	extern const long REG_VALUENOTEXIST, REG_NOVALUE, REG_DIRECTORY;


	//////////////////////////////
	// Registry Key Related		//
	//////////////////////////////
	long regCreateKey	(HKEY, string);
	bool regDeleteKey	(HKEY, string);
	bool regKeyExist	(HKEY, string);
	void regEnsureKeyExist (HKEY, string);


	//////////////////////////////////
	// Reading & Changing RegValues	//
	//////////////////////////////////
	long regValueExistType (HKEY, string, string);

	long regRead	(string*, HKEY, string, string);
	long regRead	(long*, HKEY, string, string);
	long regRead64	(string*, HKEY, string, string);
	long regRead64	(long*, HKEY, string, string);
	long _regRead	(HKEY, string, string, string*, long*, bool);

	bool regWrite	(string, HKEY, string, string, bool = false);
	bool regWrite	(long, HKEY, string, string);
	bool _regWrite	(HKEY, string, string, string, long, long);

	bool regDeleteValue (HKEY, string, string);


	//////////////////////////////////////
	// Class: Local Registry Object		//
	//////////////////////////////////////
	class RegObj
	{
	private:
		// Consts
		static const HKEY STD_ROOT;
		static const char STD_KEY[];

		// Critical Section Object
		CriticalSection cs;

		// Cache
		HKEY root;
		string key;

		// Functions: Constructor

	public:
		// Functions: Constructor, Destructor
		RegObj();
		RegObj(HKEY, string);						// (!) This automatically ensures that this key exists
		~RegObj();

		// Functions: Cache Interface
		void SetRoot(HKEY);
		void SetKey(string);
		void SetRootKey(HKEY, string);

		// Functions: Registry Access
		void Init (string, string, bool = false, HKEY = 0, string = "");
		void Init (string, long, HKEY = 0, string = "");

		long Get (string*, string, HKEY = 0, string = "");
		long Get (long*, string, HKEY = 0, string = "");

		bool Set (string, string, HKEY = 0, string = "", bool = false);
		bool Set (long, string, HKEY = 0, string = "");

		bool RegObj::Delete (string, HKEY = 0, string = "");
	};


	//////////////////////////////////
	// Extended User Functions		//
	//////////////////////////////////
	void RegFileExt (string, string, string, string, string, 
					 bool = true, bool = true, bool = true);


	//////////////////////
	// Class: RegDir	//
	//////////////////////
	class RegDir
	{
	private:
		// Critical Section Object
		CriticalSection cs;

		// Cache
		class RDI
		{
		public:
			string name;
			long type;
		};
		StructArray <RDI> rdc;
	
		// Functions
		void Clear();

	public:
		RegDir ();
		~RegDir ();
	
		// Interface
		void Enum (HKEY, string);
		long Ubound ();
	
		// Operators
		long	type			(long);
		string	operator[]		(long);
	};


	//////////////////////////////
	// Class: UACRegEdit		//
	//////////////////////////////
	class UACRegEdit
	{
	private:
		// Consts
		static const char WINSYS_REG_EXE[], TEMP_BATCH_FILE[];

		// Critical Section Object
		CriticalSection cs;

		// Cache
		StructArray <string> queue;

		// Functions: Internal Queue Management
		void AddToQueue (string);
		string BuildRegKey (HKEY, string);

	public:
		// Functions: Constructor, Destructor
		UACRegEdit();
		~UACRegEdit();

		// Functions: Interface
		long CreateKey (HKEY, string);
		bool DeleteKey (HKEY, string);
		bool Write (string, HKEY, string, string, bool = false);
		bool Write (long, HKEY, string, string);
		bool DeleteValue (HKEY, string, string);

		// Functions: Administrative Change Appliance
		bool CommitChangesToRegistry();
	};


#endif
