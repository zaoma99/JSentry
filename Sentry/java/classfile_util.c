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
#include <stdlib.h>
#include <string.h>

#include "classfile_util.h"
#include "cf_impl.h"

#include "common/error_user.h"

#ifdef CF_MEMPOOL
#include "advLib/usrMMU.h"
#endif

#pragma GCC diagnostic ignored "-Wunused-value"


#if 0  // replaced by mmp.c
//
struct _cf_mbnode_t
{
	struct _cf_mbnode_t *next;
	struct _cf_mbnode_t *prior;
};


//
CF_HIDDEN void* cf_malloc( int sys_mmp, long *mmp, size_t size )
{
	if( sys_mmp )
	{
		void *ptr = malloc( size + sizeof(struct _cf_mbnode_t) );
		if( !ptr )
			return NULL;

		struct _cf_mbnode_t *nd = (struct _cf_mbnode_t*)ptr;
		nd->next = NULL;
		nd->prior = (void*)*mmp;

		if( *mmp )
			((struct _cf_mbnode_t*)*mmp)->next = ptr;

		*mmp = (long)ptr;  // always pointer to the tail

		return ptr+sizeof(struct _cf_mbnode_t);
	}

#ifndef CF_MEMPOOL
	errno = errNotSupport;
	return NULL;
#else
	return (void*)UM_Alloc( *(UM_ID*)mmp, size );
#endif
}

//
CF_HIDDEN void cf_free( int sys_mmp, long *mmp, void *ptr )
{
	if( sys_mmp )
	{
		struct _cf_mbnode_t *_nd = (struct _cf_mbnode_t*)*mmp;
		struct _cf_mbnode_t *nd = (struct _cf_mbnode_t*)(ptr - sizeof(struct _cf_mbnode_t));

		// check node
		while( _nd && _nd != nd ) _nd = _nd->prior;

		if( _nd )
		{
			if( nd->prior )
				nd->prior->next = nd->next;
			else
				*mmp = (long)nd->next;

			if( nd->next )
				nd->next->prior = nd->prior;

			free( nd );
		}
		else
			_ERROR_PUT_( "invalid pointer" );
	}
	else
	{
#ifndef CF_MEMPOOL
		errno = errNotSupport;
#else
		UM_Free( (UM_ID)mmp, ptr );
#endif
	}
}


//
CF_HIDDEN void cf_destroy_mempool( int sys_mmp, long *mmphd )
{
	if( mmphd && *mmphd )
	{
		if( sys_mmp )
		{
			struct _cf_mbnode_t *nd2, *nd = (struct _cf_mbnode_t*)*mmphd;
			while( nd )
			{
				nd2 = nd->prior;
				free( nd );
				nd = nd2;
			}
		}
#ifdef CF_MEMPOOL
		else
			UM_DeletePool( *mmphd );
#endif

		*mmphd = 0;
	}
}


//
CF_HIDDEN int cf_create_mempool( int sys_mmp, long *mmphd )
{
	if( sys_mmp )
	{
		// delete all nodes have been linked in the sys_mem_pool
		cf_destroy_mempool( 1, mmphd );
		return 0;
	}

#ifdef CF_MEMPOOL
	// prepare MEMPOOL
	UM_ID hd = *mmphd;

	if( hd != -1 && hd != 0 )
	{
		if( UM_CheckPool(hd) != 0 )  // test existing of MEMPOOL
		{
			// invalid MEMPOOL
			if( errno == ESRCH )
				hd = -1;
			else
				return -1;  // undefined
		}
		else
		{
			// always delete
			UM_DeletePool( hd );
			hd = -1;
		}
	}

	if( hd == 0 || hd == -1 )
	{
		// create a new MEMPOOL
		*mmphd = hd = UM_CreatePoolEx2( 16, FALSE, FALSE, _MEMPOOL_INCSIZE );
		if( hd == -1 || hd == 0 )
			return -1;
	}
	return 0;
#else
	errno = errNotSupport;
	return -1;
#endif
}
#endif


// ========================================================================================================================================
// Global Variables
// ========================================================================================================================================

// Defined Constant Pool Item Name( MUST be ordered with JVM_CONSTANT_xxx )
const char CF_CP_NAME[32][32] = {
		"",
	    "Utf8",
	    "Unicode",
	    "Integer",
	    "Float",
	    "Long",
	    "Double",
	    "Class",
	    "String",
	    "Fieldref",
	    "Methodref",
	    "InterfaceMethodref",
	    "NameAndType",
		"Unknown13",
		"Unknown14",
	    "MethodHandle",
	    "MethodType",
		"Dynamic",
	    "InvokeDynamic",
		"Module",
		"Package"
};

// Defined Dynamic Reference Kind
const char CF_DYNREF_KIND_NAME[32][24] = {
		"",
		"getField",
		"getStatic",
		"putField",
		"putStatic",
		"invokeVirtual",
		"invokeStatic",
		"invokeSpecial",
		"newInvokeSpecial",
		"invokeInterface"
};

// keep the order as requirement of the opcode "newarray"
const char CF_PRIME_TYPE_NAME[16][8] = {
		"", "", "", "",
		"Boolean",  /* 4 */
		"Char"   ,  /* 5 */
		"Float"  ,  /* 6 */
		"Double" ,  /* 7 */
		"Byte"   ,  /* 8 */
		"Short"  ,  /* 9 */
		"Int"    ,  /* 10 */
		"Long"   ,  /* 11 */
		"", "", "", ""
};

// keep the order as requirement of the opcode "newarray"
const char CF_SIGNATURE_SYMBOL[16] = {
	    JVM_SIGNATURE_ENDCLASS  ,
	    JVM_SIGNATURE_FUNC      ,
	    JVM_SIGNATURE_ENDFUNC   ,

		JVM_SIGNATURE_VOID      ,

		JVM_SIGNATURE_BOOLEAN   ,
		JVM_SIGNATURE_CHAR      ,
		JVM_SIGNATURE_FLOAT     ,
		JVM_SIGNATURE_DOUBLE    ,
	    JVM_SIGNATURE_BYTE      ,
		JVM_SIGNATURE_SHORT     ,
		JVM_SIGNATURE_INT       ,
		JVM_SIGNATURE_LONG      ,

		JVM_SIGNATURE_CLASS     ,
		JVM_SIGNATURE_ARRAY     ,
		JVM_SIGNATURE_ENUM      ,  /* ? (be treated as class)*/
		0
};

// keep the order as requirement of the opcode "newarray"
const juint8 CF_SIGNATURE_SLOTS[16] = {
		0, 0, 0,
		0,  /* void */
		1,  /* boolean */
		1,  /* char */
		1,  /* float */
		2,  /* double */
		1,  /* byte */
		1,  /* short */
		1,  /* int */
		2,  /* long */
		1,  /* class */
		1,  /* array */
		1,  /* enum ? (be treated as class) */
		0
};


// keep the order as requirement of the CF_SIGNATURE_SYMBOL
CF_HIDDEN
const juint8 CF_IDX_TO_SMFVT[16] = {
		-1, -1, -1, -1,
		CF_SMFVT_int,
		CF_SMFVT_int,
		CF_SMFVT_float,
		CF_SMFVT_double,
		CF_SMFVT_int,
		CF_SMFVT_int,
		CF_SMFVT_int,
		CF_SMFVT_long,
		CF_SMFVT_object,  /* class */
		CF_SMFVT_object,  /* array */
		CF_SMFVT_object,  /* enum(be treated as class) */
		-1
};


//
const int CF_CPITEM_LEN[32] = _CP_ITEM_LEN;

//
const CP_Info CP_INFO_EMPTY = {.tag = 0};



// ========================================================================================================================================
// Local Functions
// ========================================================================================================================================

CF_HIDDEN int cf_cpinfo_endian_flip( CP_Info *cpi, int *units )
{
	*units = 1;

	if( !cpi )
		return 0;

	switch( cpi->tag )
	{
    case JVM_CONSTANT_Utf8:{
    	register int len = sizeof(CP_Utf8) + ((CP_Utf8*)cpi)->len;
   		_CF_ENDIAN_SWAP_U2( ((CP_Utf8*)cpi)->len  );
    	return len;
    }

    case JVM_CONSTANT_Float:     /* CP_Float as same as CP_Integer */
    case JVM_CONSTANT_Integer:{
#ifdef _ARCH_ENDIAN_LITTLE
		register int i32=((CP_Integer*)cpi)->i32;
		((CP_Integer*)cpi)->i32 = _CF_ENDIAN_FLIP_32( i32 );
#endif
		return sizeof( CP_Integer );
    	}

    case JVM_CONSTANT_Double:    /* CP_Double as same as CP_Long */
    case JVM_CONSTANT_Long:{
#ifdef _ARCH_ENDIAN_LITTLE
		register long long i64=((CP_Long*)cpi)->i64;
		((CP_Long*)cpi)->i64 = _CF_ENDIAN_FLIP_64( i64 );
#endif
		*units = 2;
		return sizeof( CP_Long );
		}

    case JVM_CONSTANT_Class:
#ifdef _ARCH_ENDIAN_LITTLE
   		_CF_ENDIAN_SWAP_INDEX( ((CP_Class*)cpi)->name_idx  );
#endif
    	return sizeof(CP_Class);

    case JVM_CONSTANT_String:
#ifdef _ARCH_ENDIAN_LITTLE
   		_CF_ENDIAN_SWAP_INDEX( ((CP_String*)cpi)->str_idx  );
#endif
    	return sizeof(CP_String);

    case JVM_CONSTANT_Fieldref:
#ifdef _ARCH_ENDIAN_LITTLE
		_CF_ENDIAN_SWAP_INDEX( ((CP_Fieldref*)cpi)->class_idx  );
		_CF_ENDIAN_SWAP_INDEX( ((CP_Fieldref*)cpi)->nm_tp_idx  );
#endif
    	return sizeof(CP_Fieldref);

    case JVM_CONSTANT_Methodref:
#ifdef _ARCH_ENDIAN_LITTLE
		_CF_ENDIAN_SWAP_INDEX( ((CP_Methodref*)cpi)->class_idx  );
		_CF_ENDIAN_SWAP_INDEX( ((CP_Methodref*)cpi)->nm_tp_idx  );
#endif
    	return sizeof(CP_Methodref);

    case JVM_CONSTANT_InterfaceMethodref:
#ifdef _ARCH_ENDIAN_LITTLE
		_CF_ENDIAN_SWAP_INDEX( ((CP_InterfaceMethodref*)cpi)->class_idx  );
		_CF_ENDIAN_SWAP_INDEX( ((CP_InterfaceMethodref*)cpi)->nm_tp_idx  );
#endif
    	return sizeof(CP_InterfaceMethodref);

    case JVM_CONSTANT_NameAndType:
#ifdef _ARCH_ENDIAN_LITTLE
		_CF_ENDIAN_SWAP_INDEX( ((CP_NameAndType*)cpi)->name_idx  );
		_CF_ENDIAN_SWAP_INDEX( ((CP_NameAndType*)cpi)->desc_idx  );
#endif
    	return sizeof(CP_NameAndType);

    case JVM_CONSTANT_MethodHandle:
#ifdef _ARCH_ENDIAN_LITTLE
   		_CF_ENDIAN_SWAP_INDEX( ((CP_MethodHandle*)cpi)->ref_idx  );
#endif
    	return sizeof(CP_MethodHandle);

    case JVM_CONSTANT_MethodType:
#ifdef _ARCH_ENDIAN_LITTLE
   		_CF_ENDIAN_SWAP_INDEX( ((CP_MethodType*)cpi)->desc_idx  );
#endif
    	return sizeof(CP_MethodType);

    case JVM_CONSTANT_Dynamic:  /* Dynamic */
#ifdef _ARCH_ENDIAN_LITTLE
		_CF_ENDIAN_SWAP_INDEX( ((CP_Dynamic*)cpi)->bootstrap_attr_idx  );
		_CF_ENDIAN_SWAP_INDEX( ((CP_Dynamic*)cpi)->name_type_idx  );
#endif
    	return sizeof(CP_Dynamic);

    case JVM_CONSTANT_InvokeDynamic:
#ifdef _ARCH_ENDIAN_LITTLE
		_CF_ENDIAN_SWAP_INDEX( ((CP_InvokeDynamic*)cpi)->bootstrap_attr_idx  );
		_CF_ENDIAN_SWAP_INDEX( ((CP_InvokeDynamic*)cpi)->name_type_idx  );
#endif
    	return sizeof(CP_InvokeDynamic);

    case JVM_CONSTANT_Module:  /* Module */
    case JVM_CONSTANT_Package: /* Package, as same as CP_Module */
#ifdef _ARCH_ENDIAN_LITTLE
   		_CF_ENDIAN_SWAP_INDEX( ((CP_Module*)cpi)->name_idx  );
#endif
    	return sizeof(CP_Module);

    default:
    	return 0;
	}
}


// Function:
// Arguments:
// flag: 1=base elements, 2=CP, 4=interfaces, 8=fields, 16=methods, 32=all attributes(includes method's and field's)
CF_HIDDEN int cf_map_endian_flip( CF_ClassFileMap *cf_map, int tabno, CF_ClassFile *cfptr, int flag )
{
#ifdef _ARCH_ENDIAN_LITTLE
	if( flag & 1 )
	{
		// base elements
		cfptr->magic = _CF_ENDIAN_FLIP_MAGIC( cf_map->magic );
		cfptr->minor_ver = _CF_ENDIAN_FLIP_U2( cf_map->minor_ver );
		cfptr->major_ver = _CF_ENDIAN_FLIP_U2( cf_map->major_ver );
		cfptr->cp_count = _CF_ENDIAN_FLIP_COUNT( cf_map->cptab[tabno].cp_count );
		cfptr->access.flags = _CF_ENDIAN_FLIP_ACCES( cf_map->access.flags );
		cfptr->this_class = _CF_ENDIAN_FLIP_INDEX( cf_map->this_class );
		cfptr->super_class = _CF_ENDIAN_FLIP_INDEX( cf_map->super_class );
		cfptr->cp_count = _CF_ENDIAN_FLIP_COUNT( cf_map->cptab[tabno].cp_count );
		cfptr->if_count = _CF_ENDIAN_FLIP_COUNT( cf_map->iftab[tabno].if_count );
		cfptr->field_count = _CF_ENDIAN_FLIP_COUNT( cf_map->fdtab[tabno].field_count );
		cfptr->method_count = _CF_ENDIAN_FLIP_COUNT( cf_map->mdtab[tabno].method_count );
		cfptr->attr_count = _CF_ENDIAN_FLIP_COUNT( cf_map->attrtab[tabno].attr_count );
	}


	int c, m;

	if( flag & 2 )
	{
		// constants pool
		int u;
		CP_Info **cps = cf_map->cptab[tabno].cps;
		m = cf_map->cptab[tabno].cp_count;
		for( c=1; c < m; c+=u )
			cf_cpinfo_endian_flip( cps[c], &u );
	}


	if( flag & 4 )
	{
		// interfaces
		cf_idx_t *idx = cf_map->iftab[tabno].ifs;
		m = cf_map->iftab[tabno].if_count;
		for( c=0; c < m; c++ )
			_CF_ENDIAN_SWAP_INDEX( idx[c] );
	}


	if( flag & 8 )
	{
		// fields
		CF_FieldInfo **field = cf_map->fdtab[tabno].fields;
		m = cf_map->fdtab[tabno].field_count;
		for( c=0; c < m; c++ )
		{
			_CF_ENDIAN_SWAP_ACCES( field[c]->access.flags );
			_CF_ENDIAN_SWAP_COUNT( field[c]->name_idx );
			_CF_ENDIAN_SWAP_COUNT( field[c]->desc_idx );
			_CF_ENDIAN_SWAP_COUNT( field[c]->attr_count );
		}
	}


	if( flag & 16 )
	{
		// methods
		CF_MethodInfo **method = cf_map->mdtab[tabno].methods;
		m = cf_map->mdtab[tabno].method_count;
		for( c=0; c < m; c++ )
		{
			_CF_ENDIAN_SWAP_ACCES( method[c]->access.flags );
			_CF_ENDIAN_SWAP_COUNT( method[c]->name_idx );
			_CF_ENDIAN_SWAP_COUNT( method[c]->desc_idx );
			_CF_ENDIAN_SWAP_COUNT( method[c]->attr_count );
		}
	}


#if 0  // TODO:
	if( flag & 32 )
	{
		// attributes
		CF_AttrInfo **attr = cf_map->attrtab[tabno].attr;
		m = cf_map->attrtab[tabno].attr_count;
		for( c=0; c < m; c++ )
		{
			_CF_ENDIAN_SWAP_INDEX( attr[c]->attr_nm_idx );
			_CF_ENDIAN_SWAP_U4( attr[c]->attr_len );
		}
	}
#endif

#endif

	return 0;
}


//
CF_HIDDEN int cf_chk_cpitem( CP_Info *cpi, ssize_t len, int *units, int local_endian )
{
	if( !cpi )
	{
		*units = 1;
		return 0;
	}

	switch( cpi->tag )
	{
    case JVM_CONSTANT_Utf8:{
    	int n;
    	if( (n=len - sizeof(CP_Utf8)) < 0 )
    		return n;
    	n = ((CP_Utf8*)cpi)->len;
    	n = sizeof(CP_Utf8) + _CF_ENDIAN_FLIP_16(n);
    	if( (len -= n) < 0 )
    		return len;
    	*units = 1;

#ifdef _ARCH_ENDIAN_LITTLE
    	//if( local_endian & CF_MAP_LOCAL_ENDIAN_VALUE )
    		_CF_ENDIAN_SWAP_U2( ((CP_Utf8*)cpi)->len  );
#endif
    	return n;
    }

    case JVM_CONSTANT_Integer:
    case JVM_CONSTANT_Float:    /* CP_Float as same as CP_Integer */
    	if( (len -= sizeof(CP_Integer)) < 0 )
			return len;
    	*units = 1;

#ifdef _ARCH_ENDIAN_LITTLE
    	//if( local_endian & CF_MAP_LOCAL_ENDIAN_VALUE )
    	{
    		register int i32=((CP_Integer*)cpi)->i32;
    		((CP_Integer*)cpi)->i32 = _CF_ENDIAN_FLIP_32( i32 );
    	}
#endif
    	return sizeof(CP_Integer);

    //case JVM_CONSTANT_Float:

    case JVM_CONSTANT_Long:
    case JVM_CONSTANT_Double:    /* CP_Double as same as CP_Long */
    	if( (len -= sizeof(CP_Long)) < 0 )
			return len;
    	*units = 2;

#ifdef _ARCH_ENDIAN_LITTLE
    	//if( local_endian & CF_MAP_LOCAL_ENDIAN_VALUE )
    	{
    		register long long i64=((CP_Long*)cpi)->i64;
    		((CP_Long*)cpi)->i64 = _CF_ENDIAN_FLIP_64( i64 );
    	}
#endif
    	return sizeof(CP_Long);

    //case JVM_CONSTANT_Double:

    case JVM_CONSTANT_Class:
    	if( (len -= sizeof(CP_Class)) < 0 )
			return len;
    	*units = 1;

#ifdef _ARCH_ENDIAN_LITTLE
    	//if( local_endian & CF_MAP_LOCAL_ENDIAN_INDEX )
    		_CF_ENDIAN_SWAP_INDEX( ((CP_Class*)cpi)->name_idx  );
#endif
    	return sizeof(CP_Class);

    case JVM_CONSTANT_String:
    	if( (len -= sizeof(CP_String)) < 0 )
			return len;
    	*units = 1;

#ifdef _ARCH_ENDIAN_LITTLE
    	//if( local_endian & CF_MAP_LOCAL_ENDIAN_INDEX )
    		_CF_ENDIAN_SWAP_INDEX( ((CP_String*)cpi)->str_idx  );
#endif
    	return sizeof(CP_String);

    case JVM_CONSTANT_Fieldref:
    	if( (len -= sizeof(CP_Fieldref)) < 0 )
			return len;
    	*units = 1;

#ifdef _ARCH_ENDIAN_LITTLE
    	//if( local_endian & CF_MAP_LOCAL_ENDIAN_INDEX )
    	{
    		_CF_ENDIAN_SWAP_INDEX( ((CP_Fieldref*)cpi)->class_idx  );
    		_CF_ENDIAN_SWAP_INDEX( ((CP_Fieldref*)cpi)->nm_tp_idx  );
    	}
#endif
    	return sizeof(CP_Fieldref);

    case JVM_CONSTANT_Methodref:
    	if( (len -= sizeof(CP_Methodref)) < 0 )
			return len;
    	*units = 1;

#ifdef _ARCH_ENDIAN_LITTLE
    	//if( local_endian & CF_MAP_LOCAL_ENDIAN_INDEX )
    	{
    		_CF_ENDIAN_SWAP_INDEX( ((CP_Methodref*)cpi)->class_idx  );
    		_CF_ENDIAN_SWAP_INDEX( ((CP_Methodref*)cpi)->nm_tp_idx  );
    	}
#endif
    	return sizeof(CP_Methodref);

    case JVM_CONSTANT_InterfaceMethodref:
    	if( (len -= sizeof(CP_InterfaceMethodref)) < 0 )
			return len;
    	*units = 1;

#ifdef _ARCH_ENDIAN_LITTLE
    	//if( local_endian & CF_MAP_LOCAL_ENDIAN_INDEX )
    	{
    		_CF_ENDIAN_SWAP_INDEX( ((CP_InterfaceMethodref*)cpi)->class_idx  );
    		_CF_ENDIAN_SWAP_INDEX( ((CP_InterfaceMethodref*)cpi)->nm_tp_idx  );
    	}
#endif
    	return sizeof(CP_InterfaceMethodref);

    case JVM_CONSTANT_NameAndType:
    	if( (len -= sizeof(CP_NameAndType)) < 0 )
			return len;
    	*units = 1;

#ifdef _ARCH_ENDIAN_LITTLE
    	//if( local_endian & CF_MAP_LOCAL_ENDIAN_INDEX )
    	{
    		_CF_ENDIAN_SWAP_INDEX( ((CP_NameAndType*)cpi)->name_idx  );
    		_CF_ENDIAN_SWAP_INDEX( ((CP_NameAndType*)cpi)->desc_idx  );
    	}
#endif
    	return sizeof(CP_NameAndType);

    case JVM_CONSTANT_MethodHandle:
    	if( (len -= sizeof(CP_MethodHandle)) < 0 )
			return len;
    	*units = 1;

#ifdef _ARCH_ENDIAN_LITTLE
    	//if( local_endian & CF_MAP_LOCAL_ENDIAN_INDEX )
    		_CF_ENDIAN_SWAP_INDEX( ((CP_MethodHandle*)cpi)->ref_idx  );
#endif
    	return sizeof(CP_MethodHandle);

    case JVM_CONSTANT_MethodType:
    	if( (len -= sizeof(CP_MethodType)) < 0 )
			return len;
    	*units = 1;

#ifdef _ARCH_ENDIAN_LITTLE
    	//if( local_endian & CF_MAP_LOCAL_ENDIAN_INDEX )
    		_CF_ENDIAN_SWAP_INDEX( ((CP_MethodType*)cpi)->desc_idx  );
#endif
    	return sizeof(CP_MethodType);

    case JVM_CONSTANT_Dynamic:  /* Dynamic */
    	if( (len -= sizeof(CP_Dynamic)) < 0 )
			return len;
    	*units = 1;

#ifdef _ARCH_ENDIAN_LITTLE
    	//if( local_endian & CF_MAP_LOCAL_ENDIAN_INDEX )
    	{
    		_CF_ENDIAN_SWAP_INDEX( ((CP_Dynamic*)cpi)->bootstrap_attr_idx  );
    		_CF_ENDIAN_SWAP_INDEX( ((CP_Dynamic*)cpi)->name_type_idx  );
    	}
#endif
    	return sizeof(CP_Dynamic);

    case JVM_CONSTANT_InvokeDynamic:
    	if( (len -= sizeof(CP_InvokeDynamic)) < 0 )
			return len;
    	*units = 1;

#ifdef _ARCH_ENDIAN_LITTLE
    	//if( local_endian & CF_MAP_LOCAL_ENDIAN_INDEX )
    	{
    		_CF_ENDIAN_SWAP_INDEX( ((CP_InvokeDynamic*)cpi)->bootstrap_attr_idx  );
    		_CF_ENDIAN_SWAP_INDEX( ((CP_InvokeDynamic*)cpi)->name_type_idx  );
    	}
#endif
    	return sizeof(CP_InvokeDynamic);

    case JVM_CONSTANT_Module:  /* Module */
    case JVM_CONSTANT_Package: /* Package, as same as CP_Module */
    	if( (len -= sizeof(CP_Module)) < 0 )
			return len;
    	*units = 1;

#ifdef _ARCH_ENDIAN_LITTLE
    	//if( local_endian & CF_MAP_LOCAL_ENDIAN_INDEX )
    		_CF_ENDIAN_SWAP_INDEX( ((CP_Module*)cpi)->name_idx  );
#endif
    	return sizeof(CP_Module);

    default:
    	return 0;
	}
}


//
CF_HIDDEN ssize_t cf_chk_field( CF_FieldInfo *field, ssize_t len, FILE *hfile, int local_endian )
{
	ssize_t sz, _len = len;
	ssize_t fpos_org = -1;
	CF_FieldInfo fld;

	if( (len-=sizeof(CF_FieldInfo)) < 0 )
	{
		// seek in file
		fpos_org = ftell( hfile );

		if( fread( ((void*)&fld)+_len, -len, 1, hfile ) != 1 || ferror(hfile) )
			return 0;

		memcpy( &fld, field, _len );
		field = &fld;
	}

	if( field->attr_count )  // MUST check it, because it may be zero
	{
		sz = cf_chk_attr( field->attr.ite, _CF_ENDIAN_FLIP_COUNT(field->attr_count), len>0?len:0, hfile, local_endian );
		if( sz == 0 )
			return 0;

		if( sz < 0 )
		{
			if( len > 0 )
				len = sz;
			else
				len += sz;
		}
		else
		{
			// deduces the "len" is greater or equal to the "sz"
			len -= sz;
		}
		//len += sz * (-(sz > 0) | 1);  WRONG
	}

	if( fpos_org != -1 )
		fseek( hfile, fpos_org, SEEK_SET );  // restore file position

	if( len >= 0 )
	{
#ifdef _ARCH_ENDIAN_LITTLE
		_CF_ENDIAN_SWAP_COUNT( field->attr_count );
		//if( local_endian & CF_MAP_LOCAL_ENDIAN_INDEX )
		{
			_CF_ENDIAN_SWAP_COUNT( field->name_idx );
			_CF_ENDIAN_SWAP_COUNT( field->desc_idx );
		}

		//if( local_endian & CF_MAP_LOCAL_ENDIAN_VALUE )
			_CF_ENDIAN_SWAP_ACCES( field->access.flags );
#endif
		return _len - len;
	}

	return len;    // len < 0 ? len : _len - len;
}


// ========================================================================================================================================
// General Functions
// ========================================================================================================================================

// Function:
CF_HIDDEN void cf_free_map( CF_ClassFileMap *cf_map )
{
	/*if( cf_map->auxi.mmphd )
	{
		if( cf_map->auxi.use_sysheap )
			cf_destroy_mempool( &cf_map->auxi.mmphd );
#ifdef CF_MEMPOOL
		else
			UM_DeletePool( cf_map->auxi.mmphd );
#endif
	}*/

	cf_destroy_mempool( cf_map->auxi.use_sysheap, &cf_map->auxi.mmphd );

	if( cf_map->auxi.hfile && !cf_map->auxi.src_buf )
		fclose( cf_map->auxi.hfile );

	memset( (void*)cf_map, 0, sizeof(CF_ClassFileMap) );
}


// Function:
int cf_make_map( CF_MAPHD hmap, const char *classfile )
{
	CF_ClassFileMap *map = cf_handle2stru( hmap );
	if( map )
		return cf_file2map( classfile, map, 0 );

	errno = EINVAL;
	return -1;
}


// Function:
int cf_make_map_from_buffer( CF_MAPHD hmap, const void *buf, ssize_t buflen )
{
	CF_ClassFileMap *map = cf_handle2stru( hmap );
	if( map )
		return cf_buffer2map( buf, buflen, map, 0 );

	errno = EINVAL;
	return -1;
}


// Function:
int cf_make_classfile( CF_MAPHD hmap, const char *classfile )
{
	CF_ClassFileMap *map = cf_handle2stru( hmap );
	if( map )
		return cf_map2file( map, classfile, 0 );

	errno = EINVAL;
	return -1;
}


// Function:
int cf_make_classbuffer( CF_MAPHD hmap, void *buf, ssize_t bufsize )
{
	CF_ClassFileMap *map = cf_handle2stru( hmap );
	if( map )
		return cf_map2buffer( map, buf, bufsize, 0 );

	errno = EINVAL;
	return 0;
}


// Function:
CF_MAPHD cf_create_map( int map_flags )
{
	CF_ClassFileMap *map = malloc( sizeof(CF_ClassFileMap) );
	if( map )
	{
		memset( map, 0, sizeof(CF_ClassFileMap) );
		map->auxi.flag = map_flags & CF_MAP_FLAG_MASK;

		map->auxi.flag |= CF_MAP_LOCAL_ENDIAN_INDEX | CF_MAP_LOCAL_ENDIAN_VALUE;  // ALWAYS

		CF_MAP_SET_IDENT( map->auxi.ident );

		return CF_MAP_STRU2HANDLE( map );  // (CF_MAPHD)&map->auxi.ident;
	}

	return NULL;
}

// Function:
void cf_destroy_map( CF_MAPHD *hmap )
{
	if( hmap )
	{
		CF_ClassFileMap *map = cf_handle2stru( *hmap );
		if( map )
		{
			cf_free_map( map );
			free( map );
			*hmap = NULL;
		}
	}
}


// ========================================================================================================================================
// Exported Functions: Class File Map Operator
// ========================================================================================================================================

// Function: copy utf8 string
int cf_copy_utf8( const CP_Utf8 *cp, char *dst, int len )
{
	if( cp && cp->tag == JVM_CONSTANT_Utf8 )
	{
		if( len > cp->len )
			len = cp->len;
		else
			len--;
		if( len >= 0 )
		{
			memcpy( dst, cp->b, len );
			dst[len] = 0;
		}
		return len;
	}
	return -1;
}


// Function: find constants pool item which tag is Utf8, and also it's contents completely matches the specified value
CF_HIDDEN const CP_Utf8* cf_get_cpitem_utf8( const CF_ClassFileMap *cf_map, const char *utf8, int len, cf_idx_t *idx )
{
	if( utf8 && len > 0 )
	{
		const CP_Utf8 **cps;
		cf_count_t c, n, m, i;

		i = 0;
		for( c=0; c < cf_map->cptab_count; c++ )
		{
			cps = (const CP_Utf8**)cf_map->cptab[c].cps;
			m = cf_map->cptab[c].cp_count;

			for( n=1; n < m; n++ )  // cause: start from 1
			{
				if( cps[n] && cps[n]->tag == JVM_CONSTANT_Utf8 &&
					cps[n]->len == len &&
					strncmp(cps[n]->b, utf8, len) == 0	/* case sensitive */
					)
				{
					if( idx )
						*idx = i + n;

					return cps[n];
				}
			}
			i += m;
		}
	}
	return NULL;
}


// special for searching signature of method
CF_HIDDEN const CP_Utf8* cf_get_cpitem_utf8_signature( const CF_ClassFileMap *cf_map, const char *utf8, int len, cf_idx_t *idx )
{
	if( utf8 && len > 0 )
	{
		const CP_Utf8 **cps;
		cf_count_t c, n, m, i;

		i = 0;
		for( c=0; c < cf_map->cptab_count; c++ )
		{
			cps = (const CP_Utf8**)cf_map->cptab[c].cps;
			m = cf_map->cptab[c].cp_count;

			for( n=1; n < m; n++ )  // cause: start from 1
			{
				if( cps[n] && cps[n]->tag == JVM_CONSTANT_Utf8 &&
					cps[n]->len >= len &&
					strncmp(cps[n]->b, utf8, len) == 0 &&	/* case sensitive */
					cps[n]->b[len-1] == JVM_SIGNATURE_ENDFUNC
					)
				{
					if( idx )
						*idx = i + n;

					return cps[n];
				}
			}
			i += m;
		}
	}
	return NULL;
}


// Function: get constants pool according to the index
CF_HIDDEN const CP_Info* cf_get_cpitem( const CF_ClassFileMap *cf_map, cf_idx_t idx )
{
	cf_count_t c;
	for( c=0; c < cf_map->cptab_count; c++ )
	{
		if( cf_map->cptab[c].cp_count > idx )
		{
			return cf_map->cptab[c].cps[idx];
		}
		else
		{
			idx -= cf_map->cptab[c].cp_count;
			idx += !idx;  // 0 => 1
		}
	}
	return NULL;
}


// Function:
CF_HIDDEN const CP_Info* cf_get_cpitem_xx( const CF_ClassFileMap *cf_map, CP_InfoUnion *cpi_hit, cf_idx_t *idx, const char *utf8, juint16 utf8_len )
{
	const CP_InfoUnion **cps;
	cf_count_t c, n, m, i, j;

	//juint16 utf8_len;
	//const char *utf8;
	CP_InfoUnion hit = *cpi_hit;

	if( hit.Info.tag == JVM_CONSTANT_Utf8 && !utf8 )
	{
		utf8_len = cpi_hit->Utf8.len;
		utf8 = cpi_hit->Utf8.b;
	}

	i = 0;
	for( c=0; c < cf_map->cptab_count; c++ )
	{
		cps = (const CP_InfoUnion**)cf_map->cptab[c].cps;
		m = cf_map->cptab[c].cp_count;

		for( n=1; n < m; n++ )  // cause: start from 1
		{
			if( cps[n] && cps[n]->Info.tag == hit.Info.tag )
			{
				switch( hit.Info.tag )
				{
			    case JVM_CONSTANT_Utf8:
			    	if( cps[n]->Utf8.len == utf8_len &&
			    		strncmp(cps[n]->Utf8.b, utf8, utf8_len) == 0 )	/* case sensitive */
			    		goto LAB_HIT;
			    	break;
			    case JVM_CONSTANT_Integer:
			    case JVM_CONSTANT_Float:
			    	if( cps[n]->Integer.i32 == hit.Integer.i32 )
			    		goto LAB_HIT;
			    	break;
			    case JVM_CONSTANT_Long:
			    case JVM_CONSTANT_Double:
			    	if( cps[n]->Long.i64 == hit.Long.i64 )
			    		goto LAB_HIT;
			    	break;
			    case JVM_CONSTANT_Class:
			    case JVM_CONSTANT_String:
			    	if( utf8 )
			    	{
			    		j = cps[n]->Class.name_idx;
			    		if( j < m && cps[j]->Utf8.tag == JVM_CONSTANT_Utf8 &&
							cps[j]->Utf8.len == utf8_len &&
							strncmp(cps[j]->Utf8.b, utf8, utf8_len) == 0 )
			    		{
			    			goto LAB_HIT;
			    		}
			    	}
			    	else if( cps[n]->Class.name_idx == hit.Class.name_idx )
			    		goto LAB_HIT;
			    	break;
			    case JVM_CONSTANT_Fieldref:
			    case JVM_CONSTANT_Methodref:
			    case JVM_CONSTANT_InterfaceMethodref:
			    	if( cps[n]->Field.class_idx == hit.Field.class_idx &&
			    		cps[n]->Field.nm_tp_idx == hit.Field.nm_tp_idx )
			    		goto LAB_HIT;
			    	break;
			    case JVM_CONSTANT_NameAndType:
			    	if( cps[n]->NameType.name_idx == hit.NameType.name_idx &&
			    		cps[n]->NameType.desc_idx == hit.NameType.desc_idx )
			    		goto LAB_HIT;
			    	break;
			    case JVM_CONSTANT_MethodHandle:
			    	if( cps[n]->MethodHandle.ref_kind == hit.MethodHandle.ref_kind &&
			    		cps[n]->MethodHandle.ref_idx == hit.MethodHandle.ref_idx )
			    		goto LAB_HIT;
			    	break;
			    case JVM_CONSTANT_MethodType:
			    	if( cps[n]->MethodType.desc_idx == hit.MethodType.desc_idx )
			    		goto LAB_HIT;
			    	break;
			    case JVM_CONSTANT_Dynamic:
			    case JVM_CONSTANT_InvokeDynamic:
			    	if( cps[n]->InvokeDynamic.bootstrap_attr_idx == hit.InvokeDynamic.bootstrap_attr_idx &&
			    		cps[n]->InvokeDynamic.name_type_idx == hit.InvokeDynamic.name_type_idx	)
			    		goto LAB_HIT;
			    	break;
			    case JVM_CONSTANT_Module:  /* Module */
			    case JVM_CONSTANT_Package: /* Package, as same as CP_Module */
			    	if( cps[n]->Module.name_idx == hit.Module.name_idx )
			    		goto LAB_HIT;
			    	break;
				}
				continue;

				LAB_HIT:
				if( idx )
					*idx = i + n;

				return (CP_Info*)cps[n];
			}
		}
		i += m;
	}

	return NULL;
}


// Function: alter constants index
void cf_alter_cp_index( CP_Info **cps, int num, int off, int btsmtd_off )
{
	// Note: assume byte order is local ENDIAN

	int i;
	CP_InfoUnion **cpi = (CP_InfoUnion**)cps;

	for( i=0; i < num; i++ )
	{
		if( !cps[i] )
			continue;

		switch( cps[i]->tag )
		{
		default:
	    //case JVM_CONSTANT_Utf8:
	    //case JVM_CONSTANT_Integer:
	    //case JVM_CONSTANT_Float:
			break;
	    case JVM_CONSTANT_Long:
	    case JVM_CONSTANT_Double:
	    	i++;  // occupy two slots
	    	break;

	    case JVM_CONSTANT_Class:
	    	cpi[i]->Class.name_idx += off;
	    	break;
	    case JVM_CONSTANT_String:
	    	cpi[i]->String.str_idx += off;
	    	break;
	    case JVM_CONSTANT_Fieldref:
	    case JVM_CONSTANT_Methodref:
	    case JVM_CONSTANT_InterfaceMethodref:
	    	cpi[i]->Field.class_idx += off;
	    	cpi[i]->Field.nm_tp_idx += off;
	    	break;
	    case JVM_CONSTANT_NameAndType:
	    	cpi[i]->NameType.desc_idx += off;
	    	cpi[i]->NameType.name_idx += off;
	    	break;
	    case JVM_CONSTANT_MethodHandle:
	    	cpi[i]->MethodHandle.ref_idx += off;
	    	break;
	    case JVM_CONSTANT_MethodType:
	    	cpi[i]->MethodType.desc_idx += off;
	    	break;
	    case JVM_CONSTANT_Dynamic:
	    case JVM_CONSTANT_InvokeDynamic:
	    	cpi[i]->Dynamic.bootstrap_attr_idx += btsmtd_off;
	    	cpi[i]->Dynamic.name_type_idx += off;
	    	break;
	    case JVM_CONSTANT_Module:
	    case JVM_CONSTANT_Package:
	    	cpi[i]->Package.name_idx += off;
	    	break;
		}
	}
}


// Function: resolve constants dependence
void cf_resolve_constant_dependence( CP_Info **cps, int num, cf_idx_t *mask )
{
}


// Function: count length of constants pool
ssize_t cf_count_cp_size( CP_Info **cps, int num )
{
	// Note: assume byte order is local ENDIAN
	int i;
	ssize_t s = 0;

	for( i=0; i < num; i++ )
	{
		if( cps[i] ) //&& cps[i] != &CP_INFO_NULL )
		{
			s += cps[i]->tag == JVM_CONSTANT_Utf8 ? ((CP_Utf8*)cps[i])->len + sizeof(CP_Utf8) : CF_CPITEM_LEN[cps[i]->tag];
			i += cps[i]->tag == JVM_CONSTANT_Long || cps[i]->tag == JVM_CONSTANT_Double;
		}
	}
	return s;
}

// Function:
ssize_t cf_count_cp_size_m( CP_Info **cps, int num, cf_idx_t *mask )
{
	// Note: assume byte order is local ENDIAN
	int i;
	ssize_t s = 0;

	for( i=0; i < num; i++ )
	{
		if( cps[i] ) //&& cps[i] != &CP_INFO_NULL )
		{
			if( mask[i] )
				s += cps[i]->tag == JVM_CONSTANT_Utf8 ? ((CP_Utf8*)cps[i])->len + sizeof(CP_Utf8) : CF_CPITEM_LEN[cps[i]->tag];
			i += cps[i]->tag == JVM_CONSTANT_Long || cps[i]->tag == JVM_CONSTANT_Double;
		}
	}
	return s;
}


// Function: copy whole constant pool
CF_HIDDEN int cf_clone_constants_pool( const struct _cf_cptab_item *src, struct _cf_cptab_item *dst, CF_ClassFileMap *cf_map )
{
	CP_Info **new_cps = cf_malloc( cf_map->auxi.use_sysheap, &cf_map->auxi.mmphd, sizeof(void*)*src->cp_count );
	if( !new_cps )
		return -1;

	ssize_t sz = src->cps_size > 0 ? src->cps_size : cf_count_cp_size( src->cps, src->cp_count );
	void *ptr= cf_malloc( cf_map->auxi.use_sysheap, &cf_map->auxi.mmphd, sz );
	if( !ptr )
	{
		cf_free( cf_map->auxi.use_sysheap, &cf_map->auxi.mmphd, new_cps );
		return -1;
	}

	dst->cps = new_cps;
	dst->cp_count = src->cp_count;
	dst->cps_size = sz;

	int num = src->cp_count;
	CP_Info **cps = src->cps;
	ssize_t xpos = (size_t)cps[0];
	ssize_t xlen = 0;

	int i, tag, l;

	for( i=0; i < num; i++ )
	{
		//if( cps[i] == &CP_INFO_NULL )
		//	new_cps[i] = (CP_Info*)&CP_INFO_NULL;
		//else
		if( !cps[i] )
			new_cps[i] = NULL;
		else
		{
			new_cps[i] = (CP_Info*)(ptr + xlen);

			if( (xpos != 0) & (cps[i] != (void*)xpos) )
			{
				memcpy( ptr, (void*)xpos-xlen, xlen );
				ptr += xlen;
				xlen = 0;
			}

			tag = cps[i]->tag;
			l = tag == JVM_CONSTANT_Utf8 ? ((CP_Utf8*)cps[i])->len + sizeof(CP_Utf8) : CF_CPITEM_LEN[tag];
			xlen += l;
			xpos = ((size_t)cps[i]) + l;

			if( tag == JVM_CONSTANT_Long | tag == JVM_CONSTANT_Double )
				new_cps[++i] = NULL;
		}
	}

	if( xlen != 0 )
		memcpy( ptr, (void*)xpos-xlen, xlen );

	return num;
}


// Function:
CF_HIDDEN int cf_clone_constants_pool_m( const struct _cf_cptab_item *src, struct _cf_cptab_item *dst, CF_ClassFileMap *cf_map, cf_idx_t *mask )
{
	CP_Info **new_cps = cf_malloc( cf_map->auxi.use_sysheap, &cf_map->auxi.mmphd, sizeof(void*)*src->cp_count );
	if( !new_cps )
		return -1;

	ssize_t sz = src->cps_size > 0 ? src->cps_size : cf_count_cp_size_m( src->cps, src->cp_count, mask );
	void *ptr= cf_malloc( cf_map->auxi.use_sysheap, &cf_map->auxi.mmphd, sz );
	if( !ptr )
	{
		cf_free( cf_map->auxi.use_sysheap, &cf_map->auxi.mmphd, new_cps );
		return -1;
	}

	dst->cps = new_cps;
	dst->cp_count = src->cp_count;
	dst->cps_size = sz;

	int num = src->cp_count;
	CP_Info **cps = src->cps;
	ssize_t xpos = (size_t)cps[0];
	ssize_t xlen = 0;

	int i, tag, l, j;

	for( i=j=0; i < num; i++ )
	{
		if( mask[i] )
		{
			new_cps[j] = (CP_Info*)(ptr + xlen);

			if( (xpos != 0) & (cps[i] != (void*)xpos) )
			{
				memcpy( ptr, (void*)xpos-xlen, xlen );
				ptr += xlen;
				xlen = 0;
			}

			tag = cps[i]->tag;
			l = tag == JVM_CONSTANT_Utf8 ? ((CP_Utf8*)cps[i])->len + sizeof(CP_Utf8) : CF_CPITEM_LEN[tag];
			xlen += l;
			xpos = ((size_t)cps[i]) + l;

			if( tag == JVM_CONSTANT_Long | tag == JVM_CONSTANT_Double )
			{
				new_cps[++j] = NULL;
				++i;
			}
			++j;
		}
	}

	if( xlen != 0 )
		memcpy( ptr, (void*)xpos-xlen, xlen );

	return num;
}



// Function:
CF_HIDDEN ssize_t cf_cp2buffer( const struct _cf_cptab_item *src, void *buf, ssize_t szbuf )
{
	// Note: assume local ENDIAN is used by CP

	int num = src->cp_count;
	CP_Info **cps = src->cps;
	ssize_t xpos = (size_t)cps[0];
	ssize_t xlen = 0;
	void *ptr = buf;
	int i, tag, l;

	for( i=0; i < num; i++ )
	{
		if( cps[i] ) //&& cps[i] != &CP_INFO_NULL )
		{
			if( (xpos != 0) & (cps[i] != (void*)xpos) )
			{
				memcpy( ptr, (void*)xpos-xlen, xlen );
				ptr += xlen;
				xlen = 0;
			}

			tag = cps[i]->tag;
			l = tag == JVM_CONSTANT_Utf8 ? ((CP_Utf8*)cps[i])->len + sizeof(CP_Utf8) : CF_CPITEM_LEN[tag];
			xlen += l;

			if( ptr - buf + xlen > szbuf )
			{
				xlen -= l;
				break;
			}

			xpos = ((size_t)cps[i]) + l;
			i += tag == JVM_CONSTANT_Long | tag == JVM_CONSTANT_Double;
		}
	}

	if( xlen != 0 )
		memcpy( ptr, (void*)xpos-xlen, xlen );

	return (ssize_t)(ptr-buf+xlen);
}


// Function:
CF_HIDDEN int cf_cp2file( const struct _cf_cptab_item *src, FILE *fout )
{
	// Note: assume bigger ENDIAN is used by CP

	int num = src->cp_count;
	CP_Info **cps = src->cps;
	ssize_t xpos = (size_t)cps[0];
	ssize_t xlen = 0;

	int i, tag, l;

	for( i=0; i < num; i++ )
	{
		if( cps[i] ) //&& cps[i] != &CP_INFO_NULL )
		{
			if( (xpos != 0) & (cps[i] != (void*)xpos) )
			{
				if( xlen != fwrite( (void*)xpos-xlen, 1, xlen, fout ) || ferror(fout) )
					return -1;
				xlen = 0;
			}

			tag = cps[i]->tag;
			l = tag == JVM_CONSTANT_Utf8 ? _CF_ENDIAN_FLIP_16(((CP_Utf8*)cps[i])->len) + sizeof(CP_Utf8) : CF_CPITEM_LEN[tag];
			xlen += l;
			xpos = ((size_t)cps[i]) + l;
			i += tag == JVM_CONSTANT_Long | tag == JVM_CONSTANT_Double;
		}
	}

	if( xlen != 0 )
	{
		if( xlen != fwrite( (void*)xpos-xlen, 1, xlen, fout ) || ferror(fout) )
			return -1;
	}
	return 0;
}


// Function: API formal of the corresponding function "cf_get_cpitem"
// Return: greater than 0, means completely successful;
//         equal to 0, if an error occurred, check the errno for further information;
//         less 0, negative number, means the destination buffer "dst" is not enough to contains constant,
//         and it's absolute value represents the real size of constant.
int cf_copy_cpitem( CF_MAPHD hmap, cf_idx_t idx, juint8 *dst, int size )
{
	CF_ClassFileMap *map;

	if( !(map = cf_handle2stru( hmap )) )
	{
		errno = EINVAL;
		return 0;
	}

	const CP_Info *cpi = cf_get_cpitem( map, idx );
	if( !cpi )
	{
		errno = EEXIST;
		return 0;
	}

	int len = cpi->tag == JVM_CONSTANT_Utf8 ? ((CP_Utf8*)cpi)->len + sizeof(CP_Utf8) : CF_CPITEM_LEN[cpi->tag];
	if( len > size )
		return -len;

	memcpy( dst, cpi, len );
	return len;
}


// Function: API formal of the corresponding function "cf_copy_field_method"
// Return: greater than 0, means completely successful;
//         equal to 0, if an error occurred, check the errno for further information;
//         less 0, negative number, means the destination buffer "dst" is not enough to contains constant,
//         and it's absolute value represents the real size of constant.
ssize_t cf_copy_field_method( int which, CF_MAPHD hmap, juint8 *dst, ssize_t size, cf_idx_t name_idx, cf_idx_t desc_idx, cf_idx_t *result_idx )
{
	CF_ClassFileMap *map;

	if( !(map = cf_handle2stru( hmap )) )
	{
		errno = EINVAL;
		return -1;
	}

	cf_idx_t idx;
	ssize_t len;
	const CF_FieldInfo *field = cf_get_field_method( which, map, name_idx, desc_idx, &idx, &len );
	if( !field )
	{
		errno = EEXIST;
		return -1;
	}

	if( result_idx )
		*result_idx = idx;

	if( len > size )
		return -len;

	memcpy( dst, field, len );
	return len;
}


// Function: API formal of the corresponding function "cf_copy_field2" and "cf_copy_method2"
// Return: greater than 0, means completely successful;
//         equal to 0, if an error occurred, check the errno for further information;
//         less 0, negative number, means the destination buffer "dst" is not enough to contains constant,
//         and it's absolute value represents the real size of constant.
ssize_t cf_copy_field_method_2( int which, CF_MAPHD hmap, juint8 *dst, ssize_t size, const char *name, const char *desc, cf_idx_t *result_idx )
{
	CF_ClassFileMap *map;

	if( !(map = cf_handle2stru( hmap )) )
	{
		errno = EINVAL;
		return -1;
	}

	cf_idx_t idx;
	ssize_t len;
	const CF_FieldInfo *field = which == 1 ? cf_get_field2( map, name, desc, &idx, &len )
										   : (const CF_FieldInfo*)cf_get_method2( map, name, desc, &idx, &len, NULL );
	if( !field )
	{
		errno = EEXIST;
		return -1;
	}

	if( result_idx )
		*result_idx = idx;

	if( len > size )
		return -len;

	memcpy( dst, field, len );
	return len;
}

// Function: API formal of the corresponding function "cf_copy_field3" and "cf_copy_method3"
// Return: greater than 0, means completely successful;
//         equal to 0, if an error occurred, check the errno for further information;
//         less 0, negative number, means the destination buffer "dst" is not enough to contains constant,
//         and it's absolute value represents the real size of constant.
ssize_t cf_copy_field_method_3( int which, CF_MAPHD hmap, juint8 *dst, ssize_t size, cf_idx_t index )
{
	CF_ClassFileMap *map;

	if( !(map = cf_handle2stru( hmap )) )
	{
		errno = EINVAL;
		return -1;
	}

	ssize_t len;
	const CF_FieldInfo *field = which == 1 ? cf_get_field3( map, index, &len )
										   : (const CF_FieldInfo*)cf_get_method3( map, index, &len );
	if( !field )
	{
		errno = EEXIST;
		return -1;
	}

	if( len > size )
		return -len;

	memcpy( dst, field, len );
	return len;
}


//
CF_AttrInfo* cf_seek_attribute( CF_MAPHD hmap, const CF_AttrInfo *attr, cf_count_t count,
							    const char *name, int name_len, cf_idx_t seq )
{
	CF_ClassFileMap *map;

	if( !(map = cf_handle2stru( hmap )) )
	{
		errno = EINVAL;
		return NULL;
	}

	return cf_seek_attrinfo( attr, count, name, name_len, seq, map );
}


// Function:
CF_HIDDEN void cf_print_map_ex( const CF_ClassFileMap *map, FILE *hfile )
{
	fprintf( hfile,
			"magic: %X\n"
			"minor_version: %d\n"
			"major_version: %d\n"
			"constant_pool_count: %d\n"
			"access_flags: %04X\n{",
			map->magic, map->minor_ver, map->major_ver, map->cptab[0].cp_count, map->access.flags );

	cf_print_access( map->access, hfile );

	CP_InfoConstPtr cp0, cp1;
	cf_count_t c;
	char name[512];
	int n, ifsum, fdsum, mdsum, attrsum;

	name[n=0] = 0;
	if( (cp0.Info = cf_get_cpitem( map, map->this_class )) )
	{
		if( (cp0.Info = cf_get_cpitem( map, cp0.Class->name_idx )) )
			n = cf_copy_utf8( cp0.Utf8, name, sizeof(name)-100 );
	}

	name[n += 2] = 0;
	if( (cp1.Info = cf_get_cpitem( map, map->super_class )) )
	{
		if( (cp1.Info = cf_get_cpitem( map, cp1.Class->name_idx )) )
			cf_copy_utf8( cp1.Utf8, name+n, sizeof(name) - n );
	}

	for( ifsum=0, c=0; c < map->iftab_count; c++ )
		ifsum += map->iftab[c].if_count;

	for( fdsum=0, c=0; c < map->fdtab_count; c++ )
		fdsum += map->fdtab[c].field_count;

	for( mdsum=0, c=0; c < map->mdtab_count; c++ )
		mdsum += map->mdtab[c].method_count;

	for( attrsum=0, c=0; c < map->attrtab_count; c++ )
		attrsum += map->attrtab[c].attr_count;

	fprintf( hfile, "}\n"
			"this_class: %d, %s\n"
			"super_class: %d, %s\n"
			"interfaces_count: %d\n"
			"fields_count: %d\n"
			"methods_count: %d\n"
			"attributes_count: %d\n",
			map->this_class, name, map->super_class, name+n,
			ifsum, fdsum, mdsum, attrsum );

	cf_print_cptab( map, hfile );

	for( c=0; c < map->iftab_count; c++ )
		cf_print_interface( map->iftab[c].ifs, map->iftab[c].if_count, map, hfile );;

	for( c=0; c < map->fdtab_count; c++ )
		cf_print_fieldinfo( (const CF_FieldInfo **)map->fdtab[c].fields, map->fdtab[c].field_count, map, hfile );

	for( c=0; c < map->mdtab_count; c++ )
		cf_print_methodinfo( (const CF_MethodInfo **)map->mdtab[c].methods, map->mdtab[c].method_count, map, hfile );

	cf_print_attrtab( map, hfile );
}


// Function: API formal of the corresponding function "cf_print_map_ex"
void cf_print_map( CF_MAPHD hmap, FILE *hfile )
{
	if( hfile )
	{
		CF_ClassFileMap *map = cf_handle2stru( hmap );
		if( map )
			cf_print_map_ex( map, hfile );
	}
}


int cf_enter_edit( CF_ClassFileMap *cf_map )
{
	return -1;
}


void cf_leave_edit( CF_ClassFileMap *cf_map )
{
	;
}


int cf_commit_edit( CF_ClassFileMap *cf_map )
{
	return -1;
}




//
void cf_test( const char *fn )
{
#ifdef CF_MEMPOOL
	UM_Initiate();
#endif

	CF_ClassFileMap cf_map = {0};
	CF_MAP_SET_FALG( cf_map, blksz_abap, 1 );
	CF_MAP_SET_FALG( cf_map, use_sysheap, 1 );
	CF_MAP_LOCAL_ENDIAN_IDX( cf_map );
	CF_MAP_LOCAL_ENDIAN_VAL( cf_map );

	cf_file2map( fn, &cf_map, 0 );

	cf_print_map_ex( &cf_map, stdout );

	cf_free_map( &cf_map );

#ifdef CF_MEMPOOL
	UM_Release();
#endif
}
