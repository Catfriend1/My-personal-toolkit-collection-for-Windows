// FileSystem.cpp: Standard File System Handling Functions
//
// Thread-Safe:		YES
//


//////////////////
// Includes		//
//////////////////
#include "stdafx.h"
#include "math.h"

#include "_modules.h"


//////////////
// Consts	//
//////////////
const long FS_GENERIC_ERROR			=	0xFFFFFFFF;			// [0xFFFFFFFF]
const int FS_NONE					=	0;
const int FS_NAME					=	1;
const int FS_EXT					=	2;
const int FS_NAMEEXT				=	3;

const char REG_APPS[] = "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths";


//////////////////////////
// Generic Functions	//
//////////////////////////
bool IsDirectory (string file)
{
	DWORD att = GetFileAttributes (&file[0]);
	if (att != FS_GENERIC_ERROR) { return Flag(att, FILE_ATTRIBUTE_DIRECTORY); } else { return false; };
}

bool FileExists (string file)
{
	WIN32_FIND_DATA FD;
	bool bFileFound = false;

	// Search for the file.
	HANDLE hfile = FindFirstFile (&file[0], &FD);
	if (hfile != INVALID_HANDLE_VALUE)
	{
		FindClose (hfile);

		// Return Success.
		bFileFound = true;
	}


	// Return if the file was found.
	return bFileFound;
}

bool IsFileInUse (string strFile)
{
	//
	// Note: Checks if a file is in use by another thread or process.
	//

	// Variables.
	bool bFileInUse = false;
	HANDLE hFile;

	// Test if file is in use by opening it with NO_SHARE access requested.
	hFile = CreateFile(&strFile[0], GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		bFileInUse = true;
	}
	else
	{
		CloseHandle (hFile);
	}


	// Return if file is in use.
	return bFileInUse;
}

string CreateDOS83 (string file)
{
	string buf; buf.resize (MAX_PATH);
	long len = GetShortPathName (&file[0], &buf[0], xlen(buf));
	if ((len > 0) && (len <= MAX_PATH)) { buf.resize (len); } else { buf = file; };
	return buf;
}

string CreateLFN (string file)
{
	string buf; buf.resize (MAX_PATH);
	long len = GetLongPathName (&file[0], &buf[0], xlen(buf));
	if ((len > 0) && (len <= MAX_PATH)) { buf.resize (len); } else { buf = file; };
	return buf;
}

bool FileIsDirectoryPart (string thedir, string thefile)
{
	string checkpath = CSR(thedir, "\\", thefile);
	string fullpath = GetCompletePath (checkpath);
	
	// (?) The first left bytes MATCH the directory and the file does NOT link to the directory
	if (xlen(fullpath) >= xlen(thedir))
	{
		if ((UCase(thedir) == UCase(Left(fullpath, xlen(thedir)))) && 
			(UCase(thedir) != UCase(fullpath)))
		{
			return true;
		}
	}
	return false;
}


//////////////////////////////////////
// File/Path Creation & Deletion	//
//////////////////////////////////////
void EnsureDeleteFile (string file)
{
	SetFileAttributes (&file[0], FILE_ATTRIBUTE_ARCHIVE);
	DeleteFile (&file[0]);
}

void EnsureDeleteDirectory (string kdir)
{
	EnumDir ed;
	string curobj;
	long u;
	
	ed.Enum (kdir, "*.*", FS_NONE);
	u = ed.Ubound();
	
	// Invoke the correct delete function for every file or directory
	for (long i = 1; i <= u; i++)
	{
		// Check the object location and then delete it
		if (FileIsDirectoryPart(kdir, ed[i].file))
		{
			curobj = CSR(kdir, "\\", ed[i].file);
			if (!IsDirectory(curobj))
			{
				EnsureDeleteFile (curobj);
			}
			else
			{
				EnsureDeleteDirectory (curobj);
			}
		}
		else
		{
			FatalAppExit (0, "FileSystem::EnsureDeleteDirectory->Invalid Enum");
			return;
		}
	}
	
	// Kill the now empty directory
	RemoveDirectory (&kdir[0]);
}

string CreateTempFile (string mask)
{
	string tempdir = GetTempFolder();
	string buf; buf.resize (MAX_PATH);
	if (xlen(tempdir) != 0)
	{
		long res = GetTempFileName(&tempdir[0], &mask[0], 0, &buf[0]);
		if (res != 0)
		{
			buf.resize (strlen(&buf[0]));
			return buf;
		}
	}
	return "";
}

void MakePath (string path)
{
	SECURITY_ATTRIBUTES SCA;
	StringArray f;
	string temp;
	long u;
	
	Cutquotes (&path, 34);			// Remove quotes, are ensured & needed during calls
	Cutb (&path); path += "\\";		// Ensure Backslash
	// (!) GetPathFromCall needs correct path in quotes
	while (GetPathFromCall(GetPathFromCall(CSR(Chr(34), path, Chr(34)))) != 
		   GetPathFromCall(CSR(Chr(34), path, Chr(34))))
	{
		temp = GetPathFromCall (CSR(Chr(34), path, Chr(34)));
		if (temp != path)
		{
			u = f.Ubound(); u++; f.Redim (u);
			f[u] = temp;
			path = temp;
		}
	}
	for (long i = f.Ubound(); i >= 1; i--)
	{
		CreateDirectory (&(f[i])[0], &SCA);
	}
}


//////////////////////////
// File I/O Related		//
//////////////////////////
bool ExtractResource (LPCTSTR res_id, string fullfn, string res_type)
{
	bool fail_success = false;
	long res_size;
	HGLOBAL res_memory;
	LPBYTE res_data;
	HANDLE hfile;
	DWORD numberofbyteswritten;
	HRSRC res_ref;

	// Locate Resource
	res_ref = FindResource(NULL, (LPCTSTR) res_id, &res_type[0]);

	// Load Resource into Memory
	if (res_ref == 0) { return false; };
	res_size = SizeofResource(NULL, res_ref);
	if (res_size == 0) { return false; };
	res_memory = LoadResource(NULL, res_ref);
	if (res_memory == 0) { return false; };
	res_data = (LPBYTE) LockResource(res_memory);
	if (res_data == 0) { return false; };
	
	// Store it to the specified file in current Application Path
	// Create or open the target file
	hfile = CreateFile (&fullfn[0], 
								GENERIC_WRITE, FILE_SHARE_READ, 0, 
								CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, 0);
	if (hfile != INVALID_HANDLE_VALUE)
	{
		// Write RAM to File
		WriteFile (hfile, &res_data[0], res_size, &numberofbyteswritten, NULL);
		if (numberofbyteswritten == (DWORD) res_size)
		{
			// (!) Success
			fail_success = true;
		}
		CloseHandle (hfile);
	}
	
	// Display Message if unsuccessful
	if (!fail_success) { InfoBox (0, CSR("Error writing RAM Image to Disk: \"", GetFilenameFromCall(fullfn), "\""), "Application Initialization", MB_OK | MB_ICONSTOP); };

	// Return Success
	return true;
}

BOOL WriteFileData (HANDLE hfile, string buffer)
{
	DWORD numberofbyteswritten;
	return WriteFile (hfile, &buffer[0], xlen(buffer), &numberofbyteswritten, NULL);
}


//////////////////////////////////
// Calculations	& Information	//
//////////////////////////////////
void TranslateUTC (FILETIME ft, SYSTEMTIME* st)
{
	FILETIME local;
	FileTimeToLocalFileTime (&ft, &local);
	FileTimeToSystemTime (&local, st);
}

string Attrib2Str (DWORD att)
{
	string c;
	if (att & FILE_ATTRIBUTE_DIRECTORY)		{c += "D";};
	if (att & FILE_ATTRIBUTE_ARCHIVE)		{c += "A";};
	if (att & FILE_ATTRIBUTE_READONLY)		{c += "R";};
	if (att & FILE_ATTRIBUTE_HIDDEN)		{c += "H";};
	if (att & FILE_ATTRIBUTE_SYSTEM)		{c += "S";};
	if (att & FILE_ATTRIBUTE_NORMAL)		{c += "N";};
	if (att & FILE_ATTRIBUTE_COMPRESSED)	{c += "C";};
	if (att & FILE_ATTRIBUTE_TEMPORARY)		{c += "T";};
	return c;
}

string fByte (double bytes)
{
	string c;
	long t;
	long nk;
	long ac;			// Maximal gewünschte Nachkommastellen fuer aktuellen Abschnitt
	
	if (bytes < pow((double) 1024, 1))
	{
		// Bytes
		c = CStr(floor(bytes)); c += " Bytes";
	}
	else if (bytes < pow((double) 1024, 2))
	{
		// KBytes
		c = CStr(bytes / 1024);
		t = InStrFirst(c, '.');
		if (t != 0)
		{
			nk = xlen(c) - t;
			
			// No commas at KBytes
			ac = 0;

			// Zu viele Nachkommastellen?
			if (nk > ac)
			{
				if (ac == 0) { c = Left(c, t - 1); } else { c = Left(c, t + ac); };
			}
		}
		c += " KB";
	}
	else if (bytes < pow((double) 1024, 3))
	{
		// MBytes
		c = CStr(bytes / pow((double) 1024, 2));
		t = InStrFirst(c, '.');
		if (t != 0)
		{
			nk = xlen(c) - t;

			// Max. one digits after comma at MBytes
			ac = 1;

			// Too many digits after comma?
			if (nk > ac)
			{
				if (ac == 0) { c = Left(c, t - 1); } else { c = Left(c, t + ac); };
			}
		}
		c += " MB";
	}
	else if (bytes < pow((double) 1024, 4))
	{
		// GBytes
		c = CStr(bytes / pow((double) 1024, 3));
		t = InStrFirst(c, '.');
		if (t != 0)
		{
			nk = xlen(c) - t;

			// Max. two digits after comma at GBytes
			ac = 2;

			// Too many digits after comma?
			if (nk > ac)
			{
				if (ac == 0) { c = Left(c, t - 1); } else { c = Left(c, t + ac); };
			}
		}
		c += " GB";
	}
	else
	{
		// TBytes
		c = CStr(bytes / pow((double) 1024, 4));
		t = InStrFirst(c, '.');
		if (t != 0)
		{
			nk = xlen(c) - t;

			// Max. two digits after comma at TBytes
			ac = 2;

			// Too many digits after comma?
			if (nk > ac)
			{
				if (ac == 0) { c = Left(c, t - 1); } else { c = Left(c, t + ac); };
			}
		}
		c += " TB";
	}

	// Replace "." with ",".
	t = InStrFirst(c, '.');
	if (t != 0)
	{ 
		c[t-1] = ','; 
	}
	return c;
}


//////////////////////////////////////
// Directory Retrieval Related		//
//////////////////////////////////////
string GetAppExe ()
{
	string buf;
	buf.resize (MAX_PATH);
	long len = GetModuleFileName (0, &buf[0], MAX_PATH);
	buf.resize (len);
	return buf;
}

string GetAppPath ()
{
	return GetPathFromCall(CSR(Chr(34), GetAppExe(), Chr(34)));
}

string GetWorkDir ()
{
	string buf;
	buf.resize (MAX_PATH);
	long len = GetCurrentDirectory (MAX_PATH, &buf[0]);
	if ((len > 0) && (len <= MAX_PATH))
	{
		buf.resize (len);
		Cutb (&buf);
		return buf;
	}
	return "";
}

string GetWinDir ()
{
	string buf;
	buf.resize (MAX_PATH);
	long len = GetWindowsDirectory (&buf[0], MAX_PATH);
	if ((len > 0) && (len <= MAX_PATH))
	{
		buf.resize (len);
		Cutb (&buf);
		return buf;
	}
	return "";
}

string GetSystemDir32 ()
{
	string buf;
	buf.resize (MAX_PATH);
	long len = GetSystemDirectory (&buf[0], MAX_PATH);
	if ((len > 0) && (len <= MAX_PATH))
	{
		buf.resize (len);
		Cutb (&buf);
		return buf;
	}
	return "";
}

string GetSystemDirWow64 ()
{
	string buf;
	buf.resize (MAX_PATH);
	long len = GetSystemWow64Directory (&buf[0], MAX_PATH);
	if ((len > 0) && (len <= MAX_PATH))
	{
		buf.resize (len);
		Cutb (&buf);
		return buf;
	}
	return "";
}

string GetTempFolder ()
{
	string buf;
	buf.resize (MAX_PATH);
	long len = GetTempPath (MAX_PATH, &buf[0]);
	if ((len > 0) && (len <= MAX_PATH))
	{
		buf.resize (len);
		Cutb (&buf);
		return buf;
	}
	return "";
}

string GetShellFolder (int shell_folder_id)
{
	string folder_path;
	long idx;

	// CSIDL_APPDATA			=> Servergespeichert, Ein Benutzer
	// CSIDL_LOCAL_APPDATA		=> Nicht Servergespeichert, Ein Benutzer
	// CSIDL_COMMON_APPDATA		=> Nicht Servergespeichert, Alle Benutzer

	// Prepare Buffer and retrieve the shell folder path
	folder_path.resize (MAX_PATH);
	if (SUCCEEDED(SHGetFolderPath(0, shell_folder_id, 0, SHGFP_TYPE_CURRENT, &folder_path[0])))
	{
		idx = InStrFirst(folder_path, '\0');
		return (idx > 0) ? Left(folder_path, idx - 1) : "";
	}
	else
	{
		#ifdef _DEBUG
			DebugOut ("FileSystem::GetShellFolder->The requested shell folder path was not found");
		#endif
		return "";
	}
}


//////////////////////////////////
// Path & Intelligence Related	//
//////////////////////////////////
string LookUpSystemPaths (string file)
{
	string res, ret;
	long pos;
	
	string path = GetEnvVariable ("PATH");

	// Add Windows Directories
	path += CSR(";", GetWinDir(), ";", GetSystemDir32(), ";");
	if (is_64_bit_os == true)
	{
		// Look in "\\Windows\SysWow64" directory.
		path += CSR(GetSystemDirWow64(), ";");
	}

	while (xlen(path) != 0)										// Still data available?
	{
		pos = InStrFirst (path, ';');
		if (pos == 0) { pos = xlen(path) + 1; };				// If no ";" is present
		string res = LookUpPath (Left(path, pos - 1), file);
		if (xlen(res) != 0) { ret = res; break; };				// Success!
		path = Right(path, xlen(path) - pos);
	}
	return ret;
}

string LookUpPath (string path, string file)
{
	string ret;
	string tmp = CSR(path, "\\", file);
	
	if (xlen(GetExtFromFilename(file)) == 0)			// No Extension given
	{
		if (FileExists(CSR(tmp, ".com"))) { ret = CSR(tmp, ".com"); }
		else if (FileExists(CSR(tmp, ".exe"))) { ret = CSR(tmp, ".exe"); }
		else if (FileExists(CSR(tmp, ".bat"))) { ret = CSR(tmp, ".bat"); }
		else if (FileExists(CSR(tmp, "."))) { ret = tmp; };
	}
	else
	{
		if (FileExists(tmp)) { ret = tmp; };
	}
	return ret;
}

string GetFilenameFromCall (string call)
{
	long space = FindSpaceParameter(call);
	if (space != 0)
	{
		call.resize (space - 1);
	}
	
	// Cut the quotes around the path & filename if necessary
	Cutquotes (&call);
	
	// Find the backslash and extract the left part
	long bs = InStrLast(call, '\\');
	if (bs != 0)
	{
		call = Right(call, xlen(call) - bs);
	}
	return call;		// Quick calls are identified by same PATH and FILE
}

string GetPathFromCall (string call)
{
	long space = FindSpaceParameter(call);
	if (space != 0)
	{
		call.resize (space - 1);
	}
	
	// Cut the quotes around the path & filename if necessary
	Cutquotes (&call);
	
	// Find the backslash and extract the right part
	long bs = InStrLast(call, '\\');
	if (bs != 0)
	{
		call.resize (bs - 1);
	}
	return call;		// Quick calls are identified by same PATH and FILE
}

long FindSpaceParameter (string call)
{
	long quote = 0;
	if (xlen(call) > 0)
	{
		if (call[0] == 34)						// '"'
		{
			quote = InStrFirst(call, 34, 2);			// Find the second quote
		}
	}
	// Begin searching for SPACE which indicates a parameter list
	// (if not 0, begin at position quote+1)
	return InStrFirst(call, 32, quote+1);
}

string GetParamsFromCall (string call)
{
	long space = FindSpaceParameter(call);
	if (space != 0)
	{
		// Now we have the parameter list
		return Right(call, xlen(call) - space);
	}
	return "";
}

string GetExtFromFilename (string file)
{
	long pos = InStrLast(file, '.');
	if (pos != 0)
	{
		return Right(file, xlen(file) - pos);
	}
	return "";
}

void Cutb (string* path)
{
	size_t len = (*path).length();
	if (len > 0)			// Avoid Access Violation
	{
		if ((*path)[(long) len-1] == '\\')
		{
			path->resize (len-1);
		}
	}
}

string BeautyFilename (string file)
{
	if (file == UCase(file))
	{
		return CSR(Left(file, 1), LCase(Right(file, xlen(file) - 1)));
	}
	return file;
}

string GetCompletePath (string pathref)
{
	string buf; buf.resize (MAX_PATH);
	long len = GetFullPathName (&pathref[0], xlen(buf), &buf[0], 0);
	if ((len > 0) && (len <= MAX_PATH))
	{
		buf.resize (len);
		return buf;
	}
	return "";
}


//////////////////////////
// CommonDialog Usage	//
//////////////////////////
string CreateFilter (string caption, string exts)
{
	return CSR(caption, Chr(0), exts, Chr(0));
}

string cdlgOpenFile (HWND Parent, string Title, string InitDir, 
					 string Filter, long FilterIndex)
{
	string file; file.resize (MAX_PATH); file[0] = '\0';
	string ret;
	
	OPENFILENAME ofn;
	ofn.hwndOwner = Parent;
	ofn.hInstance = 0;
	ofn.lpstrFilter = &Filter[0];
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = FilterIndex;
	ofn.lpstrFile = &file[0];
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = &InitDir[0];
	ofn.lpstrTitle = &Title[0];
	ofn.Flags = OFN_HIDEREADONLY;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = "";
	ofn.lCustData = 0;
	ofn.lpfnHook = 0;
	ofn.lpTemplateName = NULL;
	ofn.FlagsEx = OFN_EX_NOPLACESBAR;
	ofn.lStructSize = sizeof(ofn);
	
	// Call OFN API
	if (GetOpenFileName(&ofn) != 0)
	{
		// Selection has been made by the user
		ret = ofn.lpstrFile;
	}

	// Return FullFN if any file has been selected
	return ret;
}

string cdlgSaveFile (HWND Parent, string Title, string InitDir, 
					 string Filter, long FilterIndex)
{
	string file; file.resize (MAX_PATH); file[0] = '\0';
	string ret;
	
	OPENFILENAME ofn;
	ofn.hwndOwner = Parent;
	ofn.hInstance = 0;
	ofn.lpstrFilter = &Filter[0];
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = FilterIndex;
	ofn.lpstrFile = &file[0];
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = &InitDir[0];
	ofn.lpstrTitle = &Title[0];
	ofn.Flags = OFN_HIDEREADONLY;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = "";
	ofn.lCustData = 0;
	ofn.lpfnHook = 0;
	ofn.lpTemplateName = NULL;
	ofn.FlagsEx = OFN_EX_NOPLACESBAR;
	ofn.lStructSize = sizeof(ofn);

	// Call OFN API
	if (GetSaveFileName(&ofn) != 0)
	{
		// Selection has been made by the user
		ret = ofn.lpstrFile;
	}

	// Return FullFN if any file has been selected
	return ret;
}


//////////////////////////////
// Resolving Shell Calls	//
//////////////////////////////
HINSTANCE ShellExecute (HWND hwnd, string call)
{
	string path, file, param;
	ShellExResolve (call, &path, &file, &param);
	return ShellExecute (hwnd, 0, &file[0], &param[0], &path[0], SW_SHOWNORMAL);
}

HINSTANCE ShellExecuteRunAs (HWND hwnd, string call)
{
	string path, file, param;
	ShellExResolve (call, &path, &file, &param);
	return ShellExecute (hwnd, "runas", &file[0], &param[0], &path[0], SW_SHOWNORMAL);
}

DWORD ProcExecute (string call)
{
	string path, file, param;
	ShellExResolve (call, &path, &file, &param);
	return NewProcess16 (CSR(path, "\\", file), param);
}

void ShellExResolve (string call, string* retpath, string* retfile, string* retparam)
{
	string path = GetPathFromCall (call);
	string file = GetFilenameFromCall (call);
	string param = GetParamsFromCall (call);
	
	long len = xlen(file);
	if (len > 0)					// Avoid Access Violation
	{
		// Check if it is an Environment Variable
		if ((file[0] == '%') && (file[len-1] == '%'))
		{
			string var = GetEnvVariable(file);
			if (xlen(var) != 0)			// Success?
			{
				if (var != call)		// Do not allow loopback
				{
					string tmpparam;
					ShellExResolve (var, &path, &file, &tmpparam);
					param = CSR(tmpparam, " ", param);		// Combine with pre-def params
				}
			}
		}
		// Check if it is a Quick Call
		else if (path == file)
		{
			string tmpparam;
			string res = LookUpSystemPaths (file);

			// File was found in System Path
			if (xlen(res) != 0)			
			{
				ShellExResolve (res, &path, &file, &tmpparam);			// Skip param
			}

			// Look up Windows' Quick Call list
			else
			{
				string qc = file, regfile, regpath;
				if (xlen(GetExtFromFilename(file)) == 0)
				{
					qc += ".exe";							// Ensure .exe
				}

				regRead (&regfile, HKLM, CSR(REG_APPS, "\\", qc), "");
				regRead (&regpath, HKLM, CSR(REG_APPS, "\\", qc), "Path");

				if (xlen(regfile) != 0)		// // File was found in Quick Call list
				{
					Cutb (&regpath);
					ShellExResolve (CSR(Chr(34), regfile, Chr(34)), &path, &file, &tmpparam);
					if (xlen(regpath) != 0)
					{
						path = regpath;
					}
					param = CSR(tmpparam, " ", param);		// Combine with pre-def params
				}
			}
		}
		else
		// Just a normal call
		{
			string res = LookUpPath (path, file);
			if (xlen(res) != 0)			// Success
			{
				file = GetFilenameFromCall(CSR(Chr(34), res, Chr(34)));
			}
		}
	}
	
	// Debug Vector
	// MessageBox (0, CSR("->", path, "<-"), "PATH", MB_OK | MB_SYSTEMMODAL);
	// MessageBox (0, CSR("->", file, "<-"), "FILE", MB_OK | MB_SYSTEMMODAL);
	// MessageBox (0, CSR("->", param, "<-"), "PARAMETERS", MB_OK | MB_SYSTEMMODAL);

	// Return Resolved Call Information
	*retpath = path;
	*retfile = file;
	*retparam = param;
}


//////////////////////////////
// Shutdown Functions		//
//////////////////////////////
int LogoffUser (bool force)
{
	if (force) { TerminateExplorer(); };
	return ExitWindowsEx (EWX_LOGOFF | 
							((force) ? EWX_FORCE : 0), 
							0);
}

int ShutdownWindows (bool force)
{
	return ExitWindowsEx (EWX_SHUTDOWN | EWX_POWEROFF | 
							((force) ? EWX_FORCE : 0), 
							0);
}

int RestartWindows (bool force)
{
	return ExitWindowsEx (EWX_REBOOT | 
							((force) ? EWX_FORCE : 0), 
							0);
}


//////////////////////////////////
// File Handling Functions		//
//////////////////////////////////
string GetFileInfo (string file)
{
	SHFILEINFOA si;
	DWORD_PTR res;

	res = SHGetFileInfo(&file[0], 0, &si, sizeof(si), SHGFI_TYPENAME);
	return (res != 0) ? si.szTypeName : "";
}

HICON GetFileIcon (string file, bool sizebig)
{
	SHFILEINFOA si;
	long size = (sizebig) ? SHGFI_ICON : 0x101;

	// Get File Icon from Windows Shell
	SHGetFileInfo (&file[0], 0, &si, sizeof(si), size);

	// Return Icon Handle if successfull
	return (si.hIcon == (HICON) 0xcccccccccccccccc) ? 0 : si.hIcon;
}

string ShellFolder (HWND hwnd, long foldercode)
{
	long res;
	LPITEMIDLIST cache;
	string buf;
	
	res = SHGetSpecialFolderLocation(hwnd, foldercode, &cache);
	if (res == 0)
	{
		buf.resize (MAX_PATH);
		res = SHGetPathFromIDList(cache, &buf[0]);
		if (res == 1)
		{
			return &buf[0];
		}
	}
	return "";
}

void EmptyRecycleBin (bool confirm, bool progressui, bool sound)
{
	long flags = 0;

	// Build up flags
	if (!confirm)		{ flags = flags | SHERB_NOCONFIRMATION; };
	if (!progressui)	{ flags = flags | SHERB_NOPROGRESSUI; };
	if (!sound)			{ flags = flags | SHERB_NOSOUND; };

	// Empty the recycle bin of Windows Explorer
	SHEmptyRecycleBin (0, NULL, flags);
}


//////////////////////
// Class: EnumDir	//
//////////////////////
EnumDir::EnumDir ()
{
};

EnumDir::~EnumDir ()
{
	Clear();
}

void EnumDir::Clear ()																					// public
{
	cs.Enter();
	wmc.Clear();
	cs.Leave();
}

void EnumDir::Enum (string path, string mask, int sortmode)												// public
{;
	WIN32_FIND_DATA FD;
	HANDLE hfile;
	string search;
	long u = 0;

	// Enter Critical Section
	cs.Enter();

	// Prepare Enumerating
	Clear();
	Cutb (&path);
	if (xlen(mask) == 0) { mask = "*.*"; };
	search = CSR(path, "\\", mask);

	// Start Enumerating
	hfile = FindFirstFile (&search[0], &FD);
	if (hfile != INVALID_HANDLE_VALUE)
	{
		while (hfile != 0)
		{
			if (((string) FD.cFileName != ".") && 
				((string) FD.cFileName != ".."))	// (!) DANGER
			{
				u++; wmc.Redim (u);
				wmc[u].file = BeautyFilename(FD.cFileName);
				wmc[u].attrib = FD.dwFileAttributes;
				wmc[u].filesize =  (FD.nFileSizeHigh * MAXDWORD) + FD.nFileSizeLow;
				TranslateUTC (FD.ftCreationTime, &wmc[u].creation);
				TranslateUTC (FD.ftLastWriteTime, &wmc[u].lastwrite);
				TranslateUTC (FD.ftLastAccessTime, &wmc[u].lastaccess);
			}
			if (!FindNextFile(hfile, &FD))
			{
				break;
			}
		}
		FindClose (hfile);
	}
	
	// Sort the results
	if (sortmode != FS_NONE)
	{
		if (sortmode == FS_NAMEEXT)
		{
			ShellSort ();
		}
		else
		{
			HeapSort (sortmode);
		}
	}

	// Leave Critical Section
	cs.Leave();
}

void EnumDir::HeapSort (int sortmode)																	// public
{
	long heap;
	
	// Enter Critical Section
	cs.Enter();

	// Perform Heapsort Algorithm
	heap = wmc.Ubound();
	if (heap > 0)
	{
		for (long i = (heap / 2); i;)
		{
			AdjustHeap (--i, heap, sortmode);
		}
		while (--heap)
		{
			wmc.Swap (1, heap+1);
			AdjustHeap (0, heap, sortmode);
		}
	}

	// Leave Critical Section
	cs.Leave();
}

void EnumDir::AdjustHeap (long kabutt, long heap, int sortmode)											// private
{
	WMTDIR k;
	long next;
	
	// Enter Critical Section [Recursive]
	cs.Enter();

	// Adjust Heap Algorithm
	k = wmc[kabutt+1];
	while (kabutt < (heap / 2))
	{
		next = (2 * kabutt) + 1;		// Left part
		if (next < (heap - 1)) 
		{
			// Right part if false (true => left part is correct)
			if (SortCondition1(wmc[next+1].file, wmc[next+2].file, sortmode)) { next++; };	
		}
		if (!SortCondition1(k.file, wmc[next+1].file, sortmode))
		{
			break;
		}
		wmc[kabutt+1] = wmc[next+1];
		kabutt = next;
	}
	wmc[kabutt+1] = k;

	// Leave Critical Section
	cs.Leave();
}

bool EnumDir::SortCondition1 (string a, string b, int sortmode)											// private
{
	// No CS, because of: PRIVATE / gets called from AdjustHeap's CS
	if (sortmode == FS_NAME)
	{
		return UCase(a) < UCase(b);
	}
	else if (sortmode == FS_EXT)
	{
		return UCase(GetExtFromFilename(a)) <= UCase(GetExtFromFilename(b));
	}
	else
	{
		FatalAppExit (0, "FileSystem::EnumDir::HeapSort->Invalid Parameter");
		return false;
	}
}

void EnumDir::ShellSort ()																				// public
{
	long u;
	long i, j, h;
	WMTDIR wc;

	// Enter Critical Section
	cs.Enter();
	
	// Perform ShellSort Algorithm
	u = wmc.Ubound();
	for (h = 1; h <= (u / 9); h = (3 * h) + 1);
	
	for ( ; h > 0; h /= 3)
	{
		for (i = h; i < u; i++)
		{
			wc = wmc[i+1];
			for (j = i; (j >= h) && 
					!SortCondition2(wmc[j-h+1].file, wc.file, wmc[j-h+1].attrib, wc.attrib); j -=h)
			{
				wmc[j+1] = wmc[j-h+1];
			}
			wmc[j+1] = wc;
		}
	}

	// Leave Critical Section
	cs.Leave();
}

bool EnumDir::SortCondition2 (string a, string b, long aa, long ab)										// private
{
	// No CS, because of: PRIVATE / gets called from ShellSort's CS
	string uca = UCase(a), ucb = UCase(b);
	return !(
				// Failure if first comes a file and then a directory
				(!Flag(aa, FILE_ATTRIBUTE_DIRECTORY) && 
				Flag(ab, FILE_ATTRIBUTE_DIRECTORY)) 
				
				// OR
				|| 
				
				// if the extensions match and the text mismatches
				((GetExtFromFilename(uca) == GetExtFromFilename(ucb)) && (uca > ucb))
				
				// OR
				||
				
				// if the extensions just mismatch
				(GetExtFromFilename(uca) > GetExtFromFilename(ucb))
																			);
}

WMTDIR&	EnumDir::operator[]	(long idx)																	// public
{
	WMTDIR* ret;

	// CS
	cs.Enter();
	ret = &wmc[idx];
	cs.Leave();

	// Return Object Pointer --> Be CAREFUL in the CALLING THREAD!
	return *ret;
}

long EnumDir::Ubound ()																					// public
{
	long u;
	
	// CS
	cs.Enter();
	u = wmc.Ubound();
	cs.Leave();

	// Return Item Count in the current cache
	return u;
}

