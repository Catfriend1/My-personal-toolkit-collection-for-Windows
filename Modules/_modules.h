// _Modules.h: Declares everything needed to use my own modules
//

//
// Link with: ws2_32.lib Mpr.lib wininet.lib rasapi32.lib strmiids.lib 
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __MODULES_H
#define __MODULES_H


	// ---------- Windows API / SDK Includes ------------
	// StructArray Related
	#include "oaidl.h"

	// String Related
	#include "WinBase.h"

	// File System Related
	#include "shlobj.h"
	#include "commdlg.h"

	// Process Related
	#include "tlhelp32.h"
	#include "sddl.h"

	// Windows Shell
	#include "shellapi.h"
	#include "shlobj.h"

	// Common Controls
	#include "commctrl.h"

	// Active Movie
	#include "strmif.h"
	#include "Control.h"
	#include "Uuids.h"

	// Windows Sockets II
	#include "winsock2.h"
	#include "wininet.h"
	#include "ras.h"
	// InetPton
	#include "ws2tcpip.h"




	// ---------- Custom Modules ------------
	#include "StructArray.h"
	#include "TheString.h"
	#include "GDI.h"
	#include "MegaPopup.h"
	#include "Registry.h"
	#include "FileSystem.h"
	#include "Process.h"
	#include "VbStd.h"
	#include "ShellNotify.h"
	#include "ControlUI.h"
	#include "Comctls.h"

	#include "ActiveMovie.h"
	#include "Inifile.h"
	#include "CFSLogger.h"
	#include "Wsock2.h"
	#include "CSendKeys.h"


	//////////////////////////////
	// Language Definition		//
	//////////////////////////////
	#ifdef _LANG_EN
		#define LANG_EN true
	#else
		#define LANG_EN	false
	#endif


#endif
