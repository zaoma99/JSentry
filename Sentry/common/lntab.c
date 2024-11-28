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

#include "lntab.h"

//
#define LT_ARRAY_GROW_ROW_UNITS	(4)
#define LT_ARRAY_GROW_COL_UNITS	(8)
#define LT_ARRAY_MAX			(0x7ffffff8)


//
int lt_mat2d_init( lt_mat2d *mat, lt_size grow_row, lt_size grow_col )
{
	memset( mat, 0, sizeof(lt_mat2d) );
	mat->grow.r = ALIGN_SIZE( grow_row, LT_ARRAY_GROW_ROW_UNITS );
	mat->grow.c = ALIGN_SIZE( grow_col, LT_ARRAY_GROW_COL_UNITS );
	return 0;
}

//
void lt_mat2d_free( lt_mat2d *mat )
{
	if( mat->arr )
	{
		lt_size i, m = mat->narr;
		for( i=0; i < m; i++ )
		{
			if( mat->arr[i].ele )
				free( mat->arr[i].ele );
		}
		free( mat->arr );
	}
	memset( mat, 0, sizeof(lt_mat2d) );
}

//
inline static int lt_mat2d_array_grow( lt_mat2d *mat )
{
	lt_size n = ALIGN_SIZE( mat->narr, LT_ARRAY_GROW_ROW_UNITS );
	if( n >= LT_ARRAY_MAX )
		return -1;

	void *ptr = realloc( mat->arr, sizeof(lt_array)*(n + LT_ARRAY_GROW_ROW_UNITS) );
	if( !ptr )
		return -1;

	mat->grp = (struct _vtrace_symgrp*)ptr;
	memset( &mat->grp[n], 0, sizeof(struct _vtrace_symgrp)*LT_ARRAY_GROW_UNITS );
	return 0;
}

//
inline static
vt_cstr* lt_mat2d_get( lt_mat2d *mat, vt_refer ref )
{
	register long g = (long)ref / mat->grp_caps;
	register long i = (long)ref & (mat->grp_caps-1);
	return g >= mat->ngrp ? NULL : &mat->grp[g].syms[i];
}

//
vt_refer lt_mat2d_add( lt_mat2d *mat, const char *sym, size_t len )
{
	register long i, g = (long)mat->ngrp - 1;

	if( g == -1 || ((i=mat->grp[g].nsym) == mat->grp_caps) )
	{
		g++;
		if( (g & (LT_ARRAY_GROW_UNITS-1)) == 0 )
		{
			if( lt_mat2d_group_inc( mat ) != 0  )
				return NULL;
		}
		i = 0;
		mat->ngrp++;
	}

	if( !mat->grp[g].syms )
	{
		mat->grp[g].syms = mmp_malloc( 1, &mat->mpool, sizeof(vt_cstr) * mat->grp_caps );
		if( !mat->grp[g].syms )
			return NULL;
		memset( mat->grp[g].syms, 0, sizeof(vt_cstr) * mat->grp_caps );
	}

	register vt_cstr *cs = &mat->grp[g].syms[i];

	cs->cs = mmp_malloc( 1, &mat->mpool, len+1 );
	if( !cs->cs )
		return NULL;

	if( sym )
	{
		memcpy( cs->cs, sym, len );
		cs->cs[len] = 0;
	}
	else
		cs->cs[0] = 0;
	cs->len = len;

	mat->grp[g].nsym++;

#ifdef VT_SYMBOL_HIDDEN_POINTER
	++i;
	return (vt_refer)((((size_t)g * mat->grp_caps) | i) ^ (size_t)mat ^ VT_SYMBOL_MAGIC_VALUE);
#else
	return cs;
#endif
}

//
void lt_mat2d_del( lt_mat2d *mat, vt_refer ref )
{
	register vt_cstr *cs = lt_mat2d_get( mat, ref );
	if( !cs )
	{
		if( cs->cs )
			mmp_free( 1, &mat->mpool, cs->cs );
		cs->cs = NULL;
		cs->len = 0;
	}
}
