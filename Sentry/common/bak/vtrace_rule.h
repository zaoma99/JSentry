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

#ifndef _AS_VTRACE_RULE_H_
#define _AS_VTRACE_RULE_H_

#include "vt_def.h"

struct _vtrace_brief_report;


// trace rule
typedef struct _vtrace_rule_t
{
	union {
		struct {
			vt_uint  num1;
			vt_uint  num2;
		};
		vt_long num;
	};
	vt_cstr *alias;
	struct _vtrace_rulelib_t *lib;  // points to a vt_rule_lib
}vt_rule;


#define VT_RULE_FUNNAME_INITIATE		"RuleLib_Initiate"
#define VT_RULE_FUNNAME_RELEASE			"RuleLib_Release"
#define VT_RULE_FUNNAME_PREPROCESS		"Rule_PreProcess"
#define VT_RULE_FUNNAME_POSTPROCESS		"Rule_PostProcess"

typedef VT_CALLDECL vt_int (*VT_RuleFunc_Initiate)( int argn, void *args );
typedef VT_CALLDECL void (*VT_RuleFunc_Release)( void );
typedef VT_CALLDECL vt_int (*VT_RuleFunc_PreProcess)( vt_uint rule_num, vt_dpoint *dpt, vt_stage stage, vt_cause cause, vt_sot_entity *entity );
typedef VT_CALLDECL vt_int (*VT_RuleFunc_PostProcess)( vt_uint rule_num, vt_dpoint *dpt, vt_sotnode *curr_sot, struct _vtrace_brief_report *rep );

typedef struct
{
	VT_RuleFunc_Initiate      initiate;
	VT_RuleFunc_Release       release;
	VT_RuleFunc_PreProcess    preproc;
	VT_RuleFunc_PostProcess   postproc;
}vt_rule_funtab;

// trace rule library
typedef struct _vtrace_rulelib_t
{
	vt_uint     id;
	vt_handle   hlib;
	vt_cstr     *path;
	vt_lang     lang;
	vt_proto    prot;
	vt_rule     **begptr;
	vt_rule     **endptr;
	vt_rule_funtab ftab;
}vt_rule_lib;



#ifdef __cplusplus
extern "C" {
#endif


VT_EXPORT int vt_rulelib_load( vt_rule_lib *rulelib );

VT_EXPORT void vt_rulelib_unload( vt_rule_lib *rulelib );

VT_EXPORT const vt_rule* vt_rule_match( vt_long rn, const char *alias, const vt_rule *const *rules, vt_long nrule );


#ifdef __cplusplus
}
#endif

#endif //_AS_VTRACE_RULE_H_
