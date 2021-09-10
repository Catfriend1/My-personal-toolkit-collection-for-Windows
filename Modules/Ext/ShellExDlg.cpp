// ShellEx.cpp: The Intelligent ShellExecute Dialog as Class Implementation
//
// Thread-Safe: NO
//


//////////////////
// Includes		//
//////////////////
#include "stdafx.h"
#include "../../resource.h"

#include "../_modules.h"

#include "../../Boot.h"
#include "ShellExDlg.h"


//////////////
// Consts	//
//////////////
const char dlg_ShellExecute::REG_RUNMRU[]	= "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU";


//////////////////////////////////////////////////////
// Use this function to Start the ShellEx Dialog	//
//////////////////////////////////////////////////////
void ShellExDialog()
{
	// Create Dialog Class
	dlg_ShellExecute* newdlg = new dlg_ShellExecute;
}


//////////////////////////////
// Class: Init/Terminate	//
//////////////////////////////
dlg_ShellExecute::dlg_ShellExecute ()
{
	// Init RAM
	ibox = 0;

	// Load Resources
	ICON_SOFTWAREBIG = LoadIconRes (myinst, IDI_SOFTWAREBIG, 32, 32);

	// Init Filesearch Filter
	if (LANG_EN)
	{
		FILESEARCH_FILTER = CSR(CSR(CreateFilter("Programs", "*.EXE;*.COM;*.BAT;*.CMD"), 
							CreateFilter("Documents", "*.TXT;*.DOC;*.WRI;*.RTF;*.PDF"), 
							CreateFilter("Pictures", "*.BMP;*.PCX;*.GIF;*.JPG;*.PNG"), 
							CreateFilter("Audio- & Videofiles", "*.WAV;*.MP3;*.AVI;*.MPG;*.WMV;*.FLV;*.MP4"), 
							CreateFilter("All Files", "*.*")), 
							Chr(0));
	}
	else
	{
		FILESEARCH_FILTER = CSR(CSR(CreateFilter("Programme", "*.EXE;*.COM;*.BAT;*.CMD"), 
							CreateFilter("Dokumente", "*.TXT;*.DOC;*.WRI;*.RTF;*.PDF"), 
							CreateFilter("Bilder", "*.BMP;*.PCX;*.GIF;*.JPG;*.PNG"), 
							CreateFilter("Musik- und Videodateien", "*.WAV;*.MP3;*.AVI;*.MPG;*.WMV;*.FLV;*.MP4"), 
							CreateFilter("Alle Dateien", "*.*")), 
							Chr(0));
	}
	
	// Start Dialog Thread
	TVM.NewThread (&_ext_ShellEx_Thread, (void*) this);
}

long dlg_ShellExecute::_ext_ShellEx_Thread (dlg_ShellExecute* class_ptr)
{
	HWND nullwnd;
	
	// Create Null Parent Window
	nullwnd = CreateNullWindow ("*ShellExDlg");
	
	// Start the Dialog
	DialogBoxParam (myinst, (LPCTSTR) IDD_SHXDIALOG, nullwnd, &_ext_ShellEx_WndProc, (LONG_PTR) class_ptr);
	
	// Destroy Null Parent Window
	DestroyWindow (nullwnd);
	
	// Delete Dialog Class
	delete class_ptr;
	
	// Terminate Thread
	TVM.RemoveThread();
	return -110;
}

dlg_ShellExecute::~dlg_ShellExecute ()
{
	// Free Resources
	DestroyIcon (ICON_SOFTWAREBIG);
}


//////////////////////
// Window Process	//
//////////////////////
INT_PTR CALLBACK dlg_ShellExecute::_ext_ShellEx_WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	dlg_ShellExecute* dex = (dlg_ShellExecute*) GetWindowLongPtr (hwnd, GWLP_USERDATA);
	if (dex != 0) { return dex->WndProc (hwnd, msg, wparam, lparam); };
	if (msg == WM_INITDIALOG)
	{
		// lparam=InitParameter=ClassPointer
		SetWindowLongPtr (hwnd, GWLP_USERDATA, lparam);
		return _ext_ShellEx_WndProc (hwnd, msg, wparam, lparam);
	}
	return false;
}

bool dlg_ShellExecute::WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_COMMAND)
	{
		long id = LOWORD(wparam);
		long msg = HIWORD(wparam);

		if (msg == BN_CLICKED)
		{
			// Message for the parent
			if (id == IDC_OK)
			{
				string txt = cbAbsText(dia(dwnd, IDC_CBOX));
				HINSTANCE res = 0;
				
				// If it is not a Null String, call it
				if (xlen(txt) != 0)
				{
					res = ShellExecute (dwnd, txt);

					// Close dialog if no error occured
					if (res <= (HINSTANCE) 32)
					{
						// (<= 32): Error occured
						InfoBox (dwnd, CSR(LANG_EN ? "The Application could not be started: " : "Die Anwendung konnte nicht gestartet werden: ", FormatMsg(GetLastError())), "Start Application", MB_OK | MB_ICONSTOP);
					}
					else
					{
						// Success - Save it Windows Explorer compatible to the registry and end the dialog
						if (!IsComboCall(txt))			// Only if it was a new item
						{
							string mru; regRead (&mru, HKCU, REG_RUNMRU, "MRUList");
							unsigned char ascfree = 0;
							for (int asc = 97; asc <= 122; asc++)		// a-z
							{
								if (InStrFirst(mru, asc) == 0)			// Letter not used
								{
									ascfree = asc;
									break;
								}
							}
							if (ascfree == 0)		// All letters are already in use
							{
								string item = Right(mru, 1);
								mru = CSR(item, Left(mru, xlen(mru) - 1));	// "abcd" -> "dabc"
								regWrite (CSR(txt, "\\1"), HKCU, REG_RUNMRU, item);
								regWrite (mru, HKCU, REG_RUNMRU, "MRUList");
							}
							else
							{
								regWrite (CSR(txt, "\\1"), HKCU, REG_RUNMRU, Chr(ascfree));
								regWrite (CSR(Chr(ascfree), mru), HKCU, REG_RUNMRU, "MRUList");
							}
						}
						EndDialog (dwnd, true);
					}
				}
			}
			else if (id == IDC_BROWSE)
			{
					string file = cdlgOpenFile (dwnd, "Application- & Documentsearch", 
												ShellFolder(0, CSIDL_DESKTOPDIRECTORY), 
												FILESEARCH_FILTER, 1);
					if (xlen(file) != 0)		// OK / Success
					{
						// Check if filename is LFN
						if (UCase(file) != UCase(CreateDOS83(file)))
						{
							file = CSR(Chr(34), file, Chr(34));
						}
						
						ctlSetText (dia(dwnd, IDC_CBOX), file);
						UpdateStatusIcon();
					}
			}
			else if ((id == IDCANCEL) || (id == IDC_CANCEL))
			{
				EndDialog (hwnd, false);
			}
		}
		else if (id == IDC_CBOX)		// Message for my Combo Box
		{
			if ((msg == CBN_SELCHANGE) || (msg == CBN_EDITCHANGE))
			{
				UpdateStatusIcon();
			}
		}
	}
	else if (msg == WM_INITDIALOG)
	{
		dwnd = hwnd;
		
		// Align Dialog Window
		RECT rc, wa;
		GetClientRect (hwnd, &rc);
		SystemParametersInfo (SPI_GETWORKAREA, 0, &wa, 0);
		SetWindowPos (hwnd, 0, wa.right - (rc.right - rc.left) - 5,
								wa.bottom - (rc.bottom - rc.top) - 25, 
								0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
		ibox = new IconBox (hwnd, 11, 13, 32, 32, ICON_SOFTWAREBIG);

		// Set Texts
		ctlSetText (hwnd, "Programm ausführen");
		ctlSetText (dia(hwnd, IDL_TEXT), LANG_EN ? "Please enter the full path and filename of a program to run\nor simply use a \"ShellExecute\" command. The icon on the\nleft indicates if your call had been successfully resolved." : 
													"Bitte geben Sie den vollständigen Pfad und Dateinamen zum Programmstart ein oder benutzen Sie ein sogenanntes \"ShellExecute\" Kommando. Das Symbol auf der linke Seite zeigt Ihnen, ob der Programmaufruf vom System erkannt wurde.");
		ctlSetText (dia(hwnd, IDL_CALL), LANG_EN ? "Call" : "Öffnen");
		ctlSetText (dia(hwnd, IDC_CANCEL), LANG_EN ? "&Cancel" : "Abbre&chen");
		ctlSetText (dia(hwnd, IDC_BROWSE), LANG_EN ? "&Browse" : "&Durchsuchen");

		// Fill Combo Box
		string mru, item; regRead (&mru, HKCU, REG_RUNMRU, "MRUList");
		cbSetHeight (dia(dwnd, IDC_CBOX), 160);
		for (long i = 1; i <= xlen(mru); i++)
		{
			regRead (&item, HKCU, REG_RUNMRU, Mid(mru, i, 1));
			if (xlen(item) != 0)		// Valid Item
			{
				cbAdd (dia(dwnd, IDC_CBOX), Left(item, xlen(item) - 2));
			}
		}

		// Return Init Success
		return true;
	}
	else if (msg == WM_CLOSE)
	{
		// End Dialog upon close request.
		EndDialog (hwnd, false);
	}
	else if (msg == WM_DESTROY)
	{
		if (ibox != 0)
		{
			delete ibox;
			ibox = 0;
		}
	}
	return false;
}

bool dlg_ShellExecute::IsComboCall (string call)
{
	long u = cbCount(dia(dwnd, IDC_CBOX));
	bool ret = false;
	for (long i = 1; i <= u; i++)
	{
		if (call == cbText(dia(dwnd, IDC_CBOX), i - 1))
		{
			ret = true;
			break;
		}
	}
	return ret;
}

void dlg_ShellExecute::UpdateStatusIcon ()
{
	string path, file, param;
	string call = cbAbsText(dia(dwnd, IDC_CBOX));
	HICON icon = 0;
	
	// Resolve the call to a file or folder and get its icon
	if (Left(call, 2) != "\\\\")
	{
		ShellExResolve (call, &path, &file, &param);
		icon = GetFileIcon (CSR(path, "\\", file), true);
	}
	
	// Display the new icon
	if (ibox != 0)
	{
		if (icon == 0)
		{
			ibox->DrawIcon (ICON_SOFTWAREBIG);
		}
		else
		{
			ibox->DrawIcon (icon);
			DestroyIcon (icon);
		}
	}
}


