// ControlUI.cpp: Macro functions to use the Common Controls
//
// Thread-Safe: YES
//


//////////////////
// Includes		//
//////////////////
#include "stdafx.h"

#include "_modules.h"


//////////////////////
// General Macros	//
//////////////////////
HWND dia (HWND dialog, int ctlid)
{
	return GetDlgItem (dialog, ctlid);
}

int MessageBox (HWND hwnd, string text, string caption, UINT flags)
{
	return MessageBox (hwnd, &text[0], &caption[0], flags);
}

string ctlGetText (HWND ctrl)
{
	string txt; long len;
	len = (long) SendMessage (ctrl, WM_GETTEXTLENGTH, 0, 0) + 1;
	txt.resize (len);
	len = (long) SendMessage (ctrl, WM_GETTEXT, len, (LPARAM) &txt[0]);
	txt.resize (len);
	return txt;
}

void ctlSetText (HWND ctrl, string text)
{
	SendMessage (ctrl, WM_SETTEXT, 0, (LPARAM) &text[0]);
}

void ctlSetFont (HWND ctrl, HFONT font)
{
	SendMessage (ctrl, WM_SETFONT, (WPARAM) font, MAKELPARAM(true, 0));
}

bool Button_Clicked (WPARAM wparam, long id)
{
	return ((LOWORD(wparam) == id) && (HIWORD(wparam) == BN_CLICKED));
}


//////////////////////////
// Macros: Check Box	//
//////////////////////////
long chkGetState (HWND ctrl)
{
	return (long) SendMessage (ctrl, BM_GETCHECK, 0, 0);
}

void chkSetState (HWND ctrl, long state)
{
	SendMessage (ctrl, BM_SETCHECK, state, 0);
}


//////////////////////////
// Macros: ComboBox		//
//////////////////////////
void cbSetHeight (HWND ctrl, long maxheight)
{
	RECT rc; GetClientRect (ctrl, &rc);
	SetWindowPos (ctrl, 0, 0, 0, rc.right, maxheight, SWP_NOMOVE | SWP_NOACTIVATE);
}

void cbAdd (HWND ctrl, string txt)
{
	SendMessage (ctrl, CB_ADDSTRING, 0, (LPARAM) &txt[0]);
}

long cbCount (HWND ctrl)
{
	return (long) SendMessage (ctrl, CB_GETCOUNT, 0, 0);
}

long cbSelected (HWND ctrl)
{
	return (long) SendMessage (ctrl, CB_GETCURSEL, 0, 0);
}

string cbText (HWND ctrl, long index)
{
	long len = (long) SendMessage (ctrl, CB_GETLBTEXTLEN, index, 0);
	string buf; buf.resize (len + 1);
	len = (long) SendMessage (ctrl, CB_GETLBTEXT, index, (LPARAM) &buf[0]);
	buf.resize (len); return buf;
}

string cbAbsText (HWND ctrl)
{
	long sel = cbSelected(ctrl);				// Combo Selection
	if (sel == CB_ERR)							// User typed custom text
	{
		return ctlGetText (ctrl);
	}
	else
	{
		return cbText (ctrl, sel);
	}
}


//////////////////////////
// Macros: List Box		//
//////////////////////////
long lbAdd (HWND lbwnd, string text)
{
	return (long) SendMessage (lbwnd, LB_ADDSTRING, 0, (LPARAM) &text[0]);
}

long lbInsert (HWND lbwnd, string text, long index)
{
	return (long) SendMessage (lbwnd, LB_INSERTSTRING, index, (LPARAM) &text[0]);
}

void lbRemove (HWND lbwnd, long index)
{
	SendMessage (lbwnd, LB_DELETESTRING, index, 0);
}

void lbClear (HWND lbwnd)
{
	SendMessage (lbwnd, LB_RESETCONTENT, 0, 0);
}

long lbCount (HWND lbwnd)
{
	return (long) SendMessage (lbwnd, LB_GETCOUNT, 0, 0);
}

string lbText (HWND lbwnd, long index)
{
	long len = (long) SendMessage (lbwnd, LB_GETTEXTLEN, index, 0);
	string buf;
	if (len != LB_ERR)
	{
		buf.resize (len + 1);
		len = (long) SendMessage (lbwnd, LB_GETTEXT, index, (LPARAM) &buf[0]);
		buf.resize (len);
	}
	return buf;
}

long lbGetItemData (HWND lbwnd, long lngIndex)
{
	return (long) SendMessage(lbwnd, LB_GETITEMDATA, lngIndex, 0);
}

bool lbSetItemData (HWND lbwnd, long lngIndex, long lngUserData)
{
	// Returns if the operation succeeded.
	return ((long) SendMessage(lbwnd, LB_SETITEMDATA, lngIndex, lngUserData) != LB_ERR);
}

void lbSelectSingle (HWND lbwnd, long index)
{
	SendMessage (lbwnd, LB_SETCURSEL, index, 0);
}

bool lbSelectMulti (HWND lbwnd, long lngIndex, bool bSelectState)
{
	// Returns if the operation succeeded.
	return (SendMessage (lbwnd, LB_SETSEL, (bSelectState ? TRUE : FALSE), lngIndex) != LB_ERR);
}

long lbSelectedItem (HWND lbwnd)
{
	return (long) SendMessage (lbwnd, LB_GETCURSEL, 0, 0);
}

long lbGetSelItems (HWND lbwnd, long* lngArraySelIDs, long lngArraySize)
{
	// Return Value: Selected Item Count
	return (long) SendMessage (lbwnd, LB_GETSELITEMS, lngArraySize, (LPARAM) lngArraySelIDs);
}


//////////////////////////
// Macros: List View	//
//////////////////////////
void lvAddColumn (HWND lwnd, long index, string caption, long align, long width)
{
	LV_COLUMN newcol;
	newcol.pszText = &caption[0];
	newcol.cchTextMax = xlen(caption);
	newcol.fmt = align;
	newcol.cx = width;
	newcol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	SendMessage (lwnd, LVM_INSERTCOLUMN, index, (LPARAM) &newcol);
}

long lvAdd (HWND lwnd, long subindex, long index, 
				 string text, UINT state, LVI* lvi, UINT mask)
{
	// Item Position
	lvi->iItem = index;
	lvi->iSubItem = subindex;
	
	// Item state?
	lvi->state = state;
	lvi->stateMask = 0;
	mask |= LVIF_STATE;
	
	// Item text?
	if (xlen(text) != 0)
	{
		lvi->pszText = &text[0];
		lvi->cchTextMax = xlen(text);
		mask |= LVIF_TEXT;
	}
	lvi->mask = mask;
	return (long) SendMessage (lwnd, LVM_INSERTITEM, 0, (LPARAM) lvi);
}

void lvRemove (HWND lwnd, long index)
{
	SendMessage (lwnd, LVM_DELETEITEM, index, 0);
}

void lvClear (HWND lwnd)
{
	SendMessage (lwnd, LVM_DELETEALLITEMS, 0, 0);
}

long lvCount (HWND lwnd)
{
	return (long) SendMessage (lwnd, LVM_GETITEMCOUNT, 0, 0);
}

long lvSelectedItem (HWND lwnd)
{
	return (long) SendMessage (lwnd, LVM_GETNEXTITEM, -1, MAKELPARAM(LVNI_SELECTED, 0));
}

bool lvSelected (HWND lwnd, long index)
{
	if (SendMessage(lwnd, LVM_GETITEMSTATE, index, LVIS_SELECTED) & LVIS_SELECTED)
	{ return true; } else { return false; };
}

void lvSelect (HWND lwnd, long index, bool state)
{
	LVI lvi;
	lvi.mask = LVIF_STATE;
	lvi.stateMask = LVIS_SELECTED;
	
	long u = lvCount(lwnd)-1;
	for (long i = 0; i <= u; i++)
	{
		if ((lvSelected(lwnd, i) == state) && (i != index))
		{
			lvi.state = LVIS_SELECTED*(!state);
			SendMessage (lwnd, LVM_SETITEMSTATE, i, (LPARAM) &lvi);
		}
		else if ((lvSelected(lwnd, i) != state) && (i == index))
		{
			lvi.state = LVIS_SELECTED*(state);
			SendMessage (lwnd, LVM_SETITEMSTATE, i, (LPARAM) &lvi);
		}
	}
}

void lvSetItemFocus (HWND lwnd, long index)
{
	LVI lvi;
	lvi.mask = LVIF_STATE;
	lvi.stateMask = LVIS_FOCUSED;
	lvi.state = LVIS_FOCUSED;
	SendMessage (lwnd, LVM_SETITEMSTATE, index, (LPARAM) &lvi);
}

string lvGetItemText (HWND lwnd, long subindex, long index, long maxlen)
{
	string buf; buf.resize (maxlen);
	LVI lvi;
	
	// Get item text
	lvi.iItem = index;
	lvi.iSubItem = subindex;
	lvi.pszText = &buf[0];
	lvi.cchTextMax = xlen(buf);
	
	buf.resize ((long) SendMessage(lwnd, LVM_GETITEMTEXT, index, (LPARAM) &lvi));
	return buf;
}

void lvSetItemText (HWND lwnd, long subindex, long index, string text)
{
	LVI lvi;
	lvi.iItem = index;
	lvi.iSubItem = subindex;
	lvi.pszText = &text[0];
	lvi.cchTextMax = xlen(text);
	SendMessage (lwnd, LVM_SETITEMTEXT, index, (LPARAM) &lvi);
}

void lvItemRect (HWND lwnd, long index, RECT* rc, long mode)
{
	rc->left = mode;
	SendMessage (lwnd, LVM_GETITEMRECT, index, (LPARAM) rc);
}

long lvClickedItem (HWND lwnd, long x, long y)
{
	RECT rc;
	long res = -1;
	long u = lvCount(lwnd);
	for (long i = 1; i <= u; i++)
	{
		lvItemRect (lwnd, i - 1, &rc, LVIR_BOUNDS);
		if (PointInRect(x, y, rc.left, rc.top, rc.right, rc.bottom))
		{
			res = i - 1; break;
		}
	}
	return res;
}


//////////////////////////
// Macros: Toolbar		//
//////////////////////////
void CreateToolbarButton (TBBUTTON* tb, long bitmap, long idcmd, 
						  unsigned char state, unsigned char style, long strid)
{
	tb->iBitmap = bitmap;
	tb->idCommand = idcmd;
	tb->fsState = state;
	tb->fsStyle = style;
	tb->iString = strid;
	tb->dwData = 0;
}

void tbSetButtonSize (HWND twnd, long width, long height)
{
	SendMessage (twnd, TB_SETBUTTONSIZE, 0, MAKELONG(width, height));
}

void tbAutoSize (HWND twnd) { SendMessage (twnd, TB_AUTOSIZE, 0, 0); };


//////////////////////////
// Macros: Tree-View	//
//////////////////////////
HTREEITEM tvAdd (HWND twnd, 
				 HTREEITEM parent, HTREEITEM position, int ischild, 
				 string text, UINT state, TVINS* tvins, UINT mask)
{
	tvins->hParent = parent;
	tvins->hInsertAfter = position;
	
	// Item state?
	tvins->item.state = state;
	mask |= TVIF_STATE;
	
	// Item has childs?
	tvins->item.cChildren = ischild;
	mask |= TVIF_CHILDREN;
	
	// Item text?
	if (xlen(text) != 0)
	{
		tvins->item.pszText = &text[0];
		tvins->item.cchTextMax = xlen(text);
		mask |= TVIF_TEXT;
	}
	tvins->item.mask = mask;
	return (HTREEITEM) SendMessage (twnd, TVM_INSERTITEM, 0, (LPARAM) tvins);
}

void tvDelete (HWND twnd, HTREEITEM item)
{
	SendMessage (twnd, TVM_DELETEITEM, 0, (LPARAM) item);
}

void tvClear (HWND twnd)
{
	TreeView_DeleteAllItems (twnd);
}

void tvSetIndent (HWND twnd, long width)
{
	SendMessage (twnd, TVM_SETINDENT, width, 0);
}

void tvSetImageList (HWND twnd, HIMAGELIST himl, long type)
{
	SendMessage (twnd, TVM_SETIMAGELIST, type, (LPARAM) himl);
}

void tvExpand (HWND twnd, HTREEITEM item, UINT flag)
{
	SendMessage (twnd, TVM_EXPAND, flag, (LPARAM) item);
}

void tvSelect (HWND twnd, HTREEITEM item, long flag)
{
	SendMessage (twnd, TVM_SELECTITEM, flag, (LPARAM) item);
}

HTREEITEM tvSelectedItem (HWND twnd)
{
	return (HTREEITEM) SendMessage (twnd, TVM_GETNEXTITEM, TVGN_CARET, 0);
}

HTREEITEM tvGetParent (HWND twnd, HTREEITEM item)
{
	return (HTREEITEM) SendMessage (twnd, TVM_GETNEXTITEM, TVGN_PARENT, (LPARAM) item);
}

void tvGetItem (HWND twnd, HTREEITEM item, UINT mask, TVI* tvi)
{
	tvi->hItem = item;
	tvi->mask = mask;
	SendMessage (twnd, TVM_GETITEM, 0, (LPARAM) tvi);
}

void tvSetItem (HWND twnd, HTREEITEM item, UINT mask, TVI* tvi)
{
	tvi->hItem = item;
	tvi->mask = mask;
	SendMessage (twnd, TVM_SETITEM, 0, (LPARAM) tvi);
}

string tvGetItemText (HWND twnd, HTREEITEM item, long maxlen, LRESULT* param)
{
	string buf; buf.resize (maxlen);
	TVI tvtemp;
	if (item == 0) { return ""; };
	
	// Get required data
	tvtemp.cchTextMax = xlen(buf);
	tvtemp.pszText = &buf[0];
	tvGetItem (twnd, item, TVIF_TEXT | (TVIF_PARAM*(param != 0)), &tvtemp);
	
	if (param != 0) { *param = tvtemp.lParam; };
	return &buf[0];
}


//////////////////////////
// Macros: Trackbar		//
//////////////////////////
void trkSetRange (HWND trkwnd, long min, long max, bool redraw)
{
	SendMessage (trkwnd, TBM_SETRANGE, redraw, MAKELONG(min, max));
}

void trkSetThumbsize (HWND trkwnd, long size)
{
	SendMessage (trkwnd, TBM_SETTHUMBLENGTH, size, 0);
}

long trkGetPos (HWND trkwnd)
{
	return (long) SendMessage (trkwnd, TBM_GETPOS, 0, 0);
}

void trkSetPos (HWND trkwnd, long pos)
{
	SendMessage (trkwnd, TBM_SETPOS, true, pos);
}


//////////////////////////
// Macros: Tab Control	//
//////////////////////////
void tabInsert (HWND tabwnd, long index, string text, long param)
{
	TC_ITEM tci;
	tci.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM;
	tci.pszText = &text[0];
	tci.cchTextMax = xlen(text);
	tci.iImage = -1;
	tci.lParam = param;
	SendMessage (tabwnd, TCM_INSERTITEM, index, (LPARAM) &tci);
}

long tabSelected (HWND tabwnd)
{
	return (long) SendMessage (tabwnd, TCM_GETCURSEL, 0, 0);
}


//////////////////////////////
// Fast Control Creation	//
//////////////////////////////
HWND CreateButton (HWND parent, string caption, long x, long y, long width, long height, 
				   HINSTANCE hInstance, LPVOID lparam)
{
	// STANDARD HEIGHT: 23 PIXEL
	return CreateWindowEx (0, "BUTTON", &caption[0], 
							WS_VISIBLE | WS_CHILD | WS_TABSTOP | 
							BS_PUSHBUTTON, 
							x, y, width, height, parent, 0, hInstance, lparam);
}

HWND CreateEditbox (HWND parent, string caption, long x, long y, long width, 
					HINSTANCE hInstance, LPVOID lparam)
{
	// STANDARD HEIGHT: 19 PIXEL
	return CreateWindowEx (WS_EX_CLIENTEDGE, "EDIT", &caption[0], 
							WS_VISIBLE | WS_CHILD | WS_TABSTOP | 
							ES_AUTOHSCROLL, 
							x, y, width, 19, parent, 0, hInstance, lparam);
}


//////////////////////////////
// Tabbed Dialog Support	//
//////////////////////////////
TabDialog::TabDialog ()
{
	long baseunits;
	
	// Initialize
	parent = 0; mytab = 0; callback = 0; curdlg = 0;
	tabrc.x = 0; tabrc.y = 0; tabrc.w = 0; tabrc.h = 0;
	tab_running = false;
	
	// Read System Values
	baseunits = GetDialogBaseUnits();
	dlgbase_x = LOWORD(baseunits); dlgbase_y = HIWORD(baseunits);
}

TabDialog::~TabDialog ()
{
	// Shutdown if necessary
	if (tab_running) { StopTabs(); };
	
	// Free all resources
	for (long i = 1; i <= tdc.Ubound(); i++)
	{
		FreeResource (tdc[i].res);
	}
	
	// Clear the cache
	tdc.Clear();
}

bool TabDialog::SetupDone ()														// public
{
	bool res;

	// Enter Critical Section
	cs.Enter();
	
	// Check State
	res = ((parent != 0) && (callback != 0) && (tdc.Ubound() > 0));

	// Leave Critical Section
	cs.Leave();

	// Return Result
	return res;
}

void TabDialog::BasicSetup (HWND myparent, DLGPROC mycallback)						// public
{
	// Enter Critical Section
	cs.Enter();

	// Modify RAM
	parent = myparent;
	callback = mycallback;

	// Leave Critical Section
	cs.Leave();
}

void TabDialog::AddDialog (LPCTSTR residx, string caption)								// public
{
	long dlg_width, dlg_height, u;
	HRSRC dlgres;
	HGLOBAL dlgmem;
	DLGTEMPLATE* newdlg;
	
	// Enter Critical Section
	cs.Enter();

	// Find, Load and Lock the Dialog Resource
	dlgres = FindResource (0, (LPCTSTR) residx, RT_DIALOG);
	dlgmem = LoadResource (0, dlgres); 
    newdlg = (DLGTEMPLATE*) LockResource (dlgmem);
	
	// If everything went fine, add it to the cache
	u = tdc.Ubound() + 1;
	tdc.Redim (u);
	tdc[u].caption = caption;
	tdc[u].res = newdlg;
	
	// Update Minimal Area for the Tab Control
	dlg_width = MulDiv(newdlg->cx, dlgbase_x, 5);		// (!) MS recommended 4
	dlg_height = MulDiv(newdlg->cy, dlgbase_y, 8);
	if (dlg_width > tabrc.w)	{ tabrc.w = dlg_width; };
	if (dlg_height > tabrc.h)	{ tabrc.h = dlg_height; };

	// Leave Critical Section
	cs.Leave();
}

void TabDialog::RemoveDialog (long index)											// public
{
	// Enter Critical Section
	cs.Enter();

	// Check if state and index is valid
	if (tab_running || (index < 1) || (index > tdc.Ubound()))
	{
		// Leave Critical Section before return
		cs.Leave();
		return;
	}
	
	// Free Dialog Resource
	FreeResource (tdc[index].res);
	
	// Kill entry
	tdc.Remove (index);

	// Leave Critical Section
	cs.Leave();
}

void TabDialog::StartTabs (long x, long y)											// public
{
	long u, i;

	// Enter Critical Section
	cs.Enter();
	
	// Check for valid cache
	if (tab_running || !SetupDone())
	{
		// Leave Critical Section before return
		cs.Leave();
		return;
	}
	
	// Create the Tab Control
	tabrc.x = x; tabrc.y = y;
	mytab = CreateWindowEx (0, WC_TABCONTROL, "*TabDialog*", 
								WS_CHILD, 
								tabrc.x, tabrc.y, tabrc.w, tabrc.h, 
								parent, 0, 0, 0);
	if (mytab == 0)
	{
		// Leave Critical Section before return
		cs.Leave();

		// Debug Message
		DebugOut ("Failed to create Tabbed Dialog!");
		return;
	}

	// Set Tabs' Font
	ctlSetFont (mytab, locsf.MSSANS);
	
	// Loop through all dialogs
	u = tdc.Ubound();
	for (i = 1; i <= u; i++)
	{
		// Create the Dialog
		tdc[i].hwnd = CreateDialogIndirect (0, tdc[i].res, parent, callback);
		ShowWindow (tdc[i].hwnd, SW_HIDE);
		
		// Insert Tab
		tabInsert (mytab, i, tdc[i].caption);
	}
	
	// Show the Tab Control
	ShowWindow (mytab, SW_SHOW);
	SwitchDialog (1);
	tab_running = true;

	// Leave Critical Section
	cs.Leave();
}

void TabDialog::StopTabs ()															// public
{
	long u, i;

	// Enter Critical Section
	cs.Enter();
	
	// Check if state is valid
	if (!tab_running || !SetupDone())
	{
		// Leave Critical Section before return
		cs.Leave();
		return;
	}
	
	// Loop through all dialogs
	u = tdc.Ubound();
	for (i = 1; i <= u; i++)
	{
		// Destroy the Dialog
		DestroyWindow (tdc[i].hwnd);
	}
	
	// Destroy the Tab Control
	DestroyWindow (mytab);
	mytab = 0;
	tab_running = false;

	// Leave Critical Section
	cs.Leave();
}

void TabDialog::NotifyMsg (LPARAM lparam)											// public
{
	NMHDR* nmhdr = (NMHDR*) lparam;

	// Enter Critical Section
	cs.Enter();

	// Act on Event
	if ((nmhdr->code == TCN_SELCHANGE) && (nmhdr->hwndFrom == mytab))
	{
		SwitchDialog (tabSelected(mytab)+1);
	}

	// Leave Critical Section
	cs.Leave();
}

void TabDialog::SwitchDialog (long index)											// public
{
	RECT rc;

	// Enter Critical Section
	cs.Enter();
	
	// Check if Index is valid
	if ((index < 1) && (index > tdc.Ubound()))
	{
		// Leave Critical Section before return
		cs.Leave();
		return;
	}
	
	// Hide the previous dialog if necessary
	if (curdlg != 0)
	{
		ShowWindow (tdc[curdlg].hwnd, SW_HIDE);
	}
	
	// Show the new dialog
	rc.left = tabrc.x; rc.right = rc.left + tabrc.w;
	rc.top = tabrc.y; rc.bottom = rc.top + tabrc.h;
	TabCtrl_AdjustRect (mytab, false, &rc);
	SetWindowPos (tdc[index].hwnd, 0, rc.left, rc.top, 0, 0, SWP_NOSIZE);
	ShowWindow (tdc[index].hwnd, SW_SHOW);
	
	// Update current index
	curdlg = index;

	// Leave Critical Section
	cs.Leave();
}

HWND TabDialog::DetWnd (long id)													// public
{
	HWND ctlid = 0;
	long u;

	// Enter Critical Section
	cs.Enter();
	
	// Search for the given Control ID
	u = tdc.Ubound();
	for (long i = 1; i <= u; i++)
	{
		ctlid = dia(tdc[i].hwnd, id);
		if (ctlid != 0) { break; };
	}

	// Leave Critical Section
	cs.Leave();

	// Return Control ID
	return ctlid;
}


