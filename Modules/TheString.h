// TheString.h: Declares everything needed to use the TheString.cpp module
//
// Thread-Safe: YES
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __THESTRING_H
#define __THESTRING_H


	//////////////////////////
	// Macro Definitions	//
	//////////////////////////
	#define xlen(str)					(long) str.length()
	#define Mid(str, start, count)		str.mid(start, count)


	// Pre-declare class "StringArray" because we need it within "string".
	class StringArray;


	/////////////////////
	// Class: STRING	//
	//////////////////////
	class string
	{
	private:
		char* mem;
		size_t len;

		// Critical Section
		CriticalSection cs;
	
		// Main
		void Init ();
		void Assign (LPCTSTR);
		void Assign (string&);
		void Clear ();

		// Equality Checks
		bool streq (string, string);
		int intstreq (string, string);

	public:
		// Constructor block
		string ();
		string (LPCTSTR);
		string (string&);
		string (long);
		string (bool);
		~string ();

		void operator= (LPCTSTR);
		void operator= (string&);
	
		// Operator block
		char& operator[] (long);
		const char& operator[] (long) const;
		bool operator<  (string);
		bool operator<= (string);
		bool operator>  (string);
		bool operator>= (string);
		bool operator== (string);
		bool operator!= (string);
	
		void operator+= (LPCTSTR);
		void operator+= (string);

		void Append (LPCTSTR);
		void Append (string);
	
		// User Interface
		size_t length ();
		void resize (size_t);
		string mid(size_t, size_t = -1);
		long split (StringArray*, unsigned char);					// Returns Ubound of Returned StringArray

		void cutoffLeftChars(size_t);
		long Replace (string what, string newstr);					// Returns Count of Replaces
		long InStrFirst (unsigned char, long = 1);
	};


	//////////////////////////
	// String Searching		//
	//////////////////////////
	long InStrFirst (string, unsigned char, long = 1);
	long InStrLast (string, unsigned char, long = -1);
	long InStrFirstNot (string, unsigned char, long = 1);
	long InStrLastNot (string, unsigned char, long = -1);
	long InStr (string, string, long = 1);


	//////////////////////////
	// String Modification	//
	//////////////////////////
	string Left (string, long);
	string Right (string, long);

	string UCase (LPCTSTR);
	string UCase (string&);

	string LCase (LPCTSTR);
	string LCase (string&);


	//////////////////////////
	// String Conversion	//
	//////////////////////////
	char* AllocChar (string);
	string Char2Str (char*, long = -1);
	long DigitEnd (string);
	long CLong (string);


	//////////////////////////
	// String Creation		//
	//////////////////////////
	string Chr (unsigned char);
	string CSR (string, string, string = "", string = "", string = "");
	string ChrString (long, unsigned char);
	string FillStr (string, long, unsigned char = '0');
	string CStr (double);
	string StringPart (string, long, unsigned char = 0);
	void String2Buffer (string, char*, long);
	string pBuffer2String (LPSTR);




	/////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////



	//////////////////////////
	// Class: StringArray	//
	//////////////////////////
	class StringArray
	{
	private:
		// Critical Section Object
		CriticalSection cs;

		// Cache
		StructArray <string> sa;

		// Functions: Sort Algorithms
		void AdjustHeap (long, long);

	public:
		StringArray ();
		~StringArray ();
	
		// Functions: Interface
		void Clear();
		long Ubound();
		void Redim(long);
		long Append(string&);
		void Remove (long);
		void Swap (long, long);
	
		// Functions: Operators
		void operator=		(StringArray&);
		string& operator[]	(long);
		bool operator==		(StringArray&);
		bool operator!=		(StringArray&);
		string& NAC			(long);
	
		// Functions: Sort Algorithms
		void HeapSort ();
		void ShellSort ();
	};


	//////////////////////////////////////
	// Class: UNISTR - Unicode String	//
	//////////////////////////////////////
	class UNISTR
	{
	private:
		// Critical Section Object
		CriticalSection cs;

		// Cache
		string mystr;
		unsigned short *myuni;
		long unilen;

		// Functions: Init/Terminate
		void Init();

	public:
		UNISTR ();
		UNISTR (string&);
		UNISTR (UNISTR&);
		~UNISTR ();

		// Functions: Interface
		void Clear();
		void CreateUNI();
		string get();
	
		// Functions: Operators
		void operator= (string&);
		void operator= (UNISTR&);

		unsigned short& operator[] (long);
	};

	string Unicode2Str (unsigned short*, long = -1);


#endif
