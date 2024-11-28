/* ==============================================================================================================
 * Author: CXX
 * Date: 2022-04-14
 * Version:
 * Copyright (C) CXX, All rights reserved
 * Description:
 * History:
 * 20220414: C header file, be created
 * ==============================================================================================================
 */



#ifndef _AS_ABI_H_
#define _AS_ABI_H_

#include "common/error_user.h"

#pragma pack(push, 4)

#define SABI_VERSION	0x00010000	// v1.0.0

typedef unsigned int SABI_Ver;


#define SABI_FILENAME_SENTRY_BOOT_LOG		"sentry_boot_log"


// API definition
#define SABI_FUNNAME_LIB_INITIATE		"sentry_library_initiate"	// function for initiating, will called after loading
typedef int (*SABI_FUNC_Initiate)( int argc, void **argv );

// arguments:
// [0]: library-self handle
// [1]: identifier
// [2]: version required
// [3]: work directory(optional)
#define SABI_FUN_INITIATE_ARG_IDX_LIBHD		0
#define SABI_FUN_INITIATE_ARG_IDX_IDENT		1
#define SABI_FUN_INITIATE_ARG_IDX_VER		2
#define SABI_FUN_INITIATE_ARG_IDX_ENV		3


#define SABI_FUNNAME_LIB_RELEASE		"sentry_library_release"	// function for releasing, will called before unloading
typedef void (*SABI_FUNC_Release)( void );


#define SABI_FUNNAME_SENTRY_CTRL		"sentry_ctrl"
typedef int (*SABI_FUNC_Ctrl)( int opc, void *dat, long len );		// function for control sentry


#define SABI_OBJNAME_LIB_ENVIRON		"sentry_library_environ"	// exported environment name of library specific-environment

#define SABI_ENVIRON_MAGIC	(0xECDBFA)

typedef struct
{
	int      magic;		// fixed value
	int      size;		// size of this structure
	SABI_Ver ver;		// version
	int      res;		// reserved
	long     ident;		// library identifier
	void    *hlib;		// library handler
	char    *env;		// environments

	// function table
	struct{
		SABI_FUNC_Initiate	Initiate;
		SABI_FUNC_Release	Release;
		SABI_FUNC_Ctrl		Ctrl;
		// TODO:
	}func;
}SABI_Environ;


// sentry control code
typedef enum
{
	SABI_CTRL_DISABLE          = 1,		// disable sentry
	SABI_CTRL_ENABLE           = 2 ,	// enable sentry
	SABI_CTRL_RESET            = 3 ,	// reset environment, reload all settings, capabilities, and then restart sentry
	SABI_CTRL_GET_MODE         = 4 ,	// get behavior mode
	SABI_CTRL_SET_MODE         = 5 ,	// set behavior mode
	SABI_CTRL_GET_CAPACITIES   = 6 ,	// get capacities detailed
	SABI_CTRL_SET_CAPACITIES   = 7 ,	// set capacities
	SABI_CTRL_GET_SETTINGS     = 8 ,	// get setting/configures detailed
	SABI_CTRL_SET_SETTINGS     = 9 ,	// set setting/configures
	SABI_CTRL_CLR_JOURNAL      = 10 ,	// clear sentry's journal cache
	SABI_CTRL_SET_JOURNAL      = 11 ,	// set journal type/mask
	SABI_CTRL_GET_STATUS       = 12 ,	// get status

	SABI_CTRL_STOP   = SABI_CTRL_DISABLE ,
	SABI_CTRL_START  = SABI_CTRL_ENABLE ,
	SABI_CTRL_RELOAD = SABI_CTRL_RESET ,
}SABI_CTRL;


// sentry mode
typedef enum
{
	SABI_MODE_IDEL        = 0,		//
	SABI_MODE_TRACE       = 0x01,	// trace execution
	SABI_MODE_PREVENT     = 0x02,   // prevent threaten, violation, vulnerabilities, etc. meanwhile protect hosts
	SABI_MODE_OFG         = 0x04,	// capture snapshot of "Operator Flow Graph"
	SABI_MODE_DFG         = 0x08,	// capture snapshot of "Data Flow Graph"
	SABI_MODE_IAST        = 0x10,   // special scene of "Interact Application Safe Testing"
	SABI_MODE_SILENT      = 0x100,	// keep minimum functions, and do not send the BASE anything
	SABI_MODE_QUIET       = 0x200,	// keep journal quiet
	SABI_MODE_INDEPENDENT = 0x400,  // independent sentry, do all of jobs by self. e.g Vulnerabilities Analyzing
	SABI_MODE_DISTRIBUTED = 0x800,  // just means run in distributed environment, no specific functional scene.(reserved)
	SABI_MODE_AUTO        = 0x1000,	// execute automatically

	SABI_MODE_ASPP        = SABI_MODE_PREVENT,
	SABI_MODE_RASP        = SABI_MODE_PREVENT,
	SABI_MODE_TEST        = SABI_MODE_IAST,
}SABI_MODE;


// sentry state
typedef enum
{
	SABI_STATE_UNKNOWN    = 0,
	SABI_STATE_PRIMORDIAL = 1,  // just born
	SABI_STATE_INIT       = 2,  // on initiating
	SABI_STATE_READY      = 3,  // ready to run
	SABI_STATE_RUN        = 4,  // on running
	SABI_STATE_PAUSE      = 5,  // paused by controller
	SABI_STATE_WAIT       = 6,  // waiting for something
	SABI_STATE_FAULT      = 7,  // fault occurred
	SABI_STATE_EXIT       = 8,	// on exiting
	SABI_STATE_DEAD       = 9,  // all jobs done
}SABI_STATE;


// journal/log type
typedef enum
{
	SABI_JOURNAL_DIS    = 0x00,    // disable all journal type
	SABI_JOURNAL_INFO	= 0x01,    // information
	SABI_JOURNAL_WARN   = 0x02,    // warning
	SABI_JOURNAL_ERROR	= 0x04,    // general error
	SABI_JOURNAL_DEBUG  = 0x08,    // debug information
	SABI_JOURNAL_EVENT  = 0x10,    // general event
	SABI_JOURNAL_CRIT   = 0x20,    // critical error
	SABI_JOURNAL_MT     = 0x40,    // maintenance information
	SABI_JOURNAL_ALL    = 0xFF,    // all above
}SABI_JOURNAL_TYPE, SABI_LOG_TYPE;


// sentry status flag
typedef struct
{
	union{
		struct{
			int f_mode     :16;
			int f_state    :8;
			int f_ready    :1;
			int f_jourtype :7;

			int f_res;
		};
		long long flags;
	};
}SABI_Status;

#define SABI_STATUS_MASK_MODE		0xFFFF
#define SABI_STATUS_MASK_STATE		0xFF0000
#define SABI_STATUS_MASK_JOURTYPE	0xFE000000
#define SABI_STATUS_MASK_READY		0x01000000


// Error
typedef enum
{
	SABI_errFail		= errFail,
	SABI_errOk			= errSuccess,
	SABI_erInvVal		= errInvParam,
	SABI_errInterrupt	= errInterrupt,
	SABI_errWouldblock	= errWouldblock,
	SABI_errAgain		= errAgain,
	SABI_errNoMemory	= errNoMemory,
	SABI_errNotEnough	= errNotEnough,
	SABI_errIO			= errIO,
	SABI_errTimeout		= errTimeout,
	SABI_errOverflow	= errOverflow,
	SABI_errBusy		= errBusy,
	SABI_errOpenFile	= errOpenFile,
	SABI_errNotExist	= errNotExist,
	SABI_errNotReady	= errNotReady,
	SABI_errDuplicate	= errDuplicate,
	SABI_errInvFunc		= errInvFunc,
	SABI_errInvVer		= errInvVer,
	SABI_errException	= errException,
	SABI_errOpen		= errOpen,
	SABI_errNull		= errNull,
	SABI_errSema		= errSema,
	SABI_errInvFormat	= errInvFormat,
	SABI_errInvExpr		= errInvExpression,
	SABI_errInvProto	= errInvProto,
	SABI_errInvCoding	= errInvCoding,
	SABI_errInvData		= errInvData,
	SABI_errOccupy		= errOccupy,
	SABI_errReject		= errReject,
	SABI_errUnsupport	= errUnsupport,
	SABI_errCancel		= errCancel,
	SABI_errNoCapacity	= errNoCapacity,
	SABI_errIgnored		= errIgnored,
	SABI_errIllegalUser = errIllegalUser,
	SABI_errUser		= errUser,
	SABI_errMode		= errUser + 1,
	SABI_errState		= errUser + 2,
}SABI_ERROR;


#pragma pack(pop)

#endif	// _AS_ABI_H_
