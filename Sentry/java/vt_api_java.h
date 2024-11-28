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

#ifndef _AS_VTAPI_JAVA_H_
#define _AS_VTAPI_JAVA_H_

#include "vt_api.h"
#include "jbase.h"
#include "jni.h"
#include "jvmti.h"

#pragma pack(push, 4)

#define VT_JAVA_DEBUG

// default configures of java environment
#if (_JVM_VER == 11)
#define VT_JNI_VERSION_DEF		JNI_VERSION_10
#define VT_JVMTI_VERSION_DEF	JVMTI_VERSION_11
#else
#define VT_JNI_VERSION_DEF		JNI_VERSION_1_8
#define VT_JVMTI_VERSION_DEF	JVMTI_VERSION_1_2
#endif

#define VT_JAVA_ORIGIN_METHOD_STACK_DEPTH	(3)		// stack depth of the origin method when invoke the sot_append

#define VT_JAVA_ACTION_EQUAL_THREAD


// sequence of tag/type+length+value, general purpose
struct vt_java_tlv {
	//vt_int  tag;    /* tag/type of value */
	//vt_long len;    /* length(byte number) of value */
	//jobject val;    /* byte[], value(consecutive byte stream) */
	vt_int  tag;
	jobject value;
};
#define VT_JAVA_TLV_NMEMBER    (2)

// capacity configure(MUST compatible with the vt_capa defined in the vtrace.h)
struct vt_java_capacity {
	vt_long sot_max;    /* maximum of SOT recorded */
	vt_long mem_max;    /* maximum of memory occupied */
	vt_long dsk_max;    /* maximum of disk space occupied */
	vt_long live_max;   /* maximum of CPU time consumed(sec) */
	vt_long exec_tmo;   /* timeout of executions(msec) */
	union {
		struct {
			vt_int fd_max;     /* maximum of files opened */
			vt_int rep_max;
		};
		vt_long res1;
	};
	union {
		struct {
			vt_int freq;       /* trace frequency */
			vt_int doj_dup;
		};
		vt_long res2;
	};
	vt_long flags;      /* refer to vtrace.h */
};
#define VT_JAVA_CAPACITY_NMEMBER    (8)

// data sequence, general purpose
struct vt_java_data_sequence {
	vt_long  ndat;     /* length of dataset */
	jobject  uids;     /* long[], one uid is packed in two long items */
	jobject  datas;    /* Object[][], value of data item, array of array of object which represents the vt_tlv */
	jobject  names;    /* String[], name of corresponding data item */
};
#define VT_JAVA_DATA_SEQUENCE_NMEMBER    (4)

// contents and/or description of an event
struct vt_java_event_entity {
	vt_int  type;       /* event type */
	vt_int  opt;        /* reserved */
	vt_long ident_l;    /* event identifier, low 8-bytes */
	vt_long ident_h;    /* high 8-bytes */
	vt_long from_l;     /* source address of SAP(optional), low 8-bytes */
	vt_long from_h;     /* high 8-bytes */
	vt_long dest_l;     /* destination address of SAP(optional), low 8-bytes */
	vt_long dest_h;     /* high 8-bytes */
	jobject reqdat;     /* Object, description/contents of corresponding request; array of object which represents the vt_data_sequence */
};
#define VT_JAVA_EVENT_ENTITY_NMEMBER    (9)

//
struct vt_java_response {
	vt_int   tag;     /* reserved */
	vt_int   opt;     /* reserved */
	vt_long  tv_sec;
	vt_long  tv_nsec;
	jobject  respdat; /* Object, description/contents of corresponding response; array of object which represents the vt_data_sequence */
};

// description of a call frame
struct vt_java_call_frame {
	jobject full_path;  /* String, full description of method, be formated as: <package>/<class name>/<method name(parameter list)return_type> */
	vt_int  name_off;   /* name offset in the full_path */
	vt_int  decl_off;   /* declaration(parameter list and/or return value) offset in the full_path */
	vt_long pos;        /* opcode/instruction position in specified method */
};
#define VT_JAVA_CALL_FRAME_NMEMBER    (4)

// description of a call stack
struct vt_java_data_callstack {
	jobject  thread;   /* Object, thread who makes call */
	vt_int   nframe;   /* length of frames */
	jobject  frames;   /* Object[][], array of array of Object which represents a vt_call_frame */
};
#define VT_JAVA_DATA_CALLSTACK_NMEMBER    (3)

// description of an exception
struct vt_java_data_exception {
	vt_int  type;    /* exception type, predefined */
	jobject message; /* String, exception message */
	jobject bt;      /* Object, back trace of call stack, array of object which represents a vt_data_callstack */
};
#define VT_JAVA_DATA_EXCEPTION_NMEMBER    (3)

// description of journal
struct vt_java_data_journal {
	vt_int  type;     /* see sabi.java */
	//vt_int  mode;     /* 1=file only, 2=print only, 3=both */
	jobject source;   /* String, journal source */
	jobject message;  /* Stirng, journal message */
};
#define VT_JAVA_DATA_JOURNAL_NMEMBER    (3)

// system call
/*
struct vt_java_data_syscall {
  // TODO:
};
*/

// entity of a SOT(sequence of track)
struct vt_java_sot_entity {
	jobject args;      /* Object,
						  arguments of Entry, Exit or Exception;
						  On the entry or exit, it is an array of object which represents the vt_data_sequence;
						  On the exception point, it is an array of object which represents the vt_data_exception;
					   */
	jobject cstk;      /* Object, call stack at this point, array of object which represents the vt_data_callstack */
	jobject watch;     /* Object, variables watched, similar as args */
	//vt_data_memory    mem;
	//vt_data_regset    regs;
	//vt_data_syscall   sysc;
	jobject journal;   /* Object, array of object which represents the vt_data_journal */
};
#define VT_JAVA_SOT_ENTITY_NMEMBER    (4)

// brief report(MUST compatible with the vt_capa defined in the vtrace.h)
struct vt_java_brief_report {
	vt_long num_obj_exposed;
	vt_long num_obj_escaped;
	vt_long num_obj_lost;
	vt_long num_obj_bad;
	vt_long num_obj_unused;
	vt_long num_obj_unknown;
	vt_long num_obj_total;

	vt_long num_op_bad;        /* disabled operator */
	vt_long num_op_undefined;  /* unknown operator */
	vt_long num_op_missed;     /* missed necessary operator*/

	vt_long num_bt_suspicious; /* unexpected back-trace of call stack */
	vt_long num_cf_suspicious; /* unexpected control flow(program flow) */

	vt_long flags;      /* see below, VT_REPORT_FLAG_xxx (NOT USED) */
	vt_long confidence; /* final confidence, the high the better */
	vt_long vulner_id;  /* vulnerability identifier */
	vt_long seq;
};
#define VT_JAVA_BRIEF_REPORT_NMEMBER    (16)

// temporary...
#define VT_JAVA_REPORT_FLAG_AUTHORIZE_FAIL        = 0x01;
#define VT_JAVA_REPORT_FLAG_AUTHENTICATE_ERROR    = 0x02;
#define VT_JAVA_REPORT_FLAG_CREDENTIALS_BAD       = 0x04;
#define VT_JAVA_REPORT_FLAG_PERMIT_ERROR          = 0x08;


#pragma pack(pop)


#ifdef __cplusplus
extern "C"{
#endif

JNIEXPORT
int vt_java_initiate( JVM_PTR jvm, JTIENV_PTR jtienv, jint jni_ver, jint jti_ver );


JNIEXPORT
void vt_java_shutdown( BOOL jvm_exit );


JNIEXPORT
jobject JNICALL java_event_create( JNIENV_PTR jni, jobject jcls,
								   jlong   tpidx,
								   jint    mode,
								   jobject capacity,
								   jobject entity
								   );

//
JNIEXPORT
void JNICALL java_event_destroy( JNIENV_PTR jni, jobject jcls, jlong evtref );

//
VT_EXPORT
jlong JNICALL java_action_new( JNIENV_PTR jni, jobject jcls,
							   jobject new_thread,
							   jint    mode,
							   jobject capacity,
							   jlong   evtref
							   );

//
VT_EXPORT
void JNICALL java_action_del( JNIENV_PTR jni, jobject jcls, jlong actref, jlong evtref );

//
VT_EXPORT
jlong JNICALL java_action_get( JNIENV_PTR jni, jobject jcls, jobject thread, jlong evtref );  // obsoleted

//
VT_EXPORT
jlong JNICALL java_sot_append( JNIENV_PTR jni, jobject jcls,
							   jlong   tpidx,
							   jint    stage,
							   jint    cause,
							   jobject entity,
							   jlong   actref
							   );

//
VT_EXPORT
jlong JNICALL java_sot_append2( JNIENV_PTR jni, jobject jcls,
							    jstring name,
							    jstring sign,
							    jstring klass,
							    jint    stage,
							    jint    cause,
							    jobject entity,
							    jlong   actref
							    );

//
VT_EXPORT
void JNICALL java_sot_remove( JNIENV_PTR jni, jobject jcls, jlong sotref, jlong actref );  // obsoleted

VT_EXPORT
jint JNICALL java_fill_response( JNIENV_PTR jni, jobject jcls, jlong evtref, jobject resp );  // obsoleted

//
VT_EXPORT
jobject JNICALL java_calc_confidence( JNIENV_PTR jni, jobject jcls,
									  jlong   actref,
								      jlong   evtref,
								      jobject datseq
								      );

//
VT_EXPORT
jint JNICALL java_feed_data( JNIENV_PTR jni, jobject jcls,
							 jlong   actref,
							 jlong   evtref,
							 jobject datseq
							 );

//
VT_EXPORT
jobject JNICALL java_report_get( JNIENV_PTR jni, jobject jcls,
								 jlong   actref,
		 	 	 	 	 	     jlong   evtref
							     );

//
VT_EXPORT jobject JNICALL java_tp_idx_copy( JNIENV_PTR jni, jobject jcls );  // obsoleted

//
VT_EXPORT jint JNICALL java_tpix_sync( JNIENV_PTR jni, jobject jcls, jlong key, jstring klass, jstring name_sign );


//
VT_EXPORT char* vt_java_decl_split( const char *decl_name, char **sign, char **buf, const char *klass_ident );

//
VT_EXPORT vt_typeid vt_java_type_convert( const char *sign, vt_byte *ndim );

//
VT_EXPORT int vt_java_get_1sign( const char *decl_str, int idx, vt_byte *ndim, int *sign_len );

//
VT_EXPORT jint vt_java_objcmp( jobject obj1, jobject obj2 );

//
VT_EXPORT jobject vt_java_globalref_new( jobject obj );

//
VT_EXPORT void vt_java_globalref_del( jobject obj );

//
VT_EXPORT jint vt_java_callframe_fill( vt_call_frame *frames, jint count, long *mmphd );

//
VT_EXPORT jint vt_java_callstack_copy( jthread thread, jint begin, jint max, vt_call_frame **dst, jboolean onlyref, long *mmphd );

//
VT_EXPORT jint vt_java_get_last_callframe( jthread thread, vt_call_frame *dst );

//
VT_EXPORT int vt_java_decl_norm( const char *decl, int len, char **dst, int *dstsize );

//
VT_EXPORT int vt_java_decl_check( char *name, int len );

//
VT_EXPORT int vt_java_1sign_norm( const char *token, int len, char *norm );

//
VT_EXPORT jlong vt_java_map_object( jobject obj );

//
VT_EXPORT jint vt_java_bind_object( jobject obj, jlong tag );

//
VT_EXPORT void vt_java_action_exit( vt_refer actref, vt_refer arg );


#ifdef __cplusplus
}
#endif

#endif // _AS_VTAPI_JAVA_H_
