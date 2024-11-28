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
#include "vt_api.h"
#include "mmp.h"

VT_LOCAL int g_vt_initiated = 0;
VT_LOCAL vt_namespace	g_vt_ns = {0};

#ifdef _DEBUG
#define VT_REF2EVT(_REF_)	\
	( (_REF_) && VTRACE_MAGIC_MARK_CHECK(_REF_) == ((vt_event*)(_REF_))->clue_uni.magic.mark ? (vt_event*)(_REF_) : NULL )

#define VT_REF2ACT(_REF_)	\
	( (_REF_) && VTRACE_MAGIC_MARK_CHECK(_REF_) == ((vt_action*)(_REF_))->clue_evt.magic.mark ? (vt_action*)(_REF_) : NULL )
#else
#define VT_REF2EVT(_REF_)	(vt_event*)(_REF_)
#define VT_REF2ACT(_REF_)	(vt_action*)(_REF_)
#endif

#define VT_EVT2REF(_EVT_)	( (vt_refer)(_EVT_) )
#define VT_ACT2REF(_ACT_)	( (vt_refer)(_ACT_) )

#define VT_REF2SOT(_REF_)	( (vt_sot*)(_REF_) )
#define VT_SOT2REF(_SOT_)	( (vt_refer)(_SOT_) )


// NOTE: Here all functions DO NOT verify arguments validation


//
int VT_CALLDECL vt_startup( const char *home )
{
	if( __sync_val_compare_and_swap( &g_vt_initiated, 0, 1 ) != 0 )
		return -1;

	if( vt_ns_initiate( home, &g_vt_ns ) != 0 )
		return -1;

	// TODO: others

	_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "startup successfully\n" );
	return 0;
}

//
void VT_CALLDECL vt_shutdown( void )
{
	if( __sync_val_compare_and_swap( &g_vt_initiated, 1, 0 ) == 1 )
	{
		// TODO: others

		vt_ns_destroy( &g_vt_ns );

		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "shutdown\n" );
	}
}

//
const vt_class_struct* vt_class_find( const vt_ident *uid, const char *name, const vt_namespace *ns )
{
	if( !ns )
		ns = &g_vt_ns;

	if( uid && ns->defines.index.klass2 )
	{
		return vt_class_find1( uid, (const vt_class_struct**)ns->defines.index.klass2, ns->defines.index._nklass );
	}
	else if( name )
	{
		if( ns->defines.index.klass )
		{
			const vt_class_struct *k = vt_class_find2( name, (const vt_class_struct**)ns->defines.index.klass, ns->defines.index._nklass );

			if( !k && ns->defines.index._nklass < ns->defines.nklass )
			{
				// find in extended classes
				k = vt_class_find3( name,
									(const vt_class_struct**)&ns->defines.klasses[ns->defines.index._nklass],
									ns->defines.nklass - ns->defines.index._nklass );
			}
			return k;
		}
		else
			return vt_class_find3( name, (const vt_class_struct**)ns->defines.klasses, ns->defines.nklass );
	}
	return NULL;
}

//
const vt_method_struct* vt_method_find( const vt_ident *uid, const char *name, const char *sign, const char *klass, const vt_namespace *ns )
{
	if( !ns )
		ns = &g_vt_ns;

	if( uid && ns->defines.index.method2 )
	{
		return vt_method_find1( uid, (const vt_method_struct**)ns->defines.index.method2, ns->defines.index._nmethod );
	}
	else if( name || sign )
	{
		const vt_class_struct *clsptr = klass && klass[0] ? vt_class_find( NULL, klass, ns ) : NULL;
		if( clsptr )
		{
			return clsptr->mtdidx_num > 0
						? vt_method_find2( name?name:"", sign?sign:"", (const vt_method_struct**)clsptr->mtd_idx, clsptr->mtdidx_num )
						: vt_method_find3( name?name:"", sign?sign:"", (const vt_method_struct**)clsptr->methods, clsptr->method_num );
		}
	}
	return NULL;
}

//
const vt_point* vt_trace_point_find( const vt_ident *uid, const char *name, const char *sign, const char *klass, const vt_namespace *ns )
{
	if( !ns )
		ns = &g_vt_ns;

	if( uid && ns->defines.index.point2 )
	{
		return vt_trace_point_find1( uid, (const vt_point**)ns->defines.index.point2, ns->defines.index._npoint );
	}
	else if( name || sign )
	{
		const vt_class_struct *clsptr = klass && klass[0] ? vt_class_find( NULL, klass, ns ) : NULL;
		const vt_method_struct *mtd;
		if( clsptr )
		{
			mtd = clsptr->mtdidx_num > 0
					? vt_method_find2( name?name:"", sign?sign:"", (const vt_method_struct**)clsptr->mtd_idx, clsptr->mtdidx_num )
					: vt_method_find3( name?name:"", sign?sign:"", (const vt_method_struct**)clsptr->methods, clsptr->method_num );
			if( mtd )
				return (vt_point*)mtd->tpoint;
		}
	}
	return NULL;
}

#if 0
//
int vt_method_handle_set( vt_long tpidx, vt_refer hd, vt_refer actref, int tmo )
{
	if( !g_vt_initiated )
		return -1;

	vt_action *act = VT_REF2ACT( actref );

	if( !act || !act->event ||
		tpidx <= 0 || tpidx > act->dpttab.ndpt )
		return -1;

	vt_context *ctx = act->ctx;
	vt_dpoint *dpt = act->dpttab.dpts[tpidx-1];

	if( BsemTake(ctx->ns->lock, tmo) )
	{
		dpt->point->mtd_hd = hd;
		BsemGive( ctx->ns->lock );
		return 0;
	}
	return -1;
}
#endif

//
vt_refer VT_CALLDECL vt_event_create( vt_thread       init_thread,
									  vt_mode         mode,
									  const vt_capa   *capa,
									  vt_event_entity *entity,
									  vt_refer        *actref_re,
									  vt_refer        extarg
									  )
{
	if( !g_vt_initiated )
		return NULL;

	vt_action *act;
	vt_event *evt = vt_ns_alloc_event( &g_vt_ns );
	vt_context *ctx = vt_alloc_context( evt );  // vt_ns_alloc_context( &g_vt_ns );  // default action

	if( !evt || !ctx )
		return NULL;

	// setup Action
	ctx->extarg = extarg;
	act = ctx->act;
	act->event = evt;
	act->ctx = ctx;
	act->mode = mode;
	//act->thread = init_thread;
	act->capa = !capa || !capa->f_en ? g_vt_ns.runconf.caps : *capa;
	act->dpttab.ndpt = g_vt_ns.defines.npoint;
	act->dpttab.dpts = (vt_dpoint**)vt_malloc( &ctx->mmphd, (sizeof(void*) * act->dpttab.ndpt), (size_t*)&act->sta.consum.nMem );
	if( act->dpttab.dpts )
	{
		// setup Event
		evt->entity.req.objset.nobj = 0;

		if( entity->req.datseq.ndat == 0 ||
			vt_datobj_watch( 0, NULL, &entity->req.datseq, &evt->entity.req.objset, ctx, FALSE ) != -1 )
		{
			memcpy( &evt->entity, entity, _OFFSET(vt_event_entity, req) );

			memset( act->dpttab.dpts, 0, sizeof(void**) * act->dpttab.ndpt );  // clear dpoint table

#ifdef VTRACE_ACTION_HAS_DPOINT
			dlk_Init( &act->livdptlnk );
#endif

#ifdef VTRACE_OBJECT_CACHE
			vt_datobj_cache_init( &act->datobj_cache );
#endif

			// join to event
			dlk_Add( &evt->actlnk, (dlk_node*)act, 0 );

			if( actref_re )
				*actref_re = (vt_refer)VT_ACT2REF( act );

			//evt->tmbeg = _GetTicks();
			evt->nref = 1;
			return VT_EVT2REF(evt);
		}
		else
			vt_free( &ctx->mmphd, act->dpttab.dpts, 0, NULL );
	}

	//vt_ns_free_context( ctx );
	vt_free_context( ctx );
	vt_ns_free_event( evt );
	return NULL;
}

//
void VT_CALLDECL vt_event_destroy( vt_refer evtref )
{
	if( !g_vt_initiated )
		return;

	vt_event *evt = VT_REF2EVT( evtref );
	if( !evt || evt->container != &g_vt_ns )
		return;

	if( __sync_sub_and_fetch( &evt->nref, 1 ) == 0 &&
		BsemTake(evt->bsem, -1) )
	{
		vt_action *act = (vt_action*)evt->actlnk.head;
		vt_context *ctx;

		//
		if( g_vt_ns.runconf.evtres.max > 0 && g_vt_ns.evttab.ite && evt->entity._idx_ < g_vt_ns.evttab.nite )
		{
			vt_ns_evttab_item *_ev = g_vt_ns.evttab.ite + evt->entity._idx_;
			__sync_fetch_and_sub( &_ev->num, 1 );
		}

		//
		while( act )
		{
			ctx = act->ctx;

#if 1	// FOR DEBUG
			printf( "Action Consumption: met=%llu, datobjs=%llu, joints=%llu, cpu=%llu, mem=%zu(%llu), disk=%llu\n",
					act->sta.nmet, act->sta.ndatobj, act->joint_num, act->sta.consum.nTime,
					mmp_count_size(1, &ctx->mmphd),	act->sta.consum.nMem, act->sta.consum.nDisk );
#endif

			vt_datobj_references_free_ex( ctx, TRUE );

#ifdef VTRACE_OBJECT_CACHE
			//vt_datobj_cache_destroy( &act->datobj_cache, &ctx->mmphd );
#endif

			// call back to vt_api_xxx(front end)
			vt_vm_action_exit( g_vt_ns.lang, (vt_refer)act, ctx->extarg );

			act = (vt_action*)act->clue_evt._next;

			// free memory
			//vt_ns_free_context( ctx );
			vt_free_context( ctx );
		}

		vt_ns_free_event( evt );
	}
}

//
vt_refer VT_CALLDECL vt_action_new( vt_thread     new_thread,
									vt_mode       mode,
									const vt_capa *capa,
									vt_refer      evtref,
									vt_refer      extarg
									)
{
	if( !g_vt_initiated )
		return NULL;

	vt_event *evt = VT_REF2EVT( evtref );
	if( !evt || evt->container != &g_vt_ns )
		return NULL;

	vt_action *act;
	vt_context *ctx = vt_alloc_context( evt );  //vt_ns_alloc_context( &g_vt_ns );  // new action
	if( !ctx )
		return NULL;

	// setup Action
	ctx->extarg = extarg;
	act = ctx->act;
	act->event = evt;
	act->ctx = ctx;
	act->mode = mode;
	//act->thread = new_thread;
	act->capa = !capa ? g_vt_ns.runconf.caps : *capa;
	act->dpttab.ndpt = g_vt_ns.defines.npoint;
	act->dpttab.dpts = (vt_dpoint**)vt_malloc( &ctx->mmphd, (sizeof(void*) * act->dpttab.ndpt), (size_t*)&act->sta.consum.nMem );
	if( act->dpttab.dpts )
	{
		memset( act->dpttab.dpts, 0, sizeof(void*) * act->dpttab.ndpt );  // clear dpoint table

#ifdef VTRACE_ACTION_HAS_DPOINT
		dlk_Init( &act->livdptlnk );
#endif

#ifdef VTRACE_OBJECT_CACHE
		vt_datobj_cache_init( &act->datobj_cache );
#endif

		act->sta.nmet = 0;
		ctx->act->nref = 1;

		// join to event
		if( BsemTake(evt->bsem, -1) )
		{
			dlk_Add( &evt->actlnk, (dlk_node*)act, 0 );
			BsemGive( evt->bsem );
			return VT_ACT2REF(evt);
		}
	}

	//vt_ns_free_context( ctx );
	vt_free_context( ctx );
	return NULL;
}

//
void VT_CALLDECL vt_action_del( vt_refer actref, vt_refer evtref )
{
	if( !g_vt_initiated )
		return;

	vt_event *evt = VT_REF2EVT( evtref );
	vt_action *act = VT_REF2ACT( actref );

	if( act && evt && act->event == evt )
	{
		if( __sync_sub_and_fetch(&act->nref, 1) == 0 )
		{
			vt_datobj_references_free_ex( act->ctx, TRUE );

#ifdef VTRACE_OBJECT_CACHE
			//vt_datobj_cache_destroy( &act->datobj_cache, &act->ctx->mmphd );
#endif

			// leave from event
			if( BsemTake(evt->bsem, -1) )
			{
				dlk_Drop( &evt->actlnk, (dlk_node*)act, 0 );
				BsemGive( evt->bsem );
			}

			// call back to vt_api_xxx(front end)
			vt_vm_action_exit( g_vt_ns.lang, (vt_refer)act, act->ctx->extarg );

			// free memory
			//vt_ns_free_context( act->ctx );
			vt_free_context( act->ctx );
		}
	}
}

//
static inline __unused__ vt_brief_report_node* _add_brief_report( vt_brief_report *rep, vt_action *act )
{
	vt_brief_report_node *nd = vt_malloc( &act->ctx->mmphd, sizeof(vt_brief_report_node), (size_t*)&act->sta.consum.nMem );
	if( nd )
	{
		memcpy( &nd->report, rep, sizeof(vt_brief_report) );
		slk_Add( &act->replnk, (slk_node*)nd, 0 );
	}
	return nd;
}

//
static int _vt_sot_filter_a( vt_dpoint *dpt, vt_stage stage, vt_sot_entity *entity, vt_size sotseq )
{
	if( /*!(dpt->point->target->opset & VTRACE_OPSET_SOURCE) &&*/
		(dpt->point->target->opset & VTRACE_OPSET_PROPAGATE) &&
		(stage & (VTRACE_STAGE_ENTER | VTRACE_STAGE_LEAVE)) )
	{
		// suppress SOT
		vt_data_class **argkls = dpt->point->target->arglst.args;
		vt_data_sequence *_datseq = &entity->invk.args.datseq;
		vt_ident *uids = _datseq->uids;
		vt_data_object **objs;
		vt_sotnode *_sot;
		vt_long i, m, z;

#ifndef VTRACE_SOT_LINK_DPOINT
		_sot = (vt_sotnode*)dpt->last_sot;
		if( _sot && (sotseq - _sot->a_seq) <= dpt->action->ctx->ns->runconf.aux.sot_filter_dist )
#else
		int n = dpt->action->ctx->ns->runconf.aux.sot_filter_a;
		int d = dpt->action->ctx->ns->runconf.aux.sot_filter_dist;
		for( _sot=(vt_sotnode*)dpt->sotlnk.end;
			 _sot && (sotseq - _sot->a_seq) <= d && n-- > 0;
			 _sot=(vt_sotnode*)_sot->clue_dpt._prior )
#endif
		{
			if( stage == _sot->stage  /* ??? */
				&& _datseq->ndat == _sot->entity.invk.args.objset.nobj )
			{
				objs = _sot->entity.invk.args.objset.objs;
				i = (vt_long)dpt->point->target->arglst.argn - 1;
				for( m=z=0; i >= 0; i-- )
				{
					if( (argkls[i]->mod.flags & (VT_MOD_R|VT_MOD_W))  )
					{
						z++;
						m += (uids[i].high == (vt_long)objs[i]) || (uids[i].high == -1 & !argkls[i]->mod.w );
					}
				}
				if( m == z )
					return -100;  // ignored
			}
		}
	}
	return 0;
}

//pthread_mutex_t _vt_mutex;
//int _vt_mutex_init = 0;

//
vt_refer VT_CALLDECL vt_sot_append( vt_long  tpidx,
									vt_stage stage,
									vt_cause cause,
									vt_sot_entity *entity,
									vt_refer actref,
									vt_time  tmbeg
									)
{
	// NOTE: MUST call the vt_check_action and the vt_check_dpoint before call this!!!

	//if( !g_vt_initiated )
	//	return NULL;

	vt_action *act = VT_REF2ACT( actref );

	//if( !act || !act->event || tpidx <= 0 )
	//	return NULL;

	//vt_ns_definition *ns_def = &act->ctx->ns->defines;
	//if( tpidx > ns_def->npoint || ns_def->tracepoints[tpidx-1]->mode == 0 )
	//	return NULL;

	vt_context *ctx = act->ctx;
	vt_sotnode *sot = NULL;

	/*if( _vt_mutex_init == 0 )
	{
		pthread_mutexattr_t attr;
		pthread_mutexattr_init( &attr );
		pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_NORMAL );  // PTHREAD_MUTEX_RECURSIVE
		pthread_mutexattr_setrobust( &attr, PTHREAD_MUTEX_STALLED );
		pthread_mutexattr_setprotocol( &attr, PTHREAD_PRIO_INHERIT );  // PTHREAD_PRIO_NONE
		pthread_mutex_init( &_vt_mutex, &attr );
		_vt_mutex_init = 1;
	}
	if(pthread_mutex_lock(&_vt_mutex) == 0 )
	*/

	if( BsemTake(ctx->bsem, 0) )  // -1
	{
		vt_dpoint *dpt = tpidx <= act->dpttab.ndpt ? act->dpttab.dpts[tpidx-1] : NULL;

		if( !dpt )
		{
			if( tpidx > act->dpttab.ndpt )
			{
				// renew trace point table
				vt_long _npt = act->ctx->ns->defines.npoint;
				void **ptr = mmp_realloc( 1, &ctx->mmphd, act->dpttab.dpts, sizeof(void**)*_npt );
				if( !ptr )
					goto LAB_ERROR2;

				memset( &ptr[act->dpttab.ndpt], 0, sizeof(void**)*(_npt - act->dpttab.ndpt) );
				act->dpttab.dpts = (vt_dpoint**)ptr;
				act->dpttab.ndpt = _npt;
			}

			// new dpoint before append SOT
			dpt = (vt_dpoint*)vt_malloc( &ctx->mmphd, sizeof(vt_dpoint), (size_t*)&act->sta.consum.nMem );
			if( !dpt )
				goto LAB_ERROR2;

			memset( dpt, 0, sizeof(vt_dpoint) );
			dpt->action = act;
			dpt->point = ctx->ns->defines.tracepoints[tpidx-1];
			act->dpttab.dpts[tpidx-1] = dpt;

#ifdef VTRACE_ACTION_HAS_DPOINT
			dlk_Add( &act->livdptlnk, (dlk_node*)dpt, 0 );
#endif

#if 1 //def _DEBUG
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "SOT, MET(%06lld), OG(%d), TPIDX(%04lld), %s%s, %s\n",
						  act->sta.nmet,
						  dpt->point->target->opset,
						  tpidx,
						  ((vt_cstr*)dpt->point->target->name)->cs,
						  (char*)dpt->point->target->decl,
						  ((vt_cstr*)dpt->point->target->klass->name)->cs );
#endif
		}

		dpt->sta.tmbeg = tmbeg;

		//act->last_stat._tpidx = tpidx;  do this by vt_check_action

#ifdef VT_STATISTIC_ENABLE
		dpt->sta.nmet++;
		//act->sta.nmet++;  do this by vt_check_action
#endif

		if( ctx->ns->runconf.aux.sot_filter_a > 0 &&
			_vt_sot_filter_a( dpt, stage, entity, act->seqseed ) != 0 )  // current SOT may be suppressed
			goto LAB_ERROR2;

		// TO BE ...
		//if( dpt->point->rule &&
		//	dpt->point->rule->lib->ftab.preproc( dpt->point->rule->num1, dpt, stage, cause, entity ) != 0 )
		//	goto LAB_ERROR2;

		//act->last_stat._tpidx2 = 0;

		//
		//sot = ctx->last_free_sotnode ? ctx->last_free_sotnode : vt_take_node_sot( &ctx->buf_sot, &ctx->mmphd, (size_t*)&act->sta.consum.nMem );
		sot = ctx->last_free_sotnode ? ctx->last_free_sotnode : (vt_sotnode*)vt_malloc( &ctx->mmphd, sizeof(vt_sotnode), (size_t*)&act->sta.consum.nMem );
		if( sot )
		{
			memset( sot, 0, sizeof(vt_sotnode) );

			//sot->magic.mark = VTRACE_MAGIC_MARK_CHECK( sot );
			sot->dpoint = dpt;
			sot->stage = stage;
			sot->cause = cause;
			//sot->caller = NULL;
			sot->a_seq = act->seqseed + 1;

			if( stage & (VTRACE_STAGE_CATCH | VTRACE_STAGE_CRASH) )
			{
				// copy exception
				 if( vt_data_exception_clone( &sot->entity.expt, &entity->expt, ctx ) == -1 )
					 goto LAB_ERROR;

				if( vt_datobj_watch( tpidx, VT_NODE2SOT(sot), &entity->watch.args.datseq, &sot->entity.watch.args.objset, ctx, FALSE ) != 0 )
					goto LAB_ERROR;
			}
			else
			{
				// invoke arguments and watches
				if( vt_datobj_watch( tpidx, VT_NODE2SOT(sot), &entity->invk.args.datseq, &sot->entity.invk.args.objset, ctx, FALSE ) != 0 )
					goto LAB_ERROR;

				// TODO: how to process the "watched variables" ???
			}


#if 0
			if( (dpt->point->target->opset & (VTRACE_OPSET_TARGET /*|VTRACE_OPSET_SOURCE*/)) )
			{
				// copy callstack
				if( entity->cstk.num > 0 )
				{
					if( vt_data_callstack_clone( &sot->entity.cstk, &entity->cstk, ctx ) == -1 )
						goto LAB_ERROR;
				}
				else if( dpt->point->kind & VTRACE_KIND_KTP )
				{
					sot->entity.cstk.frame = NULL;
					if( act->capa.f_intensity >= VTRACE_CAPA_INTENSITY_NORMAL )
						sot->entity.cstk.num = vt_vm_callstack_copy( dpt->point->target->klass->lang, NULL,
																	 0, -1, &sot->entity.cstk.frame, TRUE, &ctx->mmphd );
				}

				// copy journal
				if( entity->log.msglen > 0 &&
					vt_data_journal_clone( &sot->entity.log, &entity->log, ctx ) == -1 )
					goto LAB_ERROR;
			}
#endif

			//
			sot->a_seq = act->seqseed++; //__sync_fetch_and_add( &act->seqseed, 1 );
			//sot->e_seq = __sync_fetch_and_add( &act->event->seqseed, 1 );

			// as "confirmed"
			act->last_stat._tpidx2 = tpidx;

			// join
#ifdef VTRACE_SOT_LINK_DPOINT
			//dpt->delta_seq = dpt->sotlnk.end ? sot->a_seq - ((vt_sotnode*)dpt->sotlnk.end)->a_seq : 0;

			dlk_Add( &dpt->sotlnk, (dlk_node*)&sot->clue_dpt, 0 );
#else
			//dpt->delta_seq = dpt->last_sot ? sot->a_seq - dpt->last_sot->a_seq : 0;
			dpt->last_sot = sot;
			dpt->sotnum++;
#endif

#ifdef VTRACE_SOT_LINK_ACTION
			dlk_Add( &act->sotlnk, (dlk_node*)&sot->clue_act, 0 );
			if( act->sotlnk.num >= ctx->ns->runconf.caps.sot_max )
#else
			//act->last_sot = sot;
			act->sotnum++;  //__sync_fetch_and_add( &act->sotnum, 1 );
			if( act->sotnum >= ctx->ns->runconf.caps.sot_max )
#endif
			{
				act->sta.flags.sot_ov = 1;
				act->sta.flags.suspend = 1; // suspend the action
			}

			if( (dpt->point->target->opset & VTRACE_OPSET_DEADZONE)
#ifdef VTRACE_SOT_LINK_DPOINT
				&& dpt->sotlnk.num >= dpt->point->sot_max_tp
#else
				&& dpt->sotnum >= dpt->point->sot_max_tp
#endif
				)
			{
				dpt->sta.flags.sot_ov = 1;
				dpt->sta.flags.suspend = 1; // suspend the trace point
			}

			// do post processing(analyzing) if possible
			if( dpt->point->rule &&
				(dpt->point->kind & VTRACE_KIND_CRIT_KTP) == VTRACE_KIND_CRIT_KTP )
			{
				if( dpt->point->rule->lib->ftab.postproc( dpt->point->rule->num1, dpt, sot, &act->report ) == 0 &&
					act->report.confidence >= ctx->ns->runconf.vulner.thres_conf )
				{
					// TODO: maybe, hit a vulnerability
					_add_brief_report( &act->report, act );
				}
			}
#if 1 // FOR DEBUG
			else if( /*dpt->point->rule &&*/ strcmp("exec", ((vt_cstr*)dpt->point->target->name)->cs) == 0 )
			//if( cause == VTRACE_CAUSE_TRACE_DONE )
			{
				dpt->point->rule->lib->ftab.postproc( dpt->point->rule->num1, dpt, sot, &act->report );
			}
#endif
		}

		ctx->last_free_sotnode = NULL;
#ifndef VT_STATISTIC_TIME_DISABLE
		act->sta.consum.nTime += _GetTicks() - tmbeg;
#endif
		BsemGive( ctx->bsem );
		//pthread_mutex_unlock( &_vt_mutex );
	}

	return VT_SOT2REF( sot );

	LAB_ERROR:
	//if( sot )
		//vt_free( &ctx->mmphd, sot, sizeof(vt_sotnode), (size_t*)&act->sta.consum.nMem );
	ctx->last_free_sotnode = sot;

	LAB_ERROR2:
#ifndef VT_STATISTIC_TIME_DISABLE
	act->sta.consum.nTime += _GetTicks() - tmbeg;
#endif
	BsemGive( ctx->bsem );
	//pthread_mutex_unlock( &_vt_mutex );
	return NULL;
}


//
VT_ERROR VT_CALLDECL vt_calc_confidence( vt_refer  actref,
										 vt_refer  evtref,
										 vt_data_sequence *datseq,
										 vt_brief_report *report	/* only feed data if it is NULL */
										 )
{
	if( !g_vt_initiated )
		return errNotReady;
	return -1;
}

//
VT_ERROR VT_CALLDECL vt_report_get( vt_refer actref,
									vt_refer evtref,
									vt_brief_report *report
									)
{
	vt_event *evt = VT_REF2EVT( evtref );
	vt_action *act = VT_REF2ACT( actref );
	VT_ERROR er = SABI_errFail;

	if( !evt || (act && act->event != evt) )
		return SABI_errFail;

	if( act )
	{
		if( BsemTake( act->ctx->bsem, -1 ) )
		{
			memcpy( report, &act->report, sizeof(vt_brief_report) );
			BsemGive( act->ctx->bsem );
			er = SABI_errOk;
		};
	}
	else
	{
		/*if( BsemTake( evt->bsem, -1 ) )
		{
			memcpy( report, &evt->report, sizeof(vt_brief_report) );
			BsemGive( evt->bsem );
			er = errOk;
		}*/
		er = SABI_errUnsupport;
	}

	return er;
}


//
VT_ERROR vt_ask_feed( vt_lang lang, vt_ident *uids, vt_long uidnum, vt_refer actref, vt_refer evtref )
{
	return errNotReady;
}


//
int VT_CALLDECL vt_check_event( vt_uint type, vt_uint *index )
{
	if( !g_vt_initiated || g_vt_ns.runconf.evtres.max == 0 )
		return -1;

	if( !g_vt_ns.evttab.ite
		/*|| (g_vt_ns.runconf.evtres.intv <= 0 && g_vt_ns.runconf.evtres.max <= 0)*/ )
		return 0;

	vt_ns_evttab_item *ite;
	time_t t = 0;

	ite = vt_evttab_match( g_vt_ns.evttab.ite, g_vt_ns.evttab.nite, type );
	if( ite )
	{
		//if( g_vt_ns.runconf.evtres.intv > 0 || g_vt_ns.runconf.evtres.max > 0 )
		{
			if( __sync_val_compare_and_swap(&ite->lock, 0, 1) == 0 )
			{
				// check interval
				t = _GetTicks();
				if( t - ite->tmlast >= g_vt_ns.runconf.evtres.intv )
				{
					// check number
					if( g_vt_ns.runconf.evtres.max <= 0 || ite->num < g_vt_ns.runconf.evtres.max )
					{
						// good
						ite->tmlast = t;
						t = 1;

						if( g_vt_ns.runconf.evtres.max > 0 )
							__sync_fetch_and_add( &ite->num, 1 );
					}
					else
						t = -2;  // out of capacity
				}
				else
					t = -3;  // do not reach at interval

				//__sync_val_compare_and_swap( &ite->lock, 1, 0 );
				__sync_fetch_and_and( &ite->lock, 0 );
			}
			else
				t = -4;
		}
		//else
		//	t = 1;  // no restrict

		*index = (vt_uint)(ite - g_vt_ns.evttab.ite);
	}// else 0

	return t;
}


//
int VT_CALLDECL vt_check_action( vt_long tpidx, vt_refer actref, vt_bool meet_plus )
{
	// CAUTION: DO NOT use the "tpidx"

	//if( !g_vt_initiated )
	//	return -1;

	vt_action *act = VT_REF2ACT( actref );
	if( !act || !act->event )
		return -1;


#ifdef VT_STATISTIC_ENABLE
	if( meet_plus )
		__sync_fetch_and_add( &act->sta.nmet, 1 );
#endif

	if( VT_STAT_IS_SUSPENDED(act->sta) )
		return -3;

	// capacities check

// replaced by the VT_STAT_IS_SUSPENDED
//#ifdef VTRACE_SOT_LINK_ACTION
//	if( act->sotlnk.num >= act->capa.sot_max )
//#else
//	if( act->sotnum >= act->capa.sot_max )
//#endif
//		return -4;

	if( act->sta.consum.nMem >= act->capa.mem_max )
	{
		act->sta.flags.suspend = 1;
		return -5;
	}

	if( act->sta.consum.nTime >= act->capa.live_max )
	{
		act->sta.flags.suspend = 1;
		return -6;
	}

//	if( act->sta.consum.nDisk >= act->capa.dsk_max )
//	{
//		act->sta.flags.suspend = 1;
//		return -7;
//	}

	return 0;
}

//
int VT_CALLDECL vt_check_point( vt_long  tpidx, vt_refer actref, vt_operator_set *opset, vt_uint *argn )
{
	// NOTE: MUST call the vt_check_action before call this!!!

	//if( !g_vt_initiated )
	//	return -1;

	//int i32;
	vt_point *tp;
	vt_dpoint *dpt;
	vt_action *act = VT_REF2ACT( actref );

	if( !act || !act->event ||
		tpidx <= 0 || tpidx > act->ctx->ns->defines.npoint )  //act->dpttab.ndpt
		return -1;

	tp = act->ctx->ns->defines.tracepoints[tpidx-1];
	if( tp->mode == 0 || !tp->cond.tpix_sync )
		return -1;

	//_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_MT, "Check Point: %ld, %ld, %s%s, %s\n",
	//		tpidx, tp->idx, ((vt_cstr*)tp->target->name)->cs, (char*)tp->target->decl, ((vt_cstr*)tp->target->klass->name)->cs );

	//if( tp->idx != tpidx )  // FOR DEBUG
	//	return -10;

	dpt = tpidx <= act->dpttab.ndpt ? act->dpttab.dpts[tpidx-1] : NULL;  // may be empty

	act->last_stat._tpidx = tpidx;

	if( tp->mode == 0 )
		return -2;

	if( tp->target->opset == VTRACE_OPSET_DISABLED ||
		(dpt && VT_STAT_IS_SUSPENDED(dpt->sta)) )
		return -3;

	if( opset )
		*opset = tp->target->opset;

	// capacities check

#if 0  // do these in the vt_check_action

#ifdef VTRACE_SOT_LINK_ACTION
	if( act->sotlnk.num >= act->capa.sot_max )
#else
	if( act->sotnum >= act->capa.sot_max )
#endif
		return -4;

	if( act->sta.consum.nMem >= act->capa.mem_max )
	{
		act->sta.flags.suspend = 1;
		return -5;
	}

	if( act->sta.consum.nTime >= act->capa.live_max )
	{
		act->sta.flags.suspend = 1;
		return -6;
	}

//	if( act->sta.consum.nDisk >= act->capa.dsk_max )
//	{
//		act->sta.flags.suspend = 1;
//		return -7;
//	}
#endif

	if( dpt && (dpt->point->target->opset & VTRACE_OPSET_PROPAGATE) )
	{
		if( act->joint_num >= act->ctx->ns->runconf.aux.joint_max )
			return -8;
		//else if( (i32=act->ctx->ns->runconf.caps.freq) > 1 && (act->sta.nmet % i32) != 0 )   bad algorithm!!!
		//	return 0;
	}

	if( argn )
		*argn = tp->target->arglst.argn;

#ifdef VTRACE_SOT_LINK_DPOINT
	return (dpt == NULL || dpt->sotlnk.num < dpt->point->sot_max_tp);
#else
	return (dpt == NULL || dpt->sotnum < dpt->point->sot_max_tp);
#endif
}

//
const vt_dpoint* VT_CALLDECL vt_get_tracepoint( vt_long tpidx, vt_refer actref )
{
	vt_action *act;
	if( g_vt_initiated )
	{
		act = VT_REF2ACT( actref );
		if( act && act->event && tpidx <= 0 && tpidx > act->dpttab.ndpt )
			return act->dpttab.dpts[tpidx-1];
	}
	return NULL;
}

//
int VT_CALLDECL vt_tpix_sync( vt_long tpidx, const char *name, const char *sign, const char *klass, const char *interface, vt_namespace *ns )
{
	int r;

	if( !ns )
		ns = &g_vt_ns;

	r = -1;

	if( BsemTake(ns->lock, -1) )
	{
		const vt_method_struct *mtd = NULL;
		const vt_class_struct *ifnptr = NULL;
		const vt_class_struct *clsptr = vt_class_find( NULL, klass, ns );
		if( !clsptr && interface )
		{
			// try to spawn a class comply with the interface name
			ifnptr = vt_class_find( NULL, interface, ns );
			if( ifnptr )
				clsptr = vt_class_dup( ns, ifnptr, klass, FALSE );
		}

		if( clsptr )
		{
			mtd = clsptr->mtdidx_num > 0
						? vt_method_find2( name, sign, (const vt_method_struct**)clsptr->mtd_idx, clsptr->mtdidx_num )
						: vt_method_find3( name, sign, (const vt_method_struct**)clsptr->methods, clsptr->method_num );

			if( !mtd && interface && !ifnptr )
			{
				ifnptr = vt_class_find( NULL, interface, ns );
				if( ifnptr )
				{
					// try to append methods which are declared in the interface
					if( vt_class_copy( ns, (vt_class_struct*)clsptr, ifnptr, FALSE ) )
					{
						mtd = clsptr->mtdidx_num > 0
									? vt_method_find2( name, sign, (const vt_method_struct**)clsptr->mtd_idx, clsptr->mtdidx_num )
									: vt_method_find3( name, sign, (const vt_method_struct**)clsptr->methods, clsptr->method_num );
					}
				}
			}

			if( mtd )
			{
				vt_long _ix = ((vt_point*)mtd->tpoint)->idx;
				r = _ix > 0 && _ix != tpidx ? vt_tpidx_set( tpidx, _ix, ns, FALSE ) : 0;
			}
		}

		if( r != 0 )
			_VT_DEBUG_PRINT_( stderr, SABI_JOURNAL_ERROR,
					"failed to synchronize trace point index%s, tpidx=%lld, method=%s.%s%s, interface=\"%s\"\n",
					!mtd?"(due to missing definition)":"", tpidx, klass, name, sign, interface?interface:"" );

		BsemGive( ns->lock );
	}

	return r;
}

//
vt_long VT_CALLDECL vt_tpix_query( const char *name, const char *sign, const char *klass, vt_namespace *ns )
{
	vt_long ix = -1;

	if( !ns )
		ns = &g_vt_ns;

	if( BsemTake(ns->lock, -1) )
	{
		const vt_method_struct *mtd = NULL;
		const vt_class_struct *clsptr = vt_class_find( NULL, klass, ns );
		if( clsptr )
		{
			mtd = clsptr->mtdidx_num > 0
						? vt_method_find2( name, sign, (const vt_method_struct**)clsptr->mtd_idx, clsptr->mtdidx_num )
						: vt_method_find3( name, sign, (const vt_method_struct**)clsptr->methods, clsptr->method_num );

			if( mtd && ((vt_point*)mtd->tpoint)->cond.tpix_sync )
				ix = ((vt_point*)mtd->tpoint)->idx;
		}

		BsemGive( ns->lock );
	}

	return ix;
}

//
int VT_CALLDECL vt_action_lock( vt_refer actref, int tmo )
{
	if( g_vt_initiated )
	{
		vt_action *act = VT_REF2ACT( actref );
		if( act )
			return BsemTake( act->ctx->bsem, tmo ) - 1;
	}
	return -1;
}

//
void VT_CALLDECL vt_action_unlock( vt_refer actref )
{
	if( g_vt_initiated )
	{
		vt_action *act = VT_REF2ACT( actref );
		if( act )
			BsemGive( act->ctx->bsem );
	}
}

//
int VT_CALLDECL vt_extarg_set( vt_refer actref, vt_refer arg )
{
	if( g_vt_initiated )
	{
		vt_action *act = VT_REF2ACT( actref );
		if( act && BsemTake( act->ctx->bsem, -1 ) )
		{
			act->ctx->extarg = arg;
			BsemGive( act->ctx->bsem );
			return 0;
		}
	}
	return -1;
}

//
vt_refer VT_CALLDECL vt_extarg_get( vt_refer actref )
{
	vt_refer arg = NULL;

	if( g_vt_initiated )
	{
		vt_action *act = VT_REF2ACT( actref );
		if( act )
		{
			arg = act->ctx->extarg;
		}
	}
	return arg;
}
