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

#ifndef _LINEAR_TABLE_H_
#define _LINEAR_TABLE_H_

#include "common/pdefine.h"
#include "mmp.h"


typedef unsigned int lt_size;
typedef void *lt_dtype;


//
typedef struct _lntab_array
{
	lt_size  num;
	lt_dtype *ele;
}lt_array;

//
typedef struct _lntab_matrix_2d
{
	lt_size  narr;
	lt_array *arr;
	struct {
		lt_size r, c;
	}grow;
}lt_mat2d;

typedef struct _lntab_matrix
{
	char res;
	unsigned char dtype;
	unsigned char unit;
	unsigned char ndim;
	union {
		struct { lt_size r, c; };
		lt_size *d;
	}dim;
	union {
		struct { lt_size r, c; };
		lt_size *s;
	}stride;
	union {
		struct { lt_size r, c; };
		lt_size *g;
	}grow;
	lt_array *arr;
}lt_mat;



#ifdef __cplusplus
extern "C"{
#endif


#ifdef __cplusplus
}
#endif

#endif //_LINEAR_TABLE_H_
