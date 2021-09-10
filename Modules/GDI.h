// GDI.h: Declares everything needed to use the GDI.cpp module
// 
// Thread-Safe: YES
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __GDI_H
#define __GDI_H


	//////////////
	// Types	//
	//////////////
	struct RECT2
	{
		long x;
		long y;
		long w;
		long h;
	};


	//////////////
	// Consts	//
	//////////////
	extern const int GWI_SMALL, GWI_BIG;


	//////////////////
	// Globals		//
	//////////////////
	extern class StdColors sc;


	//////////////////////
	// GDI Macros		//
	//////////////////////
	HDC DisplayDC ();
	void MovePen (HDC, long, long);


	//////////////////////////////
	// GDI Standard Drawing		//
	//////////////////////////////
	void DrawLine (HDC, long, long, HPEN);
	void Line2 (HDC, long, long, long, long, HPEN);
	void SetPixel2 (HDC, long, long, COLORREF);
	void FillRect2 (HDC, long, long, long, long, HBRUSH);
	void TextOutEx (HDC, long, long, string);
	void FontOut (HDC, long, long, string, HFONT, bool = false);
	void GreyFontOut (HDC, long, long, string, HFONT, bool = false);
	void UnderlineCharacter (HDC, long, long, unsigned char, HFONT);
	void DrawFontChar (HDC, long, long, unsigned char, string, int, long);


	//////////////////////////
	// GDI Information		//
	//////////////////////////
	long GetColormode ();
	long GetTextWidth (HDC, HFONT, string);
	long GetTextHeight (HDC, HFONT, string);
	string AdjustStr2Font (HDC, HFONT, string, long);


	//////////////////////////////
	// GDI General Functions	//
	//////////////////////////////
	void Color2RGB (long, unsigned char*, unsigned char*, unsigned char*);
	bool PointInRect (long, long, long, long, long, long);
	void VerifyRect (long*, long*, long*, long*);
	void BlurChannel (unsigned char*, int = 50);
	void DeBlurChannel (unsigned char*, int = 50);
	long Distance (long, long, long, long);


	//////////////////////////////////
	// GDI Graphical Objects 2D/3D	//
	//////////////////////////////////
	void DrawFrame3D (HDC, long, long, long, long);
	void DrawInvFrame3D (HDC, long, long, long, long, bool);
	void DrawBoldFrame3D (HDC, long, long, long, long);
	void DownLine (HDC, long, long, long);
	void CrossLine (HDC, long, long, long);
	void DrawCircle (HDC, long, long, long, long, bool);
	void DrawSurround (HDC, long, long, long, long, long);
	void DrawGroupBox (HDC, long, long, long, long, string = "");
	void DrawGradient (HDC, long, long, long, long, unsigned char, unsigned char, unsigned char, 
					   unsigned char, unsigned char, unsigned char, ULONG);
	int GradientFillConvert (unsigned char);


	//////////////////////////
	// GDI Icon Functions	//
	//////////////////////////
	void BlitIcon (HDC, HICON, long, long, long, long, long);
	int TransIcon (HDC, HICON, long, long, long, long);
	void InvIcon (HDC, HICON, long, long, long, long);
	HICON LoadIconRes (HINSTANCE, LONG_PTR, long, long);


	//////////////////////////
	// GDI DIB Functions	//
	//////////////////////////
	char* DIB_CreateArray (long, long);
	void DIB_DeleteArray (char*);
	void DIB_HDC2RGB (HDC, long, long, char*);
	void DIB_RGB2HDC (HDC, long, long, char*);


	//////////////////////////////
	// GDI Window DC Service	//
	//////////////////////////////
	class MemoryDC
	{
	private:
		// Critical Section Object
		CriticalSection cs;

		// Cache
		HGDIOBJ oldobj;
		HBITMAP backbuffer;
		HWND mywnd;
		HDC realdc, memdc;
		long width, height;
		bool inited;

	public:
		MemoryDC ();
		~MemoryDC ();

		// Functions: Interface
		HDC Get ();
		HDC Create (HWND, long, long);

		void Free ();
		void Resize (long, long, long);
		void Refresh ();
	};


	//////////////////////////////
	// GDI Standard Colors		//
	//////////////////////////////
	class StdColors
	{
	private:
		// Critical Section Object
		CriticalSection cs;

	public:
		HPEN	PBLACK, PWHITE, PRED, PLRED, PGREEN, PLGREEN, PBLUE, PLBLUE, PGREY, PLGREY;
		HBRUSH	BBLACK, BWHITE, BRED, BLRED, BGREEN, BLGREEN, BBLUE, BLBLUE, BGREY, BLGREY;
	
		StdColors ();
		~StdColors ();
	};


	//////////////////////////////
	// GDI Standard Fonts		//
	//////////////////////////////
	class StdFonts
	{
	private:
		// Critical Section Object
		CriticalSection cs;

	public:
		string TXT_MSSANS;
		HFONT MSSANS;
	
		StdFonts ();
		~StdFonts ();
	};


	//////////////////////
	// Fade Screen		//
	//////////////////////
	class FadeScreen
	{
	private:
		// Consts
		static const char WNDCLASS[];

		// Critical Section Object
		CriticalSection cs;

		// Cache
		bool lock;
		long sw, sh;
		HWND swnd;
		HDC sdc, deskdc;
		MemoryDC swnd_mem;
	
		// Functions
		void MeRefresh ();
		void MeRefreshTicks (long);

	public:
		FadeScreen ();
		~FadeScreen ();

		// Functions: Interface
		void Fade (int);
	
		// Window Process
		LRESULT WndProc (HWND, UINT, WPARAM, LPARAM);

	protected:
		static LRESULT CALLBACK Callback_WndProc (HWND, UINT, WPARAM, LPARAM);
	};



	//////////////////////////////////
	// Extended Window-Handling		//
	//////////////////////////////////
	HWND ActivateWindow (HWND);
	void DeActivateWindow (HWND);
	void SetForegroundWindowEx (HWND);
	void ExitAppWindow (HWND);


	//////////////////////////////
	// Window Style Management	//
	//////////////////////////////
	bool StyleExists (HWND, long);
	bool ExStyleExists (HWND, long);
	void AddStyle (HWND, long);
	void AddExStyle(HWND, long);
	void RemoveStyle (HWND, long);
	void RemoveExStyle (HWND, long);
	void AdjustStyle (HWND, long, long);
	void GWLnotify (HWND);


	//////////////////////////
	// Window Information	//
	//////////////////////////
	HWND GetAbsParent (HWND);
	bool WindowVisible (HWND);
	HICON GetWindowIcon (HWND, int);
	bool WindowHung (HWND);

	bool CheckWindowCoords (HWND, long = -1, long = -1);
	long GetWindowCaptionHeight ();


	//////////////////////////////////
	// Multiple Monitor Support		//
	//////////////////////////////////
	bool GetActiveDisplayDevices (StructArray <DISPLAY_DEVICE> *, StructArray <DEVMODE> *);
	bool GetWorkareaForWindow (HWND, RECT*);
	bool GetWorkareaForScreenPart (RECT&, RECT*);


	//////////////////////
	// Window Macros	//
	//////////////////////
	void RfMsgQueue (HWND);
	bool IsWsVisible (HWND);
	HWND ExplorerWindow ();


	//////////////////////////
	// Window Creation		//
	//////////////////////////
	void NewWindowClass (string, WNDPROC, HINSTANCE, 
						 long = 0, HICON = 0, HICON = 0, const char* = NULL, bool = false);

	void RegisterNullWindowClass ();
	void UnregisterNullWindowClass ();

	HWND CreateMainAppWindow (string, string, DWORD = 0, DWORD = 0, LPVOID = 0, HINSTANCE = 0);
	HWND CreateNullWindow (string title = "", LPVOID = 0, string = "");


#endif
