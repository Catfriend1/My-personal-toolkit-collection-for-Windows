// NetMsg_Server.cpp
//
// Builds a TCP Server on the local machine for the NT Service Part of the application.
// The Server controlsspawning of network messages and receives data from the interactive
// user clients that are allowed to spawn these messages.
//
// Thread-Safe: YES
//
//


//////////////////
// Includes		//
//////////////////
#include "stdafx.h"

#include "../Modules/_modules.h"

#include "NetMsg_vcomm.h"
#include "NetMsg_Server.h"



//////////////////////
// Class: Constants	//
//////////////////////
const long NetMsg_Server::NETMSG_TCP_PORT		= 17887;
const char NetMsg_Server::NETMSG_CAPTION[]		= "Netzwerk-Nachrichtendienst";
const char NetMsg_Server::SECTION_CLIENTS[]		= "Clients";


//////////////
// Types	//
//////////////
struct NetMsg_Server::NetClient
{
	SOCKET id;
};



//////////////////////////////////////
// Class: Constructor, Destructor	//
//////////////////////////////////////
NetMsg_Server::NetMsg_Server ()
{
	// Init RAM.
	netclients.Clear();
	signal_server_shutdown = false;

	// Init CDS.
	cds.New ();
	cds.AddSection (SECTION_CLIENTS);

	// once_net_auth_failed = false;
	// once_invalid_packet_received = false;
}


NetMsg_Server::~NetMsg_Server ()
{
}


//////////////////////
// Class: Functions	//
//////////////////////
bool NetMsg_Server::Init (string strServerPasswordParam)
{
	// Init RAM.
	strServerPassword = strServerPasswordParam;
	
	// Return Success.
	return true;
}

bool NetMsg_Server::Main ()
{
	MSG msg;

	// Start the Listening TCP Server
	if (!srv.Create())
	{
		#ifdef _DEBUG
			DebugOut (CSR("NetMsg Server Failure: Cannot create socket, Error #", CStr(WSAGetLastError())));
		#endif
		return false;
	}

	// Bind to all interfaces.
	if (!srv.Bind("", NETMSG_TCP_PORT))		
	{
		#ifdef _DEBUG
			DebugOut ("NetMsg Server Failure: Cannot bind socket");
		#endif
		return false;
	}
	if (!srv.Listen())
	{
		#ifdef _DEBUG
			DebugOut ("NetMsg Server Failure: Listen Error");
		#endif
		return false;
	}

	// Set Timer
	// SetTimer (0, 0, TIMER_DELAY, NULL);

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
				cds.WriteEntry(SECTION_CLIENTS, CStr((double) newsocket), "");
				cs.Leave();
				
				// Start a new Server Worker
				SERVER_VM.NewThread (&_ext_NetMsg_Server_Server_Worker, (void*) this);
				Sleep (500);
			}
		}
		
		// Check if we got a shutdown signal
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_TIMER)
			{
				// Timer_Event();
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
	// KillTimer (0, 0);

	// Shutdown the Listening Server.
	srv.Close();

	// Shutdown all client connections.
	cs.Enter();
	for (long i = 1; i <= netclients.Ubound(); i++)
	{
		cds.DeleteEntry (SECTION_CLIENTS, CStr((double) netclients[i].id));
		csocket pseudo (netclients[i].id);
		pseudo.Close();
	}
	cs.Leave();

	// Wait for all Server Worker's to shutdown.
	SERVER_VM.AwaitShutdown();

	// Success
	return true;
}

long NetMsg_Server::_ext_NetMsg_Server_Server_Worker (NetMsg_Server* ptr_NetMsg_Server_Class)
{
	return ptr_NetMsg_Server_Class->Server_Worker();
}

long NetMsg_Server::Server_Worker()
{
	// Variables.
	csocket client;
	csocket tempclient;
	string data;
	string outdata;
	unsigned char id;

	// Local indicator variables.
	bool admin_password_verified = false;				// Holds information if administrator logged on remotely.
	bool auth = false;
	bool closed = false;

	// (!) We must obtain the newsocket first before it becomes INVALID, 500 msecs time for that!
	client.Take (netclients[netclients.Ubound()].id);
	
	// Receive Loop.
	while (!signal_server_shutdown)
	{
		if (client.EnterReceiveLoop(&data, &closed))
		{
			// (!) Client closed the connection.
			if (closed) 
			{ 
				break; 
			}
			
			// Get ID
			if (data.length() >= 1) 
			{ 
				id = data[0];
				data.cutoffLeftChars(1);
			} 
			else 
			{ 
				id = 0; 
			}
			
			//
			// Act according to received message ID.
			//
			if (id == NMSG_VCOMM_AUTH)
			{
				if (data == NMSG_AUTH_SIGNATURE)
				{
					// (!) Authenticated
					auth = true;

					// Spawn current settings (none at the moment)
				}
				else
				{
					// (!) Wrong Authentication
					// Abort connection.
					closed = true;
					break;
				}
			}
			else if ((id == NMSG_VCOMM_LOGON) && auth)
			{
				//
				// INFO EVENT: A new machine tells its presence to the server.
				//

				// Enter CS.
				cs.Enter();

				// Add machine to CDS.
				cds.WriteEntry (SECTION_CLIENTS, CStr((double) client.Get()), data);

				// Spawn new CDS machine list to all other clients.
				if (cds.SaveToString(&outdata))
				{
					for (long i = 1; i <= netclients.Ubound(); i++)
					{
						tempclient.Take (netclients[i].id);
						if (!tempclient.mod_send(CSR(Chr(NMSG_VCOMM_SPAWN_MACHINES), outdata)))
						{
							// (!) Connection failure.
							break;
						}
					}
				}

				// Leave CS.
				cs.Leave();
			}
			else if (((id == NMSG_VCOMM_INVOKE_MESSAGE) ||
					  (id == NMSG_VCOMM_INVOKE_SHUTDOWN) || 
					  (id == NMSG_VCOMM_INVOKE_RESTART) || 
					  (id == NMSG_VCOMM_INVOKE_LOGOFF) || 
					  (id == NMSG_VCOMM_INVOKE_GPUPDATE))    
					 && auth && admin_password_verified)
			{
				// IN DATA FORMAT: <...NETIDLIST, separated by "|", example |34|500|...><NULL CHAR><TEXT MESSAGE>
				// Variables.
				StringArray saMain;
				StringArray saNetID;

				// Split data.
				data.split(&saMain, '\0');
				if (saMain.Ubound() >= 2)
				{
					string strSenderName;
					string strMsgToSend;

					// Init RAM.
					strSenderName = cds.GetEntry(SECTION_CLIENTS, CStr((double) client.Get()));
					strMsgToSend = CSR(Chr(id), Chr(0), strSenderName, Chr(0), CSR(saMain[2], Chr(0)));

					// Split first part: Get NetID List out of it.
					saMain[1].split(&saNetID, '|');

					// Walk through selected NetIDs and send the according clients the required command.
					for (long i = 1; i <= saNetID.Ubound(); i++)
					{
						tempclient.Take ((SOCKET) CLong(saNetID[i]));
						if (!tempclient.mod_send(strMsgToSend))
						{
							// (!) Connection failure.
							// Skip this machine and continue.
						}
					}
				}
			}
			else if ((id == NMSG_VCOMM_ADMIN_PASSWORD) && auth)
			{
				if (data.length() > 0)
				{
					if (data == strServerPassword)
					{
						// Password OK.
						if (client.mod_send(Chr(NMSG_VCOMM_ADMIN_PASSWORD_OK)))
						{
							// Set local flag.
							admin_password_verified = true;
						}
						else
						{
							// (!) Connection failure.
							break;
						}
					}
					else
					{
						// The Password is wrong, we will not respond to the client.
						// CAUTION: Do not remove "admin_password_verified = false;" line, 
						//			because this command will be used to reset login status when client dialog window is being exited.
						admin_password_verified = false;
					}
				}
			}
			else if ((id == NMSG_VCOMM_NOTIFY_LOGOFF) && auth)
			{
				// Abort connection because client is logging off.
				closed = true;
				break;
			}
			else
			{
				// (!) Unknown or Invalid Packet received
				// Abort connection.
				closed = true;
				break;
			}
		}
	}
	
	// Enter CS.
	cs.Enter();

	// Delete disconnected Socket from Network Cache.
	// Note: client.Close() must be issued AFTERWARDS or client.Get() will always return zero.
	for (long i = 1; i <= netclients.Ubound(); i++)
	{
		if (netclients[i].id == (SOCKET) client.Get())
		{
			cds.DeleteEntry (SECTION_CLIENTS, CStr((double) netclients[i].id));
			netclients.Remove (i);
			break;
		}
	}

	// Update other client's active machine lists.
	if (cds.SaveToString(&outdata))
	{
		for (long i = 1; i <= netclients.Ubound(); i++)
		{
			tempclient.Take (netclients[i].id);
			if (!tempclient.mod_send(CSR(Chr(NMSG_VCOMM_SPAWN_MACHINES), outdata)))
			{
				// (!) Connection failure.
				// Skip this machine and continue.
			}
		}
	}

	// Leave CS.
	cs.Leave();

	// Client closed connection. Close the connection on server-side.
	client.Close();
	
	// Remove Thread
	SERVER_VM.RemoveThread ();
	return 3;
}



