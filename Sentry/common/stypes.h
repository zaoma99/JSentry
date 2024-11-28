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



#ifndef _AS_TYPES_H_
#define _AS_TYPES_H_

#include <sys/types.h>
#include "common/ptypes.h"
#include "common/iobuf.h"

typedef long long SSIZE_T;
typedef unsigned long long SIZE_T;
typedef void *voidptr_t;
typedef void *refer_t;
typedef void *object_t;
typedef void *handle_t;

typedef union
{
	IOBUF_T vbuf;

	struct{
		char  *addr;	// 存储区首地址
		size_t size;	// 容量
		size_t len;		// 有效字节数
		union{
			size_t pos; // 当前指针
			char  *ptr;
		};
	};

#ifdef __cplusplus
#endif
}vbuf_t;

typedef vbuf_t     vstr_t;
typedef vstr_t     vpath_t;

#define VBUF_OP( __op__, __args__... )    IOBUF_##__op__( __args__ )
#define VSTR_OP    VBUF_OP



#endif	// _AS_TYPES_H_
