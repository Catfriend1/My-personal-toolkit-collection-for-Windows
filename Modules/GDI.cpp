// GDI.cpp: Windows GDI Related Functions
//
// Thread-Safe: YES
//


//////////////////
// Switches		//
//////////////////////////////////////////////////////////////////
// MY_GDI_MSIMG -- GDI imports functions from "msimg32.dll"		//
//////////////////////////////////////////////////////////////////


//////////////////
// Includes		//
//////////////////
#include "stdafx.h"
#include "math.h"

#include "_modules.h"


//////////////
// Consts	//
//////////////
const char WC_NULL[]			= "*WC_NULL#50147";			// ["*WC_NULL#50147"]

const int GWI_SMALL				= ICON_SMALL;
const int GWI_BIG				= ICON_BIG;


//////////////
// Cache	//
//////////////
StdColors sc;						// Create an instance of the Standard-Color-Class
bool wcnull_loaded = false;


//////////////////////
// GDI Macros		//
//////////////////////
HDC		DisplayDC	()							{ return CreateDC("DISPLAY", 0, 0, 0); };
void	MovePen		(HDC hdc, long x, long y)	{ MoveToEx (hdc, x, y, 0); };


//////////////////////////////
// GDI Standard Drawing		//
//////////////////////////////
void DrawLine (HDC hdc, long dstx, long dsty, HPEN pen)
{
	if (pen != 0)
	{
		HGDIOBJ oldgdiobj = SelectObject (hdc, pen);
		LineTo (hdc, dstx, dsty);
		SelectObject (hdc, oldgdiobj);
	}
	else
	{
		LineTo (hdc, dstx, dsty);
	}
}

void Line2 (HDC hdc, long x1, long y1, long x2, long y2, HPEN pen)
{
	MovePen (hdc, x1, y1); DrawLine (hdc, x2, y2, pen);
}

void SetPixel2 (HDC hdc, long x, long y, COLORREF color)
{
	HPEN tmppen = CreatePen (0, 1, color);
	Line2 (hdc, x, y, x+1, y+1, tmppen);
	DeleteObject (tmppen);
}

void FillRect2 (HDC hdc, long x, long y, long width, long height, HBRUSH brush)
{
	RECT rc;
	rc.left = x; rc.top = y; rc.right = x+width; rc.bottom = y+height;
	FillRect (hdc, &rc, brush);
}

void TextOutEx (HDC hdc, long x, long y, string text)
{
	TextOut (hdc, x, y, &text[0], xlen(text));
}

void FontOut (HDC hdc, long x, long y, string text, HFONT font, bool underline)
{
	string strchar;
	long pos;
	if (underline)
	{
		pos = InStrFirst(text, '&', 1);
		if (pos != 0)
		{
			if (pos < xlen(text))		// Character after '&' exists
			{
				strchar = Mid(text, pos + 1, 1);
			}
			text = CSR(Left(text, pos-1), Right(text, xlen(text)-pos));
		}
	}
	
	HGDIOBJ oldobj = SelectObject (hdc, font);
	TextOut (hdc, x, y, &text[0], xlen(text));
	SelectObject (hdc, oldobj);
	
	if (xlen(strchar) != 0)
	{
		UnderlineCharacter (hdc, x + GetTextWidth(hdc, font, Left(text,pos-1)), y, 
									strchar[0], font);
	}
}

void GreyFontOut (HDC hdc, long x, long y, string text, HFONT font, bool underline)
{
	SetTextColor (hdc, RGB(255, 255, 255));
	FontOut (hdc, x + 1, y + 1, text, font, underline);
	SetTextColor (hdc, RGB(127, 127, 127));
	FontOut (hdc, x, y, text, font, underline);
}

void UnderlineCharacter (HDC hdc, long x, long y, unsigned char character, HFONT font)
{
	HPEN tmppen = CreatePen (0, 1, GetTextColor(hdc));
	HGDIOBJ oldpen = SelectObject (hdc, tmppen);
	HGDIOBJ oldfont = SelectObject (hdc, font);
	SIZE sz;
	POINT old;
	
	GetTextExtentPoint32 (hdc, &Chr(character)[0], 1, &sz);
	MoveToEx (hdc, x, y + sz.cy - 1, &old);
	LineTo (hdc, x + sz.cx, y + sz.cy - 1);
	MovePen (hdc, old.x, old.y);
	
	SelectObject (hdc, oldfont);
	SelectObject (hdc, oldpen);
	DeleteObject (tmppen);
}

void DrawFontChar (HDC hdc, long x, long y, unsigned char character, 
				   string font, int size, long color)
{
	SetTextColor (hdc, color);
	HFONT hfont = CreateFont (size, 0, 0, 0, FW_NORMAL, false, false, false, DEFAULT_CHARSET, 
								OUT_TT_PRECIS, CLIP_LH_ANGLES, PROOF_QUALITY, 4, &font[0]);
	FontOut (hdc, x, y, Chr(character), hfont);
	DeleteObject (hfont);
}


//////////////////////////
// GDI Information		//
//////////////////////////
long GetColormode ()
{
	HDC hdc = GetDC(0);
	long colmode = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);
	ReleaseDC (0, hdc);
	return colmode;
}

long GetTextWidth (HDC hdc, HFONT font, string txt)
{
	SIZE sz;
	HGDIOBJ oldobj = SelectObject(hdc, font);
	GetTextExtentPoint32 (hdc, &txt[0], xlen(txt), &sz);
	SelectObject (hdc, oldobj);
	return sz.cx;
}

long GetTextHeight (HDC hdc, HFONT font, string txt)
{
	SIZE sz;
	HGDIOBJ oldobj = SelectObject(hdc, font);
	GetTextExtentPoint32 (hdc, &txt[0], xlen(txt), &sz);
	SelectObject (hdc, oldobj);
	return sz.cy;
}

string AdjustStr2Font (HDC hdc, HFONT font, string str, long maxpixel)
{
	SIZE sz;
	HGDIOBJ oldobj = SelectObject(hdc, font);
	while (true)
	{
		GetTextExtentPoint32 (hdc, &str[0], xlen(str), &sz);
		if (sz.cx <= maxpixel)		// OK
		{
			break;
		}
		else
		{
			if (xlen(str) > 0)
			{
				str = Left(str, xlen(str)-1);	// Cut one character
			}
			else
			{
				break;				// Failure - maxpixel is too small
			}
		}
	}
	SelectObject (hdc, oldobj);
	return str;
}


//////////////////////////////
// GDI General Functions	//
//////////////////////////////
void Color2RGB (long color, unsigned char* r, unsigned char* g, unsigned char* b)
{
	CopyMemory (r, &color, 1);
	CopyMemory (g, ((char*) &color) + 1, 1);
	CopyMemory (b, ((char*) &color) + 2, 1);

	// *b = ((color & 16711680) / 65536);
	// *g = ((color & 65280) / 256);
	// *r = (color & 255);
}

bool PointInRect (long x, long y, long x1, long y1, long x2, long y2)
{
	return ((x >= x1) && (x <= x2) && (y >= y1) && (y <= y2));
}

void VerifyRect (long* x1, long* y1, long* x2, long* y2)
{
	long cx1, cy1, cx2, cy2;
	cx1 = *x1; cy1 = *y1; cx2 = *x2; cy2 = *y2;
	if (cx1 > cx2) {*x1 = cx2; *x2 = cx1;}
	if (cy1 > cy2) {*y1 = cy2; *y2 = cy1;}
}

void BlurChannel (unsigned char* channel, int amount)
{
	// Amount = 50;
	if ((*channel + amount) > 255)
	{
		*channel = *channel + ((255 - *channel) / 2);
	}
	else
	{
		*channel = *channel + amount;
	}
}

void DeBlurChannel (unsigned char* channel, int amount)
{
	// Amount = 50;
	if ((*channel - amount) < 0)
	{
		*channel = *channel - (*channel / 2);
	}
	else
	{
		*channel = *channel - amount;
	}
}

long Distance (long x1, long y1, long x2, long y2)
{
	return (long) sqrt(pow((double) abs(x2-x1), 2) + pow((double) abs(y2-y1), 2));
}


//////////////////////////////////
// GDI Graphical Objects 2D/3D	//
//////////////////////////////////
void DrawFrame3D (HDC hdc, long x, long y, long width, long height)
{
	MovePen  (hdc, x+width-1, y+1);
	DrawLine (hdc, x+1, y+1, sc.PWHITE);
	DrawLine (hdc, x+1, y+height-1, sc.PWHITE);
	
	DrawLine (hdc, x+width-1, y+height-1, sc.PBLACK);
	DrawLine (hdc, x+width-1, y, sc.PBLACK);
	
	MovePen  (hdc, x+width-2, y+1);
	DrawLine (hdc, x+width-2, y+height-2, sc.PGREY);
	DrawLine (hdc, x, y+height-2, sc.PGREY);
}

void DrawInvFrame3D (HDC hdc, long x, long y, long width, long height, bool fill)
{
	MovePen  (hdc, x+width-1, y+1);
	DrawLine (hdc, x+1, y+1, sc.PBLACK);
	DrawLine (hdc, x+1, y+height-1, sc.PBLACK);
	
	DrawLine (hdc, x+width-1, y+height-1, sc.PWHITE);
	DrawLine (hdc, x+width-1, y, sc.PWHITE);
	
	if (fill)
	{
		MovePen  (hdc, x+3, y+3);
		DrawLine (hdc, x+width-2, y+3, sc.PWHITE);
		
		for (long iy = y+4; iy <= y+height-3; iy++)
		{
			for (long ix = x+3; ix < x+width-2; ix=ix+2)
			{	
				if ((iy/(double) 2) == floor(iy/(double) 2))
				{
					SetPixel2 (hdc, ix+1, iy, RGB(255,255,255));
				}
				else
				{
					SetPixel2 (hdc, ix, iy, RGB(255,255,255));
				}
			}
		}
	}
	
	MovePen  (hdc, x+width-2, y+2);
	DrawLine (hdc, x+2, y+2, sc.PGREY);
	DrawLine (hdc, x+2, y+height-2, sc.PGREY);
	DrawLine (hdc, x+width-2, y+height-2, sc.PLGREY);
	DrawLine (hdc, x+width-2, y+1, sc.PLGREY);
}

void DrawBoldFrame3D (HDC hdc, long x, long y, long width, long height)
{
	DrawFrame3D (hdc, x, y, width, height);
	// Light Grey Rectangle
	MovePen (hdc, x+width-3, y+2);
	DrawLine (hdc, x+2, y+2, sc.PLGREY);
	DrawLine (hdc, x+2, y+height-3, sc.PLGREY);
	DrawLine (hdc, x+width-3, y+height-3, sc.PLGREY);
	DrawLine (hdc, x+width-3, y+2, sc.PLGREY);
	// Dark Grey & White
	MovePen (hdc, x+width-4, y+3);
	DrawLine (hdc, x+width-4, y+height-4, sc.PWHITE);
	DrawLine (hdc, x+3, y+height-4, sc.PWHITE);
	DrawLine (hdc, x+3, y+3, sc.PGREY);
	DrawLine (hdc, x+width-4, y+3, sc.PGREY);
	// Black & Light Grey
	MovePen (hdc, x+width-5, y+4);
	DrawLine (hdc, x+width-5, y+height-5, sc.PLGREY);
	DrawLine (hdc, x+4, y+height-5, sc.PLGREY);
	DrawLine (hdc, x+4, y+4, sc.PBLACK);
	DrawLine (hdc, x+width-5, y+4, sc.PBLACK);
}

void DownLine (HDC hdc, long x, long y, long height)
{
	Line2 (hdc, x, y, x, y+height, sc.PGREY);
	Line2 (hdc, x+1, y, x+1, y+height, sc.PWHITE);
}

void CrossLine (HDC hdc, long x, long y, long width)
{
	Line2 (hdc, x, y, x+width, y, sc.PGREY);
	Line2 (hdc, x, y+1, x+width, y+1, sc.PWHITE);
}

void DrawCircle (HDC hdc, long mx, long my, long radius, long color, bool filled)
{
	long x1 = mx - radius;
	long y1 = my - radius;
	long x2 = mx + radius;
	long y2 = my + radius;
	long d;
	
	for (long i1 = x1; i1 <= x2; i1++)
	{
		for (long i2 = y1; i2 <= y2; i2++)
		{
			d = Distance(mx, my, i1, i2);
			if ((d == radius) || ((d <= radius) && filled))
			{
				SetPixel2 (hdc, i1, i2, color);
			}
		}
	}
}

void DrawSurround (HDC hdc, long x, long y, long width, long height, long color)
{
	for (long ix = 1; ix <= width; ix = ix + 2)
	{
		SetPixel2 (hdc, x+ix, y, color);
		SetPixel2 (hdc, x+ix, y+height, color);
	}
	for (long iy = 1; iy <= height; iy = iy + 2)
	{
		SetPixel2 (hdc, x, y+iy, color);
		SetPixel2 (hdc, x+width, y+iy, color);
	}
}

void DrawGroupBox (HDC hdc, long x, long y, long width, long height, string caption)
{
	// Draw the Main GroupBox
	DownLine (hdc, x, y, height);
	DownLine (hdc, x+width, y, height);
	CrossLine (hdc, x+1, y, width-1);
	CrossLine (hdc, x, y+height-2, width+1);
	
	// Draw Caption if necessary
	if (xlen(caption) != 0)
	{
		StdFonts tmpfnts;
		long wx = GetTextWidth(hdc, tmpfnts.MSSANS, caption);
		long wh = GetTextHeight(hdc, tmpfnts.MSSANS, caption);
		
		// Backup GDI Context
		HGDIOBJ lastbrush = SelectObject (hdc, sc.BWHITE);
		HGDIOBJ lastpen = SelectObject (hdc, sc.PBLACK);
		COLORREF lastcolor = SetTextColor (hdc, 0);
		
		// Draw the filled rectangle and the caption
		Rectangle (hdc, x+11, y-(wh/2)-1, x+wx+17, y-(wh/2)+wh+2);
		FontOut (hdc, x+14, y-(wh/2), caption, tmpfnts.MSSANS);
		
		// Restore GDI Context
		SetTextColor (hdc, lastcolor);
		SelectObject (hdc, lastpen);
		SelectObject (hdc, lastbrush);
		
		// -- PREVIOUS STYLE --
		// FillRect2 (hdc, x+12, y-(wh/2)+1, wx+4, wh, sc.BLGREY);
		// DrawSurround (hdc, x+11, y-(wh/2), wx+5, wh+2, RGB(0, 0, 255));
	}
}

void DrawGradient (HDC hdc, long x1, long y1, long x2, long y2, 
				   unsigned char r1, unsigned char g1, unsigned char b1, 
				   unsigned char r2, unsigned char g2, unsigned char b2, ULONG mode)
{
	GRADIENT_RECT grc;
	TRIVERTEX triv[2];
	
	// Vertex 0
	triv[0].x = x1;
	triv[0].y = y1;
	triv[0].Red			= GradientFillConvert(r1);
	triv[0].Green		= GradientFillConvert(g1);
	triv[0].Blue		= GradientFillConvert(b1);
	triv[0].Alpha = 0;
	
	// Vertex 1
	triv[1].x = x2;
	triv[1].y = y2;
	triv[1].Red			= GradientFillConvert(r2);
	triv[1].Green		= GradientFillConvert(g2);
	triv[1].Blue		= GradientFillConvert(b2);
	triv[1].Alpha = 0;
	
	// Fill it
	grc.UpperLeft = 0;
	grc.LowerRight = 1;

#ifdef MY_GDI_MSIMG
	GradientFill (hdc, &triv[0], 2, &grc, 1, mode);
#endif
}

int GradientFillConvert (unsigned char color)
{
	return (color > 0x7F) ? ((color * 0x100) - 0x10000) : (color * 0x100);
}


//////////////////////////
// GDI Icon Functions	//
//////////////////////////
void BlitIcon (HDC hdc, HICON hicon, long x, long y, 
				long width, long height, long maskcolor)
{
	HBRUSH mask = CreateSolidBrush(maskcolor);
	DrawIconEx (hdc, x, y, hicon, width, height, 0, mask, 0);
	DeleteObject(mask);
}

int TransIcon (HDC hdc, HICON hicon, long x, long y, long width, long height)
{
	return DrawIconEx (hdc, x, y, hicon, width, height, 0, 0, DI_NORMAL);
}

void InvIcon (HDC hdc, HICON hicon, long x, long y, long width, long height)
{
	// Create Main DC
	HDC main = CreateCompatibleDC (hdc);
	HBITMAP bmain = CreateCompatibleBitmap (hdc, width, height);
	HGDIOBJ oldmainobj = SelectObject (main, bmain);
	
	// Create Mask DC
	HDC mask = CreateCompatibleDC (hdc);
	HBITMAP bmask = CreateCompatibleBitmap (hdc, width, height);
	HGDIOBJ oldmaskobj = SelectObject (mask, bmask);
	
	// Perform Inverted Blit
	FillRect2 (mask, 0, 0, width, height, (HBRUSH) COLOR_WINDOW);
	TransIcon (mask, hicon, 0, 0, width, height);
	
	FillRect2 (main, 0, 0, width, height, (HBRUSH) COLOR_WINDOW);
	BitBlt (main, 0, 0, width, height, mask, 0, 0, SRCPAINT);
	BitBlt (hdc, x, y, width, height, main, 0, 0, SRCCOPY);
	
	// Delete Mask DC
	SelectObject (mask, oldmaskobj);
	DeleteObject (bmask);
	DeleteDC (mask);
	
	// Delete Main DC
	SelectObject (main, oldmainobj);
	DeleteObject (bmain);
	DeleteDC (main);
}

HICON LoadIconRes (HINSTANCE hInstance, LONG_PTR residx, long width, long height)
{
	return (HICON) LoadImage (hInstance, (LPCSTR) residx, IMAGE_ICON, width, height, LR_DEFAULTCOLOR);
}


//////////////////////////
// GDI DIB Functions	//
//////////////////////////
char* DIB_CreateArray (long width, long height)
{
	return (char*) LocalAlloc(LMEM_FIXED, width*height*3);
}

void DIB_DeleteArray (char* array)
{
	LocalFree (array);
}

void DIB_HDC2RGB (HDC hdc, long width, long height, char* array)
{
	// Create a temporary copy of the Source DC which will contain the DIB section
	HDC memdc = CreateCompatibleDC (hdc);
	
	// Initialize the BMP Info Header for the DIB
	BITMAPINFO binfo;
	binfo.bmiHeader.biBitCount = 24;
	binfo.bmiHeader.biClrImportant = 0;
	binfo.bmiHeader.biClrUsed = 0;
	binfo.bmiHeader.biCompression = BI_RGB;
	binfo.bmiHeader.biHeight = height;
	binfo.bmiHeader.biPlanes = 1;
	binfo.bmiHeader.biSizeImage = 0;
	binfo.bmiHeader.biWidth = width;
	binfo.bmiHeader.biXPelsPerMeter = 0;
	binfo.bmiHeader.biYPelsPerMeter = 0;
	binfo.bmiHeader.biSize = sizeof(binfo.bmiHeader);
	
	HBITMAP dib = CreateDIBSection (memdc, &binfo, DIB_RGB_COLORS, 0, 0, 0);
	HGDIOBJ oldobj = SelectObject (memdc, dib);
	BitBlt (memdc, 0, 0, width, height, hdc, 0, 0, SRCCOPY);
	GetDIBits(hdc, dib, 0, height, array, &binfo, DIB_RGB_COLORS);		// Get the DIB array
	SelectObject (memdc, oldobj);			// Release 'dib' to be able to delete it
	
	// Clean Up
	DeleteObject (dib); DeleteDC (memdc);
}

void DIB_RGB2HDC (HDC hdc, long width, long height, char* array)
{
	// Recreate the BMP Info Header
	BITMAPINFO binfo;
	binfo.bmiHeader.biBitCount = 24;
	binfo.bmiHeader.biClrImportant = 0;
	binfo.bmiHeader.biClrUsed = 0;
	binfo.bmiHeader.biCompression = BI_RGB;
	binfo.bmiHeader.biHeight = height;
	binfo.bmiHeader.biPlanes = 1;
	binfo.bmiHeader.biSizeImage = 0;
	binfo.bmiHeader.biWidth = width;
	binfo.bmiHeader.biXPelsPerMeter = 0;
	binfo.bmiHeader.biYPelsPerMeter = 0;
	binfo.bmiHeader.biSize = sizeof(binfo.bmiHeader);
	
	// Write the restored DIB directly to the Destination DC
	SetDIBitsToDevice (hdc, 0, 0, width, height, 0, 0, 
						0, height, array, &binfo, DIB_RGB_COLORS);
}


//////////////////////////////
// GDI Window DC Service	//
//////////////////////////////
MemoryDC::MemoryDC ()
{
	inited = false;
}

MemoryDC::~MemoryDC ()
{
	Free();
}

HDC MemoryDC::Get ()																				// public
{
	// No CS, it's an atomic variable
	return memdc;
}

HDC MemoryDC::Create (HWND hwnd, long width, long height)											// public
{
	// Enter Critical Section
	cs.Enter();

	// Free an previously in this class existing DC first before allocating a new one
	if (inited)
	{
		Free();
	}
	
	// Store Information
	mywnd = hwnd;
	realdc = GetDC(mywnd);
	
	// Create new Memory DC
	memdc = CreateCompatibleDC(realdc);
	backbuffer = CreateCompatibleBitmap(realdc, width, height);
	oldobj = SelectObject (memdc, backbuffer);
	FillRect2 (memdc, 0, 0, width, height, (HBRUSH) COLOR_WINDOW);
	
	// Return Memory DC
	inited = true;

	// Leave Critical Section
	cs.Leave();

	// Return newly created DC
	return memdc;
}

void MemoryDC::Free ()																				// public
{
	if (!inited) { return; };

	// Enter Critical Section
	cs.Enter();

	// Free cached DC
	SelectObject (memdc, oldobj);
	DeleteObject (backbuffer);
	DeleteObject (memdc);
	inited = false;

	// Leave Critical Section
	cs.Leave();
}

void MemoryDC::Resize (long width, long height, long backcolor)										// public																// public
{
	HBITMAP oldbackbuffer = backbuffer;
	BITMAPINFOHEADER binfo;
	HGDIOBJ tempoldobj;
	HBRUSH tempbrush;
	HDC tempdc;
	
	// Check if we are inited first
	if (!inited) { return; };

	// Enter Critical Section
	cs.Enter();

	// Determine old width and height
	binfo.biBitCount = 0;
	binfo.biSize = sizeof(BITMAPINFOHEADER);
	GetDIBits (memdc, oldbackbuffer, 0, 0, NULL, (BITMAPINFO*) &binfo, DIB_RGB_COLORS);
	
	// Create a Temporary DC and a new backbuffer
	tempdc = CreateCompatibleDC(memdc);
	backbuffer = CreateCompatibleBitmap(memdc, width, height);
	
	// Select the new backbuffer into the Memory DC
	SelectObject (memdc, backbuffer);
	
	// Select the old backbuffer into the Temporary DC
	tempoldobj = SelectObject (tempdc, oldbackbuffer);
	
	// Clear the new backbuffer
	tempbrush = CreateSolidBrush(backcolor);
	FillRect2 (memdc, 0, 0, width, height, tempbrush);
	DeleteObject (tempbrush);
	
	// Copy the old backbuffer (tempdc) into the new one (memdc)
	BitBlt (memdc, 0, 0, binfo.biWidth, binfo.biHeight, 
				tempdc, 0, 0, SRCCOPY);
	
	// Delete the old backbuffer and the Temporary DC
	SelectObject (tempdc, tempoldobj);
	DeleteObject (oldbackbuffer);
	DeleteObject (tempdc);
	
	// Leave Critical Section
	cs.Leave();
}

void MemoryDC::Refresh ()																			// public
{
	RECT rc;

	// Enter Critical Section
	cs.Enter();

	// Get RECT of the window owning the DC
	GetClientRect (mywnd, &rc);

	// (!) Stupid, this caused child repaints:
	// InvalidateRect (mywnd, 0, false);
	
	// Copy the Memory DC to the real Surface
	BitBlt (realdc, 0, 0, rc.right, rc.bottom, memdc, 0, 0, SRCCOPY);
	ValidateRect (mywnd, 0);

	// Leave Critical Section
	cs.Leave();
}


///////////////////////////////////////////////////////////////////////////
///////////////////		Class: StdColors	///////////////////////////////
///////////////////////////////////////////////////////////////////////////
StdColors::StdColors ()
{
	// Initialize Pens
	PBLACK = CreatePen(0, 1, RGB(0, 0, 0));
	PWHITE = CreatePen(0, 1, RGB(255, 255, 255));
	PRED = CreatePen(0, 1, RGB(128, 0, 0));
	PLRED = CreatePen(0, 1, RGB(255, 0, 0));
	PGREEN = CreatePen (0, 1, RGB(0, 128, 0));
	PLGREEN = CreatePen (0, 1, RGB(0, 255, 0));
	PBLUE = CreatePen (0, 1, RGB(0, 0, 128));
	PLBLUE = CreatePen (0, 1, RGB(0, 0, 255));
	PGREY = CreatePen(0, 1, RGB(128, 128, 128));
	PLGREY = CreatePen(0, 1, RGB(192, 192, 192));
	
	// Initialize Brushes
	BBLACK = CreateSolidBrush(RGB(0, 0, 0));
	BWHITE = CreateSolidBrush(RGB(255, 255, 255));
	BRED = CreateSolidBrush(RGB(128, 0, 0));
	BLRED = CreateSolidBrush(RGB(255, 0, 0));
	BGREEN = CreateSolidBrush(RGB(0, 128, 0));
	BLGREEN = CreateSolidBrush(RGB(0, 255, 0));
	BBLUE = CreateSolidBrush(RGB(0, 0, 128));
	BLBLUE = CreateSolidBrush(RGB(0, 0, 255));
	BGREY = CreateSolidBrush(RGB(128, 128, 128));
	BLGREY = CreateSolidBrush(RGB(192, 192, 192));
}

StdColors::~StdColors ()
{
	// Delete Pens
	DeleteObject (PBLACK); DeleteObject (PWHITE); DeleteObject (PRED); DeleteObject (PLRED);
	DeleteObject (PGREEN); DeleteObject (PLGREEN); DeleteObject (PBLUE); DeleteObject (PLBLUE);
	DeleteObject (PGREY); DeleteObject (PLGREY);
	
	// Delete Brushes
	DeleteObject (BBLACK); DeleteObject (BWHITE); DeleteObject (BRED); DeleteObject (BLRED);
	DeleteObject (BGREEN); DeleteObject (BLGREEN); DeleteObject (BBLUE); DeleteObject (BLBLUE);
	DeleteObject (BGREY); DeleteObject (BLGREY);
}
///////////////////////////////////////////////////////////////////////////
///////////////////		Class: StdColors	///////////////////////////////
///////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////
///////////////////		Class: StdFonts		///////////////////////////////
///////////////////////////////////////////////////////////////////////////
StdFonts::StdFonts ()
{
	TXT_MSSANS = "MS Sans Serif";
	MSSANS = CreateFont (8, 0, 0, 0, FW_NORMAL, false, false, false, DEFAULT_CHARSET, 
							OUT_TT_PRECIS, CLIP_LH_ANGLES, PROOF_QUALITY, 4, &TXT_MSSANS[0]);
}

StdFonts::~StdFonts ()
{
	DeleteObject (MSSANS);
}
///////////////////////////////////////////////////////////////////////////
///////////////////		Class: StdFonts		///////////////////////////////
///////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////
///////////////////		Class: FadeScreen	///////////////////////////////
///////////////////////////////////////////////////////////////////////////
const char FadeScreen::WNDCLASS[]					=		"GDI_Fader";

FadeScreen::FadeScreen ()
{
	lock = false;
	sw = ScreenWidth(); sh = ScreenHeight();
	deskdc = DisplayDC();
	
	// Register Window Class if necessary
	NewWindowClass (WNDCLASS, &Callback_WndProc, 0, 
					CS_OWNDC, 
					0, 0, NULL, true);
	
	// Create FS Window
	swnd = CreateWindowEx (
				WS_EX_TOOLWINDOW | WS_EX_TOPMOST, 
				WNDCLASS, "(***) Fader", 
				WS_DISABLED, 
				0, 0, sw, sh, 0, 0, 0, 0);
	SetWindowLongPtr (swnd, GWLP_USERDATA, (LONG_PTR) this);
	
	// Setup Window & Create Blitting Surface
	RemoveStyle (swnd, WS_CAPTION);
	sdc = swnd_mem.Create (swnd, sw, sh);
}

FadeScreen::~FadeScreen ()
{
	swnd_mem.Free();
	DeleteDC (deskdc);
	DestroyWindow (swnd);
	UnregisterClass (WNDCLASS, 0);
}

void FadeScreen::Fade (int mode)														// public
{
	// Enter Critical Section
	cs.Enter();

	// Perform Screen Fading
	if (mode == 1)
	{
		char* d = DIB_CreateArray (sw, sh);
		DIB_HDC2RGB (deskdc, sw, sh, d);
		DIB_RGB2HDC (sdc, sw, sh, d);
		DIB_DeleteArray (d);
		
		MeRefresh ();
		ShowWindow (swnd, SW_SHOW);
		
		long step = 30;
		for (long y = 0; y <= sh; y=y+2)
		{
			Line2 (sdc, 0, y, sw, y, sc.PBLACK);
			if (((double) y/step) == floor(((double) y/step)))
			{
				MeRefreshTicks (30);
			}
		}
		for (long x = 0; x <= sw; x=x+2)
		{
			Line2 (sdc, x, 0, x, sh, sc.PBLACK);
			if (((double) x/step) == floor(((double) x/step)))
			{
				MeRefreshTicks (30);
			}
		}
		MeRefresh ();
	}
	else if (mode == 2)
	{
		char* d = DIB_CreateArray (sw, sh);
		DIB_HDC2RGB (deskdc, sw, sh, d);
		DIB_RGB2HDC (sdc, sw, sh, d);
		DIB_DeleteArray (d);
		
		MeRefresh ();
		ShowWindow (swnd, SW_SHOW);
		
		long step = 10;
		long x = 0, y = 0;
		while ((x < (sw/2)+1) || (y < (sh/2)+1))
		{
			if (((double) y/step) == floor(((double) y/step)))
			{
				MeRefreshTicks (30);
			}
			Line2 (sdc, x, 0, x, sh, sc.PBLACK);
			Line2 (sdc, sw-x, 0, sw-x, sh, sc.PBLACK);
			Line2 (sdc, 0, y, sw, y, sc.PBLACK);
			Line2 (sdc, 0, sh-y, sw, sh-y, sc.PBLACK);
			x = x + 2;
			y = y + 2;
		}
		MeRefresh();
	}
	else if (mode == 3)
	{
		ShowWindow (swnd, SW_HIDE);
	}

	// Leave Critical Section
	cs.Leave();
}

LRESULT CALLBACK FadeScreen::Callback_WndProc (HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)			// protected
{
	FadeScreen* f = (FadeScreen*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if (f != 0)
	{
		return f->WndProc(hwnd, umsg, wparam, lparam);
	}
	return DefWindowProc(hwnd, umsg, wparam, lparam);
}

LRESULT FadeScreen::WndProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)								// public
{
	// No CS in Window Process, Risk of Deadlocks!
	if (msg == WM_PAINT)
	{
		MeRefresh();
		return false;
	}
	return DefWindowProc (hwnd, msg, wparam, lparam);
}

void FadeScreen::MeRefresh ()																				// private
{
	// Protected by calling function: Fade
	swnd_mem.Refresh();
}

void FadeScreen::MeRefreshTicks (long ticks)																// private
{
	// Protected by calling function: Fade
	DWORD t = GetTickCount() + ticks;
	MeRefresh();
	while (GetTickCount() < t);
}
///////////////////////////////////////////////////////////////////////////
///////////////////		Class: FadeScreen	///////////////////////////////
///////////////////////////////////////////////////////////////////////////



//////////////////////////////////
// Extended Window-Handling		//
//////////////////////////////////
HWND ActivateWindow (HWND hwnd)
{
	HWND oldactwnd = GetForegroundWindow();
	DWORD extthread = GetWindowThreadProcessId (oldactwnd, NULL);
	DWORD mythread = GetWindowThreadProcessId (hwnd, NULL);
	if (extthread != mythread)
	{
		AttachThreadInput (mythread, extthread, true);
	}
	SetForegroundWindow (hwnd);
	return oldactwnd;
}

void DeActivateWindow (HWND oldactwnd)
{
	DWORD extthread = GetWindowThreadProcessId (oldactwnd, NULL);
	DWORD mythread = GetCurrentThreadId();
	if (extthread != mythread)
	{
		AttachThreadInput (mythread, extthread, false);
	}
}

void SetForegroundWindowEx (HWND forewnd)
{
	DeActivateWindow (ActivateWindow(forewnd));
	SetWindowPos (forewnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
}

void ExitAppWindow (HWND hwnd)
{
	if (hwnd != 0) { SendNotifyMessage (hwnd, WM_SYSCOMMAND, SC_CLOSE, 0); };
}


//////////////////////////////
// Window Style Management	//
//////////////////////////////
bool StyleExists(HWND hwnd, long thestyle)
{
	LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
	if (style & thestyle) { return true; } else { return false; };
}

bool ExStyleExists(HWND hwnd, long thestyle)
{
	LONG_PTR exstyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
	if (exstyle & thestyle) { return true; } else { return false; };
}

void AddStyle(HWND hwnd, long thestyle)
{
	LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
	style |= thestyle;
	SetWindowLongPtr (hwnd, GWL_STYLE, style);
	GWLnotify (hwnd);
}

void AddExStyle(HWND hwnd, long thestyle)
{
	LONG_PTR exstyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
	exstyle |= thestyle;
	SetWindowLongPtr (hwnd, GWL_EXSTYLE, exstyle);
	GWLnotify (hwnd);
}

void RemoveStyle (HWND hwnd, long thestyle)
{
	LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
	DelFlag (&style, thestyle);
	SetWindowLongPtr (hwnd, GWL_STYLE, style);
	GWLnotify (hwnd);
}

void RemoveExStyle (HWND hwnd, long thestyle)
{
	LONG_PTR exstyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
	DelFlag (&exstyle, thestyle);
	SetWindowLongPtr (hwnd, GWL_EXSTYLE, exstyle);
	GWLnotify (hwnd);
}

void AdjustStyle (HWND hwnd, long masterstyle, long thestyle)
{
	if (masterstyle & thestyle)
	{
		AddStyle (hwnd, thestyle);
	}
	else
	{
		RemoveStyle (hwnd, thestyle);
	}
}

void GWLnotify(HWND hwnd)
{
	SetWindowPos (hwnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
}


//////////////////////////
// Window Information	//
//////////////////////////
HWND GetAbsParent (HWND window)
{
	HWND tmp;
	HWND parent = window;
	while (true)
	{
		tmp = GetParent(parent);
		if (tmp != 0) { parent = tmp; } else { break; };
	}
	return parent;
}

bool WindowVisible (HWND hwnd)
{
	LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
	LONG_PTR exstyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
	if ((Flag(exstyle, WS_EX_APPWINDOW) && Flag(style, WS_VISIBLE)) || 
	   (Flag(style, WS_GROUP) && Flag(style, WS_VISIBLE)))
	{ return true; } else { return false; };
}

HICON GetWindowIcon (HWND hwnd, int gwi_size)
{
	HICON icon = 0;
	PDWORD_PTR pRes = 0;
	
	if (gwi_size == GWI_SMALL)
	{
		SendMessageTimeout (hwnd, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG, 1, pRes);
		icon = (HICON) *pRes;
		if (icon == 0) { icon = (HICON) GetClassLongPtr (hwnd, GCLP_HICONSM); };
		if (icon == 0)
		{
			SendMessageTimeout (hwnd, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG, 1, pRes);
			icon = (HICON)*pRes;
		}
		if (icon == 0) { icon = (HICON) GetClassLongPtr (hwnd, GCLP_HICON); };
	}
	else
	{
		SendMessageTimeout (hwnd, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG, 1, pRes);
		icon = (HICON)*pRes;
		if (icon == 0) { icon = (HICON) GetClassLongPtr (hwnd, GCLP_HICON); };
		if (icon == 0)
		{
			SendMessageTimeout (hwnd, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG, 1, pRes);
			icon = (HICON) *pRes;
		}
		if (icon == 0) { icon = (HICON) GetClassLongPtr (hwnd, GCLP_HICONSM); };
	}
	return icon;
}

bool WindowHung (HWND hwnd)
{
	PDWORD_PTR pRet = 0;
	LRESULT res = SendMessageTimeout (hwnd, WM_GETICON, 0, 0, SMTO_ABORTIFHUNG, 1, pRet);
	if (res == 0) { return true; } else { return false; };
}

bool CheckWindowCoords (HWND hwnd, long lngOverrideWindowLeft, long lngOverrideWindowTop)
{
	RECT rcWorkarea;
	RECT rcWindowCA;
	
	// Get Window Coordinate Information.
	if (hwnd != 0)
	{
		GetWindowRect (hwnd, &rcWindowCA);
	}
	else
	{
		// Null Window RECT.
		rcWindowCA.left = rcWindowCA.right = rcWindowCA.top = rcWindowCA.bottom = 0;
	}

	// Use overrides if defined.
	if (lngOverrideWindowLeft != -1)
	{ 
		rcWindowCA.left = lngOverrideWindowLeft; 
		rcWindowCA.right = rcWindowCA.left + 5;				// We need a spot to look for a corresponding monitor and workarea.
	}

	if (lngOverrideWindowTop != -1)	
	{
		rcWindowCA.top = lngOverrideWindowTop;
		rcWindowCA.bottom = rcWindowCA.top + 5;				// We need a spot to look for a corresponding monitor and workarea.
	}

	// Get Workarea Coordinate Information.
	GetWorkareaForScreenPart (rcWindowCA, &rcWorkarea);

	// Return if the Window lies within allowed workarea coordinate range of the hosting monitor.
	return !(
				((rcWindowCA.left < rcWorkarea.left) || (rcWindowCA.left > rcWorkarea.right)) || 
				((rcWindowCA.top < rcWorkarea.top) || (rcWindowCA.top > rcWorkarea.bottom))
																		);
}

long GetWindowCaptionHeight ()
{
	NONCLIENTMETRICS nonclientmetrics;
	nonclientmetrics.cbSize = sizeof(nonclientmetrics);
	SystemParametersInfo (SPI_GETNONCLIENTMETRICS, sizeof(nonclientmetrics), &nonclientmetrics, 0);
	return nonclientmetrics.iCaptionHeight;
}



//////////////////////////////////
// Multiple Monitor Support		//
//////////////////////////////////
bool GetActiveDisplayDevices (StructArray <DISPLAY_DEVICE> *parDisplayDevice, StructArray <DEVMODE> *parDevMode)
{
	// Variables.
	DISPLAY_DEVICE displayDevice;
	DISPLAY_DEVICE monitor;
	DEVMODE devmode;
	int deviceIndex = 0;
	int monitorCount = 0;
	long u;
    BOOL bResult;
    
	// Fill structures.
	displayDevice.cb = sizeof(displayDevice);
	monitor.cb = sizeof(monitor);
	devmode.dmSize = sizeof(devmode);

	// Clear Return Array first.
	parDisplayDevice->Clear();
	parDevMode->Clear();
	u = 0;

	// Enumerate Display Devices.
	do
    {
		bResult = EnumDisplayDevices(NULL, deviceIndex++, &displayDevice, 0);
		if (displayDevice.StateFlags & DISPLAY_DEVICE_ACTIVE)
		{
			// Get Details of the currently found monitor.
			monitorCount++;
			EnumDisplayDevices(displayDevice.DeviceName, 0, &monitor, 0);
			
			// Get Display Settings of that monitor by its name.
			if (EnumDisplaySettings(displayDevice.DeviceName, ENUM_CURRENT_SETTINGS, &devmode))
			{
				// Data collection completed. We'll now add it to the return array.
				parDisplayDevice->Append (displayDevice);
				parDevMode->Append (devmode);
			}
		}
    } while (bResult);

	// Return Success.
	return TRUE;
}

bool GetWorkareaForWindow (HWND hwnd, RECT* rcWorkarea)
{
	RECT rcWindow;
	
	// Get Window RECT first.
	GetWindowRect(hwnd, &rcWindow);
	return GetWorkareaForScreenPart(rcWindow, rcWorkarea);
}

bool GetWorkareaForScreenPart (RECT& rcScreenPart, RECT* rcWorkarea)
{
	// Variables.
	HMONITOR hMonitor;
	MONITORINFO monitorinfo;

	// Check what workarea applies to monitor the window is currently displayed on.
	hMonitor = MonitorFromRect(&rcScreenPart, MONITOR_DEFAULTTONEAREST);
	monitorinfo.cbSize = sizeof(monitorinfo);
	GetMonitorInfo(hMonitor, &monitorinfo);

	// Return workarea for that window and monitor constellation.
	CopyMemory (rcWorkarea, &monitorinfo.rcWork, sizeof(RECT));

	// Return Success.
	return TRUE;
}


//////////////////////
// Window Macros	//
//////////////////////
void RfMsgQueue		(HWND hwnd)	{ PostMessage (hwnd, WM_PAINT, 0, 0); };
bool IsWsVisible	(HWND hwnd)	{ return StyleExists(hwnd, WS_VISIBLE); };
HWND ExplorerWindow	()			{ return FindWindow("progman", "program manager"); };


//////////////////////////
// Window Creation		//
//////////////////////////
void NewWindowClass (string classname, WNDPROC wndproc, HINSTANCE hinst, long style, 
					 HICON hicon, HICON hsmicon, const char* menu, bool nobackpaint)
{
	// Fill the Class Structure
	WNDCLASSEX wcx;
	wcx.cbClsExtra = 0;
	wcx.cbWndExtra = 0;
	
	if (!nobackpaint)
	{ wcx.hbrBackground = (HBRUSH) COLOR_WINDOW; } else { wcx.hbrBackground = 0; };
	
	wcx.hCursor = LoadCursor(0, IDC_ARROW);
	wcx.hIcon = hicon;
	wcx.hIconSm = hsmicon;
	wcx.hInstance = hinst;
	wcx.lpfnWndProc = wndproc;
	wcx.lpszClassName = &classname[0];
	wcx.lpszMenuName = menu;
	style |= CS_HREDRAW;
	style |= CS_VREDRAW;
	wcx.style = style;
	wcx.cbSize = sizeof(WNDCLASSEX);
	
	// Register Window Class if it does not already exist
	RegisterClassEx (&wcx);
}

void RegisterNullWindowClass ()
{
	if (!wcnull_loaded)
	{
		NewWindowClass (WC_NULL, &DefWindowProc, 0, 0, 0, 0, NULL, true);
		wcnull_loaded = true;
	}
}

void UnregisterNullWindowClass ()
{
	if (wcnull_loaded)
	{
		UnregisterClass (WC_NULL, 0);
		wcnull_loaded = false;
	}
}

HWND CreateMainAppWindow (string classname, string title, DWORD exstyle, DWORD style, 
							LPVOID lparam, HINSTANCE hinstance)
{
	exstyle |= WS_EX_TOOLWINDOW;	// Hide from ALT+TAB Manager
	style |= WS_VISIBLE;			// Needed for being noticed by Strg+Alt+Entf Manager
	style |= WS_POPUP;
	style |= WS_SYSMENU;			// Gives Strg+Alt+Entf Manager the chance for SC_CLOSE
	return CreateWindowEx (exstyle, &classname[0], &title[0], style, 
							0, 0, 0, 0, 0, 0, hinstance, lparam);
}

HWND CreateNullWindow (string title, LPVOID lparam, string classname)
{
	if (xlen(classname) == 0)		// No class has been specified
	{
		RegisterNullWindowClass();
		classname = WC_NULL;
	}
	return CreateWindowEx (0, &classname[0],  &title[0], 0, 0, 0, 0, 0, 0, 0, 0, lparam);
}
