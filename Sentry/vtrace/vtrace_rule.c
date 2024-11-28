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
#include "vtrace_impl.h"
#include "vtrace_rule.h"
#include "vtrace_data.h"
#include "vt_api.h"
#include "mmp.h"
#include "dlfcn.h"


#ifdef _LINUX
#define LoadLibrary(__PATH__)	dlopen( __PATH__, RTLD_NOW | RTLD_GLOBAL )
#define FreeLibrary				dlclose
#define GetProcAddress			dlsym
#endif

//
vt_int vt_rule_initiate( int argn, void *args )
{
	return 0;
}

//
void vt_rule_release( void )
{
}

//
vt_int vt_rule_preproc( vt_uint rule_num, vt_dpoint *dpt, vt_stage stage, vt_cause cause, vt_sot_entity *entity )
{
	return 0;
}

//
vt_int vt_rule_postproc( vt_uint rule_num, vt_dpoint *dpt, vt_sotnode *sot, struct _vtrace_brief_report *rep )
{
	// FOR DEBUG
	//if( strcmp(((vt_cstr*)dpt->point->target->name)->cs, "exec") == 0 )
	//if( (dpt->point->kind & VTRACE_KIND_AUTO_KTP) == VTRACE_KIND_AUTO_KTP )

	{
		vt_context *ctx = dpt->action->ctx;

		vt_datobj_graph_track_depend( &sot->entity.invk.args.objset, sot->__splitor, &dpt->action->track_seq );

		/*if( sot->entity.cstk.num == 0 )
		{
			sot->entity.cstk.num = vt_vm_callstack_copy( dpt->point->target->klass->lang, NULL,
														 0, -1, &sot->entity.cstk.frame, FALSE, &ctx->mmphd );
		}*/
	}

	return -1;
}



//===============================================================================================================================
//
inline static int vt_rule_funcs_mapping( vt_rule_lib *rulelib )
{
	// FUNC-Initiate
	rulelib->ftab.initiate = (VT_RuleFunc_Initiate)GetProcAddress( rulelib->hlib, VT_RULE_FUNNAME_INITIATE );

	// FUNC-Release
	rulelib->ftab.release = (VT_RuleFunc_Release)GetProcAddress( rulelib->hlib, VT_RULE_FUNNAME_RELEASE );

	// FUNC-PreProcess
	rulelib->ftab.preproc = (VT_RuleFunc_PreProcess)GetProcAddress( rulelib->hlib, VT_RULE_FUNNAME_PREPROCESS );

	// FUNC-PostProcess
	rulelib->ftab.postproc = (VT_RuleFunc_PostProcess)GetProcAddress( rulelib->hlib, VT_RULE_FUNNAME_POSTPROCESS );

	return ( rulelib->ftab.initiate && rulelib->ftab.release && rulelib->ftab.preproc && rulelib->ftab.postproc ) - 1;
}

//
int vt_rulelib_load( vt_rule_lib *rulelib )
{
#ifdef ___WINDOWS___
	wchar_t *fn_w;
#endif
	int    r;


	if( !rulelib->path || rulelib->path->len == 0 )
	{
		// internal
#ifdef ___WINDOWS___
		rulelib->hlib = GetModuleHandle( NULL );
#else
		rulelib->hlib = dlopen( NULL, RTLD_NOW | RTLD_GLOBAL );
#endif
		rulelib->ftab.initiate = vt_rule_initiate;
		rulelib->ftab.release = vt_rule_release;
		rulelib->ftab.preproc = vt_rule_preproc;
		rulelib->ftab.postproc = vt_rule_postproc;
	}
	else
	{
		// external

#ifdef ___WINDOWS___
		// char to wchar_t
		fn_w = malloc( sizeof(wchar_t) * (rulelib->path->len + 1) );
		if( !fn_w )
			return -1;
		char_to_wchar( rulelib->path->cs, rulelib->path->len, fn_w, rulelib->path->len );
		fn_w[rulelib->path->len] = 0;

		//
		rulelib->hlib = LoadLibrary( fn_w );

		free( fn_w );
#else
		rulelib->hlib = LoadLibrary( rulelib->path->cs );
#endif

		if( rulelib->hlib )
		{
			// mapping functions
			if( vt_rule_funcs_mapping(rulelib) == 0 )
			{
				// succeed
				_VT_DEBUG_PRINT_( stdout, LOG_TYPE_EVENT, "succeed to load the rule library \"%s\"\n", rulelib->path->cs );
			}
			else
			{
				// illegal library
				FreeLibrary( rulelib->hlib );
				rulelib->hlib = NULL;

				_VT_DEBUG_PRINT_( stdout, LOG_TYPE_ERROR, "failed to map functions of the rule library \"%s\"\n", rulelib->path->cs );
			}
		}
		else
			_VT_DEBUG_PRINT_( stdout, LOG_TYPE_ERROR, "failed to load the shared library \"%s\"\n", rulelib->path->cs );
	}

	if( rulelib->hlib )
	{
		// initiate lib
		r = rulelib->ftab.initiate( 0, NULL );
		if( r == 0 )
		{
			_VT_DEBUG_PRINT_( stdout, LOG_TYPE_EVENT, "succeed to initiate the rulelib \"%s\"\n", rulelib->path ? rulelib->path->cs : "" );
			return 0;
		}
		else
		{
			// failed
			if( rulelib->path && rulelib->path->cs )
			{
				// it is external lib
				FreeLibrary( rulelib->hlib );
			}
			rulelib->hlib = NULL;
			_VT_DEBUG_PRINT_( stdout, LOG_TYPE_ERROR, "an error(%d) occurred when call the initiate of rulelib \"%s\"\n",
							  r, rulelib->path ? rulelib->path->cs : "@INTRINSIC" );
		}
	}

	return -1;
}

//
void vt_rulelib_unload( vt_rule_lib *rulelib )
{
	if( rulelib->hlib )
	{
		if( rulelib->ftab.release )
			rulelib->ftab.release();
		FreeLibrary( rulelib->hlib );
		rulelib->hlib = NULL;
	}
}

//
const vt_rule* vt_rule_match( vt_long rn, const char *alias, const vt_rule * const *rules, vt_long nrule )
{
	// NOTE: the rule table is natural order as defined in configure file, so do not adopting any search algorithm

	vt_long i = 0;

	if( alias && alias[0] )
		for( ; i < nrule && stricmp(rules[i]->alias->cs, alias)!=0; i++ );  // by alias
	else
		for( ; i < nrule && rules[i]->num != rn; i++ );  // by number

	return i<nrule ? rules[i] : NULL;
}
