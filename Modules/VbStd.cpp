// VbStd.h: Visual Basic 6.0 Standard Functions
//
// Thread-Safe: YES
//


//////////////////
// Includes		//
//////////////////
#include "stdafx.h"
#include "math.h"

#include "_modules.h"


//////////////////////////////
// VB 6.0 Main Functions	//
//////////////////////////////
int InfoBox (HWND hwnd, string text, string caption, UINT flags)
{
	return MessageBox (hwnd, &text[0], &caption[0], flags);
}

void DebugOut (DWORD dwValue)
{
	DebugOut (CStr(dwValue));
}

void DebugOut (string strText)
{
	MessageBox (0, &strText[0], "Debug Message", MB_OK);
}

long ScreenWidth ()
{
	DEVMODE dev;
	EnumDisplaySettings (0, ENUM_CURRENT_SETTINGS, &dev);
	return dev.dmPelsWidth;
}

long ScreenHeight ()
{
	DEVMODE dev;
	EnumDisplaySettings (0, ENUM_CURRENT_SETTINGS, &dev);
	return dev.dmPelsHeight;
}

long VirtualScreenWidth ()
{
	return GetSystemMetrics(SM_CXVIRTUALSCREEN);
}

long VirtualScreenHeight ()
{
	return GetSystemMetrics(SM_CYVIRTUALSCREEN);
}


long Bool2Long (bool thebool)
{
	if (thebool) { return 1; } else { return 0; };
}

bool FnExported (string library, string func)
{
	bool success = false;
	HMODULE hlib = LoadLibrary(&library[0]);
	if (hlib != 0)
	{
		FARPROC paddr = GetProcAddress (hlib, &func[0]);
		if (paddr != 0) { success = true; };
		FreeLibrary (hlib);
	}
	return success;
}

void beep () { MessageBeep (0xFFFFFFFF); };


//////////////////////////////
// Mathematical Functions	//
//////////////////////////////
long Round (double number)
{
	long bottom = (long) floor (number);
	if ((number - bottom) >= 0.5)
	{
		return (bottom + 1);
	}
	else
	{
		return bottom;
	}	
}


//////////////////////////
// Attribute Constants	//
//////////////////////////
bool Flag (LONG_PTR opt, long att)
{
	if (opt & att) { return true; } else { return false; };
}

void AddFlag (LONG_PTR* opt, LONG_PTR att) { (*opt) |= att; };

void DelFlag (LONG_PTR* opt, LONG_PTR att)
{
	// Att can only be removed by XOR if existing
	if ((*opt) & att) { (*opt) ^= att; };
}


//////////////////////////////////
// String <-> Hex Conversion	//
//////////////////////////////////
string str2hex (string input)
{
	string buf;
	long len = xlen(input);
	
	// Create a buffer of double length
	buf.resize (len*2);
	
	// Do the conversion
	for (long i = 1; i <= len; i++)
	{
		byte2hex (input[i-1], &buf[(i-1)*2]);
	}
	return buf;
}

void byte2hex (unsigned char byte, char *twobuf)
{
	int div = byte / 16;
	int rest = byte % 16;
	
	// 1st digit
	if (div <= 9) { twobuf[0] = 48 + div; } else { twobuf[0] = 55 + div; };
	
	// 2nd digit
	if (rest <= 9) { twobuf[1] = 48 + rest; } else { twobuf[1] = 55 + rest; };
}

string hex2str (string input)
{
	string buf;
	long len = xlen(input);
	if (floor((double) len / 2) != ((double) len / 2)) { return ""; };
	
	// Create a buffer of half length
	buf.resize (len/2);
	
	// Do the conversion
	for (long i = 1; i <= len; i += 2)
	{
		hex2byte (&input[i-1], (unsigned char*) &buf[(i-1)/2]);
	}
	return buf;
}

void hex2byte (char *twobuf, unsigned char* byte)
{
	// 1st digit
	if (twobuf[0] >= 65) { *byte = 16*(twobuf[0]-55); } else { *byte = 16*(twobuf[0]-48); };
	
	// 2nd digit
	if (twobuf[1] >= 65) { *byte += (twobuf[1]-55); } else { *byte += (twobuf[1]-48); };
}



//////////////////////////
// DateTime Functions	//
//////////////////////////
string date ()
{
	SYSTEMTIME sysdate;
	GetLocalTime (&sysdate);
	return date(&sysdate);
}

string date_ger ()
{
	SYSTEMTIME sysdate;
	GetLocalTime (&sysdate);
	return date_ger(&sysdate);
}

string date (SYSTEMTIME* sysdate)
{
	return CSR(FillStr(CStr(sysdate->wDay), 2, '0'), "/", 
				FillStr(CStr(sysdate->wMonth), 2, '0'), "/", CStr(sysdate->wYear));
}

string date_ger (SYSTEMTIME* sysdate)
{
	return CSR(FillStr(CStr(sysdate->wDay), 2, '0'), ".", 
				FillStr(CStr(sysdate->wMonth), 2, '0'), ".", CStr(sysdate->wYear));
}

string time ()
{
	SYSTEMTIME systime;
	GetLocalTime (&systime);
	return time(&systime);
}

string time (SYSTEMTIME* systime)
{
	return CSR(FillStr(CStr(systime->wHour), 2, '0'), ":", 
				FillStr(CStr(systime->wMinute), 2, '0'), ":", 
				FillStr(CStr(systime->wSecond), 2, '0'));
}

string _clock ()
{
	SYSTEMTIME systime;
	GetLocalTime (&systime);
	return _clock(&systime);
}

string _clock (SYSTEMTIME* systime)
{
	return CSR(CStr(systime->wHour), ":", FillStr(CStr(systime->wMinute), 2, '0'));
}


//////////////////////////
// Time Conversions		//
//////////////////////////
void msecs2systime (DWORD milli, SYSTEMTIME* systime)
{
	systime->wHour = (unsigned short) floor(((double) milli / 1000) / 3600);
	systime->wMinute = (WORD) floor(((double) milli / 1000) / 60) - (systime->wHour * 60);
	systime->wSecond = (WORD) floor((double) milli / 1000) - (systime->wHour * 3600) - (systime->wMinute * 60);
}

string SystimeToString (SYSTEMTIME systime)
{
	string ram; ram.resize (sizeof(SYSTEMTIME));
	systime.wDayOfWeek = 0;
	CopyMemory (&ram[0], &systime, sizeof(SYSTEMTIME));
	return ram;
}

void StringToSystime (string str_time, SYSTEMTIME* systime)
{
	str_time.resize (sizeof(SYSTEMTIME));
	CopyMemory (systime, &str_time[0], sizeof(SYSTEMTIME));
	systime->wDayOfWeek = 0;
}



