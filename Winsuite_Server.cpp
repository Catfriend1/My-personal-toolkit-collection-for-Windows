// Winsuite_Server.cpp
// 
// Builds a TCP Server on the local machine for the Service Part of the application.
// The Server controls administrative modules like NTProtect to fire up and shut down if requested to.
// The Server also contains a network virtualbox interface to start and stop virtual machines under the SYSTEM account.
//
// Thread-Safe: YES
//


//////////////////
// Includes		//
//////////////////
#include "stdafx.h"

#include "Modules/_modules.h"

#include "ntService.h"									// Needed for "ntsvc_SendProgressToSCM".

#include "vcomm.h"
#include "Winsuite_Server.h"


//////////////////////
// Local Consts		//
//////////////////////
const long Winsuite_Server::TCP_PORT						= 17889;
const char Winsuite_Server::CAPTION[]						= "Winsuite";

const char Winsuite_Server::REG_POLICIES[]					= "Software\\Microsoft\\Windows\\CurrentVersion\\Policies";


//////////////
// Types	//
//////////////
struct Winsuite_Server::NetClient
{
	SOCKET id;
};



//////////////////////////////////////
// Class: Constructor, Destructor	//
//////////////////////////////////////
Winsuite_Server::Winsuite_Server ()
{
	// Init RAM.
	netclients.Clear();
	signal_server_shutdown = false;
	bIndicateFastTimer = false;							
	
	// Init All Clients Cache.
	cache_locktime = "";

	// Init Registry: NTProtect Related.
	regEnsureKeyExist (HKLM, CSR(REG_POLICIES, "\\System"));

	// Return.
	return;
}


Winsuite_Server::~Winsuite_Server ()
{
	//
}


//////////////////////
// Class: Functions	//
//////////////////////
bool Winsuite_Server::Main ()
{
	// Variables.
	MSG msg;

	//
	// Start the Listening TCP Server
	//
	if (!srv.Create())
	{
		#ifdef _DEBUG
			DebugOut (CSR("WinSuite Server Failure: Cannot create socket, Error #", CStr(WSAGetLastError())));
		#endif
		return false;
	}
	if (!srv.Bind("127.0.0.1", TCP_PORT))		// Only bind to local interface 127.0.0.1
	{
		#ifdef _DEBUG
			DebugOut ("WinSuite Server Failure: Cannot bind socket.");
		#endif
		return false;
	}
	if (!srv.Listen())
	{
		#ifdef _DEBUG
			DebugOut ("WinSuite Server Failure: Listen Error.");
		#endif
		return false;
	}


	//
	// Message Loop
	//
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
				SERVER_VM.NewThread (&_ext_Winsuite_Server_Server_Worker, (void*) this);
				Sleep (500);
			}
		}
		
		// Check if we got a shutdown signal
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				break;
			}
		}
		
		// Set the Server Thread Idle for a time
		Sleep (50);
	}


	//
	// Notify the SCM of our service that the following operation my take a while to complete ...
	//
	ntsvc_SendProgressToSCM (NTSERVICE_DEFAULT_TIMEOUT, false);


	//
	// Signal Server Shutdown.
	//
	signal_server_shutdown = true;

	// Shutdown the Listen Server
	srv.Close();

	// Shutdown all client connections.
	cs.Enter();
	for (long i = 1; i <= netclients.Ubound(); i++)
	{
		csocket pseudo (netclients[i].id);
		pseudo.Close();
	}
	cs.Leave();

	// Wait for all Server Worker's to shutdown.
	SERVER_VM.AwaitShutdown();

	// Success
	return true;
}

long Winsuite_Server::_ext_Winsuite_Server_Server_Worker (Winsuite_Server* ptr_Winsuite_Server_Class)
{
	return ptr_Winsuite_Server_Class->Server_Worker();
}

long Winsuite_Server::Server_Worker()
{
	// Variables.
	csocket client;
	string data;
	unsigned char id;
	bool auth = false;
	bool closed = false;

	// (!) We must obtain the newsocket first before it becomes INVALID, 500 msecs time for that!
	client.Take (netclients[netclients.Ubound()].id);
	
	// Receive Loop
	while (!signal_server_shutdown)
	{
		if (client.EnterReceiveLoop(&data, &closed))
		{
			// (!) Client closed the connection
			if (closed) 
			{ 
				break; 
			}
			
			// Get ID
			if (xlen(data) >= 1)
			{
				id = data[0]; 
			} 
			else 
			{ 
				id = 0; 
			};
			

			// ----------------------------------------------------------------------------
			// ------------------------------   ACT   -------------------------------------
			// ----------------------------------------------------------------------------
			if (id == VCOMM_AUTH)
			{
				if (data.mid(2) == AUTH_SIGNATURE)
				{
					// (!) Authenticated
					auth = true;
				}
				else
				{
					// (!) Wrong Authentication
					closed = true;
					break;
				}
			}
			else if ((id == VCOMM_NTPROTECT_LOCK) && auth)
			{
				// Enter Critical Section
				cs.Enter();

				// The lock can only applied, if cache_locktime == "", so if no lock is currently running
				if (xlen(cache_locktime) == 0)
				{
					// Check if DATE/TIME encrypted data is appended and store it encrypted
					if (xlen(data) > 1)
					{
						// (!) Set the Lock
						cache_locktime = data.mid(2);

						// INTEROP: Update System Policies
						NTProtect_SetPolicies (true);
					}
				}

				// Leave Critical Section
				cs.Leave();
			}
			else if ((id == VCOMM_NTPROTECT_UNLOCK) && auth)
			{
				// Enter Critical Section
				cs.Enter();

				// Check if a lock is currently applied from ANY client
				if (xlen(cache_locktime) > 0)
				{
					// Check if "cache+0x31875" matches the cache_locktime in unencrypted state
					if ((CLong(hex2str(cache_locktime))+0x31875) == CLong(hex2str(data.mid(2))))
					{
						// INTEROP: Update System Policies
						NTProtect_SetPolicies (false);

						// (!) Release the Lock
						cache_locktime = "";
					}
				}

				// Leave Critical Section
				cs.Leave();
			}
			else
			{
				// -- Unknown packet --
				#ifdef _DEBUG
					closed = true;
					DebugOut ("WinSuite Server Error: Invalid Network Message received from client.");
					break;
				#endif
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
	
	// Remove Thread
	SERVER_VM.RemoveThread ();
	return 3;
}


//////////////////////////////////////////
// Server Functions: NTProtect Related	//
//////////////////////////////////////////
void Winsuite_Server::NTProtect_SetPolicies (bool lock_unlock)
{
	#ifdef _DEBUG
	return;
	#endif

	// HKEY_LOCAL_MACHINE can be changed under the Account "LOCAL SYSTEM"
	regWrite(lock_unlock ? 1 : 0, HKLM, CSR(REG_POLICIES, "\\System"), "HideFastUserSwitching");

	// HKEY_CURRENT_USER needs to be changed for every user that is currently logged on.
	RegDir rd;
	long i, u;
	string userPolKey;
	rd.Enum(HKEY_USERS, "");
	u = rd.Ubound();
	for (i = 1; i <= u; i++) {
		// rd[i] contains the user SID
		if (rd[i].length() <= 10) {
			continue;
		}
		if (Left(rd[i], 2) != "S-") {
			continue;
		}
		if (Right(rd[i], 7) == "Classes") {
			continue;
		}
		userPolKey = CSR(rd[i], "\\", REG_POLICIES);

		regEnsureKeyExist(HKEY_USERS, userPolKey);

		regEnsureKeyExist(HKEY_USERS, CSR(userPolKey, "\\Explorer"));
		regWrite(lock_unlock ? 1 : 0, HKEY_USERS, CSR(userPolKey, "\\Explorer"), "NoLogoff");
		regWrite(lock_unlock ? 1 : 0, HKEY_USERS, CSR(userPolKey, "\\Explorer"), "NoClose");

		regEnsureKeyExist(HKEY_USERS, CSR(userPolKey, "\\System"));
		regWrite(lock_unlock ? 1 : 0, HKEY_USERS, CSR(userPolKey, "\\System"), "DisableTaskMgr");
		regWrite(lock_unlock ? 1 : 0, HKEY_USERS, CSR(userPolKey, "\\System"), "DisableLockWorkstation");
		regWrite(lock_unlock ? 1 : 0, HKEY_USERS, CSR(userPolKey, "\\System"), "DisableChangePassword");
	}
}

