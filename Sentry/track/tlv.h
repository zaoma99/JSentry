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

#ifndef _TRACK_TLV_H_
#define _TRACK_TLV_H_

#include "c_types.h"


#pragma pack(push, 1)


typedef union
{
	C_BYTE tag;

	struct{
#if defined( _ARCH_ENDIAN_BIG )
		C_BYTE k :2;
		C_BYTE c :1;
		C_BYTE t :5;
#else
		C_BYTE t :5;  // type
		C_BYTE c :1;  // constructed
		C_BYTE k :2;  // class
#endif
	};
}C_TLV_TAG;

#define C_TLV_TAG_CLS_UNI		0	// universal
#define C_TLV_TAG_CLS_APP		1	// application
#define C_TLV_TAG_CLS_SPEC		2	// context specific
#define C_TLV_TAG_CLS_PRIV		3	// private

#define C_TLV_TAG_TYPE_MASK		0x1f
#define C_TLV_TAG_TYPE_SHIFT	0
#define C_TLV_TAG_CONS_MASK		0x20
#define C_TLV_TAG_CONS_SHIFT	5
#define C_TLV_TAG_CLS_MASK		0xc0
#define C_TLV_TAG_CLS_SHIFT		6



#define C_TLV_LxV(_L_) \
struct { \
	C_BYTE len[_L_]; \
	C_BYTE val[0]; \
}

// TLV Reference
#define C_TLV_REF(_L_) \
struct { \
	C_TLV_TAG tag; \
	C_TLV_LxV(_L_); \
}

typedef C_TLV_REF(1)   C_TLV_1_REF;
typedef C_TLV_REF(2)   C_TLV_2_REF;
typedef C_TLV_REF(4)   C_TLV_4_REF;
typedef C_TLV_REF(8)   C_TLV_8_REF;


// TLV
typedef struct
{
	C_TLV_TAG tag;
	_BUFFER_1_(C_BYTE);
}C_TLV_BUF;

#if 0
inline static C_SIZE c_tlv_getlen( const C_BYTE *bs, C_SIZE bsl )
{
	(bs[0] & C_TLVHD_LEN_MASK)
}

inline static int c_tlv_setlen( C_SIZE len, C_BYTE *bs )
{
	;
}
#endif

#pragma pack(pop)


#ifdef __cplusplus
extern "C"{
#endif



#ifdef __cplusplus
}
#endif

#endif // _TRACK_TLV_H_
