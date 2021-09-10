// ShellEx.h: Declares everything needed to use the ShellEx.cpp module
// 
// Thread-Safe: NO
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __SHELLEXDLG_H
#define __SHELLEXDLG_H


	// ---------------------------------------------
	// Use this function to Start the ShellEx Dialog
	// ---------------------------------------------
	void ShellExDialog();


	//////////////////////////////////////
	// Class: Shell Execute Dialog		//
	//////////////////////////////////////
	class dlg_ShellExecute
	{
	private:
		// Consts
		static const char REG_RUNMRU[];

		// Critical Section Object
		CriticalSection cs;

		// Cache
		HWND dwnd;
		HICON ICON_SOFTWAREBIG;
		IconBox* ibox;
		string FILESEARCH_FILTER;
	
		// Functions
		bool IsComboCall (string);
		void UpdateStatusIcon ();

	public:
		// Cache
		HWND nullwnd;
	
		// Functions: Constructor, Destructor
		dlg_ShellExecute ();
		~dlg_ShellExecute ();
	
		// Functions: Callback
		bool WndProc (HWND, UINT, WPARAM, LPARAM);
		long ShellEx_Thread();

	protected:
		static INT_PTR CALLBACK _ext_ShellEx_WndProc (HWND, UINT, WPARAM, LPARAM);
		static long _ext_ShellEx_Thread(dlg_ShellExecute*);
	};


#endif