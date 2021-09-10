// TheString.cpp: Adds the String type known from Basic Applications to C++.
//
// Thread-Safe: YES
//


//////////////////
// Includes		//
//////////////////
#include "stdafx.h"
#include "math.h"

#include "_modules.h"


//////////////
// Consts	//
//////////////
const int MAX_GCVT						= 14;							// [14] = double


//////////////////////
// Functions: Main	//
//////////////////////
string::string ()						{ Init(); Assign(""); };

string::string (LPCTSTR init)			{ Init(); Assign (init); };
string::string (string& init)			{ Init(); Assign (init); };
string::string (long init)				{ Init(); Assign (CStr(init)); };
string::string (bool init)				{ Init(); Assign (CStr(init)); };

void string::operator= (LPCTSTR obj)	{ Assign (obj); };
void string::operator= (string& obj)	{ Assign (obj); };

string::~string ()
{
	Clear();
}

void string::Init ()
{
	mem = 0;
	len = 0;
}

void string::Assign (LPCTSTR obj)
{
	// Enter Critical Section
	cs.Enter();

	// Perform Assignment
	Clear();
	len = strlen(obj);
	mem = (char*) LocalAlloc (LMEM_FIXED, len+1);		// Include Null Character
	CopyMemory (mem, obj, len+1);						// Include Null Character

	// Leave Critical Section
	cs.Leave();
}

void string::Assign (string& obj)
{
	// Enter Critical Section
	cs.Enter();

	// Perform Assignment
	Clear();
	len = obj.len;
	mem = (char*) LocalAlloc (LMEM_FIXED, len+1);		// Include Null Character
	CopyMemory (mem, obj.mem, len+1);					// Include Null Character

	// Leave Critical Section
	cs.Leave();
}

void string::Append (LPCTSTR obj)
{
	size_t objlen, newlen;

	// Enter Critical Section
	cs.Enter();

	// Perform Append
	objlen = strlen(obj);
	newlen = len + objlen;
	mem = (char*) LocalReAlloc (mem, newlen+1, LMEM_MOVEABLE);		// Include Null Character
	CopyMemory (&mem[len], obj, objlen+1);							// Include Null Character
	len = newlen;

	// Leave Critical Section
	cs.Leave();
}

void string::Append (string obj)
{
	size_t objlen, newlen;

	// Enter Critical Section
	cs.Enter();

	// Perform Append
	objlen = obj.len;
	newlen = len + objlen;
	mem = (char*) LocalReAlloc (mem, newlen+1, LMEM_MOVEABLE);		// Include Null Character
	CopyMemory (&mem[len], obj.mem, objlen+1);						// Include Null Character
	len = newlen;

	// Leave Critical Section
	cs.Leave();
}

void string::Clear ()
{	
	// Enter Critical Section
	cs.Enter();

	// Free Memory if necessary
	if (mem != 0) { LocalFree (mem); mem = 0; len = 0; };

	// Leave Critical Section
	cs.Leave();
}


//////////////////////////////
// Functions: Operators		//
//////////////////////////////
char& string::operator[] (long idx)					{ return mem[idx]; };				// One Character
const char& string::operator[] (long idx) const		{ return mem[idx]; };				// Whole String

bool string::operator<  (string str)	{ return (intstreq(*this, str) < 0);  };
bool string::operator<= (string str)	{ return (intstreq(*this, str) <= 0); };
bool string::operator>  (string str)	{ return (intstreq(*this, str) > 0);  };
bool string::operator>= (string str)	{ return (intstreq(*this, str) >= 0); };
bool string::operator== (string str)	{ return   streq(*this, str);  };
bool string::operator!= (string str)	{ return (!streq(*this, str)); };

void string::operator+= (LPCTSTR obj)	{ Append (obj); };
void string::operator+= (string obj)	{ Append (obj); };

size_t string::length ()				{ return len; };								// String Length

void string::resize (size_t newlen)
{
	// Enter Critical Section
	cs.Enter();

	// Perform Reallocation of Memory
	mem = (char*) LocalReAlloc (mem, newlen+1, LMEM_MOVEABLE);		// Include Null Character		
	if (mem == 0)
	{
		// (!) Allocation failed
		cs.Leave();
		FatalAppExit (0, "TheString::Failed to allocate Memory");
		return;
	}
	mem[newlen] = '\0';												// Set Null Character
	len = newlen;

	// Leave Critical Section
	cs.Leave();
}

bool string::streq (string a, string b)
{
	bool equal = true;
	size_t both_strlength;

	// Enter Critical Section
	cs.Enter();

	// Compare the length of the two strings
	both_strlength = a.length();
	if (both_strlength != b.length())
	{
		// Leave Critical Section before return
		cs.Leave();
		return false;
	}

	// Compare the contents of the two strings with the same length
	for (long i = 1; i <= both_strlength; i++)
	{
		if (a[i-1] != b[i-1])
		{
			equal = false;
			break;
		}
	}

	// Leave Critical Section
	cs.Leave();

	// Return Result
	return equal;
}

int string::intstreq (string a, string b)
{
	size_t res = 0;
	size_t la, lb;

	// Enter Critical Section
	cs.Enter();

	// Do the comparison
	la = a.length();
	lb = b.length();
	for (long i = 1; i <= la; i++)
	{
		if (i > lb)			// If A is longer (A is greater) than B
		{
			res = la + 1;
			break;
		}
		else
		{
			if (a[i-1] != b[i-1])		// not =
			{
				if (a[i-1] < b[i-1])	// <
				{
					res = -i; break;
				}
				else					// >
				{
					res = i; break;
				}
			}
		}
	}

	// Leave Critical Section
	cs.Leave();

	// If B is longer (A is smaller) than A and the first part matches
	return ((res == 0) && (lb > la)) ? -((int) la + 1) : (int) res;
}

string string::mid (size_t start, size_t count)
{
	//
	// CAREFUL: Shared Variables: mem, len
	//
	size_t temp_buflen = len;
	string temp_buf;

	// Check Starting Stick
	if ((start < 1) || (start > temp_buflen))
	{
		// (!) Error
		return "";
	}

	// Check if we should extract until the end of the string
	if (count == -1)
	{
		// Extract till the end of the string
		count = temp_buflen - start + 1;
	}		

	// Check if Length is Invalid
	if (count < 1)
	{
		// (!) Error
		return "";
	}

	// Check Length of Extraction: Is Length to Extract > StrLength?
	if (count > temp_buflen - start + 1)
	{
		// Extract till the end of the string
		count = temp_buflen - start + 1;
	}

	// Prepare target buffer for extraction
	temp_buf.resize (count);

	// Enter Critical Section
	cs.Enter();

	// Perform Extraction into target buffer
	CopyMemory (&temp_buf[0], &mem[start-1], count);

	// Leave Critical Section
	cs.Leave();

	// Return Extracted String
	return temp_buf;
}

long string::split (StringArray* refStringArray, unsigned char ucDelimiter)
{
	//
	// Note: This function expects an input string like: <delimiter><data 1><delimiter><data 2><delimiter>
	//

	// Variables.
	long lngStartIdx = 0;
	long lngEndIdx;

	// Clear output array.
	refStringArray->Clear();

	// Enter Critical Section.
	cs.Enter();

	// Build output array.
	while ((lngStartIdx = this->InStrFirst(ucDelimiter, lngStartIdx + 1)) > 0)
	{
		lngEndIdx = this->InStrFirst(ucDelimiter, lngStartIdx + 1);
		refStringArray->Append (this->mid(lngStartIdx + 1, (lngEndIdx > 0) ? (lngEndIdx - lngStartIdx - 1) : -1));
		
		// For testing purposes only.
		// DebugOut (this->mid(lngStartIdx + 1, (lngEndIdx > 0) ? (lngEndIdx - lngStartIdx - 1) : -1));
	}

	// Leave Critical Section.
	cs.Leave();

	// Return array size of returned StringArray.
	return refStringArray->Ubound();
}

void string::cutoffLeftChars(size_t lngCharCount)
{
	//
	// CAREFUL: Shared Variables: mem, len
	//

	// Enter Critical Section
	cs.Enter();

	// Note: MoveMemory is the only function which allows overlapping memory blocks.
	MoveMemory (&mem[0], &mem[lngCharCount], len - lngCharCount);
	resize (len - lngCharCount);

	// Leave Critical Section
	cs.Leave();
}

long string::Replace (string what, string newstr)
{
	string tempstr;
	long fidx, cnt, startchar;

	// Enter Critical Section
	cs.Enter();

	// We copy the currently stored string into a temporary one first
	// The temporary string will be subsequently changed by this algorithm
	tempstr = *this;
	cnt = 0;
	startchar = 1;

	// Search for the first occurence of the "what"-String
	while ((fidx = InStr(tempstr, what, startchar)) > 0)
	{
		// We found the string at position #fidx
		tempstr = CSR(Left(tempstr, fidx - 1), newstr, tempstr.mid(fidx + what.length()));

		// Define where to start the search for the next string to replace
		startchar = (fidx - 1) + (long) newstr.length() + 1;

		// Increase "Made Replacements Counter"
		cnt++;
	}

	// Store tempstr into our current string (copy back)
	Assign (tempstr);

	// Leave Critical Section
	cs.Leave();

	// Return "Made Replacements Counter"
	return cnt;
}

long string::InStrFirst (unsigned char what, long start)
{
	//
	// CAREFUL: Shared Variables: mem, len
	//
	long pos = 0;
	long i;

	// Enter Critical Section
	cs.Enter();

	// Search for the given character in memory.
	for (i = start; i <= len; i++)
	{
		if (mem[i-1] == what) 
		{
			// Found it, return position.
			pos = i; 
			break; 
		}
	}

	// Leave Critical Section
	cs.Leave();

	// Return Result.
	return pos;
}


//////////////////////////
// String Searching		//
//////////////////////////
long InStrFirst (string str, unsigned char what, long start)
{
	return str.InStrFirst(what, start);
}

long InStrLast (string str, unsigned char what, long start)
{
	long pos = 0;

	if (start == -1) { start = xlen(str); };

	for (long i = start; i >= 1; i--)
	{
		if (str[i-1] == what) { pos = i; break; };
	}
	return pos;
}

long InStrFirstNot (string str, unsigned char what, long start)
{
	long pos = 0;
	long len = xlen(str);

	for (long i = start; i <= len; i++)
	{
		if (str[i-1] != what) { pos = i; break; };
	}
	return pos;
}

long InStrLastNot (string str, unsigned char what, long start)
{
	long pos = 0;

	if (start == -1) { start = xlen(str); };
	for (long i = start; i >= 1; i--)
	{
		if (str[i-1] != what) { pos = i; break; };
	}
	return pos;
}

long InStr (string str, string what, long start)
{
	bool success;
	long pos = 0;
	long slen = xlen(str); long wlen = xlen(what);
	long end = slen - wlen + 1;

	if ((end >= 0) && (end <= slen))		// What has same or smaller length as Str
	{
		for (long s = start; s <= end; s++)
		{
			success = true;
			for (long w = 1; w <= wlen; w++)
			{
				if (str[s+w-2] != what[w-1]) { success = false; break; };
			}
			if (success) { pos = s; break; };
		}
	}
	return pos;
}


//////////////////////////
// String Modification	//
//////////////////////////
string Left (string str, long len)
{
	return str.mid(1, len);
}

string Right (string str, long len)
{
	return str.mid(str.length()-len+1, len);
}

string UCase (LPCTSTR src)
{
	return UCase((string) src);
}

string UCase (string& src)
{
	unsigned char asc;
	long len = (long) src.length();
	string c;
	c.resize (len);
	
	for (long i = 1; i <= len; i++)
	{
		asc = src[i-1];
		if (((asc >= 97) && (asc <= 122)) ||			// a-z
			(asc == 228) ||								// ä
			(asc == 246) ||								// ö
			(asc == 252))								// ü
		{
			c[i-1] = asc - 32;
		}
		else
		{
			c[i-1] = asc;
		}
	}
	return c;
}

string LCase (LPCTSTR src)
{
	return LCase((string) src);
}

string LCase (string& src)
{
	unsigned char asc;
	long len = (long) src.length();
	string c;
	c.resize (len);
	
	for (long i = 1; i <= len; i++)
	{
		asc = src[i-1];
		if (((asc >= 65) && (asc <= 90)) ||				// A-Z
			(asc == 196) ||								// Ä
			(asc == 214) ||								// Ö
			(asc == 220))								// Ü
		{
			c[i-1] = asc + 32;
		}
		else
		{
			c[i-1] = asc;
		}
	}
	return c;
}


//////////////////////////
// String Conversion	//
//////////////////////////
char* AllocChar (string str)
{
	long len = xlen(str);
	char* buf = (char*) LocalAlloc(LMEM_FIXED, len + 1);		// Include Null Character
	CopyMemory (buf, &str[0], len + 1);		// Include Null Character
	return buf;
}

string Char2Str (char* obj, long len)
{
	if (len == -1) { len = (long) strlen(obj); };		// Find length
	string str; str.resize (len);
	CopyMemory (&str[0], obj, len);
	return str;
}

long DigitEnd (string numstr)
{
	long len = xlen(numstr);
	long end = len;
	for (long i = 1; i <= len; i++)
	{
		if ((numstr[i-1] < 48) || (numstr[i-1] > 57))      // not (0-9)
		{
			end = i - 1; break;
		}
	}
	return end;
}

long CLong (string str)
{
	long tmp = 0;
	long len = xlen(str);
	long end = DigitEnd(str);
	
	for (long i = 1; i <= len; i++)
	{
		if ((str[i-1] >= 49) && (str[i-1] <= 57))          // (1-9) = (49-57)
		{
			tmp = tmp + ((str[i-1] - 48) * (long) pow((long double) 10, (end - i)));
		}
	}
	return tmp;
}


//////////////////////////
// String Creation		//
//////////////////////////
string Chr (unsigned char asc)
{
	string buf; buf.resize (1); buf[0] = asc; return buf;
}

string CSR (string a, string b, string c, string d, string e)
{
	string all = a; all += b; all += c; all += d; all += e; return all;
}

string ChrString (long len, unsigned char asc)
{
	string neu; neu.resize (len);
	for (long i = 1; i <= len; i++)
	{
		neu[i-1] = asc;
	}
	return neu;
}

string FillStr (string str, long len, unsigned char asc)
{
	string tmp;

	if (xlen(str) < len)
	{
		// FillStr needed.
		tmp = ChrString(len - xlen(str), asc);
		tmp += str;
	}
	else
	{
		tmp = str;
	}
	return tmp;
}

string CStr (double number)
{
	char buf[25];
	_gcvt_s (&buf[0], sizeof(buf), number, MAX_GCVT);
	string ret = buf;
	if (ret[xlen(ret)-1] == '.') { ret = Left(ret, xlen(ret) - 1); };
	return ret;
}

string StringPart (string str, long which, unsigned char divchar)
{
	long pos = 0, lastpos = 0;
	string ret;
	for (long w = 1; w <= which; w++)
	{
		lastpos = pos;
		pos = InStrFirst (str, divchar, pos+1);
	}
	if (pos != 0) { ret = Mid(str, lastpos+1, pos-lastpos-1); };
	return ret;
}

void String2Buffer (string src, char *buf, long dstbytes)
{
	long oldlen = xlen(src);
	src.resize (dstbytes);
	if (dstbytes > oldlen)		// Size had to be increased
	{
		src[oldlen] = '\0';		// Append null character
	}
	CopyMemory (&buf[0], &src[0], dstbytes);
}

string pBuffer2String (LPSTR buf)
{
	return (buf != 0) ? buf : "";
}




/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////




//////////////////////////////
// Class: StringArray		//
//////////////////////////////
StringArray::StringArray ()
{
	//
}

StringArray::~StringArray ()
{
	Clear();
}

void StringArray::Clear ()
{
	sa.Clear();
}

long StringArray::Ubound ()																					// public
{
	long u;

	// Enter Critical Section
	cs.Enter();

	// Get Item Count of Array
	u = sa.Ubound();

	// Leave Critical Section
	cs.Leave();

	// Return Item Count of Array
	return u;
}

void StringArray::Redim (long nu)																			// public
{
	// CS
	cs.Enter();
	sa.Redim (nu);
	cs.Leave();
}

long StringArray::Append (string& strStringToAppend)														// public
{
	long new_u;

	// Enter CS.
	cs.Enter();

	// Append given String to StringArray.
	new_u = sa.Append (strStringToAppend);
	
	// Leave CS.
	cs.Leave();

	// Return new Ubound.
	return new_u;
}

void StringArray::Remove (long index)																		// public
{
	// CS
	cs.Enter();
	sa.Remove (index);
	cs.Leave();
}

void StringArray::Swap (long first, long second)															// public
{
	// CS
	cs.Enter();
	sa.Swap (first, second);
	cs.Leave();
}

void StringArray::operator= (StringArray& source)
{
	long i;
	long u;

	// Enter CS.
	cs.Enter();

	// Copy contents of the other class by regularly accessing them.
	u = source.Ubound();
	sa.Redim(u);
	for (i = 0; i <= u; i++)
	{
		// String assignment via public interface.
		sa[i] = source[i];
	}

	// Leave CS.
	cs.Leave();
}

string& StringArray::operator[] (long index)																// public
{
	// CAREFUL, No CS possible here!
	return sa[index];
}

bool StringArray::operator== (StringArray& target)
{
	bool bIdentic = true;
	long i;

	// Enter Critical Section
	cs.Enter();

	// Compare given String Arrays.
	if (sa.Ubound() != target.Ubound())
	{
		bIdentic = false;
	}
	else
	{
		for (i = 0; i <= sa.Ubound(); i++)
		{
			if (sa[i] != target.sa[i])
			{
				// Different entry found.
				bIdentic = false;
				break;
			}
		}
	}

	// Leave Critical Section
	cs.Leave();

	// Return if the two StringArrays hold the same content.
	return bIdentic;
}

bool StringArray::operator!= (StringArray& target)
{
	//
	// CriticalSection is done by "StringArray::operator==".
	//

	// Return if the two StringArrays hold the same content.
	return (!this->operator==(target));
}

string& StringArray::NAC (long index)																		// public
{
	// CAREFUL, No CS possible here!
	return sa[index+1];
}

void StringArray::HeapSort ()																				// public
{
	// From Index 1 to Ubound()
	long heap;

	// Enter Critical Section
	cs.Enter();
	
	// Perform HeapSort Algorithm
	heap = Ubound();
	for (long i = (heap / 2); i;)
	{
		AdjustHeap (--i, heap);
	}
	while (--heap)
	{
		Swap (1, heap+1);
		AdjustHeap (0, heap); 
	}

	// Leave Critical Section
	cs.Leave();
}

void StringArray::AdjustHeap (long kabutt, long heap)														// private
{
	// No CS needed, because: PRIVATE FUNCTION, protected by calling function
	string k = NAC(kabutt);
	long next;
	
	// Perform HeapSort Algorithm (Recursion)
	while (kabutt < (heap / 2))
	{
		next = (2 * kabutt) + 1;		// Left part
		if ((next < (heap - 1)) && 
			(NAC(next) < NAC(next+1)))
		{
			next++;						// Right part
		}
		if (k >= NAC(next)) { break; };
		NAC(kabutt) = NAC(next);
		kabutt = next;
	}
	NAC(kabutt) = k;
}

void StringArray::ShellSort ()																				// public
{
	long u = Ubound();
	long i, j, h;
	string tmp;
	
	// Enter Critical Section
	cs.Enter();

	// Perform ShellSort Algorithm
	for (h = 1; h <= (u / 9); h = (3 * h) + 1);
	for ( ; h > 0; h /= 3)
	{
		for (i = h; i < u; i++)
		{
			tmp = NAC(i);
			for (j = i; (j >= h) && (NAC(j - h) > tmp); j -=h)		// Condition = false
			{
				NAC(j) = NAC(j - h);
			}
			NAC(j) = tmp;
		}
	}

	// Leave Critical Section
	cs.Leave();
}


//////////////////////////////////////
// Class: UNISTR - Unicode String	//
//////////////////////////////////////
UNISTR::UNISTR ()
{
	Init();
}

UNISTR::UNISTR (string& obj)
{
	Init();
	operator= (obj);
}

UNISTR::UNISTR (UNISTR& obj)
{
	Init();
	operator= (obj);
}

UNISTR::~UNISTR ()
{
	Clear();
}

void UNISTR::Init ()																					// private
{
	// Enter Critical Section
	cs.Enter();

	// Clear RAM
	myuni = 0;
	unilen = 0;

	// Always allocate an Unicode String
	CreateUNI();

	// Leave Critical Section
	cs.Leave();
}

void UNISTR::Clear ()																					// public
{
	// Enter Critical Section
	cs.Enter();
	
	// If necessary, free the old Unicode String
	if (myuni != 0)
	{
		LocalFree (myuni);
		myuni = 0;
		unilen = 0;
	}

	// Leave Critical Section
	cs.Leave();
}

void UNISTR::CreateUNI ()																				// public
{
	// Enter Critical Section
	cs.Enter();

	// If necessary, free the old Unicode String before
	Clear();
	
	// Determine how many bytes are needed for the conversion
	unilen = MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, &mystr[0], xlen(mystr)+1, (LPWSTR) &myuni[0], 0);
	if (unilen != 0)
	{
		// Allocate Unicode String
		myuni = (unsigned short*) LocalAlloc(LMEM_FIXED, sizeof(unsigned short) * unilen);
		
		// Write the converted Source String
		MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, &mystr[0], xlen(mystr)+1, (LPWSTR) &myuni[0], unilen);
	}

	// Leave Critical Section
	cs.Leave();
}

void UNISTR::operator= (string& obj)																	// public
{
	cs.Enter();
	mystr = obj;
	CreateUNI();
	cs.Leave();
}

void UNISTR::operator= (UNISTR& obj)																	// public
{
	cs.Enter();
	mystr = obj.mystr;
	CreateUNI();
	cs.Leave();
}

string UNISTR::get ()																					// public
{
	// No CS possible, BE CAREFUL when calling this function
	return mystr;
}

unsigned short& UNISTR::operator[] (long idx)															// public
{
	unsigned short* ret;

	// Enter Critical Section
	cs.Enter();

	// Check if Index is Valid
	if (!((idx >= 0) && (idx <= unilen-1)))
	{
		// Leave Critical Section before return
		cs.Leave();
		FatalAppExit (0, "UNISTR::AccessMemory->Access Violation");
		return myuni[idx];
	}
	
	// Get Return Object
	ret = &myuni[idx];

	// Leave Critical Section
	cs.Leave();

	// Return Object
	return *ret;
	
}



//
// String <-> Unicode Conversion Helper Function
//
string Unicode2Str (unsigned short *uniarray, long unilen)
{
	string buf;
	long len;
	
	// Adjust length to convert
	if (unilen == -1) { unilen = lstrlenW((LPCWSTR) &uniarray[0]); };
	
	// Find a sufficient buffer and convert
	len = WideCharToMultiByte (CP_ACP, 0, (LPCWSTR) &uniarray[0], unilen, &buf[0], 0, NULL, NULL);
	if (len != 0)
	{
		buf.resize (len);
		WideCharToMultiByte (CP_ACP, 0, (LPCWSTR) &uniarray[0], unilen, &buf[0], len, NULL, NULL);
	}
	return buf;
}