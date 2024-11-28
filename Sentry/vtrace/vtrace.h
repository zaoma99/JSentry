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

#ifndef _AS_VTRACE_H_
#define _AS_VTRACE_H_

#include "vt_def.h"
#include "sabi.h"
#include "sys/taskLib.h"


#pragma pack(push, 4)


#define VT_REF_MAX	((size_t)-1>>1)


// specific-flags of struct _vtrace_capability_t
#ifdef _ARCH_ENDIAN_LITTLE
#define VTRACE_CAPAFLAG union { \
	struct { \
		vt_uint f_en     : 1;	/* enabled */ \
		vt_uint f_key    : 1;	/* key element */ \
		vt_uint f_must   : 1;	/* must defined */ \
		vt_uint f_lastn  : 1;	/* keep last n records */ \
		vt_uint f_invoke : 2;  /* input contents, such as arguments of a function; \
		                           0=disable, 1=only entry, 2=only exit/exception, 3=all */ \
		vt_uint f_return : 2;  /* output contents, such as return values of a function*/ \
		vt_uint f_cstack : 2;  /* stack of calls(function/method/routine)*/ \
		vt_uint f_except : 2;  /* uncaught exceptions/faults*/ \
		vt_uint f_catch  : 2;  /* caught exceptions/faults*/ \
		vt_uint f_watch  : 2;  /* watched variables(locals and/or globals)*/ \
		vt_uint f_memory : 2;  /* context-specific or user-specified memory rooms */ \
		vt_uint f_regs   : 2;  /* context-specific or user-specified registers */ \
		vt_uint f_syscall: 2;  /* system call(reserved) */ \
		vt_uint f_intensity: 2;/* trace intensity(0~3), the large the high */ \
		vt_uint f_journal: 8;  /* context-specific journals/logs/printable-text */ \
		vt_uint f_res32; \
	}; \
	vt_int64 flag; \
}
#else
#define VTRACE_CAPAFLAG union { \
	struct { \
		vt_uint f_res32; \
		vt_uint f_journal: 8; \
		vt_uint f_intensity: 2; \
		vt_uint f_syscall: 2; \
		vt_uint f_regs   : 2; \
		vt_uint f_memory : 2; \
		vt_uint f_watch  : 2; \
		vt_uint f_catch  : 2; \
		vt_uint f_except : 2; \
		vt_uint f_cstack : 2; \
		vt_uint f_return : 2; \
		vt_uint f_invoke : 2; \
		vt_uint f_lastn  : 1; \
		vt_uint f_must   : 1; \
		vt_uint f_key    : 1; \
		vt_uint f_en     : 1; \
	}; \
	vt_int64 flag; \
}
#endif

typedef VTRACE_CAPAFLAG		vt_capa_flag;

// flags of vt_capacity
#define VTRACE_CAPA_FLAG_ENABLE         0x01
#define VTRACE_CAPA_FLAG_KEY            0x02
#define VTRACE_CAPA_FLAG_MUST           0x04
#define VTRACE_CAPA_FLAG_LASTn          0x08

#define VTRACE_CAPA_FLAG_SHIFT_INVOKE     4
#define VTRACE_CAPA_FLAG_SHIFT_RETURN     6
#define VTRACE_CAPA_FLAG_SHIFT_CSTACK     8
#define VTRACE_CAPA_FLAG_SHIFT_EXCEPT     10
#define VTRACE_CAPA_FLAG_SHIFT_CATCH      12
#define VTRACE_CAPA_FLAG_SHIFT_WATCH      14
#define VTRACE_CAPA_FLAG_SHIFT_MEMORY     16
#define VTRACE_CAPA_FLAG_SHIFT_REGS       18
#define VTRACE_CAPA_FLAG_SHIFT_SYSCALL    20
#define VTRACE_CAPA_FLAG_SHIFT_INTENSITY  22
#define VTRACE_CAPA_FLAG_SHIFT_JOURNAL    24

#define VTRACE_CAPA_FLAG_TRACE_DIS    	0
#define VTRACE_CAPA_FLAG_TRACE_ENTRY  	1
#define VTRACE_CAPA_FLAG_TRACE_EXIT   	2
#define VTRACE_CAPA_FLAG_TRACE_ALL    	3

#define VTRACE_CAPA_INTENSITY_STRONG	3
#define VTRACE_CAPA_INTENSITY_NORMAL	2
#define VTRACE_CAPA_INTENSITY_LIGHT		1
#define VTRACE_CAPA_INTENSITY_DIVE		0


// capability of trace point
typedef struct _vtrace_capability_t
{
	vt_size  sot_max;		/* maximum of SOT recorded */
	vt_size  mem_max;		/* maximum of memory occupied */
	vt_size  dsk_max;		/* maximum of disk space occupied */
	vt_time  live_max;		/* maximum of CPU time consumed(sec) */
	vt_time  exec_tmo;		/* timeout of executions(msec) */
	vt_int32 fd_max;		/* maximum of files opened */
	vt_int32 rep_max;		/* maximum of report*/
	vt_int32 freq;			/* sample frequency */
	vt_int32 doj_dec;		/* decay of data object joint */
	VTRACE_CAPAFLAG;		/* flags */
}vt_capa;


// special sample frequency
#define VTRACE_CAPA_FREQ_EVERY		(1)		// trace every time
#define VTRACE_CAPA_FREQ_E2E		(0)		// entry, exit and exception(NOT USED)
#define VTRACE_CAPA_FREQ_ENTRY		(-1)	// at entry only(NOT USED)
#define VTRACE_CAPA_FREQ_EXIT		(-2)	// at exit/exception only(NOT USED)
#define VTRACE_CAPA_FREQ_VALID(__F__)	( (__F__) >= VTRACE_CAPA_FREQ_E2E )


// trace condition
typedef union _vtrace_condition_t
{
#ifdef _ARCH_ENDIAN_LITTLE
	struct {
		vt_uint data_propagate : 1;	/**/
		vt_uint data_transform : 1;    /**/
		vt_uint same_action    : 1;    /**/
		vt_uint same_event     : 1;    /**/
		vt_uint crit_point     : 1;    /* critical function be called */
		vt_uint crit_resource  : 1;	/* critical resource be accessed */
		vt_uint spec_trap      : 1;	/* specific trap be caught */
		vt_uint consistency    : 1;
		vt_uint integrity      : 1;
		vt_uint data_verify    : 1;
		vt_uint authorize      : 1;
		vt_uint authenticate   : 1;
		vt_uint credential     : 1;
		vt_uint permit         : 1;
		vt_uint prog_execute   : 1;
		vt_uint prog_abnormal  : 1;
		vt_uint trace_start    : 1;
		vt_uint trace_done     : 1;
		vt_uint trace_break    : 1;
		vt_uint tpix_sync      : 1; /* special for trace point index synchronizing */
	};
#else
	struct {
		vt_uint tpix_sync      : 1;
		vt_uint trace_break    : 1;
		vt_uint trace_done     : 1;
		vt_uint trace_start    : 1;
		vt_uint prog_abnormal  : 1;
		vt_uint prog_execute   : 1;
		vt_uint permit         : 1;
		vt_uint credential     : 1;
		vt_uint authenticate   : 1;
		vt_uint authorize      : 1;
		vt_uint data_verify    : 1;
		vt_uint integrity      : 1;
		vt_uint consistency    : 1;
		vt_uint spec_trap      : 1;
		vt_uint crit_resource  : 1;
		vt_uint crit_point     : 1;
		vt_uint same_event     : 1;
		vt_uint same_action    : 1;
		vt_uint data_transform : 1;
		vt_uint data_propagate : 1;
	};
#endif
	vt_int32 cond;
}vt_condition, vt_cond;


// triggered reason(according to vt_condition)
typedef enum
{
	VTRACE_CAUSE_NULL           = 0,
	VTRACE_CAUSE_DATA_PROPAGATE = 0x01,
	VTRACE_CAUSE_DATA_TRANSFORM = 0x02,
	VTRACE_CAUSE_SAME_ACTION    = 0x04,
	VTRACE_CAUSE_SAME_EVENT     = 0x08,
	VTRACE_CAUSE_CRIT_POINT     = 0x10,
	VTRACE_CAUSE_CRIT_RESOURCE  = 0x20,
	VTRACE_CAUSE_SPEC_TRAP      = 0x40,
	VTRACE_CAUSE_CONSISTENCY    = 0x80,
	VTRACE_CAUSE_INTEGRITY      = 0x100,
	VTRACE_CAUSE_DATA_VERIFY    = 0x200,
	VTRACE_CAUSE_AUTHORIZE      = 0x400,
	VTRACE_CAUSE_AUTHENTICATE   = 0x800,
	VTRACE_CAUSE_CREDENTIALS    = 0x1000,
	VTRACE_CAUSE_PERMIT         = 0x2000,
	VTRACE_CAUSE_CODE_EXECUTE   = 0x4000,
	VTRACE_CAUSE_CODE_ABNORMAL  = 0x8000,
	VTRACE_CAUSE_TRACE_START    = 0x10000,
	VTRACE_CAUSE_TRACE_DONE     = 0x20000,
	VTRACE_CAUSE_TRACE_BREAK    = 0x40000,
	VTRACE_CAUSE_TPIX_SYNC      = 0x80000,
}vt_cause;


// triggered stage
typedef enum
{
	VTRACE_STAGE_NULL		= 0,
	VTRACE_STAGE_ENTER		= 1,	// enter into trace point
	VTRACE_STAGE_LEAVE		= 2,	// return from trace point
	VTRACE_STAGE_CATCH		= 4,	// a caught exception
	VTRACE_STAGE_TRAP		= 8,	// anywhere trap(reserved)
	VTRACE_STAGE_BEGIN		= 16,	// start to tracing
	VTRACE_STAGE_END		= 32,	// stop tracing
	VTRACE_STAGE_CRASH		= 64,	// an uncaught exception
	VTRACE_STAGE_ALL		= -1
}vt_stage;


// trace mode
typedef enum
{
	VTRACE_MODE_NULL    = SABI_MODE_IDEL,
	VTRACE_MODE_TRACE   = SABI_MODE_TRACE,
	VTRACE_MODE_TEST    = SABI_MODE_TEST,
	VTRACE_MODE_ASPP    = SABI_MODE_ASPP,
	VTRACE_MODE_OFG     = SABI_MODE_OFG,
	VTRACE_MODE_DFG     = SABI_MODE_DFG,
	VTRACE_MODE_IAST    = VTRACE_MODE_TEST,
	VTRACE_MODE_RASP    = VTRACE_MODE_ASPP,
}vt_mode;


// trace kind
typedef enum
{
	VTRACE_KIND_KTP    = 1,		// key/major trace point
	VTRACE_KIND_CRIT   = 2,		// critical, will calculating/analyzing automatically
	VTRACE_KIND_BEGIN  = 4,		// begin point of tracing
	VTRACE_KIND_END    = 8,		// end point of tracing
	VTRACE_KIND_ASYN   = 16,	// do calculating asynchronously(by calculator thread)
#define VTRACE_KIND_CRIT_KTP	(VTRACE_KIND_KTP | VTRACE_KIND_CRIT)
}vt_kind;


// trace level
typedef enum
{
	VTRACE_LEVEL_HW				= 0x0 ,	  /* hardware */
	VTRACE_LEVEL_OS				= 0x01,   /* operating system */
	VTRACE_LEVEL_SYSLIB			= 0x03,   /* system library(native) */
	VTRACE_LEVEL_STDLIB			= 0x04,   /* standard library(native) */
	VTRACE_LEVEL_USER			= 0x0F,   /* user code(native) */
	VTRACE_LEVEL_VM				= 0x30,   /* virtual machine */
	VTRACE_LEVEL_FRAMEWORK		= 0x31,   /* framework on VM */
	VTRACE_LEVEL_VSTDLIB		= 0x32,   /* standard library on VM */
	VTRACE_LEVEL_VUSER			= 0x3F,   /* user code on VM */
}vt_level;


//
typedef enum
{
	VTRACE_LANG_UNKNOWN    = 0,
	VTRACE_LANG_NATIVE     = 1,
	VTRACE_LANG_ASM        = 2,
	VTRACE_LANG_C          = 3,
	VTRACE_LANG_CPP        = 4,
	VTRACE_LANG_CXX        = 4,
	VTRACE_LANG_JAVA       = 5,
	VTRACE_LANG_PYTHON     = 6,
	VTRACE_LANG_GOLANG     = 7,
	VTRACE_LANG_JAVASCRIPT = 8,
	VTRACE_LANG_PHP        = 9,
	VTRACE_LANG_DOTNET     = 10,
}vt_lang;


//
typedef enum
{
	VTRACE_PROT_UNKNOWN    = 0,
	VTRACE_PROT_HTTP       = 1,
	VTRACE_PROT_RPC        = 2,
	VTRACE_PROT_gRPC       = 3,
	VTRACE_PROT_WSOCK      = 4,
}vt_proto;


//
typedef SABI_JOURNAL_TYPE	vt_journal_type;
#define VT_JOURNAL_MODE_STORE	(1)
#define VT_JOURNAL_MODE_PRINT	(2)
#define VT_JOURNAL_MODE_BOTH	(3)


//
typedef SABI_ERROR	VT_ERROR;


//
typedef enum
{
	VTRACE_DICT_NULL             = 0,
	VTRACE_DICT_TRACE_MODE       = 1,
	VTRACE_DICT_TRACE_STAGE      = 2,
	VTRACE_DICT_TRACE_LEVEL      = 3,
	VTRACE_DICT_TRACE_LANG       = 4,
	VTRACE_DICT_TRACE_COND       = 5,  // unused
	VTRACE_DICT_TRACE_CAUSE      = 6,
	VTRACE_DICT_TRACE_KIND       = 7,
	VTRACE_DICT_DATA_TYPE        = 8,
	VTRACE_DICT_JOURNAL_TYPE     = 9,
	VTRACE_DICT_EXCEPTION_TYPE   = 10,
	VTRACE_DICT_EVENT_TYPE       = 11,
	VTRACE_DICT_REQUEST_TYPE     = 12,
	VTRACE_DICT_RESPONSE_TYPE    = 13,
	VTRACE_DICT_OPERATOR_TYPE    = 14,
	VTRACE_DICT_OPERATOR_SET     = 15,
	VTRACE_DICT_TRACE_PROTOCOL   = 16,
	//VTRACE_DICT_CONSTRAINT_RANGE = 15,
	//VTRACE_DICT_CONSTRAINT_ENUM  = 16,
	//VTRACE_DICT_CONSTRAINT_REGEX = 17,
}vt_dict_intrinsic;


// resource consumption
typedef struct _vtrace_consum_t
{
	vt_size nMem;	/* number of memory occupied */
	vt_size nDisk;	/* number of disk space occupied */
	vt_size nTime;	/* number of CPU time consumed */
	//vt_size nFile;	/* number of files opened */
}vt_consum;


//
typedef struct _vtrace_statistic_t
{
	vt_consum consum;
	vt_size   nmet;
	vt_size   ndatobj;/* number of DataObject allocated */
	vt_time   tmbeg; /* ms */
	union {
		struct {
#ifdef _ARCH_ENDIAN_LITTLE
			int   suspend : 1;
			int   sot_ov  : 1;  // out of limitation of SOT
			int   doj_ov  : 1;  // out of limitation of data object joint
			int   dep_w_ov: 1;  // out of limitation of data object dependence(wide)
			int   dif_w_ov: 1;  // out of limitation of data object diffusion(wide)
			int   dep_d_ov: 1;  // RESERVED, out of limitation of data object dependence(deep), it is difficult to calculates joint depth every time!!!
			int   dif_d_ov: 1;  // RESERVED, out of limitation of data object diffusion(deep)
#else
			int   dif_d_ov: 1;
			int   dep_d_ov: 1;
			int   dif_w_ov: 1;
			int   dep_w_ov: 1;
			int   doj_ov  : 1;
			int   sot_ov  : 1;
			int   suspend : 1;
#endif
		};
		int i32;
	}flags;
}vt_statistic;

#define VT_STATISTIC_ENABLE  // necessary !!!
#define VT_STATISTIC_TIME_DISABLE

#define VT_STAT_IS_SUSPENDED(_ST_)  	((_ST_).flags.i32 & 0x03)
#define VT_ACTION_IS_SUSPENDED(_ST_)	((_ST_).flags.i32 & 0x07)
#define VT_DPOINT_IS_SUSPENDED(_ST_)	((_ST_).flags.i32 & 0x1f)


//
struct _vtrace_sot_t;
struct _vtrace_action_t;
struct _vtrace_event_t;
struct _vtrace_rule_t;
struct _vtrace_policy_t;
struct _vtrace_data_class;
struct _vtrace_data_object;
struct _vtrace_point_t;
struct _vtrace_point_dynamic_t;
struct _vtrace_brief_report;
struct vt_ctx_t;
struct vt_ns_t;


//
typedef union _vtrace_modifier
{
#ifdef _ARCH_ENDIAN_LITTLE
	struct {
		vt_word x     :1;    // executable
		vt_word w     :1;    // writable
		vt_word r     :1;    // readable
		vt_word d     :1;    // removable
		vt_word o     :1;    // optional
		vt_word res   :7;    // reserved
		vt_word global:1;    // global object
		vt_word embed :1;    // reserved
		vt_word obj   :1;    // reserved
		vt_word hole  :1;    // equal to placeholder
	};
#else
	struct {
		vt_word hole  :1;
		vt_word obj   :1;
		vt_word embed :1;
		vt_word global:1
		vt_word res   :7;
		vt_word o     :1;
		vt_word d     :1;
		vt_word r     :1;
		vt_word w     :1;
		vt_word x     :1;
	};
#endif
	vt_word flags;
}vt_modifier __attribute__((aligned(2)));

#define VT_MOD_X		(0x0001)
#define VT_MOD_W		(0x0002)
#define VT_MOD_R		(0x0004)
#define VT_MOD_D		(0x0008)
#define VT_MOD_O		(0x0010)
#define VT_MOD_DEAD		(0x0020)	// move to the "src" of data object
#define VT_MOD_ESCAPE	(0x0040)	// move to the "src" of data object
#define VT_MOD_EFFECTED	(0x0080)	// move to the "src" of data object
#define VT_MOD_GLOBAL	(0x1000)
#define VT_MOD_EMBED	(0x2000)
#define VT_MOD_OBJECT	(0x4000)
#define VT_MOD_HOLE		(0x8000)


//
typedef union
{
	struct {
		vt_refer name;
		vt_byte  choice;
	};
	vt_range range;
	vt_enumerate enumerate;
	vt_regularization regular;
}vt_primitive_constraint;


// (may be changed in future)
typedef enum
{
	VTRACE_INOP_ANY         = 0  ,
	VTRACE_INOP_NEW         = 1  ,
	VTRACE_INOP_DELETE      = 2  ,
	VTRACE_INOP_CONCAT      = 3  ,
	VTRACE_INOP_TRUNC       = 4  ,
	VTRACE_INOP_CLEAR       = 5  ,
	VTRACE_INOP_ASSIGN      = 6  ,
	VTRACE_INOP_COPY        = 7  ,
	VTRACE_INOP_MOVE        = 8  ,
	VTRACE_INOP_SLICE       = 9  ,
	VTRACE_INOP_CHANGE      = 10 ,
	VTRACE_INOP_VERIFY      = 11 ,
	VTRACE_INOP_ENCODE      = 12 ,
	VTRACE_INOP_DECODE      = 13 ,
	VTRACE_INOP_ESCAPE      = 14 ,
	VTRACE_INOP_UNESCAPE    = 15 ,
	VTRACE_INOP_EXPOSE      = VTRACE_INOP_UNESCAPE ,
	VTRACE_INOP_FORMAT      = 16 ,
	VTRACE_INOP_SERIALIZE   = 17 ,
	VTRACE_INOP_UNSERIALIZE = 18 ,
	VTRACE_INOP_SIGNATURE   = 19 ,
	VTRACE_INOP_ENCRYPT     = 20 ,
	VTRACE_INOP_DECRYPT     = 21 ,
	VTRACE_INOP_INFLATE     = 22 ,
	VTRACE_INOP_DEFLATE     = 23 ,
	VTRACE_INOP_SHUFFLE     = 24 ,
	VTRACE_INOP_ORDER       = 25 ,

	VTRACE_INOP_EXECUTE     = 0x0100 ,

	VTRACE_INOP_AUTHORIZE     = 0x0200 ,
	VTRACE_INOP_AUTHENTICATE  = 0x0201 ,
	VTRACE_INOP_PERMIT        = 0x0202 ,

	VTRACE_INOP_SQL         = 0x0300 ,
	VTRACE_INOP_NOSQL       = 0x0400 ,
}vt_operator_type, vt_intrinsic_operator;

#define VTRACE_IS_SAFE_OPERATOR( _OP_ ) \
	((_OP_) & (VTRACE_INOP_ESCAPE | VTRACE_INOP_ENCRYPT | VTRACE_INOP_DEFLATE | VTRACE_INOP_SHUFFLE | VTRACE_INOP_ENCODE))

#define VTRACE_IS_UNSAFE_OPERATOR( _OP_ ) \
	((_OP_) & (VTRACE_INOP_UNESCAPE | VTRACE_INOP_DECRYPT | VTRACE_INOP_INFLATE | VTRACE_INOP_ORDER | VTRACE_INOP_DECODE | VTRACE_INOP_UNSERIALIZE))


//
typedef enum
{
	VTRACE_OPSET_DISABLED   = 0x0,
	VTRACE_OPSET_SOURCE		= 0x1,
	VTRACE_OPSET_PROPAGATE	= 0x2,
	VTRACE_OPSET_TARGET     = 0x4,
	VTRACE_OPSET_ORIGIN     = VTRACE_OPSET_SOURCE,
	VTRACE_OPSET_GENERAL    = VTRACE_OPSET_PROPAGATE,
	VTRACE_OPSET_SINK		= VTRACE_OPSET_TARGET,
	VTRACE_OPSET_RULE       = VTRACE_OPSET_TARGET,
	VTRACE_OPSET_DEADZONE   = 0x8,

	VTRACE_OPSET_SUSPENDED  = 0x20,  // be disabled due to unreasonable situations occurred in deadzone
	VTRACE_OPSET_ESCAPED    = 0x40,  // be escaped or processed by other safe operators
	VTRACE_OPSET_EFFECTED   = 0x80,  // be effected
}vt_operator_set;

//
typedef struct
{
	vt_ident uid;
	vt_refer name;	// name of function/method(refers to vt_cstr)
	vt_refer decl;	// declaration of parameter list and return value list(refers to char* that is a part of name's vt_cstr)
	//vt_long  pos;	// position/location
	//vt_long  idx;
	vt_word  inop;	// refer to intrinsic operation
	vt_word  opset; // operator set, for compatible with older system
	//vt_word  attribute_num;	// reserved for attribute
	//vt_refer *attributes;	// reserved for attribute
	struct {
		vt_uint   argn;
		struct _vtrace_data_class **args;    // class or object???
		vt_uint   fpnum;
	}arglst;		// arguments of method
	//vt_refer extarg[2]; // language specific, [0]:=tracepoint, [1]:=(reserved)
	vt_refer tpoint;
	//vt_refer mtd_hd;		/* method handle/address, language-specific */
	struct _vtrace_class_struct_t *klass;
}vt_method_struct, vt_func_struct;

#define VT_METHOD_ARG_MAX	(4096)
#define VT_METHOD_RET_MAX	(4096)


#if 0
//
typedef struct
{
	vt_refer klass;		// refer to vt_class_struct
	vt_refer callable;	// refer to vt_method_struct
}vt_method, vt_function;
#endif

typedef vt_method_struct	vt_method, vt_function;

//
typedef struct _vtrace_class_struct_t
{
#if 0
	vt_modifier mod;
	vt_byte  lang;
	vt_byte  super_num;
	vt_long  nref;
	union{
		struct {
			vt_refer name;
			vt_refer path;
		};
		vt_ident uid;
	};
	vt_refer *supers;
	vt_uint  method_num;
	vt_uint  field_num;
	vt_uint  attribute_num;
	vt_refer *methods;
	vt_refer *fields;
	vt_refer *attributes;
#else
	// simplest version
	vt_ident uid;
	vt_refer name; // cs_cstr
	vt_lang  lang;
	vt_uint  method_num;
	vt_uint  mtdidx_num;
	vt_method_struct **methods;
	vt_method_struct **mtd_idx;
#endif
}vt_class_struct;


// trace point descriptor
#define VTRACE_POINT(__MAGIC__, __STRU_NAME__) struct __STRU_NAME__ { \
	__MAGIC__;              /* */ \
	vt_long  idx;           /* trace point index, for fast search/match */ \
	vt_byte  kind;          /* kind */ \
	vt_byte  mode;          /* trace mode */ \
	vt_byte  stage;         /* trace stage */ \
	vt_byte  level;         /* trace level(optional) */ \
	vt_cond  cond;          /* trace condition */ \
	vt_size  sot_max_tp; \
	/*vt_capa   capa;*/       /* trace capabilities(configures) */ \
	const vt_method *target;     /* target will be traced, generally it is a function/method */ \
	const struct _vtrace_rule_t *rule; /* calculation rule */ \
	\
	/*struct { \
		vt_uint ndpt; \
		vt_uint nact; \
		struct _vtrace_point_dynamic_t **dpts; \
	}idxtab_act; */ \
}
#define VTRACE_POINT_M    VTRACE_POINT(vt_magic magic,)
typedef VTRACE_POINT(vt_magic magic, _vtrace_point_t)    vt_point, vt_trap;


#define VTRACE_SOT_LINK_DPOINT
//#define VTRACE_SOT_LINK_ACTION


// trace point of double-link-node form
/*typedef struct _vtrace_point_node_t
{
	VTRACE_DLNODE(_vtrace_ptnode_t) clue_uni;
	VTRACE_POINT_M;
}vt_point_node;
*/

// dynamic trace point descriptor(also is hash-node form)
typedef struct _vtrace_point_dynamic_t
{
	VTRACE_DLNODE(_vtrace_point_dynamic_t) clue_act;

	vt_point  *point;		/* trace point definition */
	struct _vtrace_action_t *action;

#ifdef VTRACE_SOT_LINK_DPOINT
	union{
		/*struct {
			vt_size nsot;
			struct _vtrace_sot_t **sots;
		}sottab;*/
		dlk_link  sotlnk;	/* SOT double-link-table */
	};
#else
	vt_size sotnum;
	struct _vtrace_sotnode_t *last_sot;
#endif

	dlk_link   datobjlnk;	/* local data objects(contains all SOT's data) */

	vt_statistic sta;		/* statistic information */

	//vt_ssize delta_seq;
}vt_dpoint;


//
#define VTRACE_OBJECT_REPLICA_ARRAY		(1)
#define VTRACE_OBJECT_REPLICA_DLINK		(2)
#define VTRACE_OBJECT_REPLICA_STRUCT	VTRACE_OBJECT_REPLICA_DLINK
#define VTRACE_OBJECT_REPLICA_KEEP_REFERENCE
//#define VTRACE_OBJECT_CACHE
//#define VTRACE_OBJECT_REPLICA
#define VTRACE_ACTION_HAS_DPOINT

typedef struct _trace_datobj_replica
{
#if VTRACE_OBJECT_REPLICA_STRUCT == VTRACE_OBJECT_REPLICA_DLINK
	VTRACE_DLNODE(_trace_datobj_replica) clue;
#endif

	vt_refer owner;	// sots(default), data_object or others

	vt_uint nitem;
	vt_tlv  *items;
}vt_datobj_replica;


//
typedef struct _vtrace_data_class
{
	vt_modifier mod;   // modifier and/or access control of class
	vt_byte     type;  // type of value, refer to the vt_typeid
	vt_byte     ndim;  // number of shape, will be used when this is an array
	//vt_long    nref;
	vt_refer    name;
	vt_ident    uid;   // unique identifier in class space
	vt_refer    orgtype;// origin type name(language specific)
	vt_refer    depend; // fixed dependences descriptor(cstring)

	union{
		vt_size *shape;// full dimension shape of array, used when the ndim is greater than 1
		vt_size dim1;  // first dimension of array, used when the ndim is 1
		vt_size nsub;  // number of subs, used when the class is not an array(ndim is 0)
	};

	vt_primitive_constraint *constraint; // default constraint description
	struct _vtrace_data_class *host;  // it is not NULL if this is an embed sub class
	struct _vtrace_data_class **subs;  // sub members list
}vt_data_class;


//
typedef struct _vtrace_dataclass_node
{
	VTRACE_DLNODE(_vtrace_dataclass_node) clue_uni;
	vt_data_class klass;
}vt_dataclass_node;


//
typedef struct _vtrace_data_object
{
	vt_modifier mod;	// modifier and/or access control of object
	vt_word     src;	// source, be used for OperatorSet at present
	vt_long     nref;
	vt_refer    name;
	vt_ident    uid;	// unique identifier in object space
	//vt_refer    depend; // fixed dependences descriptor(cstring)

	vt_size track_seq;  // temporary

	union{
#if VTRACE_OBJECT_REPLICA_STRUCT == VTRACE_OBJECT_REPLICA_ARRAY
		vt_size replica_num;   //
#endif
	};

	//vt_primitive_constraint *constraint; // adopted constraint description

	vt_data_class *klass; // refer to the definition of class

	union{ // measured value
#if VTRACE_OBJECT_REPLICA_STRUCT == VTRACE_OBJECT_REPLICA_ARRAY
		struct {
			//vt_size replica_num;
			vt_datobj_replica  **replica;
		};
#elif VTRACE_OBJECT_REPLICA_STRUCT == VTRACE_OBJECT_REPLICA_DLINK
		dlk_link dlk_replica;
#endif
	};
}vt_data_object;

#define VTRACE_OPSET_IS_TRACED(_OPSET_)		( _OPSET_ & (VTRACE_OPSET_EFFECTED | VTRACE_OPSET_SOURCE | VTRACE_OPSET_TARGET) )
#define VTRACE_OPSET_NOT_TRACED(_OPSET_)	( !VTRACE_OPSET_IS_TRACED(_OPSET_) )
#define VTRACE_DATOBJ_IS_TRACED(_O_)		( VTRACE_OPSET_IS_TRACED((_O_).src) )
#define VTRACE_DATOBJ_TRACE_ALLOW(_O_, _M_)		( VTRACE_OPSET_IS_TRACED((_O_).src) && ((_M_)&VT_MOD_R) )

//#define VTRACE_OBJECT_TRACE_SOT
//#define VTRACE_OBJECT_TRACE_SUPPLY
#define VTRACE_OBJECT_JOINT_RESTRICT
#define VTRACE_OBJECT_TREE_WITH_ARRAY	(1)
#define VTRACE_OBJECT_TREE_WITH_DLINK	(2)
#define VTRACE_OBJECT_TREE_STRUCT		VTRACE_OBJECT_TREE_WITH_DLINK

//
typedef struct _vtrace_dataobject_node
{
	union {
		VTRACE_DLNODE(_vtrace_dataobject_node) clue_dpt;
		VTRACE_DLNODE(_vtrace_dataobject_node) clue_loc;
	};

	vt_refer owner;  // who created it, may be tpidx(default), dpoint or other

#ifdef VTRACE_OBJECT_TRACE_SOT
	// replaced by it's replica.owner
	union{
		struct {
			vt_size nsot;
			struct _vtrace_sot_t **sots;
		}sottab;
	};
#else
	// disabled
	//struct _vtrace_sotnode_t *_first_sot;
	//struct _vtrace_sotnode_t *_last_sot;
#endif

	union{
#if VTRACE_OBJECT_TREE_STRUCT == VTRACE_OBJECT_TREE_WITH_ARRAY
		struct {
			union{
				vt_size num_depend;
				vt_size num_acquire;
			};
			union{
				struct _vtrace_datobj_joint2 **depend;
				struct _vtrace_datobj_joint2 **acquire;
			};
		};
#define depend_wide	num_depend
#elif VTRACE_OBJECT_TREE_STRUCT == VTRACE_OBJECT_TREE_WITH_DLINK
		dlk_link dlk_depend;
#define depend_wide	dlk_depend.num
#endif
	};


#ifdef VTRACE_OBJECT_TRACE_SUPPLY
	union{
#if VTRACE_OBJECT_TREE_STRUCT == VTRACE_OBJECT_TREE_WITH_ARRAY
		struct {
			union{
				vt_size num_referred;
				vt_size num_supply;
			};
			union{
				struct _vtrace_datobj_joint2 **referred;
				struct _vtrace_datobj_joint2 **supply;
			};
		};
#define diffuse_wide	num_supply
#elif VTRACE_OBJECT_TREE_STRUCT == VTRACE_OBJECT_TREE_WITH_DLINK
		dlk_link dlk_supply;
#define diffuse_wide	dlk_supply.num
#endif
	};
#endif

#ifdef VTRACE_OBJECT_JOINT_RESTRICT
	//vt_uint depend_deep;
#ifndef VTRACE_OBJECT_TRACE_SUPPLY
	vt_uint diffuse_wide;
#endif
#endif

	vt_data_object object;  // keep it at the tail
}vt_dataobject_node;

#define VT_DATOBJ_TO_NODE( _OBJ_ )		( (void*)( (size_t)(_OBJ_) - _OFFSET(vt_dataobject_node, object) ) )
#define VT_DATNODE_TO_OBJ( _NODE_ ) 	( (void*)( (size_t)(_NODE_) + _OFFSET(vt_dataobject_node, object) ) )

//#define VT_DATOBJ_JOINT_DECAY_MAX	(9999999)

#if (VTRACE_OBJECT_TREE_STRUCT == VTRACE_OBJECT_TREE_WITH_DLINK)
//
typedef struct _vtrace_datobj_joint
{
	VTRACE_DLNODE(_vtrace_datobj_joint) clue_dep;
	vt_dataobject_node *from;  // acquire
	vt_dataobject_node *to;    // referred
	struct _vtrace_sotnode_t *sot;
	//struct _vtrace_sotnode_t *sot_beg;
	//int  join_num;
	short argidx_from;
	short argidx_to;

#ifdef VTRACE_OBJECT_TRACE_SUPPLY
	VTRACE_DLNODE(_vtrace_datobj_joint) clue_sup;
#endif
}vt_datobj_joint;

#else
typedef struct _vtrace_datobj_joint2
{
	vt_dataobject_node *from;  // acquire
	vt_dataobject_node *to;    // referred
	int  join_num;
	struct _vtrace_sotnode_t *sot;
}vt_datobj_joint2;
#endif

/*
#if (VTRACE_OBJECT_TREE_STRUCT == VTRACE_OBJECT_TREE_WITH_DLINK)
//
typedef struct _vtrace_nodejoin_wrap
{
	VTRACE_DLNODE(_vtrace_nodejoin_wrap);
	void *real_node;
	void *extptr;    // NOTE: DO NOT change
	int  join_num;
}vt_nodejoin_wrap;

#else
typedef struct _vtrace_nodejoin_wrap2
{
	void *real_node;
	void *extptr;
	int  join_num;
}vt_nodejoin_wrap2;
#endif
*/

//
typedef struct
{
	vt_long  ndat;
	vt_ident *uids;
	vt_tlv   *datas;
	vt_cstr  *names;
}vt_data_sequence;

//
typedef struct
{
	vt_uint nobj;
	union{
		vt_data_object **objs;  // maybe points to a vt_data_object be contained in a vt_dataobject_node
		//vt_dataobject_node **objnds;
	};
}vt_data_objset;

typedef struct
{
	union{
		vt_data_sequence datseq;
		vt_data_objset   objset;
	}args;
}vt_data_invoke;

typedef vt_data_invoke	vt_data_return;
typedef vt_data_invoke	vt_data_watch;

typedef struct
{
	vt_object mtd_ref;
	vt_char *full_path;
	vt_uint path_len;
	vt_uint name_off;
	vt_uint decl_off;
	vt_long pos;
}vt_call_frame;

typedef struct
{
	vt_thread thread;
	vt_int num;
	vt_call_frame *frame;
}vt_data_callstack;

typedef struct
{
	vt_int type;
	vt_uint msglen;
	vt_char *msgstr;
	vt_data_callstack backtrace;
}vt_data_exception;

typedef struct
{
	// TODO:
}vt_data_memory;

typedef struct
{
	// TODO:
}vt_data_regset;

typedef struct
{
	// TODO:
}vt_data_syscall;

typedef struct
{
	vt_byte type;	// see vt_journal_type
	vt_byte mode;	// 1=file only, 2=print only, 3=both
	vt_word srclen;
	vt_uint msglen;
	vt_char *source;
	vt_char *message;
}vt_data_journal;

typedef struct
{
	//vt_uint  tag;
	//vt_uint  opt;
	//vt_clock tms;
	union{
		vt_data_sequence datseq;
		vt_data_objset   objset;
	};
}vt_data_request;

typedef struct
{
	vt_uint  tag;	/* reserved */
	vt_uint  opt;   /* reserved */
	vt_clock tms;
	union{
		vt_data_sequence datseq;
		vt_data_objset   objset;
	};
}vt_data_response;


// trace contents
typedef struct _vtrace_sot_entity_t
{
	union{
		vt_data_invoke    invk;
		vt_data_return    retn;
		vt_data_exception expt;
	};
	vt_data_callstack cstk;
	vt_data_watch     watch;
	vt_data_memory    mem;
	vt_data_regset    regs;
	vt_data_syscall   sysc;
	vt_data_journal   log;
}vt_sot_entity;


// sequence of trace (record)
#define VTRACE_SOT(__MAGIC__, __STRU_NAME__) struct __STRU_NAME__ { \
	__MAGIC__;          /* */ \
	vt_byte   stage;	/* */ \
	vt_byte   cause;	/* */ \
	vt_byte   res[2]; \
	vt_dpoint *dpoint;  /* */ \
	/*vt_method *caller;*/	/* */ \
	vt_size   a_seq;     /* sequence number of an action(within one thread) */ \
	/*vt_size   e_seq;*/     /* sequence number of an event(across multi-threads) */ \
	vt_sot_entity entity;/* contents */ \
	/*vt_refer  ext;*/ \
}
#define VTRACE_SOT_M    VTRACE_SOT(vt_magic magic,)
typedef VTRACE_SOT(vt_magic magic, _vtrace_sot_t)    vt_sot;



// SOT of double-link-node form
typedef struct _vtrace_sotnode_t
{
#ifdef VTRACE_SOT_LINK_DPOINT
	VTRACE_DLNODE(_vtrace_sotnode_t) clue_dpt;		/* by sequences of one dpoint */
#endif

#ifdef VTRACE_SOT_LINK_ACTION
	VTRACE_DLNODE(_vtrace_sotnode_t) clue_act;		/* by sequences of one action */
#endif

	//VTRACE_DLNODE(_vtrace_sotnode_t) clue_evt;		/* by sequences of one event(optional) */

	vt_sot __splitor[0];
	VTRACE_SOT_M;  // keep it at last
}vt_sotnode;

#define VT_NODE2SOT(_NODE_)		( _NODE_ ? (vt_sot*)( (size_t)(_NODE_) + _OFFSET(vt_sotnode, __splitor) ) : NULL )
#define VT_SOT2NODE(_SOT_)		( _SOT_ ? (vt_sotnode*)( (size_t)(_SOT_) - _OFFSET(vt_sotnode, __splitor) ) : NULL )


// event entity
typedef struct _vtrace_event_entity_t
{
	vt_uint  type;  	/* event type */
	vt_uint  _idx_;		/* interval purposed */
	vt_ident ident;		/* event identifier */
	vt_sapa  from;		/* optional */
	vt_sapa  dest;		/* optional */
	vt_data_request req;
}vt_event_entity;


//
typedef struct _vtrace_brief_report
{
	struct {
		vt_long num_obj_exposed;
		vt_long num_obj_escaped;
		vt_long num_obj_lost;
		vt_long num_obj_bad;
		vt_long num_obj_unused;
		vt_long num_obj_unknown;
		vt_long num_obj_total;
	}data;

	struct {
		vt_long num_op_bad;		   /* disabled operator */
		vt_long num_op_undefined;  /* unknown operator */
		vt_long num_op_missed;     /* missed necessary operator*/
		vt_long num_bt_suspicious; /* unexpected back-trace of call stack */
		vt_long num_cf_suspicious; /* unexpected control flow(program flow) */
	}prog;

	struct {
#ifdef _ARCH_ENDIAN_LITTLE
		vt_uint authorize_fail     :1;
		vt_uint authenticate_error :1;
		vt_uint credentials_bad    :1;
		vt_uint permit_error       :1;
		vt_uint :28;
#else
		vt_uint :28;
		vt_uint permit_error       :1;
		vt_uint credentials_bad    :1;
		vt_uint authenticate_error :1;
		vt_uint authorize_fail     :1;
#endif
		vt_uint  res;
	}flag;

	vt_int  confidence;	// the high the better
	vt_int  res;
	vt_long vulner_id;  // vulnerability identifier
	vt_refer sot;
}vt_brief_report;


//
typedef struct _vtrace_detailed_report
{
	vt_brief_report head;
}vt_detailed_report;

typedef struct _vtrace_brief_report_node
{
	VTRACE_SLNODE(_vtrace_brief_report_node);
	vt_brief_report report;
}vt_brief_report_node;


// event descriptor(also is hash-node form)
typedef struct _vtrace_event_t
{
	VTRACE_DLKNODE_HEAD(_vtrace_event_t) clue_uni;
	vt_long nref;

	vt_event_entity  entity; /* event entity descriptor */
	vt_data_response *resp; /* response contents */

	//vt_brief_report  report;

	union{
		struct {
			vt_size nact;
			struct _vtrace_action_t **acts;
		}acttab;
		dlk_link    actlnk;		/* action double-link-table */
	};

#if 0
	union{
		struct {
			vt_size nsot;
			vt_sot  **sots;
		}sottab;
		dlk_link    sotlnk;		/* optional */
	};
#endif

	//vt_time     tmbeg;
	//vt_uint     index;
	vt_size     seqseed;	/* seed of sequence number(across multiple threads) */
	//long       mmphd;
	SEM_ID      bsem;		/* concurrency lock */
	struct vt_ns_t *container;
}vt_event;


// trace action descriptor(also is hash-node form)
typedef struct _vtrace_action_t
{
	VTRACE_DLKNODE_HEAD(_vtrace_action_t) clue_evt;
	vt_long nref;

	vt_event  *event;
	vt_mode    mode;
	vt_capa    capa;		/* */

	vt_brief_report report;
	slk_link   replnk;

#ifdef VTRACE_ACTION_HAS_DPOINT
	struct {
		vt_long   ndpt;
		vt_dpoint **dpts;
	}dpttab;	/* all trace points, temporarily processing that binding all trace points when create action */

	dlk_link   livdptlnk;		/* live trace points */
#endif

#ifdef VTRACE_OBJECT_CACHE
	vt_cache_box datobj_cache;
#endif

#ifdef VTRACE_SOT_LINK_ACTION
	union{
		struct {
			vt_size nsot;
			vt_sot  **sots;
		}sottab;
		dlk_link   sotlnk;		/*  */
	};
#else
	vt_size sotnum;
	//vt_sotnode last_sot;
#endif

	struct {
		vt_long _tpidx;
		vt_long _tpidx2;
	}last_stat;

	vt_size joint_num;
	vt_size track_seq;		/* temporary, for traversing data object */

	vt_statistic  sta;		/* statistics */
	//vt_thread  thread;		/**/
	//vt_uint    index;		/**/
	vt_size    seqseed;		/* seed of sequence number(within one thread) */
	//long       mmphd;
	//SEM_ID     bsem;		/* concurrency lock */
	struct vt_ctx_t *ctx;
}vt_action;


#pragma pack(pop)


#ifdef __cplusplus
extern "C"{
#endif



#ifdef __cplusplus
}
#endif

#endif // _AS_VTRACE_H_
