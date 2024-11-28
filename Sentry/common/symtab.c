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
#include <symtab.h>
#include "mmp.h"

//
#define _SYMBOL_GROUP_INC_UNITS	(4)
#define _SYMBOL_GROUP_MAX			(0x7ffffff8)
//#define _SYMBOL_GROUP_CAPS		(512)
//#define _SYMBOL_GROUP_CAPS_SHF	(9)
#define _SYMBOL_MAGIC_VALUE		(0x1a8d3df0ee07c)

//
int symtab_init( symtab *tab, uint grp_caps )
{
	memset( tab, 0, sizeof(symtab) );
	tab->grp_caps = ALIGN_SIZE( grp_caps, 8 );
	mmp_create_mempool( 1, &tab->mpool );
	return 0;
}

//
void symtab_release( symtab *tab )
{
	mmp_destroy_mempool( 1, &tab->mpool );
	memset( tab, 0, sizeof(symtab) );
}

//
inline static int _symtab_group_inc( symtab *tab )
{
	uint n = ALIGN_SIZE( tab->ngrp, _SYMBOL_GROUP_INC_UNITS );
	if( n >= _SYMBOL_GROUP_MAX )
		return -1;

	void *ptr = mmp_realloc( 1, &tab->mpool, tab->grp, sizeof(symtab_group)*(n + _SYMBOL_GROUP_INC_UNITS) );
	if( !ptr )
		return -1;

	tab->grp = (symtab_group*)ptr;
	memset( &tab->grp[n], 0, sizeof(symtab_group)*_SYMBOL_GROUP_INC_UNITS );
	return 0;
}

//
#ifdef _SYMBOL_HIDDEN_POINTER
//inline static
symtab_symbol* symtab_get( symtab *tab, void* ref )
{
	ref = (void*)((size_t)ref ^ (size_t)tab ^ _SYMBOL_MAGIC_VALUE);
	register long g = (long)ref / tab->grp_caps;
	register long i = (long)ref & (tab->grp_caps-1);
	return g >= tab->ngrp ? NULL : &tab->grp[g].syms[i];
}
#endif

//
void* symtab_add( symtab *tab, const char *sym, size_t len )
{
	register long i, g = (long)tab->ngrp - 1;

	if( g == -1 || ((i=tab->grp[g].nsym) == tab->grp_caps) )
	{
		g++;
		if( (g & (_SYMBOL_GROUP_INC_UNITS-1)) == 0 )
		{
			if( _symtab_group_inc( tab ) != 0  )
				return NULL;
		}
		i = 0;
		tab->ngrp++;
	}

	if( !tab->grp[g].syms )
	{
		tab->grp[g].syms = mmp_malloc( 1, &tab->mpool, sizeof(symtab_symbol) * tab->grp_caps );
		if( !tab->grp[g].syms )
			return NULL;
		memset( tab->grp[g].syms, 0, sizeof(symtab_symbol) * tab->grp_caps );
	}

	register symtab_symbol *symbol = &tab->grp[g].syms[i];

	symbol->str = mmp_malloc( 1, &tab->mpool, len+1 );
	if( !symbol->str )
		return NULL;

	if( sym )
	{
		memcpy( symbol->str, sym, len );
		symbol->str[len] = 0;
	}
	else
		symbol->str[0] = 0;
	symbol->len = len;

	tab->grp[g].nsym++;

#ifdef VT_SYMBOL_HIDDEN_POINTER
	++i;
	return (void*)((((size_t)g * tab->grp_caps) | i) ^ (size_t)tab ^ _SYMBOL_MAGIC_VALUE);
#else
	return symbol;
#endif
}

//
void symtab_del( symtab *tab, void* ref )
{
	register symtab_symbol *cs = symtab_get( tab, ref );
	if( !cs )
	{
		if( cs->str )
			mmp_free( 1, &tab->mpool, cs->str );
		cs->str = NULL;
		cs->len = 0;
	}
}

//
void* symtab_find( const symtab *tab, const char *sym, size_t len )
{
	register void* r = NULL;
	register symtab_group *grp = tab->grp;
	register uint i;

	for( i = tab->ngrp;
		 i > 0 && !(r=symtab_match_in_group(&grp[--i], sym, len));
		);
	return r;
}

//
void* symtab_find2( const symtab *tab, const BYTE *val, size_t len )
{
	register void* r = NULL;
	register symtab_group *grp = tab->grp;
	register uint i;

	for( i = tab->ngrp;
		 i > 0 && !(r=symtab_match2_in_group(&grp[--i], val, len));
		);
	return r;
}

