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

#include "sabi.h"
#include "jvas_swi.h"
#include "jtievt.h"
#include "vtrace.h"

#include "sys/taskLib.h"
#include "common/error_user.h"
#include "facility/xmlAnalyzer.h"

#ifdef _LINUX
#include "linux/sem_base.h"
#include "linux/clock_spec.h"
#endif


// ========================================================================================================================================
// Local and/or no-Export Global Variables
// ========================================================================================================================================

// essential methods or fields of jvas_swi
JVAS_HIDDEN const struct jvas_method_desc g_jvas_swi_method_desc[] = {
		{0},	/* invalid */
		{.name="begin", .sign="(JLjava/lang/String;Ljava/lang/String;)I", .must=1},	/* _JVAS_SWI_IDX_Begin */

		{.name="end", .sign="()V", .must=1},			/* _JVAS_SWI_IDX_End */

		{.name="setmode", .sign="(I)I", .must=1},		/* _JVAS_SWI_IDX_SetMode */

		{.name="getstatus", .sign="()J", .must=1},		/* _JVAS_SWI_IDX_GetStatus */

		{.name="byteValue", .sign="(Ljava/lang/Byte;)B", .must=1},		/* _JVAS_SWI_IDX_byteValue */
		{.name="shortValue", .sign="(Ljava/lang/Short;)S", .must=1},		/* _JVAS_SWI_IDX_shortValue */
		{.name="intValue", .sign="(Ljava/lang/Integer;)I", .must=1},			/* _JVAS_SWI_IDX_intValue */
		{.name="longValue", .sign="(Ljava/lang/Long;)J", .must=1},		/* _JVAS_SWI_IDX_longValue */
		{.name="floatValue", .sign="(Ljava/lang/Float;)F", .must=1},		/* _JVAS_SWI_IDX_floatValue */
		{.name="doubleValue", .sign="(Ljava/lang/Double;)D", .must=1},		/* _JVAS_SWI_IDX_doubleValue */

		{.name="vtrace_ask_feed", .sign="(JJLjava/lang/Object;)I", .must=0},/* _JVAS_SWI_IDX_vtrace_ask_feed */
};
#define _JVAS_SWI_METHOD_COUNT		( sizeof(g_jvas_swi_method_desc)/sizeof(struct jvas_method_desc) )

#if 0
//
JVAS_HIDDEN const struct jvas_method_desc g_jvas_swi_number_method[] = {
		{.name="byteValue", .sign="()B", .must=1},		/* _JVAS_SWI_IDX_byteValue */
		{.name="shortValue", .sign="()S", .must=1},		/* _JVAS_SWI_IDX_shortValue */
		{.name="intValue", .sign="()I", .must=1},			/* _JVAS_SWI_IDX_intValue */
		{.name="longValue", .sign="()J", .must=1},		/* _JVAS_SWI_IDX_longValue */
		{.name="floatValue", .sign="()F", .must=1},		/* _JVAS_SWI_IDX_floatValue */
		{.name="doubleValue", .sign="()D", .must=1},		/* _JVAS_SWI_IDX_doubleValue */
};

//
JVAS_HIDDEN jmethodID g_jvas_swi_number_mtdID[6] = {
		0, 0, 0, 0, 0, 0
};
#endif

// sentry agent configures
extern JVAS_Conf g_jvas_conf;

//JVAS_LOCAL SEM_ID g_jvas_lock_bind_method = NULL;



//
JVAS_HIDDEN int jvas_swi_initiate( JNIENV_PTR jnienv )
{
#if 0
	jclass jcls = JNICall( jnienv, FindClass, "java/lang/Number" );
	if( jcls )
	{
		int i;
		for( i=sizeof(g_jvas_swi_number_mtdID)/sizeof(jmethodID)-1; i >= 0; i-- )
		{
			g_jvas_swi_number_mtdID[i] = JNICall( jnienv, GetMethodID, jcls,
					g_jvas_swi_number_method[i].name, g_jvas_swi_number_method[i].sign );
			if( !g_jvas_swi_number_mtdID[i] )
				break;
		}
		if( i >= 0 )
			return -1;
	}
#endif

	return 0;
}

//
JVAS_HIDDEN void jvas_swi_release( JNIENV_PTR jnienv )
{

}

//extern JTIENV_PTR g_jvas_def_jtienv;

//
JVAS_HIDDEN jmethodID jvas_swi_get_static_method( JNIENV_PTR jnienv, int cls_idx, int mtd_idx, BOOL bound )
{
	jmethodID jmtd = NULL;
	struct jvas_class_conf *clsptr = &g_jvas_conf.esse_class.classes[cls_idx];

	JNICall( jnienv, ExceptionClear );

	jmtd = JNICall( jnienv, GetStaticMethodID,
				   clsptr->handle,
				   g_jvas_swi_method_desc[mtd_idx].name,
				   g_jvas_swi_method_desc[mtd_idx].sign );

	if( jmtd && JNICall(jnienv, ExceptionCheck) == FALSE )
	{
		if( bound )
		{
			// jmethodID and jfieldID does not need be managed

			clsptr->method_table[mtd_idx].jmtd = jmtd;
			//clsptr->method_table[mtd_idx].desc = &g_jvas_swi_method_desc[mtd_idx];
			//clsptr->method_table[mtd_idx].klass = clsptr;

			// FOR DEBUG
			/*jlong tag = 0;
			jvmtiError er;
			er = JNICall( g_jvas_def_jtienv, SetTag, (jobject)jmtd, (jlong)jmtd );
			er = JNICall( g_jvas_def_jtienv, GetTag, (jobject)jmtd, &tag );
			printf( "%d, %ld\n", er, tag );
			*/

#ifdef _JVAS_DEVELOP_
			_JVAS_SWI_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "%s%s => %p\n",
					g_jvas_swi_method_desc[mtd_idx].name, g_jvas_swi_method_desc[mtd_idx].sign, clsptr->method_table[mtd_idx].jmtd );// FOR DEBUG
#endif
		}
	}
	else
		jmtd = NULL;

	return jmtd;
}

//
JVAS_HIDDEN int jvas_swi_bind_methods( JNIENV_PTR jnienv, int cls_idx )
{
	if( cls_idx < 0 || cls_idx >= g_jvas_conf.esse_class.count )
	{
		errno = EINVAL;
		return -1;
	}

	struct jvas_class_conf *clsptr = &g_jvas_conf.esse_class.classes[cls_idx];

	clsptr->method_table = (struct jvas_class_method*)malloc( sizeof(struct jvas_class_method) * _JVAS_SWI_METHOD_COUNT );
	if( !clsptr->method_table )
		return -1;

	memset( clsptr->method_table, 0, sizeof(struct jvas_class_method) * _JVAS_SWI_METHOD_COUNT );

	int i;
	for( i=1; i < _JVAS_SWI_METHOD_COUNT; i++ )
	{
		clsptr->method_table[i].desc = &g_jvas_swi_method_desc[i];
		clsptr->method_table[i].klass = clsptr;

		if( !jvas_swi_get_static_method(jnienv, cls_idx, i, TRUE ) )
		{
			if( JNICall(jnienv, ExceptionCheck) )
			{
				// TODO:
				JNICall( jnienv, ExceptionDescribe );
			}
			else
			{
#ifdef _JVAS_DEVELOP_
				_JVAS_SWI_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "no static method \"%s%s\" in the class \"%s\"",
						g_jvas_swi_method_desc[i].name, g_jvas_swi_method_desc[i].sign, clsptr->name );// FOR DEBUG
#endif
			}

			if( g_jvas_swi_method_desc[i].must )
				break;
		}
	}

	if( i < _JVAS_SWI_METHOD_COUNT )
	{
		free( clsptr->method_table );
		clsptr->method_table = NULL;
		clsptr->method_count = 0;
		return -1;
	}

	clsptr->method_count = _JVAS_SWI_METHOD_COUNT;
	return 0;
}

//
JVAS_HIDDEN void jvas_swi_unbind_methods( JNIENV_PTR jnienv, int cls_idx )
{
	if( cls_idx < 0 || cls_idx >= g_jvas_conf.esse_class.count )
		return;

	struct jvas_class_conf *clsptr = &g_jvas_conf.esse_class.classes[cls_idx];

	if( clsptr->method_table && clsptr->method_count > 0 )
	{
		/* jmethodID and jfieldID is not need be managed
		int i;
		for( i=clsptr->method_count-1; i > 0; i-- )
		{
			if( clsptr->method_table[i].jmtd )
				JNICall( jnienv, DeleteGlobalRef, (jobject)clsptr->method_table[i].jmtd );
		}*/
		free( clsptr->method_table );
	}

	clsptr->method_table = NULL;
	clsptr->method_count = 0;
}


//
inline JVAS_LOCAL jint jvas_swi_CallSwitchMethodA( JNIENV_PTR jnienv, struct jvas_class_conf *clsptr, int mtd_idx, jvalue *args )
{
	JNICall( jnienv, ExceptionClear );
	jint r = JNICall( jnienv, CallStaticIntMethodA, clsptr->handle,
					 clsptr->method_table[mtd_idx].jmtd, args );
	if( JNICall(jnienv, ExceptionCheck) == FALSE )
		return r;

#ifdef _DEBUG
	if( JNICall(jnienv, ExceptionCheck) )
		JNICall( jnienv, ExceptionDescribe );
#endif
	return -1;
}

//
inline JVAS_LOCAL jint jvas_swi_CallIntMethodA( JNIENV_PTR jnienv, struct jvas_class_conf *clsptr, int mtd_idx, jvalue *args, jvalue *retval )
{
	//JNICall( jnienv, ExceptionClear );
	retval->i = JNICall( jnienv, CallStaticIntMethodA, clsptr->handle,
					 clsptr->method_table[mtd_idx].jmtd, args );
	//if( JNICall(jnienv, ExceptionCheck) == FALSE )
		return JNI_OK;

#ifdef _DEBUG
	if( JNICall(jnienv, ExceptionCheck) )
		JNICall( jnienv, ExceptionDescribe );
#endif
	return JNI_ERR;
}

//
inline JVAS_LOCAL jint jvas_swi_CallLongMethodA( JNIENV_PTR jnienv, struct jvas_class_conf *clsptr, int mtd_idx, jvalue *args, jvalue *retval )
{
	//JNICall( jnienv, ExceptionClear );
	retval->j = JNICall( jnienv, CallStaticLongMethodA, clsptr->handle,
					 clsptr->method_table[mtd_idx].jmtd, args );
	//if( JNICall(jnienv, ExceptionCheck) == FALSE )
		return JNI_OK;

#ifdef _DEBUG
	if( JNICall(jnienv, ExceptionCheck) )
		JNICall( jnienv, ExceptionDescribe );
#endif
	return JNI_ERR;
}

//
inline JVAS_LOCAL jint jvas_swi_CallShortMethodA( JNIENV_PTR jnienv, struct jvas_class_conf *clsptr, int mtd_idx, jvalue *args, jvalue *retval )
{
	//JNICall( jnienv, ExceptionClear );
	retval->s = JNICall( jnienv, CallStaticShortMethodA, clsptr->handle,
					 clsptr->method_table[mtd_idx].jmtd, args );
	//if( JNICall(jnienv, ExceptionCheck) == FALSE )
		return JNI_OK;

#ifdef _DEBUG
	if( JNICall(jnienv, ExceptionCheck) )
		JNICall( jnienv, ExceptionDescribe );
#endif
	return JNI_ERR;
}

//
inline JVAS_LOCAL jint jvas_swi_CallByteMethodA( JNIENV_PTR jnienv, struct jvas_class_conf *clsptr, int mtd_idx, jvalue *args, jvalue *retval )
{
	//JNICall( jnienv, ExceptionClear );
	retval->b = JNICall( jnienv, CallStaticLongMethodA, clsptr->handle,
					 clsptr->method_table[mtd_idx].jmtd, args );
	//if( JNICall(jnienv, ExceptionCheck) == FALSE )
		return JNI_OK;

#ifdef _DEBUG
	if( JNICall(jnienv, ExceptionCheck) )
		JNICall( jnienv, ExceptionDescribe );
#endif
	return JNI_ERR;
}

//
inline JVAS_LOCAL jint jvas_swi_CallFloatMethodA( JNIENV_PTR jnienv, struct jvas_class_conf *clsptr, int mtd_idx, jvalue *args, jvalue *retval )
{
	//JNICall( jnienv, ExceptionClear );
	retval->f = JNICall( jnienv, CallStaticFloatMethodA, clsptr->handle,
					 clsptr->method_table[mtd_idx].jmtd, args );
	//if( JNICall(jnienv, ExceptionCheck) == FALSE )
		return JNI_OK;

#ifdef _DEBUG
	if( JNICall(jnienv, ExceptionCheck) )
		JNICall( jnienv, ExceptionDescribe );
#endif
	return JNI_ERR;
}

//
inline JVAS_LOCAL jint jvas_swi_CallDoubleMethodA( JNIENV_PTR jnienv, struct jvas_class_conf *clsptr, int mtd_idx, jvalue *args, jvalue *retval )
{
	//JNICall( jnienv, ExceptionClear );
	retval->d = JNICall( jnienv, CallStaticDoubleMethodA, clsptr->handle,
					 clsptr->method_table[mtd_idx].jmtd, args );
	//if( JNICall(jnienv, ExceptionCheck) == FALSE )
		return JNI_OK;

#ifdef _DEBUG
	if( JNICall(jnienv, ExceptionCheck) )
		JNICall( jnienv, ExceptionDescribe );
#endif
	return JNI_ERR;
}

//
inline JVAS_LOCAL jint jvas_swi_CallNull( JNIENV_PTR jnienv, struct jvas_class_conf *clsptr, int mtd_idx, jvalue *args, jvalue *retval )
{
	return JNI_ERR;
}


//
JVAS_HIDDEN jint jvas_swi_Begin( JNIENV_PTR jnienv, struct jvas_class_conf *clsptr, jlong reserved, const char *libpath, const char *wkdir )
{
	//jvalue args = {.j=reserved};
	//return clsptr->handle && clsptr->method_table[_JVAS_SWI_IDX_Begin].jmtd
	//		? jvas_swi_CallSwitchMethodA( jnienv, clsptr, _JVAS_SWI_IDX_Begin, &args )
	//		: -1;

	if( clsptr->handle && clsptr->method_table[_JVAS_SWI_IDX_Begin].jmtd )
	{
		jvalue args[3] = {{.j=reserved}};

		args[1].l = JNICall( jnienv, NewStringUTF, libpath?libpath:"" );
		args[2].l = JNICall( jnienv, NewStringUTF, wkdir?wkdir:"" );

		if( args[1].l && args[2].l )
		{
			//jsize ln = JNICall( jnienv, GetStringUTFLength, args[1].l );
			//printf( "***************       %d\n", ln );

			args[0].i = jvas_swi_CallSwitchMethodA( jnienv, clsptr, _JVAS_SWI_IDX_Begin, args );

			JNICall( jnienv, DeleteLocalRef, args[1].l );
			JNICall( jnienv, DeleteLocalRef, args[2].l );

			return args[0].i;
		}
	}
	return -1;
}


//
JVAS_HIDDEN void jvas_swi_End( JNIENV_PTR jnienv, struct jvas_class_conf *clsptr )
{
	if( clsptr->handle && clsptr->method_table[_JVAS_SWI_IDX_End].jmtd )
		jvas_swi_CallSwitchMethodA( jnienv, clsptr, _JVAS_SWI_IDX_End, NULL );
}


//
JVAS_HIDDEN jint jvas_swi_SetMode( JNIENV_PTR jnienv, struct jvas_class_conf *clsptr, jint mode )
{
	jvalue args = {.i=mode};
	return clsptr->handle && clsptr->method_table[_JVAS_SWI_IDX_SetMode].jmtd
			? jvas_swi_CallSwitchMethodA( jnienv, clsptr, _JVAS_SWI_IDX_SetMode, &args )
			: -1;
}


//
JVAS_HIDDEN jlong jvas_swi_GetStatus( JNIENV_PTR jnienv, struct jvas_class_conf *clsptr )
{
	return clsptr->handle && clsptr->method_table[_JVAS_SWI_IDX_GetStatus].jmtd
			? jvas_swi_CallSwitchMethodA( jnienv, clsptr, _JVAS_SWI_IDX_GetStatus, NULL )
			: -1;
}


//
typedef jint (*_CallXXXMethodA_)( JNIENV_PTR, struct jvas_class_conf*, int, jvalue*, jvalue* );

JVAS_LOCAL _CallXXXMethodA_    FunTab_CallXXXMethodA[] = {
		/* setup according to the vt_typeid */
		jvas_swi_CallNull,
		jvas_swi_CallByteMethodA,
		jvas_swi_CallShortMethodA,
		jvas_swi_CallIntMethodA,
		jvas_swi_CallLongMethodA,
		jvas_swi_CallNull,
		jvas_swi_CallByteMethodA,
		jvas_swi_CallShortMethodA,
		jvas_swi_CallIntMethodA,
		jvas_swi_CallLongMethodA,
		jvas_swi_CallNull,
		jvas_swi_CallFloatMethodA,
		jvas_swi_CallDoubleMethodA,
		jvas_swi_CallNull,
		jvas_swi_CallLongMethodA,
		jvas_swi_CallLongMethodA
};

//
JVAS_HIDDEN jint jvas_swi_NumberValue( JNIENV_PTR jnienv, struct jvas_class_conf *clsptr, int dtype, jobject obj, jvalue *value )
{
	//jvalue args = {.l=obj };

#if 0  // slower than FunTab_CallXXXMethodA ???
	if( g_jvas_swi_number_mtdID[0] )
	{
		//JNICall( jnienv, ExceptionClear );

		switch( dtype )
		{
		case VT_TYPE_SI8:
		case VT_TYPE_UI8:
			value->b = JNICall( jnienv, CallByteMethodA, obj,
								g_jvas_swi_number_mtdID[_JVAS_SWI_IDX_byteValue-_JVAS_SWI_IDX_byteValue], (jvalue*)&obj );
			break;
		case VT_TYPE_SI16:
		case VT_TYPE_UI16:
			value->s = JNICall( jnienv, CallShortMethodA, obj,
								g_jvas_swi_number_mtdID[_JVAS_SWI_IDX_shortValue-_JVAS_SWI_IDX_byteValue], (jvalue*)&obj );
			break;
		case VT_TYPE_SI32:
		case VT_TYPE_UI32:
			value->i = JNICall( jnienv, CallIntMethodA, obj,
								g_jvas_swi_number_mtdID[_JVAS_SWI_IDX_intValue-_JVAS_SWI_IDX_byteValue], (jvalue*)&obj );
			break;
		case VT_TYPE_SI64:
		case VT_TYPE_UI64:
		case VT_TYPE_POINTER:
		case VT_TYPE_TIME:
			value->j = JNICall( jnienv, CallLongMethodA, obj,
								g_jvas_swi_number_mtdID[_JVAS_SWI_IDX_longValue-_JVAS_SWI_IDX_byteValue], (jvalue*)&obj );
			break;
		case VT_TYPE_F32:
			value->f = JNICall( jnienv, CallFloatMethodA, obj,
								g_jvas_swi_number_mtdID[_JVAS_SWI_IDX_floatValue-_JVAS_SWI_IDX_byteValue], (jvalue*)&obj );
			break;
		case VT_TYPE_F64:
			value->d = JNICall( jnienv, CallDoubleMethodA, obj,
								g_jvas_swi_number_mtdID[_JVAS_SWI_IDX_doubleValue-_JVAS_SWI_IDX_byteValue], (jvalue*)&obj );
			break;
		default:
			return JNI_ERR;
		}

		//if( JNICall(jnienv, ExceptionCheck) )
		//	return JNI_ERR;
		return JNI_OK;
	}
#endif

	//
	if( dtype >= VT_TYPE_SI8 && dtype <= VT_TYPE_F64 )
	{
		return FunTab_CallXXXMethodA[dtype]( jnienv, clsptr,
				(dtype%5 + 5*(dtype>=VT_TYPE_F32) - 1 + _JVAS_SWI_IDX_byteValue),
				(jvalue*)&obj /*&args*/, value );
	}
	else if( dtype == VT_TYPE_POINTER || dtype == VT_TYPE_TIME )
	{
		return jvas_swi_CallLongMethodA( jnienv, clsptr, _JVAS_SWI_IDX_longValue, (jvalue*)&obj /*&args*/, value );
	}

	return JNI_ERR;
}

//
JVAS_HIDDEN jint jvas_swi_vtrace_ask_feed( JNIENV_PTR jnienv, struct jvas_class_conf *clsptr, jlong actref, jlong evtref, jobject extarg )
{
	jvalue args[3] = {{.j=actref}, {.j=evtref}, {.l=extarg}};
	return clsptr->handle && clsptr->method_table[_JVAS_SWI_IDX_vtrace_ask_feed].jmtd
			? jvas_swi_CallSwitchMethodA( jnienv, clsptr, _JVAS_SWI_IDX_vtrace_ask_feed, args )
			: -1;
}
