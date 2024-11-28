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

#ifndef _JBASE_H_
#define _JBASE_H_

#include <sys/types.h>
#include "jni.h"
#include "jvmti.h"
//#include "jvm.h"

//#include "jmm.h"

#include "stypes.h"
#include "sabi.h"


typedef unsigned char  juint8;
typedef unsigned short juint16;
typedef unsigned int   juint32;
typedef unsigned long long juint64;

typedef char  jint8;
typedef short jint16;
typedef int   jint32;
typedef long long jint64;

#define _ALIGN_SIZE_(_N_, _B_)		( (_N_ + (_B_ - 1)) & -(_B_) )
#define _ALIGN_ADDR_	_ALIGN_SIZE_

#define _MAKE_INT16_( _H_, _L_ )	\
	( (_H_)<<8 | (_L_) )

#define _MAKE_INT32_( _H3_, _H2_, _H1_, _L_)	\
	( ((_H3_)<<24) | ((_H2_)<<16) | ((_H1_)<<8) | (_L_) )

#define _MAKE_INT64_( _H7_, _H6_, _H5_, _H4_, _H3_, _H2_, _H1_, _L_)	\
	( ((_H7_)<<56) | ((_H6_)<<48) | ((_H5_)<<40) | ((_H4_)<<32) | ((_H3_)<<24) | ((_H2_)<<16) | ((_H1_)<<8) | (_L_) )



#define _ERROR_PUT_( __msgstr__ ) \
	printf( "An error(%d) occurred due to \"%s\", at %s:%d\n", errno, __msgstr__, __FUNCTION__, __LINE__ )

#define _ERROR_RETURN_( __retval__, __msgstr__ ) \
do{\
	_ERROR_PUT_( __msgstr__ );\
	return __retval__;\
}while(0)


#if 0 //def _DEBUG
//#define _DEBUG_PRINT_( _FD_, _LOG_TYPE_, _FORMAT_, _ARGS_... )	fprintf( _FD_, _FORMAT_, ##_ARGS_ )

#define _DEBUG_PRINT_( _FD_, _LOG_TYPE_, _FORMAT_, _ARGS_... )	do{\
	if( (_LOG_TYPE_) & (SABI_JOURNAL_DEBUG| SABI_JOURNAL_WARN | SABI_JOURNAL_ERROR| SABI_JOURNAL_EVENT| SABI_JOURNAL_CRIT | SABI_JOURNAL_MT ) ) \
		fprintf( _FD_, _FORMAT_, ##_ARGS_ ); \
}while(0)

#else

extern int g_jvas_journal_mask;

#define _DEBUG_PRINT_( _FD_, _LOG_TYPE_, _FORMAT_, _ARGS_... )	do{\
	if( (_LOG_TYPE_) & g_jvas_journal_mask ) \
		fprintf( _FD_, _FORMAT_, ##_ARGS_ ); \
}while(0)
#endif


#define SHOW_VAR( __show__, __FMT__, __VAR__ ) 	__show__( "%s="__FMT__, #__VAR__, __VAR__ )
#define SHOW_VAR2( __show__, __FMT__, __OBJ__, __VAR__ ) __show__( "%s="__FMT__, #__VAR__, (__OBJ__).__VAR__ )

#define PRINT_VAR( __FMT__, __VAR__ )	SHOW_VAR( printf, __FMT__, __VAR__ )
#define PRINT_VAR2( __FMT__, __OBJ__, __VAR__ )	SHOW_VAR2( printf, __FMT__, __OBJ__, __VAR__ )

#define SPRINT_VAR( __BUF__, __FMT__, __VAR__ )	sprintf( __BUF__, "%s="__FMT__, #__VAR__, __VAR__ )
#define SPRINT_VAR2( __BUF__, __FMT__, __OBJ__, __VAR__ )	sprintf( __BUF__, "%s="__FMT__, #__VAR__, (__OBJ__).__VAR__ )


// some of macros, definitions, and/or wrappers for convenience of regarding to JNI and JVMTI
typedef JavaVM   *JVM_PTR;
typedef JNIEnv   *JNIENV_PTR;
typedef jvmtiEnv *JTIENV_PTR;


#ifdef __cplusplus
#define JNIRef(__env_ptr__)	(*(__env_ptr__))

#define JNIFunc(__env_ptr__, __func__)	(__env_ptr__)->__func__

#define JNICall(__env_ptr__, __func__, __args__...)	(__env_ptr__)->__func__(__args__)

#define JNICallNoArg(__env_ptr__, __func__)	(__env_ptr__)->__func__()

#else
#define JNIRef(__env_ptr__)	(*(*(__env_ptr__)))

#define JNIFunc(__env_ptr__, __func__)	(*(__env_ptr__))->__func__

#define JNICall(__env_ptr__, __func__, __args__...)	(*(__env_ptr__))->__func__(__env_ptr__, ## __args__)

#define JNICallNoArg(__env_ptr__, __func__)	JNICall( __env_ptr__, __func__ )

#endif

// JVMTI: Allocate
#define JTI_Allocate( __env__, __size__, __ptr__ )	JNICall( (__env__), Allocate, (__size__), (unsigned char**)(__ptr__) )

// JVMTI: Deallocate
#define JTI_Deallocate( __env__, __ptr__ )	JNICall( (__env__), Deallocate, (unsigned char*)(__ptr__) )





#ifdef __cplusplus
extern "C"{
#endif



#ifdef __cplusplus
}
#endif

#endif // _JBASE_H_
