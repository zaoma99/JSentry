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

#ifndef _CF_IMPL_H_
#define _CF_IMPL_H_

#include "classfile_constants.h"

#include "jbase.h"
#include "iobuf.h"
#include "mmp.h"
#include "classfile_util.h"


//
#define CF_MAP_SET_IDENT( __IDENT__ ) {\
	(__IDENT__).stSize = sizeof(CF_ClassFileMap); \
	(__IDENT__).chkval = sizeof(CF_ClassFileMap) ^ (size_t)&(__IDENT__); \
}

//
#define CF_MAP_CHK_IDENT( __IDENT__) \
		( ((__IDENT__).stSize ^ sizeof(CF_ClassFileMap)) ^ \
		  ((__IDENT__).chkval ^ sizeof(CF_ClassFileMap) ^ (size_t)&(__IDENT__)) )

//
#define CF_MAP_STRU2HANDLE( __STRU__ )		( (CF_MAPHD)( ((size_t)(__STRU__)) + _OFFSET(CF_ClassFileMap, auxi.ident) ) )

#define CF_MAP_HANDLE2STRU( __HANDLE__ )	( (CF_ClassFileMap*)( ((size_t)(__HANDLE__)) - _OFFSET(CF_ClassFileMap, auxi.ident) ) )


// Function:
inline static CF_ClassFileMap* cf_handle2stru( CF_MAPHD hmap )
{
	if( hmap )
	{
		CF_MapIdent ident = *(CF_MapIdent*)hmap;
		if( CF_MAP_CHK_IDENT( ident ) )
			return CF_MAP_HANDLE2STRU( hmap );
	}
	return NULL;
}




/*
#ifdef CF_MEMPOOL

#define _CF_MALLOC( __MMPOOL__, __SIZE__ )	( (void*)UM_Alloc(((UM_ID)__MMPOOL__), __SIZE__) )
#define _CF_FREE( __MMPOOL__, __PTR__ )		UM_Free( ((UM_ID)__MMPOOL__), __PTR__ )
#else

#define _CF_MALLOC( __MMPOOL__, __SIZE__ )	( (void*)malloc(__SIZE__) )
#define _CF_FREE( __MMPOOL__, __PTR__ )		free(  __PTR__ )
#endif
*/

#define _MEMPOOL_PRESIZE		(1024*8)
#define _MEMPOOL_INCSIZE		(1024*8)
#define _MEMPOOL_ALLOC_FIXSIZE	(1024*8)




#ifdef __cplusplus
extern "C"{
#endif

// from classfile_util.c

/*CF_NOEXPORT void* cf_malloc( int sys_mmp, long *mmp, size_t size );

CF_NOEXPORT void cf_free( int sys_mmp, long *mmp, void *ptr );

CF_NOEXPORT void cf_destroy_mempool( int sys_mmp, long *mmphd );

CF_NOEXPORT int cf_create_mempool( int sys_mmp, long *mmphd );
*/

#define cf_malloc	mmp_malloc
#define cf_free		mmp_free
#define cf_realloc	mmp_realloc
#define cf_create_mempool	mmp_create_mempool
#define cf_destroy_mempool	mmp_destroy_mempool



CF_NOEXPORT int cf_chk_cpitem( CP_Info *cpi, ssize_t len, int *units, int local_endian );

CF_NOEXPORT ssize_t cf_chk_field( CF_FieldInfo *field, ssize_t len, FILE *hfile, int local_endian );
#define cf_chk_method	cf_chk_field

CF_NOEXPORT int cf_cpinfo_endian_flip( CP_Info *cpi, int *units );

CF_NOEXPORT int cf_map_endian_flip( CF_ClassFileMap *cf_map, int tabno, CF_ClassFile *cfptr, int flag );



CF_NOEXPORT const CF_FieldInfo* cf_get_field_method( int which, const CF_ClassFileMap *cf_map,
												     cf_idx_t name_idx, cf_idx_t desc_idx,
												     cf_idx_t *field_idx, ssize_t *field_size );

CF_NOEXPORT const CF_FieldInfo* cf_get_field_method_by_name( int which, const CF_ClassFileMap *cf_map,
															 const char *name, const char *desc,
															 cf_idx_t *field_idx, ssize_t *field_size,
															 const CP_Utf8 **desc_full );

#define cf_get_field(__MAP__, __NAMEIDX__, __DESCIDX__, __IDXPTR__, __SIZEPTR__) \
			cf_get_field_method( 1, __MAP__, __NAMEIDX__, __DESCIDX__, __IDXPTR__, __SIZEPTR__ )

#define cf_get_method(__MAP__, __NAMEIDX__, __DESCIDX__, __IDXPTR__, __SIZEPTR__) \
			( (CF_MethodInfo*)cf_get_field_method(2, __MAP__, __NAMEIDX__, __DESCIDX__, __IDXPTR__, __SIZEPTR__) )


CF_NOEXPORT const CF_FieldInfo* cf_get_field2( const CF_ClassFileMap *cf_map, const char *name, const char *desc,
											   cf_idx_t *field_idx, ssize_t *field_size );

CF_NOEXPORT const CF_MethodInfo* cf_get_method2( const CF_ClassFileMap *cf_map, const char *name, const char *desc,
											     cf_idx_t *method_idx, ssize_t *method_len, const CP_Utf8 **desc_full );

CF_NOEXPORT const CF_FieldInfo* cf_get_field3( const CF_ClassFileMap *cf_map, cf_idx_t idx, ssize_t *field_size );

CF_NOEXPORT const CF_MethodInfo* cf_get_method3( const CF_ClassFileMap *cf_map, cf_idx_t idx, ssize_t *method_len );

CF_NOEXPORT int cf_parse_method_descriptor( const char *desc, int desc_len, CF_SignExt **ite_ptr, const CF_ClassFileMap *cfmap );



CF_NOEXPORT int cf_file2map( const char *classfile, CF_ClassFileMap *cf_map, int format );  // from cf_file2map.c

CF_NOEXPORT int cf_buffer2map( const void *buf, ssize_t bufsize, CF_ClassFileMap *cf_map, int format );  // from cf_file2map.c

CF_NOEXPORT int cf_map2file( CF_ClassFileMap *cf_map, const char *classfile, int format );  // from cf_map2file.c

CF_NOEXPORT ssize_t cf_map2buffer( CF_ClassFileMap *cf_map, void *buf, ssize_t bufsize, int format );  // from cf_map2file.c

CF_NOEXPORT ssize_t cf_chk_attr( CF_AttrInfo *attr, cf_count_t count, ssize_t len, FILE *hfile, int local_endian );  // from cf_attr.c

CF_NOEXPORT
CF_AttrInfo* cf_seek_attrinfo( const CF_AttrInfo *attr, cf_count_t count,
							   const char *name, int name_len, cf_idx_t seq,
							   const CF_ClassFileMap *cf_map );  // from cf_attr.c

CF_NOEXPORT
CF_AttrInfo* cf_match_attrinfo( const CF_AttrInfo **attr, cf_count_t count,
							    const char *name, int name_len, cf_idx_t seq,
							    const CF_ClassFileMap *cf_map, cf_idx_t *idx ); // from cf_attr.c

CF_NOEXPORT
void cf_alter_BootstrapMethod( CF_BootstrapMethod *ite, int num, int cp_off ); // from cf_attr.c

CF_NOEXPORT
void cf_alter_BootstrapMethod_2( CF_BootstrapMethod *ite, int num, cf_idx_t *new_idx ); // from cf_attr.c

CF_NOEXPORT
void cf_mark_cpi_BootstrapMethod( CF_BootstrapMethod *ite, int num, cf_idx_t *mask ); // from cf_attr.c

// Function: transform specified method according to SABI
CF_NOEXPORT
CF_MethodInfo* cf_transform_method( const CF_MethodInfo *method, ssize_t method_len, CF_ClassFileMap *cfmap,
									const CF_MethodInfo *stub, ssize_t stub_len, const CF_ClassFileMap *stub_cfmap,
									ssize_t *new_method_len, const char *method_name, const char *method_desc,
									uint8 eecm, int cpi_tpix, CF_SignExt *retval_sign );  // from cf_field_method.c

// Function: transform methods specified by the methods_name and the methods_desc;
//           and, combines both constants pool
CF_NOEXPORT
int cf_transform_class( const char *methods_name[], const char *methods_desc[], int methods_num,
						const char *stubs_name[], const char *stubs_desc[], char *flags, long *tpix,
						const char *klass_name, char *mtd_mask,
						CF_ClassFileMap *cfmap, const CF_ClassFileMap *stub_cfmap );  // from cf_field_method.c

#ifdef __cplusplus
}
#endif

#endif // _CF_IMPL_H_
