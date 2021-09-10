// FileSystem.h: Declares everything needed to use the FileSystem.cpp module
// 
// Thread-Safe:		YES
//
// -----------------------------------
// (!) Requires _WIN32_WINNT >= 0x0500
// -----------------------------------
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __FILESYSTEM_H
#define __FILESYSTEM_H


	//////////////////////
	// Declarations		//
	//////////////////////
	extern const int FS_NONE, FS_NAME, FS_EXT, FS_NAMEEXT;
	extern const char REG_APPS[];


	//////////////////////////
	// Generic Functions	//
	//////////////////////////
	bool IsDirectory (string);
	bool FileExists (string);
	bool IsFileInUse (string);
	string CreateDOS83 (string);
	string CreateLFN (string);
	bool FileIsDirectoryPart (string, string);


	//////////////////////////////////////
	// File/Path Creation & Deletion	//
	//////////////////////////////////////
	void EnsureDeleteFile (string);
	void EnsureDeleteDirectory (string);
	string CreateTempFile (string);
	void MakePath (string);


	//////////////////////////
	// File I/O Related		//
	//////////////////////////
	bool ExtractResource (LPCTSTR, string, string = "custom");
	BOOL WriteFileData (HANDLE, string);


	//////////////////////////////////
	// Calculations	& Information	//
	//////////////////////////////////
	void TranslateUTC (FILETIME, SYSTEMTIME*);
	string Attrib2Str (DWORD);
	string fByte (double);


	//////////////////////////////////////
	// Directory Retrieval Related		//
	//////////////////////////////////////
	string GetAppExe ();
	string GetAppPath ();
	string GetWorkDir ();

	string GetWinDir ();
	string GetSystemDir32 ();
	string GetSystemDirWow64 ();
	string GetTempFolder ();
	string GetShellFolder (int);


	//////////////////////////////////
	// Path & Intelligence Related	//
	//////////////////////////////////
	string LookUpSystemPaths (string);
	string LookUpPath (string path, string);
	string GetFilenameFromCall (string);
	string GetPathFromCall (string);
	long FindSpaceParameter (string);
	string GetParamsFromCall (string);
	string GetExtFromFilename (string);
	void Cutb (string* path);
	string BeautyFilename (string);
	string GetCompletePath (string);


	//////////////////////////
	// CommonDialog Usage	//
	//////////////////////////
	string CreateFilter (string, string);
	string cdlgOpenFile (HWND, string, string, string, long);
	string cdlgSaveFile (HWND, string, string, string, long);


	//////////////////////////////
	// Resolving Shell Calls	//
	//////////////////////////////
	HINSTANCE ShellExecute (HWND, string);
	HINSTANCE ShellExecuteRunAs (HWND, string);
	DWORD ProcExecute (string);

	void ShellExResolve (string, string*, string*, string*);


	//////////////////////////////
	// Shutdown Functions		//
	//////////////////////////////
	int LogoffUser (bool);
	int ShutdownWindows (bool);
	int RestartWindows (bool);


	//////////////////////////////////
	// File Handling Functions		//
	//////////////////////////////////
	string GetFileInfo (string);
	HICON GetFileIcon (string, bool);
	string ShellFolder (HWND, long);
	void EmptyRecycleBin (bool, bool, bool);


	//////////////////////
	// Class: EnumDir	//
	//////////////////////
	class WMTDIR
	{
		public:
			string file;
			DWORD attrib;
			double filesize;
			SYSTEMTIME creation;
			SYSTEMTIME lastwrite;
			SYSTEMTIME lastaccess;
	};

	class EnumDir
	{
	private:
		// Cache
		CriticalSection cs;
		StructArray <WMTDIR> wmc;
	
		// Functions
		void AdjustHeap (long, long, int);
		bool SortCondition1 (string, string, int);
		bool SortCondition2 (string, string, long, long);

	public:
		EnumDir ();
		~EnumDir ();
	
		// Interface
		void Clear ();
		void Enum (string, string = "*.*", int = FS_NONE);
	
		WMTDIR& operator[] (long);
		long Ubound ();
	
		void HeapSort (int);
		void ShellSort ();
	};


#endif
