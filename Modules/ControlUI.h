// ControlUI.h: Declares everything needed to use the ControlUI.cpp module
// 
// Thread-Safe: YES
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __CONTROLUI_H
#define __CONTROLUI_H


	//////////////////
	// Includes		//
	//////////////////
	typedef tagLVITEMA LVI;
	typedef TVINSERTSTRUCTA TVINS;
	typedef TVITEMA TVI;


	//////////////////////
	// General Macros	//
	//////////////////////
	HWND dia (HWND, int);
	int MessageBox (HWND, string, string, UINT);

	string	ctlGetText	(HWND);
	void	ctlSetText	(HWND, string);
	void	ctlSetFont	(HWND, HFONT);

	bool Button_Clicked (WPARAM, long);


	//////////////////////////
	// Macros: Check Box	//
	//////////////////////////
	long chkGetState (HWND);
	void chkSetState (HWND, long);


	//////////////////////////
	// Macros: ComboBox		//
	//////////////////////////
	void cbSetHeight (HWND, long);
	void cbAdd (HWND, string);
	long cbCount (HWND);
	long cbSelected (HWND);
	string cbText (HWND, long);
	string cbAbsText (HWND);


	//////////////////////////
	// Macros: List Box		//
	//////////////////////////
	long lbAdd (HWND, string);
	long lbInsert (HWND, string, long = -1);
	void lbRemove (HWND, long);
	void lbClear (HWND);
	long lbCount (HWND);

	string lbText (HWND, long);
	long lbGetItemData (HWND, long);
	bool lbSetItemData (HWND, long, long);

	void lbSelectSingle (HWND, long);
	bool lbSelectMulti (HWND, long, bool);

	long lbSelectedItem (HWND);
	long lbGetSelItems (HWND, long*, long);


	//////////////////////////
	// Macros: List View	//
	//////////////////////////
	void lvAddColumn (HWND, long, string, long = LVCFMT_CENTER, long = 0);

	long lvAdd (HWND lwnd, long, long, string, UINT, LVI*, UINT = 0);
	void lvRemove (HWND, long);
	void lvClear (HWND);
	long lvCount (HWND);

	long lvSelectedItem (HWND);
	bool lvSelected (HWND, long);
	void lvSelect (HWND, long, bool);
	void lvSetItemFocus (HWND, long);

	void lvSetItemText (HWND, long, long, string);
	string lvGetItemText (HWND, long, long, long = 512);

	void lvItemRect (HWND, long, RECT*, long);
	long lvClickedItem (HWND, long, long);


	//////////////////////////
	// Macros: Toolbar		//
	//////////////////////////
	void CreateToolbarButton (TBBUTTON*, long, long, unsigned char, unsigned char, long);
	void tbSetButtonSize (HWND, long, long);
	void tbAutoSize (HWND);


	//////////////////////////
	// Macros: Tree-View	//
	//////////////////////////
	HTREEITEM tvAdd (HWND, HTREEITEM, HTREEITEM, int, string, UINT, TVINS*, UINT = 0);
	void tvDelete (HWND, HTREEITEM);
	void tvClear (HWND);

	void tvSetIndent (HWND, long);
	void tvSetImageList (HWND, HIMAGELIST, long = TVSIL_NORMAL);

	void tvExpand (HWND, HTREEITEM, UINT = TVE_EXPAND);

	void tvSelect (HWND, HTREEITEM, long = TVGN_CARET);
	HTREEITEM tvSelectedItem (HWND);

	HTREEITEM tvGetParent (HWND, HTREEITEM);
	void tvGetItem (HWND, HTREEITEM, UINT, TVI*);
	void tvSetItem (HWND, HTREEITEM, UINT, TVI*);

	string tvGetItemText (HWND, HTREEITEM, long = 512, LRESULT* = 0);


	//////////////////////////
	// Macros: Trackbar		//
	//////////////////////////
	void trkSetRange (HWND, long, long, bool = true);
	void trkSetThumbsize (HWND, long);
	long trkGetPos (HWND);
	void trkSetPos (HWND, long);


	//////////////////////////
	// Macros: Tab Control	//
	//////////////////////////
	void tabInsert (HWND, long, string, long = 0);
	long tabSelected (HWND);


	//////////////////////////////
	// Fast Control Creation	//
	//////////////////////////////
	HWND CreateButton (HWND, string, long, long, long, long, HINSTANCE, LPVOID = 0);
	HWND CreateEditbox (HWND, string, long, long, long, HINSTANCE, LPVOID = 0);


	//////////////////////////////
	// Tabbed Dialog Support	//
	//////////////////////////////
	class TabDialog
	{
	private:
		// CS Object
		CriticalSection cs;

		// Cache
		class TabDlgCache
		{
		public:
			string caption;
			HWND hwnd;
			DLGTEMPLATE* res;
		};
		StructArray <TabDlgCache> tdc;
		StdFonts locsf;
	
		HWND parent, mytab;
		RECT2 tabrc;
		DLGPROC callback;
		long dlgbase_x, dlgbase_y, curdlg;
		bool tab_running;

	public:
		// Init / Terminate
		TabDialog ();
		~TabDialog ();
	
		// Check
		bool SetupDone ();
	
		// Setup
		void BasicSetup (HWND, DLGPROC);
		void AddDialog (LPCTSTR, string);
		void RemoveDialog (long);
	
		void StartTabs (long, long);
		void StopTabs ();
	
		// Interface
		void NotifyMsg (LPARAM);
		void SwitchDialog (long);
	
		// Helper Functions
		HWND DetWnd (long);
	};


#endif
