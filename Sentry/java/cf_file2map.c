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



//
static CF_UNUSED int cf_append_new_block( CF_ClassFileMap *cf_map, size_t blksize )
{
	// Note: DO NOT change the file position arbitrarily;
	//       and also DO NOT arbitrarily modify the buffer be referred to by the CF_ClassFileMap.auxi.buf;

	IOBUF_T *buf = &cf_map->auxi.buf;
	void *ptr = cf_malloc( cf_map->auxi.use_sysheap, &cf_map->auxi.mmphd, blksize );
	if( !ptr )
		return -1;

	// merge previous remainders and update the buffer
	ssize_t ln = IOBUF_AVAILABLE( buf );
	if( ln > 0 )
		memcpy( ptr, IOBUF_PTR(buf), (buf->num = ln) );
	else
		buf->num = 0;
	buf->addr = ptr;
	buf->size = blksize;
	buf->pos = 0;

	return 0;
}


// allocate new buffer and read file block into the it
static int cf_append_new_block2( CF_ClassFileMap *cf_map, size_t blksize )
{
	// Note: DO NOT change the file position arbitrarily;
	//       and also DO NOT arbitrarily modify the buffer be referred to by the CF_ClassFileMap.auxi.buf;

	FILE *hf = cf_map->auxi.hfile;

	if( feof(hf) || ferror(hf) )
		return -1;

	ssize_t ln;
	IOBUF_T *buf = &cf_map->auxi.buf;
	ssize_t alloc_size = cf_map->auxi.filesize - ftell( hf ) + IOBUF_AVAILABLE( buf );

	if( !cf_map->auxi.blksz_abap && alloc_size > blksize )
		alloc_size = blksize; // choose small one between the blksize and the remained file contents

	if( IOBUF_CAPACITY(buf) < alloc_size )
	{
		void *ptr = cf_malloc( cf_map->auxi.use_sysheap, &cf_map->auxi.mmphd, alloc_size );
		if( !ptr )
			return -1;

		// merge previous remainders and update the buffer
		ln = IOBUF_AVAILABLE( buf );
		if( ln > 0 )
			memcpy( ptr, IOBUF_PTR(buf), (buf->num = ln) );
		else
			buf->num = 0;
		buf->addr = ptr;
		buf->size = alloc_size;
		buf->pos = 0;
	}

	// read file contents
	ln = fread( IOBUF_TAIL(buf), 1, IOBUF_CAPACITY(buf), hf );
	if( ln < 1 || ferror(hf) )
		return -1;

	IOBUF_ADD( buf, ln );

	return 0;
}


//
static void* cf_load_bytes( ssize_t len, CF_ClassFileMap *cf_map )
{
	FILE *hf = cf_map->auxi.hfile;
	IOBUF_T *buf = &cf_map->auxi.buf;
	ssize_t blksize = 0;
	ssize_t ava;
	void *ptr;

	if( buf->addr )
	{
		if( len <= (ava=IOBUF_AVAILABLE( buf )) )
		{
			// enough bytes to read, return immediately
			ptr = IOBUF_PTR( buf );
			IOBUF_SEEK2( buf, len );
			return ptr;
		}

		// not enough bytes to read from buffer
		if( len > buf->size - buf->pos )
		{
			// no capability to hold bytes required
			// need to append a new block
			blksize = 1;
		}
	}
	else
	{
		// append a new block at first
		blksize = 1;
		ava = 0;
	}

	if( blksize )
	{
		// allocate new block with fitting size
		// and, current buffer will be kept
		ptr = cf_malloc( cf_map->auxi.use_sysheap, &cf_map->auxi.mmphd, len );
		if( !ptr )
			return NULL;

		if( ava > 0 )
		{
			memcpy( ptr, IOBUF_PTR(buf), ava );
			buf->num -= ava;
		}
	}
	else
		ptr = IOBUF_TAIL( buf );

	// read file contents
	ssize_t n = fread( ptr+ava, 1, len-ava, hf );
	if( ferror(hf) || n+ava != len )  // MUST equal to the len
	{
		if( blksize )
			cf_free( cf_map->auxi.use_sysheap, &cf_map->auxi.mmphd, ptr );

		return NULL;
	}

	return ptr;
}

// read constants pool from file
static int cf_load_constants_pool( CF_ClassFileMap *cf_map, cf_count_t cptab_idx )
{
	IOBUF_T *buf = &cf_map->auxi.buf;
	CP_Info **cps = cf_map->cptab[cptab_idx].cps;

	if( !buf->addr )
		IOBUF_CLEAR( buf );

	int local_endian = (cf_map->auxi.local_endian_value<<1 | cf_map->auxi.local_endian_index) & 0x03;

	juint8 *data;
	ssize_t pos;
	ssize_t len;
	int nn, units;
	ssize_t addblksize = 0;
	cf_count_t c = 1;
	cf_count_t cp_count = cf_map->cptab[cptab_idx].cp_count;
	ssize_t cps_size = 0;
	cps[0] = NULL;

	while( c < cp_count )
	{
		if( addblksize > 0 || IOBUF_AVAILABLE(buf) < 3 )
		{
			len = IOBUF_CAPACITY( buf );
			if( len < 16 )  // addblksize
				len = _MEMPOOL_ALLOC_FIXSIZE;

			if( cf_append_new_block2( cf_map, len ) == -1 )
				return -1;

			addblksize = 0;
		}

		data = buf->addr;
		pos = buf->pos;
		len = buf->num;
		while( c < cp_count && pos < len )
		{
			if( (nn = cf_chk_cpitem( (CP_Info*)&data[pos], len-pos, &units, local_endian )) == 0 )
				return -1;  // undefined
			else if( nn < 0 )
			{
				addblksize = -nn;
				break;  // need to read contents
			}

			cps[c] = (CP_Info*)&data[pos];
			pos += nn;
			cps_size += nn;

			//if( units == 2 ) cps[c+1] = cps[c];
			c += units;
		}
		buf->pos = pos;
	}

	cf_map->cptab[cptab_idx].cps_size = cps_size;

	return c-1;
}


// read attributes
static int cf_load_attributes( CF_ClassFileMap *cf_map, CF_AttrInfo **attr, cf_count_t count )
{
	IOBUF_T *buf = &cf_map->auxi.buf;
	if( !buf->addr )
		IOBUF_CLEAR( buf );

	int local_endian = (cf_map->auxi.local_endian_value<<1 | cf_map->auxi.local_endian_index) & 0x03;

	juint8 *data;
	ssize_t pos;
	ssize_t len;

	ssize_t nn;
	IOBUF_T __buf = { .addr=NULL };
	ssize_t addblksize = 0;
	cf_count_t c = 0;

	while( c < count )
	{
		if( addblksize > 0 || IOBUF_AVAILABLE(buf) < sizeof(CF_AttrInfo) )
		{
			__buf.addr = NULL;

			len = addblksize > 0 ? addblksize : 32;
			if( (nn = buf->size - buf->pos) >= len )
			{
				// directly read file
				nn = fread( IOBUF_TAIL(buf), 1, IOBUF_CAPACITY(buf), cf_map->auxi.hfile );
				if( ferror(cf_map->auxi.hfile) || nn < 1 )
					return -1;
			}
			else
			{
				// need a new block

				if( nn >= 64 )
				{
					// the current buffer will be kept
					// meanwhile, it means the addblksize is greater than 64
					// DO NOT change the addblksize
					__buf = *buf;

					if( cf_append_new_block2( cf_map, addblksize ) == -1 )
						return -1;
				}
				else
				{
					// try to allocate a new block
					// the current buffer will be sealed
					if( addblksize < _MEMPOOL_ALLOC_FIXSIZE )
						addblksize = _MEMPOOL_ALLOC_FIXSIZE;

					if( cf_append_new_block2( cf_map, addblksize ) == -1 )
						return -1;
				}
			}
			addblksize = 0;
		}

		data = buf->addr;
		pos = buf->pos;
		len = buf->num;
		for( ; c < count && pos < len; ++c )
		{
			if( (nn = cf_chk_attr( (CF_AttrInfo*)&data[pos], 1, len-pos, cf_map->auxi.hfile, local_endian )) == 0 )
				return -1;  // undefined
			else if( nn < 0 )
			{
				addblksize = len - pos - nn;  // whole attribute size
				break;  // need to read contents
			}

			attr[c] = (CF_AttrInfo*)&data[pos];
			pos += nn;
		}

		if( __buf.addr )
			*buf = __buf;
		else
			buf->pos = pos;
	}

	return c;
}


// read fields
static int cf_load_fields( CF_ClassFileMap *cf_map, CF_FieldInfo **fds, ssize_t *fds_len, cf_count_t count )
{
	IOBUF_T *buf = &cf_map->auxi.buf;
	if( !buf->addr )
		IOBUF_CLEAR( buf );

	int local_endian = (cf_map->auxi.local_endian_value<<1 | cf_map->auxi.local_endian_index) & 0x03;

	juint8 *data;
	ssize_t pos;
	ssize_t len;

	ssize_t nn;
	IOBUF_T __buf = { .addr=NULL };
	ssize_t addblksize = 0;
	cf_count_t c = 0;

	while( c < count )
	{
		if( addblksize > 0 || IOBUF_AVAILABLE(buf) < sizeof(CF_FieldInfo) )
		{
			__buf.addr = NULL;

			len = addblksize > 0 ? addblksize : 32;  // sizeof(CF_FieldInfo) * 4
			if( (nn = buf->size - buf->pos) >= len )
			{
				// directly read file
				nn = fread( IOBUF_TAIL(buf), 1, IOBUF_CAPACITY(buf), cf_map->auxi.hfile );
				if( ferror(cf_map->auxi.hfile) || nn < 1 )
					return -1;
			}
			else
			{
				// need a new block

				if( nn >= 128 )
				{
					// the current buffer will be kept
					// meanwhile, it means the addblksize is greater than 128
					// DO NOT change the addblksize
					__buf = *buf;

					if( cf_append_new_block2( cf_map, addblksize ) == -1 )
						return -1;
				}
				else
				{
					// try to allocate a new block
					// the current buffer will be sealed
					if( addblksize < _MEMPOOL_ALLOC_FIXSIZE )
						addblksize = _MEMPOOL_ALLOC_FIXSIZE;

					if( cf_append_new_block2( cf_map, addblksize ) == -1 )
						return -1;
				}
			}
			addblksize = 0;
		}

		data = buf->addr;
		pos = buf->pos;
		len = buf->num;
		for( ; c < count && pos < len; ++c )
		{
			if( (nn = cf_chk_field( (CF_FieldInfo*)&data[pos], len-pos, cf_map->auxi.hfile, local_endian )) == 0 )
				return -1;  // undefined
			else if( nn < 0 )
			{
				addblksize = len - pos - nn;  // whole field size
				break;  // need to read contents
			}

			fds[c] = (CF_FieldInfo*)&data[pos];
			fds_len[c] = nn;
			pos += nn;
		}

		if( __buf.addr )
			*buf = __buf;
		else
			buf->pos = pos;
	}

	return c;
}

#define cf_load_methods		cf_load_fields

//
static FILE* cf_openfile( const char *classfile, CF_ClassFile *cfhd, ssize_t *filesize, int inbuf )
{
	// Note: after succeed to finish the job, the file pointer will be set at the first CP_Info of the file

	FILE *hf;

	hf = inbuf ? (FILE*)classfile : fopen( classfile, "rb" );
	if( hf )
	{
		// get file size
		fseek( hf, 0, SEEK_END );
		ssize_t fsz = ftell( hf );
		if( fsz <= (ssize_t)sizeof(CF_ClassFile) )
			goto LAB_EXIT;

		fseek( hf, 0, SEEK_SET );

		// read the head at first, and then check it validation
		size_t sz;
		sz = fread( (void*)cfhd, _OFFSET(CF_ClassFile, cps), 1, hf );
		if( sz != 1 )
			goto LAB_EXIT;

		// check magic value
		if( _CF_ENDIAN_SWAP_MAGIC(cfhd->magic) != CF_MAGIC_VALUE )
			goto LAB_EXIT;

		// check version
		_CF_ENDIAN_SWAP_U2( cfhd->minor_ver );
		_CF_ENDIAN_SWAP_U2( cfhd->major_ver );

		// check file size again
		if( _CF_ENDIAN_SWAP_COUNT(cfhd->cp_count) == 0 ||
			sizeof(CF_ClassFile) + cfhd->cp_count * sizeof(CP_Info) >= fsz )
			goto LAB_EXIT;

		*filesize = fsz;
		return hf;

		LAB_EXIT:
		if( !inbuf )
			fclose( hf );
	}
	return NULL;
}

//
static ssize_t cf_readfile( void *dst, ssize_t len, CF_ClassFileMap *cf_map )
{
	IOBUF_T *buf = &cf_map->auxi.buf;
	ssize_t ava;

	if( buf->addr )
	{
		if( (ava=IOBUF_AVAILABLE( buf )) > 0 )
		{
			if( ava > len )
				ava = len;
			memcpy( dst, IOBUF_PTR(buf), ava );
			IOBUF_SEEK2( buf, ava );
			len -= ava;
		}
	}
	else
		ava = 0;

	if( len > 0 )
	{
		// read file contents
		ava += fread( dst+ava, 1, len, cf_map->auxi.hfile );
		if( ferror(cf_map->auxi.hfile) )
			return -1;
	}

	return ava;
}


//
static int cf_file2map_json( FILE *hfile, CF_ClassFileMap *cf_map )
{
	errno = errUnsupport;
	return -1;
}


//
static int cf_file2map_xml( FILE *hfile, CF_ClassFileMap *cf_map )
{
	errno = errUnsupport;
	return -1;
}


//
static int cf_file2map_map( FILE *hfile, CF_ClassFileMap *cf_map )
{
	errno = errUnsupport;
	return -1;
}



// Function:
CF_HIDDEN int cf_file2map( const char *classfile, CF_ClassFileMap *cf_map, int format )
{
	FILE *hf;
	ssize_t sz;
	int buf_mode = (uint32)format >> 31;
	int light_mode = (format >> 30) & 1;

	format &= 0xFF;

	if( format > CF_FORMAT_JSON && format <= CF_FORMAT_MAP )
	{
		hf = buf_mode ? (FILE*)classfile : fopen( classfile, "rb" );
		if( !hf )
			return -1;

		// get file size
		fseek( hf, 0, SEEK_END );
		sz = ftell( hf );
		fseek( hf, 0, SEEK_SET );

		// clear map
		memset( cf_map, 0, CF_CLASSFILE_MAP_SIZE );
		memset( &cf_map->auxi.buf, 0, sizeof(cf_map->auxi.buf) );
		cf_map->auxi.hfile = hf;
		cf_map->auxi.filesize = sz;


		int r;

		switch( format )
		{
		case CF_FORMAT_JSON:	// JSon Format with the Class File Standard
			r = cf_file2map_json( hf, cf_map );
			break;
		case CF_FORMAT_XML:		// XML Format with the Class File Standard
			r = cf_file2map_xml( hf, cf_map );
			break;
		case CF_FORMAT_MAP:		// Output original mapping directly
			r = cf_file2map_map( hf, cf_map );
			break;
		default:
			r = -1;
		}

		if( !buf_mode )  // r != 0 &&
			fclose( hf );

		cf_map->auxi.hfile = NULL;

		return r;
	}


	// following, load standard binary class file

	CF_ClassFile cf;

	hf = cf_openfile( classfile, &cf, &sz, buf_mode );
	if( !hf )
		return -1;

	if( cf_create_mempool( cf_map->auxi.use_sysheap, &cf_map->auxi.mmphd ) == -1 )
		goto LAB_EXIT;


	// clear map
	memset( cf_map, 0, CF_CLASSFILE_MAP_SIZE );
	memset( &cf_map->auxi.buf, 0, sizeof(cf_map->auxi.buf) );
	cf_map->auxi.hfile = hf;
	cf_map->auxi.filesize = sz;

	//
	sz = (sizeof(struct _cf_cptab_item)
		  + sizeof(struct _cf_iftab_item)
		  + sizeof(struct _cf_fdtab_item)
		  + sizeof(struct _cf_mdtab_item)
		  + sizeof(struct _cf_attrtab_item)
		  ) * CF_MAP_TABLE_CAPA + sizeof(void*)*(cf.cp_count + 2);  // 2 padding
	void **vptr = (void**)cf_malloc( cf_map->auxi.use_sysheap, &cf_map->auxi.mmphd, sz );
	if( !vptr )
		goto LAB_EXIT2;

	memset( vptr, 0, sz );
	cf_map->cptab = (struct _cf_cptab_item*)vptr;
	cf_map->iftab = (struct _cf_iftab_item*)((void*)cf_map->cptab + sizeof(struct _cf_cptab_item)*CF_MAP_TABLE_CAPA);
	cf_map->fdtab = (struct _cf_fdtab_item*)((void*)cf_map->iftab + sizeof(struct _cf_iftab_item)*CF_MAP_TABLE_CAPA);
	cf_map->mdtab = (struct _cf_mdtab_item*)((void*)cf_map->fdtab + sizeof(struct _cf_fdtab_item)*CF_MAP_TABLE_CAPA);
	cf_map->attrtab = (struct _cf_attrtab_item*)((void*)cf_map->mdtab + sizeof(struct _cf_mdtab_item)*CF_MAP_TABLE_CAPA);

	// ======================================================================================================================
	// to load the Constants Pool

	cf_map->cptab[0].cps = (CP_Info**)((void*)cf_map->attrtab + sizeof(struct _cf_attrtab_item)*CF_MAP_TABLE_CAPA);
	cf_map->cptab[0].cp_count = cf.cp_count;
	cf_map->cptab_count = 1;
	if( cf_load_constants_pool( cf_map, 0 ) != cf.cp_count-1 )
		goto LAB_EXIT2;


	// ======================================================================================================================
	// read the "access", "this_class", "super_class", "interfaces_count"
	ssize_t len = _OFFSET(CF_ClassFile, ifs) - _OFFSET(CF_ClassFile, access);
	if( cf_readfile( &cf.access, len, cf_map ) != len )
		goto LAB_EXIT2;

	_CF_ENDIAN_SWAP_COUNT( cf.if_count );

	// ======================================================================================================================
	// to load the interfaces

	cf_idx_t *idx = (cf_idx_t*)cf_load_bytes( cf.if_count*sizeof(cf_idx_t)+sizeof(cf_count_t), cf_map );
	if( !idx )
		goto LAB_EXIT2;

	cf_map->iftab[0].ifs = cf.if_count > 0 ? idx : NULL;
	cf_map->iftab[0].if_count = cf.if_count;
	cf_map->iftab_count = !!cf.if_count;

#ifdef _ARCH_ENDIAN_LITTLE
	if( cf.if_count > 0 ) // && cf_map->auxi.local_endian_index )
	{
		// flip byte order( big => little )
		cf_count_t c;
		for( c=0; c < cf.if_count; c++ )
			_CF_ENDIAN_SWAP_INDEX( idx[c] );
	}
#endif


	// ======================================================================================================================
	// ready to load fields

	cf.field_count = idx[cf.if_count];    // the fields_count is saved at the end of the "idx"
	_CF_ENDIAN_SWAP_COUNT( cf.field_count );
	if( cf.field_count > 0 )
	{
		sz = sizeof(void*) * cf.field_count * 2;  // double size, for store related field length
		vptr = (void**)cf_malloc( cf_map->auxi.use_sysheap, &cf_map->auxi.mmphd, sz );
		if( !vptr )
			goto LAB_EXIT2;

		memset( vptr, 0, sz );
		cf_map->fdtab[0].fields = (CF_FieldInfo**)vptr;
		cf_map->fdtab[0].fields_len = (ssize_t*)&vptr[cf.field_count];
		cf_map->fdtab[0].field_count = cf.field_count;
		if( cf_load_fields( cf_map, cf_map->fdtab[0].fields, cf_map->fdtab[0].fields_len, cf.field_count ) != cf.field_count )
			goto LAB_EXIT2;
	}
	else
	{
		cf_map->fdtab[0].fields = NULL;
		cf_map->fdtab[0].field_count = 0;
	}
	cf_map->fdtab_count = !!cf.field_count;


	// ======================================================================================================================
	// ready to load methods

	if( cf_readfile( &cf.method_count, sizeof(cf_count_t), cf_map ) != sizeof(cf_count_t) )  // read the methods_count
		goto LAB_EXIT2;

	_CF_ENDIAN_SWAP_COUNT( cf.method_count );
	if( cf.method_count > 0 )
	{
		sz = sizeof(void*) * cf.method_count * 2;  // double size, for store related method length
		vptr = (void**)cf_malloc( cf_map->auxi.use_sysheap, &cf_map->auxi.mmphd, sz );
		if( !vptr )
			goto LAB_EXIT2;

		memset( vptr, 0, sz );
		cf_map->mdtab[0].methods = (CF_MethodInfo**)vptr;
		cf_map->mdtab[0].methods_len = (ssize_t*)&vptr[cf.method_count];
		cf_map->mdtab[0].method_count = cf.method_count;
		if( cf_load_methods( cf_map, cf_map->mdtab[0].methods, cf_map->mdtab[0].methods_len, cf.method_count ) != cf.method_count )
			goto LAB_EXIT2;
	}
	else
	{
		cf_map->mdtab[0].methods = NULL;
		cf_map->mdtab[0].method_count = 0;
	}
	cf_map->mdtab_count = !!cf.method_count;


	if( !light_mode )
	{
		// ======================================================================================================================
		// ready to load attributes

		if( cf_readfile( &cf.attr_count, sizeof(cf_count_t), cf_map ) != sizeof(cf_count_t) )  // read the attributes_count
			goto LAB_EXIT2;

		_CF_ENDIAN_SWAP_COUNT( cf.attr_count );
		if( cf.attr_count > 0 )
		{
			sz = sizeof(void*) * cf.attr_count;
			vptr = (void**)cf_malloc( cf_map->auxi.use_sysheap, &cf_map->auxi.mmphd, sz );
			if( !vptr )
				goto LAB_EXIT2;

			memset( vptr, 0, sz );
			cf_map->attrtab[0].attr = (CF_AttrInfo**)vptr;
			cf_map->attrtab[0].attr_count = cf.attr_count;
			if( cf_load_attributes( cf_map, (CF_AttrInfo**)vptr, cf.attr_count ) != cf.attr_count )
				goto LAB_EXIT2;
		}
		else
		{
			cf_map->attrtab[0].attr = NULL;
			cf_map->attrtab[0].attr_count = 0;
		}
		cf_map->attrtab_count = !!cf.attr_count;
	}
	else
	{
		// DO NOT load attributes(2023-02-08)
		cf_map->attrtab[0].attr = NULL;
		cf_map->attrtab[0].attr_count = 0;
		cf_map->attrtab_count = 0;
	}


	// ======================================================================================================================
	// finished
	cf_map->magic = cf.magic;
	cf_map->minor_ver = cf.minor_ver;
	cf_map->major_ver = cf.major_ver;

#ifdef _ARCH_ENDIAN_LITTLE
	if( 1 ) // cf_map->auxi.local_endian_value )
		cf_map->access.flags = _CF_ENDIAN_FLIP_ACCES( cf.access.flags );
	else
#endif
		cf_map->access = cf.access;

#ifdef _ARCH_ENDIAN_LITTLE
	if( 1 ) // cf_map->auxi.local_endian_index )
	{
		cf_map->this_class = _CF_ENDIAN_FLIP_INDEX( cf.this_class );
		cf_map->super_class = _CF_ENDIAN_FLIP_INDEX( cf.super_class );
	}
	else
#endif
	{
		cf_map->this_class = cf.this_class;
		cf_map->super_class = cf.super_class;
	}

	if( !(cf_map->auxi.src_buf = buf_mode) )
		fclose( hf );

	cf_map->auxi.hfile = NULL;

	return 0;


	LAB_EXIT2:
	cf_destroy_mempool( cf_map->auxi.use_sysheap, &cf_map->auxi.mmphd );
	//cf_map->auxi.mmphd = 0;
	memset( &cf_map->auxi.buf, 0, sizeof(cf_map->auxi.buf) );


	LAB_EXIT:
	if( !buf_mode )
		fclose( hf );
	cf_map->auxi.hfile = NULL;

	return -1;
}


// Function:
CF_HIDDEN int cf_buffer2map( const void *buf, ssize_t bufsize, CF_ClassFileMap *cf_map, int format )
{
	FILE *hfile = fmemopen( (void*)buf, bufsize, "rb" );
	if( !hfile )
		return -1;

	int r = cf_file2map( (const char *)hfile, cf_map, format | 0x80000000 );

	fclose( hfile );
	return r;
}
