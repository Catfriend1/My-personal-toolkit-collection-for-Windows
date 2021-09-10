// Inifile.h: Declares everything needed to use the Inifile.cpp module
// 
// Thread-Safe: YES
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __INIFILE_H
#define __INIFILE_H


	//////////////////////
	// Class: Inifile	//
	//////////////////////
	class Inifile
	{
	private:
		// Arrays
		class IniEntry
		{
		public:
			string name;
			string value;
		};
	
		class IniSection
		{
		public:
			string name;
			StructArray <IniEntry> entry;
		};
		StructArray <IniSection> sec;

		// Consts
		static const long INIFILE_CHUNKSIZE;
		static const long LOOKBEFORE_ENTRY_COUNT;
		static const long LOOKAHEAD_ENTRY_COUNT;
	
		// Cache
		long idxlastsec;
		long idxlastentry;
		string strlastsec;
	
		// Properties of INIclass
		bool opt_file_locked;

		// Critical Section
		CriticalSection cs;
	
		// Init / Terminate
		void ResetCache ();

	public:
		// Init / Terminate
		Inifile ();
		~Inifile ();
	
		// Functions: Section Manager
		long AddSection (string);
		void DeleteSection (string);
		void MemDeleteSection (string);
		long FindSection (string);
	
		// Functions: Entry Manager
		void WriteEntry (string, string, string);
		void InitEntry (string, string, string = "");
		void DeleteEntry (string, string);
		void MemDeleteEntry (string, string);
		long FindEntry (string, string);
		string GetEntry (string, string);
	
		// Functions: Enum Contents
		void EnumSections (StructArray <string> *);
		void EnumEntries (string, StructArray <string> *);
	
		// Functions: File I/O System
		void New ();
		bool Open (string, bool = false);
		bool Save (string, bool = false);

		// Functions: RAM I/O System
		bool LoadFromString (string, bool = false);
		bool SaveToString (string*, bool = false);
	};


#endif

