// MegaPopup.h: Declarations needed to use the MegaPopup.cpp module
//
// Thread-Safe: YES
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __MEGAPOPUP_H
#define __MEGAPOPUP_H


	//////////////////
	// Consts		//
	//////////////////
	extern const int MPM_TOPLEFT, MPM_RIGHTBOTTOM;
	extern const int MPM_NOSCROLL, MPM_HSCROLL, MPM_VSCROLL;
	extern const int MPM_DISABLED, MPM_FADED, MPM_HILITEBOX;
	extern const int MPM_NOTHING, MPM_ESC, MPM_PREV;


	//////////////////
	// Types		//
	//////////////////
	class MegaPopup;
	typedef long (mpmcb)(MegaPopup*, MegaPopup*, long, long, long, long*);

	struct MegaPopupFade
	{
		long x;
		long y;
		long* data;
		mpmcb* callback;
	};


	//////////////////////
	// Declarations		//
	//////////////////////
	class MegaPopup
	{
	private:
		// Consts
		static const char WNDCLASS[];
		static const char SFONT_MSSANS[], SFONT_WEBDINGS[];
		static const int MAX_MENUITEMS; 

		// Critical Section Object
		CriticalSection cs;

		// User Interface Cache
		class MPMUSRC
		{
		public:
			string text;
			HICON icon;
			int flags;
			bool used;
		};
		StructArray <MPMUSRC> tme;
		long nextone;
	
		// MegaPopup Cache
		class MPMC
		{
		public:
			long x, y, w, h;
			long idx;
			long flags;
			HICON icon;
			string text;
		};
		StructArray <MPMC> mc;
	
		// Dynamic Constant Cache
		long MPM_FONTSIZE, TS;
	
		// Class Cache
		bool lock, quoteMouse;
		bool fsb, scrolling;
		long sel, threadres, threaddata, scrollspeed, scrollrate;
		long lastmb;
		RECT2 me;
		HWND fwnd;
		HDC fdc;
		HFONT FONT_MSSANS;
		MemoryDC fwnd_mem;
		MegaPopupFade fx;

		// Functions: Init/Terminate
		void Init(long, long);

		// Graphical Objects
		void InitPens ();
		void UnInitPens ();
	
		// Main Popup
		void CalcSizeAlign (long, long, int);
		void CalcCoords ();
		void Scroll (long);
		void RedrawItem (long);
		void DrawHilite (long);
		void DrawScrollArrows ();
		void RefreshAllItems ();
	
		// Window Process
		void KeyboardScroll ();
		void KeyDown (long);
		void JumpPrevious (HWND);
		void MouseMove (long, long, long);
		void MouseUp (long, long, long);
	
		// Item Selection
		long SearchForward (long);
		long SearchBackward (long);
		void ChangeSelection (long);
	
		// Window Subsystem
		string GetWindowClass (HWND);
		HWND PerformEffect (int, int);

	public:
		HWND parent, jumpto;
	
		// StartUp / Terminate
		MegaPopup (long = 19, long = 8);
		~MegaPopup ();
	
		// Add/Delete Entries
		void Add (long = -1, string = "", HICON = 0, int = 0);
		void Delete (long);
		void Clear ();
	
		// User Interface
		long EntryCount ();
		string LongestEntry ();
	
		string Entry (long);
		long Flags (long);
	
		long GetWidth ();
		long GetHeight ();
	
		// Main Popup
		long Show (long, long, int = MPM_TOPLEFT, int = MPM_NOSCROLL, MegaPopupFade* = 0);
		long ShowAlternative (HWND, long, long, int = MPM_TOPLEFT);
		long ShowWindowMenu (HWND, int, MegaPopupFade* = 0);
		long GetButton ();
	
		// Window Process
		LRESULT WndProc (HWND, UINT, WPARAM, LPARAM);

	protected:
		static LRESULT CALLBACK Callback_WndProc (HWND, UINT, WPARAM, LPARAM);
	};


#endif
