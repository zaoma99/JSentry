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

#ifndef _JVAS_H_
#define _JVAS_H_

#include <sys/types.h>
#include "jbase.h"

#define JVAS_LOCAL	static
#define JVAS_EXTERN extern

#ifdef _LINUX
#include "gcc/gcc_base.h"

#define JVAS_HIDDEN 			__attribute__((visibility("hidden")))
#define JVAS_NOEXPORT	extern	__attribute__((visibility("hidden")))
#define JVAS_VISIBLE 			__attribute__((visibility("default")))
#define JVAS_EXPORT		extern	__attribute__((visibility("default")))
#define JVAS_ALIAS(__origin__, __target__)	__attribute__((alias(__origin__))) __target__
#ifndef __unused__
#define __unused__		__attribute__ ((unused))
#endif

#elif _WINDOWS

#define JVAS_HIDDEN
#define JVAS_NOEXPORT extern
#define JVAS_VISIBLE		__declspec(dllexport)
#define JVAS_EXPORT	extern	__declspec(dllexport)
#ifndef __unused__
#define __unused__		__declspec (unused)
//#define JVAS_APICALL	__stdcall
#endif
#endif

#define JVAS_APICALL	JNICALL


// macros for building
#ifndef _EXE
#define _JVAS_LIB_			// build of shared library only

//#define _JVAS_NONTRIVIAL_	// build for nontrivial usage

#define _JVAS_VTRACE_	(1)	// include vTrace, 0=disable, 1=enable, 2=enable and vtrace only
#endif

//#define _JVAS_DEVELOP_



//
#define _JVAS_NAME_		"jvas"
#define _JVAS_LIBNAME_	"jsentry"

#define _JVAS_VERSION_	0x00010000		// comply with the SABI

#define _JVAS_IDENT_	_JVAS_NAME_ "-v1.0.0"


// default configures of java environment
#if (_JVM_VER == 11)
#define JVAS_JNI_VERSION_DEF	JNI_VERSION_10
#define JVAS_JVMTI_VERSION_DEF	JVMTI_VERSION_11
#else
#define JVAS_JNI_VERSION_DEF	JNI_VERSION_1_8
#define JVAS_JVMTI_VERSION_DEF	JVMTI_VERSION_1_2
#endif


// fixed enrion, path or filename
#define _JVAS_ENVNAME_HOME		"JSENTRY_HOME"
#define _JVAS_ENVNAME_LOGPATH	"JSENTRY_LOGPATH"

#define _JVAS_CONFPATH		"conf"
#define _JVAS_CONFNAME		"JVASconf.xml"
#define _JVAS_ENVFILE		"JVASenvrion.dat"

#define _JVAS_DEBUG_PRINT_( _FD_, _LOG_TYPE_, _FORMAT_, _ARGS_... )    _DEBUG_PRINT_( _FD_, _LOG_TYPE_, _JVAS_IDENT_":"_FORMAT_, ##_ARGS_ )


// JVM PHASE NAME
#define _JVAS_JVM_PHASE_NAME {\
	"Unknown",    \
	"OnLoad",     \
	"Primordial", \
	"Start",      \
	"Live",       \
	"Dead"        \
}


//
struct jvas_method_desc
{
	char  *name;
	char  *sign;
	long   ctxid;
	uint8  must;
	uint8  dis;  // disable
	uint8  mod;  // mode, used by jvas_tc.c; bit[0~3]=mode, bit[4~7]=EECM
	uint8  flag; // used by jvas_tc.c

	/*
	Mode: 1="standard", 2="anywhere", 3="mixed"
    EECM: under the mode "standard", the mask of Entry, Exit and Catch;
          1=No Entry; 2=No Exit; 4=No Catch;
    Flag: 1=ask to insert an object "this" at the head of signature(means the stub method is static);
          2=ask to replace all objects with the "java/lang/Object";
          4=ask to replace array with the "java/lang/Object";
          8=ask to insert a "TPIX" into stub code
    */
};

struct jvas_class_conf;

struct jvas_class_method
{
	const struct jvas_method_desc *desc;
	struct jvas_class_conf *klass;
	jmethodID jmtd;
};

#define JVAS_CONF_FLAG union{ \
	struct{ \
		int must :1; \
		int main :1; \
		int defi :1; \
		int sabi :1; \
		int res3 :4; \
		int bootld :1; \
		int sysld  :1; \
		int dis  :1; \
		int lazy :1; \
	}bits;           \
	int  flags;      \
}

struct jvas_class_conf
{
	char *name;
	JVAS_CONF_FLAG;

	//
	jclass handle;
	int method_count;
	struct jvas_class_method *method_table;
};

struct jvas_jar_conf
{
	char *name;
	JVAS_CONF_FLAG;
};

// configures
typedef struct
{
	struct{
		int nontrival;
		int mode;
		struct{
			char classpath[NAME_MAX+1];
			char libpath[NAME_MAX+1];
		}java;
		char jvas_libpath[NAME_MAX+1];
		char jvas_swi_libpath[NAME_MAX+1];
	}run;

	struct{
		char home[NAME_MAX+1];
		char package[NAME_MAX+1];
		int  count;
		struct jvas_class_conf *classes;
	}esse_class;

	struct{
		int jar_count;
		struct jvas_jar_conf *jars;
	}class_loader;

	struct{
		char home[NAME_MAX+1];
		BOOL preload;
		long delay;
	}tc;
}JVAS_Conf;

#define _JVAS_CONF_ESSE_CLASS_MAX	1000
#define _JVAS_CONF_LDSRC_JAR_MAX	1000




#ifdef __cplusplus
extern "C"{
#endif


inline static const char* _jvas_phase_name( int phase )
{
	switch( phase )
	{
	case JVMTI_PHASE_ONLOAD    : return "OnLoad" ;
	case JVMTI_PHASE_PRIMORDIAL: return "Primordial";
	case JVMTI_PHASE_START     : return "Start";
	case JVMTI_PHASE_LIVE      : return "Live";
	case JVMTI_PHASE_DEAD      : return "Dead";
	default:
		return "Unknown";
	}
}


#if 1  //def _JVAS_NONTRIVIAL_

// Function: Required for Spectrum-Sentry specification, will be called by Spectrum-Main after loading
JVAS_EXPORT int sentry_library_initiate( int argc, void **argv );

// Function: Required for Spectrum-Sentry specification, will be called by Spectrum-Main before unloading
JVAS_EXPORT void sentry_library_release( void );

// Function: Required for Spectrum-Sentry specification, control sentry behavior
JVAS_EXPORT int jvas_sentry_ctrl( int opc, void *dat, long len );

#endif



// Function: Required for JNI specification, will be called when dynamically loading library
JVAS_EXPORT
jint JNICALL JNI_OnLoad( JVM_PTR jvm, void *reserved );

// Function: Required for JNI specification, will be called when dynamically unloading library
JVAS_EXPORT
void JNICALL JNI_OnUnload( JVM_PTR jvm, void *reserved );

// Function: Required for JVMTI specification
// The VM will start the agent by calling this function. It will be called early enough in VM initialization.
JVAS_EXPORT
jint JNICALL Agent_OnLoad( JVM_PTR jvm, char *options, void *reserved );

// Function: Required for JVMTI specification, will be called when starting on "live phase".
//           (It will be called in the context of a thread that is attached to the VM)
JVAS_EXPORT
jint JNICALL Agent_OnAttach( JVM_PTR jvm, char *options, void *reserved );

// Function: Required for JVMTI specification, will be called when the library is about to be unloaded
JVAS_EXPORT
void JNICALL Agent_OnUnload( JVM_PTR jvm );



// Function: load run configures
JVAS_NOEXPORT int _jvas_load_config( const char *confPath, JVAS_Conf *confPtr );

// Function: free resource of configures pointed to by the confPtr
JVAS_NOEXPORT void _jvas_free_conf( JVAS_Conf *confPtr );

// Function: print configures
JVAS_NOEXPORT void _jvas_print_conf( const JVAS_Conf *confPtr );


#ifdef _JVAS_DEBUG

//
JVAS_NOEXPORT int jvas_test( void );
JVAS_NOEXPORT int jvas_test2( void );

#endif

#ifdef __cplusplus
}
#endif

#endif // _JVAS_H_
