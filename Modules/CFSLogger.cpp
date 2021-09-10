// CFSLogger.cpp: Using the INI Standard for Logging
// Version: 2010-02-19
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
const long CFSLOG_MAXPERSEC				= 95;		// [95]


//////////////////////////
// Init / Terminate		//
//////////////////////////
CFSLog::CFSLog ()		{ maxsize = -1; };
CFSLog::~CFSLog ()		{ New(); };


//////////////////
// Interface	//
//////////////////
void CFSLog::New ()															// public								
{
	// Enter Critical Section
	cs.Enter();

	// Reset the Inifile
	log.New();

	// Leave Critical Section
	cs.Leave();
}

void CFSLog::Add (string text)												// public
{
	string datum = date(), zeit = time();
	string probe;
	long id = 0;

	// Enter Critical Section
	cs.Enter();
	
	// Create Section if necessary
	log.AddSection (datum);
	
	// Find a free identifier for the new log entry
	while (id <= CFSLOG_MAXPERSEC)
	{
		probe = FillStr(CStr(id), 2, '0');
		if (log.FindEntry(datum, CSR(zeit, "=", probe)) == 0)
		{
			log.WriteEntry (datum, CSR(zeit, "=", probe), CSR("> ", text));
			break;
		}
		else
		{
			id++;
		}
	}

	// Leave Critical Section
	cs.Leave();
}

void CFSLog::DeleteOldEntries ()											// public
{
	StructArray <string> secs;
	
	// (?) Maximum enforced
	if (maxsize == -1) { return; };

	// Enter Critical Section
	cs.Enter();
	
	// Loop until everything is cleaned up
	while (CalculateSize() > maxsize)
	{
		// Stop if only one section exists
		log.EnumSections (&secs);
		if (secs.Ubound() < 2) { break; };
		
		// Kill the first section (oldest log entry)
		log.MemDeleteSection (secs[1]);
	}

	// Leave Critical Section
	cs.Leave();
}


//////////////////
// File I/O		//
//////////////////
bool CFSLog::Open (string file)												// public
{
	bool res;

	// Enter Critical Section
	cs.Enter();

	// Clear everything first (function protects itself)
	New();
	
	// Load the log
	res = log.Open(file, false);

	// Leave Critical Section
	cs.Leave();

	return res;
}

bool CFSLog::Save (string file)												// public
{
	bool res;

	// Enter Critical Section
	cs.Enter();

	// Delete oldest entries over the limit
	DeleteOldEntries();
	
	// Save the log
	res = log.Save(file, false);

	// Leave Critical Section
	cs.Leave();

	return res;
}


//////////////////
// Settings		//
//////////////////
void CFSLog::SetMaxSize (long newmax)										// public
{
	// Check if newmax is valid
	if ((newmax < 10) && (newmax != -1)) { return; };
	
	// Apply New Logsize Maximum for saving
	maxsize = newmax;
}


//////////////////////////////
// Private Functions		//
//////////////////////////////
long CFSLog::CalculateSize ()												// private
{
	StructArray <string> secs;
	StructArray <string> ents;
	long size, us, ue;
	size = 0;

	// Enter Critical Section
	cs.Enter();
	
	// Enum Sections
	log.EnumSections (&secs);
	us = secs.Ubound();
	for (long s = 1; s <= us; s++)
	{
		size += xlen(secs[s]);
		
		// Enum Entries
		log.EnumEntries (secs[s], &ents);
		ue = ents.Ubound();
		for (long e = 1; e <= ue; e++)
		{
			size += xlen(ents[e]);
			size += xlen(log.GetEntry(secs[s], ents[e]));
		}
	}

	// Leave Critical Section
	cs.Leave();

	// Return filesize
	return size;
}


