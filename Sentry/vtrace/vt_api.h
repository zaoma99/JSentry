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

#ifndef _AS_VTAPI_H_
#define _AS_VTAPI_H_

#include "vt_def.h"
#include "vtrace.h"
#include "vtrace_impl.h"


static inline time_t _GetTicks( void )
{
	struct timespec tms;

	if( clock_gettime( CLOCK_MONOTONIC_COARSE, &tms ) != 0 )
		return -1;

	return  ((time_t)tms.tv_sec * _TICKS_PER_SEC + tms.tv_nsec / _SYSCLOCKRES);
}


//
static inline int _CheckAction( vt_long tpidx, vt_refer actref, vt_bool meet_plus )
{
	vt_action *act = (vt_action*)actref;
	if( !act || !act->event )
		return -1;

#ifdef VT_STATISTIC_ENABLE
	if( meet_plus )
		act->sta.nmet++;  //__sync_fetch_and_add( &act->sta.nmet, 1 );
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
static inline int _CheckTPoint_x( vt_long  tpidx, vt_refer actref, vt_point **tpoint_r )
{
	// NOTE: MUST call the _CheckAction before call this!!!

	//int i32;
	vt_point *tp;
	vt_dpoint *dpt;
	vt_action *act = (vt_action*)actref;

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

	if( tpoint_r )
		*tpoint_r = tp;

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


#ifdef VTRACE_SOT_LINK_DPOINT
	return (dpt == NULL || dpt->sotlnk.num < dpt->point->sot_max_tp);
#else
	return (dpt == NULL || dpt->sotnum < dpt->point->sot_max_tp);
#endif
}

static inline int _CheckTPoint( vt_long  tpidx, vt_refer actref, vt_operator_set *opset, vt_uint *argn )
{
	vt_point *tp = NULL;
	int r = _CheckTPoint_x(tpidx, actref, &tp);
	if( tp )
	{
		if( opset )
			*opset = tp->target->opset;
		if( argn )
			*argn = tp->target->arglst.argn;
	}
	return r;

#if 0
	// NOTE: MUST call the _CheckAction before call this!!!

	//int i32;
	vt_point *tp;
	vt_dpoint *dpt;
	vt_action *act = (vt_action*)actref;

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
#endif
}


#ifdef __cplusplus
extern "C"{
#endif

VT_EXPORT VT_CALLDECL int vt_startup( const char *home );

VT_EXPORT VT_CALLDECL void vt_shutdown( void );


// native declaration

VT_EXPORT
vt_refer VT_CALLDECL vt_event_create( vt_thread       init_thread,
									  vt_mode         mode,
									  const vt_capa   *capa,
									  vt_event_entity *entity,
									  vt_refer        *actref_re,
									  vt_refer        extarg
									  );

VT_EXPORT
void VT_CALLDECL vt_event_destroy( vt_refer evtref );

//VT_EXPORT
//vt_refer VT_CALLDECL vt_event_get( void );


VT_EXPORT
vt_refer VT_CALLDECL vt_action_new( vt_thread     new_thread,
									vt_mode       mode,
									const vt_capa *capa,
									vt_refer      evtref,
									vt_refer      extarg
									);

VT_EXPORT
void VT_CALLDECL vt_action_del( vt_refer actref, vt_refer evtref );

//VT_EXPORT
//vt_refer VT_CALLDECL vt_action_get( vt_thread thread, vt_refer evtref );


VT_EXPORT
vt_refer VT_CALLDECL vt_sot_append( vt_long  tpidx,
									vt_stage stage,
									vt_cause cause,
									vt_sot_entity *entity,
									vt_refer actref,
									vt_time  tmbeg
									);

VT_EXPORT
void VT_CALLDECL vt_sot_remove( vt_refer sotref, vt_refer actref );  // obsoleted


VT_EXPORT
VT_ERROR VT_CALLDECL vt_dfg_make( vt_refer actref, vt_refer evtref, vt_bool renew );

VT_EXPORT
VT_ERROR VT_CALLDECL vt_pfg_make( vt_refer actref, vt_refer evtref, vt_bool renew );

VT_EXPORT
VT_ERROR VT_CALLDECL vt_fill_response( vt_refer evtref, vt_data_response *resp );  // obsoleted

VT_EXPORT
VT_ERROR VT_CALLDECL vt_calc_confidence( vt_refer  actref,
										 vt_refer  evtref,
										 vt_data_sequence *datseq,
										 vt_brief_report *report	/* only feed data if it is NULL */
										 );

#define vt_feed_data( _ACTREF_, _EVTREF_, _DATSEQ_ )	vt_calc_confidence( _ACTREF_, _EVTREF_, _DATREF_, NULL )

VT_EXPORT
VT_ERROR VT_CALLDECL vt_report_get( vt_refer actref,
		 	 	 	 	 	 	 	vt_refer evtref,
									vt_brief_report *report
									);

VT_EXPORT
vt_ulong VT_CALLDECL vt_tp_idx_copy( vt_ulong **idxtabptr, vt_ulong tablen );  // obsoleted

VT_EXPORT
int VT_CALLDECL vt_tpix_sync( vt_long tpidx, const char *name, const char *sign, const char *klass, const char *interface, vt_namespace *ns );

VT_EXPORT
vt_long VT_CALLDECL vt_tpix_query( const char *name, const char *sign, const char *klass, vt_namespace *ns );

VT_EXPORT
int VT_CALLDECL vt_check_event( vt_uint evttype, vt_uint *index );

VT_EXPORT
int VT_CALLDECL vt_check_action( vt_long tpidx, vt_refer actref, vt_bool meet_plus );

VT_EXPORT
int VT_CALLDECL vt_check_point( vt_long  tpidx, vt_refer actref, vt_operator_set *opset, vt_uint *argn );

VT_EXPORT const vt_dpoint* VT_CALLDECL vt_get_tracepoint( vt_long tpidx, vt_refer actref );

VT_EXPORT int VT_CALLDECL vt_action_lock( vt_refer actref, int tmo );

VT_EXPORT void VT_CALLDECL vt_action_unlock( vt_refer actref );

VT_EXPORT int VT_CALLDECL vt_extarg_set( vt_refer actref, vt_refer arg );

VT_EXPORT vt_refer VT_CALLDECL vt_extarg_get( vt_refer actref );



///////////////////////////////////////////////////////////////////////////////////////////////////////////
VT_EXPORT
void vt_print_calc_confidence( FILE *fd,
							   vt_long actref,
							   vt_long evtref,
							   vt_data_sequence *datseq );

VT_EXPORT
void vt_print_sot( FILE *fd,
		   	   	   vt_long  actref,
				   vt_ulong tpidx,
				   vt_stage stage,
				   vt_cause cause,
				   vt_sot_entity *entity );

VT_EXPORT
void vt_print_sot_entity( FILE *fd, const char *level, const char *suffix,
						  vt_sot_entity *entity, vt_bool isException );

VT_EXPORT
void vt_print_exception( FILE *fd, const char *level, const char *suffix, vt_data_exception *exp );

VT_EXPORT
void vt_print_callstack( FILE *fd, const char *level, const char *suffix, vt_data_callstack *cstk );

VT_EXPORT
void vt_print_event( FILE *fd, vt_thread init_thread, int mode,
					 const vt_capa *caps, const vt_event_entity *entity );

VT_EXPORT
void vt_print_event_entity( FILE *fd, const char *level, const char *suffix, const vt_event_entity *entity );

VT_EXPORT
void vt_print_data_sequence( FILE *fd, const char *name,
							 const char *level, const char *suffix,
							 const vt_data_sequence *datseq );

VT_EXPORT
void vt_print_tlv( FILE *fd, const char *level, const char *suffix,
				   const vt_tlv *tlv, vt_long ntlv );

VT_EXPORT
void vt_print_capacity( FILE *fd, const char *level, const char *suffix, const vt_capa *caps );

VT_EXPORT
void vt_print_primitive( FILE *fd, const char *fmt, const char *suffix,
						 int type, vt_size len, vt_value *val );


#ifdef __cplusplus
}
#endif

#endif // _AS_VTAPI_H_
