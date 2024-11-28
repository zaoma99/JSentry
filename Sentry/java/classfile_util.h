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

#ifndef _CLASSFILE_UTIL_H_
#define _CLASSFILE_UTIL_H_

#include "jbase.h"
#include "classfile_constants.h"
#include "iobuf.h"


#define CF_MEMPOOL		// adopting user memory pool


#ifdef _LINUX
#define CF_HIDDEN 		__attribute__((visibility("hidden")))
#define CF_NOEXPORT		extern	__attribute__((visibility("hidden")))
#define CF_EXPORT		extern	__attribute__((visibility("default")))
#define CF_UNUSED		__attribute__ ((unused))
#define CF_ALIAS(target)	__attribute__((alias(target)))

#elif _WINDOWS

#define CF_HIDDEN
#define CF_NOEXPORT		extern
#define CF_EXPORT		extern __declspec(dllexport)
#define CF_UNUSED		__declspec (unused)
#endif // _LINUX


#define CF_CONST_CLASS_NAME_OBJECT	"java/lang/Object"


#pragma pack(push, 1)

#define CF_MAGIC_VALUE	(0xCAFEBABE)  /* fixed magic value of ClassFile */

typedef juint32   cf_magic_t;
typedef juint8    cf_opc_t;		/* opcode */
typedef juint8    cf_tag_t;
typedef juint16   cf_idx_t;		/* index, such as name_index, descriptor_index, etc. */
typedef juint16   cf_count_t;
typedef char      cf_signature_t;

// access flags
typedef union
{
	struct{
		juint8 PUBLIC    :1;    /* bit0 */
		juint8 PRIVATE   :1;
		juint8 PROTECTED :1;
		juint8 STATIC    :1;
		juint8 FINAL     :1;
		juint8 SYNCH     :1;
		juint8 VOLATILE  :1;
		juint8 VARARGS   :1;    /* bit7 */

		juint8 NATIVE    :1;    /* bit8 */
		juint8 INTERFACE :1;
		juint8 ABSTRACT  :1;
		juint8 STRICT    :1;
		juint8 SYNTHETIC :1;
		juint8 ANNO      :1;
		juint8 ENUM      :1;
		juint8 MODULE    :1;    /* bit15 */
	};

	struct{
		juint8 bit0_4    :5;
		juint8 SUPER     :1;
		juint8 BRIDGE    :1;
		juint8 TRANSIENT :1;
#define DELETED    TRANSIENT
	};
	juint16 flags;
}cf_acc_t;

#define CF_ACC_PUBLIC		(0x0001)
#define CF_ACC_PRIVATE      (0x0002)
#define CF_ACC_PROTECTED    (0x0004)
#define CF_ACC_STATIC       (0x0008)
#define CF_ACC_FINAL        (0x0010)
#define CF_ACC_SUPER        (0x0020)
#define CF_ACC_SYNCH        (0x0020)
#define CF_ACC_VOLATILE     (0x0040)
#define CF_ACC_BRIDGE       (0x0040)
#define CF_ACC_VARARGS      (0x0080)
#define CF_ACC_TRANSIENT    (0x0080)
#define CF_ACC_NATIVE       (0x0100)
#define CF_ACC_INTERFACE    (0x0200)
#define CF_ACC_ABSTRACT     (0x0400)
#define CF_ACC_STRICT       (0x0800)
#define CF_ACC_SYNTHETIC    (0x1000)
#define CF_ACC_ANNOTATION   (0x2000)
#define CF_ACC_ENUM         (0x4000)
#define CF_ACC_MODULE       (0x8000)


// extended constants
#if (JVM_CLASSFILE_MAJOR_VERSION < 53 )
#define JVM_CONSTANT_Dynamic	17
#endif

#if (JVM_CLASSFILE_MAJOR_VERSION < 56 )
#define JVM_CONSTANT_Module		19
#define JVM_CONSTANT_Package	20
#endif

#define MACRO_NOTHING


//
#define _CF_ATTRIBUTE_TYPE_( __PREFIX__ ) enum \
{ \
	__PREFIX__##_ATTR_Unknown           = 0,                 \
	__PREFIX__##_ATTR_ConstantValue		= 1,                 \
	__PREFIX__##_ATTR_Code,                                  \
	__PREFIX__##_ATTR_StackMapTable,                         \
	__PREFIX__##_ATTR_Exceptions,                            \
	__PREFIX__##_ATTR_InnerClasses,                          \
	__PREFIX__##_ATTR_EnclosingMethod,                       \
	__PREFIX__##_ATTR_Synthetic,                             \
	__PREFIX__##_ATTR_Signature,                             \
	__PREFIX__##_ATTR_SourceFile,                            \
	__PREFIX__##_ATTR_SourceDebugExtension,                  \
	__PREFIX__##_ATTR_LineNumberTable,                       \
	__PREFIX__##_ATTR_LocalVariableTable,                    \
	__PREFIX__##_ATTR_LocalVariableTypeTable,                \
	__PREFIX__##_ATTR_Deprecated,                            \
	__PREFIX__##_ATTR_RuntimeVisibleAnnotations,             \
	__PREFIX__##_ATTR_RuntimeInvisibleAnnotations,           \
	__PREFIX__##_ATTR_RuntimeVisibleParameterAnnotations,    \
	__PREFIX__##_ATTR_RuntimeInvisibleParameterAnnotations,  \
	__PREFIX__##_ATTR_RuntimeVisibleTypeAnnotations,         \
	__PREFIX__##_ATTR_RuntimeInvisibleTypeAnnotations,       \
	__PREFIX__##_ATTR_AnnotationDefault,                     \
	__PREFIX__##_ATTR_BootstrapMethods,                      \
	__PREFIX__##_ATTR_MethodParameters,                      \
	__PREFIX__##_ATTR_Module,                                \
	__PREFIX__##_ATTR_ModulePackages,                        \
	__PREFIX__##_ATTR_ModuleMainClass,                       \
	__PREFIX__##_ATTR_NestHost,                              \
	__PREFIX__##_ATTR_NestMembers,                           \
	__PREFIX__##_ATTR_Record,                                \
	__PREFIX__##_ATTR_PermittedSubclasses,                   \
	__PREFIX__##_ATTR_MAX    = JVM_ATTR_PermittedSubclasses  \
}

typedef _CF_ATTRIBUTE_TYPE_(JVM)	JVM_ATTR_TYPE;
typedef _CF_ATTRIBUTE_TYPE_(CF)		CF_ATTR_TYPE;


#ifdef _ARCH_ENDIAN_LITTLE

#define _CF_ENDIAN_FLIP_64(__LL64__)\
	((((__LL64__) & 0x00FF) << 56) | \
	 (((__LL64__) & 0x00FF00) <<  40) | \
	 (((__LL64__) & 0x00FF0000) <<  24) | \
	 (((__LL64__) & 0x00FF000000) <<  8) | \
	 (((__LL64__) & 0x00FF00000000) >>  8) | \
	 (((__LL64__) & 0x00FF0000000000) >>  24) | \
	 (((__LL64__) & 0x00FF000000000000) >>  40) | \
	 (((__LL64__) & 0xFF00000000000000) >> 56))

#define _CF_ENDIAN_FLIP_32(__I32__)\
	((((__I32__) & 0x000000FF) << 24) | \
	 (((__I32__) & 0x0000FF00) <<  8) | \
	 (((__I32__) & 0x00FF0000) >>  8) | \
	 (((__I32__) & 0x00FF000000) >> 24))

#define _CF_ENDIAN_FLIP_16(__I16__)\
	((((__I16__) & 0x00FF) << 8) | \
	 (((__I16__) & 0x00FF00) >> 8))

#define _CF_ENDIAN_SWAP_MAGIC(__MAG__)	( __MAG__ = _CF_ENDIAN_FLIP_32( __MAG__ ) )
#define _CF_ENDIAN_SWAP_INDEX(__IDX__)	( __IDX__ = _CF_ENDIAN_FLIP_16( __IDX__ ) )
#define _CF_ENDIAN_SWAP_COUNT(__CNT__)	( __CNT__ = _CF_ENDIAN_FLIP_16( __CNT__ ) )
#define _CF_ENDIAN_SWAP_ACCES(__ACC__)	( __ACC__ = _CF_ENDIAN_FLIP_16( __ACC__ ) )

#define _CF_ENDIAN_SWAP_U2(__U2__)	( __U2__ = _CF_ENDIAN_FLIP_16( __U2__ ) )
#define _CF_ENDIAN_SWAP_U4(__U4__)	( __U4__ = _CF_ENDIAN_FLIP_32( __U4__ ) )
#define _CF_ENDIAN_SWAP_U8(__U8__)	( __U8__ = _CF_ENDIAN_FLIP_64( __U8__ ) )

inline static float _CF_ENDIAN_FLIP_FLOAT( float f )
{
	union{float f; int i;} un = {.f = f};
	_CF_ENDIAN_SWAP_U4( un.i );
	return un.f;
}

inline static double _CF_ENDIAN_FLIP_DOUBLE( double d )
{
	union{double d; long long l;} un = {.d = d};
	_CF_ENDIAN_SWAP_U8( un.l );
	return un.d;
}

#else
#define _CF_ENDIAN_FLIP_64(__LL64__)	(__LL64__)
#define _CF_ENDIAN_FLIP_32(__I32__)		(__I32__)
#define _CF_ENDIAN_FLIP_16(__I16__)		(__I16__)

#define _CF_ENDIAN_SWAP_MAGIC	_CF_ENDIAN_FLIP_32
#define _CF_ENDIAN_SWAP_INDEX	_CF_ENDIAN_FLIP_16
#define _CF_ENDIAN_SWAP_COUNT	_CF_ENDIAN_FLIP_16
#define _CF_ENDIAN_SWAP_ACCES	_CF_ENDIAN_FLIP_16

#define _CF_ENDIAN_SWAP_U2		_CF_ENDIAN_FLIP_16
#define _CF_ENDIAN_SWAP_U4		_CF_ENDIAN_FLIP_32
#define _CF_ENDIAN_SWAP_U8		_CF_ENDIAN_FLIP_64

#define _CF_ENDIAN_FLIP_FLOAT(__F__)	(__F__)
#define _CF_ENDIAN_FLIP_DOUBLE(__D__)	(__D__)

#endif // _ARCH_ENDIAN_LITTLE

#define _CF_ENDIAN_SWAP_INTEGER		_CF_ENDIAN_SWAP_U4
#define _CF_ENDIAN_SWAP_LONG		_CF_ENDIAN_SWAP_U8

#define _CF_ENDIAN_FLIP_COUNT		_CF_ENDIAN_FLIP_16
#define _CF_ENDIAN_FLIP_INDEX		_CF_ENDIAN_FLIP_16
#define _CF_ENDIAN_FLIP_ACCES		_CF_ENDIAN_FLIP_16
#define _CF_ENDIAN_FLIP_MAGIC		_CF_ENDIAN_FLIP_32
#define _CF_ENDIAN_FLIP_U2			_CF_ENDIAN_FLIP_16
#define _CF_ENDIAN_FLIP_U4			_CF_ENDIAN_FLIP_32
#define _CF_ENDIAN_FLIP_U8			_CF_ENDIAN_FLIP_64
#define _CF_ENDIAN_FLIP_INTEGER		_CF_ENDIAN_FLIP_32
#define _CF_ENDIAN_FLIP_LONG		_CF_ENDIAN_FLIP_64



// ==========================================================================================================================
// Constants Pool
// ==========================================================================================================================

// Constant Pool Head
#define _CP_HEAD_ struct{ cf_tag_t tag; }
typedef _CP_HEAD_	CP_Head;


// CP: 7
typedef struct
{
	_CP_HEAD_;
	cf_idx_t name_idx;		/* refer to CP_Utf8 */
}CP_Class;


// CP: 9, 10, 11
typedef struct
{
	_CP_HEAD_;
	cf_idx_t class_idx;		/* refer to CP_Class */
	cf_idx_t nm_tp_idx;		/* refer to CP_NameAndType */
}CP_Fieldref, CP_Methodref, CP_InterfaceMethodref, CP_IfMethodref;


// CP: 8
typedef struct
{
	_CP_HEAD_;
	cf_idx_t str_idx;		/* refer to CP_Utf8 */
}CP_String;


// CP: 3, 4
typedef struct
{
	_CP_HEAD_;
	union{
		int    i32;
		float  f32;
		juint8 b[4];
	};
}CP_Integer, CP_Float;


// CP: 5, 6
typedef struct
{
	_CP_HEAD_;
	union{
		long long i64;
		double    f64;
		juint8    b[8];
	};
}CP_Long, CP_Double;


// CP: 12
typedef struct
{
	_CP_HEAD_;
	cf_idx_t name_idx;		/* refer to CP_Utf8 */
	cf_idx_t desc_idx;		/* refer to CP_Utf8 */
}CP_NameAndType;


// CP: 1
typedef struct
{
	_CP_HEAD_;
	juint16 len;		/* length of b */
	char    b[0];
}CP_Utf8;


// CP: 15
typedef struct
{
	_CP_HEAD_;
	juint8   ref_kind;		/* see classfile_constants.h, JVM_REF_xxx */
	cf_idx_t ref_idx;		/* three types, CP_Fieldref, CP_Methodref, CP_InterfaceMethodref */
}CP_MethodHandle;


// CP: 16
typedef struct
{
	_CP_HEAD_;
	cf_idx_t desc_idx;		/* refer to CP_Utf8 */
}CP_MethodType;


// CP: 17, 18
typedef struct
{
	_CP_HEAD_;
	cf_idx_t bootstrap_attr_idx;
	cf_idx_t name_type_idx;		/* refer to CP_NameAndType */
}CP_Dynamic, CP_InvokeDynamic;


// CP: 19, 20
typedef struct
{
	_CP_HEAD_;
	cf_idx_t name_idx;		/* refer to CP_Utf8 */
}CP_Module, CP_Package;


// CP: General
typedef struct
{
	_CP_HEAD_;
	juint8 info[0];
}CP_Info;


#if 0
#define _CP_INFOPTR_( __CONST__ ) union \
{ \
	__CONST__ CP_Info *Info;                      \
	__CONST__ CP_Utf8 *Utf8;                      \
	__CONST__ CP_Integer *Integer;                \
	__CONST__ CP_Float *Float;                    \
	__CONST__ CP_Long *Long;                      \
	__CONST__ CP_Double *Double;                  \
	__CONST__ CP_Class *Class;                    \
	__CONST__ CP_String *String;                  \
	__CONST__ CP_Fieldref *Field;                 \
	__CONST__ CP_Methodref *Method;               \
	__CONST__ CP_InterfaceMethodref *IfMethod;    \
	__CONST__ CP_NameAndType *NameType;           \
	__CONST__ CP_MethodHandle *MethodHandle;      \
	__CONST__ CP_Dynamic *Dynamic;                \
	__CONST__ CP_InvokeDynamic *InvokeDynamic;    \
	__CONST__ CP_Module *Module;                  \
	__CONST__ CP_Package *Package;                \
	__CONST__ void *ptr;                          \
}

typedef _CP_INFOPTR_()		CP_InfoPtr;
typedef _CP_INFOPTR_(const)	CP_InfoConstPtr;
#endif


#define _CP_INFO_UNION_( __CONST__, __AST__, __PTR__ ) union \
{                                                            \
	__CONST__ CP_Info      __AST__ Info;                     \
	__CONST__ CP_Utf8      __AST__ Utf8;                     \
	__CONST__ CP_Integer   __AST__ Integer;                  \
	__CONST__ CP_Float     __AST__ Float;                    \
	__CONST__ CP_Long      __AST__ Long;                     \
	__CONST__ CP_Double    __AST__ Double;                   \
	__CONST__ CP_Class     __AST__ Class;                    \
	__CONST__ CP_String    __AST__ String;                   \
	__CONST__ CP_Fieldref  __AST__ Field;                    \
	__CONST__ CP_Methodref __AST__ Method;                   \
	__CONST__ CP_InterfaceMethodref __AST__ IfMethod;        \
	__CONST__ CP_NameAndType   __AST__ NameType;             \
	__CONST__ CP_MethodHandle  __AST__ MethodHandle;         \
	__CONST__ CP_MethodType    __AST__ MethodType;           \
	__CONST__ CP_Dynamic       __AST__ Dynamic;              \
	__CONST__ CP_InvokeDynamic __AST__ InvokeDynamic;        \
	__CONST__ CP_Module        __AST__ Module;               \
	__CONST__ CP_Package       __AST__ Package;              \
	__PTR__;                                                 \
}

typedef _CP_INFO_UNION_(MACRO_NOTHING, MACRO_NOTHING, MACRO_NOTHING)	CP_InfoUnion;
typedef _CP_INFO_UNION_(MACRO_NOTHING, *, void *ptr)		CP_InfoPtr;
typedef _CP_INFO_UNION_(const, *, const void *ptr)			CP_InfoConstPtr;


#define _CP_ITEM_LEN  {\
	0,          /*  */     \
	sizeof(CP_Utf8),	      /* 1 */    \
	0,	      /* 2 */    \
	sizeof(CP_Integer),	      /* 3 */    \
	sizeof(CP_Float),	      /* 4 */    \
	sizeof(CP_Long),	      /* 5 */    \
	sizeof(CP_Double),	      /* 6 */    \
	sizeof(CP_Class),	      /* 7 */    \
	sizeof(CP_String),	      /* 8 */    \
	sizeof(CP_Fieldref),      /* 9 */    \
	sizeof(CP_Methodref),     /*10 */    \
	sizeof(CP_IfMethodref),   /*11*/     \
	sizeof(CP_NameAndType),   /*12 */    \
	0,	      /*13 */    \
	0,	      /*14 */    \
	sizeof(CP_MethodHandle),  /*15 */    \
	sizeof(CP_MethodType),    /*16 */    \
	sizeof(CP_Dynamic),	      /*17 */    \
	sizeof(CP_InvokeDynamic), /*18 */    \
	sizeof(CP_Module),	      /*19 */    \
	sizeof(CP_Package)	      /*20 */    \
}


// ==========================================================================================================================
// Attributes
// ==========================================================================================================================

#define _CF_ATTR_HEAD_    \
struct{                   \
	cf_idx_t attr_nm_idx; \
	juint32  attr_len;    \
}

typedef _CF_ATTR_HEAD_	CF_AttrHead;

//
#define _CF_ATTR_INFO_(__PTR__) \
struct{                         \
	_CF_ATTR_HEAD_;             \
	union{                      \
		juint8   byte[0];       \
		__PTR__;                \
	}info;                      \
}

typedef _CF_ATTR_INFO_()	CF_AttrInfo;
typedef _CF_ATTR_INFO_(juint8 *ptr)	CF_AttrInfoEx;


// Exception
typedef struct
{
	juint16 start_pc;
	juint16 end_pc;
	juint16 handler_pc;
	cf_idx_t catch_type;
}CF_ExpInfo;


// attribute: ConstantValue
typedef struct
{
	_CF_ATTR_HEAD_;
	cf_idx_t val_idx; /* CP_Integer: int, short, char, byte, boolean
					   * CP_Long: long
					   * CP_Float: float
					   * CP_Double: double
					   * CP_String: String
	 	 	 	 	   */
}CF_Attr_Constant;


// attribute: Code
#define _CF_ATTR_CODE_( __PTR__ ) \
struct{                           \
	_CF_ATTR_HEAD_;               \
	cf_count_t   max_stack;       \
	cf_count_t   max_locals;      \
	juint32      code_len;        \
	union{                        \
		juint8   byte[0];         \
		__PTR__;                  \
	}code;                        \
	cf_count_t   exp_tab_len;     \
	union{                        \
		CF_ExpInfo ite[0];        \
		__PTR__;                  \
	}exp_tab;                     \
	cf_count_t   attr_count;      \
	union{                        \
		CF_AttrInfo ite[0];       \
		__PTR__;                  \
	}attr;                        \
}

typedef _CF_ATTR_CODE_()    CF_Attr_Code;
typedef _CF_ATTR_CODE_(juint8 *ptr)    CF_Attr_CodeEx;


// attribute: Exceptions
#define _CF_ATTR_EXCEPTION_( __PTR__ ) \
struct{                                \
	_CF_ATTR_HEAD_;                    \
	cf_count_t exp_tab_len;            \
	union{                             \
		cf_idx_t ite[0];             \
		__PTR__;                       \
	}exp_tab;                          \
}

typedef _CF_ATTR_EXCEPTION_()	CF_Attr_Exception;
typedef _CF_ATTR_EXCEPTION_(juint8 *ptr)	CF_Attr_ExceptionEx;


// attribute: BootstrapMethods
typedef struct
{
	cf_idx_t method_ref;
	cf_count_t num_arguments;
	cf_idx_t arguments[0];
}CF_BootstrapMethod;


#define _CF_ATTR_BOOTSTRAP_METHOD_(__PTR__) \
struct{                                     \
	_CF_ATTR_HEAD_;                         \
	cf_count_t num_methods;                 \
	union{                                  \
		CF_BootstrapMethod ite[0];          \
		__PTR__;                            \
	}methods;                               \
}

typedef _CF_ATTR_BOOTSTRAP_METHOD_()	CF_Attr_BootstrapMethod;
typedef _CF_ATTR_BOOTSTRAP_METHOD_(juint8 *ptr)	CF_Attr_BootstrapMethodEx;


// attribute: StackMapFrame
typedef struct
{
	juint8  frame_type;
	juint8  b[0];
}CF_Attr_StackMapFrame;

// Verification Type of StackMapFrame
typedef struct
{
	juint8  tag;
	juint8  b[0];
}CF_Attr_StackMapFrame_VT;

typedef CF_Attr_StackMapFrame_VT CF_Attr_SMFVT_Top;
typedef CF_Attr_StackMapFrame_VT CF_Attr_SMFVT_Int;
typedef CF_Attr_StackMapFrame_VT CF_Attr_SMFVT_Float;
typedef CF_Attr_StackMapFrame_VT CF_Attr_SMFVT_Long;
typedef CF_Attr_StackMapFrame_VT CF_Attr_SMFVT_Double;
typedef CF_Attr_StackMapFrame_VT CF_Attr_SMFVT_Null;
typedef CF_Attr_StackMapFrame_VT CF_Attr_SMFVT_UniThis;

typedef struct
{
	juint8   tag;
	cf_idx_t obj_idx;
}CF_Attr_SMFVT_Object;

typedef struct
{
	juint8  tag;
	juint16 offset;
}CF_Attr_SMFVT_UniVar;


typedef union
{
	juint8  frame_type;

	struct _cf_same_frame{
		juint8  type; /* 0-63 */
	}same_frame;

	struct _cf_same_locals_1_stack_item_frame{
		juint8  type; /* 64-127 */
		CF_Attr_StackMapFrame_VT stack;
	}same_locals_1_stack_item_frame;

	struct _cf_same_locals_1_stack_item_frame_ext{
		juint8  type; /* 247 */
		juint16 delta;
		CF_Attr_StackMapFrame_VT stack;
	}same_locals_1_stack_item_frame_ext;

	struct _cf_chop_frame{
		juint8  type; /* 248-250 */
		juint16 delta;
	}chop_frame;

	struct _cf_same_frame_ext{
		juint8  type; /* 251 */
		juint16 delta;
	}same_frame_ext;

	struct _cf_append_frame{
		juint8  type; /* 252-254 */
		juint16 delta;
		CF_Attr_StackMapFrame_VT locals[0];
	}append_frame;

	struct _cf_full_frame{
		juint8  type; /* 255 */
		juint16 delta;
		juint16 number_of_locals;
		CF_Attr_StackMapFrame_VT locals[0];
		juint16 number_of_stack_items;
		CF_Attr_StackMapFrame_VT stack[0];
	}full_frame;

}CF_Attr_StackMapFrame_Union;

#define CF_StackMapFrame_SameFrame_Begin	0
#define CF_StackMapFrame_SameFrame_End		63

#define CF_StackMapFrame_SameLocals_1_Stack_Item_Begin	64
#define CF_StackMapFrame_SameLocals_1_Stack_Item_End	127

#define CF_StackMapFrame_Reserved_Begin		128
#define CF_StackMapFrame_Reserved_End		246

#define CF_StackMapFrame_SameLocals_1_Stack_Item_Ext	247

#define CF_StackMapFrame_ChopFrame_Begin	248
#define CF_StackMapFrame_ChopFrame_End		250

#define CF_StackMapFrame_SameFrame_Ext		251

#define CF_StackMapFrame_AppendFrame_Begin	252
#define CF_StackMapFrame_AppendFrame_End	254

#define CF_StackMapFrame_FullFrame			255


typedef union
{
	juint8 vt_tag;
	CF_Attr_SMFVT_Top       TopVar;
	CF_Attr_SMFVT_Int       IntVar;
	CF_Attr_SMFVT_Float     FloatVar;
	CF_Attr_SMFVT_Long      LongVar;
	CF_Attr_SMFVT_Double    DoubleVar;
	CF_Attr_SMFVT_Null      NullVar;
	CF_Attr_SMFVT_UniThis   UniThisVar;
	CF_Attr_SMFVT_Object    ObjectVar;
	CF_Attr_SMFVT_UniVar    UniVar;
}CF_Attr_SMFVT_Union;

#define CF_SMFVT_top		0
#define CF_SMFVT_int		1
#define CF_SMFVT_float		2
#define CF_SMFVT_double		3
#define CF_SMFVT_long		4
#define CF_SMFVT_null		5
#define CF_SMFVT_uninit_this	6
#define CF_SMFVT_object		7
#define CF_SMFVT_uninit		8

#define CF_SMFVT_LEN_INIT	{\
	1, \
	1, \
	1, \
	1, \
	1, \
	1, \
	1, \
	3, \
	3  \
}


// attribute: StackMapTable
typedef struct
{
	_CF_ATTR_HEAD_;
	cf_count_t num_entry;
	CF_Attr_StackMapFrame entry[0];
}CF_Attr_StackMapTable;


// attribute: InnerClasses
typedef struct
{
	_CF_ATTR_HEAD_;
	cf_count_t class_num;
	struct _cf_inner_class{
		cf_idx_t	inner_class_idx;
		cf_idx_t	outer_class_idx;
		cf_idx_t	inner_name_idx;
		cf_acc_t	inner_class_access;
	}cls[0];
}CF_Attr_InnerClasses;


// attribute: EnclosingMethod
typedef struct
{
	_CF_ATTR_HEAD_;
	 cf_idx_t	class_idx;
	 cf_idx_t	method_idx;
}CF_Attr_EnclosingMethod;


// attribute: Signature
typedef struct
{
	_CF_ATTR_HEAD_;
	 cf_idx_t	signature_idx;
}CF_Attr_Signature;


// attribute: SourceFile
typedef struct
{
	_CF_ATTR_HEAD_;
	 cf_idx_t	srcfile_idx;
}CF_Attr_SourceFile;


// attribute: LineNumberTable
typedef struct
{
	_CF_ATTR_HEAD_;
	 cf_count_t lines;
	 struct _cf_line_number{
		 cf_count_t start_pc;
		 cf_count_t line_num;
	 }lntab[0];
}CF_Attr_LineNumberTable;


// attribute: LocalVariableTable
typedef struct
{
	juint16 start_pc;
	juint16 length;
	juint16 name_idx;
	union{
		cf_idx_t desc_idx;
		cf_idx_t sign_idx;
	};
	juint16 index;
}CF_Attr_LocalVariable;

typedef struct
{
	_CF_ATTR_HEAD_;
	cf_count_t locals_len;
	CF_Attr_LocalVariable locals[0];
}CF_Attr_LocalVariableTable;

// attribute: LocalVariableTypeTable
typedef CF_Attr_LocalVariableTable	CF_Attr_LocalVariableTypeTable;

// attribute: Synthetic
typedef _CF_ATTR_HEAD_	CF_Attr_Synthetic;

// attribute: Deprecated
typedef _CF_ATTR_HEAD_	CF_Attr_Deprecated;

// attribute: MethodParameters
typedef struct
{
	_CF_ATTR_HEAD_;
	juint8 param_count;
	struct{
		cf_idx_t name_idx;
		cf_acc_t access;
	}param[0];
}CF_Attr_MethodParameters;


#define _CF_MODULE_INFO_(__PREFIX__) struct \
{ \
	cf_idx_t __PREFIX__##_name_idx; \
	juint16  __PREFIX__##_flags;    \
	cf_idx_t __PREFIX__##_ver_idx;  \
}

typedef _CF_MODULE_INFO_(module)	CF_ModuleInfo;

// attribute: Module
typedef struct
{
	_CF_ATTR_HEAD_;

	_CF_MODULE_INFO_(module);

	cf_count_t requires_count;
	_CF_MODULE_INFO_(requires) requires[0];

 	cf_count_t exports_count;
 	struct _cf_module_export{
 		cf_idx_t exports_index;
 		jint16 exports_flags;
 		cf_count_t exports_to_count;
 		cf_idx_t exports_to_index[0];
 	}exports[0];

	cf_count_t opens_count;
 	struct _cf_module_open{
 		cf_idx_t opens_index;
 		jint16 opens_flags;
 		cf_count_t opens_to_count;
 		cf_idx_t opens_to_index[0];
 	}opens[0];

	cf_count_t uses_count;
	cf_idx_t uses_index[0];

	cf_count_t provides_count;
	struct _cf_module_provide{
		cf_idx_t provides_index;
		cf_count_t provides_with_count;
		cf_idx_t provides_with_index[0];
	}provides[0];
}CF_Attr_Module;

// attribute: ModulePackages
typedef struct
{
	_CF_ATTR_HEAD_;
	cf_count_t package_count;
	cf_idx_t package_idx[0];
}CF_Attr_ModulePackages;

// attribute: ModuleMainClass
typedef struct
{
	_CF_ATTR_HEAD_;
	cf_idx_t main_class_idx;
}CF_Attr_ModuleMainClass;

// attribute: NestMembers
typedef struct
{
	_CF_ATTR_HEAD_;
	 cf_count_t num_class;
	 cf_idx_t cls[0];
}CF_Attr_NestMembers;

// attribute: PermittedSubclasses
typedef struct
{
	_CF_ATTR_HEAD_;
	cf_count_t num_class;
	cf_idx_t cls[0];
}CF_Attr_PermittedSubclasses;


// ==========================================================================================================================
// ClassFile
// ==========================================================================================================================

// Field and/or Method
#define _CF_FIELD_METHOD_INFO_(__PTR__) \
struct{                        \
	cf_acc_t access;           \
	cf_idx_t name_idx;         \
	cf_idx_t desc_idx;         \
	cf_count_t attr_count;     \
	union{                     \
		CF_AttrInfo ite[0];    \
		__PTR__;               \
	}attr;                     \
}

typedef _CF_FIELD_METHOD_INFO_() CF_FieldInfo, CF_MethodInfo, CF_Field_Method;
typedef _CF_FIELD_METHOD_INFO_(CF_AttrInfoEx *ptr) CF_FieldInfoEx, CF_MethodInfoEx;


// ClassFile Header for internal representation
typedef struct
{
	cf_magic_t    magic;
	juint16       minor_ver;
	juint16       major_ver;
	cf_count_t    cp_count;		/* Note: The value is equal to the number of entries in the CP table plus one;
	 	 	 	 	 	 	 	 *       Long and Double occupies two units;
	 	 	 	 	 	 	 	 */
	CP_Info       cps[0];
	cf_acc_t      access;
	cf_idx_t      this_class;
	cf_idx_t      super_class;
	cf_count_t    if_count;
	cf_idx_t      ifs[0];
	cf_count_t    field_count;
	CF_FieldInfo  fields[0];
	cf_count_t    method_count;
	CF_MethodInfo methods[0];
	cf_count_t    attr_count;
	CF_AttrInfo   attr[0];
}CF_ClassFile;


//
typedef struct
{
	int stSize;
	size_t chkval;
}CF_MapIdent;


//
typedef struct
{
	cf_magic_t     magic;
	juint16        minor_ver;
	juint16        major_ver;

	cf_count_t     cptab_count;
	struct _cf_cptab_item{
		cf_count_t cp_count;
		CP_Info  **cps;
		ssize_t    cps_size;
	}*cptab;

	cf_acc_t       access;
	cf_idx_t       this_class;
	cf_idx_t       super_class;

	cf_count_t     iftab_count;
	struct _cf_iftab_item{
		cf_count_t if_count;
		cf_idx_t  *ifs;
	}*iftab;

	cf_count_t     fdtab_count;
	struct _cf_fdtab_item{
		cf_count_t     field_count;
		CF_FieldInfo **fields;
		ssize_t       *fields_len;
	}*fdtab;

	cf_count_t     mdtab_count;
	struct _cf_mdtab_item{
		cf_count_t      method_count;
		CF_MethodInfo **methods;
		ssize_t        *methods_len;
	}*mdtab;

	cf_count_t     attrtab_count;
	struct _cf_attrtab_item{
		cf_count_t    attr_count;
		CF_AttrInfo **attr;
	}*attrtab;

	//
	struct{
		CF_MapIdent ident;

		union{
			struct{
				// following 3 flag are determined by consumer
				int local_endian_index :1;	// always except attributes
				int local_endian_value :1;	// always except attributes
				int full_load          :1;	// reserved
				int rdonly             :1;	// read only
				int res4               :4;
				//
				int blksz_abap         :1;  // block size, as bigger as possible
				int use_sysheap        :1;	// using system heap manager(malloc, free)
				int src_buf            :1;  // buffer source
			};
			int flag;
		};

		FILE   *hfile;
		size_t  filesize;
		IOBUF_T buf;

		long mmphd;  /* keep it at the end*/
	}auxi;
}CF_ClassFileMap;


#define CF_MAP_CPTAB_PREFIX		cptab
#define CF_MAP_IFTAB_PREFIX		iftab
#define CF_MAP_FDTAB_PREFIX		fdtab
#define CF_MAP_MDTAB_PREFIX		mdtab
#define CF_MAP_ATTRTAB_PREFIX	attrtab

// Table Layout: |  committed  |  reserved  |  origin  |  edit  |
#define CF_MAP_TABLE_CAPA	4

#define CF_MAP_ORGTAB_IDX( __PREFIX__, __MAPPTR__ )	( (__MAPPTR__)->__PREFIX__##_count + 1 )
#define CF_MAP_EDTTAB_IDX( __PREFIX__, __MAPPTR__ )	( (__MAPPTR__)->__PREFIX__##_count + 2 )


typedef void* CF_MAPHD;

#define CF_CLASSFILE_MAP_SIZE	( (size_t)( &((CF_ClassFileMap*)0)->auxi ) )

#define CF_MAP_SET_FALG( __MAP__, __FLAG__, __VAL__ )	( (__MAP__).auxi.__FLAG__ = __VAL__ )
#define CF_MAP_LOCAL_ENDIAN_IDX( __MAP__ )	CF_MAP_SET_FALG(__MAP__, local_endian_index, 1 )
#define CF_MAP_LOCAL_ENDIAN_VAL( __MAP__ )	CF_MAP_SET_FALG(__MAP__, local_endian_value, 1 )
#define CF_MAP_LOCAL_ENDIAN_IDX_CLR( __MAP__ )	CF_MAP_SET_FALG(__MAP__, local_endian_index, 0 )
#define CF_MAP_LOCAL_ENDIAN_VAL_CLR( __MAP__ )	CF_MAP_SET_FALG(__MAP__, local_endian_value, 0 )

// map flags
#define CF_MAP_FLAG_MASK			(0x0F0F)
#define CF_MAP_LOCAL_ENDIAN_INDEX	(0x01)
#define CF_MAP_LOCAL_ENDIAN_VALUE	(0x02)
#define CF_MAP_FULL_LOAD			(0x04)
#define CF_MAP_RDONLY				(0x08)
#define CF_MAP_BLKSIZE_ABAP			(0x100)
#define CF_MAP_USE_SYSHEAP			(0x200)
#define CF_MAP_SOURCE_BUFFER		(0x400)
#define CF_MAP_EDIT_MODE			(0x8000)


// class file format
#define CF_FORMAT_BIN	(0)		// Original Binary Format with the Class File Standard
#define CF_FORMAT_JSON	(1)		// JSon Format with the Class File Standard
#define CF_FORMAT_XML	(2)		// XML Format with the Class File Standard
#define CF_FORMAT_MAP	(3)		// // Output original mapping directly

//
#define CF_MAP_HEAP_DEFAULT		1	// system heap


//
typedef struct
{
	char *sym;
	cf_count_t len;
	cf_idx_t cls_idx;
	juint8 type_idx;
	juint8 res[3];
}CF_SignExt;


#pragma pack(pop)

#ifdef __cplusplus
extern "C"{
#endif


CF_NOEXPORT const char CF_CP_NAME[32][32];
CF_NOEXPORT const char CF_DYNREF_KIND_NAME[32][24];
CF_NOEXPORT const char CF_ATTR_NAME[32][48];
CF_NOEXPORT const char CF_PRIME_TYPE_NAME[16][8];
CF_NOEXPORT const char CF_SIGNATURE_SYMBOL[16];
CF_NOEXPORT const juint8 CF_SIGNATURE_SLOTS[16];
CF_NOEXPORT const juint8 CF_IDX_TO_SMFVT[16];
CF_NOEXPORT const CP_Info CP_INFO_EMPTY;

//
inline static juint8 cf_get_signature_slots( char sym )
{
	const char *c = strchr( CF_SIGNATURE_SYMBOL, sym );
	return c ? CF_SIGNATURE_SLOTS[c - CF_SIGNATURE_SYMBOL] : 0;
}

//
inline static juint8 cf_get_signature_index( char sym )
{
	const char *c = strchr( CF_SIGNATURE_SYMBOL, sym );
	return c ? c - CF_SIGNATURE_SYMBOL : sizeof(CF_SIGNATURE_SYMBOL)-1;  // return the last one if it is invalid signature
}

//
inline static char cf_get_return_type( const char *desc, juint8 *idx )
{
	const char *c;
	if( (c = strchr( desc, JVM_SIGNATURE_ENDFUNC )) )
	{
		*idx = cf_get_signature_index( c[1] );
		return c[1];
	}
	return 0;
}

//
inline static char cf_get_return_type2( const char *desc, int len, juint8 *idx )
{
	for( len--; len > 0 && desc[len] != JVM_SIGNATURE_ENDFUNC; len-- );
	if( len > 0 )
	{
		len++;
		*idx = cf_get_signature_index( desc[len] );
		return desc[len];
	}
	return 0;
}



CF_NOEXPORT CF_HIDDEN void cf_free_map( CF_ClassFileMap *cf_map );

CF_NOEXPORT const CP_Info* cf_get_cpitem( const CF_ClassFileMap *cf_map, cf_idx_t idx );

CF_NOEXPORT const CP_Utf8* cf_get_cpitem_utf8( const CF_ClassFileMap *cf_map, const char *utf8, int len, cf_idx_t *idx);

CF_NOEXPORT const CP_Utf8* cf_get_cpitem_utf8_signature( const CF_ClassFileMap *cf_map, const char *utf8, int len, cf_idx_t *idx );

CF_NOEXPORT const CP_Info* cf_get_cpitem_xx( const CF_ClassFileMap *cf_map, CP_InfoUnion *cpi_hit, cf_idx_t *idx, const char *utf8, juint16 utf8_len );
//CF_NOEXPORT const CP_Info* cf_get_cpitem_ex( const CF_ClassFileMap *cf_map, CP_InfoUnion *cpi, cf_idx_t *idx );
#define cf_get_cpitem_ex(_CF_MAP, _CPI_HIT, _IDX_PTR)    cf_get_cpitem_xx(_CF_MAP, _CPI_HIT, _IDX_PTR, NULL, 0)

CF_NOEXPORT void cf_print_map_ex( const CF_ClassFileMap *map, FILE *hfile );

CF_NOEXPORT int cf_clone_constants_pool( const struct _cf_cptab_item *src, struct _cf_cptab_item *dst, CF_ClassFileMap *cf_map );

CF_NOEXPORT int cf_clone_constants_pool_m( const struct _cf_cptab_item *src, struct _cf_cptab_item *dst, CF_ClassFileMap *cf_map, cf_idx_t *mask );

CF_NOEXPORT int cf_cp2file( const struct _cf_cptab_item *src, FILE *fout );

CF_NOEXPORT ssize_t cf_cp2buffer( const struct _cf_cptab_item *src, void *buf, ssize_t szbuf );


// exported

CF_EXPORT CF_MAPHD cf_create_map( int map_flags );

CF_EXPORT void cf_destroy_map( CF_MAPHD *hmap );

CF_EXPORT int cf_make_map( CF_MAPHD hmap, const char *classfile );

CF_EXPORT int cf_make_map_from_buffer( CF_MAPHD hmap, const void *buf, ssize_t buflen );

CF_EXPORT int cf_make_classfile( CF_MAPHD hmap, const char *classfile );

CF_EXPORT int cf_make_classbuffer( CF_MAPHD hmap, void *buf, ssize_t bufsize );

CF_EXPORT int cf_copy_utf8( const CP_Utf8 *cp, char *dst, int len );

CF_EXPORT int cf_copy_cpitem( CF_MAPHD hmap, cf_idx_t idx, juint8 *dst, int size );

CF_EXPORT void cf_alter_cp_index( CP_Info **cps, int num, int off, int btsmtd_off );

CF_EXPORT void cf_resolve_constant_dependence( CP_Info **cps, int num, cf_idx_t *mask );

CF_EXPORT ssize_t cf_count_cp_size( CP_Info **cps, int num );

CF_EXPORT ssize_t cf_count_cp_size_m( CP_Info **cps, int num, cf_idx_t *mask );

CF_EXPORT ssize_t cf_copy_field_method( int which, CF_MAPHD hmap, juint8 *dst, ssize_t size, cf_idx_t name_idx, cf_idx_t desc_idx, cf_idx_t *result_idx );

#define cf_copy_field(__HMAP__, __DST__, __SIZE__, __NAMEIDX__, __DESCIDX__, __IDXPTR__) \
			cf_copy_field_method( 1, __HMAP__, __DST__, __SIZE__, __NAMEIDX__, __DESCIDX__, __IDXPTR__ )

#define cf_copy_method(__HMAP__, __DST__, __SIZE__, __NAMEIDX__, __DESCIDX__, __IDXPTR__) \
			cf_copy_field_method( 2, __HMAP__, __DST__, __SIZE__, __NAMEIDX__, __DESCIDX__, __IDXPTR__ )


CF_EXPORT ssize_t cf_copy_field_method_2( int which, CF_MAPHD hmap, juint8 *dst, ssize_t size, const char *name, const char *desc, cf_idx_t *result_idx );

#define cf_copy_field2(__HMAP__, __DST__, __SIZE__, __NAME__, __DESC__, __IDXPTR__)	\
			cf_copy_field_method_2( 1, __HMAP__, __DST__, __SIZE__, __NAME__, __DESC__, __IDXPTR__ )

#define cf_copy_method2(__HMAP__, __DST__, __SIZE__, __NAME__, __DESC__, __IDXPTR__) \
			cf_copy_field_method_2( 2, __HMAP__, __DST__, __SIZE__, __NAME__, __DESC__, __IDXPTR__ )


CF_EXPORT ssize_t cf_copy_field_method_3( int which, CF_MAPHD hmap, juint8 *dst, ssize_t size, cf_idx_t index );

#define cf_copy_field3(__HMAP__, __DST__, __SIZE__, __IDX__)		cf_copy_field_method_3( 1, __HMAP__, __DST__, __SIZE__, __IDX__ )
#define cf_copy_method3(__HMAP__, __DST__, __SIZE__, __IDX__)	cf_copy_field_method_3( 2, __HMAP__, __DST__, __SIZE__, __IDX__ )


CF_EXPORT CF_AttrInfo* cf_seek_attribute( CF_MAPHD hmap, const CF_AttrInfo *attr, cf_count_t count, const char *name, int name_len, cf_idx_t seq );


CF_EXPORT void cf_print_map( CF_MAPHD hmap, FILE *hfile );



CF_EXPORT ssize_t cf_check_attribute( int type, const CF_AttrInfo *attr, ssize_t len );  // from cf_attr_check.c

CF_EXPORT JVM_ATTR_TYPE cf_get_attr_type( const char *attr_name, int name_len );  // from cf_attr_check.c

CF_EXPORT int cf_print_cpinfo( const CP_Info *cpi, int local_endian, FILE *fout );  // from cpinfo_print.c

CF_EXPORT void cf_print_cptab( const CF_ClassFileMap *cf_map, FILE *fout );  // from cpinfo_print.c

CF_EXPORT int cf_print_attrinfo( const CF_AttrInfo *attr, cf_count_t attr_count, const CF_ClassFileMap *cf_map, FILE *fout );  // from attr_print.c

CF_EXPORT void cf_print_attrtab( const CF_ClassFileMap *cf_map, FILE *fout );  // from attr_print.c

CF_EXPORT int cf_print_access( cf_acc_t acc, FILE *fout );  // from field_print.c

CF_EXPORT int cf_print_fieldinfo( const CF_FieldInfo **field, cf_count_t count, const CF_ClassFileMap *cf_map, FILE *fout );  // from field_print.c

CF_EXPORT int cf_print_methodinfo( const CF_MethodInfo **method, cf_count_t count, const CF_ClassFileMap *cf_map, FILE *fout );  // from field_print.c

CF_EXPORT int cf_print_interface( const cf_idx_t *if_idx, cf_count_t count, const CF_ClassFileMap *cf_map, FILE *fout );  // from field_print.c

CF_EXPORT void cf_translate_method( const CF_MethodInfo *method, const CF_ClassFileMap *cf_map, FILE *fout );

CF_EXPORT void cf_translate_methodEx( const char *method_name, const CF_ClassFileMap *cf_map, FILE *fout );

#ifdef __cplusplus
}
#endif

#endif // _CLASSFILE_UTIL_H_
