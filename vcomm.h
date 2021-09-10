// vcomm.h: Defines everything needed to use the vcomm.cpp module
//
// Thread-Safe: YES
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __VCOMM_H
#define __VCOMM_H


	//////////////////////////////////
	// Consts: Network Messages		//
	//////////////////////////////////
	extern const unsigned char VCOMM_AUTH;

	extern const unsigned char VCOMM_NTPROTECT_LOCK;
	extern const unsigned char VCOMM_NTPROTECT_UNLOCK;


	//////////////////////
	// Shared Values	//
	//////////////////////
	extern string AUTH_SIGNATURE;


#endif

