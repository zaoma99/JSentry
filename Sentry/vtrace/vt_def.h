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

#ifndef _VTRACE_DEF_H_
#define _VTRACE_DEF_H_

#include "stypes.h"
#include "facility/dlink.h"
#include "facility/slink.h"
#include "facility/btree.h"


#ifdef _LINUX
#include "gcc/gcc_base.h"

#define VT_HIDDEN 			__attribute__((visibility("hidden")))
#define VT_VISIBLE 			__attribute__((visibility("default")))
#define VT_NOEXPORT		extern	__attribute__((visibility("hidden")))
#define VT_EXPORT		extern	__attribute__((visibility("default")))
#define VT_ALIAS(__origin__, __target__)	__attribute__((alias(__origin__))) __target__

#elif _WINDOWS

#define VT_HIDDEN
#define VT_VISIBLE			__declspec(dllexport)
#define VT_NOEXPORT extern
#define VT_EXPORT	extern	__declspec(dllexport)
#endif

#define VT_LOCAL	static
#define VT_EXTERN 	extern
#define VT_CALLDECL


#pragma pack(push, 4)

typedef enum
{
	VT_TYPE_NULL       = 0  ,    //
	VT_TYPE_SI8        = 1  ,    // signed integer within 1-byte
	VT_TYPE_SI16       = 2  ,    // signed integer within 2-byte
	VT_TYPE_SI32       = 3  ,    // signed integer within 4-byte
	VT_TYPE_SI64       = 4  ,    // signed integer within 8-byte
	VT_TYPE_SI128      = 5  ,    // signed integer within 16-byte(reserved)
	VT_TYPE_UI8        = 6  ,    // unsigned integer within 1-byte
	VT_TYPE_UI16       = 7  ,    // unsigned integer within 2-byte
	VT_TYPE_UI32       = 8  ,    // unsigned integer within 4-byte
	VT_TYPE_UI64       = 9  ,    // unsigned integer within 8-byte
	VT_TYPE_UI128      = 10 ,    // unsigned integer within 16-byte(reserved)
	VT_TYPE_F32        = 11 ,    // IEEE-754 binary32 floating-point format
	VT_TYPE_F64        = 12 ,    // IEEE-754 binary64 floating-point format
	VT_TYPE_F128       = 13 ,    // reserved
	VT_TYPE_POINTER    = 14 ,    // same as the void*, pointer of address space
	VT_TYPE_TIME       = 15 ,    // same as the time_t
	VT_TYPE_CLOCK      = 16 ,    // same as the struct timespec
	VT_TYPE_IDENTIFIER = 17 ,	 // object identifier
	VT_TYPE_CSTRING    = 18 ,    // visible character string(UTF-8 encoding)
	VT_TYPE_BSTRING    = 19 ,    // byte string
	//
	VT_TYPE_STRUCT     = 29 ,
	VT_TYPE_CLASS      = 29 ,
	VT_TYPE_ARRAY      = 30 ,
	//
	VT_TYPE_PRIMITIVE_BEG = VT_TYPE_SI8,
	VT_TYPE_PRIMITIVE_END = VT_TYPE_IDENTIFIER,
	//
	VT_TYPE_BOOL       = VT_TYPE_UI8  ,
	VT_TYPE_BYTE       = VT_TYPE_UI8  ,
	VT_TYPE_OCTET      = VT_TYPE_UI8  ,
	VT_TYPE_CHAR       = VT_TYPE_SI8  ,
	VT_TYPE_WCHAR      = VT_TYPE_UI16 ,    // wide character(UTF-16 encoding)
	VT_TYPE_WORD       = VT_TYPE_UI16 ,
	VT_TYPE_SHORT      = VT_TYPE_SI16 ,
	VT_TYPE_INT        = VT_TYPE_SI32 ,
	VT_TYPE_UINT       = VT_TYPE_UI32 ,
	VT_TYPE_DWORD      = VT_TYPE_UI32 ,
	VT_TYPE_LONG       = VT_TYPE_SI64 ,
	VT_TYPE_ULONG      = VT_TYPE_UI64 ,
	VT_TYPE_QWORD      = VT_TYPE_UI64 ,
	VT_TYPE_FLOAT      = VT_TYPE_F32  ,
	VT_TYPE_DOUBLE     = VT_TYPE_F64  ,
#if defined( _ARCH_BIT64 )
	VT_TYPE_OFFSET     = VT_TYPE_LONG ,
	VT_TYPE_SSIZE      = VT_TYPE_LONG,
	VT_TYPE_SIZE       = VT_TYPE_ULONG,
#else
	VT_TYPE_OFFSET     = VT_TYPE_INT ,
	VT_TYPE_SSIZE      = VT_TYPE_INT,
	VT_TYPE_SIZE       = VT_TYPE_UINT,
#endif
	VT_TYPE_ENUM       = VT_TYPE_INT  ,
}vt_typeid;


typedef char			vt_char;
typedef char			vt_int8;
typedef short			vt_int16;
typedef int				vt_int32;
typedef long long		vt_int64;
typedef unsigned char	vt_byte;
typedef unsigned char	vt_int8u;
typedef unsigned short	vt_int16u;
typedef unsigned int	vt_int32u;
typedef unsigned long long	vt_int64u;
typedef float			vt_float32;
typedef double			vt_float64;
typedef float           vt_float;
typedef double          vt_double;
typedef char			vt_bool;
typedef int				vt_enum;
typedef unsigned short	vt_wchar;

typedef vt_int16u		vt_word;
typedef vt_int32u		vt_dword, vt_uint;
typedef vt_int64u		vt_qword, vt_ulong;
typedef vt_int32		vt_int;
typedef vt_int64		vt_long;

typedef void *          vt_ptr;
typedef void *			vt_class;
typedef void *			vt_object;
typedef void *			vt_handle;
typedef void *			vt_refer;
typedef void *			vt_thread;


#define VT_IDENTIFIER_LEN	16
typedef union
{
	vt_ptr  ptr[VT_IDENTIFIER_LEN/sizeof(vt_ptr)];
	vt_long idx[VT_IDENTIFIER_LEN/sizeof(vt_long)];
	vt_long l[VT_IDENTIFIER_LEN/sizeof(vt_long)];
	vt_byte b[VT_IDENTIFIER_LEN];

#ifdef _ARCH_ENDIAN_LITTLE
	struct {
		vt_ulong low;
		vt_ulong high;
	};
#else
	struct {
		vt_ulong high;
		vt_ulong low;
	};
#endif
}vt_ident;

typedef vt_ident	vt_placeholder;


#define VT_SAP_ADDRESS_LEN	16
typedef union
{
	vt_long l[VT_SAP_ADDRESS_LEN/sizeof(vt_long)];
	vt_byte b[VT_SAP_ADDRESS_LEN];

#ifdef _ARCH_ENDIAN_LITTLE
	struct {
		vt_ulong low;
		vt_ulong high;
	};
#else
	struct {
		vt_ulong high;
		vt_ulong low;
	};
#endif
}vt_sapa;


#define VT_SIGN_LEN		16
typedef union
{
	vt_long l[VT_SIGN_LEN/sizeof(vt_long)];
	vt_byte b[VT_SIGN_LEN];

#ifdef _ARCH_ENDIAN_LITTLE
	struct {
		vt_ulong low;
		vt_ulong high;
	};
#else
	struct {
		vt_ulong high;
		vt_ulong low;
	};
#endif
}vt_sign;


//typedef ssize_t			vt_ssize;
//typedef size_t			vt_size;
//typedef time_t			vt_time;
//typedef struct timespec	vt_clock;


#if defined( _ARCH_BIT64 )
typedef long long			vt_ssize;
typedef unsigned long long	vt_size;
typedef long long			vt_time;

#elif defined( _ARCH_BIT32 )
typedef int					vt_ssize;
typedef unsigned int		vt_size;
typedef int					vt_time;
#endif

typedef union
{
	vt_char	  c;
	vt_int16  s;
	vt_int32  i;
	vt_int64  l;
	vt_byte   b;
	vt_wchar  w;
	vt_float  f;
	vt_double d;
	vt_int32u u;
	vt_int64u q;
	vt_bool   bl;

	vt_ptr    p;
	vt_class  k;
	vt_object o;
	vt_byte   a[8];
}vt_value;

typedef struct
{
	vt_size  len;
	union{
		vt_char  *cs;  // utf8
		vt_value val;
	};
}vt_cstr;

typedef struct
{
	vt_size  len;
	union{
		vt_wchar *wcs;  // utf16
		vt_value val;
	};
}vt_wstr;

typedef struct
{
	vt_size  len;
	union{
		vt_byte  *bs;
		vt_value val;
	};
}vt_bstr;


typedef struct    // same as the struct timespec
{
	vt_time tv_sec;
	vt_time tv_nsec;
}vt_clock;

#define VT_TRUE		(1)
#define VT_FALSE	(0)


#define VT_TYPE_IS_PREMITIVE( _TYPE_CODE_ )	( (_TYPE_CODE_) <= VT_TYPE_PRIMITIVE_END )

#define VT_TYPE_IS_VALID( _TYPE_CODE_ )	(\
	((_TYPE_CODE_) >= VT_TYPE_PRIMITIVE_BEG && (_TYPE_CODE_) <= VT_TYPE_PRIMITIVE_END) \
	|| (_TYPE_CODE_) == VT_TYPE_CLASS \
	|| (_TYPE_CODE_) == VT_TYPE_ARRAY \
	)


#define VT_TYPE_SIZE_INITIATOR { \
	0 , \
	1 , \
	2 , \
	4 , \
	8 , \
	16, \
	1 , \
	2 , \
	4 , \
	8 , \
	16, \
	4 , \
	8 , \
	16, \
	sizeof(vt_ptr), \
	sizeof(vt_time), \
	sizeof(vt_clock), \
	sizeof(vt_ident), \
	-1, /* CSTRING */ \
	-1, /* BSTRING */ \
	 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 20~28, reserved */ \
	-1, /* CLASS */ \
	-1  /* ARRAY */ \
}


typedef struct
{
	vt_char *full;
	vt_word len;
	vt_word name_off;
}vt_path;

typedef struct
{
	vt_char *full;
	vt_size len;
	vt_size name_off;
}vt_path_l;


typedef union
{
	char  b;
	short s;
	int   i;
	long  l;
	void *ptr;
	long long ll;
}vt_pos;


typedef struct
{
	union{
		vt_uint tag;  // [0]: type
					  // [1]: only primitive available when [0] is ARRAY
					  //      and, only bstring available when [0] is CLASS
					  // [2,3]: reserved
#ifdef _ARCH_ENDIAN_LITTLE
		struct {
			vt_byte type1;
			vt_byte type2;
			vt_word vtype;  // intrinsic data type
		};
#else
		struct {
			vt_word vtype;
			vt_byte type2;
			vt_byte type1;
		};
#endif
	};

	union{
		struct {
			vt_size  len;
			vt_value val;
		};
		struct {
			size_t id;
			void   *ptr;
		}val_obj;
		vt_cstr  val_cs;
		vt_bstr  val_bs;
		vt_ident val_id;
		vt_byte  val_b16[16];
	};
}vt_tlv;  // __attribute__((aligned(1)));


inline static void *vt_tlv_parse( vt_tlv *tlv, vt_size *sz )
{
	if( tlv->type1 == VT_TYPE_CLOCK || tlv->type1 == VT_TYPE_IDENTIFIER )
	{
		*sz = sizeof(tlv->val_b16);
		return tlv->val_b16;
	}

	*sz = tlv->len;
	return &tlv->val;
}

//
inline static void *vt_tlv_parse_ex( vt_tlv *tlv, vt_size *sz, int *isptr )
{
	if( tlv->type1 == VT_TYPE_CLOCK || tlv->type1 == VT_TYPE_IDENTIFIER )
	{
		*isptr = 0;
		*sz = sizeof(tlv->val_b16);
		return tlv->val_b16;
	}
	else if( tlv->type1 == VT_TYPE_CLASS || tlv->type1 == VT_TYPE_ARRAY ||
			 tlv->type1 == VT_TYPE_CSTRING || tlv->type1 == VT_TYPE_BSTRING )
	{
		*isptr = 1;
		*sz = tlv->len;
		return tlv->val.p;
	}
	else if( tlv->len <= sizeof(vt_value) )
	{
		*isptr = 0;
		*sz = tlv->len;
		return &tlv->val;
	}
	*sz = 0;
	return NULL;
}


typedef struct
{
#if 0
	union{
		vt_uint tag;
#ifdef _ARCH_ENDIAN_LITTLE
	struct {
		vt_byte typ1;
		vt_byte typ2;
		vt_byte typ3;
		vt_byte typ4;
	};
#else
	struct {
		vt_byte typ4;
		vt_byte typ3;
		vt_byte typ2;
		vt_byte typ1;
	};
#endif
	};
#endif
	vt_refer key; // points to char*, not vt_cstr*
	vt_value val;
}vt_keyval, vt_dict;


typedef struct
{
	vt_refer name;
	vt_byte  choice;    // must 1
	vt_byte  dtype;		// only long or double is available
	vt_byte  min_opt;   // reserved
	vt_byte  max_opt;   // reserved
	vt_value min;
	vt_value max;
	vt_value step;
}vt_range;

// opt
#define VT_CONSTRAINT_RANGE_UNLIMITED	(0)
#define VT_CONSTRAINT_RANGE_CLOSED		(1)
#define VT_CONSTRAINT_RANGE_OPENED		(2)


typedef struct
{
	vt_refer name;
	vt_byte  choice;    // must 2
	vt_byte  compare;	// compare key(1) or value(0)
	vt_byte  res[2];
	vt_uint  nitem;
	vt_dict  *items;
	//vt_dict  **sorted;
}vt_enumerate;

typedef struct
{
	vt_refer name;
	vt_byte  choice;    // must 3
	vt_byte  canon;     // canonical form of the expression(user-defined)
	vt_byte  res[2];
	vt_refer expr;
}vt_regularization;

typedef enum
{
	VT_CONSTRAINT_CHOICE_RANGE		= 1,
	VT_CONSTRAINT_CHOICE_ENUM		= 2,
	VT_CONSTRAINT_CHOICE_REGU		= 3,
}vt_constraint_choice;


#define VTRACE_DLNODE(__type__) union{ \
	struct { \
		union{ \
			struct __type__ *_prior; \
			struct __type__ *_above; \
			struct __type__ *_up;    \
			struct __type__ *_left;  \
		}; \
		union{ \
			struct __type__ *_next;  \
			struct __type__ *_below; \
			struct __type__ *_down;  \
			struct __type__ *_right; \
		}; \
	}; \
	dlk_node _dlk; \
	char _dat[0];  \
}


#define VTRACE_SLNODE(__type__) union{ \
		struct __type__ *_next;  \
		slk_node _slk; \
		char _dat[0];  \
}


// magic contents
#define VTRACE_MAGIC_LEN	4  // 8

typedef union _vtrace_magic_t
{
#if 0
	struct {  // special for trace point
		vt_dword size;		// size of specific-structure, for compatibility
		vt_word  type;		// type
		vt_byte  flag[0];	// purpose-specific
	};
	vt_byte b[VTRACE_MAGIC_LEN];
#endif

	vt_dword  mark;
}vt_magic;

#if defined( _ARCH_BIT64 )
static inline vt_dword VTRACE_MAGIC_MARK_CHECK( void *ptr )
{
	size_t _M_ = (~sizeof(vt_magic) ^ (size_t)(ptr));
	return ((_M_ >> 32) ^ _M_) & 0x0FFFFFFFF;
}
#else
#define VTRACE_MAGIC_MARK_CHECK(_PTR_)	( ~sizeof(vt_magic) ^ (size_t)(ptr) )
#endif

#define VTRACE_MAGIC_FLAG_OFFSET	((long)((vt_magic*)0)->flag)

#define VTRACE_MAGIC_TYPE_POINT				(1)
#define VTRACE_MAGIC_TYPE_PTNODE			(2)
#define VTRACE_MAGIC_TYPE_DPOINT			(3)
#define VTRACE_MAGIC_TYPE_SOT				(4)
#define VTRACE_MAGIC_TYPE_SOTNODE			(5)
#define VTRACE_MAGIC_TYPE_ACTION			(6)
#define VTRACE_MAGIC_TYPE_EVENT				(7)
#define VTRACE_MAGIC_TYPE_EVENT_ENTITY		(8)
#define VTRACE_MAGIC_TYPE_CLASS				(9)
#define VTRACE_MAGIC_TYPE_DATAKLASS			(10)
#define VTRACE_MAGIC_TYPE_DATAOBJECT		(11)
#define VTRACE_MAGIC_TYPE_DATOBJ_NODE		(12)
#define VTRACE_MAGIC_TYPE_DATOBJ_JOINT		(13)
#define VTRACE_MAGIC_TYPE_TLV				(14)
#define VTRACE_MAGIC_TYPE_DATOBJ_REPLICA	(15)



#define VTRACE_DLKNODE_HEAD(__type__) struct { \
	VTRACE_DLNODE(__type__); \
	vt_magic    magic;	\
}
struct _vtrace_dlnode_t;
struct _vtrace_slnode_t;
typedef VTRACE_DLKNODE_HEAD(_vtrace_dlnode_t) vt_dlknode_h;


// experimenting...
typedef struct _vtrace_cache_item
{
	union {
		btree_node btnd;

		struct {
			btree_node_head clue;
			union {
				long   key;
				size_t val;
				time_t atm;    // last access time
				size_t nhit;   // hit number
			};
		};
	};
	void *args[0];
}vt_cache_item;

// experimenting...
typedef struct
{
	vt_long num;
	vt_long size;
	vt_cache_item *root;
}vt_cache_box;



#pragma pack(pop)


#ifdef __cplusplus
extern "C"{
#endif



#ifdef __cplusplus
}
#endif

#endif // _VTRACE_DEF_H_
