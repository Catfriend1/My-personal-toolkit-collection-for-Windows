// ActiveMovie.cpp: Adds support for the ActiveMovie Control
// Version: 2010-02-19
//
// Thread-Safe: YES
//


//////////////////////
// Instructions		//
//////////////////////
/*
Main Module:	#include "objbase.h"
				needs to call:	CoInitializeEx (0, COINIT_APARTMENTTHREADED);
								CoUninitialize ();

Compiler:		Requires switch: _WIN32_DCOM
				#include "strmif.h"
				#include "Control.h"

Linker:			Requires "strmiids.lib"
*/


//////////////////
// Includes		//
//////////////////
#include "stdafx.h"

#include "_modules.h"


//////////////
// Macros	//
//////////////
#define FreeInterface(ifx) { if (ifx != 0) { ifx->Release(); ifx = 0; }; }


//////////////
// Consts	//
//////////////
const long VOLUME_SILENT		= -3100;			// [-3100]


//////////////////////////
// Init / Terminate		//
//////////////////////////
ActiveMovie::ActiveMovie ()
{
	// Initialize the main object
	amvobj[0] = 0;
	
	// Initialize the 7 controls
	mediactrl[0] = 0;
	mediaevent[0] = 0;
	mediaeventex[0] = 0;
	mediaseek[0] = 0;
	mediapos[0] = 0;
	
	videownd[0] = 0;
	basicvideo[0] = 0;
	basicaudio[0] = 0;
}

ActiveMovie::~ActiveMovie ()
{
	Close (0);
}


//////////////////////////////
// Instance	Management		//
//////////////////////////////
long ActiveMovie::Open (string file, HWND notifywnd, long message, long param)					// public
{
	long res;

	// Enter Critical Section
	cs.Enter();
	
	// Close all interfaces if necessary
	Close();
	
	// Re-create the main object and then the 7 controls
	if (CoCreateInstance(CLSID_FilterGraph, 0, CLSCTX_INPROC_SERVER, 
								IID_IGraphBuilder, (void**) &amvobj[0]) == S_OK)
	{
		// Init DirectShow interfaces
		amvobj[0]->QueryInterface(IID_IMediaControl, (void**) &mediactrl[0]);
		amvobj[0]->QueryInterface(IID_IMediaEvent, (void**) &mediaevent[0]);
		amvobj[0]->QueryInterface(IID_IMediaEventEx, (void**) &mediaeventex[0]);
		amvobj[0]->QueryInterface(IID_IMediaSeeking, (void**) &mediaseek[0]);
		amvobj[0]->QueryInterface(IID_IMediaPosition, (void**) &mediapos[0]);
		
		// Init Video interfaces
		amvobj[0]->QueryInterface(IID_IVideoWindow, (void**) &videownd[0]);
		amvobj[0]->QueryInterface(IID_IBasicVideo, (void**) &basicvideo[0]);
		
		// Init Audio interfaces
		amvobj[0]->QueryInterface(IID_IBasicAudio, (void**) &basicaudio[0]);
		
		// Open the specified file
		UNISTR tf = file;
		res = amvobj[0]->RenderFile ((LPCWSTR) &tf[0], 0);
		
		// Set Callback if necessary
		if (notifywnd != 0)
		{
			SetCallback (notifywnd, true, message, param);
		}

		// Leave Critical Section before return
		cs.Leave();
		return res;
	}
	else
	{
		amvobj[0] = 0;

		// Leave Critical Section before return
		cs.Leave();
		InfoBox (0, "ActiveMovie::CoCreateInstance failed !", "", MB_OK | MB_SYSTEMMODAL | MB_ICONSTOP);
		return E_FAIL;
	}
}

void ActiveMovie::Close (HWND notifywnd)					// public
{
	// Enter Critical Section
	cs.Enter();

	// If the was a notify window defined, remove this callback before continuing with unload
	if (notifywnd != 0)
	{
		SetCallback (notifywnd, false, 0, 0);
	}
	Stop();
	
	// Release the 7 controls
	FreeInterface (mediaeventex[0]);
	FreeInterface (mediaevent[0]);
	FreeInterface (mediaseek[0]);
	FreeInterface (mediapos[0]);
	
	FreeInterface (basicvideo[0]);
	FreeInterface (videownd[0]);
	FreeInterface (basicaudio[0]);
	
	FreeInterface (mediactrl[0]);
	
	// Release the main object
	FreeInterface (amvobj[0]);

	// Leave Critical Section
	cs.Leave();
}


//////////////////////
// User Interface	//
//////////////////////
void ActiveMovie::Play ()											// public
{
	cs.Enter();
	if (mediactrl[0] != 0) { mediactrl[0]->Run(); };
	cs.Leave();
}

void ActiveMovie::Pause ()											// public
{
	cs.Enter();
	if (mediactrl[0] != 0) { mediactrl[0]->Pause(); };
	cs.Leave();
}

void ActiveMovie::Stop ()											// public
{
	cs.Enter();
	if (mediactrl[0] != 0) { mediactrl[0]->Stop(); };
	cs.Leave();
}

void ActiveMovie::Home ()											// public
{
	cs.Enter();
	SetPosition(0);
	cs.Leave();
}


//////////////////////////
// Stream Information	//
//////////////////////////
double ActiveMovie::GetPosition ()									// public
{
	double pos;

	if (mediapos[0] == 0) { return 0; };

	// Enter Critical Section
	cs.Enter();

	// Get Position
	if (mediapos[0]->get_CurrentPosition(&pos) == S_OK)
	{
		// Leave Critical Section before return
		cs.Leave();
		return pos;
	}
	else
	{
		// Leave Critical Section before return
		cs.Leave();
		return 0;
	}
}

void ActiveMovie::SetPosition (double pos)							// public
{
	if (mediapos[0] == 0) { return; };

	cs.Enter();
	mediapos[0]->put_CurrentPosition (pos);
	cs.Leave();
}

double ActiveMovie::GetRate ()										// public
{
	double rate;

	if (mediapos[0] == 0) { return 1; };

	// Enter Critical Section
	cs.Enter();

	// Get Rate
	if (mediapos[0]->get_Rate(&rate) == S_OK)
	{		
		// Leave Critical Section before return
		cs.Leave();
		return rate;
	}
	else
	{
		// Leave Critical Section before return
		cs.Leave();
		return 1;
	}

}

void ActiveMovie::SetRate (double newrate)							// public
{
	cs.Enter();
	if (mediapos[0] != 0) {	mediapos[0]->put_Rate (newrate); };
	cs.Leave();
}

long ActiveMovie::GetVolume ()										// public
{
	long volume;

	if (basicaudio[0] == 0) { return 0; };

	// Enter Critical Section
	cs.Enter();

	// Get Volume
	if (basicaudio[0]->get_Volume(&volume) == S_OK)
	{
		// Leave Critical Section before return
		cs.Leave();
		return volume;
	}
	else
	{
		// Leave Critical Section before return
		cs.Leave();
		return 0;
	}
}

void ActiveMovie::SetVolume (long newvol)							// public
{
	cs.Enter();
	if (basicaudio[0] != 0) { basicaudio[0]->put_Volume (newvol); };
	cs.Leave();
}

double ActiveMovie::GetLength ()									// public
{
	double length;

	if (mediapos[0] == 0) { return 0; };

	// Enter Critical Section
	cs.Enter();

	// Get Length
	if (mediapos[0]->get_Duration(&length) == S_OK)
	{
		// Leave Critical Section before return
		cs.Leave();
		return length;
	}
	else
	{
		// Leave Critical Section before return
		cs.Leave();
		return 0;
	}
}


//////////////////////
// Event Handling	//
//////////////////////
void ActiveMovie::SetCallback (HWND hwnd, bool enable, long message, long param)			// public
{
	if (mediaeventex[0] == 0) { return; };

	cs.Enter();
	if (enable)
	{
		mediaeventex[0]->SetNotifyWindow ((OAHWND) hwnd, message, param);
	}
	else
	{
		mediaeventex[0]->SetNotifyWindow (0, 0, 0);
	}
	cs.Leave();
}

long ActiveMovie::GetEvent (long* evcode, LONG_PTR* param1, LONG_PTR* param2)						// public
{
	if (mediaevent[0] == 0) { return E_FAIL; };

	// Enter Critical Section
	cs.Enter();

	// Get Event
	if (mediaevent[0]->GetEvent(evcode, param1, param2, 100) == S_OK)
	{
		// Leave Critical Section before return
		cs.Leave();
		return S_OK;
	}
	else
	{
		// Leave Critical Section before return
		cs.Leave();
		return E_FAIL;
	}
}

void ActiveMovie::FreeEvent (long evcode, long param1, long param2)							// public
{
	if (mediaevent[0] == 0) { return; };

	cs.Enter();
	mediaevent[0]->FreeEventParams (evcode, param1, param2);
	cs.Leave();
}


//////////////////////////////
// VideoWindow Related		//
//////////////////////////////
void ActiveMovie::SetVideoWindowParent (HWND hwnd)											// public
{
	cs.Enter();
	if (videownd[0] != 0) { videownd[0]->put_Owner ((OAHWND) hwnd); };
	cs.Leave();
}

void ActiveMovie::SetMsgTarget (HWND hwnd)													// public
{
	cs.Enter();
	if (videownd[0] != 0) { videownd[0]->put_MessageDrain ((OAHWND) hwnd); };
	cs.Leave();
}

void ActiveMovie::SetFullscreen (bool fsmode)												// public
{
	if (videownd[0] == 0) { return; };

	cs.Enter();
	videownd[0]->put_FullScreenMode (fsmode ? 0xFFFFFFFF : 0);
	cs.Leave();
}

void ActiveMovie::ShowWindow (bool bshow)													// public
{
	if (videownd[0] == 0) { return; };
	
	cs.Enter();
	videownd[0]->put_Visible (bshow ? 0xFFFFFFFF : 0);
	cs.Leave();
}

void ActiveMovie::SetAutoShow (bool autoshow)												// public
{
	if (videownd[0] == 0) { return; };

	cs.Enter();
	videownd[0]->put_AutoShow (autoshow ? 0xFFFFFFFF : 0);
	cs.Leave();
}

void ActiveMovie::SetWindowPosition (long x, long y, long w, long h)						// public
{
	cs.Enter();
	if (videownd[0] != 0) { videownd[0]->SetWindowPosition(x,y,w,h); };
	cs.Leave();
}

long ActiveMovie::GetWndStyle ()															// public
{
	long style = 0;

	cs.Enter();
	if (videownd[0] != 0) { videownd[0]->get_WindowStyle(&style); };
	cs.Leave();

	return style;
}

void ActiveMovie::SetWndStyle (long style)													// public
{
	cs.Enter();
	if (videownd[0] != 0) { videownd[0]->put_WindowStyle(style); };
	cs.Leave();
}




