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

#ifndef _JVAS_TC_H_
#define _JVAS_TC_H_

#include "sabi.h"
#include "jvas.h"
#include "classfile_util.h"
#include "symtab.h"
#include "facility/dlink.h"
#include "sys/semLib.h"

#define _JVAS_TC_TPIDX_MANAGE_EN	(FALSE)	// TPIDX will be managed by jvas_tc.c

#define _JVAS_TC_DEBUG_PRINT_	_JVAS_DEBUG_PRINT_
//#define _JVAS_TC_DEBUG_PRINT_( _FD_, _LOG_TYPE_, _FORMAT_, _ARGS_... )    _DEBUG_PRINT_( _FD_, _LOG_TYPE_, "jvas_tc:"_FORMAT_, ##_ARGS_ )

typedef struct
{
	char *name;
	char *sign;
	char *name2;
	int   cpidx;
}jvas_tc_name_map;


typedef struct
{
	char *klass_name;
	char *new_klass_name;
	int callee_num;
	jvas_tc_name_map *callees;
}jvas_tc_callee_map;


typedef struct
{
	dlk_node node;
	struct jvas_class_conf klass;
	struct jvas_class_conf stub;
	char *klass_ident;
	char *stub_ident;
	char *ifname;

	// callee mapping
	int callee_map_num;
	jvas_tc_callee_map *callee_map;

	//
	CF_ClassFileMap *stub_cfmap;
	SEM_ID	 sem;

#if _JVAS_TC_TPIDX_MANAGE_EN == TRUE
	ssize_t  *nref_ptr;
#endif
}jvas_tc_def;


typedef struct
{
	dlk_link lnk;
	symtab class_transformed[53];
	ssize_t count_class_transformed;
	SEM_ID	 sem;
}jvas_tc_base;

#define JVAS_TC_TRANSFORMED_CLASS_RANK(_S_) (\
		((_S_)>='A' && (_S_)<='Z') ? (_S_) - 'A' : \
		((_S_)>='a' && (_S_)<='z') ? (_S_) - 'a' + 26 : \
		sizeof(((jvas_tc_base*)0)->class_transformed)/sizeof(symtab) - 1 )

static inline int jvas_tc_transformed_rank( const char *class_name )
{
	const char *cc = strrchr( class_name, '/' );
	return cc ? JVAS_TC_TRANSFORMED_CLASS_RANK(cc[1]) : JVAS_TC_TRANSFORMED_CLASS_RANK(class_name[0]);
}


#ifdef __cplusplus
extern "C"{
#endif

JVAS_NOEXPORT int jvas_tc_read_defines( const char *fn, jvas_tc_base *tcb, BOOL load_stub );

JVAS_NOEXPORT int jvas_tc_read_def( const char *fn, jvas_tc_base *tcb );

JVAS_NOEXPORT jvas_tc_def* jvas_tc_dup_def( jvas_tc_base *tcb, jvas_tc_def *tcdef, const char *kname );

JVAS_NOEXPORT void jvas_tc_free_def( jvas_tc_def *tcdef );

JVAS_NOEXPORT int jvas_tc_find_method( const jvas_tc_def *tcdef, const char *name, const char *sign );

JVAS_NOEXPORT int jvas_tc_find_callee_map( const jvas_tc_def *tcdef, const char *klass_name );
JVAS_NOEXPORT int jvas_tc_find_callee_map2( const jvas_tc_def *tcdef, const char *klass_name, int name_len );

JVAS_NOEXPORT int jvas_tc_find_callee( jvas_tc_callee_map *map, const char *name, const char *sign );
JVAS_NOEXPORT int jvas_tc_find_callee2( jvas_tc_callee_map *map, const char *name, int name_len, const char *sign, int sign_len );


JVAS_NOEXPORT const jvas_tc_def* jvas_tc_find_define( const jvas_tc_base *tcb, const char *klass_name, BOOL onlyname, BOOL lock );

JVAS_NOEXPORT const jvas_tc_def* jvas_tc_find_define_2( const jvas_tc_base *tcb, const char *klass_name, int name_len, BOOL onlyname, BOOL lock );

JVAS_NOEXPORT int jvas_tc_transform( JTIENV_PTR jtienv, const char *name, const juint8 *src, jint srclen, juint8 **dst, jint *dstlen );

JVAS_NOEXPORT int jvas_tc_transform2( JTIENV_PTR jtienv, jvas_tc_def *tcdef,
									  const char *kname, char **mtd_mask,
									  const juint8 *src, jint srclen,
									  juint8 **dst, jint *dstlen,
									  const char *outfile,
									  CF_ClassFileMap *src_cfmap,
									  CF_ClassFileMap *ret_cfmap );

JVAS_NOEXPORT int jvas_tc_transform3( JTIENV_PTR jtienv, jvas_tc_def *tcdef, const juint8 *src, jint srclen,
		juint8 **dst, jint *dstlen, const char *outfile, CF_ClassFileMap *src_cfmap, CF_ClassFileMap *ret_cfmap );

JVAS_NOEXPORT void jvas_tc_restore( const char *name );

JVAS_NOEXPORT int jvas_tc_retransform_classes( JTIENV_PTR jtienv, JNIENV_PTR jnienv, const jvas_tc_base *tcb, BOOL load_class, BOOL lazy );

JVAS_NOEXPORT int jvas_tc_retransform_classes2( JTIENV_PTR jtienv, JNIENV_PTR jnienv, const jvas_tc_base *tcb );

JVAS_NOEXPORT int jvas_tc_retransform_all_classes( JTIENV_PTR jtienv, JNIENV_PTR jnienv, const jvas_tc_base *tcb );

JVAS_NOEXPORT int jvas_tc_remap_callee( const jvas_tc_def *tcdef, CF_ClassFileMap *cfmap );


JVAS_NOEXPORT int jvas_tc_base_open( jvas_tc_base *tcb );

JVAS_NOEXPORT void jvas_tc_base_close( jvas_tc_base *tcb, BOOL bfree );

JVAS_NOEXPORT int jvas_tc_base_add( jvas_tc_base *tcb, jvas_tc_def *node, BOOL check );

JVAS_NOEXPORT int jvas_tc_base_drop( jvas_tc_base *tcb, jvas_tc_def *node, BOOL check );

JVAS_NOEXPORT int jvas_tc_load( const char *home, jvas_tc_base *tcb );

JVAS_NOEXPORT int jvas_tc_loadEx( const char *home, jvas_tc_base *tcb, BOOL load_stub );

JVAS_NOEXPORT void jvas_tc_free( jvas_tc_base *tcb );

JVAS_NOEXPORT void jvas_tc_print( const jvas_tc_base *tcb );

JVAS_NOEXPORT
CF_MethodInfo* jvas_tc_prefix_stub_mtd( const CF_MethodInfo *stub, ssize_t stub_len,
										CF_ClassFileMap *stub_cfmap,
										ssize_t *new_stub_len, const char *stub_desc );

JVAS_NOEXPORT int jvas_tc_prefix_stub( CF_ClassFileMap *stub_cfmap, const char *fn );

#ifdef __cplusplus
}
#endif

#endif // _JVAS_TC_H_
