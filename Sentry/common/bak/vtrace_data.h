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

#ifndef _AS_VTRACE_DATA_H_
#define _AS_VTRACE_DATA_H_

#include "vtrace.h"
#include "vtrace_impl.h"

typedef int (*vt_datobj_graph_track_echo)( vt_datobj_joint *joint, void *data, size_t deep );


#ifdef __cplusplus
extern "C"{
#endif


VT_EXPORT vt_data_class* vt_datcls_match( const vt_match_hint *hint, vt_context *ctx );    // no lock
VT_EXPORT vt_long vt_datcls_search( const vt_match_hint *hints, vt_long num, vt_data_class **klasses, vt_context *ctx );    // no lock

VT_EXPORT vt_datobj_replica* vt_datobj_replica_append( vt_data_object *datobj, vt_tlv *vals, vt_uint nval,
														vt_refer owner, vt_lang lang, vt_context *ctx, vt_bool lock );
VT_EXPORT vt_long vt_datobj_replica_append_ex( vt_data_objset *objset_dst, vt_data_sequence *datseq_src,
												vt_refer owner, vt_lang lang, vt_context *ctx, vt_bool lock );
VT_EXPORT void vt_datobj_replica_remove( vt_data_object *datobj, vt_lang lang, vt_context *ctx, vt_bool lock );
VT_EXPORT void vt_datobj_replica_remove_ex( vt_data_objset *objset_dst, vt_lang lang, vt_context *ctx, vt_bool lock );

VT_EXPORT int vt_data_journal_clone( vt_data_journal *dst, const vt_data_journal *src, vt_context *ctx );
VT_EXPORT int vt_data_exception_clone( vt_data_exception *dst, const vt_data_exception *src, vt_context *ctx );
VT_EXPORT int vt_data_callstack_clone( vt_data_callstack *dst, const vt_data_callstack *src, vt_context *ctx );

VT_EXPORT vt_data_object* vt_datobj_new( vt_data_class *klass, const vt_ident *id, const char *name, vt_long tpidx, vt_context *ctx, vt_lang lang );
VT_EXPORT void vt_datobj_del( vt_data_object *obj, vt_context *ctx, vt_lang lang );
VT_EXPORT vt_data_object* vt_datobj_acquire( vt_data_object *obj, vt_context *ctx );
VT_EXPORT vt_dataobject_node* vt_datobj_match( const vt_match_hint *hint, vt_context *ctx );    // no lock
VT_EXPORT void vt_datobj_release( vt_data_object *obj, vt_context *ctx );
VT_EXPORT vt_long vt_datobj_join( vt_data_object **objs, vt_long num, const vt_method_struct *method, vt_sotnode *curr_sot, vt_context *ctx, vt_bool lock );
VT_EXPORT vt_long vt_datobj_blind_join( vt_data_object **objs_w, vt_long num_w, const vt_method_struct *method_w,
										vt_data_object **objs_r, vt_long num_r, const vt_method_struct *method_r,
										vt_sotnode *curr_sot, vt_context *ctx, vt_bool lock );
VT_EXPORT vt_long vt_datobj_search( const vt_match_hint *hints, vt_long num, vt_data_object **objs, vt_context *ctx, vt_long *ntraced );
VT_EXPORT int vt_datobj_watch( vt_long tpidx, vt_sot *curr_sot, vt_data_sequence *datseq, vt_data_objset *objs_ret, vt_context *ctx, vt_bool lock );

VT_EXPORT void vt_datobj_references_free( dlk_link *datobjlnk, vt_lang lang, vt_context *ctx, vt_bool lock );
VT_EXPORT void vt_datobj_references_free_ex( vt_context *ctx, vt_bool lock );

VT_EXPORT vt_int vt_datobj_graph_track_depend( vt_data_objset *objset, vt_sot *sot, vt_size *track_seq );


#ifdef __cplusplus
}
#endif

#endif //_AS_VTRACE_DATA_H_
