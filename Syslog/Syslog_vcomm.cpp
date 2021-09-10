// Syslog_vcomm.cpp: Defines the Network Message IDs used in System Logger Components.
//
// Thread-Safe: YES
//


//////////////////
// Includes		//
//////////////////
#include "stdafx.h"

#include "../Modules/_modules.h"

#include "Syslog_vcomm.h"


//////////////////////////////////
// Consts: Network Messages		//
//////////////////////////////////
const unsigned char SLG_VCOMM_AUTH					= 50;

const unsigned char SLG_VCOMM_NOTIFY_LOGON			= 200;
const unsigned char SLG_VCOMM_NOTIFY_LOGOFF			= 201;
const unsigned char SLG_VCOMM_SHOW_USERINFO			= 202;
const unsigned char SLG_VCOMM_SPAWN_SETTINGS		= 203;

const unsigned char SLG_VCOMM_SET_LOGGING			= 210;
const unsigned char SLG_VCOMM_SET_LOGCAPCHANGES		= 211;
const unsigned char SLG_VCOMM_ACQUIRE_LOGDATA		= 212;
const unsigned char SLG_VCOMM_RECV_LOGDATA			= 213;

const unsigned char SLG_VCOMM_LOGTHIS				= 220;


//////////////////////
// Shared Values	//
//////////////////////
string SLG_AUTH_SIGNATURE = CSR(CSR(Chr(0xC7), Chr(0x2F), Chr(0xB2), Chr(0x1F)), 
									Chr(0x01), Chr(0x54), Chr(0x29), Chr(0x11));


