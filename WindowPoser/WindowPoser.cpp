// WindowPoser.cpp: WindowPoser cares about windows not spawning under the taskbar when being shown by other programs
//
// Thread-Safe: NO
// 


//////////////////
// Includes		//
//////////////////
#include "stdafx.h"
#include "math.h"
#include "../resource.h"

#include "../Modules/_modules.h"

#include "../Boot.h"								// << WINSUITE_CAPTION[]
#include "WindowPoser.h"


//////////////////////////////
// Consts: Class Related	//
//////////////////////////////
const char WindowPoser::CAPTION[]					= 	
														#ifdef _LANG_EN 
															"Window Poser";
														#else 
															"Fensterpositionierer" ;
														#endif;

const char WindowPoser::CAPTION_START_BUTTON[]		= "Start";

const long WindowPoser::FS_PIXEL_TOLERANCE			= 5;
const long WindowPoser::UPDATE_TIMER				= 5000;												// [5000]
const long WindowPoser::THREAD_RES					= -104;


//////////////////////////////////
// Functions: Init / Terminate	//
//////////////////////////////////
WindowPoser::WindowPoser()
{
	// Init RAM
	wposer_thread = 0;

	// Spawn traffic detection Thread
	WPOSER_VM.NewThread (&_ext_WindowPoser_WorkerThread, (void*) this, &wposer_thread);
}

WindowPoser::~WindowPoser()
{
	// Class Destructor.
	Terminate();
}

void WindowPoser::Terminate ()																					// private
{
	if (wposer_thread != 0)
	{
		// Send the Traffic Detection Thread a shutdown signal
		PostThreadMessage (wposer_thread, WM_QUIT, THREAD_RES, 0);

		// Wait for Traffic Detection Thread to finish
		WPOSER_VM.AwaitShutdown();
	}

	// UnInit complete
}

long WindowPoser::_ext_WindowPoser_WorkerThread (WindowPoser* class_ptr)
{
	if (class_ptr != 0) 
	{ 
		return class_ptr->WorkerThread(); 
	} 
	else 
	{ 
		return -1; 
	}
}

long WindowPoser::WorkerThread ()
{
	RECT screenrc, workarea, wrc;
	HWND hwnd_program_manager;
	EnumApps ea;
	MSG msg;
	long i, u;

	// Wait
	Sleep (300);

	// Start WindowCheck Timer
	UINT_PTR hUpdateTimer = SetTimer (NULL, 1, UPDATE_TIMER, 0);

	// Thread Message Queue
	while (true)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_TIMER)
			{
				static bool locked = false;

				// Avoid Loopback
				if (!locked)
				{
					// Lock
					locked = true;

					// Kill the Timer.
					KillTimer (NULL, hUpdateTimer);

					// Get Current Screen RECT
					screenrc.left = 0;
					screenrc.top = 0;
					screenrc.right = ScreenWidth();				// OK like this - Fullscreen-Apps mostly prefer primary monitor!
					screenrc.bottom = ScreenHeight();			// OK like this - Fullscreen-Apps mostly prefer primary monitor!

					// Prepare needed System Environment Info
					hwnd_program_manager = ExplorerWindow();

					// Enum currently present Windows
					ea.Clear();
					ea.Enum (wmaText);
					
					// Loop through all VISIBLE Windows
					u = ea.Ubound();
					for (i = 1; i <= u; i++)
					{
						// Check if Window is Visible and Not Minimized to the Taskbar and Not Maximized 
						// and Not the "Program Manager"'s Window and Not the "Start Button"'s Window and
						// Not WS_EX_TOPMOST styled
						if (
									IsWindowVisible(ea.hwnd(i)) && 
									(!StyleExists(ea.hwnd(i), WS_MINIMIZE)) && 
									(!StyleExists(ea.hwnd(i), WS_MAXIMIZE)) &&
									(!ExStyleExists(ea.hwnd(i), WS_EX_TOPMOST)) &&
									(ea.hwnd(i) != hwnd_program_manager) &&
									(UCase(ea.name(i)) != UCase(CAPTION_START_BUTTON)) && 
									(UCase(ea.name(i)) != UCase(WINSUITE_CAPTION))
																											)
						{
							if (GetWindowRect(ea.hwnd(i), &wrc) && GetWorkareaForWindow(ea.hwnd(i), &workarea))
							{
								//
								// BEGIN PERFORMING CHECKS ON CURRENT EA.HWND(i)'s COORDINATES.
								//
								
								//
								// Check if this could be a fullscreen window, e.g. from a game application
								//
								if (((long) fabs((long double) screenrc.left - wrc.left) < FS_PIXEL_TOLERANCE) && 
									((long) fabs((long double) screenrc.top - wrc.top) < FS_PIXEL_TOLERANCE) &&
									((long) fabs((long double) screenrc.right - wrc.right) < FS_PIXEL_TOLERANCE) &&
									((long) fabs((long double) screenrc.bottom - wrc.bottom) < FS_PIXEL_TOLERANCE))
								{
									// We will now assume a fullscreen window and not check it against the workarea
									#ifdef _DEBUG
										OutputDebugString (&CSR("Detected FullScreen App: ", ea.name(i), "\n")[0]);
									#endif

									// Continue FOR
									continue;
								}

								// Check if the window coordinates lie (correctly) in the current work area
								// (!) AVOID LOOPBACK: Only apply ONE fix at a time for X and Y coordinate
								//
								// Check X
								if (wrc.left < workarea.left)
								{
									// Left X coordinate failed, check if a fix will work with the current window size
									if (workarea.left + (wrc.right - wrc.left) < workarea.right)
									{
										SetWindowPos (ea.hwnd(i), 0, workarea.left, wrc.top, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
									}

									#ifdef _DEBUG
										OutputDebugString (&CSR("X (left=", CStr(wrc.left), ") failed for: ", ea.name(i), "\n")[0]);
									#endif
								}
								else if (wrc.right > workarea.right)
								{
									// Right X coordinate failed, check if a fix will work with the current window size
									if (workarea.right - (wrc.right - wrc.left) > workarea.left)
									{
										SetWindowPos (ea.hwnd(i), 0, workarea.right - (wrc.right - wrc.left), wrc.top, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
									}

									#ifdef _DEBUG
										OutputDebugString (&CSR("X (right=", CStr(wrc.right), ") failed for: ", ea.name(i), "\n")[0]);
									#endif
								}
									
								// Check Y
								if (wrc.top < workarea.top)
								{
									// Upper Y coordinate failed, check if a fix will work with the current window size
									if (workarea.top + (wrc.bottom - wrc.top) < workarea.bottom)
									{
										SetWindowPos (ea.hwnd(i), 0, wrc.left, workarea.top, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
									}

									#ifdef _DEBUG
										OutputDebugString (&CSR("Y (top=", CStr(wrc.top), ") failed for: ", ea.name(i), "\n")[0]);
									#endif
								}
								else if (wrc.bottom > workarea.bottom)
								{
									// Lower Y coordinate failed, check if a fix will work with the current window size
									if (workarea.bottom - (wrc.bottom - wrc.top) > workarea.top)
									{
										SetWindowPos (ea.hwnd(i), 0, wrc.left, workarea.bottom - (wrc.bottom - wrc.top), 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
									}

									#ifdef _DEBUG
										OutputDebugString (&CSR("Y (bottom=", CStr(wrc.bottom), ") failed for: ", ea.name(i), "\n")[0]);
									#endif
								}
							}
						}
					}	// END FOR

					// Re-Create the Timer.
					hUpdateTimer = SetTimer (NULL, 1, UPDATE_TIMER, 0);

					// Unlock
					locked = false;
				}
			}
			else if (msg.message == WM_QUIT)
			{
				break;
			}

			//
			// Note: WM_ENDSESSION does not need to be handled here because this is a THREAD and NOT A WINDOW.
			//
		}
			
		// Set the Server Thread Idle for a time
		Sleep (50);
	}

	// Stop WindowCheck Timer
	KillTimer (NULL, hUpdateTimer);

	// We've done all the work
	WPOSER_VM.RemoveThread();
	wposer_thread = 0;
	return THREAD_RES;
}

