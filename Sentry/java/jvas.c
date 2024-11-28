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
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "sabi.h"
#include "jvas.h"
#include "jvas_swi.h"
#include "jtievt.h"
#include "classfile_util.h"
#include "jvas_tc.h"

#include "vtrace.h"
#include "vt_api_java.h"

#include "journal.h"
#include "sys/taskLib.h"
#include "common/error_user.h"
#include "facility/xmlAnalyzer.h"
#include "advLib/usrMMU.h"

#ifdef _LINUX
#include "linux/sem_base.h"
#include "linux/clock_spec.h"
#endif



// ========================================================================================================================================
// Local and/or no-Export Global Variables
// ========================================================================================================================================

// sentry agent configures
JVAS_HIDDEN JVAS_Conf g_jvas_conf = {0};

// definitions of class transformation
JVAS_HIDDEN jvas_tc_base g_jvas_tc_base = {0};

JVAS_HIDDEN int g_jvas_journal_mask = 0xFF;


// Note:
// Java Virtual Machine Descriptor(JavaVM) is process-specific, be shared among all threads within a process;
// JNI Environment(JNIEnv) is thread-specific, it means one environment per thread;
// JVMTI Environment(jvmtiEnv) works across whole process, so it be shared among all threads within a process;

JVAS_LOCAL jint g_jvas_jvm_version = JVAS_JNI_VERSION_DEF;
JVAS_LOCAL jint g_jvas_jvmti_version = JVAS_JVMTI_VERSION_DEF;

JVAS_LOCAL __unused__ jvmtiPhase g_jvas_jvm_phase = 0;

JVAS_LOCAL JVM_PTR g_jvas_def_jvm = NULL;          // default handle of JavaVM of current process(one process : one JavaVM)

JVAS_LOCAL JTIENV_PTR g_jvas_def_jtienv = NULL;    // default handle of JVMTI Environment be connected to the default JavaVM

JVAS_LOCAL JNIENV_PTR g_jvas_def_jnienv = NULL;    // jni environment of task daemon

JVAS_HIDDEN jvmtiCapabilities g_jvas_jvmti_caps;

JVAS_LOCAL SEM_ID g_jvas_lock_env = NULL;

JVAS_LOCAL TASKID g_jvas_daemon_tid = -1;
JVAS_LOCAL TASKHD g_jvas_daemon_thd = NULL;
JVAS_LOCAL SEM_ID g_jvas_daemon_wait = NULL;
JVAS_LOCAL int g_jvas_daemon_exit = 0;
JVAS_LOCAL int g_jvas_onexit_triggered = 0;		/* 1:process exit, 2: jvm exit, 3: agent unload */

JVAS_HIDDEN int g_jvas_vmdeath_triggered = 0;
JVAS_HIDDEN int g_jvas_ready = -1;

const char *_JVAS_EXIT_CAUSE[] = { "unload", "Process Terminated", "JVM Exit", "Sentry unload" };

const char g_jvas_verstr[] = _JVAS_IDENT_;


// For Spectrum-Sentry-Specification
JVAS_VISIBLE volatile SABI_Environ sentry_library_environ = { 0 };


JVAS_EXTERN volatile UINT64 g_jvas_jtievt_refers;  // from jvas_ehr.c



// local marocs
#ifdef _LINUX
//#define __nop_on_wait	_sleep_nano( 0, 10000 )    //sched_yield()

#define __raw_spin_lock(__lockptr__)\
	(__sync_val_compare_and_swap(__lockptr__, SEM_STATE_UNLOCKED, SEM_STATE_LOCKED) == SEM_STATE_UNLOCKED)

#define __raw_spin_unlock(__lockptr__)\
		__sync_val_compare_and_swap(__lockptr__, SEM_STATE_LOCKED, SEM_STATE_UNLOCKED)

#elif _WINDOWS
#define __raw_spin_lock(__lockptr__)\
	(InterlockedCompareExchange(__lockptr__, SEM_STATE_LOCKED, SEM_STATE_UNLOCKED) == SEM_STATE_UNLOCKED )

#define __raw_spin_unlokc(__lockptr__)\
		InterlockedCompareExchange(__lockptr__, SEM_STATE_UNLOCKED, SEM_STATE_LOCKED )
#endif




// ==============================================================================================================
// Local and/or no-Export Functions( says Internals )
// ==============================================================================================================

//
JVAS_LOCAL TASK_RET _jvas_task_daemon( TASK_ARG );
//
JVAS_LOCAL jint _jvas_attach_jvm( JVM_PTR *jvm, JNIENV_PTR *jenv, jint jni_ver );

#ifdef _JVAS_DEBUG
//
JVAS_LOCAL void __unused__ _jvas_print_capa( const jvmtiCapabilities *capa );
JVAS_LOCAL void _jvas_list_modifiable_classes( JTIENV_PTR jti, JNIENV_PTR jni );
#endif

#define _JVAS_SIGNAL_ENABLE_

#ifdef _JVAS_SIGNAL_ENABLE_
JVAS_LOCAL sighandler_t g_jvas_old_sighandler[_NSIG] = {0};
void _jvas_on_signal( int signum )
{
	if( signum == SIGINT || signum == SIGQUIT || signum == SIGTERM || signum == SIGABRT )
	{
#ifdef _JVAS_NONTRIVIAL_
		g_jvas_onexit_triggered = 1;
#endif
		g_jvas_vmdeath_triggered = 1;
	}

	if( g_jvas_old_sighandler[signum] )
		g_jvas_old_sighandler[signum]( signum );
}
#endif

// Function: will be called at normal process termination
JVAS_LOCAL void _jvas_lib_onexit( void )
{
	g_jvas_onexit_triggered = 1;
}

JVAS_LOCAL __unused__ void _jvas_jvm_onexit( void )
{
	g_jvas_onexit_triggered = 2;
}

// Function:
JVAS_LOCAL void __unused__ _jvas_print_logo( void )
{
	FILE *hf = fopen( "logo.txt", "rb" );
	if( hf )
	{
		fseek( hf, 0, SEEK_END );
		ssize_t sz = ftell( hf );
		if( sz > 0 && sz < 10240 )
		{
			char *buf = malloc(sz + 32);
			if( buf )
			{
				fseek( hf, 0, SEEK_SET );
				fread( buf, 1, sz, hf );
				buf[sz] = 0;
				printf( "\e[1;33m%s\e[0m", buf );
				free( buf );
			}
		}
		fclose( hf );
	}

	printf( "\e[1;32m>>> %s Created by CXX <<<\e[0m\n", _JVAS_IDENT_ );
}

// Function: print capability
JVAS_LOCAL void __unused__ _jvas_print_capa( const jvmtiCapabilities *capa )
{
	char buf[2048];
	int len;
	len = sprintf( buf, "JVMTI Capabilities{\n\t" );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_tag_objects );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_generate_field_modification_events );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_generate_field_access_events );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_get_bytecodes );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_get_synthetic_attribute );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_get_owned_monitor_info );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_get_current_contended_monitor );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_get_monitor_info );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_pop_frame );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_redefine_classes );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_signal_thread );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_get_source_file_name );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_get_line_numbers );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_get_source_debug_extension );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_access_local_variables );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_maintain_original_method_order );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_generate_single_step_events );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_generate_exception_events );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_generate_frame_pop_events );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_generate_breakpoint_events );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_suspend );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_redefine_any_class );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_get_current_thread_cpu_time );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_get_thread_cpu_time );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_generate_method_entry_events );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_generate_method_exit_events );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_generate_all_class_hook_events );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_generate_compiled_method_load_events );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_generate_monitor_events );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_generate_vm_object_alloc_events );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_generate_native_method_bind_events );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_generate_garbage_collection_events );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_generate_object_free_events );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_force_early_return );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_get_owned_monitor_stack_depth_info );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_get_constant_pool );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_set_native_method_prefix );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_retransform_classes );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_retransform_any_class );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_generate_resource_exhaustion_heap_events );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_generate_resource_exhaustion_threads_events );
#if 0
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_generate_early_vmstart );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_generate_early_class_hook_events );
    len += SPRINT_VAR2( buf+len, "%d;\n\t", *capa, can_generate_sampled_object_alloc_events );
#endif
	sprintf( buf+len-1, "}" );  // delete last '\t'

	printf( "%s\n", buf );
}


// Function:
JVAS_LOCAL __unused__ void _jvas_list_modifiable_classes( JTIENV_PTR jti, JNIENV_PTR jni )
{
	jint n;
	jclass *cls = NULL;
	jint r = JNICall( jti, GetLoadedClasses, &n, &cls );
	if( r == JVMTI_ERROR_NONE && cls )
	{
		jint i, acc;
		jlong tag;
		jboolean bl;
		char *signature_ptr=NULL, *generic_ptr=NULL;

		for( i=0; i<n; i++ )
		{
			if( JNICall(jti, IsModifiableClass, cls[i], &bl) == JVMTI_ERROR_NONE && bl )
			{
				r = JNICall( jti, GetClassSignature, cls[i], &signature_ptr, &generic_ptr );
				if( r == JVMTI_ERROR_NONE )
				{
					JNICall( jti, GetClassModifiers, cls[i], &acc );
					if( signature_ptr )
					{
						JNICall( jti, SetTag, cls[i], i+1000 );
						JNICall( jti, GetTag, cls[i], &tag );

						printf( "%ld: %02x, %s\n", tag, _CF_ENDIAN_FLIP_ACCES(acc), signature_ptr );
						JTI_Deallocate( jti, signature_ptr );
					}
					if( generic_ptr )
						JTI_Deallocate( jti, generic_ptr );
				}
			}
			JNICall( jni, DeleteLocalRef, cls[i] );
		}

		JTI_Deallocate( jti, cls );
	}
}


// Function: Initiate basic environment regarding to java
// Note: only be called by the Agent_OnLoad or the thread daemon
JVAS_LOCAL int _jvas_init_java_env( JVM_PTR jvm, jint jvmti_ver, int flag, JNIENV_PTR *ret_jnienv )
{
	// Note:
	// 1, JavaVM is available across process, means it be shared among all threads
	// 2, jvmtiEnv is available across process, means it be shared among all threads
	// 3, JNIEnv is available only in one threads
	//
	// * if the argument jvm is not NULL, means this function is called by Agent_OnLoad or JNI_OnLoad


	if( !BsemTake(g_jvas_lock_env, WAIT_FOREVER) )
		return -1;


	jsize n;
	JVM_PTR jvm2;
	jint r;
	juint8 fromOnLoad = flag & 1;
	juint8 onlyJNIEnv = flag & 0x80;

	JNIENV_PTR jnienv = NULL;
	JTIENV_PTR jtienv = NULL;


	if( (jvm2=jvm) ||
		((r=JNI_GetCreatedJavaVMs( &jvm2, 1, &n )) == JNI_OK && jvm2 && n == 1) )
	{
		if( fromOnLoad )
			goto LAB_GET_JTIENV;  // only get jvmti environment

		if( g_jvas_def_jtienv && !onlyJNIEnv )
		{
			// shutdown previous jvmtiEnv
			JNICall( g_jvas_def_jtienv, DisposeEnvironment );
			g_jvas_def_jtienv = NULL;
		}

		if( g_jvas_def_jnienv )
		{
			// detach previous JNIEnv
			JNICall( jvm2, DetachCurrentThread );
			g_jvas_def_jnienv = NULL;
		}

		// MUST attach to JNI at first
		r = _jvas_attach_jvm( &jvm2, &jnienv, g_jvas_jvm_version );  // do attach always, JNI_VERSION_1_8
		if( r != JNI_OK )
			goto LAB_EXIT;

		r = JNICall( jvm2, GetEnv, (void**)&jnienv, g_jvas_jvm_version );  // get JNIEnv, JNI_VERSION_1_8
		if( r != JNI_OK && !jnienv )
			goto LAB_EXIT;

		LAB_GET_JTIENV:
		if( !onlyJNIEnv )
		{
			// ask jvmti connects to the jvm
			r = JNICall( jvm2, GetEnv, (void**)&jtienv, jvmti_ver == 0 ? g_jvas_jvmti_version : jvmti_ver );
			if( r != JNI_OK || !jtienv )
				goto LAB_EXIT;

			// always get JVM phase
			JNICall( jtienv, GetPhase, &g_jvas_jvm_phase );

			// require capabilities
			jvmtiCapabilities capa;

			r = JNICall( jtienv, GetPotentialCapabilities, &capa);
			if( r == JVMTI_ERROR_NONE )
				r = JNICall( jtienv, AddCapabilities, &capa );

			// get current real capabilities and print out
			JNICall( jtienv, GetCapabilities, &g_jvas_jvmti_caps );

			_jvas_print_capa( &g_jvas_jvmti_caps );

			// save environment
			g_jvas_def_jvm = jvm2;
			g_jvas_def_jtienv = jtienv;
		}

		if( !fromOnLoad )
			g_jvas_def_jnienv = jnienv;  // only keep the JNIENV of thread daemon

		if( ret_jnienv )
			*ret_jnienv = jnienv;

#ifdef _JVAS_DEBUG
		// FOR DEBUG
		//_jvas_test( g_jvas_def_jvm, g_jvas_def_jtienv, g_jvas_def_jnienv );
#endif
		r = 100;
	}

	LAB_EXIT:
	BsemGive( g_jvas_lock_env );

	return -(r != 100);
}


// Function: destroy basic environment was built on _jvas_init_java_env
JVAS_LOCAL void _jvas_release_java_env( int cause )
{
	if( cause == 1 )
		return;  // caused by process termination

	if( BsemTake(g_jvas_lock_env, WAIT_FOREVER) )
	{
		if( g_jvas_def_jvm )
		{
			if( g_jvas_def_jtienv )
			{
				JNICall( g_jvas_def_jtienv, DisposeEnvironment );
			}

			if( g_jvas_def_jnienv )
			{
				JNICall( g_jvas_def_jvm, DetachCurrentThread );
			}
		}

		g_jvas_def_jvm = NULL;
		g_jvas_def_jtienv = NULL;
		g_jvas_def_jnienv = NULL;

		BsemGive( g_jvas_lock_env );
	}
}


// Function: Try to attach current thread to the JVM, and to obtain the JavaVM and the JNIEnv
JVAS_LOCAL int _jvas_attach_jvm( JVM_PTR *jvm, JNIENV_PTR *jenv, jint jni_ver )
{
	if( *jvm == NULL )
	{
		if( !g_jvas_def_jvm )
		{
			errno = errNotReady;
			return -1;
		}

		*jvm = g_jvas_def_jvm;
	}

	JavaVMAttachArgs arg = {
			.version = jni_ver == 0 ? g_jvas_jvm_version : jni_ver,
			.name    = NULL,
			.group   = NULL
	};

	return JNICall(*jvm, AttachCurrentThread, (void**)jenv, &arg );
}


// Function: Detach current thread from the JVM, and set NULL
__unused__
JVAS_LOCAL void _jvas_detach_jvm( JVM_PTR *jvm )
{
	JNICall(*jvm, DetachCurrentThread );
	*jvm = NULL;
}


//
JVAS_LOCAL void _jvas_print_system_properties( JTIENV_PTR jtienv )
{
	char *val, **props = NULL;
	jint count;

	jvmtiError er = JNICall( jtienv, GetSystemProperties, &count, &props );
	if( er == JVMTI_ERROR_NONE && props )
	{
		for( --count; count >= 0; count-- )
		{
			if( props[count] )
			{
				val = NULL;
				er = JNICall( jtienv, GetSystemProperty, props[count], &val );
				if( er == JVMTI_ERROR_NONE && val )
				{
					_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "System Property: %s = %s\n", props[count], val );
					JNICall( jtienv, Deallocate, (unsigned char*)val );
				}
				else
					_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "System Property: %s\n", props[count] );
				JNICall( jtienv, Deallocate, (unsigned char*)props[count] );
			}
		}
		JNICall( jtienv, Deallocate, (unsigned char*)props );
	}

	//char **env = __environ;
	//while( *env ) printf( "%s\n", *env ), env++;
}

//
JVAS_LOCAL __unused__ int _jvas_set_system_properties( JTIENV_PTR jtienv )
{
	jvmtiError er;
	size_t sz, ln;
	char *val, *newval;
	int r = -1;

	//java.class.path
	val = NULL;
	er = JNICall( jtienv, GetSystemProperty, "java.class.path", &val );

	ln = (val?strlen(val):0);
	sz = ln + strlen(g_jvas_conf.esse_class.home) + strlen(g_jvas_conf.run.java.classpath) + 128;  //1024;
	newval = malloc( sz );
	if( !newval )
		goto LAB_EXIT;

	sprintf( newval, "%s%s%s:%s", val?val:"", val?":":"", g_jvas_conf.esse_class.home, g_jvas_conf.run.java.classpath );
	er = JNICall( jtienv, SetSystemProperty, "java.class.path", newval );
	if( er != JVMTI_ERROR_NONE )
		_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "failed to set system property \"java.class.path=%s\", retval=%d\n" , newval, er );

	if( val )
		JNICall( jtienv, Deallocate, (unsigned char*)val );


	//java.library.path
	val = NULL;
	er = JNICall( jtienv, GetSystemProperty, "java.library.path", &val );
	ln = (val?strlen(val):0) + strlen(g_jvas_conf.run.java.libpath) + 32;
	if( ln > sz )
	{
		free( newval );
		sz = ln + 128;
		newval = malloc( sz );
		if( !newval )
			goto LAB_EXIT;
	}

	sprintf( newval, "%s%s%s", val?val:"", val?":":"", g_jvas_conf.run.java.libpath );
	er = JNICall( jtienv, SetSystemProperty, "java.library.path", newval );
	if( er != JVMTI_ERROR_NONE )
		_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "failed to set system property \"java.library.path=%s\", retval=%d\n", newval, er );

	free( newval );
	r = 0;

	LAB_EXIT:
	if( val )
		JNICall( jtienv, Deallocate, (unsigned char*)val );
	return r;
}


//
JVAS_LOCAL int _jvas_add_loader_search( JTIENV_PTR jtienv )
{
	jvmtiError er;

	int n;
	for( n=0; n < g_jvas_conf.class_loader.jar_count; n++ )
	{
		if( g_jvas_conf.class_loader.jars[n].bits.bootld )
		{
			_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "Bootstrap Class Loader Search: %s\n", g_jvas_conf.class_loader.jars[n].name );
			er = JNICall( jtienv, AddToBootstrapClassLoaderSearch, g_jvas_conf.class_loader.jars[n].name );
			if( er != JVMTI_ERROR_NONE )
			{
				_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "failed to Add Bootstrap Class Loader Search, %d\n", er );
				if( g_jvas_conf.class_loader.jars[n].bits.must )
					break;
			}
		}
		else
		if( g_jvas_conf.class_loader.jars[n].bits.sysld )
		{
			_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "System Class Loader Search: %s\n", g_jvas_conf.class_loader.jars[n].name );
			er = JNICall( jtienv, AddToSystemClassLoaderSearch, g_jvas_conf.class_loader.jars[n].name );    // Why blocked when called sometimes
			if( er != JVMTI_ERROR_NONE )
			{
				_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "failed to Add System Class Loader Search, %d\n", er );
				if( g_jvas_conf.class_loader.jars[n].bits.must )
					break;
			}
		}
	}

	return -(n != g_jvas_conf.class_loader.jar_count);
}


// Function: load a specified class
// Note: it must be called by the task daemon
JVAS_LOCAL jobject _jvas_load_class( JNIENV_PTR jnienv, struct jvas_class_conf *clsptr )
{
	FILE *hf;
	size_t ln;
	char *buf;
	jobject jcls = NULL;

	const int fnsz = 4096;
	char *fn = malloc( fnsz );
	if( !fn )
		return NULL;

	if( clsptr->name[0] == '/' )
		snprintf( fn, fnsz-1, "%s.class", clsptr->name );
	else
		snprintf( fn, fnsz-1, "%s%s%s.class",
				  g_jvas_conf.esse_class.home, g_jvas_conf.esse_class.package, clsptr->name );

	if( (hf = fopen( fn, "rb" )) )
	{
		fseek( hf, 0, SEEK_END );
		ln = ftell( hf );
		fseek( hf, 0, SEEK_SET );

		if( ln > 0 && ln < 0x7FFFFFFF )
		{
			buf = malloc( ln );
			if( buf )
			{
				if( fread( buf, 1, ln, hf ) == ln )
				{
					JNICall( jnienv, ExceptionClear );

					jcls = JNICall( jnienv, DefineClass, NULL, NULL, (jbyte*)buf, (jsize)ln );
					if( jcls && JNICall(jnienv, ExceptionCheck) == FALSE )
					{
						_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "succeed to define class %p of \"%s\"\n", jcls, fn );
					}
					else
					{
						jcls = NULL;
						_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "failed to define class \"%s\"\n", fn );

						if( JNICall(jnienv, ExceptionCheck) )
							JNICall( jnienv, ExceptionDescribe );
					}
				}
				free( buf );
			}
		}

		fclose( hf );
	}
	else
	{
		char cwd[256];
		getcwd( cwd, sizeof(cwd)-1 );
		cwd[sizeof(cwd)-1] = 0;
		_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "failed to open class file \"%s\", cwd=\"%s\"\n", fn, cwd );
	}

	free( fn );

	return  jcls;
}


// Function: make reference to essential classes which services as "SWITCH", "TOOL" for Sentry Agent
// Note: it must be called by the task daemon
JVAS_LOCAL int _jvas_refer_classes( JNIENV_PTR jnienv )
{
	if( g_jvas_conf.esse_class.count <= 0 )
		return 0;

	int n, d, b, ln;
	jobject jcls;
	SABI_Status sta;

	const int fnsz = 4096;
	char *fn = malloc( fnsz );
	if( !fn )
		return -1;

	JNICall( jnienv, ExceptionClear );

	d = snprintf( fn, fnsz-1, "%s%s", g_jvas_conf.esse_class.home, g_jvas_conf.esse_class.package );
	for( n=g_jvas_conf.esse_class.count-1; n >= 0; n-- )
	{
		ln = strlen( g_jvas_conf.esse_class.classes[n].name );
		if( d+ln >= fnsz )
		{
			if( g_jvas_conf.esse_class.classes[n].bits.must )
				break;
			continue;
		}

		b = g_jvas_conf.esse_class.classes[n].name[0] == '/';

		g_jvas_conf.esse_class.classes[n].handle = NULL;

		strcpy( &fn[d], g_jvas_conf.esse_class.classes[n].name+b );

		jcls = JNICall( jnienv, FindClass, fn+b*d );
		if( jcls )
		{
			_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "succeed to locate the class %p of \"%s\"\n", jcls, fn );


			LAB_CLASS_RDY:
			// new global reference
			g_jvas_conf.esse_class.classes[n].handle = JNICall( jnienv, NewGlobalRef, jcls );
			JNICall( jnienv, DeleteLocalRef, jcls );

			if( (jcls=g_jvas_conf.esse_class.classes[n].handle) )
			{
				if( g_jvas_conf.esse_class.classes[n].bits.sabi )
				{
					// bind necessary static methods at first
					if( jvas_swi_bind_methods( jnienv, n ) == 0 )
					{
						// check status before "Begin"
						sta.flags = jvas_swi_GetStatus( jnienv, &g_jvas_conf.esse_class.classes[n] );
						if( !sta.f_ready )
						{
							// initiate jvas_swi
							if( jvas_swi_Begin( jnienv, &g_jvas_conf.esse_class.classes[n],
												_JVAS_VERSION_, g_jvas_conf.run.jvas_swi_libpath, NULL ) == 0 )
							{
								// do something
								_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "succeed to self checking in the class \"%s\"\n", fn );
							}
							else
								_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "failed to do self checking in the class \"%s\"\n", fn );
						}
						else if( sta.flags == -1 )
							_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "failed to get status of the class \"%s\"\n", fn );
						else
							_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_INFO, "current status of the class \"%s\" is %llx\n", fn, sta.flags );
					}
					else
					{
						JNICall( jnienv, DeleteGlobalRef, jcls );
						g_jvas_conf.esse_class.classes[n].handle = NULL;
						_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "can not bind essential methods of the class \"%s\"\n", fn );
					}
				}
			}
			else
				_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "failed to create a new global reference of class\n" );
		}
		else
		{
			_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "failed to locate the class \"%s\"\n", fn );
			if( JNICall(jnienv, ExceptionCheck) )
				JNICall( jnienv, ExceptionDescribe );

			if( g_jvas_conf.esse_class.classes[n].bits.defi )
			{
				jcls = _jvas_load_class( jnienv, &g_jvas_conf.esse_class.classes[n] );
				if( jcls )
					goto LAB_CLASS_RDY;
			}
		}

		if( !g_jvas_conf.esse_class.classes[n].handle &&
			g_jvas_conf.esse_class.classes[n].bits.must )
			break;
	}

	JNICall( jnienv, ExceptionClear );
	free( fn );

	return  -(n >= 0);
}


// Function: cancel references of all classes
// Note: it must be called by the task daemon
JVAS_LOCAL void _jvas_unrefer_classes( JNIENV_PTR jnienv )
{
	int n;

	for( n=g_jvas_conf.esse_class.count-1; n >= 0; n-- )
	{
		if( g_jvas_conf.esse_class.classes[n].handle )
		{
			jvas_swi_unbind_methods( jnienv, n );
			JNICall( jnienv, DeleteGlobalRef, g_jvas_conf.esse_class.classes[n].handle );
			g_jvas_conf.esse_class.classes[n].handle = NULL;
		}
	}
}


// Function: initiate environment of class transformation
JVAS_LOCAL __unused__ int _jvas_tc_env_create( const char *home )
{
	if( jvas_tc_base_open( &g_jvas_tc_base ) != 0 )
	{
		_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "an error occurred when call the jvas_tc_base_open\n" );
		return -1;
	}

	int s;
	if( (s=jvas_tc_loadEx(home, &g_jvas_tc_base, g_jvas_conf.tc.preload)) < 1 )
	{
		_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "failed to load definitions of class-transformation from the directory \"%s\"\n", home );
		return -1;
	}

	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "succeed to load %d records of \"class-transformation\", with %lu classes\n", s, g_jvas_tc_base.lnk.num );
	return 0;
}


// Function: reversed operating against the _jvas_transform_init
JVAS_LOCAL __unused__ void _jvas_tc_env_destroy( void )
{
	jvas_tc_free( &g_jvas_tc_base );
	jvas_tc_base_close( &g_jvas_tc_base, FALSE );
}

// Function: start task daemon and some initiate works
// Note: may be called twice respectively by the destructor and the Agent_OnUnLoad,
//       DO NOT call it from other routines
JVAS_LOCAL int _jvas_begin( JVM_PTR jvm, char *options )
{
	if( g_jvas_daemon_thd == NULL )
	{
		g_jvas_daemon_wait = BsemCreate( SEM_EMPTY );

		g_jvas_daemon_exit = 0;

		g_jvas_daemon_thd =
				TaskCreate( "tDaemon_"_JVAS_IDENT_,
							1024*1024*2,	// large thread stack for potential consumption in java environment
							(TASK_FUNC)_jvas_task_daemon,
							(TASK_ARG)jvm,
							TASK_OPTION_EXPLICIT_SCHED | TASK_OPTION_SCHED_RR | TASK_OPTION_EXCEPTION_HANDLE,
							20,
							&g_jvas_daemon_tid );	}

	return -(g_jvas_daemon_thd == NULL);
}


// Function: stop task daemon and some final works
// Note: may be called twice respectively by the destructor and the Agent_OnUnLoad,
//       DO NOT call it from other routines
JVAS_LOCAL void _jvas_end( JVM_PTR jvm )
{
	if( g_jvas_daemon_thd && g_jvas_daemon_tid )
	{
		// notice task daemon to exit
		g_jvas_daemon_exit = 1;
		BsemGive( g_jvas_daemon_wait );

		WaitTaskSafeExit( g_jvas_daemon_thd, 5000, 1 );

		SemDelete( g_jvas_daemon_wait );
		g_jvas_daemon_thd = NULL;
		g_jvas_daemon_tid = 0;
	}
}


JVAS_LOCAL char *g_jvas_stdout_path = NULL;
JVAS_LOCAL char *g_jvas_stderr_path = NULL;

//
JVAS_LOCAL void _jvas_change_stdio( void )
{
	int ln;
	char *path;
	char *log_path = getenv( _JVAS_ENVNAME_LOGPATH );

	if( log_path )
	{
		if( access( log_path, F_OK ) != 0 )
		{
			if( mkdir( log_path, 0775 ) != 0 )
				_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "can not build the directory \"%s\" with an errno %d\n",
						log_path, errno );
		}

		path = malloc( 4096 );
		if( path )
		{
			getcwd( path, 4096 );
			_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "current work directory:\"%s\"\n", path );

			// save original absolute path of stdout and stderr
			g_jvas_stdout_path = path;
			sprintf( path, "/proc/self/fd/%d", fileno(stdout) );
			ln = readlink( path, g_jvas_stdout_path, 2047 );
			if( ln > 0 )
				g_jvas_stdout_path[ln] = 0;

			//
			g_jvas_stderr_path = path += 2048;
			sprintf( path, "/proc/self/fd/%d", fileno(stderr) );
			ln = readlink( path, g_jvas_stderr_path, 2047 );
			if( ln > 0 )
				g_jvas_stderr_path[ln] = 0;
		}

		ChangeStdioe( log_path, "", NULL, &stdout, &stderr, 0 );
	}
	else
	{
		path = getcwd( NULL, 0 );
		if( !path && (path=malloc(4096)) )
			getcwd( path, 4096 );
		if( path )
		{
			_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "current work directory:\"%s\"\n", path );
			free( path );
		}
		else
			_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "CRITICAL ERROR!!!" );
	}
}

//
JVAS_LOCAL void _jvas_restore_stdio( void )
{
	if( g_jvas_stdout_path )
	{
		stdout = freopen( g_jvas_stdout_path,  "w+b", stdout );
		stderr = freopen( g_jvas_stderr_path,  "w+b", stderr );

		g_jvas_stdout_path = g_jvas_stderr_path = NULL;
		free( g_jvas_stdout_path );
	}
}

// Function: initiate agent, only called by the Agent_OnLoad
JVAS_LOCAL int _jvas_agent_init( JTIENV_PTR jtienv )
{
	int n;
	char *path_home = getenv( _JVAS_ENVNAME_HOME );

	__sync_fetch_and_and( &g_jvas_ready, 0 );

	// set essential system properties
	_jvas_set_system_properties( g_jvas_def_jtienv );

	// add class loader search
	_jvas_add_loader_search( jtienv );

	// show system properties
	_jvas_print_system_properties( jtienv );

#if (_JVAS_VTRACE_ != 2 )
	// create TC-Environment
#if (_JVAS_VTRACE_ != 0 )
	_jvas_tc_env_create( g_jvas_conf.tc.home );
#else
	if( _jvas_tc_env_create(g_jvas_conf.tc.home) != 0 )
		goto LAB_ERROR;
#endif

	// setup default/predefined JVMTI events handlers
	n = 0;
	jtievt_ehr_setup( jtienv, &n );
	if( n <= 0 )
	{
		_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "failed to setup java events handlers. Terminated immediately(%p, %d)\n", g_jvas_def_jtienv, n );
		goto LAB_ERROR;
	}
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "succeed to setup JVM events handlers\n" );
#endif

#if (_JVAS_VTRACE_ != 0)
	// 2022-09-29
	if( vt_startup(path_home) == 0 )
	{
		if( vt_java_initiate( NULL, NULL, g_jvas_jvm_version, g_jvas_jvmti_version ) == 0 )
			_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "succeed to initiating the vTrace\n" );
		else
			_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "failed to initiating the vTrace(java_api)\n" );
	}
	else
		_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "failed to initiating the vTrace\n" );
#endif

	// notice
	__sync_fetch_and_add( &g_jvas_ready, 1 );
	return 0;

	LAB_ERROR:
	return -1;
}

// Function: sentry control routine
// Note: Only called by the thread daemon
JVAS_LOCAL int _jvas_sentry_ctrl( int opc, void *dat, long len )
{
	JNIENV_PTR jnienv = g_jvas_def_jnienv;

	if( !jnienv )
	{
		errno = errNotReady;
		return -1;
	}


	switch( opc )
	{
	case SABI_CTRL_DISABLE:
		// TODO: do something
		dat = 0;
	case SABI_CTRL_ENABLE:
		// TODO: do something

	case SABI_CTRL_SET_MODE:{
		// call jvas_swi_method: SetMode
		int md = (int)(long)dat;
		if( jvas_swi_SetMode( jnienv, &g_jvas_conf.esse_class.classes[0], md ) == md )
		{
			// OK
			_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "JVAS_SWI_SetMode OK\n" );
			return 0;
		}
		return -1;
	}
	case SABI_CTRL_RESET:
	case SABI_CTRL_GET_MODE:

	case SABI_CTRL_GET_CAPACITIES:
	case SABI_CTRL_SET_CAPACITIES:
	case SABI_CTRL_GET_SETTINGS:
	case SABI_CTRL_SET_SETTINGS:
	case SABI_CTRL_CLR_JOURNAL:
	case SABI_CTRL_SET_JOURNAL:
		errno = errUnsupport;
		break;
	default:
		errno = EPERM;
		break;
	}
	return -1;
}


// ==============================================================================================================
// Tasks
// ==============================================================================================================

// Task: main daemon task
JVAS_LOCAL TASK_RET _jvas_task_daemon( TASK_ARG arg )
{
	int n __unused__;
	char *path_home __unused__;

	// change stdio if possible
	_jvas_change_stdio();

	path_home = getenv( _JVAS_ENVNAME_HOME );
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "daemon task %ld enter\n", gettid() );

#ifdef _JVAS_NONTRIVIAL_
	__sync_fetch_and_and( &g_jvas_ready, 0 );

	if( arg == NULL)
	{
		// means, it be called by constructor

		// MUST initiate environment before enter into further phase
		_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "try to establish necessary JVM environments\n" );
		if( _jvas_init_java_env( NULL, 0, 0, NULL ) != 0 )
		{
			_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "failed to initiate run environment. Terminated immediately\n" );
			__sync_fetch_and_add( &g_jvas_ready, -1 );  // notice
			return (TASK_RET)-1;
		}
	}
#endif

	// to ensures the JVM phase is in living
	if( g_jvas_jvm_phase != JVMTI_PHASE_LIVE && g_jvas_jvm_phase != JVMTI_PHASE_DEAD )
	{
		_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "The JVM phase is \"%s\" now. Going to wait for it ready\n", _jvas_phase_name( g_jvas_jvm_phase ) );

		while( !g_jvas_daemon_exit && g_jvas_jvm_phase != JVMTI_PHASE_LIVE && g_jvas_jvm_phase != JVMTI_PHASE_DEAD )
		{
			BsemTake( g_jvas_daemon_wait, 100 );
			if( g_jvas_daemon_exit )
				break;

			n = JNICall( g_jvas_def_jtienv, GetPhase, &g_jvas_jvm_phase );
			if( n == JVMTI_ERROR_UNATTACHED_THREAD )
			{
				if( _jvas_init_java_env( (JVM_PTR)arg, 0, 0x80, NULL ) != 0 )  // get JNIEnv only
					goto LAB_FINAL;
			}
		}
	}

	if( g_jvas_jvm_phase == JVMTI_PHASE_DEAD || g_jvas_daemon_exit )
		goto LAB_FINAL;

#ifndef _JVAS_NONTRIVIAL_
	if( arg )
	{
		// be called by Agent_OnLoad()
		// MUST, get JNI environment at beginning
		n = _jvas_attach_jvm( &g_jvas_def_jvm, &g_jvas_def_jnienv, g_jvas_jvm_version );  // do attach always, JNI_VERSION_1_8
		if( n != JNI_OK )
			goto LAB_FINAL;

		n = JNICall( g_jvas_def_jvm, GetEnv, (void**)&g_jvas_def_jnienv, g_jvas_jvm_version );  // get JNIEnv, JNI_VERSION_1_8
		if( n != JNI_OK && !g_jvas_def_jnienv )
			goto LAB_FINAL;
	}
#endif

	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "The JVM phase is \"%s\" now\n", _jvas_phase_name( g_jvas_jvm_phase ) );

#ifdef _JVAS_NONTRIVIAL_
	// add class loader search
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "add class loader search if possible\n" );
	_jvas_add_loader_search( g_jvas_def_jtienv );

	// show system properties
	_jvas_print_system_properties( g_jvas_def_jtienv );
#endif

	//
	jvas_swi_initiate( g_jvas_def_jnienv );

	// refers to essential classes
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "try to refer to or load necessary JAVA classes which serves for sentry\n" );
	if( _jvas_refer_classes(g_jvas_def_jnienv) != 0 )
	{
		_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "failed to load essential java classes. Terminated immediately\n" );
		goto LAB_EXIT;
	}

	// enable sentry if it is set as automatic
	if( g_jvas_conf.run.mode & SABI_MODE_AUTO )
	{
		if( _jvas_sentry_ctrl( SABI_CTRL_ENABLE, (void*)(long)g_jvas_conf.run.mode, 0 ) != 0 )
		{
			_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "failed to enable sentry. Terminated immediately(errno:%d)\n", errno );
			goto LAB_EXIT;
		}
		_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "succeed to enable sentry automatically\n" );
	}


#ifdef _JVAS_NONTRIVIAL_
#if (_JVAS_VTRACE_ != 2 )
	// create TC-Environment
#if (_JVAS_VTRACE_ != 0 )
	_jvas_tc_env_create( g_jvas_conf.tc.home );
#else
	if( _jvas_tc_env_create(g_jvas_conf.tc.home) != 0 )
		goto LAB_EXIT;
#endif

	// setup default/predefined JVMTI events handlers
	n = 0;
	jtievt_ehr_setup( g_jvas_def_jtienv, &n );
	if( n <= 0 )
	{
		_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "failed to setup java events handlers. Terminated immediately(%p, %d)\n", g_jvas_def_jtienv, n );
		goto LAB_EXIT;
	}
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "succeed to setup JVM events handlers\n" );
#endif

#if (_JVAS_VTRACE_ != 0)
	// 2022-09-29
	if( vt_startup(path_home) == 0 )
	{
		if( vt_java_initiate( NULL, NULL, /*g_jvas_def_jvm, g_jvas_def_jtienv,*/ g_jvas_jvm_version, g_jvas_jvmti_version ) == 0 )
			_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "succeed to initiating the vTrace\n" );
		else
			_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "failed to initiating the vTrace(java_api)\n" );
	}
	else
		_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "failed to initiating the vTrace\n" );
#endif

	// notice
	__sync_fetch_and_add( &g_jvas_ready, 1 );
#endif

#if (_JVAS_VTRACE_ != 2)
	if( !g_jvas_daemon_exit )
	{
#ifdef _JVAS_NONTRIVIAL_
		// re-transform interesting classes have been loaded
		_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "begin to re-transforming(stage one)\n" );
		jvas_tc_retransform_all_classes( g_jvas_def_jtienv, g_jvas_def_jnienv, &g_jvas_tc_base );
#else
		if( g_jvas_conf.tc.delay > 0 )
		{
			// re-transforming stage two
			BsemTake( g_jvas_daemon_wait, g_jvas_conf.tc.delay );
			if( !g_jvas_daemon_exit )
			{
				// re-transform interesting classes have been loaded
				_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "delayed re-transforming(stage two)\n" );
				jvas_tc_retransform_classes( g_jvas_def_jtienv, g_jvas_def_jnienv, &g_jvas_tc_base, FALSE, TRUE );
			}
		}
#endif
	}
#endif

	//
	_jvas_print_logo();
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "enter into daemon loop\n" );

	// waiting for exit
	while( !g_jvas_daemon_exit )
	{
		int r=BsemTake( g_jvas_daemon_wait, 30000 );
		_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "jvas daemon, %ld, %p, %d\n", gettid(), g_jvas_daemon_thd, r );
	}
	// =========================================================================================================

	if( g_jvas_onexit_triggered != 1 && g_jvas_onexit_triggered != 2 )
	{
		// disable sentry always
		_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "disable sentry\n" );
		_jvas_sentry_ctrl( SABI_CTRL_DISABLE, NULL, 0 );

#if (_JVAS_VTRACE_ != 2)
		__sync_fetch_and_add( &g_jvas_ready, 1 );  // notice, set on "restoring"

		// stabilizing
		while( g_jvas_jtievt_refers != 0 ) usleep( 20000 );

		// re-transform selected classes have been loaded(restoring)
		_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "restore re-transformed classes\n" );
		jvas_tc_retransform_classes2( g_jvas_def_jtienv, g_jvas_def_jnienv, &g_jvas_tc_base );

		// waiting for completion of restoring
		while( g_jvas_tc_base.count_class_transformed > 0 ) usleep( 20000 );

		// cancel all JVMTI events
		jtievt_ehr_cancel( g_jvas_def_jtienv );

		// stabilizing
		while( g_jvas_jtievt_refers != 0 ) usleep( 20000 );

		// destroy TC-Environment
		_jvas_tc_env_destroy();
#endif
	}

#if (_JVAS_VTRACE_ != 0)
	// 2022-09-29
	vt_java_shutdown( g_jvas_onexit_triggered==1 || g_jvas_onexit_triggered==2 );
	vt_shutdown();
#endif

	LAB_EXIT:
	if( g_jvas_onexit_triggered != 1 && g_jvas_onexit_triggered != 2 )
	{
		_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "cancel all JVM references\n" );
		// cancel references of all classes have been referred before
		_jvas_unrefer_classes( g_jvas_def_jnienv );
		//
		jvas_swi_release( g_jvas_def_jnienv );
	}

#ifdef _JVAS_NONTRIVIAL_
	// 如果是由主程序中止引起的, 则不能再调用jvm相关的接口, 不然会异常
	// destroy environment
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "destroy all JVM environments\n" );
	_jvas_release_java_env( g_jvas_onexit_triggered );
#endif

	LAB_FINAL:
	__sync_fetch_and_add( &g_jvas_ready, -2 );  // notice
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "daemon task leaves caused by %s\n", g_jvas_onexit_triggered<4?_JVAS_EXIT_CAUSE[g_jvas_onexit_triggered]:"TMD" );

	_jvas_restore_stdio();
	return 0;
}


// ==============================================================================================================
// Export Functions( says APIs )
// ==============================================================================================================

// library reference counter
JVAS_LOCAL volatile long g_jvas_lib_ref = 0;	// for system shared library manage
JVAS_LOCAL volatile long g_jvas_agent_ref = 0;	// for JNI

#ifdef _JVAS_NONTRIVIAL_
#define __JVAS_CONSTRUCTOR__	__constructor__
#define __JVAS_DESTRUCTOR__		__destructor__
#else
#define __JVAS_CONSTRUCTOR__
#define __JVAS_DESTRUCTOR__
#endif

//
extern void libcTaskInit( void );
extern void libcTaskRelease( void );

// Function: constructor
__JVAS_CONSTRUCTOR__ void jvas_lib_load( void )
{
	// Note: this function will be called at beginning
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "<<< on library loading, lib_ref:%ld <<<\n", g_jvas_lib_ref );
	g_jvas_onexit_triggered = 0;

	if( __sync_add_and_fetch( &g_jvas_lib_ref, 1) == 1 )
	{
		g_jvas_jvm_phase = 0;  // always set 0 when the library on loading

		// to check the specific environ file at current work directory
		/*FILE *hf = fopen( _JVAS_ENVFILE, "wb" );
		if( hf )
		{
			long ll[3];

			if( fwrite( ll, sizeof(ll), 1, hf ) == 1 )  // head
			{
				if( ll[0] == (long)&sentry_library_environ && ll[1] == getpid() )  // verify
				{
				}
			}

			fclose( hf );
		}*/


		char *path_home = getenv( _JVAS_ENVNAME_HOME );

		// initiate essential modules
		libcTaskInit();
		UM_Initiate();
		xmlInitiate();

		// to load essential configures
		if( _jvas_load_config( path_home, &g_jvas_conf ) != 0 )
			goto LAB_ERROR;

		_jvas_print_conf( &g_jvas_conf );

		// to check configures
		if( g_jvas_conf.esse_class.count < 1 )
		{
			_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "bad configures\n" );
			goto LAB_ERROR;
		}

		// do something for initiating library
		g_jvas_lock_env = BsemCreate( SEM_FULL );

		// register a exit event function
		// Note: do not use the on_exit, because of on_exit will register on library-self, not root-program
		atexit( _jvas_lib_onexit );

#ifdef _JVAS_SIGNAL_ENABLE_
		g_jvas_old_sighandler[SIGQUIT] = signal( SIGQUIT, _jvas_on_signal );
		g_jvas_old_sighandler[SIGTERM] = signal( SIGTERM, _jvas_on_signal );
		g_jvas_old_sighandler[SIGABRT] = signal( SIGABRT, _jvas_on_signal );
		g_jvas_old_sighandler[SIGINT] = signal( SIGINT, _jvas_on_signal );
#endif

#ifndef _JVAS_NONTRIVIAL_
		// MUST allocate a separate memory for _jvas_jvm_onexit, otherwise the JVM will crush when exit
		//JVM_OnExit( _jvas_jvm_onexit ); // DO NOT ENABLED
#endif

		// initiate JVMTI events related
		jtievt_init();

#ifdef _JVAS_NONTRIVIAL_
		// start jvas daemon task
		if( _jvas_begin(NULL, NULL) == 0 )
		{
			// setup library environment for Spectrum-Sentry-Specification
			sentry_library_environ.magic = SABI_ENVIRON_MAGIC;
			sentry_library_environ.size  = sizeof(SABI_Environ);
			sentry_library_environ.ver = _JVAS_VERSION_;
			sentry_library_environ.func.Initiate = sentry_library_initiate;
			sentry_library_environ.func.Release = sentry_library_release;
			sentry_library_environ.func.Ctrl = jvas_sentry_ctrl;

			_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "be loaded successfully\n" );
		}
		else
		{
			__sync_sub_and_fetch( &g_jvas_lib_ref, 1 );
			_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "failed to start the daemon task\n" );
		}
#else
		_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, " be loaded successfully\n" );
#endif  // _JVAS_NONTRIVIAL_
	}
	else
	{
		_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_INFO, "library has been loaded\n" );
	}

	return;

	LAB_ERROR:
	_jvas_restore_stdio();
	xmlRelease();
	UM_Release();
	libcTaskRelease();
	__sync_sub_and_fetch( &g_jvas_lib_ref, 1 );
}


// Function: destructor
__JVAS_DESTRUCTOR__ void  jvas_lib_unload( void )
{
	// Note: this function will be called at the end
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, ">>> on library unloading, lib_ref:%ld, cause:%s >>>\n",
			g_jvas_lib_ref, g_jvas_onexit_triggered < 4 ? _JVAS_EXIT_CAUSE[g_jvas_onexit_triggered] : "TMD" );

	if( g_jvas_onexit_triggered == 1 )
		return;

	if( __sync_sub_and_fetch( &g_jvas_lib_ref, 1) == 0 )
	{
#ifdef _JVAS_NONTRIVIAL_
		// stop jvas daemon task
		_jvas_end( NULL );
#endif

		// free other resources
		xmlRelease();
		UM_Release();
		libcTaskRelease();

		_jvas_free_conf( &g_jvas_conf );

		// do something for release library
		if( g_jvas_lock_env )
			SemDelete( g_jvas_lock_env );

		// clean sentry_library_environ
		if( sentry_library_environ.env )
			free( sentry_library_environ.env );
		memset( (void*)&sentry_library_environ, 0, sizeof(sentry_library_environ) );

		_jvas_restore_stdio();

		_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "library unloaded\n" );
	}
	else
	{
		_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "reference decrease\n" );
	}
}


#if 1  //def _JVAS_NONTRIVIAL_

// Function: Required for Spectrum-Sentry specification, will be called by Spectrum-Main after loading
int sentry_library_initiate( int argc, void **argv )
{
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "%s(%d, %p)\n", __FUNCTION__, argc, argv );

	// arguments:
	// [0]: library-self handle
	// [1]: identifier
	// [2]: version required
	// [3]: work directory(optional)
	// [4]: 0(optional)

	if( argc < 3 )
		return -1;

	if( g_jvas_lib_ref > 0 && g_jvas_onexit_triggered == 3 &&
		g_jvas_daemon_thd && g_jvas_daemon_tid )
	{
		// Note: Special scene that this sentry has been release via sentry_library_release previous by Controller,
		// but this library been loaded by multiple times and/or multiple consumer, so the library is still alive
		// and keep loaded status.
		// So, try to restart sentry by calling unload and load in sequence
		// CARE MUST BE TAKEN, it is abnormal usage
		jvas_lib_unload();
		jvas_lib_load();
	}

	if( !g_jvas_daemon_thd || !g_jvas_daemon_tid )
		return -1;

	sentry_library_environ.hlib  = argv[SABI_FUN_INITIATE_ARG_IDX_LIBHD];
	sentry_library_environ.ident = (long)argv[SABI_FUN_INITIATE_ARG_IDX_IDENT];
	sentry_library_environ.ver = (long)argv[SABI_FUN_INITIATE_ARG_IDX_VER];
	sentry_library_environ.env = NULL;

	int env_len = 0;
	if( argc > SABI_FUN_INITIATE_ARG_IDX_ENV && argv[SABI_FUN_INITIATE_ARG_IDX_ENV] )
	{
		const char *env = (const char*)argv[SABI_FUN_INITIATE_ARG_IDX_ENV];
		env_len = strlen( env );
		if( env_len > 0 )
		{
			sentry_library_environ.env = malloc( env_len + 4 );
			strcpy( sentry_library_environ.env, env );
			//printf( "$$$$$$    %s\n", sentry_library_environ.env );
		}
	}

	// write to the specific-file at current work directory
	/*FILE *hf = fopen( _JVAS_ENVFILE, "wb" );
	if( hf )
	{
		long ll[] = {
				(long)&sentry_library_environ,
				getpid(),
				env_len + _OFFSET(SABI_Environ, func)
		};

		fwrite( ll, sizeof(ll), 1, hf );

		fwrite( (void*)&sentry_library_environ, _OFFSET(SABI_Environ, func), 1, hf );

		if( env_len > 0 )
			fwrite( sentry_library_environ.env, 1, env_len, hf );

		fclose( hf );
	}*/

	return _JVAS_VERSION_;
}

// Function: Required for Spectrum-Sentry specification, will be called by Spectrum-Main before unloading
void sentry_library_release( void )
{
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "%s\n", __FUNCTION__ );

	g_jvas_onexit_triggered = 3;

	if( g_jvas_daemon_thd && g_jvas_daemon_tid )
	{
		// notice task daemon to exit
		g_jvas_daemon_exit = 1;
		BsemGive( g_jvas_daemon_wait );
	}
}

// Function: Required for Spectrum-Sentry specification, control sentry behavior
int jvas_sentry_ctrl( int opc, void *dat, long len )
{
	return -1;
}

#endif  // _JVAS_NONTRIVIAL_


// Function: Required for JNI specification, will be called by JVM  when dynamically be loaded
JVAS_EXPORT
jint JNICALL JNI_OnLoad( JVM_PTR jvm, void *reserved )
{
	long ref = __sync_add_and_fetch( &g_jvas_agent_ref, 1 );

	// just only return required version of JVM
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "%s, agent reference count is %ld\n", __FUNCTION__, ref );
	return g_jvas_jvm_version;
}
__alias__("JNI_OnLoad") jint JNICALL JNI_OnLoad_Sentry();
__alias__("JNI_OnLoad") jint JNICALL JNI_OnLoad_jvas();

// Function: Required for JNI specification, will be called by JVM when dynamically be unloaded
JVAS_EXPORT
void JNICALL JNI_OnUnload( JVM_PTR jvm, void *reserved )
{
	long ref = __sync_sub_and_fetch( &g_jvas_agent_ref, 1 );

	// nothing to do now
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "%s, agent reference count is %ld\n", __FUNCTION__, ref );

#if 0 //def _JVAS_DEBUG	// FOR DEBUG
	if( g_jvas_def_jtienv )
	{
		if( JNICall( g_jvas_def_jtienv, GetPhase, &g_jvas_jvm_phase ) == JVMTI_ERROR_NONE )
		{
			if( g_jvas_jvm_phase == JVMTI_PHASE_DEAD )
				g_jvas_onexit_triggered = 2;
			_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "%s: %s\n", __FUNCTION__, _jvas_phase_name(g_jvas_jvm_phase) );
		}
	}
#endif
}
__alias__("JNI_OnUnload") jint JNICALL JNI_OnUnload_Sentry();
__alias__("JNI_OnUnload") jint JNICALL JNI_OnUnload_jvas();

#if (_JVAS_VTRACE_ != 2)

// Function: Required for JVMTI specification
// The VM will start the agent by calling this function. It will be called by JVM early enough in VM initialization.
JVAS_EXPORT
jint JNICALL Agent_OnLoad( JVM_PTR jvm, char *options, void *reserved )
{
	// ASSUME that will be called only one time by JVM in process life
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "Agent_OnLoad is called by JVM\n" );

#ifndef _JVAS_NONTRIVIAL_
	// must call library constructor at first
	jvas_lib_load();
#endif


	// MUST initiate environment before call the _jvas_begin
	if( _jvas_init_java_env( jvm, 0, 1, NULL ) == 0 )
	{
		//_jvas_set_system_properties( g_jvas_def_jtienv );

		if( _jvas_agent_init( g_jvas_def_jtienv ) == 0 )
		{
			// start thread daemon
			if( _jvas_begin( jvm, options ) == 0 )
			{
				// TODO:
				return 0;
			}
		}
	}
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "failed to initiate run environment. Terminated immediately\n" );

	return -1;
}
__alias__("Agent_OnLoad") jint JNICALL Agent_OnLoad_Sentry();
__alias__("Agent_OnLoad") jint JNICALL Agent_OnLoad_jvas();


// Function: Required for JVMTI specification, will be called by JVM when the library is about to be unloaded
// Note that it will be called even if in the case of VM start-up failure
JVAS_EXPORT
void JNICALL Agent_OnUnload( JVM_PTR jvm )
{
	// ASSUME that will be called only one time by JVM in process life
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "Agent_OnUnload is called by JVM\n" );

#ifndef _JVAS_NONTRIVIAL_
	g_jvas_onexit_triggered = 1;
#endif

	_jvas_end( jvm );


#ifndef _JVAS_NONTRIVIAL_
	// release environment if possible
	_jvas_release_java_env( 3 );

	// must call library destructor at the end
	jvas_lib_unload();
#endif
}
__alias__("Agent_OnUnload") jint JNICALL Agent_OnUnload_Sentry();
__alias__("Agent_OnUnload") jint JNICALL Agent_OnUnload_jvas();


// Function: Required for JVMTI specification, will be called by JVM when starting on "live phase".
//           (It will be called in the context of a thread that is attached to the VM)
JVAS_EXPORT
jint JNICALL Agent_OnAttach( JVM_PTR jvm, char *options, void *reserved )
{
	_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "%s was replaced by the library constructor\n", __FUNCTION__ );
	return JVMTI_ERROR_NONE;
}
__alias__("Agent_OnAttach") jint JNICALL Agent_OnAttach_Sentry();
__alias__("Agent_OnAttach") jint JNICALL Agent_OnAttach_jvas();

#endif  // (_JVAS_VTRACE_ != 2)


#ifdef _JVAS_DEBUG

jint JNICALL jvas_jni_test( JNIENV_PTR jni, jobject jcls, jlong no, jstring str )
{
	const char *_str = JNICall( jni, GetStringUTFChars, str, 0 );
	if( str )
	{
		printf( "%s: %ld, %s\n", __FUNCTION__, no, _str );
		JNICall( jni, ReleaseStringUTFChars, str, _str );
	}
	else
		printf( "%s: %ld, \"\"\n", __FUNCTION__, no );

	//printf( "this is native call(%p, %ld, %p)\n", jni, no, str );
	return 0;
}

__alias__("jvas_jni_test") jint JNICALL Java_jsentry_jvas_1swi_test__JLjava_lang_String_2();
__alias__("jvas_jni_test") jint JNICALL Java_jsentry_1test_jvas_1test_test__JLjava_lang_String_2();
//__alias__("jvas_jni_test") jint JNICALL Java_jsentry_jvas_test( jlong, jstring );


//
int jvas_test( void )
{
	JavaVMInitArgs initArgs = {
			.version = JVAS_JNI_VERSION_DEF,
			.nOptions = 0,
			.options = NULL,
			.ignoreUnrecognized = TRUE
	};

	JVM_PTR jvm;
	JNIENV_PTR jnienv;

	jint r = JNI_CreateJavaVM( &jvm, (void**)&jnienv, &initArgs );
	r = _jvas_init_java_env( 0, 0, 0, NULL );
	r = JNICall( jvm, DestroyJavaVM );
	return r;
}

#if 0
//
JVAS_LOCAL TASK_RET _jvas_test_thread( TASK_ARG arg )
{
	int x=0;
	while( !x )
		sleep(1);

	Agent_OnLoad( 0, 0, 0 );
	return NULL;
}

#if 1
//
int jvas_test2( void )
{

	TASKHD tk;
	TASKID tid;
	tk = TaskCreate( _JVAS_IDENT_"_test", 1024*256,
					 (TASK_FUNC)_jvas_test_thread, NULL,
					  TASK_OPTION_EXPLICIT_SCHED | TASK_OPTION_SCHED_RR | TASK_OPTION_EXCEPTION_HANDLE,
					  20, &tid );

	WaitTaskSafeExit( tk, WAIT_FOREVER, 1 );
	return 0;
}
#endif
#endif

#endif

