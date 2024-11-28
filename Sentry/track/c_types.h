/* ==============================================================================================================
 * Author: CXX
 * Date: 2022-06-05
 * Version:
 * Copyright (C) CXX, All rights reserved
 * Description:
 * History:
 * 20220605: C header file, be created
 * ==============================================================================================================
 */

#ifndef _TRACK_C_TYPES_H_
#define _TRACK_C_TYPES_H_

#include <sys/types.h>
#include "common/ptypes.h"


#pragma pack(push, 4)

typedef enum
{
	C_TYPE_NULL       = 0  ,    //
	C_TYPE_SI8        = 1  ,    // signed integer within 1-byte
	C_TYPE_SI16       = 2  ,    // signed integer within 2-byte
	C_TYPE_SI32       = 3  ,    // signed integer within 4-byte
	C_TYPE_SI64       = 4  ,    // signed integer within 8-byte
	C_TYPE_SI128      = 5  ,    // signed integer within 16-byte(reserved)
	C_TYPE_UI8        = 6  ,    // unsigned integer within 1-byte
	C_TYPE_UI16       = 7  ,    // unsigned integer within 2-byte
	C_TYPE_UI32       = 8  ,    // unsigned integer within 4-byte
	C_TYPE_UI64       = 9  ,    // unsigned integer within 8-byte
	C_TYPE_UI128      = 10 ,    // unsigned integer within 16-byte(reserved)
	C_TYPE_F32        = 11 ,    // IEEE-754 binary32 floating-point format
	C_TYPE_F64        = 12 ,    // IEEE-754 binary64 floating-point format
	C_TYPE_F128       = 13 ,    // reserved
	C_TYPE_POINTER    = 14 ,    // same as the void*, pointer of address space
	C_TYPE_TIME       = 15 ,    // same as the time_t
	C_TYPE_CLOCK      = 16 ,    // same as the struct timespec
	C_TYPE_IDENTIFIER = 17 ,	// object identifier
	C_TYPE_CSTRING    = 18 ,    // visible character string(UTF-8 encoding)
	C_TYPE_BSTRING    = 19 ,    // byte string
	//
	C_TYPE_SET        = 29 ,
	C_TYPE_ARRAY      = 30 ,
	//
	C_TYPE_BOOL          = C_TYPE_UI8  ,
	C_TYPE_BYTE          = C_TYPE_UI8  ,
	C_TYPE_OCTET         = C_TYPE_UI8  ,
	C_TYPE_CHAR          = C_TYPE_SI8  ,
	C_TYPE_WCHAR         = C_TYPE_UI16 ,    // wide character(UTF-16 encoding)
	C_TYPE_WORD          = C_TYPE_UI16 ,
	C_TYPE_SHORT         = C_TYPE_SI16 ,
	C_TYPE_INT           = C_TYPE_SI32 ,
	C_TYPE_UINT          = C_TYPE_UI32 ,
	C_TYPE_LONG          = C_TYPE_SI64 ,
	C_TYPE_ULONG         = C_TYPE_UI64 ,
	C_TYPE_FLOAT         = C_TYPE_F32  ,
	C_TYPE_DOUBLE        = C_TYPE_F64  ,
#if defined( _ARCH_BIT64 )
	C_TYPE_OFFSET        = C_TYPE_LONG ,
	C_TYPE_SSIZE         = C_TYPE_LONG,
	C_TYPE_SIZE          = C_TYPE_ULONG,
#else
	C_TYPE_OFFSET        = C_TYPE_INT ,
	C_TYPE_SSIZE         = C_TYPE_INT,
	C_TYPE_SIZE          = C_TYPE_UINT,
#endif
	C_TYPE_ENUM          = C_TYPE_INT  ,
	C_TYPE_REFERENCE     = C_TYPE_IDENTIFIER ,
	C_TYPE_PLACEHOLDER   = C_TYPE_IDENTIFIER ,
	//
	C_TYPE_PRIMITIVE_BEG = C_TYPE_SI8 ,
	C_TYPE_PRIMITIVE_END = C_TYPE_BSTRING ,
}C_TYPES;

typedef signed char        C_SI8;
typedef signed short       C_SI16;
typedef signed int         C_SI32;
typedef signed long long   C_SI64;
typedef unsigned char      C_UI8;
typedef unsigned short     C_UI16;
typedef unsigned int       C_UI32;
typedef unsigned long long C_UI64;
typedef float              C_F32;
typedef double             C_F64;
typedef void              *C_POINTER;

//typedef C_UI8        C_IDENTIFIER[16];

typedef union
{
	C_UI32 i;
	C_UI64 l;
	C_UI8  b[16];
}C_IDENTIFIER;

typedef C_UI8        C_BOOL;
typedef C_UI8        C_BYTE;
typedef C_UI8        C_OCTET;
typedef C_SI8        C_CHAR;
typedef C_UI16       C_WCHAR;
typedef C_UI16       C_WORD;
typedef C_SI16       C_SHORT;
typedef C_SI32       C_INT;
typedef C_UI32       C_UINT;
typedef C_SI64       C_LONG;
typedef C_UI64       C_ULONG;
typedef C_F32        C_FLOAT;
typedef C_F64        C_DOUBLE;

// typedef size_t C_SIZE;
// typedef time_t C_TIME;
// typedef struct timespec C_CLOCK;

#if defined( _ARCH_BIT64 )
typedef C_ULONG      C_SIZE;
typedef C_LONG       C_OFFSET;
typedef C_LONG       C_TIME;
#else
typedef C_UINT       C_SIZE;
typedef C_INT        C_OFFSET;
typedef C_INT        C_TIME;
#endif

typedef struct    // same as the struct timespec
{
	C_TIME tv_sec;
	C_TIME tv_nsec;
}C_CLOCK;

typedef C_INT        C_ENUM;
typedef C_POINTER    C_REFERENCE;
typedef C_POINTER    C_PLACEHOLDER;

#define C_TYPE_IS_PREMITIVE( _TYPE_CODE_ )	( (_TYPE_CODE_) <= C_TYPE_PRIMITIVE_END )

#define C_TYPE_IS_VALID( _TYPE_CODE_ )	(\
	((_TYPE_CODE_) > 0 && (_TYPE_CODE_) <= C_TYPE_PRIMITIVE_END) \
	|| (_TYPE_CODE_) == C_TYPE_SET \
	|| (_TYPE_CODE_) == C_TYPE_ARRAY \
	)

#define C_TYPES_SIZE_INITIATOR { \
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
	sizeof(C_POINTER), \
	sizeof(C_TIME), \
	sizeof(C_CLOCK), \
	sizeof(C_IDENTIFIER), \
	-1, /* CSTRING */ \
	-1, /* BSTRING */ \
	 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 20~28, reserved */ \
	-1, /* SET */ \
	-1  /* ARRAY */ \
}


#define _BUFFER_1_(_DTYPE_) struct \
{ \
	C_SIZE len; \
	union{ \
		_DTYPE_  a[sizeof(void *)]; \
		_DTYPE_ *p; \
	}val; \
}
#define _BUFFER_1_Ptr(_BUF_)	( (_BUF_)->len>sizeof((_BUF_)->val) ? (_BUF_)->val.p : (_BUF_)->val.a )


typedef _BUFFER_1_(char) C_CSTR;
#define C_CSTR_BufPtr(_CSTR_)	_BUFFER_1_Ptr(_CSTR_)

typedef _BUFFER_1_(unsigned char) C_BSTR;
#define C_BSTR_BufPtr	C_CSTR_BufPtr



#define _C_MAKE_I16_( _H_, _L_ )	\
	( (_H_)<<8 | (_L_) )

#define _C_MAKE_I32_( _H3_, _H2_, _H1_, _L_)	\
	( ((_H3_)<<24) | ((_H2_)<<16) | ((_H1_)<<8) | (_L_) )

#define _C_MAKE_I64_( _H7_, _H6_, _H5_, _H4_, _H3_, _H2_, _H1_, _L_)	\
	( ((_H7_)<<56) | ((_H6_)<<48) | ((_H5_)<<40) | ((_H4_)<<32) | ((_H3_)<<24) | ((_H2_)<<16) | ((_H1_)<<8) | (_L_) )


#ifdef _ARCH_ENDIAN_LITTLE

#define _C_ENDIAN_FLIP_I64(__LL64__)\
	((((__LL64__) & 0x00FF) << 56) | \
	 (((__LL64__) & 0x00FF00) <<  40) | \
	 (((__LL64__) & 0x00FF0000) <<  24) | \
	 (((__LL64__) & 0x00FF000000) <<  8) | \
	 (((__LL64__) & 0x00FF00000000) >>  8) | \
	 (((__LL64__) & 0x00FF0000000000) >>  24) | \
	 (((__LL64__) & 0x00FF000000000000) >>  40) | \
	 (((__LL64__) & 0xFF00000000000000) >> 56))

#define _C_ENDIAN_FLIP_I32(__I32__)\
	((((__I32__) & 0x000000FF) << 24) | \
	 (((__I32__) & 0x0000FF00) <<  8) | \
	 (((__I32__) & 0x00FF0000) >>  8) | \
	 (((__I32__) & 0xFF000000) >> 24))

#define _C_ENDIAN_FLIP_I16(__I16__)\
	((((__I16__) & 0x00FF) << 8) | \
	 (((__I16__) & 0xFF00) >> 8))

#define _C_ENDIAN_SWAP_U2(__U2__)	( __U2__ = _C_ENDIAN_FLIP_I16( __U2__ ) )
#define _C_ENDIAN_SWAP_U4(__U4__)	( __U4__ = _C_ENDIAN_FLIP_I32( __U4__ ) )
#define _C_ENDIAN_SWAP_U8(__U8__)	( __U8__ = _C_ENDIAN_FLIP_I64( __U8__ ) )

inline static float _C_ENDIAN_FLIP_FLOAT( float f )
{
	union{float f; int i;} un = {.f = f};
	_C_ENDIAN_SWAP_U4( un.i );
	return un.f;
}

inline static double _C_ENDIAN_FLIP_DOUBLE( double d )
{
	union{double d; long long l;} un = {.d = d};
	_C_ENDIAN_SWAP_U8( un.l );
	return un.d;
}

#else

#define _C_ENDIAN_FLIP_I64(__LL64__)	(__LL64__)
#define _C_ENDIAN_FLIP_I32(__I32__)		(__I32__)
#define _C_ENDIAN_FLIP_I16(__I16__)		(__I16__)

#define _C_ENDIAN_SWAP_U2		_C_ENDIAN_FLIP_I16
#define _C_ENDIAN_SWAP_U4		_C_ENDIAN_FLIP_I32
#define _C_ENDIAN_SWAP_U8		_C_ENDIAN_FLIP_I64

#define _C_ENDIAN_FLIP_FLOAT(__F__)		(__F__)
#define _C_ENDIAN_FLIP_DOUBLE(__D__)	(__D__)

#endif // _ARCH_ENDIAN_LITTLE

#define _C_ENDIAN_FLIP_U2		_C_ENDIAN_FLIP_I16
#define _C_ENDIAN_FLIP_U4		_C_ENDIAN_FLIP_I32
#define _C_ENDIAN_FLIP_U8		_C_ENDIAN_FLIP_I64

#define _C_ENDIAN_FLIP_INT		_C_ENDIAN_FLIP_U4
#define _C_ENDIAN_SWAP_INT		_C_ENDIAN_SWAP_U4
#define _C_ENDIAN_FLIP_LONG		_C_ENDIAN_FLIP_U8
#define _C_ENDIAN_SWAP_LONG		_C_ENDIAN_SWAP_U8
#define _C_ENDIAN_FLIP_SHORT	_C_ENDIAN_FLIP_U2
#define _C_ENDIAN_SWAP_SHORT	_C_ENDIAN_SWAP_U2
#define _C_ENDIAN_FLIP_WCHAR	_C_ENDIAN_FLIP_U2
#define _C_ENDIAN_SWAP_WCHAR	_C_ENDIAN_SWAP_U2
#define _C_ENDIAN_FLIP_ENUM		_C_ENDIAN_FLIP_U4
#define _C_ENDIAN_SWAP_ENUM		_C_ENDIAN_SWAP_U4

#if defined( _ARCH_BIT64 )
#define _C_ENDIAN_FLIP_OFFSET	_C_ENDIAN_FLIP_U8
#define _C_ENDIAN_SWAP_OFFSET	_C_ENDIAN_SWAP_U8
#define _C_ENDIAN_FLIP_TIME		_C_ENDIAN_FLIP_U8
#define _C_ENDIAN_SWAP_TIME		_C_ENDIAN_SWAP_U8
#define _C_ENDIAN_FLIP_SIZE		_C_ENDIAN_FLIP_U8
#define _C_ENDIAN_SWAP_SIZE		_C_ENDIAN_SWAP_U8
#else
#define _C_ENDIAN_FLIP_OFFSET	_C_ENDIAN_FLIP_U4
#define _C_ENDIAN_SWAP_OFFSET	_C_ENDIAN_SWAP_U4
#define _C_ENDIAN_FLIP_TIME		_C_ENDIAN_FLIP_U4
#define _C_ENDIAN_SWAP_TIME		_C_ENDIAN_SWAP_U4
#define _C_ENDIAN_FLIP_SIZE		_C_ENDIAN_FLIP_U4
#define _C_ENDIAN_SWAP_SIZE		_C_ENDIAN_SWAP_U4
#endif

#define _C_ENDIAN_FLIP_POINTER(__PTR__)		_C_ENDIAN_FLIP_SIZE( (C_SIZE)(__PTR__) )
#define _C_ENDIAN_SWAP_POINTER(__PTR__)		_C_ENDIAN_SWAP_SIZE( (C_SIZE)(__PTR__) )


#pragma pack(pop)


#ifdef __cplusplus
extern "C"{
#endif



#ifdef __cplusplus
}
#endif

#endif // _TRACK_C_TYPES_H_
