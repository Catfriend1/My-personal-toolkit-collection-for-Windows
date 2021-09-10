// VbStd.h: Declarations needed to use the VbStd.cpp module
// 
// Thread-Safe: YES
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __VBSTD_H
#define __VBSTD_H


	//////////////////////////////
	// VB 6.0 Main Functions	//
	//////////////////////////////
	int InfoBox (HWND, string, string, UINT);
	void DebugOut (DWORD);
	void DebugOut (string);
	long ScreenWidth ();
	long ScreenHeight ();
	long VirtualScreenWidth ();							// for multiple monitor systems
	long VirtualScreenHeight ();						// for multiple monitor systems
	long Bool2Long (bool);
	bool FnExported (string, string);
	void beep ();


	//////////////////////////////
	// Mathematical Functions	//
	//////////////////////////////
	long Round (double);


	//////////////////////////
	// Attribute Constants	//
	//////////////////////////
	bool Flag (LONG_PTR, long);
	void AddFlag (LONG_PTR*, LONG_PTR);
	void DelFlag (LONG_PTR*, LONG_PTR);


	//////////////////////////////////
	// String <-> Hex Conversion	//
	//////////////////////////////////
	string str2hex (string);
	void byte2hex (unsigned char, char*);
	string hex2str (string);
	void hex2byte (char*, unsigned char*);


	//////////////////////////
	// DateTime Functions	//
	//////////////////////////
	string date ();
	string date_ger ();

	string date (SYSTEMTIME*);
	string date_ger (SYSTEMTIME*);

	string time ();
	string time (SYSTEMTIME*);

	string _clock ();
	string _clock (SYSTEMTIME*);


	//////////////////////////
	// Time Conversions		//
	//////////////////////////
	void msecs2systime (DWORD, SYSTEMTIME*);

	string	SystimeToString (SYSTEMTIME);
	void	StringToSystime (string, SYSTEMTIME*);



#endif

