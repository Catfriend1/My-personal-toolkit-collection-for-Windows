// Boot.h: Declares everything needed to use the Boot.cpp module
//
// Thread-Safe:	NO
//


//////////////
// Includes	//
//////////////
#include "SystrayPlayer/SystrayPlayer.h"


//////////////
// Consts	//
//////////////
extern const char WINSUITE_CAPTION[];


//////////////
// Globals	//
//////////////
extern HINSTANCE myinst;

extern HICON L_IDI_WINSUITE;

// Globals: Syslog
extern long REGV_SYSLOG_ENABLE;
extern long REGV_SYSLOG_LOG_ENABLE;
extern long REGV_SYSLOG_LOG_ALL;

// Globals: NetMsg (Network Messenger)
extern long REGV_NETMSG_SERVER;
extern long REGV_NETMSG_CLIENT;
extern string REGV_NETMSG_SERVER_PASSWORD;
extern string REGV_NETMSG_SERVER_ADDRESS;

// Shared Instance: SystrayPlayer
extern SystrayPlayer* mod_systrayplayer; 


//////////////////////////
// Icons: Autoruns [3]	//
//////////////////////////
extern HICON L_IDI_REGTOOL, L_IDI_REGTOOL16, L_IDI_RESTART32;


//////////////////////////////
// Icons: NTProtect [1]		//
//////////////////////////////
extern HICON L_IDI_APPS;


//////////////////////////////
// Fonts: NTProtect [1]		//
//////////////////////////////
extern HFONT FONT_MSSANS_BOLD;


//////////////////////
// Init/Terminate	//
//////////////////////
long DebugHelper_ntService_Thread (long);
void TerminateAll ();
bool TerminateVKernel_Allowed ();
bool TerminateVKernel (bool);


//////////////////////////
// VK2 Window Process	//
//////////////////////////
LRESULT CALLBACK VK2_WndProc (HWND, UINT, WPARAM, LPARAM);



/////////////////////////////////////////////////////////////////////////
// SETUP VECTOR START HERE		/////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
class Setup
{
private:
	// Critical Section Object
	CriticalSection cs;

	// Cache
	RegObj* reg;
	RegObj* reg_ib;
	RegObj* reg_ntp;
	RegObj* reg_spl;

	// Cache: Setup Dialog Window
	HWND setupwnd;
	long cnt_selected_components;

	// Functions: Internal Component Counter Helper
	long Count (long);

public:
	// Functions: Constructor, Destructor
	Setup();
	~Setup();

	// Functions: Callback
	INT_PTR CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);

protected:
	static INT_PTR CALLBACK _ext_Setup_WndProc (HWND, UINT, WPARAM, LPARAM);
};


