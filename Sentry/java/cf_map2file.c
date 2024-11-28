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
#include "cf_impl.h"


extern const int CF_CPITEM_LEN[32];


// Function:
static ssize_t cf_map2buffer_bin( const CF_ClassFileMap *cf_map, juint8 *buf, ssize_t bufsize )
{
	// Note: this function does counting of bytes going to be consumed rather than directly allocating memory,
	//       because of caller usually have to call the "Allocate" via JVMTI.



	int c, m, n;
	ssize_t sz;
	CP_Info **cps;

	if( !buf || bufsize <= 0 )
	{
		// count bytes number
		sz = sizeof( CF_ClassFile );

		// constant pool length
		cps = cf_map->cptab[0].cps;

		if( cf_map->cptab[0].cps_size > 0 )
			sz += cf_map->cptab[0].cps_size;
		else
		{
			// re-calculate length of CP
			m = cf_map->cptab[0].cp_count;
			for( c=1; c < m; )
			{
				if( cps[c] )
				{
					sz += CF_CPITEM_LEN[cps[c]->tag] + ( cps[c]->tag == JVM_CONSTANT_Utf8 ? ((CP_Utf8*)cps[c])->len : 0 );
					c += 1 + (cps[c]->tag == JVM_CONSTANT_Long || cps[c]->tag == JVM_CONSTANT_Double);
				}
				else
					c++;
			}
		}


		// interfaces length
		sz += sizeof(cf_idx_t) * cf_map->iftab[0].if_count;

		// field table length
		m = cf_map->fdtab[0].field_count;
		for( c=0; c < m; c++ )
			sz += cf_map->fdtab[0].fields_len[c];

		// method table length
		m = cf_map->mdtab[0].method_count;
		for( c=0; c < m; c++ )
			sz += cf_map->mdtab[0].methods_len[c];

		// attribute table length
		m = cf_map->attrtab[0].attr_count;
		CF_AttrInfo **attr = cf_map->attrtab[0].attr;
		for( c=0; c < m; c++ )
		{
			n = attr[c]->attr_len;
			sz += sizeof(CF_AttrHead) + _CF_ENDIAN_FLIP_U4( n );
		}

		if( bufsize < sz || !buf )
			return -sz;
	}


	//
	CF_ClassFile *cfptr = (CF_ClassFile*)buf;

	// write: base elements
	cfptr->magic = _CF_ENDIAN_FLIP_MAGIC( cf_map->magic );
	cfptr->minor_ver = _CF_ENDIAN_FLIP_U2( cf_map->minor_ver );
	cfptr->major_ver = _CF_ENDIAN_FLIP_U2( cf_map->major_ver );
	cfptr->cp_count = _CF_ENDIAN_FLIP_COUNT( cf_map->cptab[0].cp_count );

	sz = _OFFSET( CF_ClassFile, cps );


	if( cfptr->cp_count )
	{
		// write: constants pool

		int u;
		cps = cf_map->cptab[0].cps;
		m = cf_map->cptab[0].cp_count;
		for( c=1; c < m; )
		{
			if( cps[c] )
			{
				n = CF_CPITEM_LEN[cps[c]->tag] + ( cps[c]->tag == JVM_CONSTANT_Utf8 ? ((CP_Utf8*)cps[c])->len : 0 );

				memcpy( &buf[sz], cps[c], n );
				cf_cpinfo_endian_flip( (CP_Info*)&buf[sz], &u );
				sz += n;
				c += u;  // 1 + (cps[c]->tag == JVM_CONSTANT_Long || cps[c]->tag == JVM_CONSTANT_Double);
			}
			else
				c++;
		}
	}


	// relocate
	cfptr = (CF_ClassFile*)(buf + sz - _OFFSET(CF_ClassFile, access));

	// write: access, this_class, super_class, interfaces_count
	cfptr->access.flags = _CF_ENDIAN_FLIP_ACCES( cf_map->access.flags );
	cfptr->this_class = _CF_ENDIAN_FLIP_INDEX( cf_map->this_class );
	cfptr->super_class = _CF_ENDIAN_FLIP_INDEX( cf_map->super_class );
	cfptr->if_count = _CF_ENDIAN_FLIP_COUNT( cf_map->iftab[0].if_count );

	sz += _OFFSET(CF_ClassFile, ifs ) - _OFFSET(CF_ClassFile, access );

	if( cfptr->if_count )
	{
		// write: interfaces
		m = cf_map->iftab[0].if_count;
		cf_idx_t *idxD = (cf_idx_t*)&buf[sz];
		cf_idx_t *idxS = cf_map->iftab[0].ifs;

#ifdef _ARCH_ENDIAN_LITTLE
		for( c=0; c < m; c++ )
			idxD[c] = _CF_ENDIAN_FLIP_INDEX( idxS[c] );
#else
		memcpy( idxD, idxS, m*sizeof(cf_idx_t) );
#endif
		sz += m*sizeof(cf_idx_t);
	}


	// write: fields_count
	cf_count_t *cnt = (cf_count_t*)&buf[sz];
	*cnt = _CF_ENDIAN_FLIP_COUNT( cf_map->fdtab[0].field_count );
	sz += sizeof( cf_count_t );
	if( *cnt )
	{
		// write: fields
		CF_FieldInfo *fld;
		CF_FieldInfo **field = cf_map->fdtab[0].fields;
		ssize_t *field_len = cf_map->fdtab[0].fields_len;
		m = cf_map->fdtab[0].field_count;

		for( c=0; c < m; c++ )
		{
			memcpy( &buf[sz], field[c], field_len[c] );

#ifdef _ARCH_ENDIAN_LITTLE
			fld = (CF_FieldInfo*)&buf[sz];
			_CF_ENDIAN_SWAP_ACCES( fld->access.flags );
			_CF_ENDIAN_SWAP_INDEX( fld->name_idx );
			_CF_ENDIAN_SWAP_INDEX( fld->desc_idx );
			_CF_ENDIAN_SWAP_COUNT( fld->attr_count );
#endif
			sz += field_len[c];
		}
	}


	// write: methods_count
	cnt = (cf_count_t*)&buf[sz];
	*cnt = _CF_ENDIAN_FLIP_COUNT( cf_map->mdtab[0].method_count );
	sz += sizeof( cf_count_t );
	if( *cnt )
	{
		// write: methods
		CF_MethodInfo *mtd;
		CF_MethodInfo **method = cf_map->mdtab[0].methods;
		ssize_t *method_len = cf_map->mdtab[0].methods_len;
		m = cf_map->mdtab[0].method_count;

		for( c=0; c < m; c++ )
		{
			memcpy( &buf[sz], method[c], method_len[c] );

#ifdef _ARCH_ENDIAN_LITTLE
			mtd = (CF_MethodInfo*)&buf[sz];
			_CF_ENDIAN_SWAP_ACCES( mtd->access.flags );
			_CF_ENDIAN_SWAP_INDEX( mtd->name_idx );
			_CF_ENDIAN_SWAP_INDEX( mtd->desc_idx );
			_CF_ENDIAN_SWAP_COUNT( mtd->attr_count );
#endif
			sz += method_len[c];
		}
	}


	// write: attributes_count
	cnt = (cf_count_t*)&buf[sz];
	*cnt = _CF_ENDIAN_FLIP_COUNT( cf_map->attrtab[0].attr_count );
	sz += sizeof( cf_count_t );
	if( *cnt )
	{
		// write: attributes
		CF_AttrInfo **attr = cf_map->attrtab[0].attr;
		m = cf_map->attrtab[0].attr_count;

		for( c=0; c < m; c++ )
		{
			n = attr[c]->attr_len;
			n = sizeof(CF_AttrHead) + _CF_ENDIAN_FLIP_U4( n );
			memcpy( &buf[sz], attr[c], n );
			sz += n;
		}
	}


	return sz;
}

//
static int cf_map2file_bin( CF_ClassFileMap *cf_map, FILE *hfile )
{
	int c, m;
	ssize_t sz, ln;
	CF_ClassFile cf = {0};


#ifdef _ARCH_ENDIAN_LITTLE
	cf_map_endian_flip( cf_map, 0, &cf, 0x1f );  // flip ENDIAN excepts all attributes
#endif


	// write: magic, version, cp_count
	sz = _OFFSET( CF_ClassFile, cps );
	ln = fwrite( &cf, 1, sz, hfile );
	if( ln != sz || ferror(hfile) )
		goto LAB_FINAL;


	if( cf.cp_count )
	{
		// write: constants pool
		if( cf_cp2file( &cf_map->cptab[0], hfile ) == -1 )
			goto LAB_FINAL;
	}


	// write: access, this_class, super_class, if_count
	sz = _OFFSET( CF_ClassFile, ifs ) - _OFFSET( CF_ClassFile, access );
	ln = fwrite( &cf.access, 1, sz, hfile );
	if( ln != sz || ferror(hfile) )
		goto LAB_FINAL;


	if( cf.if_count )
	{
		// write: interfaces
		sz = sizeof(cf_idx_t) * cf_map->iftab[0].if_count;
		ln = fwrite( cf_map->iftab[0].ifs, 1, sz, hfile );
		if( ln != sz || ferror(hfile) )
			goto LAB_FINAL;
	}


	// write: field_count
	ln = fwrite( &cf.field_count, sizeof(cf.field_count), 1, hfile );
	if( ln != 1 || ferror(hfile) )
		goto LAB_FINAL;

	if( cf.field_count )
	{
		// write: fields
		CF_FieldInfo **field = cf_map->fdtab[0].fields;
		ssize_t *field_len = cf_map->fdtab[0].fields_len;
		m = cf_map->fdtab[0].field_count;
		for( c=0; c < m; c++ )
		{
			ln = fwrite( field[c], 1, field_len[c], hfile );
			if( ln != field_len[c] || ferror(hfile) )
				goto LAB_FINAL;
		}
	}


	// write: method_count
	ln = fwrite( &cf.method_count, sizeof(cf.method_count), 1, hfile );
	if( ln != 1 || ferror(hfile) )
		goto LAB_FINAL;

	if( cf.method_count )
	{
		// write: methods
		CF_MethodInfo **method = cf_map->mdtab[0].methods;
		ssize_t *method_len = cf_map->mdtab[0].methods_len;
		m = cf_map->mdtab[0].method_count;
		for( c=0; c < m; c++ )
		{
			ln = fwrite( method[c], 1, method_len[c], hfile );
			if( ln != method_len[c] || ferror(hfile) )
				goto LAB_FINAL;
		}
	}


	// write: attr_count
	ln = fwrite( &cf.attr_count, sizeof(cf.attr_count), 1, hfile );
	if( ln != 1 || ferror(hfile) )
		goto LAB_FINAL;

	if( cf.attr_count )
	{
		// write: attributes
		CF_AttrInfo **attr = cf_map->attrtab[0].attr;
		m = cf_map->attrtab[0].attr_count;
		for( c=0; c < m; c++ )
		{
			sz = sizeof(CF_AttrInfo) + _CF_ENDIAN_FLIP_U4( attr[c]->attr_len );
			ln = fwrite( attr[c], 1, sz, hfile );
			if( ln != sz || ferror(hfile) )
				goto LAB_FINAL;
		}
	}

	cf.magic = 0;


	LAB_FINAL:
#ifdef _ARCH_ENDIAN_LITTLE
	cf_map_endian_flip( cf_map, 0, &cf, 0x1e );  // restore ENDIAN excepts all attributes and the head
#endif

	return -(cf.magic != 0);
}

static int cf_map2file_json( const CF_ClassFileMap *cf_map, FILE *hfile )
{
	errno = errUnsupport;
	return -1;
}

static int cf_map2file_xml( const CF_ClassFileMap *cf_map, FILE *hfile )
{
	errno = errUnsupport;
	return -1;
}

static int cf_map2file_map( const CF_ClassFileMap *cf_map, FILE *hfile )
{
	errno = errUnsupport;
	return -1;
}


// Function:
CF_HIDDEN ssize_t cf_map2buffer( CF_ClassFileMap *cf_map, void *buf, ssize_t bufsize, int format )
{
	// Note:
	// 1, only write committed groups
	// 2, assume the mapping has been verified before call this routine
	// 3, all ENDIAN is local order, except all attributes that are always represented by bigger-byte-order

	if( !cf_map || format < CF_FORMAT_BIN || format > CF_FORMAT_MAP )
	{
		errno = EINVAL;
		return 0;
	}

	int r = 0;

	switch( format )
	{
	case CF_FORMAT_BIN:		// Original Binary Format with the Class File Standard
		r = cf_map2buffer_bin( cf_map, buf, bufsize );
		break;
	case CF_FORMAT_JSON:	// JSon Format with the Class File Standard
		break;
	case CF_FORMAT_XML:		// XML Format with the Class File Standard
		break;
	case CF_FORMAT_MAP:		// Output original mapping directly
		break;
	default:
		break;
	}

	return r;
}


// Function:
CF_HIDDEN int cf_map2file( CF_ClassFileMap *cf_map, const char *classfile, int format )
{
	// Note:
	// 1, only write committed groups
	// 2, assume the mapping has been verified before call this routine
	// 3, all ENDIAN is local order, except all attributes that are always represented by bigger-byte-order

	if( !cf_map || !classfile || (format < CF_FORMAT_BIN || format > CF_FORMAT_MAP) )
	{
		errno = EINVAL;
		return -1;
	}


	FILE *hfile = fopen( classfile, "w+b" );
	if( !hfile )
		return -1;


	int r = -1;

	switch( format )
	{
	case CF_FORMAT_BIN:		// Original Binary Format with the Class File Standard
		r = cf_map2file_bin( cf_map, hfile );
		break;
	case CF_FORMAT_JSON:	// JSon Format with the Class File Standard
		r = cf_map2file_json( cf_map, hfile );
		break;
	case CF_FORMAT_XML:		// XML Format with the Class File Standard
		r = cf_map2file_xml( cf_map, hfile );
		break;
	case CF_FORMAT_MAP:		// Output original mapping directly
		r = cf_map2file_map( cf_map, hfile );
		break;
	default:
		break;
	}

	fclose( hfile );
	return r;
}


