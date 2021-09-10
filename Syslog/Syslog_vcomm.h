// Syslog_vcomm.h: Defines everything needed to use the Syslog_vcomm.cpp module
//
// Thread-Safe: YES
//


//////////////////////////
// Include Protection	//
//////////////////////////
#ifndef __SYSLOG_VCOMM_H
#define __SYSLOG_VCOMM_H


	//////////////////////////////////
	// Consts: Network Messages		//
	//////////////////////////////////
	extern const unsigned char SLG_VCOMM_AUTH;

	extern const unsigned char SLG_VCOMM_NOTIFY_LOGON;
	extern const unsigned char SLG_VCOMM_NOTIFY_LOGOFF;
	extern const unsigned char SLG_VCOMM_SHOW_USERINFO;
	extern const unsigned char SLG_VCOMM_SPAWN_SETTINGS;

	extern const unsigned char SLG_VCOMM_SET_LOGGING;
	extern const unsigned char SLG_VCOMM_SET_LOGCAPCHANGES;
	extern const unsigned char SLG_VCOMM_ACQUIRE_LOGDATA;
	extern const unsigned char SLG_VCOMM_RECV_LOGDATA;

	extern const unsigned char SLG_VCOMM_LOGTHIS;

	//////////////////////
	// Shared Values	//
	//////////////////////
	extern string SLG_AUTH_SIGNATURE;


#endif
