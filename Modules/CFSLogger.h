// CFSLogger.h: Declares everything needed to use the CFSLogger.cpp module
// 
// Thread-Safe: YES
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __CFSLOGGER_H
#define __CFGLOGGER_H


	//////////////////////////
	// CFS Logger Class		//
	//////////////////////////
	class CFSLog
	{
	private:
		Inifile log;
		long maxsize;

		// Critical Section
		CriticalSection cs;
	
		// Private Functions
		long CalculateSize ();

	public:
		// Init / Terminate
		CFSLog ();
		~CFSLog ();
	
		// Interface
		void New ();
		void Add (string);
		void DeleteOldEntries ();
	
		// File I/O
		bool Open (string);
		bool Save (string);

	
		// Settings
		void SetMaxSize (long = -1);
	};


#endif
