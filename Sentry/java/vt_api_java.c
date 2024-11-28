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

#include "vt_api_java.h"
#include "jvas_swi.h"
#include "classfile_util.h"

#include "vt_api.h"
#include "common/mmp.h"
#include "common/secure.h"

#include <pthread.h>

#include "jvm.h"


#ifdef VT_WITH_JAVA  // defined in vt_api.h

// compatible with vtrace data type
const vt_dict g_vt_java_DataType[] = {
		{/*.tag=,*/ .key="void",    .val.l=VT_TYPE_NULL},
		{/*.tag=,*/ .key="char",    .val.l=VT_TYPE_WORD},
		{/*.tag=,*/ .key="short",   .val.l=VT_TYPE_SHORT},
		{/*.tag=,*/ .key="int",     .val.l=VT_TYPE_INT},
		{/*.tag=,*/ .key="long",    .val.l=VT_TYPE_LONG},
		{/*.tag=,*/ .key="byte",    .val.l=VT_TYPE_BYTE},
		{/*.tag=,*/ .key="boolean", .val.l=VT_TYPE_BOOL},
		{/*.tag=,*/ .key="float",   .val.l=VT_TYPE_FLOAT},
		{/*.tag=,*/ .key="double",  .val.l=VT_TYPE_DOUBLE},
		{/*.tag=,*/ .key="char[]",  .val.l=VT_TYPE_CSTRING},
		{/*.tag=,*/ .key="byte[]",  .val.l=VT_TYPE_BSTRING},
};


// java primitive type
const vt_dict g_vt_java_PrimitiveType[] = {
		{/*.tag=,*/ .key="void",    .val.l=JVM_SIGNATURE_VOID},
		{/*.tag=,*/ .key="char",    .val.l=JVM_SIGNATURE_CHAR},
		{/*.tag=,*/ .key="short",   .val.l=JVM_SIGNATURE_SHORT},
		{/*.tag=,*/ .key="int",     .val.l=JVM_SIGNATURE_INT},
		{/*.tag=,*/ .key="long",    .val.l=JVM_SIGNATURE_LONG},
		{/*.tag=,*/ .key="byte",    .val.l=JVM_SIGNATURE_BYTE},
		{/*.tag=,*/ .key="boolean", .val.l=JVM_SIGNATURE_BOOLEAN},
		{/*.tag=,*/ .key="float",   .val.l=JVM_SIGNATURE_FLOAT},
		{/*.tag=,*/ .key="double",  .val.l=JVM_SIGNATURE_DOUBLE},
};

#define VT_JAVA_MAKE_DDS1( _ctx_, _md_, _src1_, _len1_, _out_, _outsz_ ) (\
		EVP_DigestInit_ex( _ctx_, _md_, NULL ) && \
		EVP_DigestUpdate( _ctx_, _src1_, _len1_ ) && \
		EVP_DigestFinal_ex( _ctx_, _out_, _outsz_ ) )

#define VT_JAVA_MAKE_DDS2( _ctx_, _md_, _src1_, _len1_, _src2_, _len2_, _out_, _outsz_ ) (\
		EVP_DigestInit_ex( _ctx_, _md_, NULL ) && \
		EVP_DigestUpdate( _ctx_, _src1_, _len1_ ) && \
		EVP_DigestUpdate( _ctx_, _src2_, _len2_ ) && \
		EVP_DigestFinal_ex( _ctx_, _out_, _outsz_ ) )

#define VT_JAVA_MAKE_DDS3( _ctx_, _md_, _src1_, _len1_, _src2_, _len2_, _src3_, _len3_, _out_, _outsz_ ) (\
		EVP_DigestInit_ex( _ctx_, _md_, NULL ) && \
		EVP_DigestUpdate( _ctx_, _src1_, _len1_ ) && \
		EVP_DigestUpdate( _ctx_, _src2_, _len2_ ) && \
		EVP_DigestUpdate( _ctx_, _src3_, _len3_ ) && \
		EVP_DigestFinal_ex( _ctx_, _out_, _outsz_ ) )

// Abbreviations:
// O:= Optional
// M:= Must
// N:= Necessary
// P:= Proposal

#define _VT_JAVA_DEBUG_LEVEL_	5	/* 0: disable ALL
									   1: disable SOT-APPEND
									   2: in the _java_sot_append, before to call the vt_java_sot_entity_extract
									   3: in the _java_sot_append, after call the vt_java_sot_entity_extract
									   4: in the _java_sot_append, before to call the vt_sot_append
									   5: anything disabled
 	 	 	 	 	 	 	 	 	 */

#define _THREAD_LOCAL_JTIENV_

//#define _RECURSION_DISABLE_	// disable recursion of API

//#define _THREAD_LOCAL_JNIENV_

#define _JTIENV_PERSISTENT_

//#define _VT_JAVA_TPIDX_DDS_

#define _VT_JAVA_PARAMETER_LIGHT_

#define _VT_JAVA_EXTRACT_LIGHT_

//#define _VT_JAVA_JNI_FAST_


//#define _VT_JAVA_DELREF_EVERY_TIME_

#ifdef _VT_JAVA_DELREF_EVERY_TIME_
#define _VT_JAVA_DelLocRef(_JNIENV_, _OBJECT_)    JNICall( _JNIENV_, DeleteLocalRef, _OBJECT_ )
#else
#define _VT_JAVA_DelLocRef(...)
#endif

#define _VT_JAVA_GetObjArrEle(_JNIENV_, _ARRAY_, _INDEX_) \
	JNICall( _JNIENV_, GetObjectArrayElement, _ARRAY_, _INDEX_ )
	//JVM_GetArrayElement( _JNIENV_, _ARRAY_, _INDEX_ )

#define _VT_JAVA_GetArrLen(_JNIENV_, _ARRAY_) \
	JNICall( _JNIENV_, GetArrayLength, _ARRAY_ )
	//JVM_GetArrayLength( _JNIENV_, _ARRAY_ )

#define _VT_JAVA_GetArrPtr(_JNIENV_, _ARRAY_) \
	JNICall( _JNIENV_, GetIntArrayElements, _ARRAY_, NULL )

#define _VT_JAVA_TakeArray(_JNIENV_, _ARRAY_, _SIZE_) \
	( (_SIZE_ == _VT_JAVA_GetArrLen(_JNIENV_, _ARRAY_)) ? _VT_JAVA_GetArrPtr(_JNIENV_, _ARRAY_) : NULL )

#define _VT_JAVA_TakeArray2(_JNIENV_, _ARRAY_, _LEN_) \
	( (_LEN_ = _VT_JAVA_GetArrLen(_JNIENV_, _ARRAY_)) > 0 ? _VT_JAVA_GetArrPtr(_JNIENV_, _ARRAY_) : NULL )

#define _VT_JAVA_GiveArray(_JNIENV_, _ARRAY_, _PTR_) \
	JNICall( _JNIENV_, ReleaseIntArrayElements, _ARRAY_, (jint*)_PTR_, JNI_ABORT )


#if 0
#define _VT_JAVA_INLINE_	inline
#else
#define _VT_JAVA_INLINE_
#endif


//
typedef struct
{
	slk_node node;

	long evtref;
	long actref;
	//long sotref;
	//jthread thread;
	long tpix_root;
	long cntr_root;

#ifdef _VT_JAVA_TPIDX_DDS_
	EVP_MD_CTX md_ctx;
#endif
	JNIENV_PTR jnienv;
	JTIENV_PTR jtienv;
	long mmphd;
}vt_java_thread_environ;

//
VT_LOCAL SEM_ID    g_vt_mutex_match_tpidx;

#if 0
VT_LOCAL slk_link         g_vt_java_envtab = {0};
VT_LOCAL SEM_ID	          g_vt_java_envtab_lock = NULL;
#endif

VT_LOCAL pthread_key_t    g_vt_java_tsd_env = -1;

VT_LOCAL jint g_vt_java_jvm_version = VT_JNI_VERSION_DEF;
VT_LOCAL jint g_vt_java_jvmti_version = VT_JVMTI_VERSION_DEF;

VT_LOCAL JVM_PTR    g_vt_java_jvm     = NULL;

VT_LOCAL JTIENV_PTR g_vt_java_jtienv0 = NULL;
VT_LOCAL JTIENV_PTR g_vt_java_jtienv2 = NULL;

VT_LOCAL jvmtiCapabilities g_vt_java_jvmti_caps;

//
VT_LOCAL jint __unused__ g_vt_java_type_size[31] = VT_TYPE_SIZE_INITIATOR;

// sentry agent configures
VT_EXTERN JVAS_Conf g_jvas_conf;

#ifdef _VT_JAVA_TPIDX_DDS_
//
VT_EXTERN const EVP_MD *g_vt_dds_md;
#endif


#ifdef _THREAD_LOCAL_JNIENV_

// Function: Try to attach current thread to the JVM, and to obtain the JavaVM and the JNIEnv
VT_LOCAL int _java_attach_jvm( JVM_PTR jvm, JNIENV_PTR *jenv, jint jni_ver )
{
	JavaVMAttachArgs arg = {
			.version = jni_ver == 0 ? g_vt_java_jvm_version : jni_ver,
			.name    = NULL,
			.group   = NULL
	};

	return JNICall(jvm, AttachCurrentThread, (void**)jenv, &arg );
}


// Function: Detach current thread from the JVM, and set NULL
VT_LOCAL void _java_detach_jvm( JVM_PTR jvm )
{
	JNICall(jvm, DetachCurrentThread );
}
#endif

//
VT_LOCAL int _java_init_jvmenv( JVM_PTR *jvmptr, JTIENV_PTR *jtiptr, jint jvmti_ver )
{
	jsize n;
	JVM_PTR jvm2;

	if( JNI_GetCreatedJavaVMs( &jvm2, 1, &n ) == JNI_OK && jvm2 && n == 1 )
	{
		// ask jvmti connects to the jvm
		JTIENV_PTR jtienv = NULL;

		if( JNICall( jvm2, GetEnv, (void**)&jtienv, jvmti_ver ) == JNI_OK && jtienv )
		{
			// always get JVM phase
			jvmtiPhase phase;
			JNICall( jtienv, GetPhase, &phase );
			_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "vt_api_java, succeed to initiate jvm environment; jvm is now on phase %d\n", phase );

			// require capabilities
			jvmtiCapabilities capa;
			if( JNICall( jtienv, GetPotentialCapabilities, &capa) == JVMTI_ERROR_NONE )
				JNICall( jtienv, AddCapabilities, &capa );

			// get current real capabilities and print out
			JNICall( jtienv, GetCapabilities, &g_vt_java_jvmti_caps );

#ifdef VT_JAVA_DEBUG
			//_jvas_print_capa( &g_vt_java_jvmti_caps );
#endif
			*jvmptr = jvm2;
			*jtiptr = jtienv;
			return 0;
		}
	}
	return -1;
}


// Function:
int vt_java_initiate( JVM_PTR jvm, JTIENV_PTR jtienv, jint jni_ver, jint jti_ver )
{
	if( pthread_key_create( &g_vt_java_tsd_env, NULL ) != 0 )
		return -1;

#if 0
	slk_Init( &g_vt_java_envtab );
	g_vt_java_envtab_lock = BsemCreate( SEM_FULL );
#endif

	g_vt_mutex_match_tpidx = BsemCreate( SEM_FULL );

	g_vt_java_jvm_version = jni_ver ? jni_ver : VT_JNI_VERSION_DEF;
	g_vt_java_jvmti_version = jti_ver ? jti_ver : VT_JVMTI_VERSION_DEF;

	if( !jvm || !jtienv )
	{
		if( _java_init_jvmenv( &jvm, &jtienv, g_vt_java_jvmti_version ) != 0 )
		{
			pthread_key_delete( g_vt_java_tsd_env );
			g_vt_java_tsd_env = -1;
		}
	}

	g_vt_java_jvm = jvm;

	g_vt_java_jtienv0 = jtienv;

	return 0;
}


// Function:
void vt_java_shutdown( BOOL jvm_exit )
{
	if( g_vt_java_tsd_env != -1 )
	{
		pthread_key_delete( g_vt_java_tsd_env );
		g_vt_java_tsd_env = -1;
	}

#if 0
	slk_node node;
	vt_java_thread_environ *env = (vt_java_thread_environ*)g_vt_java_envtab.head;
	while( env )
	{
		node = env->node;

		if( !jvm_exit )
		{
			if( env->jtienv )
				JNICall( env->jtienv, DisposeEnvironment );

#ifdef _THREAD_LOCAL_JNIENV_
			if( env->jnienv )
				_java_detach_jvm( g_vt_java_jvm );
#endif
		}

		free( env );
		env = (vt_java_thread_environ*)node.next;
	}
	slk_Init( &g_vt_java_envtab );
#endif

	if( !jvm_exit && g_vt_java_jtienv0 && g_vt_java_jtienv0 == g_vt_java_jtienv2 )
		JNICall( g_vt_java_jtienv0, DisposeEnvironment );

	g_vt_java_jtienv0 = g_vt_java_jtienv2 = NULL;

	g_vt_java_jvm = NULL;

	if( g_vt_mutex_match_tpidx )
	{
		SemDelete( g_vt_mutex_match_tpidx );
		g_vt_mutex_match_tpidx = NULL;
	}

#if 0
	if( g_vt_java_envtab_lock )
	{
		SemDelete( g_vt_java_envtab_lock );
		g_vt_java_envtab_lock = NULL;
	}
#endif
}

#if 0
//
VT_LOCAL __unused__ vt_java_thread_environ*  _java_getenv2( JNIENV_PTR jni, JTIENV_PTR jti )
{
	int r;
	jvmtiCapabilities capa;
	vt_java_thread_environ *env = (vt_java_thread_environ*)pthread_getspecific( g_vt_java_tsd_env );

	if( !env && jni && g_vt_java_jvm )
	{
		// try to setup trace environment
		env = malloc( sizeof(vt_java_thread_environ) );
		if( !env )
			return NULL;
		memset( env, 0, sizeof(vt_java_thread_environ) );

		EVP_MD_CTX_init( &env->md_ctx );

#ifdef _THREAD_LOCAL_JNIENV_
		_java_attach_jvm( g_vt_java_jvm, &env->jnienv, g_vt_java_jvm_version );
		if( JNICall( g_vt_java_jvm, GetEnv, (void**)&env->jnienv, g_vt_java_jvm_version ) != JNI_OK || !env->jnienv )
			goto LAB_ERR;
#endif

#ifndef VT_JAVA_ACTION_EQUAL_THREAD
		if( !jti )
#endif
		{
			// connect to JVMTI
			if( JNICall( g_vt_java_jvm, GetEnv, (void**)&env->jtienv, g_vt_java_jvmti_version ) != JNI_OK || !env->jtienv )
				goto LAB_ERR;

			// require capabilities
			if( JNICall( env->jtienv, GetPotentialCapabilities, &capa) == JVMTI_ERROR_NONE )
				JNICall( env->jtienv, AddCapabilities, &capa );
		}
#ifndef VT_JAVA_ACTION_EQUAL_THREAD
		else
			env->jtienv = jti;
#endif

		if( pthread_setspecific( g_vt_java_tsd_env, env ) != 0 )
		{
#ifndef VT_JAVA_ACTION_EQUAL_THREAD
			if( !jti && env->jtienv )
#endif
				JNICall( env->jtienv, DisposeEnvironment );
			goto LAB_ERR;
		}

		if( BsemTake(g_vt_java_envtab_lock, -1) )
		{
			slk_Add( &g_vt_java_envtab, (slk_node*)env, FALSE );
			BsemGive( g_vt_java_envtab_lock );
		}
	}

	return env;

	LAB_ERR:
	r = errno;
	free( env );
	errno = r;
	return NULL;
}
#endif

//
_VT_JAVA_INLINE_ VT_LOCAL vt_java_thread_environ* _java_getenv( JNIENV_PTR jni )
{
	int r;
	jvmtiCapabilities capa;
	vt_java_thread_environ *env = (vt_java_thread_environ*)pthread_getspecific( g_vt_java_tsd_env );

	if( !env && (errno == EINVAL || errno == ENOMEM) )
		return NULL;

	if( !env && jni && g_vt_java_jvm )
	{
		// try to setup trace environment
		env = malloc( sizeof(vt_java_thread_environ) );
		if( !env )
			return NULL;
		memset( env, 0, sizeof(vt_java_thread_environ) );

#ifdef _VT_JAVA_TPIDX_DDS_
		EVP_MD_CTX_init( &env->md_ctx );
#endif

#ifdef _THREAD_LOCAL_JNIENV_
		_java_attach_jvm( g_vt_java_jvm, &env->jnienv, g_vt_java_jvm_version );
		if( JNICall( g_vt_java_jvm, GetEnv, (void**)&env->jnienv, g_vt_java_jvm_version ) != JNI_OK || !env->jnienv )
			goto LAB_ERR;
#endif

#if 0
		// connect to JVMTI
		if( JNICall( g_vt_java_jvm, GetEnv, (void**)&env->jtienv, g_vt_java_jvmti_version ) != JNI_OK || !env->jtienv )
			goto LAB_ERR;

		// require capabilities
		if( JNICall( env->jtienv, GetPotentialCapabilities, &capa) == JVMTI_ERROR_NONE )
			JNICall( env->jtienv, AddCapabilities, &capa );
#endif

		if( pthread_setspecific( g_vt_java_tsd_env, env ) != 0 )
		{
			//JNICall( env->jtienv, DisposeEnvironment );
			goto LAB_ERR;
		}

#if 0
		if( BsemTake(g_vt_java_envtab_lock, -1) )
		{
			slk_Add( &g_vt_java_envtab, (slk_node*)env, FALSE );
			BsemGive( g_vt_java_envtab_lock );
		}
#endif
	}

	if( env && !env->jtienv && jni && g_vt_java_jvm )
	{
		// connect to JVMTI
		if( JNICall( g_vt_java_jvm, GetEnv, (void**)&env->jtienv, g_vt_java_jvmti_version ) == JNI_OK )
		{
			// require capabilities
			if( JNICall( env->jtienv, GetPotentialCapabilities, &capa) == JVMTI_ERROR_NONE )
				JNICall( env->jtienv, AddCapabilities, &capa );
		}
	}

	return env;

	LAB_ERR:
	r = errno;
	free( env );
	errno = r;
	return NULL;
}

//
_VT_JAVA_INLINE_ VT_LOCAL void _java_action_exit( vt_refer actref, vt_refer arg )
{
	vt_java_thread_environ *env = (vt_java_thread_environ*)arg;
	if( g_vt_java_jvm && env )
	{
#ifndef _JTIENV_PERSISTENT_
		if( env->jtienv )
		{
			JNICall( env->jtienv, DisposeEnvironment );
			env->jtienv = NULL;
		}
#endif

#ifdef _THREAD_LOCAL_JNIENV_
		if( env->jnienv )
		{
			_java_detach_jvm( g_vt_java_jvm );
			env->jnienv = NULL;
		}
#endif

		env->actref = 0;
		env->evtref = 0;
	}
}

//
inline VT_LOCAL jlong _java_object_map( vt_java_thread_environ *env, jobject obj )
{
	jlong ll;

#ifdef _DEBUG
	if( !obj )
		return -1;

	jvmtiError er = JNICall( env->jtienv, GetTag, obj, &ll );
	if( er != JVMTI_ERROR_NONE )
	{
		_VT_DEBUG_PRINT_( stderr, SABI_JOURNAL_ERROR, "%s, error, failed to GetTag of %lx, errno:%d\n", __FUNCTION__, (size_t)obj, er );
		return -1;
	}
	return ll ? ll : -1;
#else
	return JNICall( env->jtienv, GetTag, obj, &ll ) == JVMTI_ERROR_NONE && ll ? ll : -1;
#endif
}

//
inline VT_LOCAL jvmtiError _java_object_bind( vt_java_thread_environ *env, jobject obj, jlong tag )
{
	return JNICall( env->jtienv, SetTag, obj, tag );
}

//
inline VT_LOCAL jlong _java_methodID_map( jobject obj )
{
	// NOTE: g_vt_java_jtienv0 MUST
	jlong ll;
	return JNICall( g_vt_java_jtienv0, GetTag, obj, &ll ) == JVMTI_ERROR_NONE && ll ? ll : -1;
}

//
inline VT_LOCAL jvmtiError _java_methodID_bind( jobject obj, jlong tag )
{
	// NOTE: g_vt_java_jtienv0 MUST
	return JNICall( g_vt_java_jtienv0, SetTag, obj, tag );
}

//
inline VT_LOCAL jvmtiError _java_get_callframe( vt_java_thread_environ *env, jthread thread, jint depth, jvmtiFrameInfo *frame )
{
	jint count;
	jvmtiError err;

	err = JNICall( env?env->jtienv:g_vt_java_jtienv0, GetStackTrace, thread, depth, 1, frame, &count );
	if( err != JVMTI_ERROR_NONE )
		frame->method = NULL;

	return err;
}

//
VT_LOCAL jlong __unused__ _java_get_tpidx( vt_java_thread_environ *env, jthread thread )
{
	jlong tpidx;
	jclass clsptr;
	jvmtiFrameInfo frame;
	char *name, *sign, *klass, *cc;
	const vt_point *tp;
	long ll;
	vt_bool bl;
#ifdef _VT_JAVA_TPIDX_DDS_
	vt_bool dds_ok;
	vt_ident uid;
#endif

	// fetch the handle of method be traced
	if( _java_get_callframe( env, thread, VT_JAVA_ORIGIN_METHOD_STACK_DEPTH, &frame ) != JVMTI_ERROR_NONE )
		return -1;

	tpidx = _java_methodID_map( (jobject)frame.method ); // first try

	if( tpidx == ((size_t)-1)>>1 )
		return -1;  // method is not defined in trace table

	if( tpidx <= 0 )
	{
		// try to bind a tpidx with the method
		if( JNICall( env->jtienv, GetMethodName, frame.method, &name, &sign, NULL ) == JVMTI_ERROR_NONE && name && sign )
		{
			// SPECIAL CASE, SPECIAL CASE, SPECIAL CASE
			// ignore the "return type"
			if( (cc = strchr( sign, JVM_SIGNATURE_ENDFUNC )) )
				cc[1] = 0;

#ifdef _VT_JAVA_TPIDX_DDS_
			dds_ok = FALSE;
#endif
			klass = NULL;
			bl = 0;

			if( JNICall( env->jtienv, GetMethodDeclaringClass, frame.method, &clsptr ) == JVMTI_ERROR_NONE )
			{
				if( JNICall( env->jtienv, GetClassSignature, clsptr, &klass, NULL ) == JVMTI_ERROR_NONE && klass )
				{
					ll = strlen( klass );
					if( (bl = klass[0] == JVM_SIGNATURE_CLASS && klass[ll-1] == JVM_SIGNATURE_ENDCLASS) )
					{
						klass++;
						klass[ll-=2] = 0;
					}

#ifdef _VT_JAVA_TPIDX_DDS_
					dds_ok = VT_JAVA_MAKE_DDS3( &env->md_ctx, g_vt_dds_md,
							name, strlen(name), sign, strlen(sign), klass, ll, uid.b, NULL );
#endif
				}
				JNICall( env->jnienv, DeleteLocalRef, clsptr );
			}

			// find the trace point by specified name and sign
#ifdef _VT_JAVA_TPIDX_DDS_
			tp = vt_trace_point_find( dds_ok?&uid:NULL, name, sign, klass, NULL );
#else
			tp = vt_trace_point_find( NULL, name, sign, klass, NULL );
#endif
			if( tp )
			{
				tpidx = tp->idx;

				//if( tp->mtd_hd != frame.method )  // first check
				if( _java_methodID_map( (jobject)frame.method ) == -1 ) // ??? the bound value may be missed
				{
					// try to bind the tpidx
					if( g_vt_mutex_match_tpidx && BsemTake(g_vt_mutex_match_tpidx, 1000) )  // with timeout 1000ms
					{
						//if( tp->mtd_hd != frame.method )  // check again
						{
							_java_methodID_bind( (jobject)frame.method, tpidx );

							//vt_method_handle_set( tpidx, frame.method, (vt_refer)env->actref, -1 );
#if 0
							((vt_point*)tp)->mtd_hd = frame.method;  // it's illegal, but it's performance
#endif
						}
						BsemGive( g_vt_mutex_match_tpidx );
#ifdef VT_JAVA_DEBUG
						_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "%s, bind trace point with the index %ld, \"%s%s\" of \"%s\"\n",
								__FUNCTION__, tpidx, name, sign, klass?klass:"" );
#endif
					}
				}
			}
			else
			{
				if( (tpidx = _java_methodID_map( (jobject)frame.method )) == -1 )  // check again
				{
					// try to bind the invalid-tpidx, (size_t)-1>>1
					if( g_vt_mutex_match_tpidx && BsemTake(g_vt_mutex_match_tpidx, 1000) )  // with timeout 1000ms
					{
						_java_methodID_bind( (jobject)frame.method, ((size_t)-1)>>1 );
						BsemGive( g_vt_mutex_match_tpidx );
					}
#ifdef _VT_JAVA_TPIDX_DDS_
					_VT_DEBUG_PRINT_( stderr, SABI_JOURNAL_ERROR, "%s, can not match the trace point of the method \"%s%s\" of \"%s\", %llx:%llx\n",
								  __FUNCTION__, name, sign, klass?klass:"", uid.high, uid.low );
#else
					_VT_DEBUG_PRINT_( stderr, SABI_JOURNAL_ERROR, "%s, can not match the trace point of the method \"%s%s\" of \"%s\"\n",
								  __FUNCTION__, name, sign, klass?klass:"" );
#endif
				}
			}

			// MUST, release resources
			//if( name )
				JTI_Deallocate( env->jtienv, name );
			//if( sign )
				JTI_Deallocate( env->jtienv, sign );
			if( klass )
				JTI_Deallocate( env->jtienv, klass-bl );
		}
	}
	return tpidx;
}

//
VT_LOCAL jlong _java_get_tpidx1( vt_java_thread_environ *env, jstring jname, jstring jsign, jstring jklass )
{
#ifdef _VT_JAVA_TPIDX_DDS_
	vt_bool dds_ok;
	vt_ident uid;
#endif
	jlong tpidx;
	//jclass clsptr;
	jvmtiFrameInfo frame;
	const vt_point *tp;
	//long ll;
	char *name, *sign, *cc;
	const char *klass;


	// fetch the handle of method be traced
	if( _java_get_callframe( env, NULL, VT_JAVA_ORIGIN_METHOD_STACK_DEPTH, &frame ) != JVMTI_ERROR_NONE )
		return -1;

	tpidx = _java_methodID_map( (jobject)frame.method ); // first try

	if( tpidx == ((size_t)-1)>>1 )
		return -1;  // method is not defined in trace table

	if( tpidx <= 0 )
	{
		// try to bind a tpidx with the method
		if( JNICall( env->jtienv, GetMethodName, frame.method, &name, &sign, NULL ) == JVMTI_ERROR_NONE && name && sign )
		{
			// SPECIAL CASE, SPECIAL CASE, SPECIAL CASE
			// ignore the "return type"
			if( (cc = strchr( sign, JVM_SIGNATURE_ENDFUNC )) )
				cc[1] = 0;

#ifdef _VT_JAVA_TPIDX_DDS_
			dds_ok = FALSE;
#endif

			klass = JNICall( env->jnienv, GetStringUTFChars, jklass, NULL );

			//printf( ">>> %s%s  @ %s <<<\n", name, sign, klass );

#ifdef _VT_JAVA_TPIDX_DDS_
			ll = klass ? strlen(ll) : 0;
			dds_ok = VT_JAVA_MAKE_DDS3( &env->md_ctx, g_vt_dds_md,
					name, strlen(name), sign, strlen(sign), klass, ll, uid.b, NULL );
#endif

			// find the trace point by specified name and sign
#ifdef _VT_JAVA_TPIDX_DDS_
			tp = vt_trace_point_find( dds_ok?&uid:NULL, name, sign, klass, NULL );
#else
			tp = vt_trace_point_find( NULL, name, sign, klass, NULL );
#endif
			if( tp )
			{
				tpidx = tp->idx;

				//if( tp->mtd_hd != frame.method )  // first check
				if( _java_methodID_map( (jobject)frame.method ) == -1 ) // ??? the bound value may be missed
				{
					// try to bind the tpidx
					if( g_vt_mutex_match_tpidx && BsemTake(g_vt_mutex_match_tpidx, 1000) )  // with timeout 1000ms
					{
						_java_methodID_bind( (jobject)frame.method, tpidx );

#if 0
						((vt_point*)tp)->mtd_hd = frame.method;  // it's illegal, but it's performance
#endif

						BsemGive( g_vt_mutex_match_tpidx );
#ifdef VT_JAVA_DEBUG
						_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "%s, bind trace point with the index %ld, \"%s%s\" of \"%s\"\n",
								__FUNCTION__, tpidx, name, sign, klass?klass:"" );
#endif
					}
				}
			}
			else
			{
				if( (tpidx = _java_methodID_map( (jobject)frame.method )) == -1 )  // check again
				{
					// try to bind the invalid-tpidx, (size_t)-1>>1
					if( g_vt_mutex_match_tpidx && BsemTake(g_vt_mutex_match_tpidx, 1000) )  // with timeout 1000ms
					{
						_java_methodID_bind( (jobject)frame.method, ((size_t)-1)>>1 );
						BsemGive( g_vt_mutex_match_tpidx );
					}
#ifdef _VT_JAVA_TPIDX_DDS_
					_VT_DEBUG_PRINT_( stderr, SABI_JOURNAL_ERROR, "%s, can not match the trace point of the method \"%s%s\" of \"%s\", %llx:%llx\n",
								  __FUNCTION__, name, sign, klass?klass:"", uid.high, uid.low );
#else
					_VT_DEBUG_PRINT_( stderr, SABI_JOURNAL_ERROR, "%s, can not match the trace point of the method \"%s%s\" of \"%s\"\n",
								  __FUNCTION__, name, sign, klass?klass:"" );
#endif
				}
			}

			// MUST, release resources
			JTI_Deallocate( env->jtienv, name );
			JTI_Deallocate( env->jtienv, sign );
			if( klass )
				JNICall( env->jnienv, ReleaseStringUTFChars, jklass, klass );
		}
	}
	return tpidx;
}

//
_VT_JAVA_INLINE_ VT_LOCAL
jlong __unused__ _java_get_tpidx2( vt_java_thread_environ *env, jstring jsign )
{
	char *sign, *cc, *endptr;
	char buf[1024];
	jlong tpidx = -1;

	jsize len = JNICall( env->jnienv, GetStringUTFLength, jsign );
	if( len > 0 )
	{
		sign = len < sizeof(buf) ? buf : malloc( len + 4 );
		if( sign )
		{
			JNICall( env->jnienv, GetStringUTFRegion, jsign, 0, len, sign );
			sign[len] = 0;
			if( (cc = strchr( sign, JVM_SIGNATURE_ENDFUNC )) )
			{
				tpidx = strtol( ++cc, &endptr, 0 );
				if( cc == endptr || *endptr != 0 )
					tpidx = -1;
			}

			if( sign != buf )
				free( sign );
		}
	}

	return tpidx;
}


//
_VT_JAVA_INLINE_ VT_LOCAL
jlong vt_java_data_sequence_mapping( vt_java_thread_environ *env, vt_data_sequence *datseq, vt_ident *uids_ret )
{
	// always fetch the java object reference from the item "value" of a vt_tlv,
	// and treat it as an unique object identifier

	//if( !g_vt_java_jvm )
	//	return -1;

	vt_long i, n = 0;
	vt_tlv  *datas = datseq->datas;

#ifndef _VT_JAVA_EXTRACT_LIGHT_
	vt_ident *uids = datseq->uids;
	//if( !uids && !datas )
	//	return -1;
#endif

	for( i=datseq->ndat-1; i >= 0; i-- )
	{
#ifndef _VT_JAVA_EXTRACT_LIGHT_
		if( datas && datas[i].type1 == 0 )
#endif
		{
			uids_ret[i].low = datas[i].val.l;  // object reference
			uids_ret[i].high = _java_object_map( env, datas[i].val.o );  // local address of data object
			n += uids_ret[i].high != -1;
		}
#ifndef _VT_JAVA_EXTRACT_LIGHT_
		else
		{
			uids_ret[i].low = uids ? uids[i].low : 0;  // specified id
			uids_ret[i].high = 0;
		}
#endif
	}

	return n;
}

//
_VT_JAVA_INLINE_ VT_LOCAL
jlong vt_java_data_sequence_binding( vt_java_thread_environ *env, vt_data_sequence *datseq, const vt_ident *uids_old )
{
	//if( !g_vt_java_jvm )
	//	return -1;

	jvmtiError er;
	vt_long i, n = 0;
	vt_ident *uids = datseq->uids;

	for( i=datseq->ndat-1; i >= 0; i-- )
	{
		if( uids[i].low && uids[i].high && uids[i].high != -1 &&
			(!uids_old || uids[i].high != uids_old[i].high) )
		{
#ifndef VT_JAVA_DEBUG
			if( _java_object_bind( env, (jobject)uids[i].low, uids[i].high ) == JVMTI_ERROR_NONE )
				n++;
#else
			if( (er=_java_object_bind( env, (jobject)uids[i].low, uids[i].high )) == JVMTI_ERROR_NONE )
				n++;
			else
				_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "%s, set tag error(%d)\n", __FUNCTION__, er );
#endif
		}
	}

	return n;
}


//
inline VT_LOCAL jint _java_NumberValue( vt_java_thread_environ *env, vt_typeid dtype, jobject obj, jvalue *retval )
{
	return jvas_swi_NumberValue( env->jnienv, &g_jvas_conf.esse_class.classes[0], dtype, obj, retval );
}

//
_VT_JAVA_INLINE_ VT_LOCAL __unused__
void* _java_primitive_array_acquire( vt_java_thread_environ *env,
									 vt_typeid dtype, jobject obj,
									 jsize *alen, jboolean *iscopy )
{
	//JNICall( env->jnienv, ExceptionClear );

	if( (*alen = _VT_JAVA_GetArrLen( env->jnienv, obj )) <= 0
		/*|| JNICall(env->jnienv, ExceptionCheck)*/ )
		return NULL;

	switch( dtype )
	{
	case VT_TYPE_SI8:
	case VT_TYPE_UI8:
		return (void*)JNICall( env->jnienv, GetByteArrayElements, obj, iscopy );
	case VT_TYPE_SI16:
	case VT_TYPE_UI16:
		return (void*)JNICall( env->jnienv, GetShortArrayElements, obj, iscopy );
	case VT_TYPE_SI32:
	case VT_TYPE_UI32:
		return (void*)JNICall( env->jnienv, GetIntArrayElements, obj, iscopy );
	case VT_TYPE_SI64:
	case VT_TYPE_UI64:
	case VT_TYPE_POINTER:
	case VT_TYPE_TIME:
		return (void*)JNICall( env->jnienv, GetLongArrayElements, obj, iscopy );
	//case VT_TYPE_SI128:
	//case VT_TYPE_UI128:
	//	break;
	case VT_TYPE_F32:
		return (void*)JNICall( env->jnienv, GetFloatArrayElements, obj, iscopy );
	case VT_TYPE_F64:
		return (void*)JNICall( env->jnienv, GetDoubleArrayElements, obj, iscopy );
	//case VT_TYPE_F128:
	default:
		break;
	}
	return NULL;
}

_VT_JAVA_INLINE_ VT_LOCAL __unused__
jint _java_primitive_array_release( vt_java_thread_environ *env,
									vt_typeid dtype, jobject obj,
									void *ptr, jint mode )
{
	if( !ptr || !obj )
		return JNI_EINVAL;

	JNICall( env->jnienv, ExceptionClear );

	switch( dtype )
	{
	case VT_TYPE_SI8:
	case VT_TYPE_UI8:
		JNICall( env->jnienv, ReleaseByteArrayElements, obj, ptr, mode );
		break;
	case VT_TYPE_SI16:
	case VT_TYPE_UI16:
		JNICall( env->jnienv, ReleaseShortArrayElements, obj, ptr, mode );
		break;
	case VT_TYPE_SI32:
	case VT_TYPE_UI32:
		JNICall( env->jnienv, ReleaseIntArrayElements, obj, ptr, mode );
		break;
	case VT_TYPE_SI64:
	case VT_TYPE_UI64:
	case VT_TYPE_POINTER:
	case VT_TYPE_TIME:
		JNICall( env->jnienv, ReleaseLongArrayElements, obj, ptr, mode );
		break;
	//case VT_TYPE_SI128:
	//case VT_TYPE_UI128:
	//	break;
	case VT_TYPE_F32:
		JNICall( env->jnienv, ReleaseFloatArrayElements, obj, ptr, mode );
		break;
	case VT_TYPE_F64:
		JNICall( env->jnienv, ReleaseDoubleArrayElements, obj, ptr, mode );
		break;
	//case VT_TYPE_F128:
	default:
		return JNI_EINVAL;
	}

	return JNICall(env->jnienv, ExceptionCheck) ? JNI_ERR : JNI_OK;
}

#ifdef _VT_JAVA_JNI_FAST_
//
jint vt_java_string_array_acquire( vt_java_thread_environ *env, jobject src, vt_cstr **dst, vt_long *ndst )
{
	jsize n;
	vt_uint *ia = (vt_uint*)_VT_JAVA_TakeArray2( env->jnienv, src, n );
	if( !ia )
	{
		*ndst = 0;
		return JNI_OK;
	}

	jobject _objid_;
	vt_cstr *dst2 = *dst;
	jsize ndst2 = *ndst;

	if( ndst2 > n || ndst2 == 0 )
		ndst2 = n;

	if( dst2 == NULL )
	{
		dst2 = (vt_cstr*)mmp_malloc( 1, &env->mmphd, sizeof(vt_cstr)*ndst2 );
		if( !dst2 )
		{
			_VT_JAVA_GiveArray( env->jnienv, src, ia );
			return JNI_ERR;
		}
	}

	// must clear at first
	//memset( dst2, 0, sizeof(vt_cstr)*ndst2 );

	for( n=0; n < ndst2; n++ )
	{
		// UTF-8 only
		_objid_ = (jobject)(size_t)ia[n];
		dst2[n].cs = (vt_char*)JNICall( env->jnienv, GetStringUTFChars, (jobject)&_objid_, NULL );
		dst2[n].len = JNICall( env->jnienv, GetStringUTFLength, (jobject)&_objid_ );
	}

	if( n == ndst2 )
	{
		*dst = dst2;
		*ndst = ndst2;
		_VT_JAVA_GiveArray( env->jnienv, src, ia );
		return JNI_OK;
	}

	// release resources have been acquired before
	for( --n; n >= 0; n-- )
	{
		_objid_ = (jobject)(size_t)ia[n];
		JNICall( env->jnienv, ReleaseStringUTFChars, (jobject)&_objid_, dst2[n].cs );
	}

	*ndst = 0;
	_VT_JAVA_GiveArray( env->jnienv, src, ia );
	return JNI_ERR;
}

//
void vt_java_string_array_release( vt_java_thread_environ *env, jobject src, vt_cstr *astr, vt_long nastr )
{
	jobject _objid_;
	vt_long i;
	vt_long _n;
	vt_uint *ia = (vt_uint*)_VT_JAVA_TakeArray2( env->jnienv, src, _n );
	if( !ia )
		return;

	if( _n < nastr )
		nastr = _n;

	for( i=0; i < nastr; i++ )
	{
		if( astr[i].cs && astr[i].len > 0 )
		{
			_objid_ = (jobject)(size_t)ia[i];
			JNICall( env->jnienv, ReleaseStringUTFChars, (jobject)&_objid_, astr[i].cs );
		}
	}

	_VT_JAVA_GiveArray( env->jnienv, src, ia );
}

#else
//
jint vt_java_string_array_acquire( vt_java_thread_environ *env, jobject src, vt_cstr **dst, vt_long *ndst )
{
	jobject obj;
	vt_cstr *dst2 = *dst;
	jsize ndst2 = *ndst;
	jsize n = _VT_JAVA_GetArrLen( env->jnienv, src );

	if( n <= 0 )
	{
		*ndst = 0;
		return JNI_OK;
	}

	if( ndst2 > n || ndst2 == 0 )
		ndst2 = n;

	if( dst2 == NULL )
	{
		dst2 = (vt_cstr*)mmp_malloc( 1, &env->mmphd, sizeof(vt_cstr)*ndst2 );
		if( !dst2 )
			return JNI_ERR;
	}

	//JNICall( env->jnienv, ExceptionClear );

	for( n=0; n < ndst2; n++ )
	{
		//dst2[n].len = 0;
		dst2[n].cs = NULL;

		obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, n );
		if( !obj )
			continue;
		//else if( JNICall(env->jnienv, ExceptionCheck) )
		//	break;

		// UTF-8 only
		dst2[n].cs = (vt_char*)JNICall( env->jnienv, GetStringUTFChars, obj, NULL );
		dst2[n].len = JNICall( env->jnienv, GetStringUTFLength, obj );
		_VT_JAVA_DelLocRef( env->jnienv, obj );
		//JNICall( env->jnienv, DeleteLocalRef, obj );
	}

	if( n == ndst2 )
	{
		*dst = dst2;
		*ndst = ndst2;
		return JNI_OK;
	}

	// release resources have been acquired before
	for( --n; n >= 0; n-- )
	{
		obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, n );
		if( obj )
		{
			JNICall( env->jnienv, ReleaseStringUTFChars, obj, dst2[n].cs );
			_VT_JAVA_DelLocRef( env->jnienv, obj );
			//JNICall( env->jnienv, DeleteLocalRef, obj );
		}
	}

	*ndst = 0;

	return JNI_ERR;
}

//
void vt_java_string_array_release( vt_java_thread_environ *env, jobject src, vt_cstr *astr, vt_long nastr )
{
	vt_long i;
	jobject obj;

	for( i=0; i < nastr; i++ )
	{
		if( astr[i].cs && astr[i].len > 0 )
		{
			obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, i );
			if( !obj )
				continue;

			JNICall( env->jnienv, ReleaseStringUTFChars, obj, astr[i].cs );
			_VT_JAVA_DelLocRef( env->jnienv, obj );
			//JNICall( env->jnienv, DeleteLocalRef, obj );
		}
	}
}
#endif


#ifdef _VT_JAVA_JNI_FAST_
//
jint vt_java_tlv_extract( vt_java_thread_environ *env, jobject src, vt_tlv *dst )
{
	jsize n;
	jobject obj, _objid_;
	jint t, r;
	vt_uint *ia = (vt_uint*)_VT_JAVA_TakeArray( env->jnienv, src, VT_JAVA_TLV_NMEMBER );
	if( !ia )
		return JNI_ERR;

	r = JNI_OK;
	obj = (jobject)&_objid_;

#ifdef _VT_JAVA_EXTRACT_LIGHT_
	n = 1;
	dst->tag = 0;
#else
	n = 0;
#endif

	for( ; n < VT_JAVA_TLV_NMEMBER && r == JNI_OK; n++ )
	{
		_objid_ = (jobject)(size_t)ia[n];

		switch( n )
		{
		case 0:  // tag
			r = _java_NumberValue( env, VT_TYPE_INT, obj, (jvalue*)&dst->tag );
			break;
		case 1:  // value
			t = dst->tag & 0xff;

			if( t == 0 )
			{
				// java object reference
				dst->val_obj.id = (size_t)_objid_;
				dst->val_obj.ptr = &dst->val_obj.id;  // val_obj.ptr is equal to val.o
				continue;
			}
			else if( t >= VT_TYPE_SI8 && t <= VT_TYPE_TIME )
			{
				// number type
				dst->len = g_vt_java_type_size[t];
				r = _java_NumberValue( env, t, obj, (jvalue*)&dst->val );
			}
			else if( t == VT_TYPE_CSTRING )
			{
				// UTF-8 only
				dst->val_cs.cs = (vt_char*)JNICall( env->jnienv, GetStringUTFChars, obj, NULL );
				dst->val_cs.len = JNICall( env->jnienv, GetStringUTFLength, obj );
			}
			else if( t == VT_TYPE_CLOCK || t == VT_TYPE_IDENTIFIER )
			{
				// treats as long[]
				jsize ln;
				void *ptr = _java_primitive_array_acquire( env, VT_TYPE_LONG, obj, &ln, NULL );
				r = JNI_ERR;
				if( ptr )
				{
					// copy value
					if( ln == sizeof(dst->val_b16)/sizeof(vt_long) )
					{
						memcpy( dst->val_b16, ptr, sizeof(dst->val_b16) );
						r = JNI_OK;
					}
					_java_primitive_array_release( env, VT_TYPE_LONG, obj, ptr, JNI_ABORT );
				}
			}
			else if( t == VT_TYPE_ARRAY )
			{
				// only primitive type available
				dst->len = 0;
				dst->val.p = _java_primitive_array_acquire( env, dst->type2, obj, (jsize*)&dst->len, NULL );
				r = dst->val.p ? JNI_OK : JNI_ERR;
			}
			else if( t == VT_TYPE_CLASS || t == VT_TYPE_BSTRING )
			{
				// treats as byte[], encoded with specific-format
				dst->len = 0;
				dst->val.p = _java_primitive_array_acquire( env, VT_TYPE_BYTE, obj, (jsize*)&dst->len, NULL );
				r = dst->val.p ? JNI_OK : JNI_ERR;
			}
			break;
		}
	}

	_VT_JAVA_GiveArray( env->jnienv, src, ia );

	return r;
}

void vt_java_tlv_free( vt_java_thread_environ *env, jobject src, vt_tlv *tlv, jint mode )
{
	if( tlv->type1 == 0 )
	{
		tlv->val_obj.ptr = NULL;
		return;
	}

	jobject obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, 1 );
	if( !obj )
		return;

	if( tlv->type1 == VT_TYPE_CSTRING )
	{
		if( tlv->val_cs.cs && tlv->val_cs.len > 0 )
			JNICall( env->jnienv, ReleaseStringUTFChars, obj, tlv->val_cs.cs );
	}
	else if( tlv->type1 == VT_TYPE_ARRAY )
	{
		if( tlv->val.p && tlv->len > 0 )
			_java_primitive_array_release( env, tlv->type2, obj, tlv->val.p, mode );
	}
	else if( tlv->type1 == VT_TYPE_CLASS )
	{
		if( tlv->val.p && tlv->len > 0 )
			_java_primitive_array_release( env, VT_TYPE_BYTE, obj, tlv->val.p, mode );
	}

	_VT_JAVA_DelLocRef( env->jnienv, obj );  // it is not necessary
}

#else
//
_VT_JAVA_INLINE_ VT_LOCAL __unused__
jint vt_java_tlv_extract( vt_java_thread_environ *env, jobject src, vt_tlv *dst )
{
#ifdef _VT_JAVA_EXTRACT_LIGHT_
	dst->val.o = _VT_JAVA_GetObjArrEle( env->jnienv, src, 1 );
	if( dst->val.o )
	{
		dst->len = sizeof( jobject );
		dst->tag = 0;
		return JNI_OK;
	}
	return JNI_ERR;

#else

	jobject obj;
	jint t, r = JNI_OK;
	jsize n;

#if 0
	n = _VT_JAVA_GetArrLen( env->jnienv, src );
	if( n != VT_JAVA_TLV_NMEMBER )
		return JNI_ERR;
#endif

	for( n=0; n < VT_JAVA_TLV_NMEMBER && r == JNI_OK; n++ )
	{
		obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, n );
		if( !obj )
			continue;

		switch( n )
		{
		case 0:  // tag
			r = _java_NumberValue( env, VT_TYPE_INT, obj, (jvalue*)&dst->tag );
			break;
		case 1:  // value
			t = dst->tag & 0xff;
			if( t == 0 )
			{
				// java object reference
				dst->val.o = obj;
				dst->len = dst->val.o ? sizeof( jobject ) : 0;
				continue;
			}
			else if( t >= VT_TYPE_SI8 && t <= VT_TYPE_TIME )
			{
				// number type
				dst->len = g_vt_java_type_size[t];
				r = _java_NumberValue( env, t, obj, (jvalue*)&dst->val );
			}
			else if( t == VT_TYPE_CSTRING )
			{
				// UTF-8 only
				dst->val_cs.cs = (vt_char*)JNICall( env->jnienv, GetStringUTFChars, obj, NULL );
				dst->val_cs.len = JNICall( env->jnienv, GetStringUTFLength, obj );
			}
			else if( t == VT_TYPE_CLOCK || t == VT_TYPE_IDENTIFIER )
			{
				// treats as long[]
				jsize ln;
				void *ptr = _java_primitive_array_acquire( env, VT_TYPE_LONG, obj, &ln, NULL );
				r = JNI_ERR;
				if( ptr )
				{
					// copy value
					if( ln == sizeof(dst->val_b16)/sizeof(vt_long) )
					{
						memcpy( dst->val_b16, ptr, sizeof(dst->val_b16) );
						r = JNI_OK;
					}
					_java_primitive_array_release( env, VT_TYPE_LONG, obj, ptr, JNI_ABORT );
				}
			}
			else if( t == VT_TYPE_ARRAY )
			{
				// only primitive type available
				dst->len = 0;
				dst->val.p = _java_primitive_array_acquire( env, dst->type2, obj, (jsize*)&dst->len, NULL );
				r = dst->val.p ? JNI_OK : JNI_ERR;
			}
			else if( t == VT_TYPE_CLASS || t == VT_TYPE_BSTRING )
			{
				// treats as byte[], encoded with specific-format
				dst->len = 0;
				dst->val.p = _java_primitive_array_acquire( env, VT_TYPE_BYTE, obj, (jsize*)&dst->len, NULL );
				r = dst->val.p ? JNI_OK : JNI_ERR;
			}
			break;
		}

		_VT_JAVA_DelLocRef( env->jnienv, obj );  // it is not necessary
	}

	return r;
#endif
}

//
_VT_JAVA_INLINE_ VT_LOCAL __unused__
void vt_java_tlv_free( vt_java_thread_environ *env, jobject src, vt_tlv *tlv, jint mode )
{
#ifdef _VT_JAVA_EXTRACT_LIGHT_
	if( tlv->val.o )
	{
		_VT_JAVA_DelLocRef( env->jnienv, tlv->val.o );  // it is not necessary
		tlv->val.o = NULL;
	}

#else
	if( tlv->type1 == 0 )
	{
		if( tlv->val.o )
		{
			_VT_JAVA_DelLocRef( env->jnienv, tlv->val.o );  // it is not necessary
			tlv->val.o = NULL;
		}
		return;
	}

	jobject obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, 1 );
	if( !obj )
		return;

	if( tlv->type1 == VT_TYPE_CSTRING )
	{
		if( tlv->val_cs.cs && tlv->val_cs.len > 0 )
			JNICall( env->jnienv, ReleaseStringUTFChars, obj, tlv->val_cs.cs );
	}
	else if( tlv->type1 == VT_TYPE_ARRAY )
	{
		if( tlv->val.p && tlv->len > 0 )
			_java_primitive_array_release( env, tlv->type2, obj, tlv->val.p, mode );
	}
	else if( tlv->type1 == VT_TYPE_CLASS )
	{
		if( tlv->val.p && tlv->len > 0 )
			_java_primitive_array_release( env, VT_TYPE_BYTE, obj, tlv->val.p, mode );
	}

	_VT_JAVA_DelLocRef( env->jnienv, obj );  // it is not necessary
#endif
}
#endif


#ifdef _VT_JAVA_JNI_FAST_
//
jint vt_java_tlv_array_acquire( vt_java_thread_environ *env, jobject src, vt_tlv **dst, vt_long *ndst )
{
	jsize n;
	vt_uint *ia = (vt_uint*)_VT_JAVA_TakeArray2( env->jnienv, src, n );
	if( !ia )
	{
		*ndst = 0;
		return JNI_OK;
	}

	jobject _objid_;
	vt_tlv *dst2 = *dst;
	jsize ndst2 = *ndst;

	if( ndst2 > n || ndst2 == 0 )
		ndst2 = n;

	if( dst2 == NULL )
	{
		dst2 = (vt_tlv*)mmp_malloc( 1, &env->mmphd, sizeof(vt_tlv)*ndst2 );
		if( !dst2 )
		{
			_VT_JAVA_GiveArray( env->jnienv, src, ia );
			return JNI_ERR;
		}
	}

	// must clear at first
	//memset( dst2, 0, sizeof(vt_tlv)*ndst2 );

	for( n=0; n < ndst2; n++ )
	{
		dst2[n].val.o = NULL;
		_objid_= (jobject)(size_t)ia[n];
		if( vt_java_tlv_extract( env, (jobject)&_objid_, &dst2[n] ) != JNI_OK )
			break;
	}

	if( n == ndst2 )
	{
		*dst = dst2;
		*ndst = ndst2;
		_VT_JAVA_GiveArray( env->jnienv, src, ia );
		return JNI_OK;
	}

	// release all resources have been acquired before
	for( ; n >= 0; n-- )
	{
		_objid_= (jobject)(size_t)ia[n];
		vt_java_tlv_free( env, (jobject)&_objid_, &dst2[n], JNI_ABORT );
	}

	*ndst = 0;
	_VT_JAVA_GiveArray( env->jnienv, src, ia );

	return JNI_ERR;
}

//
void vt_java_tlv_array_release( vt_java_thread_environ *env, jobject src, vt_tlv *tlvs, vt_long ntlv, jint mode )
{
	jobject _objid_;
	vt_long i, _n;
	vt_uint *ia = (vt_uint*)_VT_JAVA_TakeArray2( env->jnienv, src, _n );
	if( !ia )
		return;

	if( _n < ntlv )
		ntlv = _n;

	for( i=0; i < ntlv; i++ )
	{
		_objid_ = (jobject)(size_t)ia[i];
		vt_java_tlv_free( env, (jobject)&_objid_, &tlvs[i], mode );
	}

	_VT_JAVA_GiveArray( env->jnienv, src, ia );
}

#else
//
_VT_JAVA_INLINE_ VT_LOCAL
jint vt_java_tlv_array_acquire( vt_java_thread_environ *env, jobject src, vt_tlv **dst, vt_long *ndst )
{
#ifndef _VT_JAVA_PARAMETER_LIGHT_
	jobject obj;
#endif

	vt_tlv *dst2 = *dst;
	jsize ndst2 = *ndst;

#ifdef _VT_JAVA_EXTRACT_LIGHT_
	jsize n;
	if( ndst2 == 0 )
	{
		n = _VT_JAVA_GetArrLen( env->jnienv, src );
		if( n <= 0 )
		{
			*ndst = 0;
			return JNI_OK;
		}
		ndst2 = n;
	}
#else
	jsize n = _VT_JAVA_GetArrLen( env->jnienv, src );
	if( n <= 0 )
	{
		*ndst = 0;
		return JNI_OK;
	}

	if( ndst2 > n || ndst2 == 0 )
		ndst2 = n;
#endif

	if( dst2 == NULL )
	{
		dst2 = (vt_tlv*)mmp_malloc( 1, &env->mmphd, sizeof(vt_tlv)*ndst2 );
		if( !dst2 )
			return JNI_ERR;
	}

#if 0// FOR DEBUG
	vt_uint ia[32] = {0};
	JNICall( env->jnienv, GetIntArrayRegion, src, 0, ndst2, (jint*)ia );
	for( n=0; n < ndst2; n++ )
	{
		dst2[n].tag = 0;
		dst2[n].len = sizeof( jobject );
		dst2[n].val.o = (vt_object)(size_t)ia[n];
	}
#endif

	//jsize _n = 0;

	for( n=0; n < ndst2; n++ )
	{
#ifdef _VT_JAVA_PARAMETER_LIGHT_
		if( (dst2[n].val.o = _VT_JAVA_GetObjArrEle( env->jnienv, src, n )) )
		{
			dst2[n].tag = 0;
			dst2[n].len = sizeof( jobject );
		}
		//else
		//	_n++;
#else
		obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, n );
		if( !obj )
		{
			_n++;
			dst2[n].val.o = NULL;
			continue;
		}

		if( vt_java_tlv_extract( env, obj, &dst2[n] ) != JNI_OK )
			break;
		_VT_JAVA_DelLocRef( env->jnienv, obj );
#endif
	}

	if( n == ndst2 ) //&& n != _n )
	{
		*dst = dst2;
		*ndst = ndst2;
		return JNI_OK;
	}

#ifndef _VT_JAVA_PARAMETER_LIGHT_
	//if( _n != n )
	{
		// release all resources have been acquired before
		for( --n; n >= 0; n-- )
		{
			obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, n );
			if( obj )
			{
				vt_java_tlv_free( env, obj, &dst2[n], JNI_ABORT );
				_VT_JAVA_DelLocRef( env->jnienv, obj );
			}
		}
	}
#endif

	*ndst = 0;
	return JNI_ERR;
}

//
_VT_JAVA_INLINE_ VT_LOCAL
void vt_java_tlv_array_release( vt_java_thread_environ *env, jobject src, vt_tlv *tlvs, vt_long ntlv, jint mode )
{
#ifdef _VT_JAVA_EXTRACT_LIGHT_
#ifdef _VT_JAVA_DELREF_EVERY_TIME_
	vt_long i;
	for( i=0; i < ntlv; i++ )
	{
		if( tlvs[i].val.o )
		{
			_VT_JAVA_DelLocRef( env->jnienv, tlvs[i].val.o );  // it is not necessary
			tlvs[i].val.o = NULL;
		}
	}
#else
	memset( tlvs, 0, sizeof(vt_tlv)*ntlv );
#endif

#else
	vt_long i;
	jobject obj;

	for( i=0; i < ntlv; i++ )
	{
		obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, i );
		if( !obj )
			continue;
		vt_java_tlv_free( env, obj, &tlvs[i], mode );
		_VT_JAVA_DelLocRef( env->jnienv, obj );
	}
#endif
}
#endif


#ifdef _VT_JAVA_JNI_FAST_
//
jint vt_java_data_sequence_extract( vt_java_thread_environ *env, jobject src, vt_data_sequence *dst )
{
	jsize n;
	jobject obj, _objid_;
	jint r;
	vt_uint *ia = (vt_uint*)_VT_JAVA_TakeArray( env->jnienv, src, VT_JAVA_DATA_SEQUENCE_NMEMBER );
	if( !ia )
		return JNI_ERR;

#ifdef _VT_JAVA_EXTRACT_LIGHT_
	//dst->ndat = 0;
	n = 2;
#else
	n = 0;
#endif

	r = JNI_OK;
	obj = (jobject)&_objid_;

	for( ; n < VT_JAVA_DATA_SEQUENCE_NMEMBER && r == JNI_OK; n++ )
	{
		_objid_ = (jobject)(size_t)ia[n];
		switch( n )
		{
		case 0:  // ndat
			r = _java_NumberValue( env, VT_TYPE_LONG, obj, (jvalue*)&dst->ndat );
			if( r == JNI_OK && dst->ndat <= 0 )
			{
				memset( dst, 0, sizeof(vt_data_sequence) );
				return JNI_OK;
			}

#ifdef _VT_JAVA_EXTRACT_LIGHT_
			n++;  // skip the step-1
#endif
			break;
		case 1:  // uids
			r = dst->ndat * 2; // one ID contains two long items
			dst->uids = (vt_ident*)_java_primitive_array_acquire( env, VT_TYPE_LONG, obj, &r, NULL );

			if( dst->uids && r > 0 )
			{
				dst->ndat = r / 2;
				r = JNI_OK;
			}
			else
			{
				dst->uids = NULL;
				r = -(r != 0);
			}
			break;
		case 2:  // datas
			r = vt_java_tlv_array_acquire( env, obj, &dst->datas, &dst->ndat );

#ifdef _VT_JAVA_EXTRACT_LIGHT_
			n++;  // skip the step-3
#endif
			break;
		case 3: { // names
			vt_long nms = dst->ndat;
			r = vt_java_string_array_acquire( env, obj, &dst->names, &nms );
			if( r == JNI_OK && nms > 0 && nms < dst->ndat )
			{
				_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "names is not match to datas(%s, %d)\n", __FILE__, __LINE__ );
				dst->ndat = nms;
			}
			break;
			}
		}
	}

	_VT_JAVA_GiveArray( env->jnienv, src, ia );

	return r;
}

//
void vt_java_data_sequence_free( vt_java_thread_environ *env, jobject src, vt_data_sequence *datseq, jint mode )
{
}

#else

//
_VT_JAVA_INLINE_ VT_LOCAL
jint vt_java_data_sequence_extract( vt_java_thread_environ *env, jobject src, vt_data_sequence *dst )
{
	jobject obj;
	jint r;

#if 1
	obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, 2 );
	if( obj )
	{
		r = vt_java_tlv_array_acquire( env, obj, &dst->datas, &dst->ndat );
		_VT_JAVA_DelLocRef( env->jnienv, obj );  // it is not necessary
		return r;
	}
	return JNI_ERR;

#else

	r = JNI_OK;

#ifdef _VT_JAVA_EXTRACT_LIGHT_
	jsize n;
#else
	jsize n = _VT_JAVA_GetArrLen( env->jnienv, src );
	if( n != VT_JAVA_DATA_SEQUENCE_NMEMBER )
		return JNI_ERR;
#endif


#ifdef _VT_JAVA_EXTRACT_LIGHT_
	n = 2;
	do
#else
	n = 0;
	for( ; n < VT_JAVA_DATA_SEQUENCE_NMEMBER && r == JNI_OK; n++ )
#endif
	{
		obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, n );
		if( !obj )
			continue;

		switch( n )
		{
		case 0:  // ndat
			r = _java_NumberValue( env, VT_TYPE_LONG, obj, (jvalue*)&dst->ndat );
			if( r == JNI_OK && dst->ndat <= 0 )
			{
				memset( dst, 0, sizeof(vt_data_sequence) );
				return JNI_OK;
			}

#ifdef _VT_JAVA_EXTRACT_LIGHT_
			n++;  // skip the step-1
#endif
			break;
		case 1:  // uids
			r = dst->ndat * 2; // one ID contains two long items
			dst->uids = (vt_ident*)_java_primitive_array_acquire( env, VT_TYPE_LONG, obj, &r, NULL );

			if( dst->uids && r > 0 )
			{
				dst->ndat = r / 2;
				r = JNI_OK;
			}
			else
			{
				dst->uids = NULL;
				r = -(r != 0);
			}
			break;
		case 2:  // datas
			r = vt_java_tlv_array_acquire( env, obj, &dst->datas, &dst->ndat );

#ifdef _VT_JAVA_EXTRACT_LIGHT_
			n++;  // skip the step-3
#endif
			break;
		case 3: { // names
			vt_long nms = dst->ndat;
			r = vt_java_string_array_acquire( env, obj, &dst->names, &nms );
			if( r == JNI_OK && nms > 0 && nms < dst->ndat )
			{
				_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "names is not match to datas(%s, %d)\n", __FILE__, __LINE__ );
				dst->ndat = nms;
			}
			break;
			}
		}

		_VT_JAVA_DelLocRef( env->jnienv, obj );  // it is not necessary
	}
#ifdef _VT_JAVA_EXTRACT_LIGHT_
	while(0);
#endif

	return r;
#endif
}

//
_VT_JAVA_INLINE_ VT_LOCAL
void vt_java_data_sequence_free( vt_java_thread_environ *env, jobject src, vt_data_sequence *datseq, jint mode )
{
#if 1
	if( datseq->datas && datseq->ndat > 0 )
	{
		jobject obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, 2 );
		if( obj )
		{
			vt_java_tlv_array_release( env, obj, datseq->datas, datseq->ndat, mode );
			_VT_JAVA_DelLocRef( env->jnienv, obj );  // it is not necessary
		}
	}

#else

	jobject obj;
	jsize n;

#ifdef _VT_JAVA_EXTRACT_LIGHT_
	n = 2;  // skip the step-1
	do
#else
	n = 1;
	for( ; n < VT_JAVA_DATA_SEQUENCE_NMEMBER; n++ )
#endif
	{
		switch( n )
		{
		case 1:  // uids
			if( datseq->uids )
			{
				obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, n );
				if( obj )
				{
					JNICall( env->jnienv, ReleaseLongArrayElements, obj, (jlong*)datseq->uids, mode );
					_VT_JAVA_DelLocRef( env->jnienv, obj );  // it is not necessary
				}
			}
			break;
		case 2:  // datas
			if( datseq->datas && datseq->ndat > 0 )
			{
				obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, n );
				if( obj )
				{
					vt_java_tlv_array_release( env, obj, datseq->datas, datseq->ndat, mode );
					_VT_JAVA_DelLocRef( env->jnienv, obj );  // it is not necessary
				}
			}
#ifdef _VT_JAVA_EXTRACT_LIGHT_
			n++;  // skip the step-3
#endif
			break;
		case 3:  // names
			if( datseq->names )
			{
				obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, n );
				if( obj )
				{
					vt_java_string_array_release( env, obj, datseq->names, datseq->ndat );
					_VT_JAVA_DelLocRef( env->jnienv, obj );  // it is not necessary
				}
			}
			break;
		}
	}
#ifdef _VT_JAVA_EXTRACT_LIGHT_
	while(0);
#endif

#endif
}
#endif

//
jint vt_java_event_entity_extract( vt_java_thread_environ *env, jobject src, vt_event_entity *dst, vt_bool chk_evt )
{
	jobject obj;
	jint r = JNI_OK;

#if 1
	obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, 0 );
	if( obj )
	{
		r = _java_NumberValue( env, VT_TYPE_INT, obj, (jvalue*)&dst->type );
		if( chk_evt && r == JNI_OK )
		{
			// check event type
			if( vt_check_event( dst->type, &dst->_idx_ ) < 0 )
			{
				// discard current event
				r = -100;
			}
		}
		_VT_JAVA_DelLocRef( env->jnienv, obj );  // it is not necessary
	}
	return r;

#else

	//r = JNI_OK;

#ifdef _VT_JAVA_EXTRACT_LIGHT_
	jsize n = 0;
	do
#else
	jsize n = _VT_JAVA_GetArrLen( env->jnienv, src );
	if( n != VT_JAVA_EVENT_ENTITY_NMEMBER )
		return JNI_ERR;
	for( n=0; n < VT_JAVA_EVENT_ENTITY_NMEMBER && r==JNI_OK; n++ )
#endif
	{
		obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, n );
		if( !obj )
			break;

		switch( n )
		{
		case 0: // type
			r = _java_NumberValue( env, VT_TYPE_INT, obj, (jvalue*)&dst->type );
			if( chk_evt && r == JNI_OK )
			{
				// check event type
				if( vt_check_event( dst->type, &dst->_idx_ ) < 0 )
				{
					// discard current event
					r = -100;
					break;
				}
			}

#ifdef _VT_JAVA_EXTRACT_LIGHT_
			n = 8; // skip all following the type
#else
			n++;  // skip the "opt"
#endif
			break;
		case 1: // opt
			//r = _java_NumberValue( env, VT_TYPE_INT, obj, (jvalue*)&dst->opt );
			break;
		case 2: // ident_l
			r = _java_NumberValue( env, VT_TYPE_LONG, obj, (jvalue*)&dst->ident.low );
			break;
		case 3: // ident_h
			r = _java_NumberValue( env, VT_TYPE_LONG, obj, (jvalue*)&dst->ident.high );
#ifdef _VT_JAVA_EXTRACT_LIGHT_
			n = 7;  // skip the "from" and the "dest"
#endif
			break;
		case 4: // from_l
			r = _java_NumberValue( env, VT_TYPE_LONG, obj, (jvalue*)&dst->from.low );
			break;
		case 5: // from_h
			r = _java_NumberValue( env, VT_TYPE_LONG, obj, (jvalue*)&dst->from.high );
			break;
		case 6: // dest_l
			r = _java_NumberValue( env, VT_TYPE_LONG, obj, (jvalue*)&dst->dest.low );
			break;
		case 7: // dest_h
			r = _java_NumberValue( env, VT_TYPE_LONG, obj, (jvalue*)&dst->dest.high );
			break;
		case 8: // reqdat
			r = vt_java_data_sequence_extract( env, obj, &dst->req.datseq );
			break;
		}

		_VT_JAVA_DelLocRef( env->jnienv, obj );  // it is not necessary
	}
#ifdef _VT_JAVA_EXTRACT_LIGHT_
	while(0);
#endif

	return r;
#endif
}

//
void vt_java_event_entity_free( vt_java_thread_environ *env, jobject src, vt_event_entity *entity, jint mode )
{
#ifdef _VT_JAVA_EXTRACT_LIGHT_
	return;
#endif

	if( entity->req.datseq.ndat > 0 )
	{
		jobject obj;
		obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, 8 );
		if( obj )
		{
			vt_java_data_sequence_free( env, obj, &entity->req.datseq, mode );
			_VT_JAVA_DelLocRef( env->jnienv, obj );
		}
	}
}


//
jint vt_java_callframe_extract( vt_java_thread_environ *env, jobject src, vt_call_frame *dst )
{
	jobject obj;
	jint r = JNI_OK;
	jsize n = _VT_JAVA_GetArrLen( env->jnienv, src );

	if( n != VT_JAVA_CALL_FRAME_NMEMBER )
		return JNI_ERR;

	//JNICall( env->jnienv, ExceptionClear );

	for( n=0; n < VT_JAVA_CALL_FRAME_NMEMBER && r==JNI_OK; n++ )
	{
		obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, n );
		if( !obj )
			continue;
		//else if( JNICall(env->jnienv, ExceptionCheck) )
		//	break;

		switch( n )
		{
		case 0: // full_path
			dst->full_path = (vt_char*)JNICall( env->jnienv, GetStringUTFChars, obj, NULL );
			dst->path_len = JNICall( env->jnienv, GetStringUTFLength, obj );
			break;
		case 1: // name_off
			r = _java_NumberValue( env, VT_TYPE_INT, obj, (jvalue*)&dst->name_off );
			break;
		case 2: // decl_off
			r = _java_NumberValue( env, VT_TYPE_INT, obj, (jvalue*)&dst->decl_off );
			break;
		case 3: // pos
			r = _java_NumberValue( env, VT_TYPE_LONG, obj, (jvalue*)&dst->pos );
			break;
		}

		_VT_JAVA_DelLocRef( env->jnienv, obj );  // it is not necessary
	}

	return r;
}


//
void vt_java_callframe_free( vt_java_thread_environ *env, jobject src, vt_call_frame *frm, jint mode )
{
	if( !frm || !frm->full_path )
		return;

	jobject obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, 0 );
	if( obj )
	{
		JNICall( env->jnienv, ReleaseStringUTFChars, obj, frm->full_path );
		_VT_JAVA_DelLocRef( env->jnienv, obj );
	}
}


//
jint vt_java_callframe_array_acquire( vt_java_thread_environ *env, jobject src, vt_call_frame **frms, vt_int *nfrm )
{
	jobject obj;
	vt_call_frame *frms2 = *frms;
	jsize nfrm2 = *nfrm;
	jsize n = _VT_JAVA_GetArrLen( env->jnienv, src );

	if( n <= 0 )
	{
		*nfrm = 0;
		return JNI_OK;
	}

	if( nfrm2 > n || nfrm2 == 0 )
		nfrm2 = n;

	if( frms2 == NULL )
	{
		frms2 = (vt_call_frame*)mmp_malloc( 1, &env->mmphd, sizeof(vt_call_frame)*nfrm2 );
		if( !frms2 )
			return JNI_ERR;
	}

	// must clear at first
	memset( frms2, 0, sizeof(vt_call_frame)*nfrm2 );

	//JNICall( env->jnienv, ExceptionClear );

	for( n=0; n < nfrm2; n++ )
	{
		obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, n );
		if( !obj )
			continue;
		//else if( JNICall(env->jnienv, ExceptionCheck) )
		//	break;

		if( vt_java_callframe_extract( env, obj, &frms2[n] ) != JNI_OK )
			break;

		_VT_JAVA_DelLocRef( env->jnienv, obj );
	}

	if( n == nfrm2 )
	{
		*frms = frms2;
		*nfrm = nfrm2;
		return JNI_OK;
	}

	// release all resources have been acquired before
	for( ; n >= 0; n-- )
	{
		obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, n );
		if( obj )
		{
			vt_java_callframe_free( env, obj, &frms2[n], JNI_ABORT );
			_VT_JAVA_DelLocRef( env->jnienv, obj );
		}
	}

	return JNI_ERR;
}


//
void vt_java_callframe_array_release( vt_java_thread_environ *env, jobject src, vt_call_frame *frms, vt_int nfrm )
{
	if( !frms || nfrm < 1 )
		return;

	vt_int i;
	jobject obj;

	//JNICall( env->jnienv, ExceptionClear );

	for( i=0; i < nfrm; i++ )
	{
		obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, i );
		if( !obj )
			continue;
		//else if( JNICall(env->jnienv, ExceptionCheck) )
		//	break;

		vt_java_callframe_free( env, obj, &frms[i], JNI_ABORT );
		_VT_JAVA_DelLocRef( env->jnienv, obj );
	}
}


//
jint vt_java_callstack_extract( vt_java_thread_environ *env, jobject src, vt_data_callstack *dst )
{
	jobject obj;
	jint r = JNI_OK;
	jsize n = _VT_JAVA_GetArrLen( env->jnienv, src );

	if( n != VT_JAVA_DATA_CALLSTACK_NMEMBER )
		return JNI_ERR;

	//JNICall( env->jnienv, ExceptionClear );

	for( n=0; n < VT_JAVA_DATA_CALLSTACK_NMEMBER && r==JNI_OK; n++ )
	{
		obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, n );
		if( !obj )
			continue;
		//else if( JNICall(env->jnienv, ExceptionCheck) )
		//	break;

		switch( n )
		{
		case 0: // thread
			dst->thread = obj; // ???
			break;
		case 1: // nframe
			r = _java_NumberValue( env, VT_TYPE_INT, obj, (jvalue*)&dst->num );
			if( r == JNI_OK && dst->num <= 0 )
			{
				dst->num = 0;
				dst->frame = NULL;
				return JNI_OK;
			}
			break;
		case 2: // frames
			r = vt_java_callframe_array_acquire( env, obj, &dst->frame, &dst->num );
			break;
		}

		_VT_JAVA_DelLocRef( env->jnienv, obj );  // it is not necessary
	}

	return r;
}


//
void vt_java_callstack_free( vt_java_thread_environ *env, jobject src, vt_data_callstack *cstk )
{
	if( !cstk->frame || cstk->num < 1 )
		return;

	jobject obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, 2 );
	if( obj )
	{
		vt_java_callframe_array_release( env, obj, cstk->frame, cstk->num );
		_VT_JAVA_DelLocRef( env->jnienv, obj );
	}
}


//
jint vt_java_exception_extract( vt_java_thread_environ *env, jobject src, vt_data_exception *dst )
{
	jobject obj;
	jint r = JNI_OK;
	jsize n = _VT_JAVA_GetArrLen( env->jnienv, src );

	if( n != VT_JAVA_DATA_EXCEPTION_NMEMBER )
		return JNI_ERR;

	//JNICall( env->jnienv, ExceptionClear );

	for( n=0; n < VT_JAVA_DATA_EXCEPTION_NMEMBER && r==JNI_OK; n++ )
	{
		obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, n );
		if( !obj )
			continue;
		//else if( JNICall(env->jnienv, ExceptionCheck) )
		//	break;

		switch( n )
		{
		case 0: // type
			r = _java_NumberValue( env, VT_TYPE_INT, obj, (jvalue*)&dst->type );
			break;
		case 1: // message
			dst->msgstr = (vt_char*)JNICall( env->jnienv, GetStringUTFChars, obj, NULL );
			dst->msglen = (vt_uint)JNICall( env->jnienv, GetStringUTFLength, obj );
			break;
		case 2: // backtrace
			r = vt_java_callstack_extract( env, obj, &dst->backtrace );
			break;
		}

		_VT_JAVA_DelLocRef( env->jnienv, obj );  // it is not necessary
	}

	return r;
}


//
void vt_java_exception_free( vt_java_thread_environ *env, jobject src, vt_data_exception *exp )
{
	jobject obj;

	if( exp->msgstr && exp->msglen > 0 )
	{
		obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, 1 );
		if( obj )
		{
			JNICall( env->jnienv, ReleaseStringUTFChars, obj, exp->msgstr );
			_VT_JAVA_DelLocRef( env->jnienv, obj );
		}
	}

	//
	obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, 2 );
	if( obj )
	{
		vt_java_callstack_free( env, obj, &exp->backtrace );
		_VT_JAVA_DelLocRef( env->jnienv, obj );
	}
}


//
jint vt_java_journal_extract( vt_java_thread_environ *env, jobject src, vt_data_journal *dst )
{
	jobject obj;
	jint r = JNI_OK;
	jsize n = _VT_JAVA_GetArrLen( env->jnienv, src );

	if( n != VT_JAVA_DATA_JOURNAL_NMEMBER )
		return JNI_ERR;

	//JNICall( env->jnienv, ExceptionClear );

	for( n=0; n < VT_JAVA_DATA_JOURNAL_NMEMBER && r==JNI_OK; n++ )
	{
		obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, n );
		if( !obj )
			continue;
		//else if( JNICall(env->jnienv, ExceptionCheck) )
		//	break;

		switch( n )
		{
		case 0: // type
			r = _java_NumberValue( env, VT_TYPE_INT, obj, (jvalue*)&dst->type );
			break;
		case 1: {// source
			dst->source = (vt_char*)JNICall( env->jnienv, GetStringUTFChars, obj, NULL );
			jsize ln = JNICall( env->jnienv, GetStringUTFLength, obj );
			dst->srclen = ln >= 4096 ? 4096 : ln;
			break;
		}
		case 2: // message
			dst->message = (vt_char*)JNICall( env->jnienv, GetStringUTFChars, obj, NULL );
			dst->msglen = (vt_uint)JNICall( env->jnienv, GetStringUTFLength, obj );
			break;
		}

		_VT_JAVA_DelLocRef( env->jnienv, obj );  // it is not necessary
	}

	return r;
}


//
void vt_java_journal_free( vt_java_thread_environ *env, jobject src, vt_data_journal *log )
{
	jobject obj;

	if( log->source && log->srclen > 0 )
	{
		obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, 1 );
		if( obj )
		{
			JNICall( env->jnienv, ReleaseStringUTFChars, obj, log->source );
			_VT_JAVA_DelLocRef( env->jnienv, obj );
		}
	}

	if( log->message && log->msglen > 0 )
	{
		obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, 2 );
		if( obj )
		{
			JNICall( env->jnienv, ReleaseStringUTFChars, obj, log->message );
			_VT_JAVA_DelLocRef( env->jnienv, obj );
		}
	}
}


#ifdef _VT_JAVA_JNI_FAST_
//
jint vt_java_sot_entity_extract( vt_java_thread_environ *env, jobject src, vt_sot_entity *dst, vt_bool isException )
{
	jsize n;
	jobject obj, _objid_;
	jint r;
	vt_uint *ia = (vt_uint*)_VT_JAVA_TakeArray( env->jnienv, src, VT_JAVA_SOT_ENTITY_NMEMBER );
	if( !ia )
		return JNI_ERR;

	r = JNI_OK;
	obj = (jobject)&_objid_;

	for( n=0; n < VT_JAVA_SOT_ENTITY_NMEMBER && r==JNI_OK; n++ )
	{
		_objid_ = (jobject)(size_t)ia[n];
		switch( n )
		{
		case 0: // args
			if( isException )
			{
				r = vt_java_exception_extract( env, obj, &dst->expt );
				n = 2; // going to jumping to step-3
			}
			else
				r = vt_java_data_sequence_extract( env, obj, &dst->invk.args.datseq );
			break;
		case 1: // cstk
			r = vt_java_callstack_extract( env, obj, &dst->cstk );
			break;
		case 2: // watch
			r = vt_java_data_sequence_extract( env, obj, &dst->watch.args.datseq );
			break;
		case 3: // journal
			r = vt_java_journal_extract( env, obj, &dst->log );
			break;
		}

#ifdef _VT_JAVA_EXTRACT_LIGHT_
		break;  // only extract "args"
#endif
	}

	_VT_JAVA_GiveArray( env->jnienv, src, ia );

	return r;
}

//
void vt_java_sot_entity_free( vt_java_thread_environ *env, jobject src, vt_sot_entity *entity, jint mode, vt_bool isException )
{
	jsize n;
	jobject obj, _objid_;
	vt_uint *ia = (vt_uint*)_VT_JAVA_TakeArray( env->jnienv, src, VT_JAVA_SOT_ENTITY_NMEMBER );
	if( !ia )
		return;

	obj = (jobject)&_objid_;
	for( n=0; n < VT_JAVA_SOT_ENTITY_NMEMBER; n++ )
	{
		_objid_ = (jobject)(size_t)ia[n];

		switch( n )
		{
		case 0:  // args
			if( isException )
			{
				vt_java_exception_free( env, obj, &entity->expt );
				n = 2; // going to jumping to step-3
			}
			else if( entity->invk.args.datseq.ndat > 0 )
				vt_java_data_sequence_free( env, obj, &entity->invk.args.datseq, mode );
			break;
		case 1:  // cstk
			vt_java_callstack_free( env, obj, &entity->cstk );
			break;
		case 2:  // watch
			if( entity->watch.args.datseq.ndat > 0 )
				vt_java_data_sequence_free( env, obj, &entity->watch.args.datseq, mode );
			break;
		case 3:  // journal
			vt_java_journal_free( env, obj, &entity->log );
			break;
		}

#ifdef _VT_JAVA_EXTRACT_LIGHT_
		break;  // only free "args"
#endif
	}

	_VT_JAVA_GiveArray( env->jnienv, src, ia );
}


#else
//
jint vt_java_sot_entity_extract( vt_java_thread_environ *env, jobject src, vt_sot_entity *dst, vt_bool isException )
{
#ifdef _VT_JAVA_EXTRACT_LIGHT_
	jint r;
	jobject obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, 0 );
	if( obj )
	{
		r = vt_java_data_sequence_extract( env, obj, &dst->invk.args.datseq );
		_VT_JAVA_DelLocRef( env->jnienv, obj );  // it is not necessary
		return r;
	}
	return JNI_ERR;

#else

	jobject obj;
	jint r = JNI_OK;
	jsize n;

#if 1
	n = _VT_JAVA_GetArrLen( env->jnienv, src );
	if( n != VT_JAVA_SOT_ENTITY_NMEMBER )
		return JNI_ERR;
#endif

	for( n=0; n < VT_JAVA_SOT_ENTITY_NMEMBER && r==JNI_OK; n++ )
	{
		obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, n );
		if( !obj )
			continue;

		switch( n )
		{
		case 0: // args
			if( isException )
			{
				r = vt_java_exception_extract( env, obj, &dst->expt );
				n = 2; // going to jumping to step-3
			}
			else
				r = vt_java_data_sequence_extract( env, obj, &dst->invk.args.datseq );
			break;
		case 1: // cstk
			r = vt_java_callstack_extract( env, obj, &dst->cstk );
			break;
		case 2: // watch
			r = vt_java_data_sequence_extract( env, obj, &dst->watch.args.datseq );
			break;
		case 3: // journal
			r = vt_java_journal_extract( env, obj, &dst->log );
			break;
		}

		_VT_JAVA_DelLocRef( env->jnienv, obj );  // it is not necessary
	}

	return r;
#endif
}

//
void vt_java_sot_entity_free( vt_java_thread_environ *env, jobject src, vt_sot_entity *entity, jint mode, vt_bool isException )
{
#ifdef _VT_JAVA_EXTRACT_LIGHT_
	if( entity->invk.args.datseq.ndat > 0 )
	{
		jobject obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, 0 );
		if( obj )
		{
			vt_java_data_sequence_free( env, obj, &entity->invk.args.datseq, mode );
			_VT_JAVA_DelLocRef( env->jnienv, obj );  // it is not necessary
		}
	}

#else

	jobject obj;
	jsize n;

	for( n=0; n < VT_JAVA_SOT_ENTITY_NMEMBER; n++ )
	{
		switch( n )
		{
		case 0:  // args
			if( isException )
			{
				obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, n );
				if( obj )
				{
					vt_java_exception_free( env, obj, &entity->expt );
					_VT_JAVA_DelLocRef( env->jnienv, obj );  // it is not necessary
				}
				n = 2; // going to jumping to step-3
			}
			else if( entity->invk.args.datseq.ndat > 0 )
			{
				obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, n );
				if( obj )
				{
					vt_java_data_sequence_free( env, obj, &entity->invk.args.datseq, mode );
					_VT_JAVA_DelLocRef( env->jnienv, obj );  // it is not necessary
				}
			}
			break;
		case 1:  // cstk
			obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, n );
			if( obj )
			{
				vt_java_callstack_free( env, obj, &entity->cstk );
				_VT_JAVA_DelLocRef( env->jnienv, obj );  // it is not necessary
			}
			break;
		case 2:  // watch
			if( entity->watch.args.datseq.ndat > 0 )
			{
				obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, n );
				if( obj )
				{
					vt_java_data_sequence_free( env, obj, &entity->watch.args.datseq, mode );
					_VT_JAVA_DelLocRef( env->jnienv, obj );  // it is not necessary
				}
			}
			break;
		case 3:  // journal
			obj = _VT_JAVA_GetObjArrEle( env->jnienv, src, n );
			if( obj )
			{
				vt_java_journal_free( env, obj, &entity->log );
				_VT_JAVA_DelLocRef( env->jnienv, obj );  // it is not necessary
			}
			break;
		}
	}
#endif
}
#endif


// Function:
// Parameter:
//   P; init_thread:= initial thread object; method will treats the current thread as the initiator if it is NULL
//   O; mode:= see "trace mode"
//   O; capacity:= array of long which represents the vt_java_capacity
//   N; entity: array of object which represents the vt_java_event_entity
// Return:
//   [0]:= event reference
//   [1]:= initial action reference
//   ,otherwise empty array(length=0) will be returned.
jobject JNICALL java_event_create( JNIENV_PTR jni, jobject jcls,
								   jlong   tpidx,
								   jint    mode,
								   jobject capacity,
								   jobject entity
								   )
{
#if (_VT_JAVA_DEBUG_LEVEL_ == 0)
	return NULL;
#endif

	if( !g_vt_java_jvm ||
		!entity ||
		!(mode & 0x1f)
		)
	{
#ifdef VT_JAVA_DEBUG
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_WARN, "%s, wrong parameters, %p, %p, %x, line.%d\n",
				__FUNCTION__, g_vt_java_jvm, entity, mode, __LINE__ );
#endif
		return NULL;
	}

	// get environment of current thread
	vt_java_thread_environ *env = _java_getenv( jni );
	if( !env || !env->jtienv )
		return NULL;

	env->jnienv = jni;

#ifndef _RECURSION_DISABLE_
	long _mmphd_ = env->mmphd;
	env->mmphd = 0;
#endif

	// initiate mempool at first
	mmp_create_mempool( 1, &env->mmphd );

	//jboolean bl;
	int r;
	vt_event_entity entity2 = {0};
	jobject init_thread = NULL;

#ifndef _VT_JAVA_EXTRACT_LIGHT_
	vt_ident *uids_new, *uids_old;
	struct vt_java_capacity *caps = NULL;
#endif

	//
	jobject ret_obj = JNICall( env->jnienv, NewLongArray, 2 );
	if( !ret_obj )
		return NULL;

	//
	jlong *ret_val = (jlong*)JNICall( env->jnienv, GetLongArrayElements, ret_obj, NULL );
	if( !ret_val )
	{
		_VT_JAVA_DelLocRef( env->jnienv, ret_obj );
		return NULL;
	}

#ifndef _VT_JAVA_EXTRACT_LIGHT_
	//
	JNICall( env->jtienv, GetCurrentThread, &init_thread );

	//
	if( capacity )
	{
		r = _VT_JAVA_GetArrLen( env->jnienv, capacity );
		if( r == VT_JAVA_CAPACITY_NMEMBER )
			caps = (struct vt_java_capacity*)JNICall( env->jnienv, GetLongArrayElements, capacity, NULL );
	}
#endif

	env->evtref = 0;
	env->actref = 0;

#ifdef VT_JAVA_DEBUG
	r = -100;
	entity2.type = 1;
	if( vt_check_event( 1, &entity2._idx_ ) >= 0 )
#else
	r = vt_java_event_entity_extract( env, entity, &entity2, TRUE ); // extract resources from the object "entity"
	if( r == JNI_OK )
#endif
	{
#if 0 //def VT_JAVA_DEBUG
		vt_print_event( stdout, init_thread, mode, (vt_capa*)caps, &entity2 );
#endif

		r = 0;

#ifndef _VT_JAVA_EXTRACT_LIGHT_
		uids_old = entity2.req.datseq.uids;

		if( entity2.req.datseq.ndat > 0 )
		{
			// data object mapping
			uids_new = (vt_ident*)mmp_malloc( 1, &env->mmphd, sizeof(vt_ident) * entity2.req.datseq.ndat );
			if( uids_new )
			{
				vt_java_data_sequence_mapping( env, &entity2.req.datseq, uids_new );
				entity2.req.datseq.uids = uids_new;
			}
			else
				r = -1;
		}

		if( r == 0 )
#endif
		{
			ret_val[1] = 0;
			ret_val[0] = (jlong)vt_event_create(
											#ifndef _VT_JAVA_EXTRACT_LIGHT_
												thd, mode, (caps && (caps->flags&VTRACE_CAPA_FLAG_ENABLE) ? (vt_capa*)caps : NULL),
											#else
												init_thread, mode, NULL,
											#endif
												 &entity2, (vt_refer*)&ret_val[1], NULL );  //(vt_refer)env );
			if( ret_val[0] != 0 )
			{
#ifndef _VT_JAVA_EXTRACT_LIGHT_
				if( entity2.req.datseq.ndat > 0 )
				{
					// Re-Binding if possible
					vt_java_data_sequence_binding( env, &entity2.req.datseq, uids_old );
				}
#endif
				env->evtref = ret_val[0];
				env->actref = ret_val[1];
				env->cntr_root = 1;
				env->tpix_root = tpidx;
			}
			else
			{
				r = -1;
#ifdef VT_JAVA_DEBUG
				_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "%s, failed to create an event\n", __FUNCTION__ );
#endif
			}
		}

#ifndef _VT_JAVA_EXTRACT_LIGHT_
		entity2.req.datseq.uids = uids_old;
#endif
	}
	else if( r == -100 )
	{
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_WARN, "%s, ignore the event(%u) due to restrictions\n", __FUNCTION__, entity2.type );
		r = -1;
	}
#ifdef VT_JAVA_DEBUG
	else
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "%s, illegal structure \"EVENT_ENTITY\"\n", __FUNCTION__ );
#endif

#ifndef _VT_JAVA_EXTRACT_LIGHT_
	_VT_JAVA_DelLocRef( env->jnienv, init_thread );

	if( caps )
		JNICall( env->jnienv, ReleaseLongArrayElements, capacity, (jlong*)caps, JNI_ABORT );

	// free resources belong to the object "entity"
	vt_java_event_entity_free( env, entity, &entity2, JNI_ABORT );
#endif

	// destroy mempool at last
	mmp_destroy_mempool( 1, &env->mmphd );

#ifndef _RECURSION_DISABLE_
	env->mmphd = _mmphd_;
#endif

	if( r == 0 )
	{
#if 0	// FOR DEBUG
		struct timespec ts;
		clock_gettime( CLOCK_MONOTONIC, &ts );
		printf( "BBBB %ld.%ld\n", ts.tv_sec, ts.tv_nsec );
#endif
		JNICall( env->jnienv, ReleaseLongArrayElements, ret_obj, ret_val, 0 );
		return ret_obj;
	}

	_VT_JAVA_DelLocRef( env->jnienv, ret_obj );

	return NULL;
}
__alias__("java_event_create") jobject JNICALL Java_jsentry_jvas_1vtrace_event_1create(); //__Ljava_lang_Object_2ILjava_lang_Object_2Ljava_lang_Object_2();


//
jlong JNICALL java_event_create2( JNIENV_PTR jni, jobject jcls,
								   jlong   tpidx,
								   jint    mode,
								   jobject capacity,
								   jobject entity
								   )
{
#if (_VT_JAVA_DEBUG_LEVEL_ == 0)
	return NULL;
#endif

	if( !g_vt_java_jvm ||
		!entity ||
		!(mode & 0x1f)
		)
	{
#ifdef VT_JAVA_DEBUG
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_WARN, "%s, wrong parameters, %p, %p, %x, line.%d\n",
				__FUNCTION__, g_vt_java_jvm, entity, mode, __LINE__ );
#endif
		return 0;
	}

	// get environment of current thread
	vt_java_thread_environ *env = _java_getenv( jni );
	if( !env || !env->jtienv )
		return 0;

	env->jnienv = jni;

#ifndef _RECURSION_DISABLE_
	long _mmphd_ = env->mmphd;
	env->mmphd = 0;
#endif

	// initiate mempool at first
	mmp_create_mempool( 1, &env->mmphd );

	//jboolean bl;
	int r;
	long ret_val[2];
	vt_event_entity entity2 = {0};
	jobject init_thread = NULL;

#ifndef _VT_JAVA_EXTRACT_LIGHT_
	vt_ident *uids_new, *uids_old;
	struct vt_java_capacity *caps = NULL;

	//
	JNICall( env->jtienv, GetCurrentThread, &init_thread );

	//
	if( capacity )
	{
		r = _VT_JAVA_GetArrLen( env->jnienv, capacity );
		if( r == VT_JAVA_CAPACITY_NMEMBER )
			caps = (struct vt_java_capacity*)JNICall( env->jnienv, GetLongArrayElements, capacity, NULL );
	}
#endif

	env->evtref = 0;
	env->actref = 0;

#ifdef VT_JAVA_DEBUG
	r = -100;
	entity2.type = 1;
	if( vt_check_event( 1, &entity2._idx_ ) >= 0 )
#else
	r = vt_java_event_entity_extract( env, entity, &entity2, TRUE ); // extract resources from the object "entity"
	if( r == JNI_OK )
#endif
	{
#if 0 //def VT_JAVA_DEBUG
		vt_print_event( stdout, init_thread, mode, (vt_capa*)caps, &entity2 );
#endif

		r = 0;

#ifndef _VT_JAVA_EXTRACT_LIGHT_
		uids_old = entity2.req.datseq.uids;

		if( entity2.req.datseq.ndat > 0 )
		{
			// data object mapping
			uids_new = (vt_ident*)mmp_malloc( 1, &env->mmphd, sizeof(vt_ident) * entity2.req.datseq.ndat );
			if( uids_new )
			{
				vt_java_data_sequence_mapping( env, &entity2.req.datseq, uids_new );
				entity2.req.datseq.uids = uids_new;
			}
			else
				r = -1;
		}

		if( r == 0 )
#endif
		{
			ret_val[1] = 0;
			ret_val[0] = (jlong)vt_event_create(
											#ifndef _VT_JAVA_EXTRACT_LIGHT_
												thd, mode, (caps && (caps->flags&VTRACE_CAPA_FLAG_ENABLE) ? (vt_capa*)caps : NULL),
											#else
												init_thread, mode, NULL,
											#endif
												 &entity2, (vt_refer*)&ret_val[1], NULL );  //(vt_refer)env );
			if( ret_val[0] != 0 )
			{
#ifndef _VT_JAVA_EXTRACT_LIGHT_
				if( entity2.req.datseq.ndat > 0 )
				{
					// Re-Binding if possible
					vt_java_data_sequence_binding( env, &entity2.req.datseq, uids_old );
				}
#endif
				env->evtref = ret_val[0];
				env->actref = ret_val[1];
				env->cntr_root = 1;
				env->tpix_root = tpidx;
			}
			else
			{
				r = -1;
#ifdef VT_JAVA_DEBUG
				_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "%s, failed to create an event\n", __FUNCTION__ );
#endif
			}
		}

#ifndef _VT_JAVA_EXTRACT_LIGHT_
		entity2.req.datseq.uids = uids_old;
#endif
	}
	else if( r == -100 )
	{
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_WARN, "%s, ignore the event(%u) due to restrictions\n", __FUNCTION__, entity2.type );
		r = -1;
	}
#ifdef VT_JAVA_DEBUG
	else
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "%s, illegal structure \"EVENT_ENTITY\"\n", __FUNCTION__ );
#endif

#ifndef _VT_JAVA_EXTRACT_LIGHT_
	_VT_JAVA_DelLocRef( env->jnienv, init_thread );

	if( caps )
		JNICall( env->jnienv, ReleaseLongArrayElements, capacity, (jlong*)caps, JNI_ABORT );

	// free resources belong to the object "entity"
	vt_java_event_entity_free( env, entity, &entity2, JNI_ABORT );
#endif

	// destroy mempool at last
	mmp_destroy_mempool( 1, &env->mmphd );

#ifndef _RECURSION_DISABLE_
	env->mmphd = _mmphd_;
#endif

	if( r == 0 )
	{
#if 0	// FOR DEBUG
		struct timespec ts;
		clock_gettime( CLOCK_MONOTONIC, &ts );
		printf( "BBBB %ld.%ld\n", ts.tv_sec, ts.tv_nsec );
#endif
		return ret_val[0];
	}

	return 0;
}
__alias__("java_event_create2") jlong JNICALL Java_jsentry_jvas_1vtrace_event_1create2();


//
jlong JNICALL java_event_create3( JNIENV_PTR jni, jobject jcls,
								   jlong   tpidx,
								   jint    mode,
								   jint    etype
								   )
{
#if (_VT_JAVA_DEBUG_LEVEL_ == 0)
	return NULL;
#endif

	if( !g_vt_java_jvm ||
		!(mode & 0x1f)
		)
	{
#ifdef VT_JAVA_DEBUG
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_WARN, "%s, wrong parameters, %p, %d, %x, line.%d\n",
				__FUNCTION__, g_vt_java_jvm, etype, mode, __LINE__ );
#endif
		return 0;
	}

	// get environment of current thread
	vt_java_thread_environ *env = _java_getenv( jni );
	if( !env || !env->jtienv )
		return 0;

	env->jnienv = jni;

	long ret_val[2] = {0};
	vt_event_entity entity2 = {0};

	env->evtref = 0;
	env->actref = 0;
	entity2.type = etype;
	if( vt_check_event( 1, &entity2._idx_ ) >= 0 )
	{
		ret_val[0] = (jlong)vt_event_create( NULL, mode, NULL, &entity2, (vt_refer*)&ret_val[1], NULL );
		if( ret_val[0] != 0 )
		{
			env->evtref = ret_val[0];
			env->actref = ret_val[1];
			env->cntr_root = 1;
			env->tpix_root = tpidx;
		}
#ifdef VT_JAVA_DEBUG
		else
			_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "%s, failed to create an event\n", __FUNCTION__ );
#endif
	}
#ifdef VT_JAVA_DEBUG
	else
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_WARN, "%s, ignore the event(%u) due to restrictions\n", __FUNCTION__, entity2.type );
#endif

	return ret_val[0];
}
__alias__("java_event_create3") jlong JNICALL Java_jsentry_jvas_1vtrace_event_1create3();


// Function:
jlong JNICALL java_event_counter_set( JNIENV_PTR jni, jobject jcls, jlong tpidx, jint delta )
{
	if( !g_vt_java_jvm )
		return -1;

	// get environment of current thread
	vt_java_thread_environ *env = _java_getenv( jni );
	if( !env )
		return -1;

	if( !env->evtref || !env->actref )
		return -(delta < 0);  // return 0 if the delta >= 0, otherwise -1 will be returned

	if( env->tpix_root != tpidx )
		return env->cntr_root;

	return __sync_add_and_fetch( &env->cntr_root, delta );
}
__alias__("java_event_counter_set") jlong JNICALL Java_jsentry_jvas_1vtrace_event_1counter_1set();


// Function:
// Parameter:
//   N; evtref:= event reference been returned by the event_create
//               method will treats the last "event" has attached on current thread as the target if it is 0
// Return: none
void JNICALL java_event_destroy( JNIENV_PTR jni, jobject jcls, jlong evtref )
{
#if (_VT_JAVA_DEBUG_LEVEL_ == 0)
	return;
#endif

	vt_java_thread_environ *env = _java_getenv( jni );
	if( !env || !env->jtienv )
		return;

	if( evtref == 0 )
		evtref = env->evtref;

	if( evtref )
	{
#if 0	// FOR DEBUG
		struct timespec ts;
		clock_gettime( CLOCK_MONOTONIC, &ts );
		printf( "EEEE %ld.%ld\n", ts.tv_sec, ts.tv_nsec );
#endif

		vt_event_destroy( (vt_refer)evtref );

		_java_action_exit( NULL, env );

		//env->actref = 0;
		//env->evtref = 0;

#if 0	// FOR DEBUG
		clock_gettime( CLOCK_MONOTONIC, &ts );
		printf( "EEEE2 %ld.%ld\n", ts.tv_sec, ts.tv_nsec );
#endif
	}
}
__alias__("java_event_destroy") void JNICALL Java_jsentry_jvas_1vtrace_event_1destroy(); //__J();


// Function:
// Parameter:
//   N; new_thread:= a new thread object
//   O; mode:= see "trace mode"
//   O; capacity:= array of long which represents the vt_capacity
//   N; evtref:= event reference been created by the event_create
//               method will treats the last "event" has attached on current thread as the target if it is 0
// Return:
//   action reference if succeed otherwise -1 will be returned.
jlong JNICALL java_action_new( JNIENV_PTR jni, jobject jcls,
							   jobject new_thread,
							   jint    mode,
							   jobject capacity,
							   jlong   evtref
							   )
{
	if( !g_vt_java_jvm ||
		!evtref ||
		!(mode & 0x1f)
		)
	{
#ifdef VT_JAVA_DEBUG
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_WARN, "%s, wrong parameters, line.%d\n", __FUNCTION__, __LINE__ );
#endif
		return -1;
	}

	// get environment of current thread
	vt_java_thread_environ *env = _java_getenv( jni );
	if( !env || !env->jtienv )
		return -1;

	env->jnienv = jni;

#ifndef _RECURSION_DISABLE_
	long _mmphd_ = env->mmphd;
	env->mmphd = 0;
#endif

	// initiate mempool at first
	mmp_create_mempool( 1, &env->mmphd );


	jlong actref = -1;

#ifndef _VT_JAVA_EXTRACT_LIGHT_
	struct vt_java_capacity *caps = NULL;
	jthread thd;
	int r;
#endif

	env->evtref = 0;
	env->actref = 0;

#ifndef _VT_JAVA_EXTRACT_LIGHT_
	//
	if( capacity )
	{
		r = _VT_JAVA_GetArrLen( env->jnienv, capacity );
		if( r == VT_JAVA_CAPACITY_NMEMBER )
			caps = (struct vt_java_capacity*)JNICall( env->jnienv, GetLongArrayElements, capacity, NULL );
	}

#ifdef VT_JAVA_DEBUG
	fprintf( stdout, "ActionNew:{\n\tnew_thread: %p,\n\tmode: %d,\n\t", new_thread, mode );
	if( caps )
		vt_print_capacity( stdout, "\n\t\t", "\n", (vt_capa*)caps );
	fprintf( stdout, "}\n" );
#endif

	//
	if( !new_thread )
		JNICall( env->jtienv, GetCurrentThread, &thd );  //new_thread = env->thread;
	else
		thd = new_thread;

	actref = (jlong)vt_action_new( thd, mode, (vt_capa*)caps, (vt_refer)evtref, NULL );  //(vt_refer)env );

#else
	actref = (jlong)vt_action_new( new_thread, mode, NULL, (vt_refer)evtref, NULL );  //(vt_refer)env );
#endif

	if( actref )
	{
		env->evtref = evtref;
		env->actref = actref;
		env->cntr_root = 0;
		env->tpix_root = 0;
	}
#ifdef VT_JAVA_DEBUG
	else
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "%s, failed to create an action\n", __FUNCTION__ );
#endif

#ifndef _VT_JAVA_EXTRACT_LIGHT_
	if( !new_thread )
		_VT_JAVA_DelLocRef( env->jnienv, thd );

	if( caps )
		JNICall( env->jnienv, ReleaseLongArrayElements, capacity, (jlong*)caps, JNI_ABORT );
#endif

	// destroy mempool at last
	mmp_destroy_mempool( 1, &env->mmphd );

#ifndef _RECURSION_DISABLE_
	env->mmphd = _mmphd_;
#endif

	return actref;
}
__alias__("java_action_new") jlong JNICALL Java_jsentry_jvas_1vtrace_action_1new();


// Function:
// Parameter:
//   O; actref:= delete all if it is 0, otherwise the specified action will be deleted;
//   N; evtref:= event reference been created by the event_create
//               method will treats the last "event" has attached on current thread as the target if it is 0
// Return: none
void JNICALL java_action_del( JNIENV_PTR jni, jobject jcls,
							  jlong actref,
							  jlong evtref )
{
	vt_java_thread_environ *env = _java_getenv( jni );
	if( !env || !env->jtienv )
		return;

	env->jnienv = jni;

	if( evtref == 0 )
		evtref = env->evtref;
	if( actref == 0 )
		actref = env->actref;

	vt_action_del( (vt_refer)actref, (vt_refer)evtref );

	_java_action_exit( NULL, env );

	//env->actref = 0;
	//env->evtref = 0;
}
__alias__("java_action_del") void JNICALL Java_jsentry_jvas_1vtrace_action_1del();


#if 0
// Function:
// Parameter:
// Return:
//   action reference if succeed otherwise -1 will be returned.
jlong JNICALL java_action_get( JNIENV_PTR jni, jobject jcls,
							   jobject thread,
							   jlong evtref )
{
	return -1;
}
__alias__("java_action_get") jlong JNICALL Java_jsentry_jvas_1vtrace_action_1get();
#endif


// Function:
// Parameter:
//   N; tpidx:= index of trace point(not identifier); can be obtained via the method tp_idx_copy()
//   N; stage:= see "trace stage"
//   P; cause:= see "triggered reason"
//   N; entity:= contents be traced; array of object which represents the vt_sot_entity;
//   N; actref:= action reference been returned from the event_create or the action_new
//               method will treats the last "action" has attached on current thread as the target if it is 0
// Return:
//   SOT reference if succeed otherwise -1 will be returned.
VT_LOCAL
jlong JNICALL _java_sot_append( vt_java_thread_environ *env,
							    jlong   tpidx,
							    jint    stage,
							    jint    cause,
							    jobject entity,
							    jlong   actref,
								vt_time tmbeg
							    )
{
	// check traced state
	vt_operator_set opset;
	vt_uint argn;
	int tpsta = _CheckTPoint( tpidx, (vt_refer)actref, &opset, &argn );  // vt_check_point
	if( tpsta < 0 )
		return -1;
	//else if( tpsta == 0 && opset == VTRACE_OPSET_PROPAGATE )  // reach at top, FOR DEBUG
	//	return 0;


	jlong num, sotref = -1;
	vt_sot_entity sot_entity = {0};
	vt_tlv args[32];
#ifdef _VT_JAVA_EXTRACT_LIGHT_
	vt_ident *uids_new[1], *uids_old[1], _uids_[1][32];
	vt_bool __unused__ isException = FALSE;
#else
	vt_ident *uids_new[2], *uids_old[2], _uids_[2][32];
	vt_bool isException = (stage==VTRACE_STAGE_CATCH || stage==VTRACE_STAGE_CRASH);
#endif


#ifndef _RECURSION_DISABLE_
	long _mmphd_ = env->mmphd;
	env->mmphd = 0;
#endif

	// initiate mempool at first
	mmp_create_mempool( 1, &env->mmphd );

	//JNICall( env->jnienv, ExceptionClear );

#if (_VT_JAVA_DEBUG_LEVEL_ > 2)

	sot_entity.invk.args.datseq.ndat = argn;
	sot_entity.invk.args.datseq.datas = argn <= sizeof(args)/sizeof(vt_tlv) ? args : NULL;

#ifndef _VT_JAVA_PARAMETER_LIGHT_
	if( vt_java_sot_entity_extract( env, entity, &sot_entity, isException ) == JNI_OK )
#else
	if( vt_java_tlv_array_acquire( env, entity, &sot_entity.invk.args.datseq.datas, &sot_entity.invk.args.datseq.ndat ) == JNI_OK )
#endif
	{
#ifdef VT_JAVA_DEBUG
		//vt_print_sot( stdout, actref, tpidx, stage, cause, &sot_entity );
#endif

		uids_new[0] = NULL;
		uids_old[0] = sot_entity.invk.args.datseq.uids;

#ifndef _VT_JAVA_EXTRACT_LIGHT_
		uids_new[1] = NULL
		uids_old[1] = sot_entity.watch.args.datseq.uids;
#endif
		num = 0;

#if (_VT_JAVA_DEBUG_LEVEL_ > 3)

#ifndef _VT_JAVA_EXTRACT_LIGHT_
		if( !isException )
#endif
		{
			// do data object mapping at first
			if( sot_entity.invk.args.datseq.ndat > 0 )
			{
				uids_new[0] = sot_entity.invk.args.datseq.ndat <= sizeof(args)/sizeof(vt_tlv)
						? _uids_[0] : (vt_ident*)mmp_malloc( 1, &env->mmphd, sizeof(vt_ident) * sot_entity.invk.args.datseq.ndat );
				if( uids_new[0] )
				{
					num = vt_java_data_sequence_mapping( env, &sot_entity.invk.args.datseq, uids_new[0] );  // datas => uids
					sot_entity.invk.args.datseq.uids = uids_new[0];
				}
				else
					actref = 0;
			}
		}

#ifndef _VT_JAVA_EXTRACT_LIGHT_
		if( sot_entity.watch.args.datseq.ndat > 0 && actref != 0 )
		{
			uids_new[1] = sot_entity.watch.args.datseq.ndat <= sizeof(args)/sizeof(vt_tlv)
					? _uids_[1] : (vt_ident*)mmp_malloc( 1, &env->mmphd, sizeof(vt_ident) * sot_entity.watch.args.datseq.ndat );
			if( uids_new[1] )
			{
				num += vt_java_data_sequence_mapping( env, &sot_entity.watch.args.datseq, uids_new[1] );
				sot_entity.watch.args.datseq.uids = uids_new[1];
			}
			else
				actref = 0;
		}
#endif

#if (_VT_JAVA_DEBUG_LEVEL_ > 4)

		if( actref != 0 && (num > 0 || VTRACE_OPSET_IS_TRACED(opset)) )
		{
			// append SOT
			sotref = (jlong)vt_sot_append( tpidx, stage, cause, &sot_entity, (vt_refer)actref, tmbeg );
			if( sotref != 0 )
			{
#ifndef VT_JAVA_ACTION_EQUAL_THREAD
				if( vt_action_lock((vt_refer)actref, -1) )
#endif
				{
					// Re-Binding if possible
					if( uids_new[0] )
						vt_java_data_sequence_binding( env, &sot_entity.invk.args.datseq, uids_old[0] );

#ifndef _VT_JAVA_EXTRACT_LIGHT_
					if( uids_new[1] )
						vt_java_data_sequence_binding( env, &sot_entity.watch.args.datseq, uids_old[1] );
#endif

#ifndef VT_JAVA_ACTION_EQUAL_THREAD
					vt_action_unlock( (vt_refer)actref );
#endif
				}
			}
#ifdef VT_JAVA_DEBUG
			else if( num>0 && VTRACE_OPSET_IS_TRACED(opset) )
				_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "%s, failed to append SOT\n", __FUNCTION__ );
#endif
		}

#endif  // #if (_VT_JAVA_DEBUG_LEVEL_ > 4)
#endif  // #if (_VT_JAVA_DEBUG_LEVEL_ > 3)

		sot_entity.invk.args.datseq.uids = uids_old[0];
#ifndef _VT_JAVA_EXTRACT_LIGHT_
		sot_entity.watch.args.datseq.uids = uids_old[1];
#endif
	}
#ifdef VT_JAVA_DEBUG
	else
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "%s, illegal structure \"SOT_ENTITY\"\n", __FUNCTION__ );
	//			((vt_cstr*)((vt_action*)actref)->ctx->ns->defines.methods[tpidx-1]->name)->cs,
	//			(char*)((vt_action*)actref)->ctx->ns->defines.methods[tpidx-1]->decl,
	//			((vt_cstr*)((vt_action*)actref)->ctx->ns->defines.methods[tpidx-1]->klass->name)->cs,
#endif

#if !defined( _VT_JAVA_EXTRACT_LIGHT_ ) && !defined( _VT_JAVA_PARAMETER_LIGHT_ )
	vt_java_sot_entity_free( env, entity, &sot_entity, JNI_ABORT, isException );
#endif
#endif

	// destroy mempool at last
	mmp_destroy_mempool( 1, &env->mmphd );

#ifndef _RECURSION_DISABLE_
	env->mmphd = _mmphd_;
#endif

	return sotref;
}


// Note: this function adapts specially to the language JAVA
jlong JNICALL java_sot_append( JNIENV_PTR jni, jobject jcls,
							   jlong   tpidx,
							   jint    stage,
							   jint    cause,
							   jobject entity,
							   jlong   actref
							   )
{
#if (_VT_JAVA_DEBUG_LEVEL_ <= 1)
	return 0;
#endif

#ifdef _DEBUG
	if( !g_vt_java_jvm
		|| !entity
		|| tpidx < 0
		/*|| !stage*/
		/*|| stage != VTRACE_STAGE_LEVEL*/	/* FOR TEST */
		/*|| !actref*/
		)
#else
	if( !g_vt_java_jvm )
#endif
	{
#ifdef VT_JAVA_DEBUG
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_WARN, "%s, wrong parameters, %ld, %d, %d, %p, %ld, line.%d\n",
				__FUNCTION__, tpidx, stage, cause, entity, actref, __LINE__ );
#endif
		return -1;
	}

#ifndef VT_STATISTIC_TIME_DISABLE
	time_t tmbeg = _GetTicks();
#endif

	// get environment of current thread
	vt_java_thread_environ *env = _java_getenv( jni );
	if( !env || !env->jtienv )
		return -1;

	env->jnienv = jni;

	if( !actref )
	{
		if( !env->actref )
			return -1;
		actref = env->actref;
	}
	//else
	//	env->actref = actref;

	tpidx++;

	// check traced state
	if( _CheckAction( tpidx, (vt_refer)actref, TRUE ) < 0 )  // vt_check_action
		return -1;

#if 0
	if( tpidx <= 0 )
	{
		// try to get the tpidx
		// bind a tpidx with the method if it is not found
		tpidx = _java_get_tpidx( env, NULL );
		if( tpidx <= 0 )
			return -1;
	}
#endif

	return _java_sot_append( env, tpidx, stage, cause, entity, actref,
															#ifndef VT_STATISTIC_TIME_DISABLE
																		tmbeg
															#else
																		0
															#endif
																		);
}
__alias__("java_sot_append") jlong JNICALL Java_jsentry_jvas_1vtrace_sot_1append();


// Function: OBSOLETED
// Parameter:
//   N; name:=
//   N: sign:=
//   N: klass:=
//   N; stage:= see "trace stage"
//   P; cause:= see "triggered reason"
//   N; entity:= contents be traced; array of object which represents the vt_sot_entity;
//   N; actref:= action reference been returned from the event_create or the action_new
//               method will treats the last "action" has attached on current thread as the target if it is 0
// Return:
//   SOT reference if succeed otherwise -1 will be returned.
jlong JNICALL java_sot_append2( JNIENV_PTR jni, jobject jcls,
							    jstring name,
							    jstring sign,
							    jstring klass,
							    jint    stage,
							    jint    cause,
							    jobject entity,
							    jlong   actref
							    )
{
#if (_VT_JAVA_DEBUG_LEVEL_ <= 1)
	return 0;
#endif

	if( !g_vt_java_jvm
		|| !entity
		|| stage != VTRACE_STAGE_LEAVE	/* FOR TEST */
		/*|| !stage*/
		/*!actref*/
		|| !name || !sign || !klass
		)
	{
#ifdef VT_JAVA_DEBUG
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_WARN, "%s, wrong parameters, %p, %p, %p, %d, %d, %p, %ld, line.%d\n",
				__FUNCTION__, name, sign, klass, stage, cause, entity, actref, __LINE__ );
#endif
		return -1;
	}


	// get environment of current thread
	vt_java_thread_environ *env = _java_getenv( jni );
	if( !env || !env->jtienv )
		return -1;

	env->jnienv = jni;

	if( !actref )
	{
		if( !env->actref )
			return -1;
		actref = env->actref;
	}
	//else
	//	env->actref = actref;


	// check traced state
	if( vt_check_action( 0, (vt_refer)actref, TRUE ) < 0 )
		return -1;


	// try to get the tpidx
	//jlong tpidx = _java_get_tpidx( env, NULL );

	jlong tpidx = _java_get_tpidx1( env, name, sign, klass );

	//jlong tpidx = _java_get_tpidx2( env, sign );

	if( tpidx <= 0 )
		return -1;

	return _java_sot_append( env, tpidx, stage, cause, entity, actref,
															#ifndef VT_STATISTIC_TIME_DISABLE
																		_GetTicks()
															#else
																		0
															#endif
																		);
}
__alias__("java_sot_append2") jlong JNICALL Java_jsentry_jvas_1vtrace_sot_1append2();  // OBSOLETED


// Function: rebuild argument list if there are scalars packed as floating-point value
__attribute__((optimize("O3")))
_VT_JAVA_INLINE_ VT_LOCAL
void _java_rebuild_arglist_due_fp( jint narg, void **args, void **args_ret, vt_point *tp )
{
	register jint i, j, f;
	register struct _vtrace_data_class **arg_def = tp->target->arglst.args;

	j = f = 0;
	for( i=0; i < narg && f < 8; i++ )
	{
		if( arg_def[i]->type == VT_TYPE_FLOAT || arg_def[i]->type == VT_TYPE_DOUBLE )
		{
			// xmm0 ~ xmm7
			switch( f )
			{
			case 0:
				__asm__ volatile ( "movsd %%xmm0,(%[arg_r])\n\t"
						: /* no output*/
						: [arg_r] "rm" (&args_ret[i])
						: "memory"
				);
				break;
			case 1:
				__asm__ volatile ( "movsd %%xmm1,(%[arg_r])\n\t"
						: /* no output*/
						: [arg_r] "rm" (&args_ret[i])
						: "memory"
				);
				break;
			case 2:
				__asm__ volatile ( "movsd %%xmm2,(%[arg_r])\n\t"
						: /* no output*/
						: [arg_r] "rm" (&args_ret[i])
						: "memory"
				);
				break;
			case 3:
				__asm__ volatile ( "movsd %%xmm3,(%[arg_r])\n\t"
						: /* no output*/
						: [arg_r] "rm" (&args_ret[i])
						: "memory"
				);
				break;
			case 4:
				__asm__ volatile ( "movsd %%xmm4,(%[arg_r])\n\t"
						: /* no output*/
						: [arg_r] "rm" (&args_ret[i])
						: "memory"
				);
				break;
			case 5:
				__asm__ volatile ( "movsd %%xmm5,(%[arg_r])\n\t"
						: /* no output*/
						: [arg_r] "rm" (&args_ret[i])
						: "memory"
				);
				break;
			case 6:
				__asm__ volatile ( "movsd %%xmm6,(%[arg_r])\n\t"
						: /* no output*/
						: [arg_r] "rm" (&args_ret[i])
						: "memory"
				);
				break;
			case 7:
				__asm__ volatile ( "movsd %%xmm7,(%[arg_r])\n\t"
						: /* no output*/
						: [arg_r] "rm" (&args_ret[i])
						: "memory"
				);
				break;
			}
			f++;
		}
		else
		{
			args_ret[i] = args[j++];
		}
	}

	if( i < narg )
		memcpy( &args_ret[i], &args[j], sizeof(void*)*(narg-i) );
}

// Function:
// Parameter:
//   N; tpidx:= index of trace point(not identifier); can be obtained via the method tp_idx_copy()
//   N; stage:= see "trace stage"
//   P; cause:= see "triggered reason"
//   N; args:=
//   N; narg:=
//   N; actref:= action reference been returned from the event_create or the action_new
//               method will treats the last "action" has attached on current thread as the target if it is 0
// Return:
//   SOT reference if succeed otherwise -1 will be returned.
VT_LOCAL
jlong JNICALL _java_sot_append_x( vt_java_thread_environ *env,
								  jlong    tpidx,
								  jint     stage,
								  jint     cause,
								  void   **args,
								  jint     narg,
								  jlong    actref,
								  vt_time  tmbeg
							    )
{
	// check traced state
	vt_point *tp;
	int tpsta = _CheckTPoint_x( tpidx, (vt_refer)actref, &tp );  // vt_check_point
	if( tpsta < 0 )
	{
#ifdef VT_JAVA_DEBUG
		_VT_DEBUG_PRINT_( stderr, SABI_JOURNAL_ERROR, "check point error, tpidx=%ld\n", tpidx );
#endif
		return -1;
	}

	// rebuild argument list if there are scalars packed as floating-point value
	void *args_new[256]; // maximum rooms enough to save potential arguments
	if( tp->target->arglst.fpnum > 0 )
	{
		_java_rebuild_arglist_due_fp( narg, args, args_new, tp );
		args = args_new;
	}

	if( stage & (VTRACE_STAGE_CATCH | VTRACE_STAGE_CRASH) )
		narg--;  // assume the last argument is an exception message string

	if( tp->target->arglst.argn != narg )  // FOR DEBUG
	{
#ifdef VT_JAVA_DEBUG
		_VT_DEBUG_PRINT_( stderr, SABI_JOURNAL_ERROR, "unmatched number of arguments, %d!=%d; in method %s%s\n",
				 tp->target->arglst.argn, narg, ((vt_cstr*)tp->target->name)->cs, (char*)tp->target->decl );
#endif
		return -100;
	}

#if 0 //def VT_JAVA_DEBUG
	// FOR DEBUG
	_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "%s: %ld, %X, %s%s, %s\n", __FUNCTION__, tpidx, stage,
			((vt_cstr*)tp->target->name)->cs, (char*)tp->target->decl, ((vt_cstr*)tp->target->klass->name)->cs );
#endif

	jint i, num;
	jlong sotref = -1;
	vt_sot_entity sot_entity = {0};
	vt_ident _uids_[2][64], *uids_new, *uids_old;


#ifndef _RECURSION_DISABLE_
	long _mmphd_ = env->mmphd;
	env->mmphd = 0;
#endif

	// initiate mempool at first
	mmp_create_mempool( 1, &env->mmphd );

#if (_VT_JAVA_DEBUG_LEVEL_ > 2)
	sot_entity.invk.args.datseq.ndat = narg;
	//sot_entity.invk.args.datseq.datas = NULL;
	//sot_entity.invk.args.datseq.names = NULL;
	//sot_entity.invk.args.datseq.uids = NULL;
	if( narg > 0 )
	{
		if( narg <= sizeof(_uids_)/sizeof(vt_ident) )
			sot_entity.invk.args.datseq.uids = uids_new = _uids_[0];
		else
			sot_entity.invk.args.datseq.uids = uids_new = (vt_ident*)mmp_malloc( 1, &env->mmphd, sizeof(vt_ident)*narg*2 );

		if( uids_new )
		{
			struct _vtrace_data_class **arglst = tp->target->arglst.args;
			for( i=0; i < narg; i++ )
			{
				uids_new[i].low = (vt_ulong)args[i];  // original value(object reference or measurement)
				if( args[i] && arglst[i]->type == VT_TYPE_CLASS && !arglst[i]->mod.o )
				{
					uids_new[i].high = _java_object_map( env, args[i] );  // local address of data object
					num += uids_new[i].high != -1;
				}
				else
				{
					uids_new[i].high = -1;
				}
			}

			// copy origins for Re-Binding
			uids_old = &uids_new[narg];
			memcpy( uids_old, uids_new, narg*sizeof(vt_ident) );
		}
		else
			actref = 0;
	}
	else
	{
		num = 0;
		uids_new = uids_old = NULL;
	}


#if (_VT_JAVA_DEBUG_LEVEL_ > 3)
#if (_VT_JAVA_DEBUG_LEVEL_ > 4)
	if( actref != 0 && (num > 0 || VTRACE_OPSET_IS_TRACED(tp->target->opset)) )
	{
		if( stage & (VTRACE_STAGE_CATCH | VTRACE_STAGE_CRASH) )
		{
			sot_entity.watch = sot_entity.invk;

			memset( &sot_entity.expt, 0, sizeof(vt_data_exception) );
			if( args[narg] )
			{
				// copy exception message
				i = JNICall( env->jnienv, GetStringUTFLength, args[narg] );
				if( i > 0 )
				{
					sot_entity.expt.msgstr = mmp_malloc( 1, &env->mmphd, i + 4 );
					if( sot_entity.expt.msgstr )
					{
						JNICall( env->jnienv, GetStringUTFRegion, args[narg], 0, i, sot_entity.expt.msgstr );
						sot_entity.expt.msgstr[i] = 0;
						sot_entity.expt.msglen = i;
					}
				}
			}
		}

		// append SOT
		sotref = (jlong)vt_sot_append( tpidx, stage, cause, &sot_entity, (vt_refer)actref, tmbeg );
		if( sotref != 0 )
		{
			if( uids_new )
			{
#ifndef VT_JAVA_ACTION_EQUAL_THREAD
				if( vt_action_lock((vt_refer)actref, -1) )
#endif
				{
					// Re-Binding if possible
					for( i=narg-1; i >= 0; i-- )
					{
						if( uids_old[i].low && uids_new[i].high && uids_new[i].high != uids_old[i].high )  // && uids_new[i].high != -1
						{
							if( _java_object_bind( env, (jobject)uids_old[i].low, uids_new[i].high ) != JVMTI_ERROR_NONE )
								_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "%s, set tag error; %llx, %llx\n", __FUNCTION__, uids_old[i].low, uids_new[i].high );
						}
					}

#ifndef VT_JAVA_ACTION_EQUAL_THREAD
				vt_action_unlock( (vt_refer)actref );
#endif
				}
			}
		}
#ifdef VT_JAVA_DEBUG
		else if( num>0 && VTRACE_OPSET_IS_TRACED(tp->target->opset) )
			_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "%s, failed to append SOT; tpix=%ld, narg=%d, actref=%p\n",
					__FUNCTION__, tpidx, narg, (void*)actref );
#endif
	}

#endif  // #if (_VT_JAVA_DEBUG_LEVEL_ > 4)
#endif  // #if (_VT_JAVA_DEBUG_LEVEL_ > 3)
#endif  // #if (_VT_JAVA_DEBUG_LEVEL_ > 2)

	// destroy mempool at last
	mmp_destroy_mempool( 1, &env->mmphd );

#ifndef _RECURSION_DISABLE_
	env->mmphd = _mmphd_;
#endif

	return sotref;
}


#ifdef _VT_JAVA_PARAMETER_LIGHT_
// Note: this function can servers to language C and JAVA
jlong JNICALL java_sot_append3( JNIENV_PTR jni, jobject jcls,
							   jlong   tpidx,
							   jint    stage,
							   jint    cause,
							   jobject entity
							   )
{
#if (_VT_JAVA_DEBUG_LEVEL_ <= 1)
	return 0;
#endif

#ifdef _DEBUG
	if( !g_vt_java_jvm
		|| !entity
		|| tpidx < 0
		/*|| !stage*/
		/*|| stage != VTRACE_STAGE_LEVEL*/	/* FOR TEST */
		/*|| !actref*/
		)
#else
	if( !g_vt_java_jvm )
#endif
	{
#ifdef VT_JAVA_DEBUG
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_WARN, "%s, wrong parameters, %ld, %d, %d, %p, line.%d\n",
				__FUNCTION__, tpidx, stage, cause, entity, __LINE__ );
#endif
		return -1;
	}

#ifndef VT_STATISTIC_TIME_DISABLE
	time_t tmbeg = _GetTicks();
#endif

	// get environment of current thread
	vt_java_thread_environ *env = _java_getenv( jni );
	if( !env || !env->jtienv )
		return -1;

	env->jnienv = jni;

	if( !env->actref )
		return -1;

	tpidx++;

	// check traced state
	if( _CheckAction( tpidx, (vt_refer)env->actref, TRUE ) < 0 )  // vt_check_action
		return -1;

	return _java_sot_append( env, tpidx, stage, cause, entity, env->actref,
															#ifndef VT_STATISTIC_TIME_DISABLE
																		tmbeg
															#else
																		0
															#endif
																		);
}
__alias__("java_sot_append3") jlong JNICALL Java_jsentry_jvas_1vtrace_sot_1append3();
#endif


// Note: this function can servers to language C and JAVA
__attribute__((optimize("O3")))
jlong JNICALL java_sot_append1( JNIENV_PTR jni, jobject jcls,
							   jlong   tpidx,
							   jint    stage,
							   jint    cause,
							   jint    ndat,
							   void   *placehold[0]
							   )
{
#if (_VT_JAVA_DEBUG_LEVEL_ <= 1)
	return 0;
#endif

#ifdef _DEBUG
	if( !g_vt_java_jvm
		|| tpidx <= 0	/* MUST greater than 0 */
		/*|| !stage*/
		/*|| stage != VTRACE_STAGE_LEVEL*/	/* FOR TEST */
		)
#else
	if( !g_vt_java_jvm )
#endif
	{
#ifdef VT_JAVA_DEBUG
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_WARN, "%s, wrong parameters, %ld, %d, %d, %d, line.%d\n",
				__FUNCTION__, tpidx, stage, cause, ndat, __LINE__ );
#endif
		return -1;
	}

#ifndef VT_STATISTIC_TIME_DISABLE
	time_t tmbeg = _GetTicks();
#endif

	// get environment of current thread
	vt_java_thread_environ *env = _java_getenv( jni );
	if( !env || !env->jtienv )
		return -1;

	env->jnienv = jni;

	//tpidx++;  Note, do not plus one

#if 0
	if( stage & VTRACE_STAGE_BEGIN )
	{
		// begin to tracing
		if( !env->actref )
		{
			// to create an event
			env->tpix_root = tpidx;
			env->cntr_root = 1;
		}
		else if( env->tpix_root == tpidx )
			env->cntr_root++;
	}
#endif

	if( !env->actref )
		return -1;

	// check traced state
	if( _CheckAction( tpidx, (vt_refer)env->actref, TRUE ) < 0 )  // vt_check_action
		return -1;

	//void **dats = (void**)&placehold;  // care must to be taken, this assignment is necessary if called directly by JVM
	//jlong ret =
	return _java_sot_append_x( env, tpidx, stage, cause,
							   (void**)&placehold, ndat,
							   env->actref,
				#ifndef VT_STATISTIC_TIME_DISABLE
							   tmbeg
				#else
							   0
				#endif
							   );

#if 0
	if( stage & VTRACE_STAGE_END )
	{
		// end of tracing
		if( env->tpix_root == tpidx && --env->cntr_root <= 0 )
		{
			// to destroy current event
		}
	}

	return ret;
#endif
}

// Function: Java_jsentry_jvas_1vtrace_sot_1append0, stub assemble code, only used for redirecting to the java_sot_append1
// Note: only for testing or debugging
__asm__ (
        ".global Java_jsentry_jvas_1vtrace_sot_1append0\n\t"
        "Java_jsentry_jvas_1vtrace_sot_1append0:\n\t"
        "test %r9d,%r9d\n\t"        	/* test ndat */
        "jle _LAB_WITHOUT_ARGS\n\t"     /* jump to _LAB_WITHOUT_ARGS if ndat <=0 */
        "lea 0x8(%rsp),%rax\n\t"		/* skip "rip" */
        "pushq %rax\n\t"           		/* push the first "data object"(%rsp+0x10) onto current stack */
        "_LAB_READY_CALL:\n\t"
        "lea _LAB_AFTER_CALL(%rip),%rax\n\t"
        "pushq %rax\n\t"   				/* push an address when return from the java_sot_append1 */
        "jmp java_sot_append1\n\t"      /* call the java_sot_append1 */
        "_LAB_AFTER_CALL:\n\t"
        "add $0x08,%rsp\n\t"			/* restore the %rsp */
        "retq\n\t"             			/* NOTE: the return value has been wrote into the %rax by the java_sot_append1 */
        "_LAB_WITHOUT_ARGS:\n\t"
        "pushq $0\n\t"                  /* without "data object" */
        "jmp _LAB_READY_CALL\n\t"
        );


#if 0
// Note: this function can servers to language C and JAVA
//#pragma GCC push_options
//#pragma GCC optimize("O3")
__attribute__((optimize("O3")))
void JNICALL java_sot_append0( JNIENV_PTR jni, jobject jcls,
							   	jlong   tpidx,
								jint    stage,
								jint    cause,
								jint    ndat
								)
{
    __asm__ volatile(
            "test %%r9d,%%r9d\n\t"          /* test ndat */
            "jle _LAB_WITHOUT_ARGS\n\t"     /* jump to _LAB_WITHOUT_ARGS if ndat <=0 */
#ifdef _RELEASE
            "lea 0x8(%%rsp),%%rax\n\t"		/* skip "rip" */
#else
    		"lea 0x10(%%rsp),%%rax\n\t"		/* skip "rip" and "rbp" */
#endif
            "pushq %%rax\n\t"           	/* push the first "data object"(%rsp+0x10) onto current stack */
            "_LAB_READY_CALL:\n\t"
            "lea _LAB_AFTER_CALL(%%rip),%%rax\n\t"
            "pushq %%rax\n\t"   			/* push an address when return from the java_sot_append1 */
            "jmp java_sot_append1\n\t"      /* call the java_sot_append1 */
            "_LAB_AFTER_CALL:\n\t"
            "add $0x08,%%rsp\n\t"           /* restore the %rsp */
            "jmp _LAB_EXIT\n\t"             /* NOTE: the return value has been wrote into the %rax by the java_sot_append1 */
            "_LAB_WITHOUT_ARGS:\n\t"
            "pushq $0\n\t"                  /* without "data object" */
            "jmp _LAB_READY_CALL\n\t"
            "_LAB_EXIT:\n\t"
            :::
            );
}
//#pragma GCC pop_options
//__alias__("java_sot_append0") void JNICALL Java_jsentry_jvas_1vtrace_sot_1append0();
#endif

__alias__("java_sot_append1") jlong JNICALL Java_jsentry_jvas_1vtapi_1stub_sot_1append_1x();  // jsentry.jvas_vtapi_stub.sot_append_x()
__alias__("java_sot_append1") jlong JNICALL Java_jsentry_jvas_1vtrace_sot_1append_1x();

__alias__("java_sot_append1") jlong JNICALL Java_jsentry_jvas_1vtrace_sot_1append_1x0();
__alias__("java_sot_append1") jlong JNICALL Java_jsentry_jvas_1vtrace_sot_1append_1x1();
__alias__("java_sot_append1") jlong JNICALL Java_jsentry_jvas_1vtrace_sot_1append_1x2();
__alias__("java_sot_append1") jlong JNICALL Java_jsentry_jvas_1vtrace_sot_1append_1x3();
__alias__("java_sot_append1") jlong JNICALL Java_jsentry_jvas_1vtrace_sot_1append_1x4();
__alias__("java_sot_append1") jlong JNICALL Java_jsentry_jvas_1vtrace_sot_1append_1x5();
__alias__("java_sot_append1") jlong JNICALL Java_jsentry_jvas_1vtrace_sot_1append_1x6();
__alias__("java_sot_append1") jlong JNICALL Java_jsentry_jvas_1vtrace_sot_1append_1x7();
__alias__("java_sot_append1") jlong JNICALL Java_jsentry_jvas_1vtrace_sot_1append_1x8();
__alias__("java_sot_append1") jlong JNICALL Java_jsentry_jvas_1vtrace_sot_1append_1x9();
__alias__("java_sot_append1") jlong JNICALL Java_jsentry_jvas_1vtrace_sot_1append_1x10();


// Function: OBSOLETED
// Parameter:
//   O; sotref:= SOT reference be created by sot_append(); remove all if it is 0;
//   N; actref:= action reference
//               method will treats the last "action" has attached on current thread as the target if it is 0
// Return: none
void JNICALL java_sot_remove( JNIENV_PTR jni, jobject jcls,
							  jlong sotref,
							  jlong actref )
{
}
__alias__("java_sot_remove") void JNICALL Java_jsentry_jvas_1vtrace_sot_1remove();


// Function: (OBSOLETED; be replaced by the sot_append)
// Parameter:
//   P; evtref:= event reference
//               method will treats the last "event" has attached on current thread as the target if it is 0
//   N; response:= contents of response; array of object which represents the vt_java_response
// Return:
//   return 0 if succeed, or -1 if failed.
jint JNICALL java_fill_response( JNIENV_PTR jni, jobject jcls,
								 jlong evtref,
								 jobject resp )
{
	return -1;
}
__alias__("java_fill_response") void JNICALL Java_jsentry_jvas_1vtrace_fill_1response();


// Function:
// Parameter:
//   O; actref:= action reference; specific action only if it is valid actref, or all actions if it is 0
//   N; evtref:= event reference
//               method will treats the last "event" has attached on current thread as the target if it is 0
//   P; datseq:= array of object which represents the vt_data_sequence;
// Return:
//   array of long which represents the vt_brief_report if succeed, otherwise an empty array will be returned.
jobject JNICALL java_calc_confidence_x( JNIENV_PTR jni, jobject jcls,
									    jlong    actref,
								        jlong    evtref,
								        jobject  datseq,
										jboolean feedonly
								        )
{
	if( !g_vt_java_jvm )
	{
#ifdef VT_JAVA_DEBUG
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_WARN, "%s, wrong parameters, line.%d\n", __FUNCTION__, __LINE__ );
#endif
		return NULL;
	}

	vt_java_thread_environ *env = _java_getenv( jni );
	if( !env || !env->jtienv )
		return NULL;

	env->jnienv = jni;

#ifndef _RECURSION_DISABLE_
	long _mmphd_ = env->mmphd;
	env->mmphd = 0;
#endif

	// initiate mempool at first
	mmp_create_mempool( 1, &env->mmphd );

	vt_data_sequence datseq2 = {0};
	vt_brief_report report;

	if( vt_java_data_sequence_extract( env, datseq, &datseq2 ) == JNI_OK )
	{
#ifdef VT_JAVA_DEBUG
		vt_print_calc_confidence( stdout, actref, evtref, &datseq2 );
#endif

		if( evtref == 0 )
			evtref = env->evtref;
		if( actref == 0 )
			actref = env->actref;

		// TODO:
		if( vt_calc_confidence( (vt_refer)actref, (vt_refer)evtref, &datseq2, &report ) == JNI_OK )
		{
			;
		}
	}
#ifdef VT_JAVA_DEBUG
	else
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "%s, illegal structure \"DATA_SEQUENCE\"\n", __FUNCTION__ );
#endif

	vt_java_data_sequence_free( env, datseq, &datseq2, JNI_ABORT );

	// destroy mempool at last
	mmp_destroy_mempool( 1, &env->mmphd );

#ifndef _RECURSION_DISABLE_
	env->mmphd = _mmphd_;
#endif

	return NULL;
}

jobject JNICALL java_calc_confidence( JNIENV_PTR jni, jobject jcls,
									  jlong   actref,
								      jlong   evtref,
								      jobject datseq
								      )
{
	return java_calc_confidence_x( jni, jcls, actref, evtref, datseq, VT_FALSE );
}
__alias__("java_calc_confidence") jobject JNICALL Java_jsentry_jvas_1vtrace_calc_1confidence();


// Function:
// Parameter: same as the java_calc_confidence
// Return:
//   return 0 if succeed, or -1 if failed.
jint JNICALL java_feed_data( JNIENV_PTR jni, jobject jcls,
							 jlong   actref,
							 jlong   evtref,
							 jobject datseq
							 )
{
	return -(java_calc_confidence_x( jni, jcls, actref, evtref, datseq, VT_TRUE ) == NULL);
}
__alias__("java_feed_data") jint JNICALL Java_jsentry_jvas_1vtrace_feed_1data();


// Function:
// Parameter:
//   O; actref:= action reference; specific action only if it is valid actref, or all actions if it is 0
//   N; evtref:= event reference
//               method will treats the last "event" has attached on current thread as the target if it is 0
// Return:
//   array of long which represents the vt_brief_report if succeed, otherwise an empty array will be returned.
jobject JNICALL java_report_get( JNIENV_PTR jni, jobject jcls,
								 jlong   actref,
		 	 	 	 	 	     jlong   evtref
							     )
{
	if( sizeof(struct vt_java_brief_report) ^ sizeof(vt_brief_report) )
		return NULL;

	vt_java_thread_environ *env = _java_getenv( jni );
	if( !env || !env->jtienv )
		return NULL;

	env->jnienv = jni;

	if( evtref == 0 )
		evtref = env->evtref;
	if( actref == 0 )
		actref = env->actref;

	jobject ret_obj = JNICall( env->jnienv, NewLongArray, sizeof(struct vt_java_brief_report) / sizeof(vt_long) );
	if( !ret_obj )
		return NULL;

	vt_brief_report *rep;
	jlong *ret_val = (jlong*)JNICall( env->jnienv, GetLongArrayElements, ret_obj, NULL );
	if( !ret_val )
	{
		_VT_JAVA_DelLocRef( env->jnienv, ret_obj );
		return NULL;
	}

	// CAUTION: the vt_java_brief_report MUST is equal to the vt_brief_report
	memset( ret_val, 0, sizeof(struct vt_java_brief_report) );

	if( vt_report_get( (vt_refer)actref, (vt_refer)evtref, (rep=(vt_brief_report*)ret_val) ) == 0 )
	{
		if( rep->sot )
			rep->sot = (vt_refer)((vt_sot*)rep->sot)->a_seq;
	}

	JNICall( env->jnienv, ReleaseLongArrayElements, ret_obj, ret_val, 0 );

	return ret_obj;
}
__alias__("java_report_get") jobject JNICALL Java_jsentry_jvas_1vtrace_report_1get();


// Function: OBSOLETED
// Parameter: none
// Return:
//   array of long which represents "index of trace point" if succeed, otherwise an empty array will be returned.
jobject JNICALL java_tp_idx_copy( JNIENV_PTR jni, jobject jcls )
{
	return NULL;
}
__alias__("java_tp_idx_copy") jobject JNICALL Java_jsentry_jvas_1vtrace_tp_1idx_1copy();


// Function:
// Parameter:
// Return:
//   return 0 if succeed, or -1 if failed.
jint JNICALL java_tpix_sync( JNIENV_PTR jni, jobject jcls, jlong key, jstring jklass, jstring jname_sign )
{
	if( !g_vt_java_jvm
		|| key < 0
		|| !jklass
		|| !jname_sign
		)
		return -1;

	int r = -1;
	const char *_klass = JNICall( jni, GetStringUTFChars, jklass, NULL );
	const char *_name_sign = JNICall( jni, GetStringUTFChars, jname_sign, NULL );

	if( _klass && _name_sign )
	{
		const char *_sign = strchr( _name_sign, JVM_SIGNATURE_FUNC );
		if( _sign )
		{
			// note: the _klass layout is "class name[,interface name]"

			const char *_ifn = strchr( _klass, ',' );  // seek the interface name if it is exists
			ssize_t fz = _ifn ? _ifn - _klass : 0;  // length of the class name if _ifn is not null, otherwise 0 is set
			char *_klass2;

			ssize_t nz = _sign - _name_sign;  // length of the method name
			char *_name = malloc( nz + fz + 2 );

			if( _name )
			{
				// copy method name
				memcpy( _name, _name_sign, nz );
				_name[nz] = 0;

				if( fz > 0 )
				{
					// copy class name
					_klass2 = _name + nz + 1;
					memcpy( _klass2, _klass, fz );
					_klass2[fz] = 0;
					_ifn++;
				}
				else
					_klass2 = (char*)_klass;

				r = vt_tpix_sync( key+1, _name, _sign, _klass2, _ifn, NULL );

				//
				free( _name );
			}
		}
	}

	if( _klass )
		JNICall( jni, ReleaseStringUTFChars, jklass, _klass );
	if( _name_sign )
		JNICall( jni, ReleaseStringUTFChars, jname_sign, _name_sign );

	return r;
}
__alias__("java_tpix_sync") jint JNICALL Java_jsentry_jvas_1vtrace_tpix_1sync();


//
char* vt_java_decl_split( const char *decl_name, char **sign, char **buf, const char *klass_ident )
{
	char *c = strchr( decl_name, JVM_SIGNATURE_FUNC );
	if( !c )
		return NULL;

	ssize_t d = c - decl_name;
	ssize_t n = -(klass_ident && strncmp(klass_ident, decl_name, d) == 0 && klass_ident[d] == 0) & 6;

	char *name = *buf;

	if( !name )
	{
		name = malloc(strlen(decl_name) + 2 + (n - d & -(n!=0)) );
		if( !name )
			return NULL;
		*buf = name;
	}

	if( n )
		strcpy( name, "<init>" );
	else
		strncpy( name, decl_name, d ), name[n=d] = 0;

	*sign = name + n + 1; // leave a terminal '\0'
	strcpy( *sign, decl_name+d );
	return name;
}

//
int vt_java_decl_check( char *name, int len )
{
	// only, convert level separator
	for( --len; len >= 0; --len )
	{
		if( name[len] == '.' )
			name[len] = '/';
	}

	return len;
}

//
int vt_java_1sign_norm( const char *token, int len, char *norm )
{
	const vt_dict *dic;
	char *cc, *cc2;
	int n, newlen = 0;

	// does it an array
	if( (cc2 = strchr( token, JVM_SIGNATURE_ARRAY )) )
	{
		// convert array representation( e.g, XXX[][] => [[XXX )
		*cc2 = 0;  // must be terminated by '\0'
		cc = cc2;
		do
		{
			norm[newlen++] = JVM_SIGNATURE_ARRAY;
			cc++;
		} while( (cc=strchr(cc, JVM_SIGNATURE_ARRAY)) );
	}

	dic = vt_dict_match_key2( g_vt_java_PrimitiveType,
							  sizeof(g_vt_java_PrimitiveType)/sizeof(vt_dict),
							  token );
	if( dic )
	{
		// primitive
		norm[newlen++] = dic->val.c;
	}
	else
	{
		// object
		n = cc2 ? cc2 - token : len;
		norm[newlen++] = JVM_SIGNATURE_CLASS;
		memcpy( &norm[newlen], token, n );
		newlen += n;
		norm[newlen++] = JVM_SIGNATURE_ENDCLASS;
	}
	norm[newlen] = 0;

	if( cc2 )
		*cc2 = JVM_SIGNATURE_ARRAY;  // restore

	return newlen;
}

// for example: field(java.lang.String[]) => field([Ljava.lang.String;)
int vt_java_decl_norm( const char *decl, int len, char **dst, int *dstsize )
{
	char *name, *retval, *token, *nextptr, *norm;  // *cc, *cc2
	static const char *sepchr = " ,\t\r\n";
	//const vt_dict *dic;
	int n, newlen, token_num;

	name = strchr( decl, JVM_SIGNATURE_FUNC );
	retval = strchr( decl, JVM_SIGNATURE_ENDFUNC );
	if( !name || !retval )
		return -1;

	// count number of parameter
	for( token=name+1, n=0; (token=strchr(token, ',')); token++, n++ );
	token_num = n + 1;
	n += len + 8;

	norm = *dst;
	if( n > *dstsize )
	{
		norm = realloc( norm, n );  // allocate buffer with possible maximum size
		if( !norm )
			return -1;
		*dstsize = n;
		*dst = norm;
	}

	newlen = name - decl + 1;
	memcpy( norm, decl, newlen );

	*retval = 0;
	token = strtok_r( name+1, sepchr, &nextptr );
	while( token && --token_num >= 0 )
	{
#if 0
		// does it an array
		if( (cc2 = strchr( token, JVM_SIGNATURE_ARRAY )) )
		{
			// convert array representation( e.g, XXX[][] => [[XXX )
			*cc2 = 0;  // must be terminated by '\0'
			cc = cc2;
			do
			{
				norm[newlen++] = JVM_SIGNATURE_ARRAY;
				cc++;
			} while( (cc=strchr(cc, JVM_SIGNATURE_ARRAY)) );
		}

		dic = vt_dict_match_key2( g_vt_java_PrimitiveType,
								  sizeof(g_vt_java_PrimitiveType)/sizeof(vt_dict),
								  token );
		if( dic )
		{
			// primitive
			norm[newlen++] = dic->val.c;
		}
		else
		{
			// object
			n = cc2 ? cc2 - token : nextptr - token - 1;  // strlen( token );
			norm[newlen++] = JVM_SIGNATURE_CLASS;
			memcpy( &norm[newlen], token, n );
			norm[newlen+=n] = JVM_SIGNATURE_ENDCLASS;
			newlen++;
		}

		if( cc2 )
			*cc2 = JVM_SIGNATURE_ARRAY;  // restore
#endif

		newlen += vt_java_1sign_norm( token, nextptr-token-(nextptr[0]!=0), &norm[newlen] );
		token = strtok_r( NULL, sepchr, &nextptr );
	}

	*retval = JVM_SIGNATURE_ENDFUNC;
	strcpy( norm+newlen, retval );
	newlen += &decl[len] - retval;  // strlen( retval );

	return newlen;
}


//
vt_typeid vt_java_type_convert( const char *sign, vt_byte *ndim )
{
	/*if( *sign == 0 )
	{
		*ndim = 0;
		return VT_TYPE_NULL;
	}*/

	int n;
	const char *primitive_sign = "VBCSIJFDZ";
	const int primitive_type[] = {VT_TYPE_NULL, VT_TYPE_BYTE, VT_TYPE_CHAR,
			VT_TYPE_SHORT, VT_TYPE_INT, VT_TYPE_LONG, VT_TYPE_FLOAT, VT_TYPE_DOUBLE, VT_TYPE_BOOL };
	const char *cc;

	// count array dimenstions
	for( n=0; *sign == JVM_SIGNATURE_ARRAY; sign++, n++ );
	*ndim = n & 0xff;

	switch( *sign )
	{
	case JVM_SIGNATURE_CLASS:
	case JVM_SIGNATURE_ENUM:
		cc = strchr( sign, JVM_SIGNATURE_ENDCLASS );
		if( cc )
			return VT_TYPE_CLASS;
		break;
	case 0:
		break;
	default:
		cc = strchr( primitive_sign, *sign );
		if( cc )
			return primitive_type[cc - primitive_sign];
		break;
	}

	/*const vt_dict *dic = vt_dict_match_key2( g_vt_java_DataType,
							  sizeof(g_vt_java_DataType)/sizeof(vt_dict),
							  sign );
	return dic ? dic->val.i : VT_TYPE_CLASS;
	*/

	return VT_TYPE_NULL;
}


//
int vt_java_get_1sign( const char *decl_str, int idx, vt_byte *ndim, int *sign_len )
{
	// ()
	// ()Y
	// (XXX)
	// (XXX)Y
	int n;
	const char *primitive_sign = "VBCSIJFDZ";
	const int primitive_type[] = {VT_TYPE_NULL, VT_TYPE_BYTE, VT_TYPE_CHAR,
			VT_TYPE_SHORT, VT_TYPE_INT, VT_TYPE_LONG, VT_TYPE_FLOAT, VT_TYPE_DOUBLE, VT_TYPE_BOOL };
	const char *cc;
	//const char *decl = decl_str + (*decl_str == JVM_SIGNATURE_FUNC || *decl_str == JVM_SIGNATURE_ENDFUNC);
	const char *decl = decl_str + ((idx == 1 && *decl_str == JVM_SIGNATURE_FUNC) || (idx == -1 && *decl_str == JVM_SIGNATURE_ENDFUNC));

	// count array dimenstions
	for( n=0; *decl == JVM_SIGNATURE_ARRAY; decl++, n++ );
	if( ndim )
		*ndim = n & 0xff;

	switch( *decl )
	{
	case JVM_SIGNATURE_CLASS:
	case JVM_SIGNATURE_ENUM:
		cc = strchr( decl, JVM_SIGNATURE_ENDCLASS );
		if( cc )
		{
			*sign_len = cc - decl_str + 1;
			return VT_TYPE_CLASS;
		}
		break;
	case 0:
		*sign_len = decl - decl_str;
		return -2;  // represents "END"
	case JVM_SIGNATURE_FUNC:
		if( idx == 0 )
		{
			*sign_len = 1;
			return -3; // represents "HEAD"
		}
		break;
	case JVM_SIGNATURE_ENDFUNC:
		if( *decl_str == JVM_SIGNATURE_FUNC )
		{
			*sign_len = 2;  // nothing in pair "()"
			return VT_TYPE_NULL;
		}
		break;
	default:
		cc = strchr( primitive_sign, *decl );
		if( cc )
		{
			*sign_len = decl - decl_str + 1;
			return primitive_type[cc - primitive_sign];
		}
		break;
	}
	*sign_len = 0;
	return -1;
}


//
jint vt_java_objcmp( jobject obj1, jobject obj2 )
{
	vt_java_thread_environ *env;

	if( g_vt_java_jvm )
	{
		env = _java_getenv( NULL );
		if( env && env->jnienv )
			return JNICall( env->jnienv, IsSameObject, obj1, obj2 ) - 1;
	}
	return -1;
}


//
jobject vt_java_globalref_new( jobject obj )
{
	vt_java_thread_environ *env;

	if( g_vt_java_jvm )
	{
		env = _java_getenv( NULL );
		if( env && env->jnienv )
			return JNICall( env->jnienv, NewGlobalRef, obj );
	}
	return NULL;
}

//
void vt_java_globalref_del( jobject obj )
{
	vt_java_thread_environ *env;

	if( g_vt_java_jvm )
	{
		env = _java_getenv( NULL );
		if( env && env->jnienv )
			JNICall( env->jnienv, DeleteGlobalRef, obj );
	}
}

//
_VT_JAVA_INLINE_ VT_LOCAL
jint _java_fill_callframe( vt_java_thread_environ *env, vt_call_frame *frames, const char *name, const char *sign, long *mmphd )
{
	int name_len;

	frames[0].path_len = (name_len=name?strlen(name):0) + (sign?strlen(sign):0);
	frames[0].full_path = mmp_malloc( 1, mmphd, frames[0].path_len+1 );
	if( !frames[0].full_path )
	{
		if( name )
			JTI_Deallocate( env->jtienv, name );
		if( sign )
			JTI_Deallocate( env->jtienv, sign );
		return -1;
	}

	frames[0].name_off = 0;
	frames[0].decl_off = name_len;
	if( name )
	{
		strcpy( frames[0].full_path, name );
		JTI_Deallocate( env->jtienv, name );
	}

	if( sign )
	{
		strcpy( frames[0].full_path+name_len, sign );
		JTI_Deallocate( env->jtienv, sign );
	}
	return 0;
}

//
jint vt_java_callframe_fill( vt_call_frame *frames, jint count, long *mmphd )
{
	jint i = 0;
	char *name, *sign;
	vt_java_thread_environ *env;

	if( g_vt_java_jvm )
	{
		env = _java_getenv( NULL );
		if( env && env->jnienv && env->jtienv )
		{
			for( ; i < count; i++ )
			{
				if( frames[i].mtd_ref && frames[i].path_len == 0 )
				{
					if( JNICall( env->jtienv, GetMethodName, frames[i].mtd_ref, &name, &sign, NULL ) == JVMTI_ERROR_NONE && (name || sign) )
					{
						if( _java_fill_callframe( env, &frames[i], name, sign, mmphd ) == -1 )
							break;
					}
					else
					{
						frames[i].full_path = NULL;
						frames[i].path_len = 0;
					}
				}
			}
		}
	}

	return i;
}

//
jint vt_java_callstack_copy( jthread thread, jint begin, jint max, vt_call_frame **dst, jboolean onlyref, long *mmphd )
{
	char *name, *sign;
	vt_call_frame *dst2;
	vt_java_thread_environ *env;
	jvmtiFrameInfo frame[64], *frminf = frame;
	jint i, count;

	if( !g_vt_java_jvm || (*dst && max <= -1) )
		return -1;

	env = _java_getenv( NULL );
	if( !env || !env->jnienv || !env->jtienv )
		return -1;

	count = -1;
	dst2 = *dst;

	if( !dst2 || max <= -1 )
	{
		if( JNICall( env->jtienv, GetFrameCount, thread, &max ) != JVMTI_ERROR_NONE || max <= -1 )
			goto LAB_EXIT;
	}

	if( begin >= max || -begin > max )
		goto LAB_EXIT;

	if( max > sizeof(frame)/sizeof(jvmtiFrameInfo) )
	{
		frminf = (jvmtiFrameInfo*)malloc( sizeof(jvmtiFrameInfo) * max );
		if( !frminf )
			goto LAB_EXIT;
	}

	if( !dst2 )
	{
		max = begin < 0 ? -begin : max - begin;
		dst2 = (vt_call_frame*)mmp_malloc( 1, mmphd, sizeof(vt_call_frame) * max );
		if( !dst2 )
			goto LAB_EXIT;
	}

	// CAUTION: assume size of the structure "vt_call_frame" is larger than the "jvmtiFrameInfo"
	if( JNICall( env->jtienv, GetStackTrace, thread, begin, max, frminf, &count ) == JVMTI_ERROR_NONE )
	{
		if( onlyref )
		{
			// only reference
			for( i=0; i < count; i++ )
			{
				dst2[i].pos = frminf[i].location;
				dst2[i].mtd_ref = frminf[i].method;  // DO NOT need new reference for method
				dst2[i].path_len = 0;
			}
		}
		else
		{
			// fill contents
			for( i=0; i < count; i++ )
			{
				if( JNICall( env->jtienv, GetMethodName, frminf[i].method, &name, &sign, NULL ) == JVMTI_ERROR_NONE && (name || sign) )
				{
					if( _java_fill_callframe( env, &dst2[i], name, sign, mmphd ) == -1 )
						break;
				}
				else
				{
					dst2[i].full_path = NULL;
					dst2[i].path_len = 0;
				}
				dst2[i].mtd_ref = frminf[i].method;  // DO NOT need new reference for method
				dst2[i].pos = frminf[i].location;
			}
			count = i;
		}

		if( *dst != dst2 )
		{
			if( count > 0 )
				*dst = dst2;
			else
				mmp_free( 1, mmphd, dst2 );
		}
	}

	LAB_EXIT:
	if( frminf != frame )
		free( frminf );

	return count;
}

//
jint vt_java_get_callframe( jthread thread, jint depth, vt_call_frame *dst )
{
	vt_java_thread_environ *env;
	jvmtiFrameInfo frame;

	if( g_vt_java_jvm )
	{
		env = _java_getenv( NULL );
		if( env && env->jnienv && env->jtienv )
		{
			if( _java_get_callframe( env, thread, depth, &frame ) == JVMTI_ERROR_NONE )
			{
				dst->pos = frame.location;
				dst->mtd_ref = frame.method;  // DO NOT need new reference for method
				dst->path_len = 0;
				return 0;
			}
		}
	}
	return -1;
}

//
jlong vt_java_map_object( jobject obj )
{
	vt_java_thread_environ *env;
	if( g_vt_java_jvm )
	{
		env = _java_getenv( NULL );
		return _java_object_map( env, obj );
	}
	return -1;
}

//
jint vt_java_bind_object( jobject obj, jlong tag )
{
	vt_java_thread_environ *env;
	if( g_vt_java_jvm )
	{
		env = _java_getenv( NULL );
		if( env && env->jtienv )
			return -(JNICall( env->jtienv, SetTag, obj, tag ) != JVMTI_ERROR_NONE);
	}
	return -1;
}

//
void vt_java_action_exit( vt_refer actref, vt_refer arg )
{
	_java_action_exit( actref, arg );
}

#endif

