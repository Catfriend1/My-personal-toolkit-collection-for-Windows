// vcomm.cpp: Defines Network Message IDs for the Winsuite_Server.cpp module and client modules.
//
// Thread-Safe: YES
//


//////////////////
// Includes		//
//////////////////
#include "stdafx.h"

#include "Modules/_modules.h"

#include "vcomm.h"


//////////////////////////////////
// Consts: Network Messages		//
//////////////////////////////////
const unsigned char VCOMM_AUTH							= 3;

const unsigned char VCOMM_NTPROTECT_LOCK				= 200;
const unsigned char VCOMM_NTPROTECT_UNLOCK				= 201;


//////////////////////
// Shared Values	//
//////////////////////
string AUTH_SIGNATURE									= CSR(CSR(Chr(0x6C), Chr(0x2F), Chr(0xE9), Chr(0xF1)), 
																	Chr(0xFE), Chr(0xDA), Chr(0x05), Chr(0x11));


