// NetMsg_vcomm.h: Defines everything needed to use the NetMsg_vcomm.cpp module
//
// Thread-Safe: YES
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __NETMSG_VCOMM_H
#define __NETMSG_VCOMM_H


	//////////////////////////////////
	// Consts: Network Messages		//
	//////////////////////////////////
	extern const unsigned char NMSG_VCOMM_AUTH;

	extern const unsigned char NMSG_VCOMM_LOGON;
	extern const unsigned char NMSG_VCOMM_NOTIFY_LOGOFF;

	extern const unsigned char NMSG_VCOMM_SPAWN_MACHINES;
	extern const unsigned char NMSG_VCOMM_INVOKE_SHUTDOWN;
	extern const unsigned char NMSG_VCOMM_INVOKE_RESTART;
	extern const unsigned char NMSG_VCOMM_INVOKE_MESSAGE;
	extern const unsigned char NMSG_VCOMM_ADMIN_PASSWORD;

	// Messages stored in RespQueue
	extern const unsigned char NMSG_VCOMM_ADMIN_PASSWORD_OK;
	extern const unsigned char NMSG_VCOMM_INVOKE_LOGOFF;
	extern const unsigned char NMSG_VCOMM_INVOKE_GPUPDATE;


	//////////////////////
	// Shared Values	//
	//////////////////////
	extern string NMSG_AUTH_SIGNATURE;


#endif

