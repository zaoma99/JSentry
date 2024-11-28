/* ==============================================================================================================
 * Author: CXX
 * Date: 2022-04-14
 * Version:
 * Copyright (C) CXX, All rights reserved
 * Description:
 * History:
 * 20220414: C source file, be created
 * ==============================================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jtievt.h"
#include "jvas_ehr.h"

#include "sys/taskLib.h"
#include "common/error_user.h"

#ifdef _LINUX
#include "linux/sem_base.h"
#include "linux/clock_spec.h"
#endif


// ========================================================================================================================================
// Global Variables
// ========================================================================================================================================

// event handler map, containers event type, handler, and offset
JVAS_HIDDEN const jvas_jtievt_idx_t JTIEVT_EHR_MAP[] = {
		{ JVMTI_EVENT_VM_INIT,                   _OFFSET(jvmtiEventCallbacks, VMInit),
		  0, 1, (JTI_EHR)NULL }, /* jvas_ehr_VMInit */

		{ JVMTI_EVENT_VM_DEATH,                  _OFFSET(jvmtiEventCallbacks, VMDeath),
		  0, 1, (JTI_EHR)jvas_ehr_VMDeath },

		{ JVMTI_EVENT_THREAD_START,              _OFFSET(jvmtiEventCallbacks, ThreadStart),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_THREAD_END,                _OFFSET(jvmtiEventCallbacks, ThreadEnd),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_CLASS_FILE_LOAD_HOOK,      _OFFSET(jvmtiEventCallbacks, ClassFileLoadHook),
		  0, 0, (JTI_EHR)jvas_ehr_ClassFileLoadHook },

		{ JVMTI_EVENT_CLASS_LOAD,                _OFFSET(jvmtiEventCallbacks, ClassLoad),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_CLASS_PREPARE,             _OFFSET(jvmtiEventCallbacks, ClassPrepare),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_VM_START,                  _OFFSET(jvmtiEventCallbacks, VMStart),
		  0, 1, (JTI_EHR)NULL }, /* jvas_ehr_VMStart */

		{ JVMTI_EVENT_EXCEPTION,                 _OFFSET(jvmtiEventCallbacks, Exception),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_EXCEPTION_CATCH,           _OFFSET(jvmtiEventCallbacks, ExceptionCatch),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_SINGLE_STEP,               _OFFSET(jvmtiEventCallbacks, SingleStep),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_FRAME_POP,                 _OFFSET(jvmtiEventCallbacks, FramePop),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_BREAKPOINT,                _OFFSET(jvmtiEventCallbacks, Breakpoint),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_FIELD_ACCESS,              _OFFSET(jvmtiEventCallbacks, FieldAccess),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_FIELD_MODIFICATION,        _OFFSET(jvmtiEventCallbacks, FieldModification),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_METHOD_ENTRY,              _OFFSET(jvmtiEventCallbacks, MethodEntry),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_METHOD_EXIT,               _OFFSET(jvmtiEventCallbacks, MethodExit),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_NATIVE_METHOD_BIND,        _OFFSET(jvmtiEventCallbacks, NativeMethodBind),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_COMPILED_METHOD_LOAD,      _OFFSET(jvmtiEventCallbacks, CompiledMethodLoad),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_COMPILED_METHOD_UNLOAD,    _OFFSET(jvmtiEventCallbacks, CompiledMethodUnload),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_DYNAMIC_CODE_GENERATED,    _OFFSET(jvmtiEventCallbacks, DynamicCodeGenerated),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_DATA_DUMP_REQUEST,         _OFFSET(jvmtiEventCallbacks, DataDumpRequest),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_MONITOR_WAIT,              _OFFSET(jvmtiEventCallbacks, MonitorWait),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_MONITOR_WAITED,            _OFFSET(jvmtiEventCallbacks, MonitorWaited),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_MONITOR_CONTENDED_ENTER,   _OFFSET(jvmtiEventCallbacks, MonitorContendedEnter),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_MONITOR_CONTENDED_ENTERED, _OFFSET(jvmtiEventCallbacks, MonitorContendedEntered),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_RESOURCE_EXHAUSTED,        _OFFSET(jvmtiEventCallbacks, ResourceExhausted),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_GARBAGE_COLLECTION_START,  _OFFSET(jvmtiEventCallbacks, GarbageCollectionStart),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_GARBAGE_COLLECTION_FINISH, _OFFSET(jvmtiEventCallbacks, GarbageCollectionFinish),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_OBJECT_FREE,               _OFFSET(jvmtiEventCallbacks, ObjectFree),
		  0, 1, (JTI_EHR)NULL },

		{ JVMTI_EVENT_VM_OBJECT_ALLOC,           _OFFSET(jvmtiEventCallbacks, VMObjectAlloc),
		  0, 1, (JTI_EHR)NULL },

#if JTIEVT_END > 84
		{ jvmtiEventSampledObjectAlloc,          _OFFSET(jvmtiEventCallbacks, SampledObjectAlloc)
		  0, 1, (JTI_EHR)NULL },
#endif
};

JVAS_LOCAL void *JTIEVT_EHR_IDX[JTIEVT_MAX] = {0};		// index for fast seeking

JVAS_LOCAL jvmtiEventCallbacks JTIEVT_WANT_EHR = {0};	// wanted(predefined) event handler

JVAS_HIDDEN jvmtiEventCallbacks JTIEVT_LIVE_EHR = {0};	// actual live event handler



// ========================================================================================================================================
// Functions
// ========================================================================================================================================

// Function: initiate event environment, one shot call
void jtievt_init( void )
{
	JTI_EHR *ehr = (JTI_EHR*)&JTIEVT_WANT_EHR;
	int n = sizeof(JTIEVT_EHR_MAP) / sizeof(JTIEVT_EHR_MAP[0]) - 1;

	assert( JTIEVT_MAX >= n );

	for( ; n >= 0; --n )
	{
		JTIEVT_EHR_IDX[JTIEVT_EHR_MAP[n].evt - JTIEVT_BEGIN] = (void*)&JTIEVT_EHR_MAP[n];

		ehr[JTIEVT_EHR_MAP[n].off/sizeof(JTI_EHR)] = JTIEVT_EHR_MAP[n].ehr;
	}
}


// Function:
const jvas_jtievt_idx_t* jtievt_getevtconf( jvmtiEvent evt )
{
	return (jvas_jtievt_idx_t*)(evt>=JTIEVT_BEGIN && evt<=JTIEVT_END ? JTIEVT_EHR_IDX[evt-JTIEVT_BEGIN] : NULL);
}


// Function: Setup default JVMTI events which are desired in this agent
jvmtiError jtievt_ehr_setup( JTIENV_PTR jtienv, int *nDone )
{
	jvmtiError err = JNICall( jtienv, SetEventCallbacks, &JTIEVT_WANT_EHR, sizeof(JTIEVT_WANT_EHR) );  // set callback functions(event handler) at first
	if( err != JVMTI_ERROR_NONE )
		return err;

	// enable event one by one
	int m = 0;
	JTI_EHR *ehr = (JTI_EHR*)&JTIEVT_LIVE_EHR;
	int n = sizeof(JTIEVT_EHR_MAP) / sizeof(JTIEVT_EHR_MAP[0]) - 1;
	for( ; n >= 0; --n )
	{
		if( JTIEVT_EHR_MAP[n].ehr )
		{
			err = JNICall( jtienv, SetEventNotificationMode, JVMTI_ENABLE, JTIEVT_EHR_MAP[n].evt, NULL );
			if( err == JVMTI_ERROR_NONE )
			{
				ehr[JTIEVT_EHR_MAP[n].off/sizeof(JTI_EHR)] = JTIEVT_EHR_MAP[n].ehr;
				m++;
			}
			else if( !JTIEVT_EHR_MAP[n].opt )
				break;
		}
	}

	if( n < 0 && nDone )
		*nDone = m;

	return err;
}


// Function: Cancel all JVMTI events which have been setup before
jvmtiError jtievt_ehr_cancel( JTIENV_PTR jtienv )
{
	memset( &JTIEVT_LIVE_EHR, 0, sizeof(JTIEVT_LIVE_EHR) );
	return JNICall( jtienv, SetEventCallbacks, NULL, sizeof(JTIEVT_LIVE_EHR) );
}


