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

#ifndef _JVAS_EHR_H_
#define _JVAS_EHR_H_


#include "jvas.h"
#include "jtievt.h"



#ifdef __cplusplus
extern "C"{
#endif

JVAS_NOEXPORT void JNICALL jvas_ehr_VMInit( JTIENV_PTR jvmti, JNIENV_PTR jni, jthread thread );
JVAS_NOEXPORT void JNICALL jvas_ehr_VMStart( JTIENV_PTR jvmti, JNIENV_PTR jni );
JVAS_NOEXPORT void JNICALL jvas_ehr_VMDeath( JTIENV_PTR jvmti, JNIENV_PTR jni );

JVAS_NOEXPORT
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
										 );

JVAS_NOEXPORT
void JNICALL jvas_ehr_ClassLoad( JTIENV_PTR jvmti,
								 JNIENV_PTR jni,
								 jthread thread,
								 jclass klass
								 );

JVAS_NOEXPORT
void JNICALL jvas_ehr_ClassPrepare( JTIENV_PTR jvmti,
									JNIENV_PTR jni,
									jthread thread,
									jclass klass
									);

JVAS_NOEXPORT
void JNICALL jvas_ehr_NativeMethodBind( JTIENV_PTR jvmti,
										JNIENV_PTR jni,
										jthread thread,
										jmethodID method,
										void* address,
										void** new_address_ptr
										);


JVAS_NOEXPORT
void JNICALL jvas_ehr_VMObjectAlloc( JTIENV_PTR jvmti,
									 JNIENV_PTR jni,
									 jthread thread,
									 jobject object,
									 jclass object_klass,
									 jlong size
									 );

JVAS_NOEXPORT
void JNICALL jvas_ehr_ResourceExhausted( JTIENV_PTR jvmti,
										 JNIENV_PTR jni,
										 jint flags,
										 const void* reserved,
										 const char* description
										 );


JVAS_NOEXPORT void JNICALL jvas_ehr_ObjectFree( JTIENV_PTR jvmti, jlong tag );

JVAS_NOEXPORT void JNICALL jvas_ehr_ThreadEnd( JTIENV_PTR jvmti, JNIENV_PTR jni, jthread thread );

JVAS_NOEXPORT void JNICALL jvas_ehr_ThreadStart( JTIENV_PTR jvmti, JNIENV_PTR jni, jthread thread );


#ifdef __cplusplus
}
#endif

#endif // _JVAS_EHR_H_
