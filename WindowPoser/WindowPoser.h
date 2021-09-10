// WindowPoser.h: Declares everything needed to use the WindowPoser.cpp module
//
// Thread-Safe: NO
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __WINDOWPOSER_H
#define __WINDOWPOSER_H


	//////////////////////////////
	// Class: WindowPoser		//
	//////////////////////////////
	class WindowPoser
	{
	private:
		// Consts
		static const char CAPTION[];
		static const char CAPTION_START_BUTTON[];

		static const long FS_PIXEL_TOLERANCE;
		static const long UPDATE_TIMER;
		static const long THREAD_RES;

		// Critical Section Object
		CriticalSection cs;

		// Cache
		ThreadManager WPOSER_VM;
		DWORD wposer_thread;

		// Functions: Init/Terminate
		void Terminate();

	public:
		// Constructor/Destructor
		WindowPoser();
		~WindowPoser();

		// Functions: Callback
		long WorkerThread ();

		LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);

	protected:
		// Functions: Callback
		static long _ext_WindowPoser_WorkerThread(WindowPoser*);
	};


#endif