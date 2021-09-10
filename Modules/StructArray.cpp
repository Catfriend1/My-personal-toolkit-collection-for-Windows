// StructArray.cpp: Allows using Arrays as known from VisualBasic
//
// Thread-Safe: YES
//

//////////////////
// Includes		//
//////////////////
#include "stdafx.h"

#ifdef _WIN32
	// #include <windows.h>: Already Included in StdAfx.h
#else
	#include <unistd.h>
	#include <pthread.h>
#endif

#include "_modules.h"


//////////////////////////////////
// MEMORY ALLOCATION LOCK		//
//////////////////////////////////
/* bool MEMACTIONS_LOCKED = false; */


//////////////////////////////////////
// MAKE ALLOCATIONS THREAD-SAFE		//
//////////////////////////////////////

/* 2010-02-17: Not Needed if we are going to link against a MULTITHREADED LIBRARY */
/*			   Use "Project Settings"->"C++"->"Code Generation"->"Multithreaded Debug Library" */

/*
void* operator new(size_t size)
{
	void* res;

	while (MEMACTIONS_LOCKED) { Sleep(50); };
	MEMACTIONS_LOCKED = true;
	res = malloc(size);				// Protected Operation for Thread-Safety
	MEMACTIONS_LOCKED = false;

	if (res == 0) { FatalAppExit (0, "new->Bad Allocation"); };
	return res;
}

void* operator new[](size_t size)
{
  	void* res;

	while (MEMACTIONS_LOCKED) { Sleep(50); };
	MEMACTIONS_LOCKED = true;
	res = malloc(size);				// Protected Operation for Thread-Safety
	MEMACTIONS_LOCKED = false;

	if (res == 0) { FatalAppExit (0, "new[]->Bad Allocation"); };
	return res;
}

void operator delete(void* ptr)
{
	while (MEMACTIONS_LOCKED) { Sleep(50); };
	MEMACTIONS_LOCKED = true;
	free(ptr);						// Protected Operation for Thread-Safety
	MEMACTIONS_LOCKED = false;
}

void operator delete[](void* ptr)
{
	while (MEMACTIONS_LOCKED) { Sleep(50); };
	MEMACTIONS_LOCKED = true;
	free(ptr);
	MEMACTIONS_LOCKED = false;
}
*/


//////////////////////////////
// Class: CriticalSection	//
//////////////////////////////
CriticalSection::CriticalSection()
{
	#ifdef _WIN32
	if (0 == InitializeCriticalSectionAndSpinCount(&this->m_cSection, 0)) { throw("Could not create Critical Section Object."); };
	#else
		if (pthread_mutex_init(&this->m_cSection, NULL) != 0) { throw("Could not create Critical Section Object."); };
	#endif
};

CriticalSection::~CriticalSection()
{
	// Make Sure the CS Object is not in use before it is being deleted
	this->Enter();
	this->Leave();

	#ifdef _WIN32
		DeleteCriticalSection(&this->m_cSection);
	#else
		pthread_mutex_destroy(&this->m_cSection);
	#endif
};

void CriticalSection::Enter()
{
	#ifdef _WIN32
		EnterCriticalSection(&this->m_cSection);
	#else
		pthread_mutex_lock(&this->m_cSection);
	#endif
};

bool CriticalSection::TryEnter()
{
	#ifdef _WIN32
		return(TRUE == TryEnterCriticalSection(&this->m_cSection));
	#else
		return(0 == pthread_mutex_trylock(&this->m_cSection));
	#endif
};

void CriticalSection::Leave()
{
	// This function will only work if the current thread holds the current lock on the CriticalSection object called by Enter()
	#ifdef _WIN32
		LeaveCriticalSection(&this->m_cSection);
	#else
		pthread_mutex_unlock(&this->m_cSection);
	#endif
};





