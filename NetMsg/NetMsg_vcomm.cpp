// NetMsg_vcomm.cpp: Defines the Network Message IDs used in NetMsg Components.
//
// Thread-Safe: YES
//


//////////////////
// Includes		//
//////////////////
#include "stdafx.h"

#include "../Modules/_modules.h"

#include "NetMsg_vcomm.h"


//////////////////////////////////
// Consts: Network Messages		//
//////////////////////////////////
const unsigned char NMSG_VCOMM_AUTH						= 70;

const unsigned char NMSG_VCOMM_LOGON					= 71;
const unsigned char NMSG_VCOMM_NOTIFY_LOGOFF			= 72;

const unsigned char NMSG_VCOMM_SPAWN_MACHINES			= 73;
const unsigned char NMSG_VCOMM_INVOKE_SHUTDOWN			= 74;
const unsigned char NMSG_VCOMM_INVOKE_RESTART			= 75;
const unsigned char NMSG_VCOMM_INVOKE_MESSAGE			= 76;
const unsigned char NMSG_VCOMM_ADMIN_PASSWORD			= 77;			// Message for setting ADMINMODE and resetting ADMINMODE

// Messages stored in RespQueue
const unsigned char NMSG_VCOMM_ADMIN_PASSWORD_OK		= 78;
const unsigned char NMSG_VCOMM_INVOKE_LOGOFF			= 79;
const unsigned char NMSG_VCOMM_INVOKE_GPUPDATE			= 80;


//////////////////////
// Shared Values	//
//////////////////////
string NMSG_AUTH_SIGNATURE = CSR(CSR(Chr(0x2A), Chr(0xF3), Chr(0x92), Chr(0xC8)), 
									Chr(0xDF), Chr(0x54), Chr(0x18), Chr(0x13));


