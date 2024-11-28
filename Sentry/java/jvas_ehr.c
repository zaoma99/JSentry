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

#include "jvas_ehr.h"
#include "jvas_tc.h"

#include "sys/taskLib.h"
#include "common/error_user.h"

#ifdef _LINUX
#include "linux/sem_base.h"
#include "linux/clock_spec.h"
#endif


JVAS_HIDDEN UINT64 g_jvas_jtievt_refers = 0;

// ========================================================================================================================================
// External Global Variables
// ========================================================================================================================================

JVAS_EXTERN volatile int g_jvas_vmdeath_triggered;  // from jvas.c
JVAS_EXTERN volatile int g_jvas_ready;  // from jvas.c


// ========================================================================================================================================
// JVMTI Essential Event Handlers
// ========================================================================================================================================

// Event: JVMTI_EVENT_VM_INIT
// The VM initialization event signals the completion of VM initialization.
// Once this event is generated, the agent is free to call any JNI or JVM TI function.
// The VM initialization event can be preceded by or can be concurrent with other events,
// but the preceding events should be handled carefully, if at all, because the VM has not completed its initialization.
// The thread start event for the main application thread is guaranteed not to occur until after the handler for the VM initialization event returns.
// NOTE: In the case of VM start-up failure, this event will not be sent.
// REFERENCES: The "JVM(TM) Tool Interface of the Java Virtual Machine Specification"
void JNICALL jvas_ehr_VMInit( JTIENV_PTR jvmti, JNIENV_PTR jni, jthread thread )
{
	// nothing to do
}


// Event: JVMTI_EVENT_VM_START
// The VM start event signals the start of the VM. At this time JNI is live but the VM is not yet fully initialized.
// Once this event is generated, the agent is free to call any JNI function.
// This event signals the beginning of the start phase, JVM TI functions permitted in the start phase may be called.
// NOTE: In the case of VM start-up failure, this event will not be sent.
// REFERENCES: The "JVM(TM) Tool Interface of the Java Virtual Machine Specification"
void JNICALL jvas_ehr_VMStart( JTIENV_PTR jvmti, JNIENV_PTR jni )
{
	// nothing to do
}


// Event: JVMTI_EVENT_VM_DEATH
// The VM death event notifies the agent of the termination of the VM.
// No events will occur after the VMDeath event.
// NOTE: In the case of VM start-up failure, this event will not be sent.
// REFERENCES: The "JVM(TM) Tool Interface of the Java Virtual Machine Specification"
void JNICALL jvas_ehr_VMDeath( JTIENV_PTR jvmti, JNIENV_PTR jni )
{
	g_jvas_vmdeath_triggered = 1;

#ifdef _JVAS_DEVELOP_
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "%s\n", __FUNCTION__ );
#endif
}


// Event: JVMTI_EVENT_CLASS_FILE_LOAD_HOOK
// This event is sent when the VM obtains class file data, but before it constructs the in-memory representation for that class.
// This event is also sent when the class is being modified by the RetransformClasses function or the RedefineClasses function, called in any JVM TI environment.
// The agent can instrument the existing class file data sent by the VM to include profiling/debugging hooks.
// REFERENCES: The "JVM(TM) Tool Interface of the Java Virtual Machine Specification"
// Capability Requirement: can_generate_all_class_hook_events

//static long hook_nhits = 0;
void JNICALL jvas_ehr_ClassFileLoadHook( JTIENV_PTR jvmti,
										 JNIENV_PTR jni,
										 jclass class_being_redefined,
										 jobject loader,
										 const char* name,
										 jobject protection_domain,
										 jint class_data_len,
										 const unsigned char* class_data,
										 jint* new_class_data_len,
										 unsigned char** new_class_data
										 )
{
#ifdef _JVAS_DEVELOP_
		_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "%s: %p, %p, %s, %p, %d, %p, %p, %p\n", __FUNCTION__,
				class_being_redefined, loader, name?name:"(NULL)", protection_domain, class_data_len, class_data, new_class_data_len, new_class_data );
#endif


	if( g_jvas_ready > 0 && g_jvas_vmdeath_triggered == 0 )
	{
		__sync_fetch_and_add( &g_jvas_jtievt_refers, 1 );

		if( class_data && new_class_data && new_class_data_len )
		{
			if( g_jvas_ready == 1 )
			{
				// try to re-transform class file
				jvas_tc_transform( jvmti, name, class_data, class_data_len, new_class_data, new_class_data_len );
			}
			else if( class_being_redefined )
			{
				// restoring only
				jvas_tc_restore( name );
			}
		}
		//else
		//	printf( "XXXXXXXXXXXXXXXXXX %ld, %s, %p, %d, %p, %p\n", ++hook_nhits, name, class_data, class_data_len, new_class_data, new_class_data_len );

		__sync_fetch_and_sub( &g_jvas_jtievt_refers, 1 );
	}
}


// Event: JVMTI_EVENT_CLASS_LOAD
// A class load event is generated when a class or interface is created.
// This event is sent at an early stage in loading the class. As a result the class should be used carefully.
// NOTE: Array class creation does not generate a class load event. The creation of a primitive class does not generate a class load event.
// REFERENCES: The "JVM(TM) Tool Interface of the Java Virtual Machine Specification"
void JNICALL jvas_ehr_ClassLoad( JTIENV_PTR jvmti,
								 JNIENV_PTR jni,
								 jthread thread,
								 jclass klass
								 )
{
#ifdef _JVAS_DEVELOP_
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "%s: %p, %p\n", __FUNCTION__, thread, klass );
#endif
}


// Event: JVMTI_EVENT_CLASS_PREPARE
// A class prepare event is generated when class preparation is complete.
// At this point, class fields, methods, and implemented interfaces are available, and no code from the class has been executed.
// NOTE:  Class prepare events are not generated for array class and primitive classes.
// REFERENCES: The "JVM(TM) Tool Interface of the Java Virtual Machine Specification"
void JNICALL jvas_ehr_ClassPrepare( JTIENV_PTR jvmti,
									JNIENV_PTR jni,
									jthread thread,
									jclass klass
									)
{
#ifdef _JVAS_DEVELOP_
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "%s: %p, %p\n", __FUNCTION__, thread, klass );
#endif
}


// Event:
void JNICALL jvas_ehr_NativeMethodBind( JTIENV_PTR jvmti,
										JNIENV_PTR jni,
										jthread thread,
										jmethodID method,
										void* address,
										void** new_address_ptr
										)
{
#ifdef _JVAS_DEVELOP_
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "%s: %p, %p, %p, %p\n", __FUNCTION__, thread, method, address, new_address_ptr );
#endif
}


// Event:
void JNICALL jvas_ehr_CompiledMethodLoad( JTIENV_PTR jvmti,
										  jmethodID method,
										  jint code_size,
										  const void* code_addr,
										  jint map_length,
										  const jvmtiAddrLocationMap* map,
										  const void* compile_info
										  )
{}


// Event:
void JNICALL jvas_ehr_CompiledMethodUnload( JTIENV_PTR jvmti, jmethodID method, const void* code_addr )
{}


// Event:
void JNICALL jvas_ehr_DataDumpRequest( JTIENV_PTR jvmti )
{}


// Event:
void JNICALL jvas_ehr_DynamicCodeGenerated( JTIENV_PTR jvmti,
											const char* name,
											const void* address,
											jint length
											)
{}


// Event:
void JNICALL jvas_ehr_VMObjectAlloc( JTIENV_PTR jvmti,
									 JNIENV_PTR jni,
									 jthread thread,
									 jobject object,
									 jclass object_klass,
									 jlong size
									 )
{
#ifdef _JVAS_DEVELOP_
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "%s: %p, %p, %p, %ld\n", __FUNCTION__, thread, object, object_klass, size );
#endif
}


// Event:
void JNICALL jvas_ehr_ObjectFree( JTIENV_PTR jvmti, jlong tag )
{
#ifdef _JVAS_DEVELOP_
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "%s: %ld\n", __FUNCTION__, tag );
#endif
}


// Event:
void JNICALL jvas_ehr_ThreadEnd( JTIENV_PTR jvmti, JNIENV_PTR jni, jthread thread )
{
#ifdef _JVAS_DEVELOP_
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "%s: %p\n", __FUNCTION__, thread );
#endif
}


// Event:
void JNICALL jvas_ehr_ThreadStart( JTIENV_PTR jvmti, JNIENV_PTR jni, jthread thread )
{
#ifdef _JVAS_DEVELOP_
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "%s: %p\n", __FUNCTION__, thread );
#endif
}


// Event:
void JNICALL jvas_ehr_ResourceExhausted( JTIENV_PTR jvmti,
										 JNIENV_PTR jni,
										 jint flags,
										 const void* reserved,
										 const char* description
										 )
{
}


// Event:
void JNICALL jvas_ehr_Exception( JTIENV_PTR jvmti,
		  	  	  	  	  	  	 JNIENV_PTR jni,
								 jthread thread,
								 jmethodID method,
								 jlocation location,
								 jobject exception,
								 jmethodID catch_method,
								 jlocation catch_location
								 )
{}


// Event:
void JNICALL jvas_ehr_ExceptionCatch( JTIENV_PTR jvmti,
		  	  	  	  	  	  	  	  JNIENV_PTR jni,
									  jthread thread,
									  jmethodID method,
									  jlocation location,
									  jobject exception
									  )
{}


// Event:
void JNICALL jvas_ehr_FieldAccess( JTIENV_PTR jvmti,
		  	  	  	  	  	  	   JNIENV_PTR jni,
								   jthread thread,
								   jmethodID method,
								   jlocation location,
								   jclass field_klass,
								   jobject object,
								   jfieldID field
								   )
{}


// Event:
void JNICALL jvas_ehr_FieldModification( JTIENV_PTR jvmti,
										 JNIENV_PTR jni,
										 jthread thread,
										 jmethodID method,
										 jlocation location,
										 jclass field_klass,
										 jobject object,
										 jfieldID field,
										 char signature_type,
										 jvalue new_value
										 )
{}


// Event:
void JNICALL jvas_ehr_FramePop( JTIENV_PTR jvmti,
								JNIENV_PTR jni,
								jthread thread,
								jmethodID method,
								jboolean was_popped_by_exception
								)
{}


// Event:
void JNICALL jvas_ehr_GarbageCollectionFinish( JTIENV_PTR jvmti )
{
#ifdef _JVAS_DEVELOP_
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "%s\n", __FUNCTION__ );
#endif
}


// Event:
void JNICALL jvas_ehr_GarbageCollectionStart( JTIENV_PTR jvmti )
{
#ifdef _JVAS_DEVELOP_
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "%s\n", __FUNCTION__ );
#endif
}


// Event:
void JNICALL jvas_ehr_MethodEntry( JTIENV_PTR jvmti,
								   JNIENV_PTR jni,
								   jthread thread,
								   jmethodID method
								   )
{}


// Event:
void JNICALL jvas_ehr_MethodExit( JTIENV_PTR jvmti,
								  JNIENV_PTR jni,
								  jthread thread,
								  jmethodID method,
								  jboolean was_popped_by_exception,
								  jvalue return_value
								  )
{}


// Event:
void JNICALL jvas_ehr_MonitorContendedEnter( JTIENV_PTR jvmti,
 	  	  	   	   	   	   	   	   	   	   	 JNIENV_PTR jni,
											 jthread thread,
											 jobject object
											 )
{}


// Event:
void JNICALL jvas_ehr_MonitorContendedEntered( JTIENV_PTR jvmti,
											   JNIENV_PTR jni,
											   jthread thread,
											   jobject object
											   )
{}


// Event:
void JNICALL jvas_ehr_MonitorWait( JTIENV_PTR jvmti,
								   JNIENV_PTR jni,
								   jthread thread,
								   jobject object,
								   jlong timeout
								   )
{}


// Event:
void JNICALL jvas_ehr_MonitorWaited( JTIENV_PTR jvmti,
									 JNIENV_PTR jni,
									 jthread thread,
									 jobject object,
									 jboolean timed_out
									 )
{}


// Event:
void JNICALL jvas_ehr_SingleStep( JTIENV_PTR jvmti,
								  JNIENV_PTR jni,
								  jthread thread,
								  jmethodID method,
								  jlocation location
								  )
{}

// Event:
void JNICALL jvas_ehr_Breakpoint( JTIENV_PTR jvmti,
		  	  	  	  	  	  	  JNIENV_PTR jni,
								  jthread thread,
								  jmethodID method,
								  jlocation location
								  )
{}


