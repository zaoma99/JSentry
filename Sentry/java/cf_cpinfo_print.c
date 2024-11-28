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

#include <stdio.h>
#include <string.h>
#include "classfile_util.h"


//
static int _print_cp_info( const CP_Info *cpi, int local_endian, FILE *fout )
{
	return fprintf( fout, "tag%d:{}", cpi->tag );
}

//
static CF_UNUSED int _print_cp_utf8_fast( CP_Utf8 *cpi, int local_endian, FILE *fout )
{
	cf_count_t ln;

#ifdef xx_ARCH_ENDIAN_LITTLE
    if( !(local_endian & CF_MAP_LOCAL_ENDIAN_VALUE) )
    	ln = _CF_ENDIAN_FLIP_16( cpi->len );
    else
#endif
    	ln = cpi->len;


	char c = cpi->b[ln];
	cpi->b[ln] = 0;
	int r = fprintf( fout, "%s:{len:%d, utf8:%s}", CF_CP_NAME[cpi->tag], ln, cpi->b );
	cpi->b[ln] = c;
	return r;
}

static int _print_cp_utf8( const CP_Utf8 *cpi, int local_endian, FILE *fout )
{
	/*
	cf_count_t ln;

#ifdef xx_ARCH_ENDIAN_LITTLE
    if( !(local_endian & CF_MAP_LOCAL_ENDIAN_VALUE) )
    	ln = _CF_ENDIAN_FLIP_16( cpi->len );
    else
#endif
    	ln = cpi->len;

	char *s = malloc( ln+1 );
	if( s )
	{
		memcpy( s, cpi->b, ln );
		s[ln] = 0;
	}

	int r = fprintf( fout, "%s:{len:%d, utf8:%s}", CF_CP_NAME[cpi->tag], ln, s?s:"*OOM*" );

	if( s )
		free( s );
	*/

	int r, n;

	r = fprintf( fout, "%s:{len:%d, utf8:", CF_CP_NAME[cpi->tag], cpi->len );
	if( r > 0 )
	{
		n = (int)fwrite( cpi->b, 1, cpi->len, fout );
		if( n > 0 )
			r += n;

		n = fprintf( fout, "}" );
		if( n > 0 )
			r += n;
	}
	return r;
}

//
static int _print_cp_integer( const CP_Integer *cpi, int local_endian, FILE *fout )
{
	int i;

#ifdef xx_ARCH_ENDIAN_LITTLE
    if( !(local_endian & CF_MAP_LOCAL_ENDIAN_VALUE) )
    	i = _CF_ENDIAN_FLIP_32( cpi->i32 );
    else
#endif
    	i = cpi->i32;

	return fprintf( fout, "%s:{value:%d}", CF_CP_NAME[cpi->tag], i );
}

//
static int _print_cp_float( const CP_Float *cpi, int local_endian, FILE *fout )
{
	union{
		float f;
		int i;
	}un = {.f = cpi->f32};

#ifdef xx_ARCH_ENDIAN_LITTLE
    if( !(local_endian & CF_MAP_LOCAL_ENDIAN_VALUE) )
    	_CF_ENDIAN_SWAP_U4( un.i );
#endif

	return fprintf( fout, "%s:{value:%f}", CF_CP_NAME[cpi->tag], un.f );
}

//
static int _print_cp_long( const CP_Long *cpi, int local_endian, FILE *fout )
{
	long long ll = cpi->i64;

#ifdef xx_ARCH_ENDIAN_LITTLE
    if( !(local_endian & CF_MAP_LOCAL_ENDIAN_VALUE) )
    	_CF_ENDIAN_SWAP_U8( ll );
#endif

	return fprintf( fout, "%s:{value:%lld}", CF_CP_NAME[cpi->tag], ll );
}

//
static int _print_cp_double( const CP_Double *cpi, int local_endian, FILE *fout )
{
	union{
		double d;
		long long l;
	}un = {.d = cpi->f64};

#ifdef xx_ARCH_ENDIAN_LITTLE
    if( !(local_endian & CF_MAP_LOCAL_ENDIAN_VALUE) )
    	_CF_ENDIAN_SWAP_U8( un.l );
#endif

	return fprintf( fout, "%s:{value:%f}", CF_CP_NAME[cpi->tag], un.d );
}

//
static int _print_cp_class( const CP_Class *cpi, int local_endian, FILE *fout )
{
	cf_idx_t i = cpi->name_idx;

#ifdef xx_ARCH_ENDIAN_LITTLE
    if( !(local_endian & CF_MAP_LOCAL_ENDIAN_VALUE) )
    	_CF_ENDIAN_SWAP_INDEX( i );
#endif

	return fprintf( fout, "%s:{index:%d}", CF_CP_NAME[cpi->tag], i );
}

#define _print_cp_string		_print_cp_class
#define _print_cp_methodtype	_print_cp_class
#define _print_cp_module		_print_cp_class
#define _print_cp_package		_print_cp_class

//
static int _print_cp_field( const CP_Fieldref *cpi, int local_endian, FILE *fout )
{
	cf_idx_t i = cpi->class_idx;
	cf_idx_t j = cpi->nm_tp_idx;

#ifdef xx_ARCH_ENDIAN_LITTLE
    if( !(local_endian & CF_MAP_LOCAL_ENDIAN_VALUE) )
    {	_CF_ENDIAN_SWAP_INDEX( i ); _CF_ENDIAN_SWAP_INDEX( j ); }
#endif

	return fprintf( fout, "%s:{class_idx:%d, name_type_idx:%d}", CF_CP_NAME[cpi->tag], i, j );
}

#define _print_cp_method		_print_cp_field
#define _print_cp_ifmethod		_print_cp_field


//
static int _print_cp_nametype( const CP_NameAndType *cpi, int local_endian, FILE *fout )
{
	cf_idx_t i = cpi->name_idx;
	cf_idx_t j = cpi->desc_idx;

#ifdef xx_ARCH_ENDIAN_LITTLE
    if( !(local_endian & CF_MAP_LOCAL_ENDIAN_VALUE) )
    {	_CF_ENDIAN_SWAP_INDEX( i ); _CF_ENDIAN_SWAP_INDEX( j ); }
#endif

	return fprintf( fout, "%s:{name_idx:%d, desc_idx:%d}", CF_CP_NAME[cpi->tag], i, j );
}

//
static int _print_cp_dynamic( const CP_Dynamic *cpi, int local_endian, FILE *fout )
{
	cf_idx_t i = cpi->bootstrap_attr_idx;
	cf_idx_t j = cpi->name_type_idx;

#ifdef xx_ARCH_ENDIAN_LITTLE
    if( !(local_endian & CF_MAP_LOCAL_ENDIAN_VALUE) )
    {	_CF_ENDIAN_SWAP_INDEX( i ); _CF_ENDIAN_SWAP_INDEX( j ); }
#endif

	return fprintf( fout, "%s:{bootstrap_method_attr_idx:%d, name_type_idx:%d}", CF_CP_NAME[cpi->tag], i, j );
}

#define _print_cp_invokedynamic		_print_cp_dynamic

//
static int _print_cp_methodhandle( const CP_MethodHandle *cpi, int local_endian, FILE *fout )
{
	cf_idx_t i = cpi->ref_idx;

#ifdef xx_ARCH_ENDIAN_LITTLE
    if( !(local_endian & CF_MAP_LOCAL_ENDIAN_VALUE) )
    	_CF_ENDIAN_SWAP_INDEX( i );
#endif

	return fprintf( fout, "ref_kind=%s, ref_idx=%d}", CF_DYNREF_KIND_NAME[cpi->ref_kind], i );
}


//
typedef int (*FUNPTR_PRINT_CPINFO)( const CP_Info*, int, FILE* );

static FUNPTR_PRINT_CPINFO cf_func_print_cpinfo[] = {
		(FUNPTR_PRINT_CPINFO)_print_cp_info,		/* unused */
		(FUNPTR_PRINT_CPINFO)_print_cp_utf8,
		(FUNPTR_PRINT_CPINFO)_print_cp_info,		/* unused */
		(FUNPTR_PRINT_CPINFO)_print_cp_integer,
		(FUNPTR_PRINT_CPINFO)_print_cp_float,
		(FUNPTR_PRINT_CPINFO)_print_cp_long,
		(FUNPTR_PRINT_CPINFO)_print_cp_double,
		(FUNPTR_PRINT_CPINFO)_print_cp_class,
		(FUNPTR_PRINT_CPINFO)_print_cp_string,
		(FUNPTR_PRINT_CPINFO)_print_cp_field,
		(FUNPTR_PRINT_CPINFO)_print_cp_method,
		(FUNPTR_PRINT_CPINFO)_print_cp_ifmethod,
		(FUNPTR_PRINT_CPINFO)_print_cp_nametype,
		(FUNPTR_PRINT_CPINFO)_print_cp_info,		/* unused */
		(FUNPTR_PRINT_CPINFO)_print_cp_info,		/* unused */
		(FUNPTR_PRINT_CPINFO)_print_cp_methodhandle,
		(FUNPTR_PRINT_CPINFO)_print_cp_methodtype,
		(FUNPTR_PRINT_CPINFO)_print_cp_dynamic,
		(FUNPTR_PRINT_CPINFO)_print_cp_invokedynamic,
		(FUNPTR_PRINT_CPINFO)_print_cp_module,
		(FUNPTR_PRINT_CPINFO)_print_cp_package
};

static const int _FUNC_PRINT_COUNT_ = sizeof(cf_func_print_cpinfo) / sizeof(FUNPTR_PRINT_CPINFO);


// Function:
int cf_print_cpinfo( const CP_Info *cpi, int local_endian, FILE *fout )
{
	return (cpi && cpi->tag >= 0 && cpi->tag < _FUNC_PRINT_COUNT_) ? cf_func_print_cpinfo[cpi->tag]( cpi, local_endian, fout ) : -1;
}



// Function:
void cf_print_cptab( const CF_ClassFileMap *cf_map, FILE *fout )
{
	cf_count_t c, n, m;
	CP_Info **cp;
	int local_endian = (cf_map->auxi.local_endian_value<<1 | cf_map->auxi.local_endian_index) & 0x03;

	for( n=0; n < cf_map->cptab_count; n++ )
	{
		cp = cf_map->cptab[n].cps;
		m = cf_map->cptab[n].cp_count;

		for( c=1; c < m; c++ )
		{
			if( cp[c] )
			{
				fprintf( fout, "cp[%d] = ", c );
				cf_print_cpinfo( cp[c], local_endian, fout );
				fprintf( fout, "\n" );
			}
		}
		fprintf( fout, "total cp: %d\n", c );
	}
}
