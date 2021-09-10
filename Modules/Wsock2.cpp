// Wsock2.cpp: Using the Windows Sockets II Implementation
//
// Thread-Safe: YES
//


//////////////////
// Includes		//
//////////////////
#include "stdafx.h"

#include "_modules.h"


//////////////////////
// Declarations		//
//////////////////////
typedef DWORD (WINAPI*lpRasGetConnStats)(HRASCONN hRasConn, RAS_STATS* lpStatistics);

typedef struct _RASENTRYNAME95 { 
  DWORD dwSize;
  TCHAR szEntryName[RAS_MaxEntryName + 1];
} RASENTRYNAME95;

typedef struct _RASCONN95 { 
  DWORD     dwSize;
  HRASCONN  hrasconn;
  TCHAR     szEntryName[RAS_MaxEntryName + 1];
#if (WINVER >= 0x400)
  TCHAR    szDeviceType[ RAS_MaxDeviceType + 1 ];
  TCHAR    szDeviceName[ RAS_MaxDeviceName + 1 ];										   
#endif
} RASCONN95;


//////////////
// Consts	//
//////////////
const DWORD MAX_COMPUTERNAME			= 1024;		// [1024 for WinNT] [16 for Win9x]
const DWORD NETENUM_BUFFER				= 16384;	// [16384]



//////////////
// Cache	//
//////////////
HMODULE RNA_NT_LIB_DLL = 0;
lpRasGetConnStats dynimp_RasGetConnStats = 0;


//////////////////////////
// Init, Terminate		//
//////////////////////////
const long csocket::WSII_MAX_RECV				= 16384;			// [16384] [8192]

csocket::csocket ()
{
	Init();
}

csocket::csocket (SOCKET take_this_from_outside)
{
	Take (take_this_from_outside);
}

csocket::~csocket ()
{
	// Close Socket if necessary:
	// Only close sockets in the destructor that HAVE NOT BEEN TAKEN from another class
	if (!i_have_taken_a_socket) { Close(); };
}

void csocket::Init()																			// private
{
	// No CS needed because: PRIVATE INIT FUNCTION

	// Inidicate that socket is currently unallocated
	mysocket = 0;

	// Init RAM
	recv_cache.resize (0);
	recv_data.resize (0);
	recv_pos = 0;
	recv_end = 0;
	i_have_taken_a_socket = false;			// Only close sockets in the destructor that HAVE NOT BEEN TAKEN from another class
}

void csocket::Take (SOCKET take_this_from_outside)												// public
{
	// Enter Critical Section
	cs.Enter();

	// Reset RAM first to take a completely different socket in our class instance in
	Init();

	// Store the new socket handle in RAM
	mysocket = take_this_from_outside;
	i_have_taken_a_socket = true;					// Only adjusts the destructor NOT TO CLOSE THIS SOCKET

	// Leave Critical Section
	cs.Leave();
}


//////////////////////////////////
// Socket Creation & Shutdown	//
//////////////////////////////////
bool csocket::Create ()																			// public
{
	bool fail_success;

	// Enter Critical Section
	cs.Enter();

	// Close existing socket first before creating a new one
	if (mysocket != 0) { Close(); };
	
	// Create socket
	mysocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	fail_success = (mysocket != INVALID_SOCKET);
	if (!fail_success) { mysocket = 0; };
	
	// Leave Critical Section
	cs.Leave();

	// Return if operation succeeded
	return fail_success;
}

bool csocket::Close ()																			// public
{
	bool fail_success = false;

	// Enter Critical Section
	cs.Enter();

	if (mysocket != 0)
	{
		// 2012-07-19: Shutdown sequence sometimes blocks when this command is not issued before closing a socket.
		shutdown(mysocket, SD_SEND);

		// Socket is valid
		fail_success = (closesocket(mysocket) == 0);

		// Inidicate that socket is currently unallocated
		mysocket = 0;
	}
	
	// Leave Critical Section
	cs.Leave();

	// Return if operation succeeded
	return fail_success;
}


//////////////////////////////////////
// Socket Bind, Listen, Accept		//
//////////////////////////////////////
bool csocket::Bind (string strLocalHost, int LocalPort)											// public
{
	sockaddr_in sin;
	long lngLocalHost;
	int adrfam;
	bool fail_success = false;
	
	// Check socket allocation
	if (mysocket == 0)
	{
		// Socket is not allocated
		return false;
	}

	// Enter Critical Section
	cs.Enter();
	
	// Determine Address Family
	adrfam = AF_INET;
	
	// Get Long IP Address of LocalHost
	if (xlen(strLocalHost) != 0)
	{
		lngLocalHost = WS2_DetLngAddress(strLocalHost);
		if (lngLocalHost == INADDR_NONE)
		{
			// Leave Critical Section before return
			cs.Leave();
			return false;
		}
	}
	else
	{
		// (!) Bind to any interface
		lngLocalHost = INADDR_ANY;
	}
	
	// Bind
	sin.sin_family = adrfam;
	sin.sin_addr.S_un.S_addr = lngLocalHost;
	sin.sin_port = htons (LocalPort);
	
	fail_success = (bind(mysocket, (sockaddr*) &sin, sizeof(sin)) == 0);
	
	// Leave Critical Section
	cs.Leave();

	// Return if operation succeeded
	return fail_success;
}

bool csocket::Listen ()																			// public
{
	bool fail_success = false;

	// Enter Critical Section
	cs.Enter();

	// Check if socket is allocated
	if (mysocket != 0)
	{
		fail_success = (listen(mysocket, SOMAXCONN) == 0);
	}
	
	// Leave Critical Section
	cs.Leave();

	// Return if operation succeeded
	return fail_success;
}

bool csocket::Accept (SOCKET* newsocket)														// public
{
	sockaddr_in sin;
	SOCKET newhandle;
	int sinlen = sizeof(sin);
	bool fail_success = false;

	// Enter Critical Section
	cs.Enter();
	
	// Check if socket is allocated
	if (mysocket != 0)
	{
		newhandle = accept(mysocket, (sockaddr*) &sin, &sinlen);
		if (newhandle != INVALID_SOCKET)
		{
			fail_success = true;
			*newsocket = newhandle;
		}
	}
	
	// Leave Critical Section
	cs.Leave();

	// Return if operation succeeded
	return fail_success;
}


//////////////////////
// Socket Connect	//
//////////////////////
bool csocket::Connect (string strRemoteHost, int RemotePort)									// public
{
	sockaddr_in sin;
	long lngRemoteHost;
	int adrfam;
	bool fail_success = false;

	// Enter Critical Section
	cs.Enter();
	
	// Check if socket is allocated
	if (mysocket != 0)
	{
		// Determine Address Family
		adrfam = AF_INET;
		
		// Get Long Address of LocalHost
		lngRemoteHost = WS2_DetLngAddress(strRemoteHost);
		if (lngRemoteHost != INADDR_NONE)
		{
			sin.sin_family = adrfam;
			sin.sin_addr.S_un.S_addr = lngRemoteHost;
			sin.sin_port = htons (RemotePort);
			
			fail_success = (connect(mysocket, (sockaddr*) &sin, sizeof(sin)) == 0);
		}
	}
	
	// Leave Critical Section
	cs.Leave();

	// Return if operation succeeded
	return fail_success;
}


//////////////////////
// Socket Status	//
//////////////////////
bool csocket::readable ()																		// public
{
	fd_set read;
	timeval time;
	bool ret = false;

	// Check if socket is allocated
	if (mysocket != 0)
	{
		// Enter Critical Section
		cs.Enter();
	
		// Check for readability: successful if connection is attempted/accessed/closed
		read.fd_count = 1;
		read.fd_array[0] = mysocket;

		// Set Timeout
		time.tv_sec = 0;
		time.tv_usec = 0;

		// Leave Critical Section
		cs.Leave();

		// (!) We are trying working with No CS here, because this can be a blocking operation within WINSOCK2!
		ret = (select(0, &read, 0, 0, &time) == 1);
	}
	
	// Return if socket is "readable"
	return ret;
}

bool csocket::connected ()																		// public
{
	fd_set write;
	timeval time;
	bool ret = false;

	// Check if socket is allocated
	if (mysocket != 0)
	{
		// Enter Critical Section
		cs.Enter();
	
		// Check for connection status: successful if connection is attempted/accessed/closed
		write.fd_count = 1;
		write.fd_array[0] = mysocket;

		// Set Timeout
		time.tv_sec = 0;
		time.tv_usec = 0;

		// Leave Critical Section
		cs.Leave();

		// (!) We are trying working with No CS here, because this can be a blocking operation within WINSOCK2!
		ret = (select(0, 0, &write, 0, &time) == 1);
	}
	
	// Return if socket is "connected"
	return ret;
}


//////////////////////////////
// Socket Data Exchange		//
//////////////////////////////
//
// INFO: This Function is going to send a RAW DATA PACKET over the previously established connection
//
bool csocket::raw_send (string data)															// public
{
	bool fail_success = false;
	
	// Enter Critical Section
	cs.Enter();

	// Check if socket is allocated
	if (mysocket != 0)
	{ 
		// Check if data can be sent
		fail_success = (::send(mysocket, &data[0], xlen(data), 0) != SOCKET_ERROR);
	}
		
	// Leave Critical Section
	cs.Leave();
	
	// Return if the operation succeeded
	return fail_success;
}

//
// INFO: This Function send a DATA PACKET over the previously established connection BY USING OUR CUSTOM PROTOCOL
// PROTOCOL: One Data Buffer consists of <DESCRIPTOR_4-bytes=length-of-packet><DATA-BLOCK_with-length-of-packet-length>
//
const long csocket::MOD_PROTO_DESC_LEN		= 4;		// [4] Must be set according to the descriptor block length, s.o.!

bool csocket::mod_send (string data)															// public
{
	string datbuf;
	long netto_data_length;
	bool fail_success = false;

	// Prepare Data Buffer (datbuf) before attempting to send it
	netto_data_length = xlen(data);
	datbuf.resize(MOD_PROTO_DESC_LEN + netto_data_length);
	CopyMemory (&datbuf[0], &netto_data_length, MOD_PROTO_DESC_LEN);
	CopyMemory (&datbuf[4], &data[0], netto_data_length);
	
	// Enter Critical Section
	cs.Enter();

	// Check if socket is allocated
	if (mysocket != 0)
	{ 
		// Check if data can be sent
		fail_success = (::send(mysocket, &datbuf[0], xlen(datbuf), 0) != SOCKET_ERROR);
	}
		
	// Leave Critical Section
	cs.Leave();
	
	// Return if the operation succeeded
	return fail_success;
}

bool csocket::raw_recv (string* data, bool* closed)												// public
{
	string buf;
	long res;
	bool fail_success = false;

	// Prepare for receiving data
	buf.resize (WSII_MAX_RECV);
	*closed = false;

	//
	// (!) DEADLOCK RISK - CAREFUL!!!
	// INFO: THIS HAD BEEN LOCKED UNTIL 2010-02-21
	//       IT HAS NOW BEEN RELEASED BECAUSE THE WS2::RECV COMMAND BLOCKS UNTIL IT RECEIVES ANY DATA
	//       IF THEN THE CLIENT WANTS TO SEND DATA (TO ACQUIRE DATA PACKETS) BOTH CRITICAL SECTIONS OF SEND/RECV WILL
	//       DEADLOCK FOREVER
	//
	// Enter Critical Section
	//cs.Enter();
	
	// Check if socket is allocated
	if (mysocket != 0)
	{
		// Receive (check had to be done before manually!)
		res = ::recv(mysocket, &buf[0], xlen(buf), 0);
		if (res == 0)
		{
			data->resize (0);
			*closed = true;
			fail_success = true;
		}
		else if (res != SOCKET_ERROR)
		{
			// Return received data from local buffer to target memory
			*data = Left(buf, res);
			fail_success = true;
		}
		else
		{
			// We failed
			fail_success = false;
		}
	}

	// Leave Critical Section
	//cs.Leave();
	
	// Return if the operation succeeded
	return fail_success;
}

bool csocket::EnterReceiveLoop (string* data, bool* conn_closed)								// public
{
	string buf;
	bool sub_closed;
	bool res;
	
	// Clear output buffer and set connection status to 'connected'
	data->resize (0);
	*conn_closed = false;

	// Enter Critical Section
	cs.Enter();
	
	// (?) Have we already handled all data of the IN-STREAM
	if (HandleReceivedFragment("", data))
	{
		// No, and we just received a new chunk "offline"
		// Leave Critical Section before return
		cs.Leave();
		return true;
	}
	
	// Check if new data is available
	//
	// (!) DEADLOCK RISK - CAREFUL!!!
	// INFO: THIS HAD BEEN LOCKED UNTIL 2010-02-21
	//       IT HAS NOW BEEN RELEASED BECAUSE THE WS2::RECV COMMAND BLOCKS UNTIL IT RECEIVES ANY DATA
	//       IF THEN THE CLIENT WANTS TO SEND DATA (TO ACQUIRE DATA PACKETS) BOTH CRITICAL SECTIONS OF SEND/RECV WILL
	//       DEADLOCK FOREVER
	//
	cs.Leave();
	res = raw_recv(&buf, &sub_closed);
	cs.Enter();

	// Check result of raw_recv
	if (res)
	{
		if (sub_closed)
		{
			// (!) Connection has been closed
			*conn_closed = true;

			// Leave Critical Section before return
			cs.Leave();
			return true;
		}
		else
		{
			// (!) We got a new chunk
			bool ret_value = HandleReceivedFragment (buf, data);

			// Leave Critical Section before return
			cs.Leave();
			return ret_value;
		}
	}
	
	// (?) Connection terminated forcefully
	if (WSAGetLastError() == WSAECONNRESET)
	{
		// (!) Connection has been closed
		*conn_closed = true;

		// Leave Critical Section before return
		cs.Leave();
		return true;
	}

	// Leave Critical Section
	cs.Leave();
	
	// (!) No new data available
	return false;
}

bool csocket::HandleReceivedFragment (string buffer, string* ret)								// private
{
	//
	// INFO: No CS here, because that function calls recursively and is protected by its parent with CS!
	//
	// Add the new buffer to the IN-STREAM
	recv_data += buffer;
	
	// (?) Cache already filled so we only need to complete existing data
	if (xlen(recv_cache) != 0)
	{
		long bytesneeded = recv_end - recv_pos;
		
		// Cache is filled but NOT yet COMPLETE
		if (bytesneeded == 0)
		{
			if (WS2_TestMemory(xlen(recv_cache)+100))
			{
				*ret = recv_cache;
				
				// Clear CACHE
				recv_cache.resize (0);
				recv_pos = 0;
				recv_end = 0;
				
				// (!) Success
				return true;
			}
		}
		else if ((xlen(recv_data) <= bytesneeded) && (xlen(recv_data) > 0))
		{
			CopyMemory (&recv_cache[recv_pos], &recv_data[0], xlen(recv_data));
			recv_pos += xlen(recv_data);
			
			// All Data has been copied to the cache => discard the whole IN-STREAM
			recv_data.resize (0);
			
			// Re-Invoke the Handler
			return HandleReceivedFragment ("", ret);
		}
		else if (xlen(recv_data) > bytesneeded)
		{
			// Only copy what is needed to complete the current chunk in CACHE
			CopyMemory (&recv_cache[recv_pos], &recv_data[0], bytesneeded);
			recv_pos += bytesneeded;
			
			// Cut off all "bytesneeded" in the IN-STREAM
			recv_data = Right(recv_data, xlen(recv_data) - bytesneeded);
			
			// Re-Invoke the Handler
			return HandleReceivedFragment ("", ret);
		}
	}
	else
	{
		long chunklen = 0;
		
		// Cache is empty so this is a NEW chunk including the header
		if (xlen(recv_data) >= MOD_PROTO_DESC_LEN)
		{
			CopyMemory (&chunklen, &recv_data[0], MOD_PROTO_DESC_LEN);
			if (WS2_TestMemory(chunklen+100))
			{
				recv_cache.resize (chunklen);
				recv_pos = 0;
				recv_end = chunklen;
				
				// Cut off the MOD_PROTO_DESC_LEN bytes
				recv_data = recv_data.mid(MOD_PROTO_DESC_LEN + 1);
				
				// Re-Invoke the Handler
				return HandleReceivedFragment ("", ret);
			}
		}
	}
	
	// (!) Failed
	return false;
}


//////////////////////
// User Interface	//
//////////////////////
SOCKET csocket::Get ()																			// public
{
	// No CS, atomic variable is being read from shared memory
	return mysocket;
}


//////////////////////////
// Helper Functions		//
//////////////////////////
bool WS2_Start ()
{
	WSADATA wsad;
	bool fail_success;

	// Init WINSOCK2 API once
	fail_success = (WSAStartup(MAKEWORD(2, 2), &wsad) == 0);
	return fail_success;
}

long WS2_DetLngAddress (string host)
{
	HOSTENT* hostent;
	long lngHost;
	
	InetPton(AF_INET, _T(&host[0]), &lngHost);
	if (lngHost == INADDR_NONE)
	{
		hostent = gethostbyname (&host[0]);
		if (hostent != 0)
		{
			CopyMemory (&lngHost, hostent->h_addr_list[0], hostent->h_length);
		}
		else
		{
			lngHost = INADDR_NONE;
		}
	}
	return lngHost;
}

string WS2_DetIPAddress (string host)
{
	long lngaddr = WS2_DetLngAddress (host);
	string tmp, ret;
	if (lngaddr != INADDR_NONE)
	{
		tmp.resize (4);
		CopyMemory (&tmp[0], &lngaddr, 4);
		ret = CSR(CSR(CStr((unsigned char) tmp[0]), ".", CStr((unsigned char) tmp[1]), ".", CStr((unsigned char) tmp[2])), ".", CStr((unsigned char) tmp[3]));
	}
	return ret;
}

bool WS2_TestMemory (long bytes)
{
	HLOCAL testmem;
	
	// Perform the test Memory Allocation
	testmem = LocalAlloc (LMEM_FIXED, bytes);
	if (testmem == 0)
	{
		// (!) -- Out of memory --
		#ifdef _DEBUG
			OutputDebugString ("Wsock2::TestMemory => Out of memory\n");
		#endif
	}
	else
	{
		// Success
		LocalFree (testmem);
	}

	// Return if the operation succeeded
	return (testmem != 0);
}


//////////////////////////
// Class: CommQueue		//
//////////////////////////
CommQueue::CommQueue ()
{
	// Clear() is CS protected.
	Clear();
}

CommQueue::~CommQueue ()
{
	// Clear() is CS protected.
	Clear();
}

void CommQueue::Clear ()																				// public
{
	// CS
	cs.Enter();
	queue.Clear();
	cs.Leave();
}

void CommQueue::Add (string data)																		// public
{
	long u;
	
	// Enter Critical Section
	cs.Enter();

	// Add String to the Queue
	u = queue.Ubound() + 1;
	queue.Redim (u);
	queue[u] = data;

	// Leave Critical Section
	cs.Leave();
}

string CommQueue::Get ()																				// public
{
	long idx;
	string ret;
	
	// Enter Critical Section
	cs.Enter();

	// Get and Remove the next element from the queue
	idx = queue.Ubound();
	if (idx >= 1)
	{
		ret = queue[1];
		queue.Remove(1);
	}

	// Leave Critical Section
	cs.Leave();

	// Return Element
	return ret;
}

bool CommQueue::SearchAndRemove (unsigned char msgid, string* data)										// public
{
	bool msg_found_and_removed = false;
	*data = "";

	// Enter Critical Section
	cs.Enter();

	// Search a specific OWN PROTOCOL MESSAGE ID and REMOVE the MESSAGE FROM THE QUEUE
	for (long i = 1; i <= queue.Ubound(); i++)
	{
		if (queue[i].length() >= 1)
		{
			if ((unsigned char) queue[i][0] == msgid)
			{
				*data = queue[i];
				queue.Remove (i);
				msg_found_and_removed = true;
				break;
			}
		}
	}

	// Leave Critical Section
	cs.Leave();

	// Return if the Message had been found and removed
	return msg_found_and_removed;
}


//////////////////////////
// Class: EnumNetwork	//
//////////////////////////
EnumNetwork::EnumNetwork ()
{
}

EnumNetwork::~EnumNetwork ()
{
	Clear();
}

void EnumNetwork::Clear ()																				// public
{
	// CS
	cs.Enter();
	wmnet.Clear();
	cs.Leave();
}

void EnumNetwork::Enum ()																				// public
{
	// Enter Critical Section
	cs.Enter();

	// Clear and Start Enum
	Clear();
	EnumResource (NULL);

	// Leave Critical Section
	cs.Leave();
}

void EnumNetwork::EnumResource (NETRESOURCE* netres_start)												// private
{
	//
	// INFO: No CS needed because: PRIVATE FUNCTION, protected by the CS of the calling procedure
	//
	NETRESOURCE* netres;
	HANDLE henum;
	DWORD i, entries;		
	DWORD bufsize;
	long eres, u;

	// Allocate Memory
	netres = (NETRESOURCE*) GlobalAlloc (GMEM_FIXED, NETENUM_BUFFER);
	
	// Start Enum
	if (WNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_ANY, 0, netres_start, &henum) == NO_ERROR)
	{
		do
		{
			// Clear Memory and Params for WNetEnumResource
			ZeroMemory (netres, NETENUM_BUFFER);
			bufsize = NETENUM_BUFFER;	// (size of local buffer)
			entries = -1;				// (as many entries as possible)
			
			// Enumerate
			eres = WNetEnumResource(henum, &entries, netres, &bufsize);
			if (eres == NO_ERROR)
			{
				for (i = 0; i < entries; i++)
				{
					// Add Resource to Cache
					u = wmnet.Ubound() + 1;
					wmnet.Redim (u);
					wmnet[u].local		= pBuffer2String(netres[i].lpLocalName);
					wmnet[u].remote		= pBuffer2String(netres[i].lpRemoteName);
					wmnet[u].prov		= pBuffer2String(netres[i].lpProvider);
					wmnet[u].comment	= pBuffer2String(netres[i].lpComment);
					wmnet[u].type		= netres[i].dwType;
					wmnet[u].disptype	= netres[i].dwDisplayType;
					
					// Check if subentries do exist
					if (netres[i].dwUsage & RESOURCEUSAGE_CONTAINER)
					{
						EnumResource (&netres[i]);
					}
				}
			}
		} while (eres != ERROR_NO_MORE_ITEMS);
		
		// Close current enumeration
		WNetCloseEnum (henum);
	}
	
	// Free Memory
	GlobalFree ((HGLOBAL) netres);
}

WMTNET&	EnumNetwork::operator[] (long idx)																// public
{
	WMTNET* ret;

	// CS
	cs.Enter();
	ret = &wmnet[idx];
	cs.Leave();

	// Return Object Pointer --> Be CAREFUL in the CALLING THREAD!
	return *ret;
}

long EnumNetwork::Ubound ()																				// public
{
	long u;

	// Enter Critical Section
	cs.Enter();

	// Get Item Count in Cache
	u = wmnet.Ubound();

	// Leave Critical Section
	cs.Leave();

	// Return value
	return u;
}


//////////////////////
// Network Info		//
//////////////////////
string GetComputerName ()
{
	string buf; buf.resize (MAX_COMPUTERNAME);
	DWORD length = MAX_COMPUTERNAME;
	if (GetComputerName(&buf[0], &length))
	{
		buf.resize (length);
	}
	else
	{
		buf.resize (0);
	}
	return buf;
}


//////////////////////////////
// RAS Module Bootstrap		//
//////////////////////////////
bool RNA_NT_LoadModules ()
{
	// Start loading
	// Not available for Windows 9x/Me
	RNA_NT_LIB_DLL = LoadLibrary ("RasApi32.dll");
	if (RNA_NT_LIB_DLL == 0)
	{
		return false;
	}
	
	dynimp_RasGetConnStats = (lpRasGetConnStats) GetProcAddress (RNA_NT_LIB_DLL, "RasGetConnectionStatistics");
	if (dynimp_RasGetConnStats == 0)
	{
		return false;
	}
	
	// (!) Success
	return true;
}

void RNA_NT_UnloadModules ()
{
	if (RNA_NT_LIB_DLL != 0)
	{
		FreeLibrary (RNA_NT_LIB_DLL);
		RNA_NT_LIB_DLL = 0;
	}
}


//////////////////////////////////////////////
// Functions: RNA Connect / Disconnect		//
//////////////////////////////////////////////
DWORD RNA_Connect (HWND parent, string conn, long flags)
{
	DWORD hrasconn;
	if (InternetDial(parent, &conn[0], flags, &hrasconn, 0) == ERROR_SUCCESS)
	{
		return hrasconn;
	}
	return 0;
}

bool RNA_Disconnect (DWORD hconn)
{
	return (InternetHangUp(hconn, 0) == ERROR_SUCCESS);
}


//////////////////////////////
// Functions: WinInet		//
//////////////////////////////
bool WinInet_DownloadFile (string* strRetBuf, string strURL, string* strRetError, DWORD dwInternetOpenType)
{
	// Variables.
	HINTERNET hint;
	HINTERNET hfile;
	DWORD dwBytesRead = 0;
	string buf;
	string strResultBuffer;

	// Open WinInet Interface.
	hint = InternetOpen ("WinInet Client Application", dwInternetOpenType, NULL, NULL, 0);
	if (hint == 0)
	{
		// (!) Failed.
		// Return Error.
		if (strRetError != NULL)
		{
			(*strRetError) = "WinInet_DownloadFile: InternetOpen() failed.\n";
		}
		return false;
	}

	// Connect to Server and request the file given by the specified URL.
	hfile = InternetOpenUrl (hint, &strURL[0], NULL, 0, INTERNET_FLAG_RELOAD, 0);
	if (hfile == 0)
	{
		// (!) Failed.
		// Close WinInet Interface.
		InternetCloseHandle (hint);

		// Return Error.
		if (strRetError != NULL)
		{
			(*strRetError) = CSR("WinInet_DownloadFile: Error downloading \"", strURL, "\".\n");
		}
		return false;
	}

	// Prepare buffers.
	buf.resize (512 * 1024);			// 512 KByte Receive Buffer
	strResultBuffer.resize(0);

	// File Retrieve Loop.
	while (true)
	{
		if (InternetReadFile(hfile, &buf[0], (DWORD) buf.length(), &dwBytesRead))
		{
			if (dwBytesRead == 0)
			{
				// Finished file download.
				break;
			}
			else
			{
				// Download still in progress.
				// Add temporary buffer to output result cache.
				strResultBuffer.Append (buf.mid(1, dwBytesRead));
			}
		}
		else
		{
			// (!) Transfer unexpectedly interrupted.
			// Close connection gracely on our side.
			InternetCloseHandle (hfile);

			// Close WinInet Interface.
			InternetCloseHandle (hint);

			// Report Error.
			if (strRetError != NULL)
			{
				(*strRetError) += CSR("WinInet_DownloadFile: Transfer interrupted while downloading \"", strURL, "\".\n");
			}

			// Wait a little before retrying the download.
			Sleep(5000);

			// Retry the download.
			return WinInet_DownloadFile(strRetBuf, strURL, strRetError);
		}

		// Put wait state.
		Sleep (80);
	}

	// Finished downloading.
	if (dwBytesRead == 0)
	{
		// Content is being returned via the "strRetBuf" reference to the calling function.
		if (strRetBuf != NULL)
		{
			// Output buffer pointer is valid.
			(*strRetBuf) = strResultBuffer;
		}
	}
	else
	{
		// Report Error.
		if (strRetError != NULL)
		{
			(*strRetError) += "WinInet_DownloadFile: Unexpected amount of bytes have been read.\n";
		}
	}

	// Close connection.
	InternetCloseHandle (hfile);
		
	// Close WinInet Interface.
	InternetCloseHandle (hint);

	// Return Success.
	return true;
}

