// RawPrintServer.cpp
//
// Thread-Safe: NO
//


//////////////////
// Includes		//
//////////////////
#include "stdafx.h"
#include "../Modules/_modules.h"

#include <windows.h>
#include <stdio.h>
#include <winspool.h>

#include "RawPrintServer.h"


//////////////////////
// Local Consts		//
//////////////////////
#define SLEEP_TIME 5000
#define LOGFILE "C:\\PrintServer.log"


//////////////
// Types	//
//////////////



//////////////////////////////////////
// Class: Constructor, Destructor	//
//////////////////////////////////////
RawPrintServer::RawPrintServer ()
{
	// Init RAM.
		
	
	// Init Registry.
	// regEnsureKeyExist (HKLM, CSR(REG_POLICIES, "\\..."));


	// Return.
	return;
}


RawPrintServer::~RawPrintServer ()
{
	//
}


//////////////////////////
// Class: Functions		//
//////////////////////////
bool RawPrintServer::Run (string strPrinterName, int lngServerPort)
{
	// Variables.
	SOCKET sock;
	char buffer[1024];
	DWORD wrote;
	HANDLE hPrinter = NULL;

	// WS2 Start
	if (!WS2_Start())
	{
		// FAIL.
		return false;
	}

	// Let's do the loop.
	while (true)
	{
    	sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock == INVALID_SOCKET)
		{
			// sprintf(strTemp, "Error: no socket: %d", WSAGetLastError());
			// WriteToLog(strTemp);
			return 0;
		}

		sockaddr_in addr = {0};
		addr.sin_family = AF_INET;
		addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(lngServerPort);

		if (bind(sock, (const struct sockaddr *) &addr, sizeof(addr)) != 0)
		{
			// sprintf(strTemp, "Error: couldn't bind: %d", WSAGetLastError());
			// WriteToLog(strTemp);
			closesocket(sock);
			return 0;
		}

		sockaddr_in client = {0};
		int size = sizeof(client);

		while (listen(sock, 2) != SOCKET_ERROR)
		{
			SOCKET sock2 = accept(sock, (struct sockaddr *) &client, &size);
			if (sock2 != INVALID_SOCKET)
			{
				// sprintf(strTemp, "Accept print job for %s from %d.%d.%d.%d", printerName, client.sin_addr.S_un.S_un_b.s_b1, client.sin_addr.S_un.S_un_b.s_b2, client.sin_addr.S_un.S_un_b.s_b3, client.sin_addr.S_un.S_un_b.s_b4);
				// WriteToLog(strTemp);

				DOC_INFO_1 info;
				info.pDocName = "Forwarded Job";
				info.pOutputFile = NULL;
				info.pDatatype   = "RAW";

				//char *printerName = "Test;//"Brother HL-1430 series";
				if (!OpenPrinter(&strPrinterName[0], &hPrinter, NULL) || 
                    !StartDocPrinter(hPrinter, 1, (LPBYTE)&info))
				{
					// WriteToLog ("Error opening print job.");
				}
				else
				{
					while (1)
					{
						int result = recv(sock2, buffer, sizeof(buffer), 0);
						if (result <= 0)
						{
							break;
						}

						WritePrinter(hPrinter, buffer, result, &wrote);
						if (wrote != (DWORD) result)
						{
							// WriteToLog ("Couldn't print all data.");
							break;
						}
					}
					ClosePrinter(hPrinter);
				}
			}
			closesocket(sock2);
		}

		closesocket(sock);
		return 1;
	}

	// WS2 Cleanup.
	WSACleanup();

	// Return Success.
	return true;
}

