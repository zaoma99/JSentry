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

#ifndef _AS_VTRACE_IMPL_H_
#define _AS_VTRACE_IMPL_H_

#include <stdio.h>
#include <string.h>
#include "vtrace.h"
#include "vtrace_rule.h"
#include "mmp.h"
#include "facility/log_recorder.h"

// CAUTION: DO NOT include the "vt_api.h"


#define VT_WITH_JAVA


// fixed enrion, path or filename
#define VT_ENVNAME_HOME	"VTRACE_HOME"
#define VT_CONFPATH		"conf"
#define VT_CONFNAME		"vtrace_conf.xml"
#define VT_DATAPATH		"vtdat"
#define VT_METAPATH		"meta"
#define VT_REPOPATH		"report"
#define VT_PROOPATH		"proof"

#define _VT_IDENT_	"VTrace"

VT_HIDDEN extern vt_journal_type    g_vt_journal_mask;
VT_HIDDEN extern vt_cstr g_vt_path_data;
VT_HIDDEN extern vt_cstr g_vt_path_report;
//VT_HIDDEN extern vt_cstr g_vt_path_proof;
VT_HIDDEN extern vt_cstr g_vt_path_meta;
VT_HIDDEN extern vt_cstr g_vt_cstr_null;


#if 0 //def _DEBUG
//#define _VT_DEBUG_PRINT_( _FD_, _LOG_TYPE_, _FORMAT_, _ARGS_... )	fprintf( _FD_, _VT_IDENT_":"_FORMAT_, ##_ARGS_ )

#define _VT_DEBUG_PRINT_( _FD_, _LOG_TYPE_, _FORMAT_, _ARGS_... )	do{\
	if( (_LOG_TYPE_) & (SABI_JOURNAL_INFO|SABI_JOURNAL_DEBUG| SABI_JOURNAL_WARN | SABI_JOURNAL_ERROR| SABI_JOURNAL_EVENT| SABI_JOURNAL_CRIT | SABI_JOURNAL_MT ) ) \
		fprintf( _FD_, _FORMAT_, ##_ARGS_ ); \
}while(0)

#else

#define _VT_DEBUG_PRINT_( _FD_, _LOG_TYPE_, _FORMAT_, _ARGS_... )	do{\
	if( (_LOG_TYPE_) & g_vt_journal_mask ) \
		fprintf( _FD_, _FORMAT_, ##_ARGS_ ); \
}while(0)

#endif



struct vt_ctx_t;
struct vt_ns_t;

//
typedef struct
{
	vt_long nklass;
	vt_long nmethod;
	vt_long npoint;  // equal to the nmethod
	vt_long ndataclass;
	vt_long nrulelib;
	vt_long nrule;
	vt_long nconstraint;
	vt_class_struct  **klasses;
	vt_method_struct **methods;		// combination of all classes' methods
	vt_data_class    **dataclasses;
	vt_rule_lib      **rulelibs;
	vt_rule          **rules;
	vt_point         **tracepoints;	// the order is same as the methods
	vt_primitive_constraint **constraints;

	struct {
		vt_long _nklass;
		vt_long _nmethod;
		vt_long _npoint;

		struct {
			// by string
			vt_class_struct  **klass;
			vt_method_struct **method;  // OBSOLETED
			vt_point         **point;   // OBSOLETED
			//vt_data_class    **datkls;
		};

		struct {
			// by uid
			vt_class_struct  **klass2;
			vt_method_struct **method2;  // OBSOLETED
			vt_point         **point2;   // OBSOLETED
			//vt_data_class    **datkls2;
		};
	}index;

	struct {
		vt_long _klasses;
		vt_long _methods;
		vt_long _tracepoints;
	}caps;
}vt_ns_definition;


//
typedef struct
{
	vt_uint grp_caps;
	vt_uint ngrp;
	struct _vtrace_symgrp{
		vt_uint nsym;
		vt_cstr *syms;
	}*grp;
	long mpool;
}vt_symtab;

//
typedef struct
{
	vt_symtab symb;

	union{
		struct {
			vt_dict *trace_mode;
			vt_dict *trace_stage;
			vt_dict *trace_level;
			vt_dict *language;
			vt_dict *_cond;
			vt_dict *trace_cause;
			vt_dict *trace_kind;
			vt_dict *primitive_type;
			vt_dict *journal_type;
			vt_dict *excption_type;
			vt_dict *event_type;
			vt_dict *request_type;
			vt_dict *response_type;
			vt_dict *operator_type;
			vt_dict *operator_set;
			vt_dict *protocol;
			//
			vt_uint num_trace_mode;
			vt_uint num_trace_stage;
			vt_uint num_trace_level;
			vt_uint num_language;
			vt_uint num_cond;
			vt_uint num_trace_cause;
			vt_uint num_trace_kind;
			vt_uint num_primitive_type;
			vt_uint num_journal_type;
			vt_uint num_excption_type;
			vt_uint num_event_type;
			vt_uint num_request_type;
			vt_uint num_response_type;
			vt_uint num_operator_type;
			vt_uint num_operator_set;
			vt_uint num_protocol;
		};
		struct {
			vt_dict *grp[16];
			vt_uint ngrp[16];
			//vt_dict **sorted[16];
		};
	}dict;
}vt_ns_constant;

//
typedef struct
{
	union{
		dlk_link datobjlnk;
		struct {
			vt_long num;
			vt_dataobject_node **node;
		}datobjtab;
	};
}vt_ns_variable;

//
typedef struct
{
	vt_int thres_conf;
	struct {
		vt_cstr *data;
		vt_cstr *meta;
		vt_cstr *report;
	}path;
}vt_ns_vulner;

//
typedef struct
{
	int nthread;
	int jobmax;
}vt_ns_asyncalc;

//
typedef struct
{
	vt_mode mode;
	vt_capa caps;
	vt_ns_vulner   vulner;
	vt_ns_asyncalc asyn_calc;
	struct {
		vt_size sot_max_tp;// in trace point range
		vt_size joint_max; // in action range
		vt_uint depend_deep_max;  // reserved, it is difficult to calculates joint depth every time
		vt_uint diffuse_wide_max; // in data object range
		int sot_filter_a;
		int sot_filter_dist;
	}aux;

	struct {
		vt_time intv;
		vt_long max;
	}evtres;
}vt_ns_runconf;

//
typedef struct
{
	vt_uint type;
	vt_uint num;
	vt_time tmlast;
	int  lock;
}vt_ns_evttab_item;

//
typedef struct vt_ns_t
{
	vt_ns_runconf    runconf;
	vt_ns_constant   consts;
	vt_ns_definition defines;
	vt_ns_variable   globals;
	//dlk_link         evtlnk;
	dlk_link         ctxlnk;

	struct {
		vt_uint nite;
		vt_ns_evttab_item *ite;
	}evttab;

	char   *name;
	vt_lang lang;
	long   mmphd;
	long   mmphd_dyn;
	//long   mmphd_garb;
	SEM_ID lock;
	//SEM_ID sem_ctxlnk;
}vt_namespace;

//
typedef struct
{
	void *head;
	void *tail;
	long num;
}vt_node_buffer;

//
typedef struct vt_ctx_t
{
	VTRACE_DLNODE(vt_ctx_t) clue;
	vt_action      *act;
	vt_namespace   *ns;
	vt_refer       extarg;
	vt_symtab      locsym;
	vt_ns_variable locals;
	//vt_node_buffer buf_sot, buf_datobj, buf_joint, buf_replica, buf_tlv;
	void *last_free_sotnode;// FOR DEBUG
	long mmphd;
	SEM_ID bsem;
}vt_context;


//
//#define VT_SYMBOL_HIDDEN_POINTER


typedef struct
{
	const vt_ident *id;
	const char *name;
	vt_long tpidx;
	vt_long ext;
	vt_word ctx_aft_tp :1;
	vt_word ignore     :1;
	vt_word spec_tp    :1;
	vt_word res        :14;
	vt_word lang;
}vt_match_hint;



#ifdef __cplusplus
extern "C"{
#endif

//
#define VTRACE_DYNAMIC_MMP		(1)

#ifdef VT_STATISTIC_ENABLE

VT_LOCAL inline void* vt_malloc( long *mmp, size_t size, size_t *count )
{

	void *ptr = mmp_malloc( VTRACE_DYNAMIC_MMP, mmp, size );
	if( (long)ptr & (long)count )
		*count += size;
	return ptr;
}

VT_LOCAL inline void vt_free( long *mmp, void *ptr, size_t size, size_t *count )
{
	mmp_free( VTRACE_DYNAMIC_MMP, mmp, ptr );
	if( count )
		*count -= size;
}

#else
#define vt_malloc( _mmp_, _size_, ... )  mmp_malloc( VTRACE_DYNAMIC_MMP, _mmp_, _size_ )

#define vt_free( _mmp_, _ptr_, ...)  mmp_free( VTRACE_DYNAMIC_MMP, _mmp_, _ptr_ )
#endif

#define vt_create_mempool( _ARGS_... )	mmp_create_mempool( VTRACE_DYNAMIC_MMP, _ARGS_ )
#define vt_destroy_mempool( _ARGS_... ) mmp_destroy_mempool( VTRACE_DYNAMIC_MMP, _ARGS_ )

//
VT_LOCAL inline void* vt_take_node( long node_len, long *num, vt_node_buffer *buf, long *mmphd, size_t *count, long growth_num )
{
	void *ptr;
	long n = *num;

	if( buf->num == 0 )
	{
		if( growth_num <= 0 )
			return NULL;
		//
		ptr = vt_malloc( mmphd, growth_num*node_len, count );
		if( !ptr )
			return NULL;
		buf->num = growth_num;
		buf->head = buf->tail = ptr;
	}

	if( n > buf->num )
		n = buf->num;

	*num = n;
	ptr = buf->tail;

	buf->tail += node_len * n;
	buf->num -= n;

	return ptr;
}

VT_LOCAL inline void* vt_take_one_node( long node_len, vt_node_buffer *buf, long *mmphd, size_t *count, long growth_num )
{
	long num = 1;
	return vt_take_node( node_len, &num, buf, mmphd, count, growth_num );
}

#define vt_take_node_sot( _ARGS_... )	( (vt_sotnode*)vt_take_one_node( sizeof(vt_sotnode), _ARGS_, 256 ) )

//
VT_LOCAL inline vt_refer vt_symtab_match_in_group( struct _vtrace_symgrp *grp, const char *sym, size_t len )
{
	register vt_uint i;
	register vt_cstr *syms = grp->syms;
	for( i=grp->nsym;
		 i > 0
		 && (len!=syms[i-1].len || strncmp(sym, syms[i-1].cs, len)!=0);
		 i--
		 );
	return i==0 ? NULL : &syms[i];
}


VT_EXPORT int vt_parse_text_line( char *txt, const char *sepchr, const char **argv, int argn );
VT_EXPORT const char * vt_seek_argument( int argc, const char **argv, const char *argname );
VT_EXPORT const char * vt_seek_argument2( int argc, const char **argv, const char *argname );

VT_EXPORT void vt_dict_sort_byval( vt_dict *dict, long len );
VT_EXPORT void vt_dict_sort_bykey( vt_dict *dict, long len );
VT_EXPORT const vt_dict* vt_dict_match_val( const vt_dict *dict, long len, vt_value val );
VT_EXPORT const vt_dict* vt_dict_match_val2( const vt_dict *dict, long len, vt_value val );
VT_EXPORT const vt_dict* vt_dict_match_key( const vt_dict *dict, long len, const char *key );
VT_EXPORT const vt_dict* vt_dict_match_key2( const vt_dict *dict, long len, const char *key );
VT_EXPORT int vt_dict_getvalue_1( const vt_dict *dict, long len, const char *sym, long *val );
VT_EXPORT int vt_dict_getvalue_2( const vt_dict *dict, long len, const char *sym, long *val );

VT_EXPORT void vt_evttab_sort( vt_ns_evttab_item *ite, long nite );
VT_EXPORT vt_ns_evttab_item* vt_evttab_match( vt_ns_evttab_item *ite, long nite, vt_uint type );

VT_EXPORT int vt_symtab_init( vt_symtab *tab, vt_uint grp_caps );
VT_EXPORT void vt_symtab_release( vt_symtab *tab );
VT_EXPORT vt_refer vt_symtab_add( vt_symtab *tab, const char *sym, size_t len );
VT_EXPORT void vt_symtab_del( vt_symtab *tab, vt_refer ref );
VT_EXPORT vt_refer vt_symtab_find( vt_symtab *tab, const char *sym, size_t len );

#ifdef VT_SYMBOL_HIDDEN_POINTER
VT_EXPORT vt_cstr* vt_symtab_get( vt_symtab *tab, vt_refer ref );
#else
#define vt_symtab_get( _TABLE__, __REF__ )	(__REF__)
#endif


VT_EXPORT int vt_datobj_cache_init( vt_cache_box *cache );
VT_EXPORT void vt_datobj_cache_destroy( vt_cache_box *cache, long *mmphd );
VT_EXPORT vt_cache_item* vt_datobj_cache_insert( vt_cache_box *cache, long key, long *mmphd );
VT_EXPORT vt_cache_item* vt_datobj_cache_hit( vt_cache_box *cache, long key );
VT_EXPORT void vt_datobj_cache_drop( vt_cache_box *cache, long key );


VT_EXPORT int vt_vm_callstack_copy( vt_lang lang, vt_thread thread, int begin, int max, vt_call_frame **dst, vt_bool onlyref, long *mmphd );
VT_EXPORT vt_object vt_vm_globalref_new( vt_lang lang, vt_object obj );
VT_EXPORT void vt_vm_globalref_del( vt_lang lang, vt_object obj );
VT_EXPORT vt_object vt_vm_replica_ref_new( vt_lang lang, vt_object obj, vt_long uid );
VT_EXPORT void vt_vm_replica_ref_del( vt_lang lang, vt_object obj, vt_long uid );
VT_EXPORT int vt_vm_objcmp( vt_lang lang, vt_object obj1, vt_object obj2 );
VT_EXPORT int vt_vm_bind_object( vt_lang lang, vt_object obj, long tag );
VT_EXPORT void vt_vm_action_exit( vt_lang lang, vt_refer actref, vt_refer arg );

VT_EXPORT VT_ERROR vt_ask_feed( vt_lang lang, vt_ident *uids, vt_long uidnum, vt_refer actref, vt_refer evtref );

VT_EXPORT const vt_class_struct* vt_class_find( const vt_ident *uid, const char *name, const vt_namespace *ns );
VT_EXPORT const vt_method_struct* vt_method_find( const vt_ident *uid, const char *name, const char *sign, const char *klass, const vt_namespace *ns );
VT_EXPORT const vt_point* vt_trace_point_find( const vt_ident *uid, const char *name, const char *sign, const char *klass, const vt_namespace *ns );

VT_EXPORT const vt_class_struct* vt_class_find1( const vt_ident *uid, const vt_class_struct * const *klasses, vt_long num );
VT_EXPORT const vt_method_struct* vt_method_find1( const vt_ident *uid, const vt_method_struct * const *methods, vt_long num );
VT_EXPORT const vt_point* vt_trace_point_find1( const vt_ident *uid, const vt_point * const *points, vt_long num );

VT_EXPORT const vt_class_struct* vt_class_find2( const char *name, const vt_class_struct * const *klasses, vt_long num );
VT_EXPORT const vt_method_struct* vt_method_find2( const char *name, const char *sign, const vt_method_struct * const *methods, vt_long num );
VT_EXPORT const vt_point* vt_trace_point_find2( const char *name, const char *sign, vt_refer klass, const vt_point * const *points, vt_long num );

VT_EXPORT const vt_class_struct* vt_class_find3( const char *name, const vt_class_struct * const *klasses, vt_long num );
VT_EXPORT const vt_method_struct* vt_method_find3( const char *name, const char *sign, const vt_method_struct * const *methods, vt_long num );
VT_EXPORT const vt_point* vt_trace_point_find3( const char *name, const char *sign, vt_refer klass, const vt_point * const *points, vt_long num );

VT_NOEXPORT vt_class_struct** vt_class_struct_build_index( const vt_class_struct * const *klasses, vt_long num, vt_long ncap, long *mmphd );
VT_NOEXPORT vt_method_struct** vt_method_struct_build_index( const vt_method_struct * const *methods, vt_long num, long *mmphd );
VT_NOEXPORT vt_point** vt_trace_point_build_index( const vt_point * const *points, vt_long num, long *mmphd );
VT_NOEXPORT
int vt_method_point_build_index( vt_class_struct * const *klass_idx,
								 vt_long nklass,
								 const vt_method_struct * const *methods,
								 const vt_point * const *points,
								 vt_long num,
								 vt_long ncap,
								 vt_method_struct ***mtd_idx,
								 vt_point ***tpidx,
								 long *mmphd );

VT_EXPORT int vt_tpidx_set( vt_long new_idx, vt_long old_idx, const vt_namespace *ns, vt_bool lock );

VT_EXPORT vt_class_struct* vt_class_copy( vt_namespace *ns, vt_class_struct *dst, const vt_class_struct *src, vt_bool lock );
VT_EXPORT const vt_class_struct* vt_class_dup( vt_namespace *ns, const vt_class_struct *klass, const char *newname, vt_bool lock );

VT_NOEXPORT int vt_method_handle_set( vt_long tpidx, vt_refer hd, vt_refer actref, int tmo );

VT_EXPORT int vt_conf_load( const char *home, vt_namespace *ns );
VT_EXPORT int vt_ns_initiate( const char *home, vt_namespace *ns );
VT_EXPORT void vt_ns_destroy( vt_namespace *ns );

VT_EXPORT vt_event* vt_ns_alloc_event( vt_namespace *ns );
VT_EXPORT void vt_ns_free_event( vt_event *evt );
VT_EXPORT vt_context* vt_ns_alloc_context( vt_namespace *ns );
VT_EXPORT void vt_ns_free_context( vt_context *ctx );

VT_EXPORT vt_context* vt_alloc_context( vt_event *evt );
VT_EXPORT void vt_free_context( vt_context *ctx );

VT_EXPORT void vt_ns_print_conf( FILE *fd, vt_namespace *ns );

#ifdef __cplusplus
}
#endif

#endif //_AS_VTRACE_IMPL_H_
