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

#ifndef _JVMTI_EVENT_H_
#define _JVMTI_EVENT_H_


#include "jvas.h"


typedef jvmtiEventReserved	JTI_EHR;

#define JTIEVT_BEGIN	JVMTI_MIN_EVENT_TYPE_VAL
#define JTIEVT_END		JVMTI_MAX_EVENT_TYPE_VAL

#define JTIEVT_MAX	(JTIEVT_END - JTIEVT_BEGIN + 1)

// node of event handler map
typedef struct _jvas_jtievt_handler_idx
{
	jvmtiEvent evt;
	long       off;
	int        res;  // reserved
	int        opt;  // optional
	JTI_EHR    ehr;
}jvas_jtievt_idx_t;



JVAS_NOEXPORT jvmtiEventCallbacks JTIEVT_LIVE_EHR;


#ifdef __cplusplus
extern "C"{
#endif

JVAS_NOEXPORT void jtievt_init( void );

JVAS_NOEXPORT const jvas_jtievt_idx_t* jtievt_getevtconf( jvmtiEvent evt );

JVAS_NOEXPORT jvmtiError jtievt_ehr_setup( JTIENV_PTR jtienv, int *nDone );

JVAS_NOEXPORT jvmtiError jtievt_ehr_cancel( JTIENV_PTR jtienv );

#ifdef __cplusplus
}
#endif

#endif // _JVMTI_EVENT_H_
