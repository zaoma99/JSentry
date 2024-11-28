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
#include "vtrace.h"
#include "vtrace_data.h"
#include "vtrace_impl.h"
#include "mmp.h"


// test requirement of global reference
#define _VT_ACQUIRE_NEWGLOBALREF_	VTRACE_DATOBJ_IS_TRACED


//
vt_data_class* vt_datcls_match( const vt_match_hint *hint, vt_context *ctx )
{
	vt_namespace *ns = ctx->ns;
	vt_data_class **klass = ns->defines.dataclasses;
	vt_long nklass = ns->defines.ndataclass;
	vt_long i;
	vt_cstr *cs;

	if( hint->id )
	{
		for( i=0; i < nklass; i++ )
		{
			if( (hint->id->low == klass[i]->uid.low | hint->id->low == -1) &
				(hint->id->high == klass[i]->uid.high | hint->id->high == -1) )
				return klass[i];
		}
	}
	else if( hint->name )
	{
		for( i=0; i < nklass; i++ )
		{
			if( (cs=(vt_cstr*)klass[i]->name) && stricmp(hint->name, cs->cs) == 0 )
				return klass[i];
		}
	}
	return NULL;
}

//
vt_long vt_datcls_search( const vt_match_hint *hints, vt_long num, vt_data_class **klasses, vt_context *ctx )
{
	vt_long i, n = 0;
	//vt_namespace *ns = ctx->ns;

	//if( BsemTake(ns->lock, -1) )
	{
		for( i=0; i < num; i++ )
		{
			if( hints[i].id || hints[i].name )
			{
				if( (klasses[i] = vt_datcls_match( &hints[i], ctx )) )
					n++;
			}
			else
				klasses[i] = NULL;
		}

		//BsemGive( ns->lock );
	}
	return n;
}



//
inline static int vt_call_frame_copy( vt_call_frame *dst, const vt_call_frame *src, vt_context *ctx )
{
	// DO NOT add to symbol table
	dst->full_path = vt_malloc( (ctx?&ctx->mmphd:NULL), (src->path_len + 1), (ctx?(size_t*)&ctx->act->sta.consum.nMem:NULL) );
	if( dst->full_path )
	{
		memcpy( dst, src, sizeof(vt_call_frame) );
		memcpy( dst->full_path, src->full_path, src->path_len );
		dst->full_path[dst->path_len] = 0;
		return 0;
	}
	return -1;
}

//
int vt_data_callstack_clone( vt_data_callstack *dst, const vt_data_callstack *src, vt_context *ctx )
{
	vt_int i;
	long *mmphd_ptr;
	size_t *count_ptr;

	dst->thread = src->thread;
	dst->num = src->num;
	if( dst->num < 1 )
	{
		// empty
		dst->frame = NULL;
		return 0;
	}

	i = 0;
	if( ctx )
	{
		mmphd_ptr = &ctx->mmphd;
		count_ptr = (size_t*)&ctx->act->sta.consum.nMem;
	}
	else
	{
		mmphd_ptr = NULL;
		count_ptr = NULL;
	}

	dst->frame = (vt_call_frame*)vt_malloc( mmphd_ptr, (sizeof(vt_call_frame) * src->num), count_ptr );
	if( dst->frame )
	{
		memset( dst->frame, 0, sizeof(vt_call_frame) * src->num );

		for( i=0; i < src->num; i++ )
		{
			if( vt_call_frame_copy( &dst->frame[i], &src->frame[i], ctx ) == -1 )
				break;
		}

		if( i < src->num )
		{
			for( --i; i >= 0; --i )
				vt_free( mmphd_ptr, dst->frame[i].full_path, dst->frame[i].path_len, count_ptr );
			vt_free( mmphd_ptr, dst->frame, (sizeof(vt_call_frame) * src->num), count_ptr );
		}
	}

	return -(i != src->num);
}

//
int vt_data_exception_clone( vt_data_exception *dst, const vt_data_exception *src, vt_context *ctx )
{
	// DO NOT add the "msgstr" to symbol table

	if( src->msglen < 1 || src->backtrace.num < 1 )
	{
		// empty
		memset( dst, 0, sizeof(vt_data_exception) );
		return 0;
	}

	dst->msgstr = vt_malloc( (ctx?&ctx->mmphd:NULL), (src->msglen+1), (ctx?(size_t*)&ctx->act->sta.consum.nMem:NULL) );
	if( dst->msgstr )
	{
		strcpy( dst->msgstr, src->msgstr );
		dst->msglen = src->msglen;
		dst->type = src->type;

		if( vt_data_callstack_clone( &dst->backtrace, &src->backtrace, ctx ) == 0 )
			return 0;

		vt_free( (ctx?&ctx->mmphd:NULL), dst->msgstr, (src->msglen+1), (ctx?(size_t*)&ctx->act->sta.consum.nMem:NULL) );
	}

	return -1;
}

//
int vt_data_journal_clone( vt_data_journal *dst, const vt_data_journal *src, vt_context *ctx )
{
	// DO NOT add the "source" and "message" to symbol table

	if( src->msglen < 1 || src->srclen < 1 )
	{
		// empty
		memset( dst, 0, sizeof(vt_data_journal) );
		return 0;
	}

	dst->message = vt_malloc( (ctx?&ctx->mmphd:NULL), (src->msglen + src->srclen + 2), (ctx?(size_t*)&ctx->act->sta.consum.nMem:NULL) );
	if( dst->message )
	{
		dst->type = src->type;
		dst->mode = src->mode;
		dst->msglen = src->msglen;
		strcpy( dst->message, src->message );

		if( (dst->srclen = src->srclen) > 0 )
		{
			dst->source = dst->message + dst->msglen + 1;
			strcpy( dst->source, src->source );
		}
		else
			dst->source = NULL;
		return 0;
	}
	return -1;
}


//
vt_datobj_replica* vt_datobj_replica_append( vt_data_object *datobj, vt_tlv *vals, vt_uint nval,
											 vt_refer owner, vt_lang lang, vt_context *ctx, vt_bool lock )
{
	vt_datobj_replica *replica;
	vt_tlv *vals_dst;
	void *ptr, *ptr_s;
	vt_uint i, n, flag;
	vt_size sz, tt = 0;
	//vt_lang lang = ((vt_sot*)owner)->dpoint->point->target->klass->lang;


	// counting valid values and memory needed
	for( i=n=0; i < nval; i++ )
	{
		if( (ptr=vt_tlv_parse_ex( &vals[i], &sz, (int*)&flag )) )
		{
			n++;
			if( flag )
				tt += sz;
		}
	}

	if( n == 0 )
		return NULL;

	if( lock && !BsemTake(ctx->bsem, -1) )
		return NULL;

	//
	sz = sizeof(vt_datobj_replica) + sizeof(vt_tlv) * n;
	replica = (vt_datobj_replica*)vt_malloc( &ctx->mmphd, (sz + tt), (size_t*)&ctx->act->sta.consum.nMem );
	if( replica )
	{
		replica->owner = owner;
		replica->nitem = n;
		replica->items = vals_dst = (vt_tlv*)&replica[1];
		ptr = (void*)replica + sz;  // starting pointer for values that be stored at external buffer

		// copy values
		for( i=n=0; i < nval; i++ )
		{
			if( (ptr_s=vt_tlv_parse_ex( &vals[i], &sz, (int*)&flag )) )
			{
				vals_dst[n] = vals[i];  // memcpy( &vals_dst[n], &vals[i], sizeof(vt_tlv) );

				if( vals_dst[n].type1 == 0 )
				{
					// new global reference
					vals_dst[n].val.l = (vt_ulong)vt_vm_replica_ref_new( lang, *(vt_object**)ptr_s, datobj->uid.low ); // ptr_s == &vals[i].val
				}
				else if( flag )
				{
					// copy external buffer
					vals_dst[n].val.p = ptr;
					memcpy( ptr, ptr_s, sz );
					ptr += sz;
				}
				n++;
			}
		}

#if VTRACE_OBJECT_REPLICA_STRUCT == VTRACE_OBJECT_REPLICA_DLINK
		dlk_Add( &datobj->dlk_replica, (dlk_node*)replica, 0 );
#else
		if( (datobj->replica_num & 7) == 0 )
		{
			ptr = vt_realloc( &ctx->mmphd, datobj->replica, sizeof(void**) * (datobj->replica_num + 8), NULL );
			if( !ptr )
			{
				vt_free( &ctx->mmphd, replica, 0, NULL );
				replica = NULL;
				goto LAB_EXIT;
			}
			memset( ptr+datobj->replica_num*sizeof(void**), 0, sizeof(void**)*8 );
			datobj->replica = (vt_datobj_replica**)ptr;
		}
		datobj->replica[datobj->replica_num++] = replica;
#endif
	}

#if VTRACE_OBJECT_REPLICA_STRUCT == VTRACE_OBJECT_REPLICA_ARRAY
	LAB_EXIT:
#endif

	if( lock )
		BsemGive( ctx->bsem );

	return replica;
}

//
vt_long vt_datobj_replica_append_ex( vt_data_objset *objset_dst, vt_data_sequence *datseq_src,
									 vt_refer owner, vt_lang lang, vt_context *ctx, vt_bool lock )
{
	if( !datseq_src->datas )
		return 0;

	vt_long i, n;
	vt_data_object **objs;
	vt_datobj_replica *_replica_last_;
	vt_tlv *dats_src;
	vt_bool isref;

	if( lock && !BsemTake(ctx->bsem, -1) )
		return -1;

	n = 0;
	dats_src = datseq_src->datas;
	objs = objset_dst->objs;
	i = objset_dst->nobj > datseq_src->ndat ? datseq_src->ndat : objset_dst->nobj;
	for( --i; i >= 0; --i )
	{
		if( objs[i] && VTRACE_OPSET_IS_TRACED(objs[i]->src) )
		{
#if VTRACE_OBJECT_REPLICA_STRUCT == VTRACE_OBJECT_REPLICA_DLINK
			_replica_last_ = (vt_datobj_replica*)objs[i]->dlk_replica.end;
#else
			_replica_last_ = objs[i]->replica_num > 0 ? objs[i]->replica[objs[i]->replica_num-1] : NULL;
#endif

			isref = ( _replica_last_ && _replica_last_->items[0].type1 == 0 && _replica_last_->items[0].val.o );

			if( dats_src[i].type1 == 0 )
			{
				// a reference
				// record a reference if the last record is not reference or there is empty
				if( isref )
					continue;
			}
			else
			{
				// a measured value
#ifndef VTRACE_OBJECT_REPLICA_KEEP_REFERENCE
				// going to replace the last reference if possible
				if( isref )
				{
#if VTRACE_OBJECT_REPLICA_STRUCT == VTRACE_OBJECT_REPLICA_DLINK
					dlk_Drop( &objs[i]->dlk_replica, (dlk_node*)_replica_last_, 0 );
#else
					objs[i]->replica_num--;
#endif
				}
#endif
			}

			//
			if( vt_datobj_replica_append( objs[i], &dats_src[i], 1, owner, lang, ctx, FALSE ) )
			{
				n++;

				if( isref )
				{
					// delete reference
					vt_vm_replica_ref_del( lang, _replica_last_->items[0].val.o, objs[i]->uid.low );

#ifndef VTRACE_OBJECT_REPLICA_KEEP_REFERENCE
					// free buffer occupied by the "_replica_last_"
					vt_free( &ctx->mmphd, _replica_last_, 0, NULL );
#else
					_replica_last_->items[0].val.o = NULL;
#endif
				}
			}
#ifndef VTRACE_OBJECT_REPLICA_KEEP_REFERENCE
			else if( isref )
			{
				// restore replica item which has been dropped before
#if VTRACE_OBJECT_REPLICA_STRUCT == VTRACE_OBJECT_REPLICA_DLINK
				dlk_Add( &objs[i]->dlk_replica, (dlk_node*)_replica_last_, 0 );
#else
				objs[i]->replica_num++;
#endif
			}
#endif
		}
	}

	if( lock )
		BsemGive( ctx->bsem );

	return n;
}

//
void vt_datobj_replica_remove( vt_data_object *datobj, vt_lang lang, vt_context *ctx, vt_bool lock )
{
#ifdef VTRACE_OBJECT_REPLICA
	if( lock && !BsemTake(ctx->bsem, -1) )
		return;

	long j, m __unused__;
	vt_datobj_replica *replica;
	vt_long uid = datobj->uid.low;

#if VTRACE_OBJECT_REPLICA_STRUCT == VTRACE_OBJECT_REPLICA_DLINK
	replica = (vt_datobj_replica*)datobj->dlk_replica.end;
	while( replica )
	{
		for( j=replica->nitem-1; j >=0; j-- )
		{
			if( replica->items[j].type1 == 0 && replica->items[j].val.o )  // && replica->items[j].len == sizeof(vt_object)
			{
				vt_vm_replica_ref_del( lang, replica->items[j].val.o, uid );
				replica->items[j].val.o = NULL;
				//break;
			}
		}
		replica = (vt_datobj_replica*)replica->clue._prior;
	}
#else
	for( m=datobj->replica_num-1; m >= 0; m-- )
	{
		replica = datobj->replica[m];
		for( j=replica->nitem-1; j >=0; j-- )
		{
			if( replica->items[j].type1 == 0 && replica->items[j].val.o )  // && replica->items[j].len == sizeof(vt_object)
			{
				vt_vm_replica_ref_del( lang, replica->items[j].val.o, uid );
				replica->items[j].val.o = NULL;
				//break;
			}
		}
	}
#endif

	/*if( vt_vm_bind_object( lang, (vt_object)datobj->uid.low, 0 ) == 0 )
	{
		//if( _VT_ACQUIRE_NEWGLOBALREF_(*datobj) )
			vt_vm_globalref_del( lang, (vt_object)datobj->uid.low );  // need to delete the global reference of this data object
	}
	datobj->uid.low = 0;
	*/

	if( lock )
		BsemGive(ctx->bsem );
#endif
}

//
void vt_datobj_replica_remove_ex( vt_data_objset *objset_dst, vt_lang lang, vt_context *ctx, vt_bool lock )
{
#ifdef VTRACE_OBJECT_REPLICA
	vt_long i;
	vt_data_object **objs;

	if( lock && !BsemTake(ctx->bsem, -1) )
		return;

	objs = objset_dst->objs;
	for( i=objset_dst->nobj-1; i >= 0; --i )
	{
		if( objs[i] )
		{
			vt_datobj_replica_remove( objs[i], lang, ctx, FALSE );
		}
	}

	if( lock )
		BsemGive( ctx->bsem );
#endif
}


//
inline static vt_data_object* _datobj_new( vt_data_class *klass, const vt_ident *id, const char *name,
										   vt_long tpidx, vt_context *ctx, vt_lang lang )
{
	vt_dataobject_node *node;
	vt_dpoint *dpt;

	if( tpidx > ctx->act->dpttab.ndpt || (tpidx > 0 && !ctx->act->dpttab.dpts[tpidx-1]) )
		return NULL;

	node = (vt_dataobject_node*)vt_malloc( &ctx->mmphd, sizeof(vt_dataobject_node), (size_t*)&ctx->act->sta.consum.nMem );
	if( node )
	{
		memset( node, 0, sizeof( vt_dataobject_node) );

		dpt = tpidx > 0 ? ctx->act->dpttab.dpts[tpidx-1] : NULL;

		node->object.mod = klass->mod;
		memcpy( &node->object.uid, id, sizeof(vt_ident) );
		node->object.name = !name ? klass->name : vt_symtab_add( &ctx->locsym, name, strlen(name) );
		//node->object.depend = klass->depend;
		//node->object.constraint = klass->constraint;
		node->object.klass = klass;
		node->object.nref = 1;
		if( tpidx > 0 )
		{
			node->object.src = dpt->point->target->opset;
			node->owner = (vt_refer)tpidx;  // dpt
		}
		else
		{
			node->object.src = dpt->point->target->opset | VTRACE_OPSET_ESCAPED;
			node->owner = NULL;
		}

		if( lang ) //&& _VT_ACQUIRE_NEWGLOBALREF_(node->object) )
		{
			// acquire a global reference of this data object
			node->object.uid.low = (vt_ulong)vt_vm_globalref_new( lang, (vt_object)id->low );

			// TODO: sign a tag at once
		}

		//__sync_fetch_and_add( &ctx->act->sta.ndatobj, 1 );
		ctx->act->sta.ndatobj++;

		if( tpidx > 0 )
			dlk_Add( &dpt->datobjlnk, (dlk_node*)node, 0 );
		else
			dlk_Add( &ctx->locals.datobjlnk, (dlk_node*)node, 0 );

		//printf( "newobj: %02x, %p, %p, %s, %s\n",
		//		node->object.src, (void*)node->object.uid.low, &node->object,
		//		((vt_cstr*)node->object.name)->cs, ((vt_cstr*)dpt->point->target->name)->cs );

#ifdef VTRACE_OBJECT_CACHE
		// caching data object
#endif
	}
	return node ? VT_DATNODE_TO_OBJ( node ) : NULL;
}

vt_data_object* vt_datobj_new( vt_data_class *klass, const vt_ident *id, const char *name,
							   vt_long tpidx, vt_context *ctx, vt_lang lang )
{
	vt_data_object *obj = NULL;
	if( BsemTake(ctx->bsem, -1) )
	{
		obj = _datobj_new( klass, id, name, tpidx, ctx, lang );
		BsemGive( ctx->bsem );
	}
	return obj;
}

//
inline static void _datobj_del( vt_data_object *obj, vt_context *ctx, vt_lang lang )
{
	vt_dataobject_node *node = VT_DATOBJ_TO_NODE( obj );
	vt_long tpidx = (vt_long)node->owner;

	if( tpidx > ctx->act->dpttab.ndpt || (tpidx > 0 && !ctx->act->dpttab.dpts[tpidx-1]) )
		return;

	if( lang ) //&& _VT_ACQUIRE_NEWGLOBALREF_(*obj) )
	{
		// TODO: cancel the tag at once

		// need to delete the global reference of this data object
		vt_vm_globalref_del( lang, (vt_object)node->object.uid.low );
	}

	//__sync_fetch_and_sub( &ctx->act->sta.ndatobj, 1 );
	ctx->act->sta.ndatobj--;

	if( tpidx > 0 )
		dlk_Drop( &ctx->act->dpttab.dpts[tpidx-1]->datobjlnk, (dlk_node*)node, 0 );
	else
		dlk_Drop( &ctx->locals.datobjlnk, (dlk_node*)node, 0 );

	vt_free( &ctx->mmphd, node, 0, NULL );
}

void vt_datobj_del( vt_data_object *obj, vt_context *ctx, vt_lang lang )
{
	if( BsemTake(ctx->bsem, -1) )
	{
		_datobj_del( obj, ctx, lang );
		BsemGive( ctx->bsem );
	}
}

//
vt_data_object* vt_datobj_acquire( vt_data_object *obj, vt_context *ctx )
{
	if( BsemTake(ctx->bsem, -1) )
	{
		if( obj->nref < VT_REF_MAX )
			obj->nref++;
		else
			obj = NULL;
		BsemGive( ctx->bsem );

		return obj;
	}
	return NULL;
}

//
void vt_datobj_release( vt_data_object *obj, vt_context *ctx )
{
	if( BsemTake(ctx->bsem, -1) )
	{
		if( obj->nref-- <= 0 )
			_datobj_del( obj, ctx, -1 );
		BsemGive( ctx->bsem );
	}
}

//
vt_dataobject_node* vt_datobj_match( const vt_match_hint *hint, vt_context *ctx )
{
	dlk_link *lnk;
	vt_dataobject_node *node;
	vt_cstr *cs;
	int n;

	if( hint->tpidx > ctx->act->dpttab.ndpt || (hint->tpidx > 0 && !ctx->act->dpttab.dpts[hint->tpidx-1]) )
		return NULL;

	lnk = hint->tpidx > 0 ? &ctx->act->dpttab.dpts[hint->tpidx-1]->datobjlnk : &ctx->locals.datobjlnk;

	LAB_LOOP:
	n = hint->res;
	node = (vt_dataobject_node*)lnk->end;

	while( node && n >= 0 )
	{
		if( hint->id )
		{
			if( (hint->id->low == -1 || vt_vm_objcmp( hint->lang, (vt_object)node->object.uid.low, (vt_object)hint->id->low ) == 0) &&
				(hint->id->high <= 0 || hint->id->high == node->object.uid.high) )
				break;
		}
		else if( hint->name && (cs=(vt_cstr*)node->object.name) )
		{
			if( strcmp(hint->name, cs->cs) == 0 )
				break;
		}
		node = (vt_dataobject_node*)node->clue_dpt._prior;
		n--;
	}

	if( n < 0 )
		node = NULL;

	if( node == NULL && lnk != &ctx->locals.datobjlnk && hint->ctx_aft_tp )
	{
		lnk = &ctx->locals.datobjlnk;
		goto LAB_LOOP;
	}

#ifdef VTRACE_OBJECT_CACHE
	// TODO: search from cache

	// TODO: re-caching
#endif

	return node;
}

//
inline static vt_long _datobj_search( const vt_match_hint *hints, vt_long num, vt_data_object **objs, vt_context *ctx, vt_long *ntraced )
{
	vt_long i, m, n;
	vt_dataobject_node *node;

	for( i=m=n=0; i < num; i++ )
	{
		if( !hints[i].ignore && (hints[i].id || hints[i].name) )
		{
			if( (node = vt_datobj_match( &hints[i], ctx )) )
			{
				objs[i] = VT_DATNODE_TO_OBJ( node );
				n++;

				m += !!VTRACE_DATOBJ_IS_TRACED( node->object );
			}
			else
				objs[i] = NULL;
		}
		else
			objs[i] = NULL;
	}

	if( ntraced )
		*ntraced += m;

	return n;
}

vt_long vt_datobj_search( const vt_match_hint *hints, vt_long num, vt_data_object **objs, vt_context *ctx, vt_long *ntraced )
{
	vt_long n = 0;
	if( BsemTake(ctx->bsem, -1) )
	{
		n = _datobj_search( hints, num, objs, ctx, ntraced );
		BsemGive( ctx->bsem );
	}
	return n;
}

//
#if VTRACE_OBJECT_TREE_STRUCT == VTRACE_OBJECT_TREE_WITH_DLINK
VT_LOCAL vt_datobj_joint* _datobj_join_dlk( vt_dataobject_node *from, vt_dataobject_node *to,
											vt_sotnode *curr_sot, vt_action *act )
{
	int n;
	vt_datobj_joint *joint;
	dlk_link *dlink;


	if( act->capa.doj_dec >= 0 )
	{
		// search the last one
#ifdef VTRACE_OBJECT_TRACE_SUPPLY
		if( from->dlk_depend.num <= to->dlk_supply.num ) {
#endif
			dlink = &from->dlk_depend;
			n = act->capa.doj_dec + !act->capa.doj_dec;
			joint = (vt_datobj_joint*)dlink->end;
			while( joint && joint->to != to && --n > 0 ) joint = joint->clue_dep._prior;
#ifdef VTRACE_OBJECT_TRACE_SUPPLY
		} else {
			dlink = &to->dlk_supply;
			n = act->capa.doj_dec + !act->capa.doj_dec;
			joint = (vt_datobj_joint*)dlink->end;
			while( joint && joint->from != from && --n > 0 ) joint = joint->clue_sup._prior;
		}
#endif

		if( joint && n > 0 )
		{
			// exists joint

			joint->sot = curr_sot;
			//joint->join_num += joint->join_num < 0x7fffffff;

#ifdef VTRACE_OBJECT_TRACE_SUPPLY
			dlink = &from->dlk_depend;
#endif
			if( (dlk_node*)&joint->clue_dep != dlink->end )
			{
				// drop and then add back; for keeping order
				dlk_Drop( dlink, (dlk_node*)&joint->clue_dep, 0 );
				dlk_Add( dlink, (dlk_node*)&joint->clue_dep, 0 );
			}

#ifdef VTRACE_OBJECT_TRACE_SUPPLY
			dlink = &to->dlk_supply;
			if( (dlk_node*)&joint->clue_sup != dlink->end )
			{
				dlk_Drop( dlink, (dlk_node*)&joint->clue_sup, 0 );
				dlk_Add( dlink, (dlk_node*)&joint->clue_sup, 0 );
			}
#endif
			return joint;
		}
	}

	return NULL;
}


#else
//
VT_LOCAL vt_datobj_joint2* _datobj_join_arr( vt_dataobject_node *from, vt_dataobject_node *to,
											 vt_sotnode *curr_sot, vt_action *act )
{
	int i, n;
	vt_datobj_joint2 **joint;

	if( act->capa.doj_dec >= 0 && from->num_depend > 0 )
	{
		joint = from->depend;

		// search the last one
		i = from->num_depend - 1;
		n = from->num_depend > act->capa.doj_dec ? from->num_depend-act->capa.doj_dec : 0;
		for( ; i>=n && joint[i]->to != to; i-- );

		if( i >= n )
		{
			// exists joint
			joint[i]->sot = curr_sot;
			//joint[i]->join_num += joint[i]->join_num < 0x7fffffff;
			return joint[i];
		}
	}

	return NULL;
}

#endif

//
vt_long vt_datobj_blind_join( vt_data_object **objs_w, vt_long num_w, const vt_method_struct *method_w,
							  vt_data_object **objs_r, vt_long num_r, const vt_method_struct *method_r,
							  vt_sotnode *curr_sot, vt_context *ctx, vt_bool lock )
{
#if VTRACE_OBJECT_TREE_STRUCT == VTRACE_OBJECT_TREE_WITH_DLINK
	vt_datobj_joint *joint, *_joint;
#else
	vt_datobj_joint2 *joint, *_joint;
#endif
	vt_dataobject_node *node[2];
	//vt_namespace *ns = ctx->ns;
	vt_action *act = ctx->act;
	vt_modifier mod;
	vt_long i, j, n;
	//vt_bool intensity_less_strong = ns->runconf.caps.f_intensity < VTRACE_CAPA_INTENSITY_STRONG;

	if( lock && !BsemTake(ctx->bsem, -1) )
		return -1;

	for( n=i=0; i < num_w; i++ )
	{
		if( !objs_w[i] )
			continue;

		node[0] = VT_DATOBJ_TO_NODE( objs_w[i] );

//#ifdef VTRACE_OBJECT_TRACE_SOT
//		if( (node[0]->sottab.nsot & 7) == 0 )
//		{
//			void *ptr = vt_realloc( &ctx->mmphd, node[0]->sottab.sots, sizeof(void**) * (node[0]->sottab.nsot + 8), NULL );
//			if( !ptr )
//				goto LAB_ERROR;
//			memset( ptr+sizeof(void**)*node[0]->sottab.nsot, 0, sizeof(void**) * 8 );
//			node[0]->sottab.sots = (vt_sot**)ptr;
//		}
//		node[0]->sottab.sots[node[0]->sottab.nsot++] = (vt_sot*)curr_sot->__splitor;
//#else
//		// disabled
//		//if( !node[0]->_first_sot )
//		//	node[0]->_first_sot = curr_sot;
//		//node[0]->_last_sot = curr_sot;
//#endif

		mod = method_w ? method_w->arglst.args[i]->mod : objs_w[i]->mod;
		if( !mod.w ||
			(mod.o /*&& intensity_less_strong*/) )
			continue;

//#ifdef VTRACE_OBJECT_JOINT_RESTRICT
//		// restrict dependences wide
//		if( intensity_less_strong && node[0]->depend_wide >= ns->runconf.aux.diffuse_wide_max )
//			continue;
//#endif

		// depend all readable "traced objects" except optional
		for( j=0; j < num_r; j++ )
		{
			if( !objs_r[j] || objs_w[i] == objs_r[j] )
				continue;

			mod = method_r ? method_r->arglst.args[j]->mod : objs_r[j]->mod;

			if( /*VTRACE_OPSET_NOT_TRACED(objs_r[j]->src) ||*/
				!(objs_r[j]->src & (VTRACE_OPSET_EFFECTED | VTRACE_OPSET_SOURCE))    /* MUST "effected" or "source" */
				|| !mod.w    /* MUST join to the outputs of method_r !!! */
				|| (mod.o /*&& intensity_less_strong*/) )
				continue;

			// make "dependences"
			node[1] = VT_DATOBJ_TO_NODE( objs_r[j] );

//#ifdef VTRACE_OBJECT_JOINT_RESTRICT
//			// restrict dependence deep and diffuse wide
//			if( intensity_less_strong &&
//				(node[1]->diffuse_wide >= ns->runconf.aux.diffuse_wide_max /*||
//				 node[1]->depend_deep >= ns->runconf.aux.depend_deep_max*/) )
//				continue;
//#endif

#if VTRACE_OBJECT_TREE_STRUCT == VTRACE_OBJECT_TREE_WITH_DLINK
			_joint = _datobj_join_dlk( node[0], node[1], curr_sot, act );
			if( !_joint )
			{
				// new record
				joint = (vt_datobj_joint*)vt_malloc( &ctx->mmphd, sizeof(vt_datobj_joint), (size_t*)&act->sta.consum.nMem );
				if( !joint )
					goto LAB_ERROR;

				//printf( "jointx: %s, %s, %p(%p) -> %p(%p)\n",
				//      ((vt_cstr*)method_w->name)->cs, ((vt_cstr*)node[0]->object.name)->cs,
				//      (void*)node[0]->object.uid.low, &node[0]->object, (void*)node[1]->object.uid.low, &node[1]->object );

				joint->from = node[0];
				joint->to = node[1];
				joint->sot = curr_sot;
				joint->argidx_from = i + 1;
				joint->argidx_to = -1 - j;
				//joint->join_num = 1;

				dlk_Add( &node[0]->dlk_depend, (dlk_node*)&joint->clue_dep, 0 );

#ifdef VTRACE_OBJECT_TRACE_SUPPLY
				dlk_Add( &node[1]->dlk_supply, (dlk_node*)&joint->clue_sup, 0 );
#elif defined( VTRACE_OBJECT_JOINT_RESTRICT )
				node[1]->diffuse_wide++;
#endif
				act->joint_num++;
			}
//			else
//			{
//				_joint->argidx_from = i + 1;
//				_joint->argidx_to = -1 - j;
//			}
#else
			_joint = _datobj_join_arr( node[0], node[1], curr_sot, act );
			if( !_joint )
			{
				// new record
				if( (node[0]->num_depend & 7) == 0 )
				{
					void *ptr = vt_realloc( &ctx->mmphd, node[0]->depend, sizeof(vt_datobj_joint2**) * (node[0]->num_depend + 8), NULL );
					if( !ptr )
						goto LAB_ERROR;
					memset( ptr+sizeof(vt_datobj_joint2**)*node[0]->num_depend, 0, sizeof(vt_datobj_joint2**) * 8 );
					node[0]->depend = (vt_datobj_joint2**)ptr;
				}
				joint = vt_malloc( &ctx->mmphd, sizeof(vt_datobj_joint2), (size_t*)&act->sta.consum.nMem );
				if( !joint )
					goto LAB_ERROR;
				node[0]->depend[node[0]->num_depend++] = joint;
				joint->from = node[0];
				joint->to = node[1];
				joint->sot = curr_sot;
				joint->argidx_from = i + 1;
				joint->argidx_to = -1 - j;
				//joint->join_num = 1;
#ifdef VTRACE_OBJECT_TRACE_SUPPLY
				if( (node[1]->num_supply & 7) == 0 )
				{
					void *ptr = vt_realloc( &ctx->mmphd, node[1]->supply, sizeof(vt_datobj_joint2**) * (node[1]->num_supply + 8), NULL );
					if( !ptr )
						goto LAB_ERROR;
					memset( ptr+sizeof(vt_datobj_joint2**)*node[1]->num_supply, 0, sizeof(vt_datobj_joint2**) * 8 );
					node[1]->supply = (vt_datobj_joint2**)ptr;
				}
				node[1]->supply[node[1]->num_supply++] = joint;
#elif defined( VTRACE_OBJECT_JOINT_RESTRICT )
				node[1]->diffuse_wide++;
#endif
				act->joint_num++;
			}
//			else
//			{
//				_joint->argidx_from = i + 1;
//				_joint->argidx_to = j + 1;
//			}
#endif

#ifdef VTRACE_OBJECT_JOINT_RESTRICT
			//if( node[0]->depend_deep <= node[1]->depend_deep )
			//	node[0]->depend_deep = node[1]->depend_deep + 1;
#endif
			//if( (objs_r[j]->src & (VTRACE_OPSET_EFFECTED | VTRACE_OPSET_SOURCE)) )
				objs_w[i]->src |= VTRACE_OPSET_EFFECTED;  // set mark "EFFECTED"

			n++;
		}
	}

	if( lock )
		BsemGive( ctx->bsem );
	return n;

	LAB_ERROR:
	if( lock )
		BsemGive( ctx->bsem );
	return -1;
}

//
vt_long vt_datobj_join( vt_data_object **objs, vt_long num, const vt_method_struct *method,
						vt_sotnode *curr_sot, vt_context *ctx, vt_bool lock )
{
#if VTRACE_OBJECT_TREE_STRUCT == VTRACE_OBJECT_TREE_WITH_DLINK
	vt_datobj_joint *joint, *_joint;
#else
	vt_datobj_joint2 *joint, *_joint;
#endif
	vt_dataobject_node *node[2];
	vt_namespace *ns = ctx->ns;
	vt_action *act = ctx->act;
	vt_modifier mod;
	vt_long i, j, n;
	vt_bool is_propagator = method && (method->opset & VTRACE_OPSET_PROPAGATE);
	//vt_bool intensity_less_strong = ns->runconf.caps.f_intensity < VTRACE_CAPA_INTENSITY_STRONG;

	if( lock && !BsemTake(ctx->bsem, -1) )
		return -1;

	for( n=i=0; i < num; i++ )
	{
		if( !objs[i] )
			continue;

		node[0] = VT_DATOBJ_TO_NODE( objs[i] );

#ifdef VTRACE_OBJECT_TRACE_SOT
		if( (node[0]->sottab.nsot & 7) == 0 )
		{
			void *ptr = vt_realloc( &ctx->mmphd, node[0]->sottab.sots, sizeof(void**) * (node[0]->sottab.nsot + 8), NULL );
			if( !ptr )
				goto LAB_ERROR;
			memset( ptr+sizeof(void**)*node[0]->sottab.nsot, 0, sizeof(void**) * 8 );
			node[0]->sottab.sots = (vt_sot**)ptr;
		}
		node[0]->sottab.sots[node[0]->sottab.nsot++] = (vt_sot*)curr_sot->__splitor;
#else
		// disabled
		//if( !node[0]->_first_sot )
		//	node[0]->_first_sot = curr_sot;
		//node[0]->_last_sot = curr_sot;
#endif

		mod = method ? method->arglst.args[i]->mod : objs[i]->mod;
		if( !mod.w ||
			(mod.o /*&& intensity_less_strong*/) )
			continue;

#ifdef VTRACE_OBJECT_JOINT_RESTRICT
		// restrict dependences wide
		if( is_propagator &&
			/*intensity_less_strong &&*/
			node[0]->depend_wide >= ns->runconf.aux.diffuse_wide_max )
			continue;
#endif

		// depend all readable "traced objects" except optional
		for( j=0; j < num; j++ )
		{
			if( i == j || !objs[j] || objs[i] == objs[j] )
				continue;

			mod = method ? method->arglst.args[j]->mod : objs[j]->mod;
			if( VTRACE_OPSET_NOT_TRACED(objs[j]->src)    /* MUST "effected", "source" or "target"*/
				|| !mod.r    /* MUST join to the inputs of method*/
				|| (mod.o /*&& intensity_less_strong*/) )
				continue;

			// make "dependences"
			node[1] = VT_DATOBJ_TO_NODE( objs[j] );

#ifdef VTRACE_OBJECT_JOINT_RESTRICT
			// restrict dependence deep and diffuse wide
			if( is_propagator &&
				/*intensity_less_strong &&*/
				(node[1]->diffuse_wide >= ns->runconf.aux.diffuse_wide_max /*||
				 node[1]->depend_deep >= ns->runconf.aux.depend_deep_max*/) )
				continue;
#endif

#if VTRACE_OBJECT_TREE_STRUCT == VTRACE_OBJECT_TREE_WITH_DLINK
			_joint = _datobj_join_dlk( node[0], node[1], curr_sot, act );
			if( !_joint )
			{
				// new record
				joint = (vt_datobj_joint*)vt_malloc( &ctx->mmphd, sizeof(vt_datobj_joint), (size_t*)&act->sta.consum.nMem );
				if( !joint )
					goto LAB_ERROR;

				//printf( "joint: %s, %s, %p(%p) -> %p(%p)\n",
				//      ((vt_cstr*)method->name)->cs, ((vt_cstr*)node[0]->object.name)->cs,
				//      (void*)node[0]->object.uid.low, &node[0]->object, (void*)node[1]->object.uid.low, &node[1]->object );

				joint->from = node[0];
				joint->to = node[1];
				//joint->sot_beg =
				joint->sot = curr_sot;
				joint->argidx_from = i + 1;
				joint->argidx_to = j + 1;
				//joint->join_num = 1;

				dlk_Add( &node[0]->dlk_depend, (dlk_node*)&joint->clue_dep, 0 );

#ifdef VTRACE_OBJECT_TRACE_SUPPLY
				dlk_Add( &node[1]->dlk_supply, (dlk_node*)&joint->clue_sup, 0 );
#elif defined( VTRACE_OBJECT_JOINT_RESTRICT )
				node[1]->diffuse_wide++;
#endif
				if( method && node[0]->depend_wide >= ns->runconf.aux.diffuse_wide_max )
					curr_sot->dpoint->sta.flags.dep_w_ov = 1;

				if( method && node[1]->diffuse_wide >= ns->runconf.aux.diffuse_wide_max )
					curr_sot->dpoint->sta.flags.dif_w_ov = 1;

				if( ++act->joint_num >= ns->runconf.aux.joint_max )
					act->sta.flags.doj_ov = 1;
			}
			else
			{
				_joint->argidx_from = i + 1;
				_joint->argidx_to = j + 1;
			}
#else
			_joint = _datobj_join_arr( node[0], node[1], curr_sot, act );
			if( !_joint )
			{
				// new record
				if( (node[0]->num_depend & 7) == 0 )
				{
					void *ptr = vt_realloc( &ctx->mmphd, node[0]->depend, sizeof(vt_datobj_joint2**) * (node[0]->num_depend + 8), NULL );
					if( !ptr )
						goto LAB_ERROR;
					memset( ptr+sizeof(vt_datobj_joint2**)*node[0]->num_depend, 0, sizeof(vt_datobj_joint2**) * 8 );
					node[0]->depend = (vt_datobj_joint2**)ptr;
				}
				joint = vt_malloc( &ctx->mmphd, sizeof(vt_datobj_joint2), (size_t*)&act->sta.consum.nMem );
				if( !joint )
					goto LAB_ERROR;
				node[0]->depend[node[0]->num_depend++] = joint;
				joint->from = node[0];
				joint->to = node[1];
				joint->sot = curr_sot;
				joint->argidx_from = i + 1;
				joint->argidx_to = j + 1;
				//joint->join_num = 1;
#ifdef VTRACE_OBJECT_TRACE_SUPPLY
				if( (node[1]->num_supply & 7) == 0 )
				{
					void *ptr = vt_realloc( &ctx->mmphd, node[1]->supply, sizeof(vt_datobj_joint2**) * (node[1]->num_supply + 8), NULL );
					if( !ptr )
						goto LAB_ERROR;
					memset( ptr+sizeof(vt_datobj_joint2**)*node[1]->num_supply, 0, sizeof(vt_datobj_joint2**) * 8 );
					node[1]->supply = (vt_datobj_joint2**)ptr;
				}
				node[1]->supply[node[1]->num_supply++] = joint;
#elif defined( VTRACE_OBJECT_JOINT_RESTRICT )
				node[1]->diffuse_wide++;
#endif
				if( method && node[0]->depend_wide >= ns->runconf.aux.diffuse_wide_max )
					curr_sot->dpoint->sta.flags.dep_w_ov = 1;

				if( method && node[1]->diffuse_wide >= ns->runconf.aux.diffuse_wide_max )
					curr_sot->dpoint->sta.flags.dif_w_ov = 1;

				if( ++act->joint_num >= ns->runconf.aux.joint_max )
					act->sta.flags.doj_ov = 1;
			}
			else
			{
				_joint->argidx_from = i + 1;
				_joint->argidx_to = j + 1;
			}
#endif

#ifdef VTRACE_OBJECT_JOINT_RESTRICT
			//if( node[0]->depend_deep <= node[1]->depend_deep )
			//	node[0]->depend_deep = node[1]->depend_deep + 1;
#endif
			//if( VTRACE_OPSET_IS_TRACED(objs[j]->src) )
			if( (objs[j]->src & (VTRACE_OPSET_EFFECTED | VTRACE_OPSET_SOURCE)) )
				objs[i]->src |= VTRACE_OPSET_EFFECTED;  // set mark "EFFECTED"

			n++;
		}
	}

	if( lock )
		BsemGive( ctx->bsem );
	return n;

	LAB_ERROR:
	if( lock )
		BsemGive( ctx->bsem );
	return -1;
}

//
int vt_datobj_watch( vt_long tpidx, vt_sot *curr_sot, vt_data_sequence *datseq, vt_data_objset *objs_ret, vt_context *ctx, vt_bool lock )
{
	vt_long i, n, m, w, e, ndat;
	//vt_action *act = ctx->act;
	//vt_event *evt = act->event;
	vt_namespace *ns = ctx->ns;
	vt_ident *uids = datseq->uids;
	vt_cstr *names = datseq->names;
	vt_data_class *datcls;
	const vt_method_struct *method = tpidx > 0 ? ns->defines.tracepoints[tpidx-1]->target : NULL;
	vt_lang lang = method ? method->klass->lang : ns->lang;

#ifdef VTRACE_OBJECT_REPLICA
	vt_data_objset _tmpobjset;
#endif

	vt_data_object *_objs_[32], **objs = _objs_;
	vt_match_hint _hints_[32], *hints = _hints_;
	int r, not_traced_tp, is_safe_method;


	if( (ndat=datseq->ndat) == 0 )
		return -100;

	if( ndat < 0 || (lock && !BsemTake(ctx->bsem, -1)) )
		return -1;

	if( method && ndat > method->arglst.argn )
		ndat = method->arglst.argn;  // restrict argument number

	r = -1;

	not_traced_tp = method && VTRACE_OPSET_NOT_TRACED(method->opset);

	is_safe_method = 0;
	if( method )
	{
		if( VTRACE_IS_SAFE_OPERATOR(method->inop) )
			is_safe_method = 1;
		else if( VTRACE_IS_UNSAFE_OPERATOR(method->inop) )
			is_safe_method = -1;
	}


	//
	if( !objs_ret )
	{
		if( ndat > sizeof(_hints_)/sizeof(_hints_[0]) )
		{
			hints = malloc( (sizeof(vt_match_hint)+sizeof(void*)) * ndat );
			if( !hints )
			{
				if( lock )
					BsemGive( ctx->bsem );
				return -1;
			}
			objs = (vt_data_object**)&hints[ndat];
		}
	}
	else
	{
		if( ndat > sizeof(_hints_)/sizeof(_hints_[0]) )
		{
			hints = malloc( (sizeof(vt_match_hint)) * ndat );
			if( !hints )
			{
				if( lock )
					BsemGive( ctx->bsem );
				return -1;
			}
		}

		objs = vt_malloc( &ctx->mmphd, sizeof(void**) * ndat, NULL );
		if( !objs )
		{
			if( hints != _hints_ )
				free( hints );
			if( lock )
				BsemGive( ctx->bsem );
			return -1;
		}
	}

	// try to map data object always
	for( i=n=m=e=0; i < ndat; i++ )
	{
		if( uids[i].high == 0 || uids[i].high == -1 )
		{
			objs[n] = NULL;
			hints[n].tpidx = tpidx;
			hints[n].id = &uids[i];
			hints[n].name = names ? names[i].cs : NULL;
			hints[n].ext = i;  // keep original index
			hints[n].spec_tp = 0;
			hints[n].res = 0; //uids[i].high == -1 ? 16 : 4096;  // for testing
			hints[n].lang = (vt_word)lang;
			e += !!(hints[n].ignore = uids[i].high == -1); // do not search if it is -1
			hints[n++].ctx_aft_tp = 1;
		}
		else
		{
			// test trace state
			w = method ? method->arglst.args[i]->mod.flags : ((vt_data_object*)uids[i].high)->mod.flags | VT_MOD_R;
			m += VTRACE_DATOBJ_TRACE_ALLOW( *((vt_data_object*)uids[i].high), w );  // the modifier may be overlapped by current method
		}
	}

	//if( ns->runconf.caps.f_intensity < VTRACE_CAPA_INTENSITY_STRONG )
	{
		if( n == 0 && m == 0 && not_traced_tp )
		{
			r = -100;
			goto LAB_EXIT;  // nothing traced
		}
	}

	if( n > 0 )
	{
		// do mapping(data object)
		w = n==e ? 0 : _datobj_search( hints, (int)n, objs, ctx, &m );

		if( n-w != i )
		{
			// something mapped
			if( /*ns->runconf.caps.f_intensity < VTRACE_CAPA_INTENSITY_STRONG &&*/
				m == 0 && not_traced_tp )
			{
				r = -100;
				goto LAB_EXIT;  // nothing traced
			}
		}
		else // if( n == i )
		{
			// nothing mapped
			if( not_traced_tp )
			{
				r = -100;
				goto LAB_EXIT;  // nothing traced
			}
		}

		for( i=0; i < n; i++ )
		{
			if( !objs[i] && hints[i].id->low )
			{
				// try to create a data object
				if( method )
				{
					// according to the "tpidx" and the index of data
					datcls = method->arglst.args[hints[i].ext];  //(w = hints[i].ext) < method->arglst.argn ? method->arglst.args[w] : NULL;
				}
				else
				{
					// assume global data objects ???
					// according to the data id(assume it is unique)
					// TODO:
					datcls = NULL;
				}

				if( datcls && !datcls->mod.o && ((datcls->mod.flags&VT_MOD_W) | !not_traced_tp) )
				{
					// create a new data object
					objs[i] = _datobj_new( datcls, hints[i].id, hints[i].name, tpidx, ctx, lang );
					if( objs[i] )
					{
						m += VTRACE_DATOBJ_TRACE_ALLOW( *objs[i], datcls->mod.flags );  // the modifier is overlapped by current method
					}
					else
					{
						// roll back
						for( --i; i >= 0; --i )
							_datobj_del( objs[i], ctx, lang );
						break;
					}
				}
			}

			// j = hints[i].id - uids;
			uids[hints[i].ext].high = (vt_ulong)objs[i];  // write back the pointer of data object
		}
		if( i != n )
			goto LAB_EXIT;
	}


	// fill array "objs", and count "writable"s
	for( i=n=w=0; i < ndat; i++ )
	{
		objs[i] = (vt_data_object*)uids[i].high;
		if( objs[i] )
		{
			w += !!(method ? method->arglst.args[i]->mod.w : objs[i]->mod.w);  // the modifier is overlapped by current method
			n++;

			if( is_safe_method == 1)
				objs[i]->src |= VTRACE_OPSET_ESCAPED; // set mark "ESCAPED"
			else if( is_safe_method == -1)
				objs[i]->src &= ~VTRACE_OPSET_ESCAPED;// cancel the mark "ESCAPED" if possible
		}
	}

	if( w > 0 || !method )
	{
#ifdef VTRACE_OBJECT_REPLICA
		// append replica before data object joining
		_tmpobjset.nobj = i;
		_tmpobjset.objs = objs;
		vt_datobj_replica_append_ex( &_tmpobjset, datseq, curr_sot, lang, ctx, FALSE );
#endif

		// join data objects
		n = i;

#ifdef VTRACE_OBJECT_JOINT_RESTRICT
		int is_propagator = method && (method->opset & VTRACE_OPSET_PROPAGATE);
		i = is_propagator && ctx->act->joint_num >= ns->runconf.aux.joint_max
					? 0 : vt_datobj_join( objs, n, method, VT_SOT2NODE(curr_sot), ctx, FALSE );
#else
		i = vt_datobj_join( objs, n, method, VT_SOT2NODE(curr_sot), ctx, FALSE );
#endif

		if( i >= 0 && objs_ret )
		{
			objs_ret->nobj = n;
			objs_ret->objs = objs;
			r = 0;

			if( method && (method->opset & VTRACE_OPSET_TARGET) &&
				(m = ctx->act->last_stat._tpidx2) > 0 )
			{
				// try to making "blind-joint"
				vt_dpoint *_dpt = ctx->act->dpttab.dpts[m-1]; // assume it is always available
#ifdef VTRACE_SOT_LINK_DPOINT
				vt_sotnode *_sotnd = (vt_sotnode*)_dpt->sotlnk.end;
#else
				vt_sotnode *_sotnd = _dpt->last_sot;
#endif
				if( _sotnd )
				{
					if( (i == 0 & ns->runconf.caps.f_intensity >= VTRACE_CAPA_INTENSITY_STRONG) ||
						VT_ACTION_IS_SUSPENDED(ctx->act->sta) ||
						VT_DPOINT_IS_SUSPENDED(_dpt->sta) )
					{
						vt_datobj_blind_join( objs, n, method,
								_sotnd->entity.invk.args.objset.objs,
								_sotnd->entity.invk.args.objset.nobj,
								_dpt->point->target,
								VT_SOT2NODE(curr_sot), ctx, FALSE );
					}
				}
			}
		}
	}
	else if( method && (method->opset & (VTRACE_OPSET_SOURCE | VTRACE_OPSET_TARGET)) )
	{
		r = 0;
		if( objs_ret )
		{
			objs_ret->nobj = n;
			objs_ret->objs = objs;
		}
	}


	LAB_EXIT:
	if( hints != _hints_ )
		free( hints );

	if( r != 0 && objs_ret )
		vt_free( &ctx->mmphd, objs, 0, NULL );

	if( lock )
		BsemGive( ctx->bsem );

	return r;
}

//
void vt_datobj_references_free( dlk_link *datobjlnk, vt_lang lang, vt_context *ctx, vt_bool lock )
{
	if( lock && !BsemTake(ctx->bsem, -1) )
		return;

	long j __unused__, m, k __unused__;
	vt_long uid;
	vt_dataobject_node *datobj;
	vt_datobj_replica *replica __unused__;

	m = k = 0;
	datobj = (vt_dataobject_node*)datobjlnk->head;
	while( datobj )
	{
		uid = datobj->object.uid.low;
		if( uid != -1 && uid != 0 )
		{
#ifdef VTRACE_OBJECT_REPLICA
#if VTRACE_OBJECT_REPLICA_STRUCT == VTRACE_OBJECT_REPLICA_DLINK
			replica = (vt_datobj_replica*)datobj->object.dlk_replica.end;
			while( replica )
			{
				for( j=replica->nitem-1; j >=0; j-- )
				{
					if( replica->items[j].type1 == 0 &&
						replica->items[j].val.o )  // && replica->items[j].len == sizeof(vt_object)
					{
						vt_vm_replica_ref_del( lang, replica->items[j].val.o, uid );
						replica->items[j].val.o = NULL;
						//break;
					}
				}
				replica = (vt_datobj_replica*)replica->clue._prior;
			}
#else
			for( m=datobj->object.replica_num-1; m >= 0; m-- )
			{
				replica = datobj->object.replica[m];
				for( j=replica->nitem-1; j >=0; j-- )
				{
					if( replica->items[j].type1 == 0 &&
						replica->items[j].val.o )  // && replica->items[j].len == sizeof(vt_object)
					{
						vt_vm_replica_ref_del( lang, replica->items[j].val.o, uid );
						replica->items[j].val.o = NULL;
						//break;
					}
				}
			}
#endif
#endif

			m++;
			if( vt_vm_bind_object( lang, (vt_object)datobj->object.uid.low, 0 ) == 0 )  // cancel the tag
			{
				k++;
				//if( _VT_ACQUIRE_NEWGLOBALREF_(datobj->object) )
					vt_vm_globalref_del( lang, (vt_object)datobj->object.uid.low );  // need to delete the global reference of this data object
			}
			datobj->object.uid.low = 0;
		}
		datobj = (vt_dataobject_node*)datobj->clue_dpt._next;
	}
#if 0	// FOR DEBUG
	printf( "=======================    %ld,  %ld\n", k, m );
#endif
	if( lock )
		BsemGive(ctx->bsem );
}

//
void vt_datobj_references_free_ex( vt_context *ctx, vt_bool lock )
{
	if( lock && !BsemTake(ctx->bsem, -1) )
		return;

	vt_dpoint *dpt = (vt_dpoint*)ctx->act->livdptlnk.head;

	while( dpt )
	{
#if 0	// FOR DEBUG
		printf( "%llu, %zu,  %s%s,  %s\n",
				dpt->sta.nmet, dpt->datobjlnk.num,
				((vt_cstr*)dpt->point->target->name)->cs, (char*)dpt->point->target->decl, ((vt_cstr*)dpt->point->target->klass->name)->cs );
#endif
		vt_datobj_references_free( &dpt->datobjlnk, dpt->point->target->klass->lang, ctx, FALSE );
		dpt = (vt_dpoint*)dpt->clue_act._next;
	}

	//
	vt_datobj_references_free( &ctx->locals.datobjlnk, ctx->ns->lang, ctx, FALSE );

	if( lock )
		BsemGive(ctx->bsem );
}

//
#if VTRACE_OBJECT_TREE_STRUCT == VTRACE_OBJECT_TREE_WITH_DLINK
VT_LOCAL void __unused__ _datobj_graph_track_depend( vt_dataobject_node *origin, void *echo_data, vt_datobj_graph_track_echo echo )
{
	register size_t stk_size = 0, stk_deep = 0;
	register vt_datobj_joint **stk_ptr = NULL;
	register vt_datobj_joint *joint = (vt_datobj_joint*)origin->dlk_depend.head;
	register vt_size _track_seq = origin->object.track_seq ;  // temporary

#if 0	// FOR DEBUG
	printf( "*%llu, %zu\n", _track_seq, origin->dlk_depend.num );
#endif

	while( joint )
	{
		do
		{
			if( stk_deep == stk_size )
			{
				void *temp = realloc( stk_ptr, sizeof(void**) * (stk_size += 64) );
				if( !temp )
				{
					if( stk_ptr )
						free( stk_ptr );
					return;
				}
				stk_ptr = (vt_datobj_joint**)temp;
			}
			stk_ptr[stk_deep++] = joint;  // push one

			joint->to->object.track_seq = _track_seq;

			joint = (vt_datobj_joint*)joint->to->dlk_depend.head;
			while( joint && joint->to->object.track_seq == _track_seq ) joint = joint->clue_dep._next;  // skip joints that have been traversed
		}while( joint );

		do{
			joint = stk_ptr[--stk_deep];  // pop one

			if( echo( joint, echo_data, stk_deep ) != 0 )
				break;

			while( (joint = joint->clue_dep._next) && joint->to->object.track_seq == _track_seq );  // skip joints that have been traversed
		}while( !joint && stk_deep > 0 );
	}

	if( stk_ptr )
		free( stk_ptr );
}
#endif

//
VT_LOCAL int __unused__ _datobj_show_depend( vt_datobj_joint *joint, vt_sot *sot, size_t deep )
{
	vt_dataobject_node *datobj_to = joint->to;
	vt_dpoint *dpt = joint->argidx_to > 0 ? joint->sot->dpoint : sot->dpoint->action->dpttab.dpts[(vt_long)datobj_to->owner - 1];
	int argidx_to = joint->argidx_to > 0 ? joint->argidx_to-1 : -joint->argidx_to-1;
	char fmt[64];

	sprintf( fmt, "%%%dzu|__%s", (int)((deep * 2) % 32 + 2), "%p, %p, %s, %s,  %s\n" );

	printf( fmt, deep,
			(void*)datobj_to->object.uid.low, &datobj_to->object,
			((vt_cstr*)dpt->point->target->arglst.args[argidx_to]->name)->cs,
			((vt_cstr*)dpt->point->target->name)->cs,
			((vt_cstr*)dpt->point->target->klass->name)->cs );
	return 0;
}

//
vt_int vt_datobj_graph_track_depend( vt_data_objset *objset, vt_sot *sot, vt_size *track_seq )
{
	if( objset->nobj > 0 )
	{
		register vt_uint i;
		const vt_method *method = sot->dpoint->point->target;

		printf( "%s%s,  %s:\n", ((vt_cstr*)method->name)->cs, (char*)method->decl, ((vt_cstr*)method->klass->name)->cs );

		for( i=0; i < objset->nobj; i++ )
		{
			if( objset->objs[i] )
			{
				objset->objs[i]->track_seq = ++(*track_seq);
				_datobj_graph_track_depend( VT_DATOBJ_TO_NODE(objset->objs[i]), sot, (vt_datobj_graph_track_echo)_datobj_show_depend );
				printf( "%p, %p, %s;\n", (void*)objset->objs[i]->uid.low, objset->objs[i], ((vt_cstr*)method->arglst.args[i]->name)->cs );  //objset->objs[i]->name)->cs );
			}
		}
	}
	return 0;
}

