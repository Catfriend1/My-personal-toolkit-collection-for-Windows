// Syslog_Server.cpp
//
// Builds a TCP Server on the local machine for the NT Service Part of the application
// The Server controls the INI Logfile in the \System32 directory and receives data from the interactive user clients
//
// Thread-Safe: NO
//


//////////////////
// Includes		//
//////////////////
#include "stdafx.h"

#include "../Modules/_modules.h"

#include "Syslog_Shared.h"
#include "Syslog_vcomm.h"
#include "Syslog_Server.h"



//////////////////////
// Class: Constants	//
//////////////////////
const bool Syslog_Server::DEBUG_MODE			= false;
const long Syslog_Server::SYSLOG_TCP_PORT		= 17888;
const char Syslog_Server::SYSLOG_CAPTION[]		= "Syslog";

const long Syslog_Server::SLINI_ATTRIB_LOCK		= FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
const long Syslog_Server::SLINI_ATTRIB_UNLOCK	= FILE_ATTRIBUTE_ARCHIVE;

const long Syslog_Server::TIMER_DELAY			= 1000;
const char Syslog_Server::MSG_WINSTART[]		= "[¤] Windows startup successful.";
const char Syslog_Server::MSG_WINSHUT[]			= "[×] Windows is shutting down...";


//
// Use this with caution because the process image path will not be checked before excluding the corresponding process from syslog.
const char Syslog_Server::LOG_PROC_WHITELIST_BY_PROCNAME_ONLY[]	= "*cmd.exe*dllhost.exe*fc.exe*find.exe*findstr.exe*netsh.exe*nslookup.exe*sed.exe*sort.exe*sppsvc.exe*";


//////////////
// Types	//
//////////////
struct Syslog_Server::NetClient
{
	SOCKET id;
};



//////////////////////////////////////
// Class: Constructor, Destructor	//
//////////////////////////////////////
Syslog_Server::Syslog_Server ()
{
	// Init RAM.
	log_general = false;
	log_capchanges = false;

	netclients.Clear();

	slfile = INVALID_HANDLE_VALUE;

	signal_server_shutdown = false;

	once_net_auth_failed = false;
	once_invalid_packet_received = false;

	// 
	// Build whitelist for processes running from the windows directory or one of its sub-directories.
	LOG_PROC_WHITELIST_WIN10.resize(0);
	LOG_PROC_WHITELIST_WIN10 += "*Memory Compression*";
	// 
	//		"%SystemRoot%\\%System%" = "%SystemRoot%\\System32", "%SystemRoot%\\SysWow64";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\%System%\\cmd.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\%System%\\DllHost.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\%System%\\fc.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\%System%\\find.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\%System%\\findstr.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\%System%\\fontdrvhost.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\%System%\\net.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\%System%\\net1.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\%System%\\netsh.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\%System%\\nslookup.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\%System%\\SearchFilterHost.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\%System%\\SearchIndexer.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\%System%\\SearchProtocolHost.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\%System%\\sort.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\%System%\\timeout.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\%System%\\UserInit.exe*";
	// 
	//		"%SystemRoot%\\System32".
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\System32\\ApplicationFrameHost.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\System32\\conhost.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\System32\\csrss.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\System32\\dwm.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\System32\\LogonUI.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\System32\\RuntimeBroker.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\System32\\sihost.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\System32\\smartscreen.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\System32\\TaskHostW.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\System32\\winlogon.exe*";
	// 
	//		"%SystemRoot%".
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\FSCapture.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\sed.exe*";
	//
	//		AppX - System Apps
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\SystemApps\\Microsoft.Windows.Cortana_cw5n1h2txyewy\\SearchUI.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\SystemApps\\ShellExperienceHost_cw5n1h2txyewy\\ShellExperienceHost.exe*";
	//
	//		AppX - User Apps
	// 
	//		ATI Graphics Driver
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\System32\\atieclxx.exe*";
	//
	//		Intel Graphics Driver
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\System32\\igfxEM.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%SystemRoot%\\System32\\igfxHK.exe*";
	//
	//		Realtek HD Audio Driver
	LOG_PROC_WHITELIST_WIN10 += "*%ProgramFiles%\\Realtek\\Audio\\HDA\\RAVCpl64.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%ProgramFiles%\\Realtek\\Audio\\HDA\\RAVBg64.exe*";
	//
	//		Sophos Anti-Virus
	LOG_PROC_WHITELIST_WIN10 += "*%ProgramFiles(x86)%\\Sophos\\AutoUpdate\\ALMon.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%ProgramFiles(x86)%\\Sophos\\AutoUpdate\\ALUpdate.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%ProgramFiles(x86)%\\Sophos\\AutoUpdate\\ALsvc.exe*";
	LOG_PROC_WHITELIST_WIN10 += "*%ProgramFiles(x86)%\\Sophos\\AutoUpdate\\SophosUpdate.exe*";
	//
	// Expand all variables in the filters given above.
	string strEnvSystemRoot = GetWinDir();
	string strEnvProgramFilesX86 = GetEnvVariable("ProgramFiles(x86)");
	//
	//		Compatibility Issue - This will return "Program Files (x86)" path on 64-bit os because we are running under as WOW 32-bit process.
	string strEnvProgramFiles = GetEnvVariable("ProgramFiles");
	if (is_64_bit_os)														// declared globally by "Process.cpp".
	{
		// 64-Bit OS Fix.
		strEnvProgramFiles.Replace(" (x86)", "");
	}
	else
	{
		// 32-Bit OS Fix.
		strEnvProgramFilesX86 = strEnvProgramFiles;
	}
	//
	// Replace static Environment Variables.
	LOG_PROC_WHITELIST_WIN10.Replace ("%SystemRoot%", strEnvSystemRoot);
	LOG_PROC_WHITELIST_WIN10.Replace ("%ProgramFiles(x86)%", strEnvProgramFilesX86);
	LOG_PROC_WHITELIST_WIN10.Replace ("%ProgramFiles%", strEnvProgramFiles);
	//
	// Expand filter for every Dynamic Environment Variable content, e.g. "%System%".
	if (is_64_bit_os)
	{
		// 64-Bit OS has to directories that the filter should match accordingly: "\\Windows\\System32", "\\Windows\SysWOW64".
		string tmp_expand_system32 = LOG_PROC_WHITELIST_WIN10;
		string tmp_expand_syswow64 = LOG_PROC_WHITELIST_WIN10;

		tmp_expand_system32.Replace("%System%", "System32");
		tmp_expand_syswow64.Replace("%System%", "SysWOW64");

		LOG_PROC_WHITELIST_WIN10 = CSR(tmp_expand_system32, tmp_expand_syswow64);
	}
	else
	{
		// 32-Bit OS "\\Windows\\System32".
		LOG_PROC_WHITELIST_WIN10.Replace("%System%", "System32");
	}
}


Syslog_Server::~Syslog_Server ()
{
}


//////////////////////
// Class: Functions	//
//////////////////////
bool Syslog_Server::Init (long runmode)
{
	// Build log filename.
#ifdef _DEBUG
	slini = "X:\\Syslog.dat";
#else
	slini = CSR(GetSystemDir32(), "\\Syslog.dat");
#endif

	// Log file access
	if (!FileExists(slini))
	{
		HANDLE crf = CreateFile(&slini[0], GENERIC_READ | GENERIC_WRITE, (DEBUG_MODE ? FILE_SHARE_READ : 0), 
								NULL, CREATE_NEW, SLINI_ATTRIB_LOCK, 0);
		if (crf != INVALID_HANDLE_VALUE)
		{
			CloseHandle (crf);
		}
		else
		{
			// (!) WARNING: Unable to create INI-file
			MessageBox (0, &CSR("(!) Unable to create logfile\n\n", 
								"--> WARNING: System Logger is currently disabled !")[0],
								"System Logger - Warning", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
	}
	
	// Cache, Lock and Initialize "Syslog.dat"
	log.Open (slini, false);
	if (!LockINI()) 
	{ 
		// FAILED.
		return false; 
	}

	// Init RAM
	ep2 = new EnumProcs;
	ep2->Enum();

	//
	// Cache Last Session Date/Time
	//
	StructArray <string> secs;
	StructArray <string> ents;
	long su, eu;
	//
	log.EnumSections (&secs);
	su = secs.Ubound();
	for (long s = su; s >= 1; s--)
	{
		log.EnumEntries (secs[s], &ents);
		eu = ents.Ubound();
		for (long e = eu; e >= 1; e--)
		{
			if (log.GetEntry(secs[s], ents[e]).mid(1, 5) == "> [¤]")
			{
				lastsess_date = secs[s];
				lastsess_time = ents[e];
				break;
			}
		}

		if ((xlen(lastsess_date) != 0) || (xlen(lastsess_time) != 0)) 
		{ 
			// Found last session date and time.
			break; 
		}
	}
	//
	// Cleanup RAM.
	//
	ents.Clear();
	secs.Clear();

	// Log System Startup
	WriteLog (MSG_WINSTART, true, true);

	// Get Runmode
	if ((runmode & SYSLOG_MODE_LOG_ENABLE) || (runmode & SYSLOG_MODE_LOG_ALL))
	{
		Switch_LogGeneral (true);

		if (runmode & SYSLOG_MODE_LOG_ALL)
		{
			log_capchanges = true;
		}
	}
	
	// (!) Success
	return true;
}

bool Syslog_Server::Main ()
{
	MSG msg;

	// Start the Listening TCP Server
	if (!srv.Create())
	{
		#ifdef _DEBUG
			DebugOut (CSR("Syslog Server Failure: Cannot create socket, Error #", CStr(WSAGetLastError())));
		#endif
		return false;
	}
	if (!srv.Bind("127.0.0.1", SYSLOG_TCP_PORT))		// Only bind to local interface 127.0.0.1
	{
		#ifdef _DEBUG
			DebugOut ("Syslog Server Failure: Cannot bind socket");
		#endif
		return false;
	}
	if (!srv.Listen())
	{
		#ifdef _DEBUG
			DebugOut ("Syslog Server Failure: Listen Error");
		#endif
		return false;
	}

	// Set Timer
	SetTimer (0, 0, TIMER_DELAY, NULL);

	// Message Loop
	SOCKET newsocket;
	while (true)
	{
		// Check if we got a new connection request
		if (srv.readable())
		{
			// (!) A connection request is currently pending
			if (srv.Accept(&newsocket))
			{
				long u;
				
				// Add the new client
				cs.Enter();
				u = netclients.Ubound() + 1;
				netclients.Redim (u);
				netclients[u].id = newsocket;
				cs.Leave();
				
				// Start a new Server Worker
				SERVER_VM.NewThread (&_ext_Syslog_Server_Server_Worker, (void*) this);
				Sleep (500);
			}
		}
		
		// Check if we got a shutdown signal
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_TIMER)
			{
				Timer_Event();
			}
			else if (msg.message == WM_QUIT)
			{
				break;
			}
		}
		
		// Set the Server Thread Idle for a time
		Sleep (50);
	}

	// Signal Server Shutdown
	signal_server_shutdown = true;

	// Remove Timer
	KillTimer (0, 0);

	// Stop Logging First if necessary
	Switch_LogGeneral (false);

	// Shutdown the Listen Server
	srv.Close();

	// Shutdown all client connections
	cs.Enter();
	for (long i = 1; i <= netclients.Ubound(); i++)
	{
		csocket pseudo (netclients[i].id);
		pseudo.Close();
	}
	cs.Leave();

	// Wait for all Server Worker's to shutdown
	SERVER_VM.AwaitShutdown();

	// Indicate Shutdown
	WriteLog (MSG_WINSHUT, true, true);

	// Unlock Logfile
	UnlockINI();			// This will not reset the file Atttributes

	// Remove Cache Elements
	delete ep2;

	// Success
	return true;
}

long Syslog_Server::_ext_Syslog_Server_Server_Worker (Syslog_Server* ptr_Syslog_Server_Class)
{
	return ptr_Syslog_Server_Class->Server_Worker();
}

long Syslog_Server::Server_Worker()
{
	csocket client;
	string data;
	string local_username;
	unsigned char id;
	bool auth = false;
	bool closed = false;
	bool bClientShutdownByLogoff = false;				// Used to indicate if someone killed the client process.

	// (!) We must obtain the newsocket first before it becomes INVALID, 500 msecs time for that!
	client.Take (netclients[netclients.Ubound()].id);
	
	// Receive Loop
	while (!signal_server_shutdown)
	{
		if (client.EnterReceiveLoop(&data, &closed))
		{
			// (!) Client closed the connection
			if (closed) { break; };
			
			// Get ID
			if (xlen(data) >= 1) { id = data[0]; } else { id = 0; };
			
			// ------------------------------   ACT   -------------------------------------
			if (id == SLG_VCOMM_AUTH)
			{
				if (Right(data, xlen(data) - 1) == SLG_AUTH_SIGNATURE)
				{
					// (!) Authenticated
					auth = true;

					// Spawn current settings
					if (!client.mod_send(CSR(Chr(SLG_VCOMM_SPAWN_SETTINGS), Chr(log_general), Chr(log_capchanges)))) { break; };
				}
				else
				{
					// (!) Wrong Authentication
					if (!once_net_auth_failed)
					{
						// Log this event only once per overall service instance.
						once_net_auth_failed = true;
						WriteLog ("[!] NETAUTH ERROR", true);
					}

					// Abort connection.
					closed = true;
					break;
				}
			}
			else if ((id == SLG_VCOMM_NOTIFY_LOGON) && auth)
			{
				string out;

				// Log the new user and extend our log entries with his name
				local_username = Right(data, xlen(data) - 1);
				WriteLog (CSR(Chr(34), local_username, Chr(34), " logged on."), true);

				// Display Welcome Message
				out = CSR("Last Session:|", lastsess_date, ", ", lastsess_time);
				if (!client.mod_send(CSR(Chr(SLG_VCOMM_SHOW_USERINFO), Chr(1), out))) { break; };
			}
			else if ((id == SLG_VCOMM_NOTIFY_LOGOFF) && auth)
			{
				// Assume that the "local_username" is already cached in the local functions' memory
				WriteLog (CSR(Chr(34), local_username, Chr(34), " logged off."), true);
				bClientShutdownByLogoff = true;
			}
			else if ((id == SLG_VCOMM_LOGTHIS) && auth)
			{
				string out = (xlen(local_username) > 0) ? CSR(" <", local_username, ">") : "";
				WriteLog (CSR(out, Right(data, xlen(data) - 1)), false);
			}
			else if ((id == SLG_VCOMM_SET_LOGGING) && auth)
			{
				// Apply Logging Policy - (!) INSECURITY (!) -
				if (xlen(data) > 1)
				{
					// Act on previous state change
					if ((data[1] == 1) && !log_general)
					{
						// Logging was off, should now be turned on
						Switch_LogGeneral (true);		// adjusts "log_general"
					}
					else if ((data[1] == 0) && log_general)
					{
						// Logging was on, should now be turned off
						Switch_LogGeneral (false);		// adjusts "log_general"
					}

					// Spawn Settings to all clients
					cs.Enter();
					for (long i = 1; i <= netclients.Ubound(); i++)
					{
						csocket pseudo;
						pseudo.Take (netclients[i].id);
						if (!pseudo.mod_send(CSR(Chr(SLG_VCOMM_SPAWN_SETTINGS), Chr(log_general), Chr(log_capchanges)))) { };		// Ignore errors on foreign connections
					}
					cs.Leave();

					// User Notification
					if (!client.mod_send(CSR(Chr(SLG_VCOMM_SHOW_USERINFO), Chr(0), SYSLOG_CAPTION, "|OK"))) { break; };
				}
			}
			else if ((id == SLG_VCOMM_SET_LOGCAPCHANGES) && auth)
			{
				// Apply Logging Policy - (!) INSECURITY (!) -
				if (xlen(data) > 1)
				{
					// Update RAM
					log_capchanges = (data[1] == 1);

					// Spawn Settings to all clients
					cs.Enter();
					for (long i = 1; i <= netclients.Ubound(); i++)
					{
						csocket pseudo (netclients[i].id);
						if (!pseudo.mod_send(CSR(Chr(SLG_VCOMM_SPAWN_SETTINGS), Chr(log_general), Chr(log_capchanges)))) 
						{
							// FAILED.
							// Ignore errors on foreign connections.
						}	
					}
					cs.Leave();

					// User Notification
					if (!client.mod_send(CSR(Chr(SLG_VCOMM_SHOW_USERINFO), Chr(0), SYSLOG_CAPTION, "|OK"))) { break; };
				}
			}
			else if ((id == SLG_VCOMM_ACQUIRE_LOGDATA) && auth)
			{
				string tmpfile;
				if (log.SaveToString(&tmpfile))
				{
					if (!client.mod_send(CSR(Chr(SLG_VCOMM_RECV_LOGDATA), tmpfile))) { break; };
				}
			}
			else
			{
				// Unknown or Invalid Packet received.
				if (!once_invalid_packet_received)
				{
					// Log this event only once per overall service instance.
					once_invalid_packet_received = true;
					WriteLog ("[!] INVALID NETDATA", true);
				}

				// Abort connection.
				closed = true;
				break;
			}
		}
	}
	
	// Delete Socket from Network Client Cache.
	// Note: client.Close() must be issued AFTERWARDS or client.Get() will always return zero.
	cs.Enter();
	for (long i = 1; i <= netclients.Ubound(); i++)
	{
		if (netclients[i].id == (SOCKET) client.Get())
		{
			netclients.Remove (i);
			break;
		}
	}
	cs.Leave();

	// Close Client Connection.
	client.Close();

	// Log if the client process was killed or hit an error.
	if (!bClientShutdownByLogoff)
	{
		WriteLog ("[!] Client process unexpectedly exited.", true);
	}
	
	// Remove Thread
	SERVER_VM.RemoveThread ();
	return 3;
}

void Syslog_Server::Timer_Event ()
{
	if (log_general)
	{
		long a, b, u_a, u_b;
		long i;
		bool is_newproc, is_procdown;
		StringArray sa;
		
		// -2- exists, -1- does not exist;
		ep1 = new EnumProcs;
		ep1->Enum();
		
		// Compare Processes
		u_a = ep1->Ubound();
		u_b = ep2->Ubound();
		for (a = 1; a <= u_a; a++)
		{
			is_newproc = true;
			for (b = 1; b <= u_b; b++)
			{
				if (ep1->procid(a) == ep2->procid(b))
				{
					is_newproc = false; break;
				}
			}

			// Was the current a new process?
			if (is_newproc && !SnapShot_ProcessFiltered(ep1->exepath(a)))
			{
				// (!) A New Process has been started
				sa.Append (CSR(" [-->] ", ep1->exepath(a)));
			}
		}
		
		for (b = 1; b <= u_b; b++)
		{
			is_procdown = true;
			for (a = 1; a <= u_a; a++)
			{
				if (ep1->procid(a) == ep2->procid(b))
				{
					is_procdown = false; break;
				}
			}
			if (is_procdown && !SnapShot_ProcessFiltered(ep2->exepath(b)))
			{
				// (!) A Process has been terminated
				sa.Append (CSR(" [<--] ", ep2->exepath(b)));
			}
		}
		
		// Delete -2-, Move -1- to -2-;
		delete ep2;
		ep2 = ep1;

		// Send messages to logfile and flush logfile after sending the last message.
		if (sa.Ubound() > 1)
		{
			for (i = 1; i < sa.Ubound(); i++)
			{
				WriteLog (sa[i], false);
			}
			WriteLog (sa[i], true);
		}
		else if (sa.Ubound() == 1)
		{
			WriteLog (sa[1], true);
		}
	}
}

bool Syslog_Server::SnapShot_ProcessFiltered(string strProcFullFN)
{
	// 
	// Return Values:
	//
	//		false			:= PROCESS_WILL_BE_LOGGED					:= unfiltered
	//		true			:= PROCESS_WILL_BE_EXCLUDED_FROM_LOG		:= filtered
	// 

	// We only attempt filtering for Windows 7 or higher.

	// EXCLUDE MYSELF: Is the found process a Syslog Process?
	if (UCase(strProcFullFN) == UCase(GetAppExe()))
	{
		// Filter Syslog Process.
		return true;
	}


	//
	// Exclude by process name only (unsafe way).
	//		Note - "strProcFullFN" could be FullFN or EXEName only.
	//				EXEName only occurs if the process cannot be opened in "EnumProcs" due to insufficient rights.
	//
	if (InStr(UCase(LOG_PROC_WHITELIST_BY_PROCNAME_ONLY), UCase(CSR("*", strProcFullFN, "*"))) > 0)
	{
		// Filter Syslog Process.
		return true;
	}

	//
	// EXCLUDE BY FULL PROCESS IMAGE PATH
	//		according to whitelist filter "LOG_PROC_WHITELIST_WIN10" containing trusted processes NOT to be logged.
	// 
	return (InStr(UCase(LOG_PROC_WHITELIST_WIN10), UCase(CSR("*", strProcFullFN, "*"))) > 0);

	// LOG_PROC_WHITELIST_BY_PROCNAME_ONLY
}


//////////////////////////////
// Log File Operations		//
//////////////////////////////
bool Syslog_Server::LockINI ()
{
	// Ensure the right attributes (no readonly!)
	SetFileAttributes (&slini[0], SLINI_ATTRIB_LOCK);
	
	// Pseudo-Open the file and disallow sharing
	slfile = CreateFile (&slini[0], GENERIC_READ | GENERIC_WRITE, (DEBUG_MODE ? FILE_SHARE_READ : 0), 
							NULL, OPEN_ALWAYS, SLINI_ATTRIB_LOCK, 0);
	if (slfile == INVALID_HANDLE_VALUE)
	{
		MessageBox (0, &CSR("(!) Unable to lock logfile\n\n", 
						"Maybe it is already in use by another application!")[0], 
						"System Logger - Warning", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	
	// (!) Success
	return true;
}

void Syslog_Server::UnlockINI ()
{
	if (slfile != INVALID_HANDLE_VALUE)
	{
		CloseHandle (slfile);
		slfile = INVALID_HANDLE_VALUE;
	}
}


//////////////////////////////////////////
// Thread Interface: CS PROTECTED		//
//////////////////////////////////////////
void Syslog_Server::WriteLog (string strOut, bool bFlush, bool bRareEvent)
{
	//
	// 2012-09-30/Note: If "bRareEvent" is set, no time counter is applied to this entry. Do not use for frequent logging.
	//

	// Variables.
	string datum;
	string zeit;
	string probe;
	long i = 0;

	// Prepare.
	datum = date();
	zeit = time();
	
	// Enter CS.
	cs.Enter();

	// Write log data.
	log.AddSection (datum);
	
	// Check if we need a TIME COUNTER.
	if (bRareEvent)
	{
		// We do NOT need a time counter.
		log.WriteEntry (datum, time(), CSR("> ", strOut));
	}
	else
	{
		// We NEED a time counter.
		while (i <= 99)
		{
			// Generate a time counter.
			probe = FillStr(CStr(i), 2, '0');

			// Check if we need to increment the time counter.
			if (log.FindEntry(datum, CSR(zeit, "=", probe)) == 0)
			{
				log.WriteEntry (datum, CSR(zeit, "=", probe), CSR("> ", strOut));
				break;
			}
			else
			{
				i++;
			}
		}
	}

	// Flush file output if necessary.
	if (bFlush)
	{ 
		// Turn off lock
		UnlockINI();
		SetFileAttributes (&slini[0], SLINI_ATTRIB_UNLOCK);

		// Save
		log.Save (slini, false);

		// Lock again
		LockINI();
	}

	// Leave CS.
	cs.Leave();
}

void Syslog_Server::Switch_LogGeneral (bool onoff)
{
	if (log_general != onoff)
	{
		if (onoff == false)
		{
			// Disable Logging
			WriteLog ("__________________________", false);
			WriteLog ("[!] System logging stopped", false);
			WriteLog ("¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯", true);
		}
		else if (onoff == true)
		{
			EnumProcs ept;
			string allprocs;
			long u, i;

			// Enable Logging
			WriteLog ("__________________________", false);
			WriteLog ("[!] System logging started", false);
			WriteLog ("¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯", false);
			
			// Log Initial Processes
			ept.Enum(); 
			u = ept.Ubound();
			for (i = 1; i <= u; i++)
			{
				allprocs += " "; allprocs += ept.exename(i);
			}
			WriteLog ("Current Processes:", false);
			WriteLog (allprocs, true);
		}

		// Update State
		log_general = onoff;
	}
}

