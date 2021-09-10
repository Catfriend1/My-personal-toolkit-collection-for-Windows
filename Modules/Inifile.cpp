// Inifile.cpp: Providing Interface to INI-files
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
const long Inifile::INIFILE_CHUNKSIZE				= 8388608;			// [8388608 = 8 MB] [8192 = 8 KB]
const long Inifile::LOOKBEFORE_ENTRY_COUNT			= 50;				// [50]
const long Inifile::LOOKAHEAD_ENTRY_COUNT			= 50;				// [50]


//////////////////////////
// Init / Terminate		//
//////////////////////////
Inifile::Inifile ()					// private
{
	// Public Properties
	opt_file_locked = false;

	// Clear stored INI data
	ResetCache();					// ResetCache is public, no CS needed!
}

Inifile::~Inifile ()				// private
{
	// Clear stored INI data
	ResetCache();					// ResetCache is public, no CS needed!
}

void Inifile::ResetCache ()			// public
{
	// Enter Critical Section
	cs.Enter();

	// Clear Section Cache
	sec.Redim (0);

	// Clear Zero Section so that no Entries exist in this Section
	sec[0].entry.Clear();
	
	// Reset Cache IDX
	idxlastsec = -1;
	idxlastentry = -1;

	// Leave Critical Section
	cs.Leave();
}


//////////////////////////
// Section Manager		//
//////////////////////////
long Inifile::AddSection (string name)									// public
{
	// 2012-09-30/note: This function now returns the section index.
	long u;

	// Enter Critical Section
	cs.Enter();
	
	// Check if section already exists
	u = FindSection(name);
	if (u == 0)
	{
		// Add new section to Cache.
		// 2013-01-04/note: Slow code, Improvement impossible ?
		u = sec.Ubound() + 1;
		sec.Redim (u);
		sec[u].name = name;
		sec[u].entry.Clear();
	}

	// Leave Critical Section
	cs.Leave();

	// Return Section Index
	return u;
}

void Inifile::DeleteSection (string name)								// public
{
	long s;
	
	// Enter Critical Section
	cs.Enter();

	s = FindSection(name);
	if (s != 0)
	{
		// Modify Cache
		if (s == idxlastsec)		// Invalidates 'idxlastsec' if it equals the current section
		{
			idxlastsec = -1;
		}
		sec[s].name = "";			// Invalidates the Section to avoid memory re-organizing
		sec[s].entry.Clear();
	}

	// Leave Critical Section
	cs.Leave();
}

void Inifile::MemDeleteSection (string name)							// public
{
	// (!) This function physically deletes a section from Memory (!)
	// (!) Performance leaks may be possible (!)
	long s;
	
	// Enter Critical Section
	cs.Enter();

	s = FindSection(name);
	if (s != 0)
	{
		// Modify Cache
		idxlastsec = -1;
		sec.Remove (s);


	}

	// Leave Critical Section
	cs.Leave();
}

long Inifile::FindSection (string name)									// public
{
	long i, u;
	bool success = false;
	
	// Is the Section Name invalid?
	if (name.length() == 0) { return 0; };

	// Enter Critical Section
	cs.Enter();
	
	// Was it the last section that had been searched?
	if ((name == strlastsec) && (idxlastsec != -1))
	{
		// Leave the Critical Section before returning the result
		cs.Leave();
		return idxlastsec;
	}
	
	u = sec.Ubound();
	for (i = 1; i <= u; i++)
	{
		if (UCase(sec[i].name) == UCase(name))
		{
			// (!) Found it
			success = true;
			break;
		}
	}

	if (success)
	{
		idxlastsec = i;
		strlastsec = sec[i].name;

		// Leave the Critical Section before returning the result
		cs.Leave();
		return i;
	}
	else
	{
		// Last search failed, so it is invalid now
		idxlastsec = -1;
		strlastsec = "";

		// Leave the Critical Section before returning the result
		cs.Leave();
		return 0;
	}
}


//////////////////////
// Entry Manager	//
//////////////////////
void Inifile::WriteEntry (string section, string name, string value)		// public
{
	long s, e;
	IniEntry ie_data;
	
	// Enter Critical Section
	cs.Enter();

	s = FindSection(section);
	if (s != 0)
	{
		e = FindEntry(section, name);
		if (e == 0)
		{
			// Entry does not exist yet, create & return it => e
			e = sec[s].entry.Append (ie_data);
		}
		sec[s].entry[e].name = name;
		sec[s].entry[e].value = value;
	}

	// Leave Critical Section
	cs.Leave();
}

void Inifile::InitEntry (string section, string name, string stdvalue)					// public
{
	// Enter Critical Section
	cs.Enter();

	if (FindEntry(section, name) == 0)
	{
		WriteEntry (section, name, stdvalue);
	}

	// Leave Critical Section
	cs.Leave();
}

void Inifile::DeleteEntry (string section, string name)									// public
{
	long s;

	// Enter Critical Section
	cs.Enter();

	s = FindSection(section);
	if (s != 0)
	{
		long e = FindEntry(section, name);
		if (e != 0)
		{
			// Modify Cache
			sec[s].entry[e].name = "";	// Invalidates the Entry to avoid memory re-organizing
			sec[s].entry[e].value = "";
		}
	}

	// Leave Critical Section
	cs.Leave();
}

void Inifile::MemDeleteEntry (string section, string name)								// public
{
	// (!) This function physically deletes an entry from Memory (!)
	// (!) Performance leaks may be possible (!)
	long s, e;

	// Enter Critical Section
	cs.Enter();

	s = FindSection(section);
	if (s != 0)
	{
		e = FindEntry(section, name);
		if (e != 0)
		{
			// Modify Cache
			sec[s].entry.Remove (e);
		}
	}

	// Leave Critical Section
	cs.Leave();
}

long Inifile::FindEntry (string section, string name)									// public
{
	long s, i, u;
	long guessidx_start;
	long guessidx_end;
	bool success = false;
	
	// Enter Critical Section
	cs.Enter();

	// Begin search
	s = FindSection(section);
	if (s != 0)
	{
		// Section found.
		u = sec[s].entry.Ubound();

		// Try to find the entry near the last one found in cache.
		// Use "idxlastentry" as an indicator where to start and end search.
		if (idxlastentry != -1)
		{
			guessidx_start = idxlastentry - LOOKBEFORE_ENTRY_COUNT;
			guessidx_end = idxlastentry + LOOKAHEAD_ENTRY_COUNT;

			if (guessidx_start < 0)		{ guessidx_start = 0; };
			if (guessidx_end > u)		{ guessidx_end = u; };

			for (i = guessidx_start; i <= guessidx_end; i++)
			{
				if (UCase(sec[s].entry[i].name) == UCase(name))
				{
					// (!) Found it
					success = true;
					break;
				}
			}
		}

		// If we haven't had success yet, we'll continue searching for the given entry name in cache.
		if (!success)
		{
			// Read through all entries in cache.
			for (i = 1; i <= u; i++)
			{
				if (UCase(sec[s].entry[i].name) == UCase(name))
				{
					// (!) Found it
					success = true;
					break;
				}
			}
		}
	}

	// Leave Critical Section
	cs.Leave();

	// Return Result
	if (success)
	{
		idxlastentry = i;
		return i;
	}
	else
	{
		return 0;
	}
}

string Inifile::GetEntry (string section, string name)									// public
{
	long s, e;
	string ret = "";

	// Enter Critical Section
	cs.Enter();

	s = FindSection(section);
	if (s != 0)
	{
		e = FindEntry(section, name);
		if (e != 0) 
		{ 
			// Entry found, return its value.
			ret = sec[s].entry[e].value; 
		}
	}

	// Leave Critical Section
	cs.Leave();

	// Return Result
	return ret;
}


//////////////////////
// Enum Contents	//
//////////////////////	
void Inifile::EnumSections (StructArray <string> *es)									// public
{
	long uall;
	long ubound_es;

	// Enter Critical Section
	cs.Enter();

	// Clear Output Array
	es->Clear();
	ubound_es = es->Ubound();
	
	// Read Cache
	uall = sec.Ubound();
	for (long i = 1; i <= uall; i++)
	{
		if (sec[i].name.length() != 0)		
		{
			// Valid Section found
			es->Redim (++ubound_es);
			(*es)[ubound_es] = sec[i].name;
		}
	}

	// Leave Critical Section
	cs.Leave();
}

void Inifile::EnumEntries (string section, StructArray <string> *ee)					// public
{
	long s;
	long uall;
	long i;
	long ubound_ee;

	// Enter Critical Section
	cs.Enter();

	// Clear Output Array
	ee->Clear();
	ubound_ee = ee->Ubound();

	// Find the Section to Enum
	s = FindSection(section);
	if (s != 0)
	{
		// Read Cache, Output to Output Array
		uall = sec[s].entry.Ubound();
		for (i = 1; i <= uall; i++)
		{
			if (sec[s].entry[i].name.length() != 0)
			{
				ee->Redim (++ubound_ee);
				(*ee)[ubound_ee] = sec[s].entry[i].name;
			}
		}
	}

	// Leave Critical Section
	cs.Leave();
}


//////////////////////////////////////
// Functions: File I/O System		//
//////////////////////////////////////
void Inifile::New ()												// public
{
	ResetCache();
}


bool Inifile::Open (string strFullFN, bool bDataIsCrypted)			// public
{
	HANDLE hFile;
	DWORD dwBytesRead;
	string strData;
	string strFileContents;
	bool fail_success = false;
	
	// Enter Critical Section
	cs.Enter();
	
	// Reset the Cache
	ResetCache();
	
	// Try to open the file
	hFile = CreateFile (&strFullFN[0], GENERIC_READ, 
							FILE_SHARE_READ, NULL, 
							OPEN_EXISTING, 
							FILE_ATTRIBUTE_ARCHIVE, 0);
	
	if (hFile == INVALID_HANDLE_VALUE)
	{
		#ifdef _DEBUG
			DebugOut (CSR("Inifile Error: Could not open ", Chr(34), strFullFN, Chr(34)));
		#endif

		// Leave Critical Section before return
		cs.Leave();
		return false;
	}
	
	// Read file contents to memory.
	strData.resize (INIFILE_CHUNKSIZE);
	strFileContents.resize(0);
	do
	{
		if (ReadFile(hFile, &strData[0], xlen(strData), &dwBytesRead, NULL))
		{
			// (!) Read succeeded.
			strFileContents.Append(strData.mid(1, dwBytesRead));
		}
	} while (dwBytesRead == INIFILE_CHUNKSIZE);
	
	// Close the file
	CloseHandle (hFile);

	// Load read file contents to INI Cache.
	if (LoadFromString(strFileContents, bDataIsCrypted))
	{
		// Return Success.
		fail_success = true;
	}

	// Leave Critical Section
	cs.Leave();

	// Success
	return fail_success;
}


bool Inifile::Save (string file, bool bEncryptData)							// public
{
	DWORD file_attr, ticks_starttime; 
	HANDLE hfile;
	string file_out_buffer;

	// Enter Critical Section
	cs.Enter();
	
	// Get Current Attributes of the file on disk (if exists)
	file_attr = GetFileAttributes (&file[0]);
	if (file_attr == INVALID_FILE_ATTRIBUTES) { file_attr = FILE_ATTRIBUTE_ARCHIVE;	};
	
	// Open file for saving.
	ticks_starttime = GetTickCount();										// in ms
	while ((hfile = CreateFile (&file[0], GENERIC_WRITE, 
							0, NULL, 
							CREATE_ALWAYS, 
							file_attr, 0)) == INVALID_HANDLE_VALUE)
	{
		// Check how long this critical operation did not succeed.
		if ((GetTickCount() - ticks_starttime) > 1000)
		{
			OutputDebugString ("Inifile::Save->Could not resolve a file writing problem within 1 sec");
			#ifdef _DEBUG
				DebugOut (CSR("Inifile::Save->Could not save """, file, """ -> ", FormatMsg(GetLastError())));
			#endif

			// Leave Critical Section before return.
			cs.Leave();
			return false;
		}
	}

	// Collect all contents.
	SaveToString (&file_out_buffer, bEncryptData);

	// Start saving to disk.
	if (xlen(file_out_buffer) > 0)
	{
		WriteFileData (hfile, file_out_buffer);
	}

	// Close the file
	CloseHandle (hfile);

	// Leave Critical Section
	cs.Leave();

	// Success
	return true;
}


//////////////////////////////////////
// Functions: RAM I/O System		//
//////////////////////////////////////
// public
bool Inifile::LoadFromString (string strFileContent, bool bDataIsCrypted)
{
	// Variables.
	int prev_thread_priority;		// temporary variable
	long line_xlen;					// temporary variable
	long spos;
	long lpos;
	long idxcursec_entry_u = 0;
	long idxcursec = -1;
	string name;
	string value;
	string line;
	IniEntry ie_data;

	// Enter CS.
	cs.Enter();

	// Temporarily change thread priority.
	prev_thread_priority = GetThreadPriority(GetCurrentThread());
	SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

	// Clear Memory first.
	ResetCache();

	// Search for "End of Line" (0xA).
	lpos = strFileContent.InStrFirst(10, 1);
	while (lpos != 0)
	{
		// Get one line.
		line = strFileContent.mid(1, lpos - 1);
		line_xlen = (long) line.length();

		// Find and cut off <return> at the end of the line.
		if (line_xlen > 0)			
		{
			// Line is not empty.
			if (line[line_xlen-1] == 13)
			{ 
				line.resize (line_xlen - 1); 
				line_xlen--;
			}

			// Work through the line.
			if ((line[0] == '[') && (line[line_xlen - 1] == ']'))
			{
				// (!) SECTION FOUND
				// Try to add the section if it does not exist.
				// public, no CS needed!
				idxcursec = AddSection((bDataIsCrypted) ? hex2str(line.mid(2, line_xlen - 2)) : line.mid(2, line_xlen - 2));
				
				// 2012-09-30: Optimized Code.
				idxcursec_entry_u = 0;
			}
			else if (idxcursec != -1)	// Could be an entry, but we need a current section first
			{
				// (!) ENTRY FOUND.
				spos = line.InStrFirst('=', 1);
				if (spos != 0)
				{
					// Slow code: WriteEntry (strcursec, name, value);
					// Use idxcursec for faster access.
					
					// 2012-09-30: Optimized Code.
					// Slow Code: idxcursec_entry_u = sec[idxcursec].entry.Ubound() + 1;
					
					// 2012-09-30: Now we'll count ourself how much entries we got in the current entry array.
					//idxcursec_entry_u++;
					//sec[idxcursec].entry.Redim (idxcursec_entry_u);
					//sec[idxcursec].entry[idxcursec_entry_u].name = ((bDataIsCrypted)	? hex2str(line.mid(1, spos - 1))	: line.mid(1, spos - 1));
					//sec[idxcursec].entry[idxcursec_entry_u].value = ((bDataIsCrypted)	? hex2str(line.mid(spos + 1))		: line.mid(spos + 1));

					// 2013-01-04: Fill IniEntry structure and append it to the entry-array (fastest code).
					ie_data.name = ((bDataIsCrypted)	? hex2str(line.mid(1, spos - 1))	: line.mid(1, spos - 1));
					ie_data.value = ((bDataIsCrypted)	? hex2str(line.mid(spos + 1))		: line.mid(spos + 1));
					sec[idxcursec].entry.Append (ie_data);
				}
			}
		}
		
		// Remove it from the buffer.
		// Slow code: strFileContent = Right(strFileContent, xlen(strFileContent) - lpos);
		strFileContent.cutoffLeftChars(lpos);
		
		// Search for "End of Line" (0xA).
		lpos = strFileContent.InStrFirst(10, 1);
	}

	// Restore Thread Priority.
	SetThreadPriority (GetCurrentThread(), prev_thread_priority);

	// Leave CS.
	cs.Leave();

	// Return Success.
	return true;
}

// public
bool Inifile::SaveToString (string* file_out, bool bEncryptData)
{
	// Variables.
	int prev_thread_priority;		// temporary variable
	long a, b, uall, uent;

	// Enter Critical Section
	cs.Enter();

	// Temporarily change thread priority.
	prev_thread_priority = GetThreadPriority(GetCurrentThread());
	SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

	// Clear Output String first
	*file_out = "";

	// Start Saving
	uall = sec.Ubound();
	for (a = 1; a <= uall; a++)
	{
		// Section Valid ?
		if (sec[a].name.length() != 0)		
		{
			file_out->Append(CSR("[", 
							(!bEncryptData) ? sec[a].name : str2hex(sec[a].name), 
							"]\r\n"));

			// Go through the entries
			uent = sec[a].entry.Ubound();
			for (b = 1; b <= uent; b++)
			{
				// Entry Valid ?
				if (xlen(sec[a].entry[b].name) != 0)	
				{
					file_out->Append(CSR(
									(!bEncryptData) ? sec[a].entry[b].name : str2hex(sec[a].entry[b].name), 
									"=", 
									(!bEncryptData) ? sec[a].entry[b].value : str2hex(sec[a].entry[b].value), 
									"\r\n"));
				}
			}
			
			// Add free line
			file_out->Append ("\r\n");
		}
	}

	// Restore Thread Priority.
	SetThreadPriority (GetCurrentThread(), prev_thread_priority);

	// Leave Critical Section
	cs.Leave();

	// Return Success
	return true;
}

