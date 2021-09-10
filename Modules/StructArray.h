// StructArray.h: Defines a template for using Arrays as known from VisualBasic
//
// Thread-Safe: YES
//


//////////////////////////////////////
// MAKE ALLOCATIONS THREAD-SAFE		//
//////////////////////////////////////
/* 2010-02-17: Not Needed if we are going to link against a MULTITHREADED LIBRARY */
/*			   Use "Project Settings"->"C++"->"Code Generation"->"Multithreaded Debug Library" */
// void* operator new(size_t);
// void* operator new[](size_t);
// void operator delete(void*);
// void operator delete[](void*);


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __STRUCTARRAY_H
#define __STRUCTARRAY_H


	//////////////////////////////
	// Class: CriticalSection	//
	//////////////////////////////
	class CriticalSection
	{
	public:
		explicit CriticalSection();
		~CriticalSection();

		// Interface
		void Enter();
		bool TryEnter();
		void Leave();

	private:
		// Copy Operations are private to prevent copying
		CriticalSection(const CriticalSection&);
		CriticalSection& operator=(const CriticalSection&);

		// Cache
		#ifdef _WIN32
			CRITICAL_SECTION m_cSection;
		#else
			pthread_mutex_t m_cSection;
		#endif
	};


	//////////////////////////////
	// Class: StructArray		//
	//////////////////////////////
	template <class MYCLASS> class StructArray
	{
	private:
		// Types
		struct SFAT
		{
			MYCLASS* x;
		};

		// Critical Section Object
		CriticalSection cs;

		// Cache
		LPSAFEARRAY FAR *sfc;

		// Functions
		void	CreateSFA	(LPSAFEARRAY FAR*, long, unsigned short = 0);
		void	DestroySFA	(LPSAFEARRAY FAR*);
		void*	RedimSFA	(LPSAFEARRAY FAR*, long);

	public:
		StructArray ();
		~StructArray ();
	
		// Public Functions: Interface
		long Ubound ();
		void Clear ();
		void Redim (long);
		void Remove (long);
		void Swap (long, long);
		long Append (MYCLASS&);
	
		// Functions: Operators
		MYCLASS& operator[] (long);
	};



	/////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////// Class: StructArray -- PRIVATE FUNCTIONS --  /////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class MYCLASS> void StructArray<MYCLASS>::CreateSFA (LPSAFEARRAY FAR *SFA, long sizeoftype, unsigned short features)		// private
	{
		SAFEARRAY* neu;

		// Enter Critical Section
		cs.Enter();

		// Allocate Descriptor
		SafeArrayAllocDescriptor (1, &neu);
		neu->fFeatures = features;
		neu->cLocks = 0;
		neu->cbElements = sizeoftype;

		// Create SafeArray
		SafeArrayAllocData (neu);

		// Leave Critical Section
		cs.Leave();

		// Return New SafeArray
		*SFA = neu;
	}

	template <class MYCLASS> void StructArray<MYCLASS>::DestroySFA (LPSAFEARRAY FAR *SFA)				// private
	{
		// Enter CS.
		cs.Enter();

		// Destroy Safe-Array.
		if (*SFA != 0) 
		{ 
			SafeArrayDestroy (*SFA); 
		}

		// Leave CS.
		cs.Leave();
	}

	template <class MYCLASS> void* StructArray<MYCLASS>::RedimSFA (LPSAFEARRAY FAR *SFA, long ubound)	// private
	{
		SAFEARRAYBOUND *neu;
	
		// Enter Critical Section
		cs.Enter();

		// Check if Safe-Array Pointer is valid
		if (*SFA == 0)
		{
			// Leave Critical Section before return
			cs.Leave();
			FatalAppExit (0, "SafeArray::Redim->Access Violation");
			return 0;
		}

		// Allocate temporary Safe-Array Boundary Descriptor
		neu = new SAFEARRAYBOUND[1];
		neu[0].lLbound = 0;
		neu[0].cElements = ubound+1;
	
		// Adjust Array Dimensions
		SafeArrayRedim (*SFA, &neu[0]);

		// Destroy temporary Safe-Array Boundary Descriptor
		delete[] neu;

		// Leave Critical Section
		cs.Leave();

		// Return new Safe-Array
		return (*SFA)->pvData;
	}


	//
	// 2013-01-04: This function is now directly placed in calling function's code.
	//
	//template <class MYCLASS> long StructArray<MYCLASS>::UboundSFA (LPSAFEARRAY FAR *SFA)			// private
	//{
	//	// No CS possible, BE CAREFUL
	//	return ((*SFA)->rgsabound[0].cElements - 1);
	//}


	/////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////// Class: StructArray -- PUBLIC FUNCTIONS --  //////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class MYCLASS> StructArray<MYCLASS>::StructArray ()							// Constructor
	{
		// No CS
		sfc = new LPSAFEARRAY;
		CreateSFA (sfc, sizeof(SFAT), 0);
		RedimSFA (sfc, 0);
		SFAT *my = (SFAT*) (*sfc)->pvData;
		my[0].x = new MYCLASS;
	}

	template <class MYCLASS> StructArray<MYCLASS>::~StructArray ()							// Destructor
	{
		// No CS
		SFAT *my = (SFAT*) (*sfc)->pvData;
		delete my[0].x;
		Clear();
		DestroySFA (sfc);
		delete sfc;
	}

	template <class MYCLASS> long StructArray<MYCLASS>::Ubound ()							// public
	{
		long u;

		// Enter Critical Section
		cs.Enter();

		// Get Item Count in Safe-Array
		u = ((*sfc)->rgsabound[0].cElements - 1);			// u = UboundSFA(sfc);

		// Leave Critical Section
		cs.Leave();

		// Return Item Count in Safe-Array
		return u;
	}

	template <class MYCLASS> void StructArray<MYCLASS>::Clear ()							// public
	{
		// CS
		cs.Enter();
		Redim (0);
		cs.Leave();
	}

	template <class MYCLASS> void StructArray<MYCLASS>::Redim (long nu)						// public
	{
		long u;
		SFAT *my;

		// Enter Critical Section
		cs.Enter();
	
		// Get Current Item Count
		u = ((*sfc)->rgsabound[0].cElements - 1);			// u = UboundSFA(sfc);
		if (nu != u)
		{
			// Item Count should be changed
			if (nu > u)		// Size Up
			{
				my = (SFAT*) RedimSFA (sfc, nu);
				for (long i = u+1; i <= nu; i++)	// New element reset
				{
					my[i].x = new MYCLASS;			// Create New Class
				}
			}
			else			// Size Down
			{
				my = (SFAT*) (*sfc)->pvData;
				for (long i = u; i > nu; i--)
				{
					delete my[i].x;					// Destroy Class
				}
				RedimSFA (sfc, nu);
			}
		}

		// Leave Critical Section
		cs.Leave();
	}

	template <class MYCLASS> long StructArray<MYCLASS>::Append (MYCLASS& refElement)		// public
	{
		SFAT *my;
		long new_u;

		// Enter Critical Section
		cs.Enter();

		// Redim Safe-Array.
		new_u = ((*sfc)->rgsabound[0].cElements - 1) + 1;			// new_u = UboundSFA(sfc) + 1;
		my = (SFAT*) RedimSFA (sfc, new_u);
		my[new_u].x = new MYCLASS;

		// Assign refElement to newly created array element.
		//
		// WARNING: DO NOT USE CopyMemory HERE BECAUSE OF HEAP/CLASS CONTENT CORRUPTION.
		// WARNING: CopyMemory (my[new_u].x, &refElement, sizeof(MYCLASS));
		//
		*my[new_u].x = refElement;

		// Leave Critical Section
		cs.Leave();

		// Return new Ubound.
		return new_u;
	}

	template <class MYCLASS> void StructArray<MYCLASS>::Remove (long index)					// public
	{
		SFAT *my;
		long u;

		// Enter Critical Section
		cs.Enter();
	
		// Check if Index is Valid
		u = ((*sfc)->rgsabound[0].cElements - 1);			// u = UboundSFA(sfc);
		if (!((index >= 1) && (index <= u)))
		{
			// Leave Critical Section before return
			cs.Leave();
			FatalAppExit (0, "StructArray::Remove->Access Violation");
			return;
		}

		// Access the SFA;
		my = (SFAT*) (*sfc)->pvData;

		// Delete Item marked for Removal
		delete my[index].x;

		// Shift all item above #index down
		for (long i = index; i <= (u - 1); i++)
		{
			my[i].x = my[i+1].x;
		}

		// Decrement the dimension of the array by one (last element got free)
		RedimSFA (sfc, u - 1);

		// Leave Critical Section
		cs.Leave();
	}

	template <class MYCLASS> void StructArray<MYCLASS>::Swap (long first, long second)			// public
	{
		MYCLASS tmp;
	
		// Enter Critical Section
		cs.Enter();

		// Swap two SFA items
		tmp = (*this)[first];
		(*this)[first] = (*this)[second];
		(*this)[second] = tmp;

		// Leave Critical Section
		cs.Leave();
	}

	template <class MYCLASS> MYCLASS& StructArray<MYCLASS>::operator[] (long index)				// public
	{
		long u;

		// Enter Critical Section
		cs.Enter();
	
		// Check if Index is Valid
		u = ((*sfc)->rgsabound[0].cElements - 1);			// u = UboundSFA(sfc);
		SFAT *my = (SFAT*) (*sfc)->pvData;
		if (!((index >= 0) && (index <= u)))
		{
			// Leave Critical Section before return
			cs.Leave();
			FatalAppExit (0, "StructArray::AccessMemory->Access Violation");
			return *my[index].x;
		}

		// Leave Critical Section
		cs.Leave();
	
		// No CS possible, BE CAREFUL IN THE PARENT THREAD
		return (MYCLASS&) *my[index].x;
	}



#endif
