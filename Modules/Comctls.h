// Comctls.h: Declares everything needed to use the Comctls.cpp module
// 
// Thread-Safe: YES
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __COMCTLS_H
#define __COMCTLS_H


	//////////////////////////////
	// Class: BitmapButton		//
	//////////////////////////////
	class BitmapButton
	{
	private:
		// Consts
		static const char WNDCLASS[];

		// Cache
		CriticalSection cs;

		RECT2 me;
		HWND bwnd, parent;
		MemoryDC bwnd_mem;
		HDC bdc;
		HICON icon;
		HFONT font;
		string text;
		bool altselmode;
	
		POINT md;
		long icony, fonty;
		bool active, focused, selected;

	public:
		BitmapButton (HWND, long, long, long, long, HICON, HFONT, string, bool = false);
		~BitmapButton ();
		void DrawMe (bool);
	
		// Events
		void ButtonClick ();
		bool clicked (WPARAM, LPARAM);
	
		// Window Process
		LRESULT WndProc (HWND, UINT, WPARAM, LPARAM);
	
		// Properties
		void SetCaption (string);
		void SetFont (HFONT);
		void Enable (bool);
		HWND hwnd ();

	protected:
		static LRESULT CALLBACK Callback_WndProc (HWND, UINT, WPARAM, LPARAM);
	};



	//////////////////////////
	// Class: IconBox		//
	//////////////////////////
	class IconBox
	{
	private:
		// Consts
		static const char WNDCLASS[];

		// Cache
		CriticalSection cs;

		RECT2 me;
		MemoryDC iwnd_mem;
		HDC idc;
		HWND parent, iwnd;
		bool lock;

	public:
		IconBox (HWND, long, long, long, long, HICON = 0);
		~IconBox ();
	
		// Window Process
		LRESULT WndProc (HWND, UINT, WPARAM, LPARAM);
	
		// Interface
		void DrawIcon (HICON);
		HWND hwnd ();

	protected:
		static LRESULT CALLBACK Callback_WndProc (HWND, UINT, WPARAM, LPARAM);
	};



	//////////////////////////////
	// Class: 3D Line Window	//
	//////////////////////////////
	class LineWindow
	{
	private:
		// Consts
		static const char WNDCLASS[];

		// Cache
		CriticalSection cs;

		RECT2 me;
		MemoryDC lwnd_mem;
		HDC ldc;
		HWND parent, lwnd;
		bool lock;
		long mode;
	
		void DrawMe ();

	public:
		LineWindow (HWND, long, long, long, int);
		~LineWindow ();
	
		LRESULT WndProc (HWND, UINT, WPARAM, LPARAM);
		HWND hwnd ();

	protected:
		static LRESULT CALLBACK Callback_WndProc (HWND, UINT, WPARAM, LPARAM);
	};


#endif
