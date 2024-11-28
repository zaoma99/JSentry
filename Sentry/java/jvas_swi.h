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

#ifndef _JVAS_SWI_H_
#define _JVAS_SWI_H_

#include "jvas.h"


#define _JVAS_SWI_DEBUG_PRINT_	_JVAS_DEBUG_PRINT_
//#define _JVAS_SWI_DEBUG_PRINT_( _FD_, _LOG_TYPE_, _FORMAT_, _ARGS_... )    _DEBUG_PRINT_( _FD_, _LOG_TYPE_, "jvas_swi:"_FORMAT_, ##_ARGS_ )


// stub: begin
#define _JVAS_SWI_IDX_Begin		1
typedef jint (*JVAS_SWI_Begin)( JNIENV_PTR, jlong, jobject, jobject );

// stub: end
#define _JVAS_SWI_IDX_End		2
typedef void (*JVAS_SWI_End)( JNIENV_PTR );

// stub: set mode
#define _JVAS_SWI_IDX_SetMode	3
typedef jint (*JVAS_SWI_SetMode)( JNIENV_PTR, jint );

// stub: get status
#define _JVAS_SWI_IDX_GetStatus	4
typedef jint (*JVAS_SWI_GetStatus)( JNIENV_PTR );

// func: assistant functions
#define _JVAS_SWI_IDX_byteValue		5
typedef jbyte (*JVAS_SWI_byteValue)( JNIENV_PTR, jobject );

#define _JVAS_SWI_IDX_shortValue	6
typedef jshort (*JVAS_SWI_shortValue)( JNIENV_PTR, jobject );

#define _JVAS_SWI_IDX_intValue		7
typedef jint (*JVAS_SWI_intValue)( JNIENV_PTR, jobject );

#define _JVAS_SWI_IDX_longValue		8
typedef jlong (*JVAS_SWI_longValue)( JNIENV_PTR, jobject );

#define _JVAS_SWI_IDX_floatValue	9
typedef jfloat (*JVAS_SWI_floatValue)( JNIENV_PTR, jobject );

#define _JVAS_SWI_IDX_doubleValue	10
typedef jdouble (*JVAS_SWI_doubleValue)( JNIENV_PTR, jobject );

#define _JVAS_SWI_IDX_vtrace_ask_feed	11
typedef jint (*JVAS_SWI_vtrace_ask_feed)( JNIENV_PTR, jlong, jlong, jobject );



#ifdef __cplusplus
extern "C"{
#endif

JVAS_NOEXPORT int jvas_swi_initiate( JNIENV_PTR jnienv );

JVAS_NOEXPORT void jvas_swi_release( JNIENV_PTR jnienv );

JVAS_NOEXPORT jmethodID jvas_swi_get_static_method( JNIENV_PTR jnienv, int cls_idx, int mtd_idx, BOOL bound );

JVAS_NOEXPORT int jvas_swi_bind_methods( JNIENV_PTR jnienv, int cls_idx );

JVAS_NOEXPORT void jvas_swi_unbind_methods( JNIENV_PTR jnienv, int cls_idx );



JVAS_NOEXPORT jint jvas_swi_Begin( JNIENV_PTR jnienv, struct jvas_class_conf *clsptr, jlong reserved, const char *libpath, const char *wkdir );

JVAS_NOEXPORT void jvas_swi_End( JNIENV_PTR jnienv, struct jvas_class_conf *clsptr );

JVAS_NOEXPORT jint jvas_swi_SetMode( JNIENV_PTR jnienv, struct jvas_class_conf *clsptr, jint mode );

JVAS_NOEXPORT jlong jvas_swi_GetStatus( JNIENV_PTR jnienv, struct jvas_class_conf *clsptr );

JVAS_NOEXPORT jint jvas_swi_NumberValue( JNIENV_PTR jnienv, struct jvas_class_conf *clsptr, int dtype, jobject obj, jvalue *value );

JVAS_NOEXPORT jint jvas_swi_vtrace_ask_feed( JNIENV_PTR jnienv, struct jvas_class_conf *clsptr, jlong actref, jlong evtref, jobject extarg );


#ifdef __cplusplus
}
#endif

#endif // _JVAS_SWI_H_
