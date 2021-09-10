// Registry.cpp: Interface Module to use the Windows Registry Services
//
// Thread-Safe: YES
//


//////////////////
// Includes		//
//////////////////
#include "stdafx.h"

#include "_modules.h"


//////////////
// Consts	//
//////////////
const HKEY HKCR = HKEY_CLASSES_ROOT;
const HKEY HKCU = HKEY_CURRENT_USER;
const HKEY HKLM = HKEY_LOCAL_MACHINE;

const long REG_VALUENOTEXIST	= -7;		// [-7]		(!) Custom
const long REG_NOVALUE			= -8;		// [-8]		(!) Custom
const long REG_DIRECTORY		= -1;		// [-1]		(!) Custom


//////////////////////////////
// Registry Key Related		//
//////////////////////////////
long regCreateKey (HKEY root, string subkey)
{
	HKEY hkey; DWORD back;
	long res = RegCreateKeyEx (root, &subkey[0], 0, 0, 
								  REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &hkey, &back);
	if (res == ERROR_SUCCESS)
	{
		if (RegFlushKey(hkey) == ERROR_SUCCESS) { RegCloseKey(hkey); };
	}
	return back;		// back: (0:Error / 1:Success / 2:Exist)
}

bool regDeleteKey (HKEY root, string subkey)
{
	return (RegDeleteKey(root, &subkey[0]) == 0);
}

bool regKeyExist (HKEY root, string subkey)
{
	HKEY hkey;
	long res = RegOpenKeyEx (root, &subkey[0], 0, KEY_READ, &hkey);
	if (res == ERROR_SUCCESS) { RegCloseKey(hkey); };
	return (res == ERROR_SUCCESS);
}

void regEnsureKeyExist (HKEY root, string subkey)
{
	if (!regKeyExist(root, subkey))
	{
		regCreateKey (root, subkey);
	}
}


//////////////////////////////////
// Reading & Changing RegValues	//
//////////////////////////////////
long regValueExistType (HKEY root, string subkey, string name)
{
	HKEY hkey;
	DWORD dwtype, dwlen;
	long res = REG_VALUENOTEXIST;
	
	if (RegOpenKeyEx(root, &subkey[0], 0, KEY_READ, &hkey) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx(hkey, &name[0], 0, &dwtype, 0, &dwlen) == ERROR_SUCCESS)
		{
			res = dwtype;
		}
		RegCloseKey (hkey);	
	}
	return res;
}

long regRead (string* ret, HKEY root, string subkey, string name)
{
	// Read 32-Bit Registry.
	return _regRead (root, subkey, name, ret, 0, false);
}

long regRead (long* ret, HKEY root, string subkey, string name)
{
	// Read 32-Bit Registry.
	return _regRead (root, subkey, name, 0, ret, false);
}

long regRead64 (string* ret, HKEY root, string subkey, string name)
{
	// Read 64-Bit Registry.
	return _regRead (root, subkey, name, ret, 0, true);
}

long regRead64 (long* ret, HKEY root, string subkey, string name)
{
	// Read 64-Bit Registry.
	return _regRead (root, subkey, name, 0, ret, true);
}

long _regRead (HKEY root, string subkey, string name, string* str, long* lng, bool access64bitregistry)
{
	HKEY hkey;
	DWORD type, len;
	long res = REG_NOVALUE;
	
	if (RegOpenKeyEx(root, &subkey[0], 0, 
							access64bitregistry ? (KEY_READ | KEY_WOW64_64KEY) : KEY_READ, 
							&hkey) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx(hkey, &name[0], 0, &type, 0, &len) == ERROR_SUCCESS)
		{
			if (((type == REG_SZ) || 
				 (type == REG_EXPAND_SZ) || 
				 (type == REG_BINARY)
				) && (str != 0))
			{
				string buf;
				buf.resize (len);
				if (RegQueryValueEx(hkey, &name[0], 0, &type, (LPBYTE) &buf[0], &len) == ERROR_SUCCESS)
				{
					if (len == 0) { len = 1; };		// (!) If no value exists, set buffer to zero
					buf.resize (len-1); *str = buf;
					res = type;
				}
			}
			else if ((type == REG_DWORD) && (lng != 0))
			{
				long tmp;
				if (RegQueryValueEx (hkey, &name[0], 0, &type, (LPBYTE) &tmp, &len) == ERROR_SUCCESS)
				{
					*lng = tmp;
					res = type;
				}
			}
		}
		RegCloseKey (hkey);
	}
	return res;
}

bool regWrite (string let, HKEY root, string subkey, string name, bool writebin)
{
	return (_regWrite (root, subkey, name, let, 0, (writebin) ? REG_BINARY : REG_SZ));
}

bool regWrite (long let, HKEY root, string subkey, string name)
{
	return (_regWrite (root, subkey, name, "", let, REG_DWORD));
}

bool _regWrite (HKEY root, string subkey, string name, string str, long lng, long regtype)
{
	HKEY hkey;
	DWORD res;
	bool fail_success = false;

	// Open the Registry Key for Read, Write Access
	if ((res = RegOpenKeyEx(root, &subkey[0], 0, KEY_WRITE, &hkey)) == ERROR_SUCCESS)
	{
		if (regtype == REG_SZ)
		{
			fail_success = ((res = RegSetValueEx(hkey, &name[0], 0, REG_SZ, (BYTE*) &str[0], xlen(str))) == ERROR_SUCCESS);
		}
		else if (regtype == REG_EXPAND_SZ)
		{
			fail_success = ((res = RegSetValueEx(hkey, &name[0], 0, REG_EXPAND_SZ, (BYTE*) &str[0], xlen(str))) == ERROR_SUCCESS);
		}
		else if (regtype == REG_BINARY)
		{
			fail_success = ((res = RegSetValueEx(hkey, &name[0], 0, REG_BINARY, (BYTE*) &str[0], xlen(str))) == ERROR_SUCCESS);
		}
		else if (regtype == REG_DWORD)
		{
			fail_success = ((res = RegSetValueEx(hkey, &name[0], 0, REG_DWORD, (BYTE*) &lng, 4)) == ERROR_SUCCESS);
		}

		// Check if RegSetValueEx succeeded
		#ifdef _DEBUG
			if (!fail_success)
			{
				DebugOut (CSR("_regWrite::RegSetValueEx> ", FormatMsg(res)));
			}
		#endif

		// Close Regkey
		RegCloseKey (hkey);
	}
	else
	{
		#ifdef _DEBUG
			DebugOut (CSR("_regWrite::RegOpenKeyEx> ", FormatMsg(res)));
		#endif
	}

	// Return if the operation succeeded
	return fail_success;
}

bool regDeleteValue (HKEY root, string subkey, string name)
{
	HKEY hkey;
	bool fail_success = false;
	
	// Open Reg Key for Delete Access
	if (RegOpenKeyEx(root, &subkey[0], 0, KEY_WRITE, &hkey) == ERROR_SUCCESS)
	{
		fail_success = (RegDeleteValue(hkey, &name[0]) == ERROR_SUCCESS);
		
		// Close Regkey
		RegCloseKey (hkey);
	}

	// Return if the operation succeeded
	return fail_success;
}


//////////////////////////////////////
// Class: Local Registry Object		//
//////////////////////////////////////
const HKEY RegObj::STD_ROOT						= HKEY_CURRENT_USER;
const char RegObj::STD_KEY[]					= "Software\\C32_DebugApp";

RegObj::RegObj ()
{
	// Chain Init
	SetRootKey (STD_ROOT, STD_KEY);
}

RegObj::RegObj (HKEY new_root, string new_key)
{
	// Chain Init
	SetRootKey (new_root, new_key);
}

RegObj::~RegObj ()
{
	//
}

void RegObj::SetRoot (HKEY new_root)																// public
{
	// CS
	cs.Enter();
	root = new_root;
	cs.Leave();
}

void RegObj::SetKey (string new_key)																// public
{
	// CS
	cs.Enter();
	key = new_key;
	cs.Leave();
}

void RegObj::SetRootKey (HKEY new_root, string new_key)
{
	// (!) This function automatically ensures that the given key exists in the registry base

	// Enter Critical Section
	cs.Enter();

	// Modify RAM
	root = new_root;
	key = new_key;

	// Check if we inited something different than the standard values
	if ((root != STD_ROOT) || (key != STD_KEY))
	{
		regEnsureKeyExist (root, key);
	}

	// Leave Critical Section
	cs.Leave();
}

void RegObj::Init (string v_name, string v_default, bool v_writebin, HKEY v_root, string v_key)		// public
{
	long regtype, extype;
	
	// Detect required RegType
	regtype = (v_writebin) ? REG_BINARY : REG_SZ;

	// Enter Critical Section
	cs.Enter();

	// Substitute missing parameters
	if (xlen(v_key) == 0)	{ v_key = key; };
	if (v_root == 0)		{ v_root = root; };
	
	// Check if the Value exists and if it has the correct type
	extype = regValueExistType (v_root, v_key, v_name);
	if ((v_writebin && (extype != REG_BINARY)) || 
		(!v_writebin && (extype != REG_SZ) && (extype != REG_EXPAND_SZ))
			)
	{
		// Not matching or does not exist
		if (extype != REG_VALUENOTEXIST)
		{
			// Exists - but wrong type
			regDeleteValue (v_root, v_key, v_name);
		}

		// Rewrite the Value
		regWrite (v_default, v_root, v_key, v_name, v_writebin);
	}

	// Leave Critical Section
	cs.Leave();
}

void RegObj::Init (string v_name, long v_default, HKEY v_root, string v_key)						// public
{
	long extype;

	// Enter Critical Section
	cs.Enter();

	// Substitute missing parameters
	if (xlen(v_key) == 0)	{ v_key = key; };
	if (v_root == 0)		{ v_root = root; };
	
	// Check if the Value exists and if it has the correct type
	extype = regValueExistType (v_root, v_key, v_name);
	if (extype != REG_DWORD)
	{
		// Not matching or does not exist
		if (extype != REG_VALUENOTEXIST)
		{
			// Exists - but wrong type
			regDeleteValue (v_root, v_key, v_name);
		}

		// Rewrite the Value
		regWrite (v_default, v_root, v_key, v_name);
	}

	// Leave Critical Section
	cs.Leave();
}

long RegObj::Get (string* ret, string v_name, HKEY v_root, string v_key)							// public
{
	long read_res;

	// Enter Critical Section
	cs.Enter();

	// Substitute missing parameters
	if (xlen(v_key) == 0)				{ v_key = key; };
	if (v_root == 0)					{ v_root = root; };

	// Read the requested Value
	read_res = regRead(ret, v_root, v_key, v_name);

	// Leave Critical Section
	cs.Leave();

	// Return Type of Value
	return read_res;
}

long RegObj::Get (long* ret, string v_name, HKEY v_root, string v_key)								// public
{
	long read_res;

	// Enter Critical Section
	cs.Enter();

	// Substitute missing parameters
	if (xlen(v_key) == 0)				{ v_key = key; };
	if (v_root == 0)					{ v_root = root; };

	// Read the requested Value
	read_res = regRead(ret, v_root, v_key, v_name);

	// Leave Critical Section
	cs.Leave();

	// Return Type of Value
	return read_res;
}

bool RegObj::Set (string v_let, string v_name, HKEY v_root, string v_key, bool v_writebin)			// public
{
	bool fail_success;

	// Enter Critical Section
	cs.Enter();

	// Substitute missing parameters
	if (xlen(v_key) == 0)				{ v_key = key; };
	if (v_root == 0)					{ v_root = root; };

	// Write the specified Value
	fail_success = regWrite(v_let, v_root, v_key, v_name, v_writebin);

	// Leave Critical Section
	cs.Leave();

	// Return if write operation succeeded
	return fail_success;
}

bool RegObj::Set (long v_let, string v_name, HKEY v_root, string v_key)								// public
{
	bool fail_success;

	// Enter Critical Section
	cs.Enter();

	// Substitute missing parameters
	if (xlen(v_key) == 0)				{ v_key = key; };
	if (v_root == 0)					{ v_root = root; };

	// Write the specified Value
	fail_success = regWrite(v_let, v_root, v_key, v_name);

	// Leave Critical Section
	cs.Leave();

	// Return if write operation succeeded
	return fail_success;
}

bool RegObj::Delete (string v_name, HKEY v_root, string v_key)										// public
{
	bool fail_success;

	// Enter Critical Section
	cs.Enter();

	// Substitute missing parameters
	if (xlen(v_key) == 0)				{ v_key = key; };
	if (v_root == 0)					{ v_root = root; };

	// Delete the Value
	fail_success = regDeleteValue(v_root, v_key, v_name);

	// Leave Critical Section
	cs.Leave();

	// Return if the operation succeeded
	return fail_success;
}


//////////////////////////////////
// Extended User Functions		//
//////////////////////////////////
void RegFileExt (string ext, string desc, string context, string thecall, string theicon, 
				 bool standard, bool showalways, bool confirmafterdownload)
{
	string ref;
	regEnsureKeyExist (HKCR, CSR(".", ext));			// Main key exists?			[.wav]
	regRead (&ref, HKCR, CSR(".", ext), "");			// Std value ref			[wavfile]
	if (xlen(ref) == 0) { ref = CSR(ext, "file"); };	// if none, create reference
	regWrite (ref, HKCR, CSR(".", ext), "");			// Apply					[wavfile]
	
	regEnsureKeyExist (HKCR, ref);						// Referenced key exists?	[wavfile]
	regWrite (desc, HKCR, ref, "");						// Write description		[Wave Audio]
	
	// Always Show Extension?
	regWrite ("", HKCR, ref, "AlwaysShowExt");
	if (!showalways) { regDeleteValue (HKCR, ref, "AlwaysShowExt"); };
	
	// Open after Download?
	if (!confirmafterdownload)
	{
		regWrite (65536, HKCR, ref, "EditFlags");		// No
	}
	else
	{
		regWrite (0, HKCR, ref, "EditFlags");			// Yes
	}
	
	// Context Menu
	regEnsureKeyExist (HKCR, CSR(ref, "\\shell"));
	if (standard) { regWrite (context, HKCR, CSR(ref, "\\shell"), ""); };
	
	regEnsureKeyExist (HKCR, CSR(ref, "\\shell\\", context));
	regWrite (context, HKCR, CSR(ref, "\\shell\\", context), "");	// Write Item	[Open]
	
	regEnsureKeyExist (HKCR, CSR(ref, "\\shell\\", context, "\\command"));
	regWrite (thecall, HKCR, CSR(ref, "\\shell\\", context, "\\command"), "");
	
	regEnsureKeyExist (HKCR, CSR(ref, "\\DefaultIcon"));
	regWrite (theicon, HKCR, CSR(ref, "\\DefaultIcon"), "");
}


//////////////////////
// Class: RegDir	//
//////////////////////
RegDir::RegDir ()
{
}

RegDir::~RegDir ()
{
	Clear();
}

void RegDir::Clear ()																							// private
{
	// No CS, because: PRIVATE FUNCTION
	rdc.Redim (0);
}

void RegDir::Enum (HKEY root, string subkey)																	// public
{
	HKEY hkey;
	DWORD regtype, objlen;
	FILETIME ftime;
	string obj;
	long cnt = 0, u = 0;
	
	// Enter Critical Section
	cs.Enter();

	// Enum Registry Keys
	Clear();
	if (RegOpenKeyEx(root, &subkey[0], 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
	{
		while (true)
		{
			objlen = 256; obj.resize (objlen);
			if (RegEnumKeyEx(hkey, cnt, &obj[0], &objlen, 0, 0, 0, &ftime) == ERROR_SUCCESS)
			{
				u++; rdc.Redim (u);
				obj.resize (objlen);
				rdc[u].name = obj;
				rdc[u].type = REG_DIRECTORY;
			}
			else
			{
				break;
			}
			cnt++;
		}
		
		cnt = 0;
		while (true)
		{
			objlen = 256; obj.resize (objlen);
			if (RegEnumValue(hkey, cnt, &obj[0], &objlen, 0, &regtype, 0, 0) == ERROR_SUCCESS)
			{
				u++; rdc.Redim (u);
				obj.resize (objlen);
				rdc[u].name = obj;
				rdc[u].type = regtype;
			}
			else
			{
				break;
			}
			cnt++;
		}
		RegCloseKey (hkey);
	}

	// Leave Critical Section
	cs.Leave();
}

long RegDir::Ubound	()																						// public			
{
	long u;

	// CS
	cs.Enter();
	u = rdc.Ubound();
	cs.Leave();

	// Return Item Count in current Cache
	return u;
}

long RegDir::type (long idx)																				// public	
{
	long cur_items_type;
	
	// CS
	cs.Enter();
	cur_items_type = rdc[idx].type;
	cs.Leave();

	// Return current items type
	return cur_items_type;
}

string RegDir::operator[] (long idx)																		// public
{
	string cur_items_name;

	// CS
	cs.Enter();
	cur_items_name = rdc[idx].name;
	cs.Leave();

	// Return current items name
	return cur_items_name;
}



//////////////////////////////
// Class: UACRegEdit		//
//////////////////////////////
const char UACRegEdit::WINSYS_REG_EXE[]					= "REG.EXE";
const char UACRegEdit::TEMP_BATCH_FILE[]				= "Registry_ApplyUserRequestedChanges.cmd";

UACRegEdit::UACRegEdit ()
{
	// Init RAM
	queue.Clear();
}

UACRegEdit::~UACRegEdit ()
{
	// If not all registry changes have been already committed to the registry, do that now
	if (queue.Ubound() > 0)
	{
		CommitChangesToRegistry();
	}
}

void UACRegEdit::AddToQueue (string regexe_params)																// private
{
	long u;

	// Enter Critical Section
	cs.Enter();

	// Add to Queue
	u = queue.Ubound() + 1;
	queue.Redim(u);
	queue[u] = regexe_params;

	// Leave Critical Section
	cs.Leave();
}

string UACRegEdit::BuildRegKey (HKEY root, string key)															// private
{
	if (root == HKEY_CURRENT_USER)
	{
		string cur_user_sid;
		cur_user_sid = GetCurrentUserSID();

		if (xlen(cur_user_sid) != 0)
		{
			// If we succeeded in retrieving the current user's SID for registry access, we'll use it
			return CSR("\"HKU\\", cur_user_sid, "\\", key, "\"");
		}
		else
		{
			// No SID retrieved, so fall back to the unsecure layer of accessing HKEY_CURRENT_USER
			return CSR("\"HKCU\\", key, "\"");
		}
	}
	else if (root == HKEY_LOCAL_MACHINE)
	{
		return CSR("\"HKLM\\", key, "\"");
	}
	else if (root == HKEY_CLASSES_ROOT)
	{
		return CSR("\"HKCR\\", key, "\"");
	}
	else if (root == HKEY_USERS)
	{
		return CSR("\"HKU\\", key, "\"");
	}
	else
	{
		#ifdef _DEBUG
			DebugOut ("UACRegEdit::BuildRegKey->Error: Invalid Registry Root given to function.");
		#endif
		return "?";
	}
}

long UACRegEdit::CreateKey (HKEY root, string key)																// public
{
	if (GetCurrentUsersElevation() == TokenElevationTypeFull)
	{
		// Windows 9x/Me
		return regCreateKey(root, key);
	}
	else
	{
		// Windows XP/Vista/7

		// Filter Strings
		key.Replace("\r", ""); key.Replace("\n", ""); key.Replace ("\"", "\\\"");

		AddToQueue (CSR("ADD ", BuildRegKey(root, key)));
		return true;
	}
}

bool UACRegEdit::DeleteKey (HKEY root, string key)																// public
{
	if (GetCurrentUsersElevation() == TokenElevationTypeFull)
	{
		// Windows 9x/Me
		return regDeleteKey(root, key);
	}
	else
	{
		// Windows XP/Vista/7

		// Filter Strings
		key.Replace("\r", ""); key.Replace("\n", ""); key.Replace ("\"", "\\\"");

		AddToQueue (CSR("DELETE /f ", BuildRegKey(root, key)));
		return true;
	}
}

bool UACRegEdit::Write (string let, HKEY root, string key, string name, bool writebin)							// public
{
	if (GetCurrentUsersElevation() == TokenElevationTypeFull)
	{
		// Windows 9x/Me
		return regWrite(let, root, key, name, writebin);
	}
	else
	{
		// Windows XP/Vista/7
		if (!writebin)
		{
			// Filter Strings
			key.Replace("\r", ""); key.Replace("\n", ""); key.Replace ("\"", "\\\"");
			name.Replace("\r", ""); name.Replace("\n", ""); name.Replace ("\"", "\\\"");
			let.Replace("\r", ""); let.Replace("\n", ""); let.Replace ("\"", "\\\"");

			AddToQueue (CSR("ADD ", BuildRegKey(root, key), " /f /v \"", name, 
								CSR("\" /t ", (writebin) ? "REG_BINARY" : "REG_SZ", " /d \"", 
											(writebin) ? "Not supported" : let, "\"")));
			return true;
		}
		else
		{
			// Unsupported in UACRegEdit Module
			DebugOut ("UACRegEdit::Write->A call to the write queue with the parameter writebin has been issued and is unsupported.");
			return false;
		}
	}
}

bool UACRegEdit::Write (long let, HKEY root, string key, string name)											// public
{
	if (GetCurrentUsersElevation() == TokenElevationTypeFull)
	{
		// Windows 9x/Me
		return regWrite(let, root, key, name);
	}
	else
	{
		// Windows XP/Vista/7

		// Filter Strings
		key.Replace("\r", ""); key.Replace("\n", ""); key.Replace ("\"", "\\\"");
		name.Replace("\r", ""); name.Replace("\n", ""); name.Replace ("\"", "\\\"");

		AddToQueue (CSR("ADD ", BuildRegKey(root, key), " /f /v \"", name, CSR("\" /t REG_DWORD /d \"", CStr(let), "\"")));
		return true;
	}
}

bool UACRegEdit::DeleteValue (HKEY root, string key, string name)												// public
{	
	if (GetCurrentUsersElevation() == TokenElevationTypeFull)
	{
		// Windows 9x/Me
		return regDeleteValue(root, key, name);
	}
	else
	{
		// Windows XP/Vista/7

		// Filter Strings
		key.Replace("\r", ""); key.Replace("\n", ""); key.Replace ("\"", "\\\"");
		name.Replace("\r", ""); name.Replace("\n", ""); name.Replace ("\"", "\\\"");

		AddToQueue (CSR("DELETE ", BuildRegKey(root, key), " /f /v \"", name, "\""));
		return true;
	}
}

bool UACRegEdit::CommitChangesToRegistry()																		// public
{
	bool fail_success = false;
	string system_dir, temp_dir, regexe_fullfn, batchfile;
	HANDLE hfile;
	long u, i;

	// This function is only available when running Windows XP/Vista/7
	if (queue.Ubound() == 0)
	{
		// Simulate Success
		return true;
	}

	// Enter Critical Section
	cs.Enter();

	// Get System Path
	system_dir = GetSystemDir32();
	if (xlen(system_dir) != 0)
	{ 
		// Check if "REG.EXE" exists in this path
		regexe_fullfn = CSR(system_dir, "\\", WINSYS_REG_EXE);
		if (FileExists(regexe_fullfn))
		{
			// Get Temp Folder
			temp_dir = GetTempFolder();
			if (xlen(temp_dir) != 0)
			{
				// Build Temp Batch Filename
				batchfile = CSR(temp_dir, "\\", TEMP_BATCH_FILE);

				// Delete any possibly previosly existing file
				EnsureDeleteFile (batchfile);

				// Create a new batch file
				hfile = CreateFile(&batchfile[0], GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, 0);
				if (hfile != INVALID_HANDLE_VALUE)
				{
					WriteFileData(hfile, "@echo off\r\n");

					// Write Batch file in relation to "REG.EXE"
					u = queue.Ubound();
					for (i = 1; i <= u; i++)
					{
						WriteFileData (hfile, CSR(regexe_fullfn, " ", queue[i], "\r\n"));
					}

					// Write Autodeletion Part into batch file
					WriteFileData (hfile, CSR("del \"", batchfile, "\"\r\n"));

					// Close the file
					CloseHandle (hfile);

					// Clear the Queue
					queue.Clear();

					// Attempt to start the batch file as Administrator
					if (ShellExecuteRunAs(0, CSR(GetSystemDir32(), "\\CMD.EXE /c \"", batchfile, "\"")) > (HINSTANCE) 32)
					{
						DWORD curticks = GetTickCount();

						// Wait until file has been deleted, so it is sure that all changes have been applied
						while (FileExists(batchfile) && (GetTickCount() < (curticks + 5000)))
						{
							Sleep (50);
						}

						// Success
						fail_success = true;
					}
					else
					{
						// Error
						DebugOut ("UACRegEdit::CommitChangesToRegistry->Could not apply the requested changes to the windows configuration.");
					}
				}
			}
		}
	}

	// Leave Critical Section
	cs.Leave();

	// Return if the operation succeeded
	return fail_success;
}

