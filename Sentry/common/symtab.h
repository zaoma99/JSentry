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

#ifndef _SYMTAB_H_
#define _SYMTAB_H_

#include "stypes.h"


typedef struct
{
	size_t  len;
	union{
		char *str;  // utf8
		BYTE *val;
	};
}symtab_symbol;

typedef struct
{
	uint nsym;
	symtab_symbol *syms;
}symtab_group;

typedef struct
{
	uint grp_caps;
	uint ngrp;
	symtab_group *grp;
	long mpool;
}symtab;

//
static inline void* symtab_match_in_group( symtab_group *grp, const char *sym, size_t len )
{
	register uint i;
	register symtab_symbol *syms = grp->syms;
	for( i=grp->nsym;
		 i > 0
		 && (len!=syms[i-1].len || strncmp(sym, syms[i-1].str, len)!=0);
		 i--
		 );
	return i==0 ? NULL : &syms[i];
}

//
static inline void* symtab_match2_in_group( symtab_group *grp, const BYTE *val, size_t len )
{
	register uint i;
	register symtab_symbol *syms = grp->syms;
	for( i=grp->nsym;
		 i > 0
		 && (len!=syms[i-1].len || memcmp(val, syms[i-1].val, len)!=0);
		 i--
		 );
	return i==0 ? NULL : &syms[i];
}

#ifdef __cplusplus
extern "C" {
#endif

extern int symtab_init( symtab *tab, uint grp_caps );
extern void symtab_release( symtab *tab );
extern void* symtab_add( symtab *tab, const char *sym, size_t len );
extern void symtab_del( symtab *tab, void* ref );
extern void* symtab_find( const symtab *tab, const char *sym, size_t len );
extern void* symtab_find2( const symtab *tab, const BYTE *val, size_t len );

#ifdef VT_SYMBOL_HIDDEN_POINTER
VT_EXPORT void* symtab_get( symtab *tab, void* ref );
#else
#define symtab_get( _TABLE__, __REF__ )	(__REF__)
#endif

#ifdef __cplusplus
}
#endif

#endif //_SYMTAB_H_
