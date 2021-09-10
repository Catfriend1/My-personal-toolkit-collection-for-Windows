// NetMsg_Client.cpp
//
// Performs a connection to the NTService via the TCP Protocol.
// Receives commands to display messages or to shutdown/restart the system.
// 
// Thread-Safe: YES
//



//////////////////
// Includes		//
//////////////////
#include "stdafx.h"
#include "../resource.h"

#include "../Modules/_modules.h"
#include "../Boot.h"

#include "NetMsg_vcomm.h"
#include "NetMsg_Client.h"



//////////////////////
// Class: Constants	//
//////////////////////
const long NetMsg_Client::NETMSG_TCP_PORT					= 17887;
const char NetMsg_Client::NETMSG_CAPTION[]					= "Netzwerk-Nachrichtendienst";
const char NetMsg_Client::CLS_HOSTWINDOW[]					= "NetMsg_HostWndClass";
const long NetMsg_Client::MAX_LISTBOX_ITEMS					= 2048;
const long NetMsg_Client::CONNECTION_TIMEOUT				= 5000;						// [5000] = 5 sec

const int  NetMsg_Client::SYSICON_TIMER_ID					= 1;
const long NetMsg_Client::SYSICON_TIMER_INTERVAL			= 15000;					// [15000]

const int  NetMsg_Client::LISTUPDATE_TIMER_ID				= 2;
const long NetMsg_Client::LISTUPDATE_TIMER_INTERVAL			= 5000;						// [5000]

const long NetMsg_Client::ENTERPWRES_STAYLOCKED				= 0;
const long NetMsg_Client::ENTERPWRES_UNLOCK					= 1;

const long NetMsg_Client::MAINMESSAGE_THREAD_RES			= -114;


//////////////////////////
// Class: CDS Consts	//
//////////////////////////
const char NetMsg_Client::SECTION_CLIENTS[]					= "Clients";


//////////////
// Types	//
//////////////



//////////////////////////////////////
// Class: Constructor, Destructor	//
//////////////////////////////////////
NetMsg_Client::NetMsg_Client ()
{
	// Init RAM.
	MAINMESSAGE_THREAD_ID = 0;
	signal_client_shutdown = false;
	sysicon = 0;
	signal_cds_update = false;
	cds.New();

	// Load Resources.
	LoadResources();

	// Register UI Objects.
	NewWindowClass (CLS_HOSTWINDOW, &_ext_NetMsg_Client_HostWndProc, myinst);

	// Add Systray Icon.
	sysicon = new SystrayIcon;

	// Spawn Main Message Queue Thread.
	NETMSG_VM.NewThread (&_ext_NetMsg_MainMessageQueue_Thread, (void*) this, &MAINMESSAGE_THREAD_ID);
	
	// Return Success.
	// return true;
}

NetMsg_Client::~NetMsg_Client ()
{
	// Terminate Main Message Queue Thread.
	if (MAINMESSAGE_THREAD_ID != 0)
	{
		PostThreadMessage (MAINMESSAGE_THREAD_ID, WM_QUIT, MAINMESSAGE_THREAD_RES, 0);

		// Wait for Main Message Queue Thread to finish.
		NETMSG_VM.AwaitShutdown();
	}

	// Remove Systray Icon
	if (sysicon != 0) 
	{ 
		sysicon->Hide();
		delete sysicon; 
	}

	// Destroy Window Objects
	DestroyWindow (hostwnd);
	
	// Unregister Window Class
	UnregisterClass (CLS_HOSTWINDOW, myinst);

	// Free Resources
	FreeResources();
}


//////////////////////////////////
// Class: Resource Management	//
//////////////////////////////////
void NetMsg_Client::LoadResources ()
{
	L_IDI_NETMSG_NOMSG = LoadIconRes (myinst, IDI_NETMSG_NOMSG, 16, 16);
	L_IDI_NETMSG_NEWMSG = LoadIconRes (myinst, IDI_NETMSG_NEWMSG, 16, 16);
	L_IDI_NETMSG_CONNECTING = LoadIconRes (myinst, IDI_NETMSG_CONNECTING, 16, 16);
	L_IDI_NETMSG_DISCONNECTED = LoadIconRes (myinst, IDI_NETMSG_DISCONNECTED, 16, 16);
}

void NetMsg_Client::FreeResources ()
{
	DestroyIcon (L_IDI_NETMSG_NOMSG);
	DestroyIcon (L_IDI_NETMSG_NEWMSG);
	DestroyIcon (L_IDI_NETMSG_CONNECTING);
	DestroyIcon (L_IDI_NETMSG_DISCONNECTED);
}


//////////////////////////////
// Functions: Main Thread	//
//////////////////////////////
// protected
long NetMsg_Client::_ext_NetMsg_MainMessageQueue_Thread (NetMsg_Client* class_ptr)
{
	if (class_ptr != 0) 
	{ 
		return class_ptr->MainMessageQueue_Thread(); 
	} 
	else 
	{ 
		return -1; 
	}
}

// private
long NetMsg_Client::MainMessageQueue_Thread ()
{
	MSG msg;

	// Create Host & Tooltip Window
	hostwnd = CreateNullWindow ("Host Window", 0, CLS_HOSTWINDOW);
	SetWindowLongPtr (hostwnd, GWLP_USERDATA, (LONG_PTR) this);
	sysicon->SetData (L_IDI_NETMSG_NEWMSG, NETMSG_CAPTION, hostwnd, 0);
	sysicon->Show();
	SetTimer (hostwnd, SYSICON_TIMER_ID, SYSICON_TIMER_INTERVAL, NULL);

	// Start NetClient Worker Thread
	CLIENT_VM.NewThread (&_ext_NetMsg_Client_Client_Worker, (void*) this);

	// Message Loop
	while (true)
	{
		// Check if we got a shutdown signal
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				// Abort Message Loop
				break;
			}
			else if (!IsDialogMessage(GetAbsParent(msg.hwnd), &msg))
			{
				TranslateMessage (&msg);
				DispatchMessage (&msg);
			}
		}
		
		// Set the Client Thread Idle for a time
		Sleep (50);
	}

	// Stop Timers
	KillTimer (hostwnd, SYSICON_TIMER_ID);

	// Signal all CLIENT_VM's a shutdown
	signal_client_shutdown = true;


	//
	// Note: If the client application is shutting down (due to a windows logoff event),
	//		 notify the server of this.
	if (signal_client_shutdown && client.connected())
	{
		// Note: It is not necessary to send logoff messages.
		//		 We'll just close the connection.
		// if (!client.mod_send(Chr(NMSG_VCOMM_NOTIFY_LOGOFF))) 
		// {
		// 	// Unhandled connection error.
		// }
	}

	// Client Receive Loop is blocked by call to WS2::recv. Close the connection forcefully to interrupt.
	client.Close();

	// Shutdown NetClient Workers.
	CLIENT_VM.AwaitShutdown();

	// Remove Thread.
	NETMSG_VM.RemoveThread();

	// (!) Success.
	return (long) msg.wParam;
}

long NetMsg_Client::_ext_NetMsg_Client_Client_Worker (NetMsg_Client* ptr_NetMsg_Client_Class)
{
	return ptr_NetMsg_Client_Class->Client_Worker();
}

long NetMsg_Client::Client_Worker()
{
	while (!signal_client_shutdown)
	{
		// Initialize Client Socket
		if (!client.Create())
		{
			DebugOut ("NetMsg Client Failure: Cannot create socket.");
			CLIENT_VM.RemoveThread();
			return 1114;
		}
		
		// Try to connect
		if (client.Connect(REGV_NETMSG_SERVER_ADDRESS, NETMSG_TCP_PORT))
		{
			string data;
			unsigned char id;
			bool closed = false;
			
			// Authenticate with the server.
			if (!client.mod_send(CSR(Chr(NMSG_VCOMM_AUTH), NMSG_AUTH_SIGNATURE))) { break; };
			
			// Tell the server who we are exactly.
			if (!client.mod_send(CSR(Chr(NMSG_VCOMM_LOGON), GetComputerName(), "\\", GetCurrentUser())))
			{
				// Connection failure.
				break;
			}

			// CLIENT WORKERS RECEIVE LOOP
			while (!signal_client_shutdown)
			{
				if (client.EnterReceiveLoop(&data, &closed))
				{
					// (!) Server closed the connection
					if (closed) 
					{ 
						break; 
					}
					
					// Get Network Message ID.
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
					// Act according to network message ID.
					//
					if (id == NMSG_VCOMM_SPAWN_MACHINES)
					{
						//
						// Info: Server sent us its CDS cache. We'll copy it over to our class memory.
						//		 The server performs this automatically when the list of active machines changes.
						//
						// CS needed, because dialog window frequently checks CDS storage for updates.
						cs.Enter();
						cds.LoadFromString (data);
						signal_cds_update = true;
						cs.Leave();
					}
					else if (id == NMSG_VCOMM_INVOKE_RESTART)
					{
						//
						// ACTION: Restart windows.
						//

						// Variables.
						StringArray saTemp;

						// Split data.
						data.split(&saTemp, '\0');
						if (saTemp.Ubound() >= 2)
						{
							ShellExecute (0, CSR(GetSystemDir32(), "\\shutdown.exe -r -f -t 60 -c \"", 
												(LANG_EN ? CSR("Please save all your work. This computer will be restarted in 60 seconds by ", saTemp[1], ".") :  
													CSR("Bitte speichern Sie Ihre Arbeit. Der Computer wird in 60 Sekunden von ", saTemp[1], " neu gestartet.")), 
													"\""));
						}
					}
					else if (id == NMSG_VCOMM_INVOKE_SHUTDOWN)
					{
						//
						// ACTION: Shutdown windows.
						//
						
						// Variables.
						StringArray saTemp;

						// Split data.
						data.split(&saTemp, '\0');
						if (saTemp.Ubound() >= 2)
						{
							ShellExecute (0, CSR(GetSystemDir32(), "\\shutdown.exe -s -f -t 60 -c \"",  
												(LANG_EN ? CSR("Please save all your work. This computer will be shutdown in 60 seconds by ", saTemp[1], ".") :  
													CSR("Bitte speichern Sie Ihre Arbeit. Der Computer wird in 60 Sekunden von ", saTemp[1], " heruntergefahren.")), 
													"\""));
						}
					}
					else if (id == NMSG_VCOMM_INVOKE_LOGOFF)
					{
						//
						// ACTION: Logoff current user from windows.
						//
						
						// Variables.
						StringArray saTemp;

						// Split data.
						data.split(&saTemp, '\0');
						if (saTemp.Ubound() >= 2)
						{
							//
							// 2013-03-12/note: Windows 7 shutdown does NOT support "-t" or "-c" parameter.
							//
							// string tmpMsg = (LANG_EN ? CSR("Please save all your work. You will be logged off from windows in 60 seconds by ", saTemp[1], ".") :  
							//								CSR("Bitte speichern Sie Ihre Arbeit. Sie werden in 60 Sekunden durch ", saTemp[1], " von Windows abgemeldet."));
							//
							ShellExecute (0, CSR(GetSystemDir32(), "\\shutdown.exe -l -f"));
						}
					}
					else if (id == NMSG_VCOMM_INVOKE_GPUPDATE)
					{
						//
						// ACTION: Run gpupdate in local user's context and reboot machine afterwards.
						//

						// Variables.
						StringArray saTemp;

						// Split data.
						data.split(&saTemp, '\0');
						if (saTemp.Ubound() >= 2)
						{
							// ShellExecute "gpupdate".
							ShellExecute (0, CSR(GetSystemDir32(), "\\gpupdate.exe /Force /Boot"));
						}
					}
					else if (id == NMSG_VCOMM_INVOKE_MESSAGE)
					{
						//
						// ACTION: Show message to user.
						//

						// Variables.
						StringArray saTemp;
						string strDateTimeUI = CSR("Datum: ", date_ger(), ", Zeit: ", _clock(), " Uhr");

						// Split data.
						data.split(&saTemp, '\0');
						if (saTemp.Ubound() >= 2)
						{
							// Display an administrative message to the current user.
							ThreadInfoBox (0, CSR(strDateTimeUI, "\n", saTemp[2]), 
												CSR("Nachricht von ", saTemp[1]), 
												MB_OK | MB_SYSTEMMODAL);
						}
					}
					else if (id == NMSG_VCOMM_ADMIN_PASSWORD_OK)
					{
						// Add this message to the Response Queue for later processing.
						RespQueue.Add(CSR(Chr(id), data));
					}
					else
					{
						// -- Unknown packet --
						#ifdef _DEBUG
							closed = true;
							DebugOut ("NetMsg Client Error: Invalid Network Message received from server");
							break;
						#endif
					}
				}
			}
		}

		// Transfer finished or no connection, so close that socket and retry
		client.Close();

		// Wait some time before attempting to reconnect to the service
		if (!signal_client_shutdown) 
		{ 
			Sleep(10000); 
		}
	}

	// Remove Thread
	CLIENT_VM.RemoveThread();
	return 5;
}



//////////////////////
// Window Process	//
//////////////////////
// Host Window
LRESULT CALLBACK NetMsg_Client::_ext_NetMsg_Client_HostWndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	NetMsg_Client* cbclass = (NetMsg_Client*) GetWindowLongPtr (hwnd, GWLP_USERDATA);
	if (cbclass != 0)
	{
		return cbclass->HostWndProc (hwnd, msg, wparam, lparam);
	}
	else
	{
		return DefWindowProc (hwnd, msg, wparam, lparam);
	}
}

LRESULT CALLBACK NetMsg_Client::HostWndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// Static Variables.
	static bool bRecallLocked = false;

	// Event Handlers.
	if (msg == WM_TIMER)
	{
		if ((wparam == SYSICON_TIMER_ID) && (sysicon != 0))
		{
			sysicon->Show();
		}
	}
	else if (msg == WM_CLOSE)
	{
		// Do not invoke WM_DESTROY.
		// The client may not be externally closed.
		return false;
	}
	else if (msg == WM_QUERYENDSESSION)
	{
		if (!TerminateVKernel_Allowed())
		{
			return false;
		}
		else
		{
			// It's ok to terminate Windows Session
			return true;
		}
	}
	else if (msg == WM_ENDSESSION)
	{
		if (!TerminateVKernel_Allowed())
		{
			return false;
		}

		if (wparam == 1)
		{
			// Session is being SURELY ended after return (this is the MainThread).
			TerminateVKernel (true);
			return false;
		}
	}
	else if (msg == WM_LBUTTONUP)
	{
		if (lparam == WM_RBUTTONUP)
		{
			// Recall Lock: Open Popup Menu.
			if (!bRecallLocked)
			{
				bRecallLocked = true;
				ShowSystrayMenu();
				bRecallLocked = false;
			}
		}
	}
	return DefWindowProc (hwnd, msg, wparam, lparam);
}

// Password Checking Dialog
INT_PTR CALLBACK NetMsg_Client::_ext_NetMsg_Client_Callback_EnterPW_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static NetMsg_Client* class_ptr = 0;
	if (class_ptr != 0)
	{
		return class_ptr->EnterPW_WndProc (hwnd, msg, wparam, lparam);
	}
	else
	{
		if (msg == WM_INITDIALOG)
		{
			class_ptr = (NetMsg_Client*) lparam;
			return class_ptr->EnterPW_WndProc (hwnd, msg, wparam, lparam);
		}
	}
	return DefWindowProc (hwnd, msg, wparam, lparam);
}

INT_PTR CALLBACK NetMsg_Client::EnterPW_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_INITDIALOG)
	{
		// Set Texts
		ctlSetText (hwnd, LANG_EN ? "Enter Password" : "Passworteingabe");
		ctlSetText (dia(hwnd, IDL_LOGINPROMPT), 
							LANG_EN ? "Please enter the administrative password to access Network Messenger's main menu." : 
										"Bitte geben Sie das administrative Passwort ein, um auf die Einstellungen des Netzwerk-Nachrichtendienstes zuzugreifen.");
		ctlSetText (dia(hwnd, IDCANCEL), LANG_EN ? "Can&cel" : "Abbre&chen");
		
		// Focus the Password Field
		SetFocus (dia(hwnd, IDC_PW));
	}
	else if (msg == WM_COMMAND)
	{
		long id = LOWORD(wparam);
		long msg = HIWORD(wparam);
		
		// Buttons
		if (msg == BN_CLICKED)
		{
			if (id == IDOK)
			{
				if (EnterPW_IsPWCorrect(ctlGetText(dia(hwnd, IDC_PW))))
				{
					// Passwork OK.
					EndDialog (hwnd, ENTERPWRES_UNLOCK);
				}
				else
				{
					// Wrong Password.
					InfoBox (hwnd, LANG_EN ? "Logon failed: Incorrect password or server unavailable!" : 
												"Login fehlgeschlagen: Das Passwort ist falsch oder der Server ist momentan nicht verfügbar!", 
													NETMSG_CAPTION, MB_OK | MB_ICONEXCLAMATION);
					ctlSetText (dia(hwnd, IDC_PW), "");
					SetFocus (dia(hwnd, IDC_PW));
				}
			}
			else if (id == IDCANCEL)
			{
				EndDialog (hwnd, ENTERPWRES_STAYLOCKED);
			}
		}
	}
	else if (msg == WM_CLOSE)
	{
		// End password dialog upon user request.
		EndDialog (hwnd, ENTERPWRES_STAYLOCKED);
	}
	return false;
}

bool NetMsg_Client::EnterPW_IsPWCorrect (string strPassword)
{
	DWORD dwTicks;
	string data;

	// Avoid sending empty passwords because they are nasty.
	if (strPassword.length() == 0)
	{
		// Simulate Wrong Password.
		return false;
	}

	// Clear Response Queue. This avoids old authentication acks to be present for current processing.
	RespQueue.Clear();

	// Tell the server our given password.
	if (!client.mod_send(CSR(Chr(NMSG_VCOMM_ADMIN_PASSWORD), strPassword)))
	{
		// (!) Connection failure. Abort authentication process.
		return false;
	}

	// Wait for server response.
	dwTicks = GetTickCount();
	while (!RespQueue.SearchAndRemove(NMSG_VCOMM_ADMIN_PASSWORD_OK, &data) && 
			!(GetTickCount() > (dwTicks + CONNECTION_TIMEOUT)) &&
			!signal_client_shutdown)
	{
		Sleep (50);
	}

	// Return "Password OK".
	return (data == Chr(NMSG_VCOMM_ADMIN_PASSWORD_OK));
}

// NetMsg Main Window Dialog
INT_PTR CALLBACK NetMsg_Client::_ext_NetMsg_Client_MainDlg_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static NetMsg_Client* class_ptr = 0;
	if (class_ptr != 0)
	{
		return class_ptr->MainDlg_WndProc (hwnd, msg, wparam, lparam);
	}
	else
	{
		if (msg == WM_INITDIALOG)
		{
			class_ptr = (NetMsg_Client*) lparam;
			return class_ptr->MainDlg_WndProc (hwnd, msg, wparam, lparam);
		}
	}
	return DefWindowProc (hwnd, msg, wparam, lparam);
}

INT_PTR CALLBACK NetMsg_Client::MainDlg_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_INITDIALOG)
	{
		// Set update timer for machine list.
		SetTimer (hwnd, LISTUPDATE_TIMER_ID, LISTUPDATE_TIMER_INTERVAL, NULL);
		PostMessage (hwnd, WM_TIMER, LISTUPDATE_TIMER_ID, 0);
		signal_cds_update = true;
	}
	else if (msg == WM_TIMER)
	{
		if (wparam == LISTUPDATE_TIMER_ID)
		{
			//
			// (!) CS needed: The list should not be updated in background via network while GUI is accessing it.
			//
			// Enter CS.
			cs.Enter();

			// Check if the machine list has been updated by the network client thread.
			if (signal_cds_update)
			{
				HWND hLB = dia(hwnd, IDC_LB);
				StructArray <string> saClientIDs;
				long lngOldSelCount;
				long lngArrayOldSelItems[MAX_LISTBOX_ITEMS];
				string strOldSel;
				string strMachineUser;
				long lngNewItem;

				// Read old list selections.
				lngOldSelCount = lbGetSelItems(hLB, lngArrayOldSelItems, MAX_LISTBOX_ITEMS);
				for (long i = 0; i < lngOldSelCount; i++)		// zero-based return array
				{
					// Store a string with all selected machine and user names.
					strOldSel += CSR(Chr(0), lbText(hLB, lngArrayOldSelItems[i]), Chr(0));
				}

				// Update machine list.
				// ListBox: Selection := Mehrfach, Sorted := True;
				cds.EnumEntries (SECTION_CLIENTS, &saClientIDs);
				lbClear (hLB);
				for (long i = 1; i <= saClientIDs.Ubound(); i++)
				{
					strMachineUser = cds.GetEntry(SECTION_CLIENTS, saClientIDs[i]);
					lngNewItem = lbAdd(hLB, strMachineUser);
					lbSetItemData (hLB, lngNewItem, CLong(saClientIDs[i]));

					// Re-select the current item if it was previously selected.
					if (InStr(strOldSel, CSR(Chr(0), strMachineUser, Chr(0))) > 0)
					{
						lbSelectMulti (hLB, lngNewItem, true);
					}
				}

				// Set local flag that the list is up-to-date again.
				signal_cds_update = false;
			}

			// Leave CS.
			cs.Leave();
		}
	}
	else if (msg == WM_COMMAND)
	{
		long id = LOWORD(wparam);
		long msg = HIWORD(wparam);

		// Check if something has been clicked ...
		if (msg == BN_CLICKED)
		{
			// Check if a relevant button was clicked ...
			if ((id == IDC_RESTART) || 
				(id == IDC_SHUTDOWN) ||
				(id == IDC_LOGOFF) || 
				(id == IDC_GPUPDATE) ||
				(id == IDC_SENDMSG))
			{
				//
				// Note: One of the action buttons has been pressed.
				//

				// Variables.
				HWND hLB = dia(hwnd, IDC_LB);
				string strMessage;
				string strActionGUIText;
				string strNetIDList;
				long lngSelCount;
				long lngArraySelItems[MAX_LISTBOX_ITEMS];

				bool fail_success = false;
				string strError;

				//
				// (!) CS needed: Avoid background updates of the CDS class while the GUI is using it.
				//
				// Enter CS.
				cs.Enter();

				// Build Net ID List.
				lngSelCount = lbGetSelItems(hLB, lngArraySelItems, MAX_LISTBOX_ITEMS);
				for (long i = 0; i < lngSelCount; i++)		// zero-based return array
				{
					// Store a string with all selected machine and user names.
					strNetIDList += CSR("|", lbGetItemData(hLB, lngArraySelItems[i]));
				}

				// Leave CS.
				cs.Leave();

				// Get Message entered by the user.
				strMessage = ctlGetText(dia(hwnd, IDC_MESSAGE));

				// Check if NetIDList is empty.
				if (strNetIDList.length() == 0)
				{
					InfoBox (hwnd, "Fehler: Sie haben keine Computer und Benutzer ausgewählt.", 
									NETMSG_CAPTION, MB_OK | MB_ICONEXCLAMATION);
				}
				else if ((id == IDC_SENDMSG) && (strMessage.length() == 0))
				{
					// FAIL.
					InfoBox (hwnd, "Fehler: Sie müssen zuerst eine Textnachricht eingeben.", 
									NETMSG_CAPTION, 
									MB_OK | MB_ICONEXCLAMATION);
				}
				else
				{
					// Generate text out of selected action.
					if (id == IDC_RESTART)	{ strActionGUIText = (LANG_EN ? "REBOOT" : "NEUSTART"); };
					if (id == IDC_SHUTDOWN)	{ strActionGUIText = (LANG_EN ? "SHUTDOWN" : "HERUNTERFAHREN"); };
					if (id == IDC_LOGOFF)	{ strActionGUIText = (LANG_EN ? "LOGOFF" : "ABMELDEN"); };
					if (id == IDC_GPUPDATE)	{ strActionGUIText = (LANG_EN ? "GPUPDATE" : "GPO-UPDATE"); };
					if (id == IDC_SENDMSG)	{ strActionGUIText = (LANG_EN ? "MESSAGE DISPLAY" : "NACHRICHTEN-ANZEIGE"); };

					// Ask User if the clicked command should be really issued to the selected client machines.
					if (MessageBox(hwnd, CSR("Möchten Sie die Aktion \"", strActionGUIText,
											   "\" wirklich auf den ausgewählten Clients ausführen?"),
													"Sicherheitsabfrage", MB_ICONQUESTION | MB_YESNO) == IDYES)
					{
						//
						// USER CONFIRMED.
						//
						
						// Check which button was clicked.
						if (id == IDC_RESTART)
						{
							fail_success = (UI_Send_Command(CSR(Chr(NMSG_VCOMM_INVOKE_RESTART), Chr(0), strNetIDList, Chr(0), " ")));
						}
						else if (id == IDC_SHUTDOWN)
						{
							fail_success = (UI_Send_Command(CSR(Chr(NMSG_VCOMM_INVOKE_SHUTDOWN), Chr(0), strNetIDList, Chr(0), " ")));
						}
						else if (id == IDC_LOGOFF)
						{
							fail_success = (UI_Send_Command(CSR(Chr(NMSG_VCOMM_INVOKE_LOGOFF), Chr(0), strNetIDList, Chr(0), " ")));
						}
						else if (id == IDC_GPUPDATE)
						{
							fail_success = (UI_Send_Command(CSR(Chr(NMSG_VCOMM_INVOKE_GPUPDATE), Chr(0), strNetIDList, Chr(0), " ")));
						}
						else if (id == IDC_SENDMSG)
						{
							fail_success = (UI_Send_Command(CSR(Chr(NMSG_VCOMM_INVOKE_MESSAGE), Chr(0), strNetIDList, Chr(0), strMessage)));
						}

						// Notify the user if the operation succeeded.
						InfoBox (hwnd, (fail_success ? "Der Befehl wurde erfolgreich an die ausgewählten Clients und Benutzer versandt." :
													   "Beim Senden des Befehls an die ausgewählten Clients und Benutzer ist ein Fehler aufgetreten."
													   ), NETMSG_CAPTION, 
															MB_OK | (fail_success ? MB_ICONINFORMATION : MB_ICONEXCLAMATION));
					}
				}
			}
		}
	}
	else if (msg == WM_CLOSE)
	{
		// Notify Server that the admin logged off.
		// Note: This can be achieved by submitting a wrong password, so the server will invalide the admin session.
		UI_Send_Command(CSR(Chr(NMSG_VCOMM_ADMIN_PASSWORD), Chr(0)));

		// Stop Timers.
		KillTimer (hwnd, LISTUPDATE_TIMER_ID);

		// End dialog upon user request.
		EndDialog (hwnd, 0);
	}
	return false;
}



//////////////////////////////
// Systray Menu Related		//
//////////////////////////////
void NetMsg_Client::ShowSystrayMenu ()
{
	// Variables.
	MegaPopup spm;
	POINT pt;
	long hv;
	bool bUserIsAdminMember;

	// Check current users elevation first.
	bUserIsAdminMember = ((GetCurrentUsersElevation() == TokenElevationTypeLimited) ||
								(GetCurrentUsersElevation() == TokenElevationTypeFull));
	
	// Prepare Popup Menu.
	// Add admin-only menu commands.
	spm.Add (1, "Öffne Netzwerk-Nachrichtendienst", L_IDI_NETMSG_NEWMSG, MPM_HILITEBOX | (MPM_DISABLED * !bUserIsAdminMember));
	
	// Show Popup Menu.
	GetCursorPos (&pt);
	hv = spm.Show (pt.x, pt.y, MPM_RIGHTBOTTOM, 0);
	
	// Act depending on menu entry selection.
	if (hv == 1)
	{
		// Perform the Password Checking Dialog.
		if (DialogBoxParam (myinst, (LPCTSTR) IDD_NETMSG_CHECKPW, 0, &_ext_NetMsg_Client_Callback_EnterPW_WndProc, (LONG_PTR) this) == ENTERPWRES_UNLOCK)
		{
			// Passwork OK, now perform the NetMsg Main Dialog.
			if (DialogBoxParam (myinst, (LPCTSTR) IDD_NETMSG_MAIN, 0, &_ext_NetMsg_Client_MainDlg_WndProc, (LONG_PTR) this) == 0)
			{
				// Dialog finished
			}
		}
	}
	
	return;
}


//////////////////////////
// (!) Thread Interface //
//////////////////////////
bool NetMsg_Client::UI_Send_Command (string strCommand)
{
	//
	// (!) Careful: Calling this function from the CLIENT_VM Thread can lead to DEADLOCK
	//
	DWORD dwTicks;

	// Timeout send data loop.
	dwTicks = GetTickCount();
	while (!client.connected() && 
			!signal_client_shutdown && 
			!(GetTickCount() > (dwTicks + CONNECTION_TIMEOUT)) ) 
	{ 
		Sleep (50); 
	}

	// Return if the operation succeeded.
	return (client.mod_send(strCommand));
}

