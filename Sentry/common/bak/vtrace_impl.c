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
#include <unistd.h>
#include <sys/stat.h>

#include "vtrace_impl.h"
#include "vtrace_rule.h"
#include "vt_api.h"
#include "mmp.h"
#include "secure.h"

#include "facility/xmlAnalyzer.h"
#include "facility/xjson.h"

#include "java/vt_api_java.h"


// general symbols
#define _VT_SYMBOL_DEFINE_( _SYM_ )	{.len=sizeof(_SYM_)-1, .cs=_SYM_}

vt_cstr g_vt_path_data   = _VT_SYMBOL_DEFINE_( VT_DATAPATH );
vt_cstr g_vt_path_report = _VT_SYMBOL_DEFINE_( VT_DATAPATH"/"VT_REPOPATH );
//vt_cstr g_vt_path_proof  = _VT_SYMBOL_DEFINE_( VT_DATAPATH"/"VT_PROOPATH );
vt_cstr g_vt_path_meta   = _VT_SYMBOL_DEFINE_( VT_DATAPATH"/"VT_METAPATH );
vt_cstr g_vt_cstr_null   = _VT_SYMBOL_DEFINE_( "" );


// general dictionaries
 vt_dict g_vt_dict_TraceMode[] = {
		{/*.tag=VTRACE_DICT_TRACE_MODE,*/ .key="TRACE", .val.l=VTRACE_MODE_TRACE},
		{/*.tag=VTRACE_DICT_TRACE_MODE,*/ .key="RASP",  .val.l=VTRACE_MODE_RASP},
		{/*.tag=VTRACE_DICT_TRACE_MODE,*/ .key="IAST",  .val.l=VTRACE_MODE_IAST},
};

 vt_dict g_vt_dict_TraceKind[] = {
		{/*.tag=VTRACE_DICT_TRACE_KIND,*/ .key="KTP",   .val.l=VTRACE_KIND_KTP},
		{/*.tag=VTRACE_DICT_TRACE_KIND,*/ .key="CRIT",  .val.l=VTRACE_KIND_CRIT},
		{/*.tag=VTRACE_DICT_TRACE_KIND,*/ .key="AUTO",  .val.l=VTRACE_KIND_CRIT},
		{/*.tag=VTRACE_DICT_TRACE_KIND,*/ .key="BEGIN", .val.l=VTRACE_KIND_BEGIN},
		{/*.tag=VTRACE_DICT_TRACE_KIND,*/ .key="END",  .val.l=VTRACE_KIND_END},
		{/*.tag=VTRACE_DICT_TRACE_KIND,*/ .key="ASYN",  .val.l=VTRACE_KIND_ASYN},
};

 vt_dict g_vt_dict_TraceStage[] = {
		{/*.tag=,*/ .key="Enter", .val.l=VTRACE_STAGE_ENTER},
		{/*.tag=,*/ .key="Leave", .val.l=VTRACE_STAGE_LEAVE},
		{/*.tag=,*/ .key="Catch", .val.l=VTRACE_STAGE_CATCH},
		{/*.tag=,*/ .key="Trap",  .val.l=VTRACE_STAGE_TRAP},
		{/*.tag=,*/ .key="Begin", .val.l=VTRACE_STAGE_BEGIN},
		{/*.tag=,*/ .key="End",   .val.l=VTRACE_STAGE_END},
		{/*.tag=,*/ .key="Crash", .val.l=VTRACE_STAGE_CRASH},
};

 vt_dict g_vt_dict_TraceLevel[] = {
		{/*.tag=,*/ .key="os",        .val.l=VTRACE_LEVEL_OS},
		{/*.tag=,*/ .key="syslib",    .val.l=VTRACE_LEVEL_SYSLIB},
		{/*.tag=,*/ .key="stdlib",    .val.l=VTRACE_LEVEL_STDLIB},
		{/*.tag=,*/ .key="user",      .val.l=VTRACE_LEVEL_USER},
		{/*.tag=,*/ .key="vm",        .val.l=VTRACE_LEVEL_VM},
		{/*.tag=,*/ .key="framework", .val.l=VTRACE_LEVEL_FRAMEWORK},
		{/*.tag=,*/ .key="vstdlib",   .val.l=VTRACE_LEVEL_VSTDLIB},
		{/*.tag=,*/ .key="vuser",     .val.l=VTRACE_LEVEL_VUSER},
};

 vt_dict g_vt_dict_Language[] = {
		{/*.tag=,*/ .key="unknown", .val.l=VTRACE_LANG_UNKNOWN},
		{/*.tag=,*/ .key="native",  .val.l=VTRACE_LANG_NATIVE},
		{/*.tag=,*/ .key="asm",     .val.l=VTRACE_LANG_ASM},
		{/*.tag=,*/ .key="c",       .val.l=VTRACE_LANG_C},
		{/*.tag=,*/ .key="cpp",     .val.l=VTRACE_LANG_CPP},
		{/*.tag=,*/ .key="java",    .val.l=VTRACE_LANG_JAVA},
		{/*.tag=,*/ .key="py",      .val.l=VTRACE_LANG_PYTHON},
		{/*.tag=,*/ .key="go",      .val.l=VTRACE_LANG_GOLANG},
		{/*.tag=,*/ .key="js",      .val.l=VTRACE_LANG_JAVASCRIPT},
		{/*.tag=,*/ .key="php",     .val.l=VTRACE_LANG_PHP},
		{/*.tag=,*/ .key=".net",    .val.l=VTRACE_LANG_DOTNET},
};

 vt_dict g_vt_dict_Protocol[] = {
 		{/*.tag=,*/ .key="unknown", .val.l=VTRACE_PROT_UNKNOWN},
 		{/*.tag=,*/ .key="http",    .val.l=VTRACE_PROT_HTTP},
 		{/*.tag=,*/ .key="rpc",     .val.l=VTRACE_PROT_RPC},
 		{/*.tag=,*/ .key="grpc",    .val.l=VTRACE_PROT_gRPC},
 		{/*.tag=,*/ .key="winsock", .val.l=VTRACE_PROT_WSOCK},
 };

 vt_dict g_vt_dict_TraceCause[] = {
		{/*.tag=,*/ .key="unknown",           .val.l=VTRACE_CAUSE_NULL},
		{/*.tag=,*/ .key="data_propagate",    .val.l=VTRACE_CAUSE_DATA_PROPAGATE},
		{/*.tag=,*/ .key="data_transform",    .val.l=VTRACE_CAUSE_DATA_TRANSFORM},
		{/*.tag=,*/ .key="same_action",       .val.l=VTRACE_CAUSE_SAME_ACTION},
		{/*.tag=,*/ .key="same_event",        .val.l=VTRACE_CAUSE_SAME_EVENT},
		{/*.tag=,*/ .key="critical_point",    .val.l=VTRACE_CAUSE_CRIT_POINT},
		{/*.tag=,*/ .key="critical_resource", .val.l=VTRACE_CAUSE_CRIT_RESOURCE},
		{/*.tag=,*/ .key="specific_trap",     .val.l=VTRACE_CAUSE_SPEC_TRAP},
		{/*.tag=,*/ .key="consistency",       .val.l=VTRACE_CAUSE_CONSISTENCY},
		{/*.tag=,*/ .key="integrity",         .val.l=VTRACE_CAUSE_INTEGRITY},
		{/*.tag=,*/ .key="data_verity",       .val.l=VTRACE_CAUSE_DATA_VERIFY},
		{/*.tag=,*/ .key="authorize",         .val.l=VTRACE_CAUSE_AUTHORIZE},
		{/*.tag=,*/ .key="authenticate",      .val.l=VTRACE_CAUSE_AUTHENTICATE},
		{/*.tag=,*/ .key="credentials",       .val.l=VTRACE_CAUSE_CREDENTIALS},
		{/*.tag=,*/ .key="permit",            .val.l=VTRACE_CAUSE_PERMIT},
		{/*.tag=,*/ .key="code_execute",      .val.l=VTRACE_CAUSE_CODE_EXECUTE},
		{/*.tag=,*/ .key="code_abnormal",     .val.l=VTRACE_CAUSE_CODE_ABNORMAL},
		{/*.tag=,*/ .key="trace_start",       .val.l=VTRACE_CAUSE_TRACE_START},
		{/*.tag=,*/ .key="trace_done",        .val.l=VTRACE_CAUSE_TRACE_DONE},
		{/*.tag=,*/ .key="trace_break",       .val.l=VTRACE_CAUSE_TRACE_BREAK},
};

 vt_dict g_vt_dict_DataType[] = {
		{/*.tag=,*/ .key="void",    .val.l=VT_TYPE_NULL},
		{/*.tag=,*/ .key="char",    .val.l=VT_TYPE_CHAR},
		{/*.tag=,*/ .key="short",   .val.l=VT_TYPE_SHORT},
		{/*.tag=,*/ .key="int",     .val.l=VT_TYPE_INT},
		{/*.tag=,*/ .key="enum",    .val.l=VT_TYPE_ENUM},
		{/*.tag=,*/ .key="long",    .val.l=VT_TYPE_LONG},
		{/*.tag=,*/ .key="si16",    .val.l=VT_TYPE_SI128},
		{/*.tag=,*/ .key="byte",    .val.l=VT_TYPE_BYTE},
		{/*.tag=,*/ .key="bool",    .val.l=VT_TYPE_BOOL},
		{/*.tag=,*/ .key="wchar",   .val.l=VT_TYPE_WCHAR},
		{/*.tag=,*/ .key="word",    .val.l=VT_TYPE_WORD},
		{/*.tag=,*/ .key="uint",    .val.l=VT_TYPE_UINT},
		{/*.tag=,*/ .key="ulong",   .val.l=VT_TYPE_ULONG},
		{/*.tag=,*/ .key="ui16",    .val.l=VT_TYPE_UI128},
		{/*.tag=,*/ .key="float",   .val.l=VT_TYPE_FLOAT},
		{/*.tag=,*/ .key="double",  .val.l=VT_TYPE_DOUBLE},
		{/*.tag=,*/ .key="f16",     .val.l=VT_TYPE_F128},
		{/*.tag=,*/ .key="pointer", .val.l=VT_TYPE_POINTER},
		{/*.tag=,*/ .key="time",    .val.l=VT_TYPE_TIME},
		{/*.tag=,*/ .key="clock",   .val.l=VT_TYPE_CLOCK},
		{/*.tag=,*/ .key="ident",   .val.l=VT_TYPE_IDENTIFIER},
		{/*.tag=,*/ .key="cstring", .val.l=VT_TYPE_CSTRING},
		{/*.tag=,*/ .key="bstring", .val.l=VT_TYPE_BSTRING},
		{/*.tag=,*/ .key="struct",  .val.l=VT_TYPE_STRUCT},
		{/*.tag=,*/ .key="array",   .val.l=VT_TYPE_ARRAY},
};

 vt_dict g_vt_dict_JournalType[] = {
		{/*.tag=,*/ .key="disabled",    .val.l=SABI_JOURNAL_DIS},
		{/*.tag=,*/ .key="information", .val.l=SABI_JOURNAL_INFO},
		{/*.tag=,*/ .key="warning",     .val.l=SABI_JOURNAL_WARN},
		{/*.tag=,*/ .key="error",       .val.l=SABI_JOURNAL_ERROR},
		{/*.tag=,*/ .key="debug",       .val.l=SABI_JOURNAL_DEBUG},
		{/*.tag=,*/ .key="event",       .val.l=SABI_JOURNAL_EVENT},
		{/*.tag=,*/ .key="critical",    .val.l=SABI_JOURNAL_CRIT},
		{/*.tag=,*/ .key="maintain",    .val.l=SABI_JOURNAL_MT},
};

 vt_dict g_vt_dict_OperatorType[] = {
		{/*.tag=,*/ .key="any",           .val.l=VTRACE_INOP_ANY},
		{/*.tag=,*/ .key="new",           .val.l=VTRACE_INOP_NEW},
		{/*.tag=,*/ .key="delete",        .val.l=VTRACE_INOP_DELETE},
		{/*.tag=,*/ .key="concat",        .val.l=VTRACE_INOP_CONCAT},
		{/*.tag=,*/ .key="trunc",         .val.l=VTRACE_INOP_TRUNC},
		{/*.tag=,*/ .key="clear",         .val.l=VTRACE_INOP_CLEAR},
		{/*.tag=,*/ .key="assign",        .val.l=VTRACE_INOP_ASSIGN},
		{/*.tag=,*/ .key="copy",          .val.l=VTRACE_INOP_COPY},
		{/*.tag=,*/ .key="move",          .val.l=VTRACE_INOP_MOVE},
		{/*.tag=,*/ .key="slice",         .val.l=VTRACE_INOP_SLICE},
		{/*.tag=,*/ .key="change",        .val.l=VTRACE_INOP_CHANGE},
		{/*.tag=,*/ .key="verify",        .val.l=VTRACE_INOP_VERIFY},
		{/*.tag=,*/ .key="encode",        .val.l=VTRACE_INOP_ENCODE},
		{/*.tag=,*/ .key="decode",        .val.l=VTRACE_INOP_DECODE},
		{/*.tag=,*/ .key="escape",        .val.l=VTRACE_INOP_ESCAPE},
		{/*.tag=,*/ .key="expose",        .val.l=VTRACE_INOP_EXPOSE},  /* UnEscape*/
		{/*.tag=,*/ .key="format",        .val.l=VTRACE_INOP_FORMAT},
		{/*.tag=,*/ .key="serialize",     .val.l=VTRACE_INOP_SERIALIZE},
		{/*.tag=,*/ .key="unserialize",   .val.l=VTRACE_INOP_UNSERIALIZE},
		{/*.tag=,*/ .key="signature",     .val.l=VTRACE_INOP_SIGNATURE},
		{/*.tag=,*/ .key="encrypt",       .val.l=VTRACE_INOP_ENCRYPT},
		{/*.tag=,*/ .key="decrypt",       .val.l=VTRACE_INOP_DECRYPT},
		{/*.tag=,*/ .key="inflate",       .val.l=VTRACE_INOP_INFLATE},
		{/*.tag=,*/ .key="deflate",       .val.l=VTRACE_INOP_DEFLATE},
		{/*.tag=,*/ .key="shuffle",       .val.l=VTRACE_INOP_SHUFFLE},
		{/*.tag=,*/ .key="order",         .val.l=VTRACE_INOP_ORDER},
		{/*.tag=,*/ .key="sql",           .val.l=VTRACE_INOP_SQL},
		{/*.tag=,*/ .key="nosql",         .val.l=VTRACE_INOP_NOSQL},

		{/*.tag=,*/ .key="execute",       .val.l=VTRACE_INOP_EXECUTE},
		{/*.tag=,*/ .key="authorize",     .val.l=VTRACE_INOP_AUTHORIZE},
		{/*.tag=,*/ .key="authenticate",  .val.l=VTRACE_INOP_AUTHENTICATE},
		{/*.tag=,*/ .key="permit",        .val.l=VTRACE_INOP_PERMIT},
};

vt_dict g_vt_dict_OperatorSet[] = {
		{/*.tag=,*/ .key="source",        .val.l=VTRACE_OPSET_SOURCE},
		{/*.tag=,*/ .key="origin",        .val.l=VTRACE_OPSET_ORIGIN},
		{/*.tag=,*/ .key="propagate",     .val.l=VTRACE_OPSET_PROPAGATE},
		{/*.tag=,*/ .key="propagator",    .val.l=VTRACE_OPSET_PROPAGATE},
		{/*.tag=,*/ .key="general",       .val.l=VTRACE_OPSET_GENERAL},
		{/*.tag=,*/ .key="target",        .val.l=VTRACE_OPSET_TARGET},
		{/*.tag=,*/ .key="sink",          .val.l=VTRACE_OPSET_SINK},
		{/*.tag=,*/ .key="rule",          .val.l=VTRACE_OPSET_RULE},
		{/*.tag=,*/ .key="deadzone",      .val.l=VTRACE_OPSET_PROPAGATE|VTRACE_OPSET_DEADZONE},
		{/*.tag=,*/ .key="prop_dzone",    .val.l=VTRACE_OPSET_PROPAGATE|VTRACE_OPSET_DEADZONE},
		{/*.tag=,*/ .key="sink_dzone",    .val.l=VTRACE_OPSET_SINK|VTRACE_OPSET_DEADZONE},
		{/*.tag=,*/ .key="tag",           .val.l=VTRACE_OPSET_PROPAGATE|VTRACE_OPSET_ESCAPED},
		{/*.tag=,*/ .key="prop_safe",     .val.l=VTRACE_OPSET_PROPAGATE|VTRACE_OPSET_ESCAPED},
		{/*.tag=,*/ .key="source_safe",   .val.l=VTRACE_OPSET_SINK|VTRACE_OPSET_ESCAPED},
		{/*.tag=,*/ .key="sink_safe",     .val.l=VTRACE_OPSET_SINK|VTRACE_OPSET_ESCAPED},
		{/*.tag=,*/ .key="sql",           .val.l=VTRACE_OPSET_SINK},
		{/*.tag=,*/ .key="nosql",         .val.l=VTRACE_OPSET_SINK},
		{/*.tag=,*/ .key="validator",     .val.l=VTRACE_OPSET_SINK},
		{/*.tag=,*/ .key="http",          .val.l=VTRACE_OPSET_SOURCE},
		{/*.tag=,*/ .key="custom",        .val.l=VTRACE_OPSET_PROPAGATE},
};


#define VT_DICT_SORT_BY		2	// 1=value, 2=key

//
static int g_vt_conf_initiated = 0;

vt_journal_type    g_vt_journal_mask = \
		SABI_JOURNAL_WARN | SABI_JOURNAL_ERROR| SABI_JOURNAL_EVENT| SABI_JOURNAL_CRIT | SABI_JOURNAL_MT;
//
const EVP_MD *g_vt_dds_md = NULL;

#define VT_MAKE_DDS1( _ctx_, _md_, _src1_, _len1_, _out_, _outsz_ ) (\
		EVP_DigestInit_ex( _ctx_, _md_, NULL ) && \
		EVP_DigestUpdate( _ctx_, _src1_, _len1_ ) && \
		EVP_DigestFinal_ex( _ctx_, _out_, _outsz_ ) )

#define VT_MAKE_DDS2( _ctx_, _md_, _src1_, _len1_, _src2_, _len2_, _out_, _outsz_ ) (\
		EVP_DigestInit_ex( _ctx_, _md_, NULL ) && \
		EVP_DigestUpdate( _ctx_, _src1_, _len1_ ) && \
		EVP_DigestUpdate( _ctx_, _src2_, _len2_ ) && \
		EVP_DigestFinal_ex( _ctx_, _out_, _outsz_ ) )

#define VT_MAKE_DDS3( _ctx_, _md_, _src1_, _len1_, _src2_, _len2_, _src3_, _len3_, _out_, _outsz_ ) (\
		EVP_DigestInit_ex( _ctx_, _md_, NULL ) && \
		EVP_DigestUpdate( _ctx_, _src1_, _len1_ ) && \
		EVP_DigestUpdate( _ctx_, _src2_, _len2_ ) && \
		EVP_DigestUpdate( _ctx_, _src3_, _len3_ ) && \
		EVP_DigestFinal_ex( _ctx_, _out_, _outsz_ ) )





// ===================================================================================================================================== //
#define _IS_SPACE_CHAR(_C_)	\
		( (_C_) == '\r' || (_C_) == '\n' || \
		  (_C_) == '\t' || (_C_) == '\b' || \
		  (_C_) == '\f' || (_C_) == 32 )

#define _IS_SEPA_CHAR(_C_)	( (_C_) == '=' || (_C_) == '\0' || _IS_SPACE_CHAR(_C_) )


//
int vt_parse_text_line( char *txt, const char *sepchr, const char **argv, int argn )
{
	int n;
	char *s;


	s = txt;
	n = 0;
	while( n < argn )
	{
		// left trim
		while( _IS_SPACE_CHAR(*s) || strchr(sepchr, *s) ) s++;
		if( *s == 0 )
			break;

		argv[n++] = s++;

		// seek the end of current argument(!_IS_BLANK_CHAR( *s ))
		while( *s && !strchr(sepchr, *s) ) s++;
		if( *s == 0 )
			break;

		*s++ = 0;
	}

	if( n > 0 )
	{
		s--;
		while( _IS_SPACE_CHAR( *s ) ) s--;
		*(++s) = 0;
	}

	return n;
}


//
const char * vt_seek_argument( int argc, const char **argv, const char *argname )
{
	size_t l = strlen( argname );
	int i;
	for( i=0; i < argc; i++ )
	{
		if( strnicmp( argv[i], argname, l ) == 0 && argv[i][l] == '=' )
		{
			return &argv[i][l+1];
		}
	}

	return NULL;
}

//
const char * vt_seek_argument2( int argc, const char **argv, const char *argname )
{
	char cc;
	size_t l = strlen( argname );
	int i;
	for( i=0; i < argc; i++ )
	{
		if( strnicmp( argv[i], argname, l ) == 0 )
		{
			cc = argv[i][l];
			if( cc == 0 || _IS_SEPA_CHAR(cc) )
				return &argv[i][l];
		}
	}

	return NULL;
}

//
inline static char * _is_num_or_cstr( char *sym, vt_value *val )
{
	char *tail;
	val->l = strtol( sym, &tail, 0 );
	return ( !tail || tail == sym || *tail != 0 ) ? sym : NULL;
}

//
void vt_dict_sort_byval( vt_dict *dict, long len )
{
	long i, j;
    vt_dict _item;

    if( !dict || len == 0 )
       	return;

    for( i=len-1; i > 0; i-- )
    {
        for( j=0; j < i; j++ )
        {
            if( dict[j].val.l > dict[j+1].val.l )
            {
                _item = dict[j];
                dict[j] = dict[j+1];
                dict[j+1] = _item;
            }
        }
    }
}

//
void vt_dict_sort_bykey( vt_dict *dict, long len )
{
	long i, j;
    vt_dict _item;

    if( !dict || len == 0 )
    	return;

    for( i=len-1; i > 0; i-- )
    {
        for( j=0; j < i; j++ )
        {
        	if( strcmp(dict[j].key, dict[j+1].key) > 0 )
            {
                _item = dict[j];
                dict[j] = dict[j+1];
                dict[j+1] = _item;
            }
        }
    }
}

//
const vt_dict* vt_dict_match_val( const vt_dict *dict, long len, vt_value val )
{
	register long b = 0, i;

	while( b < len )
	{
		i = b + (len - b) / 2;

		if( dict[i].val.l == val.l )
			return &dict[i];

		if( dict[i].val.l < val.l )
			b = i + 1;
		else
			len = i;
	}
	return NULL;
}

//
const vt_dict* vt_dict_match_key( const vt_dict *dict, long len, const char *key )
{
	register long b = 0, i, d;

	while( b < len )
	{
		i = b + (len - b) / 2;

		if( (d=strcmp(dict[i].key, key)) == 0 )
			return &dict[i];

		if( d < 0 )
			b = i + 1;
		else
			len = i;
	}
	return NULL;
}

//
const vt_dict* vt_dict_match_val2( const vt_dict *dict, long len, vt_value val )
{
	long i;

	for( i=0; i < len && dict[i].val.l != val.l; i++ );
	return i < len ? &dict[i] : NULL;
}

//
const vt_dict* vt_dict_match_key2( const vt_dict *dict, long len, const char *key )
{
	long i;

	for( i=0; i < len && strcmp(dict[i].key, key) != 0; i++ );
	return i < len ? &dict[i] : NULL;
}

// assume the dictionary has been sorted by value
int vt_dict_getvalue_1( const vt_dict *dict, long len, const char *sym, long *val )
{
	char *tail;
	const vt_dict *d;
	vt_value v;

	v.l = strtol( sym, &tail, 0 );

	// assume the dictionary has been sorted by value
	if( !tail || tail == sym || *tail != 0 )
		d = vt_dict_match_key2( dict, len, sym );  // match key
	else
		d = vt_dict_match_val( dict, len, v );     // match value

	if( !d )
		return -1;

	*val = d->val.l;
	return 0;
}

// assume the dictionary has been sorted by key
int vt_dict_getvalue_2( const vt_dict *dict, long len, const char *sym, long *val )
{
	char *tail;
	const vt_dict *d;
	vt_value v;

	v.l = strtol( sym, &tail, 0 );

	// assume the dictionary has been sorted by key
	if( !tail || tail == sym || *tail != 0 )
		d = vt_dict_match_key( dict, len, sym );  // match key
	else
		d = vt_dict_match_val2( dict, len, v );   // match value

	if( !d )
		return -1;

	*val = d->val.l;
	return 0;
}

//
inline static int vt_dict_getvalue( const vt_dict *dict, long len, const char *sym, long *val )
{
#if (VT_DICT_SORT_BY == 2)
	return vt_dict_getvalue_2( dict, len, sym, val );
#else
	return vt_dict_getvalue_1( dict, len, sym, val );
#endif
}


//
void vt_evttab_sort( vt_ns_evttab_item *ite, long nite )
{
    long i, j;
    vt_ns_evttab_item _item;

    if( !ite || nite == 0 )
       	return;

    for( i=nite-1; i > 0; i-- )
    {
        for( j=0; j < i; j++ )
        {
            if( ite[j].type > ite[j+1].type )
            {
                _item = ite[j];
                ite[j] = ite[j+1];
                ite[j+1] = _item;
            }
        }
    }
}

//
vt_ns_evttab_item* vt_evttab_match( vt_ns_evttab_item *ite, long nite, vt_uint type )
{
	register long b = 0, i;

	while( b < nite )
	{
		i = b + (nite - b) / 2;

		if( ite[i].type == type )
			return &ite[i];

		if( ite[i].type < type )
			b = i + 1;
		else
			nite = i;
	}
	return NULL;
}

//
static vt_ns_evttab_item* _vt_evttab_setup( const vt_dict *dict, long len, long *mmphd )
{
	vt_ns_evttab_item *ite = mmp_malloc( 1, mmphd, sizeof(vt_ns_evttab_item) * len );
	if( ite )
	{
		long i;
		vt_time t = _TICKGET() - 3600000;

		memset( ite, 0, sizeof(vt_ns_evttab_item) * len );

		for( i=0; i < len; i++ )
		{
			ite[i].type = dict[i].val.u;
			ite[i].tmlast = t;
		}

		vt_evttab_sort( ite, len );
	}
	return ite;
}


// ===================================================================================================================================== //
//
#define VT_SYMBOL_GROUP_INC_UNITS	(4)
#define VT_SYMBOL_GROUP_MAX			(0x7ffffff8)
//#define VT_SYMBOL_GROUP_CAPS		(512)
//#define VT_SYMBOL_GROUP_CAPS_SHF	(9)
#define VT_SYMBOL_MAGIC_VALUE		(0x1a8d3df0ee07c)

//
int vt_symtab_init( vt_symtab *tab, vt_uint grp_caps )
{
	memset( tab, 0, sizeof(vt_symtab) );
	tab->grp_caps = ALIGN_SIZE( grp_caps, 8 );
	mmp_create_mempool( 1, &tab->mpool );
	return 0;
}

//
void vt_symtab_release( vt_symtab *tab )
{
	mmp_destroy_mempool( 1, &tab->mpool );
	memset( tab, 0, sizeof(vt_symtab) );
}

//
inline static int vt_symtab_group_inc( vt_symtab *tab )
{
	vt_uint n = ALIGN_SIZE( tab->ngrp, VT_SYMBOL_GROUP_INC_UNITS );
	if( n >= VT_SYMBOL_GROUP_MAX )
		return -1;

	void *ptr = mmp_realloc( 1, &tab->mpool, tab->grp, sizeof(struct _vtrace_symgrp)*(n + VT_SYMBOL_GROUP_INC_UNITS) );
	if( !ptr )
		return -1;

	tab->grp = (struct _vtrace_symgrp*)ptr;
	memset( &tab->grp[n], 0, sizeof(struct _vtrace_symgrp)*VT_SYMBOL_GROUP_INC_UNITS );
	return 0;
}

//
#ifdef VT_SYMBOL_HIDDEN_POINTER
//inline static
vt_cstr* vt_symtab_get( vt_symtab *tab, vt_refer ref )
{
	ref = (vt_refer)((size_t)ref ^ (size_t)tab ^ VT_SYMBOL_MAGIC_VALUE);
	register long g = (long)ref / tab->grp_caps;
	register long i = (long)ref & (tab->grp_caps-1);
	return g >= tab->ngrp ? NULL : &tab->grp[g].syms[i];
}
#endif

//
vt_refer vt_symtab_add( vt_symtab *tab, const char *sym, size_t len )
{
	register long i, g = (long)tab->ngrp - 1;

	if( g == -1 || ((i=tab->grp[g].nsym) == tab->grp_caps) )
	{
		g++;
		if( (g & (VT_SYMBOL_GROUP_INC_UNITS-1)) == 0 )
		{
			if( vt_symtab_group_inc( tab ) != 0  )
				return NULL;
		}
		i = 0;
		tab->ngrp++;
	}

	if( !tab->grp[g].syms )
	{
		tab->grp[g].syms = mmp_malloc( 1, &tab->mpool, sizeof(vt_cstr) * tab->grp_caps );
		if( !tab->grp[g].syms )
			return NULL;
		memset( tab->grp[g].syms, 0, sizeof(vt_cstr) * tab->grp_caps );
	}

	register vt_cstr *cs = &tab->grp[g].syms[i];

	cs->cs = mmp_malloc( 1, &tab->mpool, len+1 );
	if( !cs->cs )
		return NULL;

	if( sym )
	{
		memcpy( cs->cs, sym, len );
		cs->cs[len] = 0;
	}
	else
		cs->cs[0] = 0;
	cs->len = len;

	tab->grp[g].nsym++;

#ifdef VT_SYMBOL_HIDDEN_POINTER
	++i;
	return (vt_refer)((((size_t)g * tab->grp_caps) | i) ^ (size_t)tab ^ VT_SYMBOL_MAGIC_VALUE);
#else
	return cs;
#endif
}

//
void vt_symtab_del( vt_symtab *tab, vt_refer ref )
{
	register vt_cstr *cs = vt_symtab_get( tab, ref );
	if( !cs )
	{
		if( cs->cs )
			mmp_free( 1, &tab->mpool, cs->cs );
		cs->cs = NULL;
		cs->len = 0;
	}
}

//
vt_refer vt_symtab_find( vt_symtab *tab, const char *sym, size_t len )
{
	register vt_refer r = NULL;
	register struct _vtrace_symgrp *grp = tab->grp;
	register vt_uint i;

	for( i = tab->ngrp;
		 i > 0 && !(r=vt_symtab_match_in_group(&grp[--i], sym, len));
		);
	return r;
}

// ===================================================================================================================================== //
//
int vt_datobj_cache_init( vt_cache_box *cache )
{
	memset( cache, 0, sizeof(vt_cache_box) );
	return 0;
}

//
static int _cache_destroy_echo( btree_node *node, void *hd )
{
	mmp_free( 1, (long*)hd, node );
	return 0;
}

//
void vt_datobj_cache_destroy( vt_cache_box *cache, long *mmphd )
{
	if( cache && cache->root )
		btree_traverse1( (btree_node*)cache->root, mmphd, _cache_destroy_echo );
}

//
vt_cache_item* vt_datobj_cache_insert( vt_cache_box *cache, long key, long *mmphd )
{
	vt_cache_item *ite = mmp_malloc( 1, mmphd, sizeof(vt_cache_item) );
	if( ite )
	{
		ite->key = key;
		btree_insert( (btree_node**)&cache->root, (btree_node*)ite );
	}
	return ite;
}

//
vt_cache_item* vt_datobj_cache_hit( vt_cache_box *cache, long key )
{
	return (cache && cache->root) ? (vt_cache_item*)btree_search( (btree_node*)cache->root, key, FALSE ) : NULL;
}

//
void vt_datobj_cache_drop( vt_cache_box *cache, long key )
{
	if( cache && cache->root )
	{
		btree_node *nd = btree_search( (btree_node*)cache->root, key, FALSE );
		if( nd )
			btree_drop( (btree_node**)&cache->root, nd );
	}
}


// ===================================================================================================================================== //
//
inline static char* vt_decl_split( vt_lang lang, const char *decl_name, char **sign, char **buf, const char *klass_ident )
{
	switch( lang )
	{
	case VTRACE_LANG_JAVA:
		return vt_java_decl_split( decl_name, sign, buf, klass_ident );
	default:
		return NULL;
	}
}

//
int vt_decl_check( vt_lang lang, char *decl, int len )
{
	switch( lang )
	{
	case VTRACE_LANG_JAVA:
		return vt_java_decl_check( decl, len );
	default:
		return 0;
	}
}

//
int vt_decl_norm( vt_lang lang, const char *decl, int len, char **dst, int *dstsize )
{
	switch( lang )
	{
	case VTRACE_LANG_JAVA:
		return vt_java_decl_norm( decl, len, dst, dstsize );
	default:
		return 0;
	}
}

//
int vt_type_norm( vt_lang lang, const char *token, int len, char *new_token )
{
	switch( lang )
	{
	case VTRACE_LANG_JAVA:
		return vt_java_1sign_norm( token, len, new_token );
	default:
		return 0;
	}
}

//
vt_typeid vt_type_convert( vt_lang lang, const char *sym, int len, vt_byte *ndim, vt_size **shape )
{
	// NOTE: assume declaration has been checked and normalized

	switch( lang )
	{
	case VTRACE_LANG_JAVA:
		*ndim = 0;
		*shape = NULL;
		return sym && len > 0 ? vt_java_type_convert( sym, ndim ) : VT_TYPE_NULL;
	default:
		*ndim = 0;
		*shape = NULL;
		break;
	}
	return VT_TYPE_NULL;
}

//
int vt_decl_get_1arg( vt_lang lang, const char *decl_str, int idx, vt_byte *ndim, int *arg_strlen )
{
	// NOTE: assume declaration has been checked and normalized

	switch( lang )
	{
	case VTRACE_LANG_JAVA:
		return decl_str ? vt_java_get_1sign( decl_str, idx, ndim, arg_strlen ) : -1;
	default:
		break;
	}
	return -1;
}

//
int vt_vm_callstack_copy( vt_lang lang, vt_thread thread, int begin, int max, vt_call_frame **dst, vt_bool onlyref, long *mmphd )
{
	switch( lang )
	{
	case VTRACE_LANG_JAVA:
		return vt_java_callstack_copy( thread, begin, max, dst, onlyref, mmphd );
	default:
		return -1;
	}
}

//
vt_object vt_vm_globalref_new( vt_lang lang, vt_object obj )
{
	switch( lang )
	{
	case VTRACE_LANG_JAVA:
		//return obj;
		return vt_java_globalref_new( obj );
	default:
		return obj;
	}
}

//
void vt_vm_globalref_del( vt_lang lang, vt_object obj )
{
	switch( lang )
	{
	case VTRACE_LANG_JAVA:
		vt_java_globalref_del( obj );
		break;
	default:
		break;
	}
}

//
vt_object vt_vm_replica_ref_new( vt_lang lang, vt_object obj, vt_long uid )
{
	switch( lang )
	{
	case VTRACE_LANG_JAVA:
		return (vt_object)uid;
	default:
		return obj;
	}
}

//
void vt_vm_replica_ref_del( vt_lang lang, vt_object obj, vt_long uid )
{
	/*switch( lang )
	{
	case VTRACE_LANG_JAVA:
		break;
	default:
		break;
	}*/
}

//
int vt_vm_objcmp( vt_lang lang, vt_object obj1, vt_object obj2 )
{
	switch( lang )
	{
	case VTRACE_LANG_JAVA:
		return vt_java_objcmp( obj1, obj2 );
	default:
		return (obj1 == obj2) - 1;
	}
}

//
int vt_vm_bind_object( vt_lang lang, vt_object obj, long tag )
{
	switch( lang )
	{
	case VTRACE_LANG_JAVA:
		return vt_java_bind_object( obj, tag );
	default:
		return -1;
	}
}

//
void vt_vm_action_exit( vt_lang lang, vt_refer actref, vt_refer arg )
{
	switch( lang )
	{
	case VTRACE_LANG_JAVA:
		//return vt_java_action_exit( actref, arg );
	default:
		break;
	}
}


//
const vt_class_struct* vt_class_find1( const vt_ident *uid, const vt_class_struct * const *klasses, vt_long num )
{
	register vt_long b = 0, i;
	register vt_size _h, _l, h = uid->high, l = uid->low;

	while( b < num )
	{
		i = b + (num - b) / 2;

		_h = klasses[i]->uid.high;
		_l = klasses[i]->uid.low;
		if( _h == h )
		{
			if( _l == l )
				return klasses[i];
			else if( _l < l )
				b = i + 1;
			else
				num = i;
		}
		else if( _h < h )
			b = i + 1;
		else
			num = i;
	}
	return NULL;
}

//
const vt_class_struct* vt_class_find2( const char *name, const vt_class_struct * const *klasses, vt_long num )
{
	register vt_long b = 0, i, d;

	while( b < num )
	{
		i = b + (num - b) / 2;

		d = strcmp( ((vt_cstr*)klasses[i]->name)->cs, name );
		if( d == 0 )
			return klasses[i];

		if( d < 0 )
			b = i + 1;
		else
			num = i;
	}
	return NULL;
}

//
const vt_class_struct* vt_class_find3( const char *name, const vt_class_struct * const *klasses, vt_long num )
{
	register vt_long i;
	for( i=num-1; i>=0 && strcmp( ((vt_cstr*)klasses[i]->name)->cs, name )!=0; i-- );
	return i>=0 ? klasses[i] : NULL;
}

//
const vt_method_struct* vt_method_find1( const vt_ident *uid, const vt_method_struct * const *methods, vt_long num )
{
	register vt_long b = 0, i;
	register vt_size _h, _l, h = uid->high, l = uid->low;

	while( b < num )
	{
		i = b + (num - b) / 2;

		_h = methods[i]->uid.high;
		_l = methods[i]->uid.low;
		if( _h == h )
		{
			if( _l == l )
				return methods[i];
			else if( _l < l )
				b = i + 1;
			else
				num = i;
		}
		else if( _h < h )
			b = i + 1;
		else
			num = i;
	}
	return NULL;
}

//
const vt_method_struct* vt_method_find2( const char *name, const char *sign, const vt_method_struct * const *methods, vt_long num )
{
	register vt_long b = 0, i, d;

	while( b < num )
	{
		i = b + (num - b) / 2;

		//if( (d = ((vt_refer)methods[i]->klass - klass)) == 0 )
		{
			d = strcmp( ((vt_cstr*)methods[i]->name)->cs, name );
			if( d == 0 )
				d = strcmp( methods[i]->decl, sign );
			if( d == 0 )
				return methods[i];
		}

		if( d < 0 )
			b = i + 1;
		else
			num = i;
	}
	return NULL;
}

//
const vt_method_struct* vt_method_find3( const char *name, const char *sign, const vt_method_struct * const *methods, vt_long num )
{
	register vt_long i;

	for( i=num-1;
		 i>=0 && (strcmp( ((vt_cstr*)methods[i]->name)->cs, name )!=0 || strcmp( methods[i]->decl, sign )!=0);
	     i-- );
	return i>=0 ? methods[i] : NULL;
}

//
const vt_point* vt_trace_point_find1( const vt_ident *uid, const vt_point * const *points, vt_long num )
{
	register vt_long b = 0, i;
	register vt_size _h, _l, h = uid->high, l = uid->low;

	while( b < num )
	{
		i = b + (num - b) / 2;

		_h = points[i]->target->uid.high;
		_l = points[i]->target->uid.low;
		if( _h == h )
		{
			if( _l == l )
				return points[i];
			else if( _l < l )
				b = i + 1;
			else
				num = i;
		}
		else if( _h < h )
			b = i + 1;
		else
			num = i;
	}
	return NULL;
}

//
const vt_point* vt_trace_point_find2( const char *name, const char *sign, vt_refer klass, const vt_point * const *points, vt_long num )
{
	register vt_long b = 0, i, d;

	while( b < num )
	{
		i = b + (num - b) / 2;

		if( (d = ((vt_refer)points[i]->target->klass - klass)) == 0 )
		{
			d = strcmp( ((vt_cstr*)points[i]->target->name)->cs, name );
			if( d == 0 )
				d = strcmp( points[i]->target->decl, sign );
			if( d == 0 )
				return points[i];
		}

		if( d < 0 )
			b = i + 1;
		else
			num = i;
	}
	return NULL;
}

//
const vt_point* vt_trace_point_find3( const char *name, const char *sign, vt_refer klass, const vt_point * const *points, vt_long num )
{
	register vt_long i;

	for( i=num-1;
		 i>=0 && ((klass && points[i]->target->klass!=klass) ||
				  strcmp( ((vt_cstr*)points[i]->target->name)->cs, name )!=0 ||
				  strcmp( points[i]->target->decl, sign )!=0);
	     i-- );
	return i>=0 ? points[i] : NULL;
}

//
vt_class_struct** vt_class_struct_build_index( const vt_class_struct * const *klasses, vt_long num, vt_long ncap, long *mmphd )
{
    register vt_long i, j;
    register vt_class_struct *t, **index = NULL;

    if( klasses && num > 0 )
    {
    	if( ncap < num )
    		ncap = num;

		index = (vt_class_struct**)mmp_malloc( 1, mmphd, sizeof(void**) * ncap * 2 + sizeof(size_t)*2 );  // temporary process
		if( index )
		{
			// by string

			//index += ncap - num;

			memcpy( index, klasses, sizeof(void**)*num );
			for( i=num-1; i > 0; i-- )
			{
				for( j=0; j < i; j++ )
				{
					if( strcmp(((vt_cstr*)index[j]->name)->cs, ((vt_cstr*)index[j+1]->name)->cs) > 0 )
					{
						t = index[j];
						index[j] = index[j+1];
						index[j+1] = t;
					}
				}
			}

			// by uid
			index = &index[ncap];
			memcpy( index, klasses, sizeof(void**)*num );
			for( i=num-1; i > 0; i-- )
			{
				for( j=0; j < i; j++ )
				{
					if( index[j]->uid.high > index[j+1]->uid.high ||
						(index[j]->uid.high == index[j+1]->uid.high &&
						 index[j]->uid.low > index[j+1]->uid.low) )
					{
						t = index[j];
						index[j] = index[j+1];
						index[j+1] = t;
					}
				}
			}
			index -= ncap;
		}
    }
    return index;
}

#if 0
//
vt_method_struct** vt_method_struct_build_index( const vt_method_struct * const *methods, vt_long num, long *mmphd )
{
	register vt_long i, j, t;
	register vt_method_struct **index = NULL;

    if( methods && num > 0 )
    {
		index = (vt_method_struct**)mmp_malloc( 1, mmphd, sizeof(void**) * num * 2);
		if( index )
		{
			// by string
			memcpy( index, methods, sizeof(void**)*num );
			for( i=num-1; i > 0; i-- )
			{
				for( j=0; j < i; j++ )
				{
					if( (t = index[j]->klass - index[j+1]->klass) == 0 )
						t = strcmp( ((vt_cstr*)index[j]->name)->cs, ((vt_cstr*)index[j+1]->name)->cs );
					if( t > 0 || (t == 0 && strcmp(index[j]->decl, index[j+1]->decl) > 0) )
					{
						t = (vt_long)index[j];
						index[j] = index[j+1];
						index[j+1] = (vt_method_struct*)t;
					}
				}
			}

			// by uid
			index = &index[num];
			memcpy( index, methods, sizeof(void**)*num );
			for( i=num-1; i > 0; i-- )
			{
				for( j=0; j < i; j++ )
				{
					if( index[j]->uid.high > index[j+1]->uid.high ||
						(index[j]->uid.high == index[j+1]->uid.high &&
						 index[j]->uid.low > index[j+1]->uid.low) )
					{
						t = (vt_long)index[j];
						index[j] = index[j+1];
						index[j+1] = (vt_method_struct*)t;
					}
				}
			}
			index -= num;
		}
    }
    return index;
}

//
vt_point** vt_trace_point_build_index( const vt_point * const *points, vt_long num, long *mmphd )
{
	register vt_long i, j, t;
	register vt_point **index = NULL;

    if( points && num > 0 )
    {
		index = (vt_point**)mmp_malloc( 1, mmphd, sizeof(void**) * num * 2 );
		if( index )
		{
			// by string
			memcpy( index, points, sizeof(void**)*num );
			for( i=num-1; i > 0; i-- )
			{
				for( j=0; j < i; j++ )
				{
					if( (t = index[j]->target->klass - index[j+1]->target->klass) == 0 )
						t = strcmp( ((vt_cstr*)index[j]->target->name)->cs, ((vt_cstr*)index[j+1]->target->name)->cs );
					if( t > 0 || (t == 0 && strcmp(index[j]->target->decl, index[j+1]->target->decl) > 0) )
					{
						t = (vt_long)index[j];
						index[j] = index[j+1];
						index[j+1] = (vt_point*)t;
					}
				}
			}

			// by string
			index = &index[num];
			memcpy( index, points, sizeof(void**)*num );
			for( i=num-1; i > 0; i-- )
			{
				for( j=0; j < i; j++ )
				{
					if( index[j]->target->uid.high > index[j+1]->target->uid.high ||
						(index[j]->target->uid.high == index[j+1]->target->uid.high &&
						 index[j]->target->uid.low > index[j+1]->target->uid.low) )
					{
						t = (vt_long)index[j];
						index[j] = index[j+1];
						index[j+1] = (vt_point*)t;
					}
				}
			}
			index -= num;
		}
    }
    return index;
}
#endif

//
int vt_method_point_build_index( vt_class_struct * const *klass_idx,
								 vt_long nklass,
								 const vt_method_struct * const *methods,
								 const vt_point * const *points,
								 vt_long num,
								 vt_long ncap,
								 vt_method_struct ***mtd_idx,
								 vt_point ***tp_idx,
								 long *mmphd )
{
	register vt_long i, j, t, k;
	register vt_method_struct **idx_m;
	register vt_point **idx_p;

    if( !methods || !points || num <= 0 )
    	return -1;
    if( ncap < num )
    	ncap = num;

	idx_m = (vt_method_struct**)mmp_malloc( 1, mmphd, sizeof(void**) * ncap * 4 + sizeof(void**)*2 );  // temporary process
	if( idx_m )
	{
		idx_p = (vt_point**)&idx_m[ncap*2];

		*mtd_idx = idx_m;
		*tp_idx = idx_p;

		// group by class
		for( k=0; k < nklass; k++ )
		{
			i = klass_idx[k]->method_num;
			klass_idx[k]->mtd_idx = idx_m;
			klass_idx[k]->mtdidx_num = i;
			memcpy( idx_m, klass_idx[k]->methods, sizeof(void**)*i );

			// sort by declaration
			for( i--; i > 0; i-- )
			{
				for( j=0; j < i; j++ )
				{
					t = strcmp( ((vt_cstr*)idx_m[j]->name)->cs, ((vt_cstr*)idx_m[j+1]->name)->cs );
					if( t > 0 || (t == 0 && strcmp(idx_m[j]->decl, idx_m[j+1]->decl) > 0) )
					{
						t = (vt_long)idx_m[j];
						idx_m[j] = idx_m[j+1];
						idx_m[j+1] = (vt_method_struct*)t;
					}
				}
			}
			idx_m += klass_idx[k]->method_num;
		}

		// trace point order is same as methods
		idx_m = *mtd_idx;
		for( i=num-1; i >= 0; i-- ) idx_p[i] = idx_m[i]->tpoint;

		// by uid
		idx_m = *mtd_idx + ncap;
		idx_p = *tp_idx + ncap;
		memcpy( idx_m, methods, sizeof(void**)*num );
		memcpy( idx_p, points, sizeof(void**)*num );
		for( i=num-1; i > 0; i-- )
		{
			for( j=0; j < i; j++ )
			{
				if( idx_m[j]->uid.high > idx_m[j+1]->uid.high ||
					(idx_m[j]->uid.high == idx_m[j+1]->uid.high &&
					 idx_m[j]->uid.low > idx_m[j+1]->uid.low) )
				{
					t = (vt_long)idx_m[j];
					idx_m[j] = idx_m[j+1];
					idx_m[j+1] = (vt_method_struct*)t;
					//
					t = (vt_long)idx_p[j];
					idx_p[j] = idx_p[j+1];
					idx_p[j+1] = (vt_point*)t;
				}
			}
		}
	}
    return 0;
}

//
vt_class_struct* vt_class_copy( vt_namespace *ns, vt_class_struct *dst, const vt_class_struct *src, vt_bool lock )
{
	vt_long n, j, t;
	vt_method *mtd;
	vt_point  **tp;
	vt_method **mtd_d = dst->methods;
	vt_method **mtd_s = src->methods;
	vt_long nmtd_d = dst->method_num;
	vt_long nmtd_s = src->method_num;

	if( !mtd_s || nmtd_s < 1 )
		return NULL;

	if( !lock || BsemTake(ns->lock, -1) )
	{
		// check method table capacity of the "dst"
		n = ALIGN_SIZE( nmtd_d+nmtd_s, 64 );
		if( n > ALIGN_SIZE(nmtd_d, 64) )
		{
			// re-allocate method table of the "dst"
			mtd_d = (vt_method_struct**)mmp_malloc( 1, &ns->mmphd, sizeof(void**) * n + sizeof(size_t)*2 );  // temporary process
			if( !mtd_d )
				goto LAB_ERROR;

			// TODO: put old method table into the garbage-bin

			memcpy( mtd_d, dst->methods, sizeof(void**)*nmtd_d );
			dst->methods = mtd_d;
		}

		// copy methods definition
		for( j=nmtd_d,n=0; n < nmtd_s; n++,j++ )
		{
			mtd_d[j] = mtd = (vt_method_struct*)mmp_malloc( 1, &ns->mmphd, (sizeof(vt_method)+sizeof(vt_point)) );
			if( !mtd )
				goto LAB_ERROR;
			*mtd = *mtd_s[n];
			mtd->klass = dst;
			mtd->tpoint = (vt_refer)&mtd[1];
			memset( &mtd->uid, 0, sizeof(mtd->uid) );  // without uid
			//
			*(vt_point*)mtd->tpoint = *(vt_point*)mtd_s[n]->tpoint;
			((vt_point*)mtd->tpoint)->target = mtd;
			((vt_point*)mtd->tpoint)->idx = 0;
		}

		// renew the global table of all methods and all trace points
		j = ns->defines.nmethod;
		mtd_d = ns->defines.methods;
		tp = ns->defines.tracepoints;
		n = j + nmtd_s;
		if( n >= ns->defines.caps._methods )
		{
			// re-allocate global table
			tp = (vt_point**)mmp_malloc( 1, &ns->mmphd, sizeof(void**) * (n+=1024) * 2 + sizeof(size_t)*2 );  // temporary process
			if( !tp )
				goto LAB_ERROR;

			// TODO: put old global table into the garbage-bin

			mtd_d = (vt_method_struct**)&tp[n];
			memcpy( mtd_d, ns->defines.methods, sizeof(void**)*j );
			memcpy( tp, ns->defines.tracepoints, sizeof(void**)*j );
			ns->defines.methods = mtd_d;
			ns->defines.tracepoints = tp;
			//__sync_synchronize();
			ns->defines.caps._tracepoints = ns->defines.caps._methods = n;
			//
			n = j + nmtd_s;
		}

		// copy table
		memcpy( &mtd_d[j], &dst->methods[nmtd_d], sizeof(void**) * nmtd_s );

		// set original "tpix"
		for( ; j < n; j++ )
		{
			tp[j] = mtd_d[j]->tpoint;
			tp[j]->idx = j + 1;
		}

		// update lengths
		__sync_fetch_and_add( &dst->method_num, nmtd_s );
		__sync_fetch_and_add( &ns->defines.npoint, nmtd_s );
		__sync_fetch_and_add( &ns->defines.nmethod, nmtd_s );

		// re-build index table of the "dst"
		n = dst->mtdidx_num + nmtd_s;
		mtd_d = (vt_method_struct**)mmp_malloc( 1, &ns->mmphd, sizeof(void**) * n + sizeof(size_t)*2 );  // temporary process
		if( !mtd_d )
			goto LAB_ERROR;
		memcpy( mtd_d, dst->mtd_idx, sizeof(void**)*dst->mtdidx_num );
		memcpy( &mtd_d[dst->mtdidx_num], &dst->methods[nmtd_d], sizeof(void**) * nmtd_s );

		// sort by name
		for( --n; n > 0; n-- )
		{
			for( j=0; j < n; j++ )
			{
				t = strcmp( ((vt_cstr*)mtd_d[j]->name)->cs, ((vt_cstr*)mtd_d[j+1]->name)->cs );
				if( t > 0 || (t == 0 && strcmp(mtd_d[j]->decl, mtd_d[j+1]->decl) > 0) )
				{
					t = (vt_long)mtd_d[j];
					mtd_d[j] = mtd_d[j+1];
					mtd_d[j+1] = (vt_method_struct*)t;
				}
			}
		}
		// TODO: put old index table into the garbage-bin
		dst->mtd_idx = mtd_d;
		__sync_fetch_and_add( &dst->mtdidx_num, nmtd_s );

		//
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_MT, "copy class: to:%s,  from: %s,  methods: %lld\nAfter class copy, now has totally %lld classes and %lld methods\n",
						  ((vt_cstr*)dst->name)->cs, ((vt_cstr*)src->name)->cs, nmtd_s, ns->defines.nklass, ns->defines.nmethod );

		if( lock )
			BsemGive( ns->lock );
	}

	return dst;

	LAB_ERROR:
	if( lock )
		BsemGive( ns->lock );
	return NULL;
}

//
const vt_class_struct* vt_class_dup( vt_namespace *ns, const vt_class_struct *klass, const char *newname, vt_bool lock )
{
	if( !klass->methods || klass->method_num < 1 )
		return NULL;

	vt_class_struct *new_klass = NULL;

	if( !lock || BsemTake(ns->lock, -1) )
	{
		// check class again(only find in extended classes)
		vt_long m = ns->defines.nklass;
		new_klass = lock ? (vt_class_struct*)vt_class_find3( newname,
											(const vt_class_struct**)&ns->defines.klasses[ns->defines.index._nklass],
											m - ns->defines.index._nklass ) : NULL;
		if( !new_klass )
		{
			// check class table capacity
			vt_class_struct **_klstab = ns->defines.klasses;
			if( m >= ns->defines.caps._klasses )
			{
				// extend class table
				_klstab = (vt_class_struct**)mmp_malloc( 1, &ns->mmphd, sizeof(void**) * (m + 256) + sizeof(size_t)*2 );  // temporary process
				if( !_klstab )
					goto LAB_ERROR;

				// TODO: put old class table into the garbage-bin

				memcpy( _klstab, ns->defines.klasses, sizeof(void**)*m );
				ns->defines.klasses = _klstab;
				//__sync_synchronize();
				ns->defines.caps._klasses = m + 256;
			}

			// allocate new class
			_klstab[m] = new_klass = (vt_class_struct*)mmp_malloc( 1, &ns->mmphd, sizeof(vt_class_struct) );
			if( !new_klass )
				goto LAB_ERROR;

			// setup the new class
			memcpy( new_klass, klass, sizeof(vt_class_struct) );
			new_klass->name = vt_symtab_add( &ns->consts.symb, newname, strlen(newname) );
			if( !new_klass->name )
				goto LAB_ERROR;
			memset( &new_klass->uid, 0, sizeof(new_klass->uid) );  // without uid
			new_klass->method_num = 0;
			new_klass->mtdidx_num = 0;
			new_klass->methods = NULL;
			new_klass->mtd_idx = NULL;
			__sync_fetch_and_add( &ns->defines.nklass, 1 );

			// copy methods
			if( !vt_class_copy( ns, new_klass, klass, FALSE ) )
				goto LAB_ERROR;

			// TODO: re-build index table of classes
		}

		if( lock )
			BsemGive( ns->lock );
	}

	return new_klass;

	LAB_ERROR:
	if( lock )
		BsemGive( ns->lock );
	return NULL;
}


//
int vt_tpidx_set( vt_long new_idx, vt_long old_idx, const vt_namespace *ns, vt_bool lock )
{
	if( !lock || BsemTake(ns->lock, -1) )
	{
		vt_point *pt = ns->defines.tracepoints[old_idx - 1];
		vt_method_struct *mtd = ns->defines.methods[old_idx - 1];

		if( new_idx != old_idx )
		{
			ns->defines.tracepoints[old_idx - 1] = ns->defines.tracepoints[new_idx - 1];
			ns->defines.tracepoints[new_idx - 1] = pt;

			//ns->defines.tracepoints[new_idx - 1];
			pt->idx = new_idx;
			ns->defines.tracepoints[old_idx - 1]->idx = old_idx;

			ns->defines.methods[old_idx - 1] = ns->defines.methods[new_idx - 1];
			ns->defines.methods[new_idx - 1] = mtd;
		}
		pt->cond.tpix_sync = 1;  // "tpix" is ready

		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_MT, "new=%04lld, old=%04lld, %s%s, %s\n",
				new_idx, old_idx,
				((vt_cstr*)mtd->name)->cs, (char*)mtd->decl,
				((vt_cstr*)mtd->klass->name)->cs);

		if( lock )
			BsemGive( ns->lock );
		return 0;
	}
	return -1;
}


// ===================================================================================================================================== //
vt_dict* vt_conf_load_dictionary( HXML hxml, vt_namespace *ns, vt_long *num )
{
	XML_ELEMENT *ele;
	long a, k;
	char *tail;
	xjs_kv_t kv;
	vt_cstr *cs;
	vt_dict *dict = NULL;

	a = xmlGetCurSubEleNum( hxml, "d" );
	if( a > 0 )
	{
		if( a > 0xFFFFFFFF )
			a = 0xFFFFFFFF;  // HAHA...
		*num = 0;
		dict = (vt_dict*)mmp_malloc( 1, &ns->mmphd, sizeof(vt_dict) * a );
		if( dict )
		{
			k = 0;
			ele = xmlGetCurSubEle( hxml, "d", 0 );
			while( k < a && ele )
			{
				if( stricmp(ele->Name, "d") == 0 )
				{
					if( ele->Value )
					{
						if( xjs_seek_keyval_ex( ele->Value, strlen(ele->Value), &kv, '=', FALSE ) == 0 &&
							kv.vtype == XJS_VTYPE_NUMBER )
						{
							cs = vt_symtab_add( &ns->consts.symb, kv.key, kv.klen );
							if( !cs )
								break;
							dict[k].key = cs->cs;
							dict[k].val.l = strtol( kv.val, &tail, 0 );
							k++;
						}
					}
					ele = xmlGetNextElement( hxml );  // next node
				}
			}
			*num = k;
		}
		xmlGetParentElement( hxml );  // back to upper level
	}

	return dict;
}

//
int vt_conf_load_constraints( HXML hxml, vt_namespace *ns )
{
	XML_ELEMENT *ele;
	XML_ATTRIBUTE *attrs[8];
	long n, m[3], a, ll;
	char *tail;
	vt_primitive_constraint **constraints;

	ele = xmlGetCurSubEle( hxml, "Constraints", 0 );
	if( ele )
	{
		m[0] = xmlGetCurSubEleNum( hxml, "Range" );
		m[1] = xmlGetCurSubEleNum( hxml, "Enum" );
		m[2] = xmlGetCurSubEleNum( hxml, "Regu" );
		if( (a=m[0] + m[1] + m[2]) < 1 )
		{
			xmlGetParentElement( hxml );  // back to upper level
			return 0;
		}

		ns->defines.constraints = constraints = (vt_primitive_constraint**)mmp_malloc( 1, &ns->mmphd, sizeof(void**) * a );
		if( !constraints )
			return -1;
		memset( constraints, 0, sizeof(void**) * a );

		n = 0;
		ele = xmlGetSonElement( hxml );
		while( n < a && ele )
		{
			if( stricmp(ele->Name, "Range") == 0 )
			{
				attrs[0] = xmlGetEleAttr( ele, "name" );
				attrs[1] = xmlGetEleAttr( ele, "dtype" );
				attrs[2] = xmlGetEleAttr( ele, "from" );
				attrs[3] = xmlGetEleAttr( ele, "to" );
				attrs[4] = xmlGetEleAttr( ele, "step" );
				if( attrs[0] && attrs[0]->Value &&
					attrs[1] && attrs[1]->Value &&
					attrs[2] && attrs[2]->Value &&
					attrs[3] && attrs[3]->Value &&
					attrs[4] && attrs[4]->Value )
				{
					constraints[n] = (vt_primitive_constraint*)mmp_malloc( 1, &ns->mmphd, sizeof(vt_range) );
					if( !constraints[n] )
						return -1;
					memset( constraints[n], 0, sizeof(vt_range) );

					constraints[n]->name = vt_symtab_add( &ns->consts.symb, attrs[0]->Value, strlen(attrs[0]->Value) );
					if( !constraints[n]->name )
						return -1;
					constraints[n]->choice = VT_CONSTRAINT_CHOICE_RANGE;
					if( vt_dict_getvalue( g_vt_dict_DataType, sizeof(g_vt_dict_DataType) / sizeof(vt_dict), attrs[1]->Value, &ll ) == -1 )
						return -1;
					constraints[n]->range.dtype = (vt_byte)ll;
					switch( ll )
					{
					case VT_TYPE_SI32:
					case VT_TYPE_UI32:
					case VT_TYPE_SI64:
					case VT_TYPE_UI64:
						constraints[n]->range.min.l = strtol( attrs[2]->Value, &tail, 0 );
						constraints[n]->range.max.l = strtol( attrs[3]->Value, &tail, 0 );
						constraints[n]->range.step.l = strtol( attrs[4]->Value, &tail, 0 );
						break;
					case VT_TYPE_FLOAT:
					case VT_TYPE_DOUBLE:
						constraints[n]->range.min.d = strtod( attrs[2]->Value, &tail );
						constraints[n]->range.max.d = strtod( attrs[3]->Value, &tail );
						constraints[n]->range.step.d = strtod( attrs[4]->Value, &tail );
						break;
					default:
						return -1;
					}
					n++;
				}
			}
			else if( stricmp(ele->Name, "Enum") == 0 )
			{
				attrs[0] = xmlGetEleAttr( ele, "name" );
				attrs[1] = xmlGetEleAttr( ele, "comp_key" );

				constraints[n] = (vt_primitive_constraint*)mmp_malloc( 1, &ns->mmphd, sizeof(vt_enumerate) );
				if( !constraints[n] )
					return -1;
				memset( constraints[n], 0, sizeof(vt_enumerate) );

				constraints[n]->name = vt_symtab_add( &ns->consts.symb, attrs[0]->Value, strlen(attrs[0]->Value) );
				if( !constraints[n]->name )
					return -1;
				constraints[n]->choice = VT_CONSTRAINT_CHOICE_ENUM;
				constraints[n]->enumerate.compare = attrs[1] && attrs[1]->Value ? stricmp(attrs[1]->Value, "true") == 0 : 0;
				if( (constraints[n]->enumerate.items = vt_conf_load_dictionary( hxml, ns, (vt_long*)&ll )) )
				{
					constraints[n]->enumerate.nitem = (vt_uint)ll;
#if (VT_DICT_SORT_BY == 2)
					vt_dict_sort_bykey( constraints[n]->enumerate.items, ll );
#else
					vt_dict_sort_byval( constraints[n]->enumerate.items, ll );
#endif
				}
				n++;
			}
			else if( stricmp(ele->Name, "Regu") == 0 )
			{
				attrs[0] = xmlGetEleAttr( ele, "name" );
				attrs[1] = xmlGetEleAttr( ele, "form" );
				if( attrs[0] && attrs[0]->Value &&
					attrs[1] && attrs[1]->Value &&
					ele->Value && ele->Value[0] )
				{
					constraints[n] = (vt_primitive_constraint*)mmp_malloc( 1, &ns->mmphd, sizeof(vt_regularization) + strlen(ele->Value) + 1 );
					if( !constraints[n] )
						return -1;
					constraints[n]->name = vt_symtab_add( &ns->consts.symb, attrs[0]->Value, strlen(attrs[0]->Value) );
					if( !constraints[n]->name )
						return -1;
					constraints[n]->choice = VT_CONSTRAINT_CHOICE_REGU;
					constraints[n]->regular.canon = (vt_byte)atoi( attrs[1]->Value );
					constraints[n]->regular.expr = (void*)constraints[n] + sizeof(vt_regularization);
					strcpy( constraints[n]->regular.expr, ele->Value );
					n++;
				}
			}
			ele = xmlGetNextElement( hxml );  // next node
		}

		ns->defines.nconstraint = n;
		xmlGetParentElement( hxml );  // back to "Constraints"
		xmlGetParentElement( hxml );  // back to upper level
	}
	return 0;
}

//
int vt_conf_load_rules( HXML hxml, vt_namespace *ns )
{
	XML_ELEMENT *ele;
	XML_ATTRIBUTE *attrs[8];
	long n, m, a, b, rule_count, lang, prot;
	vt_rule_lib **rulelibs;
	vt_rule **rules, **_rule;
	int autoload;
	char *ptr;
	const char *lvl_name;


	ele = xmlGetCurSubEle( hxml, "TraceRules", 0 );
	if( ele )
	{
		// different version may choose different level name "Engine" or "Group"
		m = xmlGetCurSubEleNum( hxml, (lvl_name="Engine") );
		if( m < 1 )
		{
			m = xmlGetCurSubEleNum( hxml, (lvl_name="Group") );
			if( m < 1 )
			{
				xmlGetParentElement( hxml );  // back to upper level
				return 0;
			}
		}

		ns->defines.rulelibs = (vt_rule_lib**)mmp_malloc( 1, &ns->mmphd, sizeof(void**) * m );
		if( !ns->defines.rulelibs )
			return -1;

		rulelibs = ns->defines.rulelibs;
		memset( rulelibs, 0, sizeof(void**) * m );
		rules = NULL;
		rule_count = 0;
		n = 0;
		ele = xmlGetCurSubEle( hxml, lvl_name, 0 );
		while( n < m && ele )
		{
			if( stricmp(ele->Name, lvl_name) == 0 )
			{
				rulelibs[n] = (vt_rule_lib*)mmp_malloc( 1, &ns->mmphd, sizeof(vt_rule_lib) );
				if( !rulelibs[n] )
					return -1;
				memset( rulelibs[n], 0, sizeof(vt_rule_lib) );

				// Group's attributes
				attrs[0] = xmlGetEleAttr( ele, "id" );
				attrs[1] = xmlGetEleAttr( ele, "lib" );
				attrs[2] = xmlGetEleAttr( ele, "lang" );
				attrs[3] = xmlGetEleAttr( ele, "loadmode" );
				attrs[4] = xmlGetEleAttr( ele, "prot" );
				if( !attrs[1] || !attrs[1]->Value || !attrs[1]->Value[0] )
					return -1;

				if( stricmp(attrs[1]->Value, "@INTRINSIC") == 0 )
					rulelibs[n]->path = NULL;
				else
				{
					rulelibs[n]->path = vt_symtab_add( &ns->consts.symb, attrs[1]->Value, strlen(attrs[1]->Value) );
					if( !rulelibs[n]->path )
						return -1;
				}

				rulelibs[n]->id = attrs[0] && attrs[0]->Value ? (vt_uint)atoi(attrs[0]->Value) : n+1;

				if( !attrs[2] || !attrs[2]->Value || !attrs[2]->Value[0] )
					rulelibs[n]->lang = ns->lang;
				else
					rulelibs[n]->lang = vt_dict_getvalue( g_vt_dict_Language, sizeof(g_vt_dict_Language) / sizeof(vt_dict),
														  attrs[2]->Value, &lang ) == -1 ? ns->lang : (vt_lang)lang;

				autoload = attrs[3] && attrs[3]->Value ? stricmp( attrs[3]->Value, "loadtime" ) == 0 : 0;

				if( !attrs[4] || !attrs[4]->Value || !attrs[4]->Value[0] )
					rulelibs[n]->prot = VTRACE_PROT_UNKNOWN;
				else
					rulelibs[n]->prot = vt_dict_getvalue( g_vt_dict_Protocol, sizeof(g_vt_dict_Protocol) / sizeof(vt_dict),
														  attrs[4]->Value, &prot ) == -1 ? VTRACE_PROT_UNKNOWN : (vt_proto)prot;

				// to load "tr"s of "group"
				a = xmlGetCurSubEleNum( hxml, "tr" );
				if( a > 0 )
				{
					ptr = mmp_realloc( 1, &ns->mmphd, rules, sizeof(void**) * (rule_count + a) );
					if( !ptr )
						return -1;
					rules = (vt_rule**)ptr;
					_rule = &rules[rule_count];
					memset( _rule, 0, sizeof(void**) * a );
					rulelibs[n]->begptr = _rule;
					rulelibs[n]->endptr = &_rule[a-1];

					//
					b = 0;
					ele = xmlGetCurSubEle( hxml, "tr", 0 );
					while( b < a && ele )
					{
						if( stricmp(ele->Name, "tr") == 0 )
						{
							_rule[b] = (vt_rule*)mmp_malloc( 1, &ns->mmphd, sizeof(vt_rule) );
							if( !_rule[b] )
								return -1;
							memset( _rule[b], 0, sizeof(vt_rule) );

							// tr's attributes
							attrs[0] = xmlGetEleAttr( ele, "num" );
							attrs[1] = xmlGetEleAttr( ele, "alias" );
							_rule[b]->num = strtol( attrs[0]->Value, &ptr, 0 );
							if( ptr && ptr > attrs[0]->Value && *ptr == 0 )
							{
								_rule[b]->lib = (vt_refer)rulelibs[n];
								_rule[b]->alias = attrs[1] && attrs[1]->Value ?
										(vt_cstr*)vt_symtab_add( &ns->consts.symb, attrs[1]->Value, strlen(attrs[1]->Value) ) : NULL;
								rule_count++;
							}
							b++;
						}
						ele = xmlGetNextElement( hxml );  // next "tr"
					}
					xmlGetParentElement( hxml );  // back to the "Group"
				}

				//
				if( autoload )
				{
					// try to load the library
					vt_rulelib_load( rulelibs[n] );  // whatever success or failure
				}

				ns->defines.nrulelib = ++n;
			}
			ele = xmlGetNextElement( hxml );  // next "Group"
		}
		//ns->defines.nrulelib = n;
		ns->defines.nrule = rule_count;
		ns->defines.rules = rules;
		xmlGetParentElement( hxml );  // back to the "TraceRules"
		xmlGetParentElement( hxml );  // back to upper level
	}
	return 0;
}

//
int vt_conf_load_methods( HXML hxml, vt_namespace *ns )
{
	XML_ELEMENT *ele, *eles[8];
	XML_ATTRIBUTE *attrs[8];
	long n, _n, m, a, b, _b, ll, ln, lang;
	char *cch[4];
	vt_cstr *cs;
	vt_value val;
	vt_class_struct **klass, *curr_klass;
	vt_method_struct **method;
	vt_data_class **data;
	vt_point **tracepoint, *tp;

	vt_bool normalized, check_args;
	char *decl_new = NULL;
	int decl_newsize = 0;

	char *klass_name;
	long klass_name_len;
	EVP_MD_CTX md_ctx;
	const EVP_MD *md = g_vt_dds_md;  //EVP_md5();  // must, 16bytes
	EVP_MD_CTX_init( &md_ctx );



	ele = xmlGetCurSubEle( hxml, "Methods", 0 );
	if( ele )
	{
		attrs[0] = xmlGetEleAttr( ele, "normalized" );
		normalized = attrs[0] ? stricmp(attrs[0]->Value, "true") == 0 : FALSE;  // normalized declaration

		attrs[0] = xmlGetEleAttr( ele, "check_args" );
		check_args = attrs[0] ? stricmp(attrs[0]->Value, "true") == 0 : TRUE;  // check arguments(number, type)

		m = xmlGetCurSubEleNum( hxml, "Class" );
		if( m < 1 )
			return -1;

		// class list may be duplicated, and will be merged when loading methods
		ns->defines.klasses = (vt_class_struct**)mmp_malloc( 1, &ns->mmphd, sizeof(void**) * m + sizeof(size_t)*2 );  // temporary process
		if( !ns->defines.klasses )
			return -1;

		klass = ns->defines.klasses;
		memset( klass, 0, sizeof(void*) * m );

		ns->defines.nklass = 0;
		ns->defines.nmethod = 0;
		ns->defines.npoint = 0;

		n = _n = 0;
		ele = xmlGetCurSubEle( hxml, "Class", 0 );
		while( n < m && ele )
		{
			if( stricmp(ele->Name, "Class") == 0 )
			{
				// Class's attr: name
				attrs[0] = xmlGetEleAttr( ele, "name" );
				attrs[1] = xmlGetEleAttr( ele, "lang" );
				if( !attrs[0] || !attrs[1] )
					return -1;

				p_strtrim( attrs[0]->Value );
				p_strtrim( attrs[1]->Value );

				if( !attrs[0] || !attrs[0]->Value || !attrs[0]->Value[0] ) //|| !attrs[1] || !attrs[1]->Value )
					return -1;

				if( !attrs[1] || !attrs[1]->Value || !attrs[1]->Value[0] )
					lang = ns->lang;
				else if( vt_dict_getvalue( g_vt_dict_Language, sizeof(g_vt_dict_Language) / sizeof(vt_dict), attrs[1]->Value, &lang ) == -1 )
					return -1;

				ll = xmlUnescape( attrs[0]->Value, attrs[0]->Value, -1, TRUE );
				attrs[0]->Value[ll] = 0;

				vt_decl_check( lang, attrs[0]->Value, ll ); // check name according to specified language
				klass_name = attrs[0]->Value;
				klass_name_len = ll;

				// may be duplicated, try to match the class that has loaded before
				for( ll=_n-1; ll>=0 && strcmp(((vt_cstr*)klass[ll]->name)->cs, klass_name)!=0; ll-- );
				if( ll >= 0 )
				{
					// duplicated, merged it
					curr_klass = klass[ll];
				}
				else
				{
					// new one
					klass[_n] = curr_klass = (vt_class_struct*)mmp_malloc( 1, &ns->mmphd, sizeof(vt_class_struct) );
					if( !curr_klass )
						return -1;
					memset( curr_klass, 0, sizeof(vt_class_struct) );

					curr_klass->name = vt_symtab_add( &ns->consts.symb, klass_name, klass_name_len );  // (ll=strlen(attrs[0]->Value))
					if( !curr_klass->name )
						return -1;

					// make a digest sign of class name
					if( !VT_MAKE_DDS1( &md_ctx, md, klass_name, klass_name_len, curr_klass->uid.b, NULL ) )
					{
						_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "failed to make a DDS for class name \"%s\"\n", attrs[0]->Value );
						return -1;
					}

					_n++;
					curr_klass->lang = lang;
				}

				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// to load "m"s of "Class"
				a = xmlGetCurSubEleNum( hxml, "m" );
				if( a > 0 )
				{
					// merge method table if possible
					b = ALIGN_SIZE(a+curr_klass->method_num, 64);
					method = (vt_method_struct**)mmp_realloc( 1, &ns->mmphd, curr_klass->methods,
															  sizeof(void**) * b + sizeof(size_t)*2 );  // temporary process
					if( !method )
						return -1;
					curr_klass->methods = method;
					method = &method[curr_klass->method_num];
					memset( method, 0, sizeof(void*) * a );

					b = _b = 0;
					eles[0] = xmlGetCurSubEle( hxml, "m", 0 );
					while( _b < a && eles[0] )
					{
						if( stricmp(eles[0]->Name, "m") == 0 )
						{
							if( !method[b] )
							{
								method[b] = (vt_method_struct*)mmp_malloc( 1, &ns->mmphd, sizeof(vt_method_struct) );
								if( !method[b] )
									return -1;
								memset( method[b], 0, sizeof(vt_method_struct) );
							}

							attrs[0] = xmlGetEleAttr( eles[0], "id" );	  // replaced by digest sign
							attrs[1] = xmlGetEleAttr( eles[0], "decl" );  // necessary
							attrs[2] = xmlGetEleAttr( eles[0], "index" );
							attrs[3] = xmlGetEleAttr( eles[0], "op" );
							attrs[4] = xmlGetEleAttr( eles[0], "group" );
							if( attrs[1] && attrs[1]->Value && attrs[1]->Value[0] &&
								attrs[4] && attrs[4]->Value && attrs[4]->Value[0] )
							{
								p_strtrim( attrs[1]->Value );
								p_strtrim( attrs[4]->Value );
								if( attrs[3] )
									p_strtrim( attrs[3]->Value );

								if( vt_dict_getvalue( g_vt_dict_OperatorSet, sizeof(g_vt_dict_OperatorSet) / sizeof(vt_dict), attrs[4]->Value, &ll ) != -1 )
								{
									method[b]->opset = (vt_word)ll;

									//
									ll = xmlUnescape( attrs[1]->Value, attrs[1]->Value, -1, TRUE );
									attrs[1]->Value[ll] = 0;

									// check declaration according to specified language
									vt_decl_check( lang, attrs[1]->Value, ll );  // TODO: enhance

									// normalize declaration
									ln = normalized ? 0 : vt_decl_norm( lang, attrs[1]->Value, ll, &decl_new, &decl_newsize );
									if( ln < 0 )
									{
										_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR,
												"wrong method declaration \"%s\" of the class \"%s\"\n", attrs[1]->Value, ((vt_cstr*)curr_klass->name)->cs );
										return -1;
									}
									else if( ln > 0 )
									{
										ll = ln;
										cch[2] = decl_new;
									}
									else
										cch[2] = attrs[1]->Value;

									cs = vt_symtab_add( &ns->consts.symb, NULL, ll + 8 );  // allocate a buffer for vt_decl_split
									if( !cs )
										return -1;

									cch[0] = vt_decl_split( lang, cch[2], &cch[1], &cs->cs, NULL );

									// to eliminate duplication if possible
									if( vt_method_find3(cs->cs, cch[1], (const vt_method_struct**)curr_klass->methods, curr_klass->method_num+b) )
									{
										vt_symtab_del( &ns->consts.symb, cs );
										goto LAB_NEXT_MTD;
									}

									method[b]->name = cs;     // as vt_cstr*
									method[b]->decl = cch[1]; // as char*

									// make a digest sign of method sign
									if( !VT_MAKE_DDS3( &md_ctx, md, cs->cs, strlen(cs->cs),
											cch[1], strlen(cch[1]), klass_name, klass_name_len, method[b]->uid.b, NULL ) )
									{
										_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "failed to make a DDS for method declaration\"%s%s\"\n", cs->cs, cch[1] );
										return -1;
									}
									//
									//method[b]->pos = attrs[2]&&attrs[2]->Value ? strtol(attrs[2]->Value, &cch[0], 0) : 0;
									//method[b]->idx = attrs[2]&&attrs[2]->Value ? strtol(attrs[2]->Value, &cch[0], 0) : 0;

									if( !attrs[3] || !attrs[3]->Value ||
										vt_dict_getvalue( g_vt_dict_OperatorType, sizeof(g_vt_dict_OperatorType) / sizeof(vt_dict), attrs[3]->Value, &ll ) == -1 )
										method[b]->inop = VTRACE_INOP_ANY;
									else
										method[b]->inop = (vt_word)ll;
									//
									/*if( !attrs[4] || !attrs[4]->Value ||
										vt_dict_getvalue( g_vt_dict_OperatorSet, sizeof(g_vt_dict_OperatorSet) / sizeof(vt_dict), attrs[4]->Value, &ll ) == -1 )
										method[b]->opset = VTRACE_OPSET_PROPAGATE | VTRACE_OPSET_DEADZONE;  // VTRACE_OPSET_DISABLED
									else
										method[b]->opset = (vt_word)ll;
									*/
									if( method[b]->opset & VTRACE_OPSET_ESCAPED )
										method[b]->inop = VTRACE_INOP_ESCAPE;

									// to load "p"s of "m"
									long h, p = xmlGetCurSubEleNum( hxml, "p" );
									if( p > 0 )
									{
										if( p > VT_METHOD_ARG_MAX )
											p = VT_METHOD_ARG_MAX;

										method[b]->arglst.args = (vt_data_class**)mmp_malloc( 1, &ns->mmphd, sizeof(void**) * p );
										if( !method[b]->arglst.args )
											return -1;

										data = method[b]->arglst.args;
										memset( data, 0, sizeof(void**)*p );

										//
										int typ, sln;
										cch[3] = method[b]->decl;
										h = 0;
										eles[1] = xmlGetCurSubEle( hxml, "p", 0 );
										while( h < p && eles[1] )
										{
											if( stricmp(eles[1]->Name, "p") == 0 )
											{
												data[h] = (vt_data_class*)mmp_malloc( 1, &ns->mmphd, sizeof(vt_data_class) );  // NOT vt_dataclass_node
												if( !data[h] )
													return -1;
												memset( data[h], 0, sizeof(vt_data_class) );

												attrs[0] = xmlGetEleAttr( eles[1], "id" );
												attrs[1] = xmlGetEleAttr( eles[1], "name" );
												attrs[2] = xmlGetEleAttr( eles[1], "type" );  // necessary
												attrs[3] = xmlGetEleAttr( eles[1], "mod" );   // necessary
												attrs[4] = xmlGetEleAttr( eles[1], "depend" );
												attrs[5] = xmlGetEleAttr( eles[1], "constrain" );
												if( attrs[2] && attrs[2]->Value ) //&& attrs[2]->Value[0] )
												{
													if( attrs[1] )
														p_strtrim( attrs[1]->Value );
													p_strtrim( attrs[2]->Value );

													if( attrs[2]->Value[0] )
													{
														ll = xmlUnescape( attrs[2]->Value, attrs[2]->Value, -1, TRUE );
														attrs[2]->Value[ll] = 0;
														vt_decl_check( lang, attrs[2]->Value, ll ); // check sign according to specified language

														if( !normalized && decl_newsize < ll + 8 )
														{
															decl_new = realloc( decl_new, (decl_newsize=ll + 1024) );
															if( !decl_new )
																return -1;
														}

														ln = normalized ? 0 : vt_type_norm( lang, attrs[2]->Value, ll, decl_new );
														if( ln > 0 )
														{
															cch[1] = decl_new;
															ll = ln;
														}
														else
															cch[1] = attrs[2]->Value;
													}
													else
													{
														cch[1] = "V";
														ll = 1;
													}

													data[h]->orgtype = vt_symtab_add( &ns->consts.symb, cch[1], ll );
													if( !data[h]->orgtype )
														return -1;

													data[h]->type = vt_type_convert( lang, cch[1], ll, &data[h]->ndim, &data[h]->shape );
													if( data[h]->ndim > 0 )
														data[h]->type = VT_TYPE_CLASS;  // all array will be treated as an object
													else if( data[h]->type == VT_TYPE_FLOAT || data[h]->type == VT_TYPE_DOUBLE )
														method[b]->arglst.fpnum++;

													data[h]->uid.low = attrs[0] && attrs[0]->Value ? strtoul( attrs[0]->Value, &cch[0], 0 ) : 0;

													data[h]->name = attrs[1] && attrs[1]->Value ?
															vt_symtab_add( &ns->consts.symb, attrs[1]->Value, strlen(attrs[1]->Value) ) : &g_vt_cstr_null;

													if( check_args )
													{
														// try to correct argument definition
														if( h == 0 && stricmp(((vt_cstr*)data[h]->name)->cs, "this") == 0 )
															val.i = 0;
														else if( h+1 == p && stricmp(((vt_cstr*)data[h]->name)->cs, "return") == 0 )
															val.i = -1;
														else
															val.i = h + !h;

														// get one argument
														typ = vt_decl_get_1arg( lang, cch[3], val.i, &data[h]->ndim, &sln );
														if( typ == -1 || typ == -2 )
														{
															if( typ == -1 || val.i != -1 )  // the "return" maybe doesn't represented in declaration string
															{
																printf( "%s: bad declaration of method \"%s%s\", \"%s\"\n", __FUNCTION__,
																	((vt_cstr*)method[b]->name)->cs, (char*)method[b]->decl, ((vt_cstr*)curr_klass->name)->cs );
																//return -1;
																data[h]->type = VT_TYPE_NULL;  // ignored
															}
														}
														else
														{
															if( data[h]->ndim > 0 )
																typ = VT_TYPE_CLASS;  // all array will be treated as an object

															if( typ > VT_TYPE_NULL && data[h]->type != typ )
															{
																printf( "%s: inconsistent type of argument(%ld) of method \"%s%s\", \"%s\"\n", __FUNCTION__,
																		h, ((vt_cstr*)method[b]->name)->cs, (char*)method[b]->decl, ((vt_cstr*)curr_klass->name)->cs );
																data[h]->type = typ;
															}
														}
														cch[3] += sln;
													}

													//
													data[h]->mod.flags = attrs[3] && attrs[3]->Value ? strtoul( attrs[3]->Value, &cch[0], 0 ) : 0x0f;  //0
													if( (data[h]->mod.flags & 0x0f) == 0x0f )  // data[h]->mod.flags == 0 )
													{
														// incorrect modifier
														// adopting default modifier
														if( (method[b]->opset & VTRACE_OPSET_SOURCE) )
														{
															data[h]->mod.flags = VT_MOD_R;
														}
														else
															data[h]->mod.flags = 0;
	#if 0
														/*else if( (method[b]->opset & VTRACE_OPSET_TARGET) )
														{
															data[h]->mod.flags = VT_MOD_W;
														}*/
														else // if( (method[b]->opset & VTRACE_OPSET_PROPAGATE) )
														{
															if( stricmp( ((vt_cstr*)data[h]->name)->cs, "return" ) == 0 )
																data[h]->mod.flags = VT_MOD_W;// | VT_MOD_R;
															/*else if( stricmp( ((vt_cstr*)data[h]->name)->cs, "this" ) == 0 )
																data[h]->mod.flags = VT_MOD_R;*/
															else
																data[h]->mod.flags = VT_MOD_R;
														}
	#endif

														data[h]->mod.flags |= stricmp( ((vt_cstr*)data[h]->name)->cs, "return" ) == 0
																					? VT_MOD_W : VT_MOD_R;
													}
													else
													{
														// correct modifier(hypothesized)
														if( (method[b]->opset & (VTRACE_OPSET_SOURCE|VTRACE_OPSET_TARGET)) )
														{
															//data[h]->mod.flags |= VT_MOD_R; // always with readable

															if( !data[h]->mod.w && stricmp( ((vt_cstr*)data[h]->name)->cs, "return" ) == 0 )
																data[h]->mod.flags |= VT_MOD_W;
														}
													}

													// 2023-03-13
													if( data[h]->type != VT_TYPE_CLASS ) // VT_TYPE_IS_PREMITIVE(data[h]->type)
														data[h]->mod.flags |= VT_MOD_O;  // treat as optional if it is not structure

													//
													data[h]->depend = attrs[4] && attrs[4]->Value ?
															vt_symtab_add( &ns->consts.symb, attrs[4]->Value, strlen(attrs[4]->Value) ) : NULL;
													data[h]->constraint = NULL;  // TODO:
												}

												h++;
											}
											eles[1] = xmlGetNextElement( hxml );  // next "p"
										}
										method[b]->arglst.argn = h;
										method[b]->klass = curr_klass;
										xmlGetParentElement( hxml ); // back to the "m"
									}

									// to load "t" of "m"
									//method[b]->extarg[0 = ]method[b]->extarg[1] = NULL;
									method[b]->tpoint = tp = (vt_point*)mmp_malloc( 1, &ns->mmphd, sizeof(vt_point) );  // MUST exists, NOT vt_point_node
									if( !tp )
										return -1;

									eles[2] = xmlGetCurSubEle( hxml, "t", 0 );
									if( eles[2] )
									{
										attrs[0] = xmlGetEleAttr( eles[2], "stage" );
										attrs[1] = xmlGetEleAttr( eles[2], "cond" );
										attrs[2] = xmlGetEleAttr( eles[2], "rule" );
										attrs[3] = xmlGetEleAttr( eles[2], "kind" );
										attrs[4] = xmlGetEleAttr( eles[2], "mode" );
										attrs[5] = xmlGetEleAttr( eles[2], "sot_max" );

										//tp->magic;
										tp->idx = b + 1;  //method[b]->id;
										tp->stage = attrs[0] && attrs[0]->Value ? (vt_byte)strtoul( attrs[0]->Value, &cch[0], 0 ) : VTRACE_STAGE_LEAVE;
										tp->cond.cond = attrs[1] && attrs[1]->Value ? strtoul( attrs[1]->Value, &cch[0], 0 ) : -1;

										if( (method[b]->opset & VTRACE_OPSET_TARGET)		/* temporary constrain */
											&& ns->defines.rules && attrs[2] && attrs[2]->Value )
										{
											cch[0] = _is_num_or_cstr( attrs[2]->Value, &val );
											tp->rule = vt_rule_match( val.l, cch[0], (const vt_rule**)ns->defines.rules, ns->defines.nrule );
										}
										else
											tp->rule = NULL;

										tp->kind = attrs[3] && attrs[3]->Value ? (vt_byte)strtoul( attrs[3]->Value, &cch[0], 0 ) : 0;
										if( tp->kind == 0 && (method[b]->opset & VTRACE_OPSET_TARGET) )
											tp->kind |= VTRACE_KIND_KTP;  // VTRACE_KIND_CRIT_KTP,  compatible with older system

										if( attrs[4] && attrs[4]->Value &&
											vt_dict_getvalue( g_vt_dict_TraceMode, sizeof(g_vt_dict_TraceMode) / sizeof(vt_dict), attrs[4]->Value, &ll ) == -1 )
											tp->mode = (vt_byte)ll;
										else
											tp->mode = VTRACE_MODE_IAST;

										tp->sot_max_tp = attrs[5] && attrs[5]->Value ? strtoul( attrs[5]->Value, &cch[0], 0 ) : -1;
										if( tp->sot_max_tp == (vt_size)-1 && (method[b]->opset & VTRACE_OPSET_DEADZONE) )
											tp->sot_max_tp = ns->runconf.aux.sot_max_tp;  // compatible with older system

										tp->level = 0;
										tp->target = method[b];

										xmlGetParentElement( hxml ); // back to the "m"
									}
									else
									{
										memset( tp, 0, sizeof(vt_point) );
									}
									///////////////////////////////////////////////////////////////////////////////////////////////////////////////
									b++;
								}
							}
							LAB_NEXT_MTD:
							_b++;
						}
						eles[0] = xmlGetNextElement( hxml );  // next "m"
					}
					curr_klass->method_num += b;
					ns->defines.nmethod += b;
					xmlGetParentElement( hxml ); // back to the "Class"
				}
				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				n++;
			}
			ele = xmlGetNextElement( hxml );  // next "Class"
		}
		ns->defines.nklass += _n;
		xmlGetParentElement( hxml ); // back to the "Methods"
		xmlGetParentElement( hxml ); // back to the "NameSpace"
	}

	memset( &ns->defines.index, 0, sizeof(ns->defines.index) );

	if( (ns->defines.npoint = ns->defines.nmethod) > 0 )
	{
		// setup global table of all methods and trace points
		a = ns->defines.npoint;
		ns->defines.tracepoints = tracepoint = (vt_point**)mmp_malloc( 1, &ns->mmphd, sizeof(void**) * a * 2 + sizeof(size_t)*2 );  // temporary process
		if( !tracepoint )
			return -1;

		ns->defines.methods = (vt_method_struct**)&tracepoint[a];

		klass = ns->defines.klasses;
		m = ns->defines.nklass;
		ll = 0;
		for( n=0; n < m; n++ )
		{
			method = klass[n]->methods;
			a = ns->defines.klasses[n]->method_num;
			for( b=0; b < a; b++, ll++ )
			{
				ns->defines.methods[ll] = method[b];
				tracepoint[ll] = method[b]->tpoint;
				tracepoint[ll]->idx = ll + 1;  // MUST, a consecutive number, [1, point_max]
			}
		}

		// build indexes
		ns->defines.index.klass = vt_class_struct_build_index( (const vt_class_struct**)ns->defines.klasses,
										ns->defines.nklass, 0, &ns->mmphd );  // ns->defines.nklass+256
		if( ns->defines.index.klass )
		{
			ns->defines.index.klass2 = &ns->defines.index.klass[ns->defines.nklass];
			ns->defines.index._nklass = ns->defines.nklass;

			//
			if( vt_method_point_build_index( (vt_class_struct*const*)ns->defines.index.klass,
											 ns->defines.nklass,
											 (const vt_method_struct**)ns->defines.methods,
											 (const vt_point**)ns->defines.tracepoints,
											 ll,
											 0,  /* ll+1024, */
											 &ns->defines.index.method,
											 &ns->defines.index.point,
											 &ns->mmphd ) != -1 )
			{
				ns->defines.index.method2 = &ns->defines.index.method[ll];
				ns->defines.index.point2 = &ns->defines.index.point[ll];
				ns->defines.index._nmethod = ns->defines.nmethod;
				ns->defines.index._npoint = ns->defines.npoint;
			}
		}

#if 0
		// show table
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_MT, "Table of methods & trace-point:\n"
				"============================================================================================================================\n" );
#if 1
		m = ns->defines.nmethod;
		method = ns->defines.methods;
		for( b=0; b < m; b++ )
			_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_INFO, "%s%s, %s\n",
					((vt_cstr*)method[b]->name)->cs, (char*)method[b]->decl,
					((vt_cstr*)method[b]->klass->name)->cs);
		/*
		m = ns->defines.nmethod;
		method = ns->defines.index.method ? ns->defines.index.method : ns->defines.methods;
		for( b=0; b < m; b++ )
			_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_INFO, "%ld, %u, %04lld, %p:%p, %s%s, %s\n",
					method[b]->klass->mtd_idx - method, method[b]->klass->method_num,
					method[b]->tpoint?((vt_point*)method[b]->tpoint)->idx:-1,
					(void*)method[b]->uid.high, (void*)method[b]->uid.low,
					((vt_cstr*)method[b]->name)->cs, (char*)method[b]->decl,
					((vt_cstr*)method[b]->klass->name)->cs);*/
#else
		m = ns->defines.nklass;
		klass = ns->defines.index.klass ? ns->defines.index.klass : ns->defines.klasses;
		for( n=0; n < m; n++ )
		{
			method = klass[n]->mtd_idx ? klass[n]->mtd_idx : klass[n]->methods;
			a = klass[n]->method_num;

			_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_MT, ">>> %ld, %u, Class \"%s\"\n",
					method - ns->defines.index.method, klass[n]->method_num,
					((vt_cstr*)klass[n]->name)->cs );
			for( b=0; b < a; b++ )
			{
				_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_MT, "%04lld, %p:%p, %s%s\n",
						method[b]->tpoint?((vt_point*)method[b]->tpoint)->idx:-1,
						(void*)method[b]->uid.high, (void*)method[b]->uid.low,
						((vt_cstr*)method[b]->name)->cs, (char*)method[b]->decl );
			}
		}
#endif
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_DEBUG, "Total: %lld methods of %lld classes\n"
				"============================================================================================================================\n",
				ns->defines.nmethod, ns->defines.nklass );
#endif
	}

	// temporary process(for dynamically growth)
	ns->defines.caps._klasses = ns->defines.nklass;
	ns->defines.caps._methods = ns->defines.nmethod;
	ns->defines.caps._tracepoints = ns->defines.npoint;

#if 1
	if( ns->defines.klasses )
	{
		klass = mmp_realloc( 1, &ns->mmphd, ns->defines.klasses, sizeof(void**) * (a=ns->defines.nklass+512) + sizeof(size_t)*2 );
		if( klass )
		{
			ns->defines.klasses = klass;
			ns->defines.caps._klasses = a;
		}
	}

	if( ns->defines.tracepoints )
	{
		tracepoint = mmp_realloc( 1, &ns->mmphd, ns->defines.tracepoints, sizeof(void**) * (a=ns->defines.npoint+2048)*2 + sizeof(size_t)*2 );
		if( tracepoint )
		{
			memcpy( &tracepoint[a], ns->defines.methods, sizeof(void**)*ns->defines.npoint );
			ns->defines.tracepoints = tracepoint;
			ns->defines.methods = (vt_method_struct**)&tracepoint[a];
			ns->defines.caps._methods = a;
			ns->defines.caps._tracepoints = a;
		}
	}
#endif

	//
	if( decl_new )
		free( decl_new );

	EVP_MD_CTX_cleanup( &md_ctx );

	return 0;
}


//
int vt_conf_load_runconf( HXML hxml, vt_namespace *ns )
{
	XML_ELEMENT *ele;
	XML_ATTRIBUTE *attrs[8];
	long ll;
	char *cc;

	ele = xmlGetCurSubEle( hxml, "RunConf", 0 );
	if( ele )
	{
		ele = xmlGetCurSubEle( hxml, "TraceIntensity", 0 );
		if( ele )
		{
			ns->runconf.caps.f_intensity = ele->Value ? atoi(ele->Value) : VTRACE_CAPA_INTENSITY_NORMAL;
			xmlGetParentElement( hxml );
		}

		//
		ele = xmlGetCurSubEle( hxml, "TraceMode", 0 );
		if( ele )
		{
			if( vt_dict_getvalue( g_vt_dict_TraceMode, sizeof(g_vt_dict_TraceMode) / sizeof(vt_dict), ele->Value, &ll ) == 0 )
			{
				ns->runconf.mode = ll & 0xff;
				ns->runconf.caps.f_en = 1;
			}
			xmlGetParentElement( hxml );
		}

		//
		ele = xmlGetCurSubEle( hxml, "Capacities", 0 );
		if( ele )
		{
			ele = xmlGetCurSubEle( hxml, "sot_max", 0 );
			if( ele )
			{
				ns->runconf.caps.sot_max = strtoul( ele->Value, &cc, 0 );
				xmlGetParentElement( hxml );
			}

			//
			ele = xmlGetCurSubEle( hxml, "mem_max", 0 );
			if( ele )
			{
				ns->runconf.caps.mem_max = strtoul( ele->Value, &cc, 0 );
				xmlGetParentElement( hxml );
			}
			//
			ele = xmlGetCurSubEle( hxml, "dsk_max", 0 );
			if( ele )
			{
				ns->runconf.caps.dsk_max = strtoul( ele->Value, &cc, 0 );
				xmlGetParentElement( hxml );
			}
			//
			ele = xmlGetCurSubEle( hxml, "cputm_max", 0 );
			if( !ele )
				ele = xmlGetCurSubEle( hxml, "live_max", 0 );
			if( ele )
			{
				ns->runconf.caps.live_max = strtoul( ele->Value, &cc, 0 );
				xmlGetParentElement( hxml );
			}
			//
			ele = xmlGetCurSubEle( hxml, "trace_tmo", 0 );
			if( ele )
			{
				ns->runconf.caps.exec_tmo = strtoul( ele->Value, &cc, 0 );
				xmlGetParentElement( hxml );
			}
			//
			ele = xmlGetCurSubEle( hxml, "fd_max", 0 );
			if( ele )
			{
				ns->runconf.caps.fd_max = strtoul( ele->Value, &cc, 0 );
				xmlGetParentElement( hxml );
			}
			//
			ele = xmlGetCurSubEle( hxml, "freq", 0 );
			if( ele )
			{
				ns->runconf.caps.freq = strtoul( ele->Value, &cc, 0 );
				xmlGetParentElement( hxml );
			}
			//
			ele = xmlGetCurSubEle( hxml, "doj_dec", 0 );
			if( ele )
			{
				ns->runconf.caps.doj_dec = (typeof(ns->runconf.caps.doj_dec))strtoul( ele->Value, &cc, 0 );
				//if( ns->runconf.caps.doj_dec > VT_DATOBJ_JOINT_DECAY_MAX )
				//	ns->runconf.caps.doj_dec = VT_DATOBJ_JOINT_DECAY_MAX;
				xmlGetParentElement( hxml );
			}
			//
			ele = xmlGetCurSubEle( hxml, "joint_max", 0 );
			if( ele )
			{
				ns->runconf.aux.joint_max = strtoul( ele->Value, &cc, 0 );
				xmlGetParentElement( hxml );
			}
			if( ns->runconf.aux.joint_max == 0 )
				ns->runconf.aux.joint_max = -1;
			//
			ele = xmlGetCurSubEle( hxml, "report_max", 0 );
			if( ele )
			{
				ns->runconf.caps.rep_max = (int)strtol( ele->Value, &cc, 0 );
				xmlGetParentElement( hxml );
			}
			//
			ele = xmlGetCurSubEle( hxml, "sot_max_tp", 0 );
			if( ele )
			{
				ns->runconf.aux.sot_max_tp = strtoul( ele->Value, &cc, 0 );
				xmlGetParentElement( hxml );
			}
			if( ns->runconf.aux.sot_max_tp == 0 )
				ns->runconf.aux.sot_max_tp = -1;
			//
			ele = xmlGetCurSubEle( hxml, "depend_deep_max", 0 );
			if( ele )
			{
				ns->runconf.aux.depend_deep_max = (vt_uint)strtol( ele->Value, &cc, 0 );
				xmlGetParentElement( hxml );
			}
			if( ns->runconf.aux.depend_deep_max == 0 )
				ns->runconf.aux.depend_deep_max = -1;
			//
			ele = xmlGetCurSubEle( hxml, "diffuse_wide_max", 0 );
			if( ele )
			{
				ns->runconf.aux.diffuse_wide_max = (vt_uint)strtol( ele->Value, &cc, 0 );
				xmlGetParentElement( hxml );
			}
			if( ns->runconf.aux.diffuse_wide_max == 0 )
				ns->runconf.aux.diffuse_wide_max = -1;
			//
			ele = xmlGetCurSubEle( hxml, "sot_filter_a", 0 );
			if( ele )
			{
				ns->runconf.aux.sot_filter_a = atoi( ele->Value );
				attrs[0] = xmlGetEleAttr( ele, "distance" );
				ns->runconf.aux.sot_filter_dist = attrs[0] ? atoi( attrs[0]->Value ) : 16;
				xmlGetParentElement( hxml );
			}

			xmlGetParentElement( hxml );
		}

		//
		ele = xmlGetCurSubEle( hxml, "EventRestrict", 0 );
		if( ele )
		{
			attrs[0] = xmlGetEleAttr( ele, "intv" );
			attrs[1] = xmlGetEleAttr( ele, "max" );
			ns->runconf.evtres.intv = attrs[0] ? (vt_time)strtol( attrs[0]->Value, &cc, 0 ) : 0;
			ns->runconf.evtres.max = attrs[1] ? (vt_long)strtol( attrs[1]->Value, &cc, 0 ) : -1;

			xmlGetParentElement( hxml );
		}

		//
		ele = xmlGetCurSubEle( hxml, "Vulnerability", 0 );
		if( ele )
		{
			ele = xmlGetCurSubEle( hxml, "confidence_threshold", 0 );
			if( ele )
			{
				ns->runconf.vulner.thres_conf = (int)strtol( ele->Value, &cc, 0 );
				xmlGetParentElement( hxml );
			}

			ele = xmlGetCurSubEle( hxml, "datapath", 0 );
			if( ele )
			{
				ns->runconf.vulner.path.data = &g_vt_path_data;
				ns->runconf.vulner.path.meta = &g_vt_path_meta;
				ns->runconf.vulner.path.report = &g_vt_path_report;

				attrs[0] = xmlGetEleAttr( ele, "home" );
				attrs[1] = xmlGetEleAttr( ele, "meta" );
				attrs[2] = xmlGetEleAttr( ele, "report" );
				if( attrs[0] && attrs[0]->Value[0] )
					ns->runconf.vulner.path.data = vt_symtab_add( &ns->consts.symb, attrs[0]->Value, strlen(attrs[0]->Value) );
				if( attrs[1] && attrs[1]->Value[0] )
				{
					ns->runconf.vulner.path.meta = vt_symtab_add( &ns->consts.symb, NULL, strlen(attrs[1]->Value)+ns->runconf.vulner.path.data->len+1 );
					if( ns->runconf.vulner.path.meta )
						sprintf( ns->runconf.vulner.path.meta->cs, "%s/%s", ns->runconf.vulner.path.data->cs, attrs[1]->Value );
				}
				if( attrs[2] && attrs[2]->Value[0] )
				{
					ns->runconf.vulner.path.report = vt_symtab_add( &ns->consts.symb, NULL, strlen(attrs[2]->Value)+ns->runconf.vulner.path.data->len+1 );
					if( ns->runconf.vulner.path.report )
						sprintf( ns->runconf.vulner.path.report->cs, "%s/%s", ns->runconf.vulner.path.data->cs, attrs[2]->Value );
				}
				xmlGetParentElement( hxml );
			}
			xmlGetParentElement( hxml );
		}

		//
		ele = xmlGetCurSubEle( hxml, "AsynCalculate", 0 );
		if( ele )
		{
			attrs[0] = xmlGetEleAttr( ele, "threads" );
			attrs[1] = xmlGetEleAttr( ele, "jobmax" );
			ns->runconf.asyn_calc.nthread = attrs[0] ? (int)strtol( attrs[0]->Value, &cc, 0 ) : 0;
			ns->runconf.asyn_calc.jobmax = attrs[1] ? (int)strtol( attrs[1]->Value, &cc, 0 ) : -1;

			xmlGetParentElement( hxml );
		}

		//
		ele = xmlGetCurSubEle( hxml, "Journal", 0 );
		if( ele )
		{
			attrs[0] = xmlGetEleAttr( ele, "mask" );
			if( attrs[0] && attrs[0]->Value[0] )
				g_vt_journal_mask = (vt_journal_type)strtol( attrs[0]->Value, &cc, 0 );
			xmlGetParentElement( hxml );
		}

		xmlGetParentElement( hxml );
	}

	return 0;
}


// Function: load configures
int vt_conf_load( const char *home, vt_namespace *ns )
{
	HXML hxml;
	XML_ELEMENT *ele;
	XML_ATTRIBUTE *attr;
	vt_long ll;
	char fnm[1024];


	//
	snprintf( fnm, sizeof(fnm), "%s/"VT_CONFPATH"/"VT_CONFNAME, home?home:"." );

	if( xmlLoad(fnm, &hxml) != XML_ERR_OK )
	{
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "Can not load configure file \"%s\"\n", fnm );
		return -1;
	}


	if( !xmlGetElement( hxml, "SPECTRUM-VTRACE-CONFIG" ) )
	{
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "Can not seek the root node \"SPECTRUM-VTRACE-CONFIG\"\n" );
		goto LAB_ERROR;
	}


	// NameSpace
	ele = xmlGetCurSubEle( hxml, "NameSpace", 0 );
	if( ele )
	{
		ns->name = NULL;
		attr = xmlGetEleAttr( ele, "name" );
		if( attr && attr->Value[0] )
		{
			ns->name = mmp_malloc( 1, &ns->mmphd, strlen(attr->Value) + 1 );
			if( !ns->name )
				goto LAB_ERROR;
			strcpy( ns->name, attr->Value );
		}

		ns->lang = VTRACE_LANG_UNKNOWN;
		attr = xmlGetEleAttr( ele, "lang" );
		if( attr && attr->Value[0] )
		{
			long ll;
			if( vt_dict_getvalue( g_vt_dict_Language, sizeof(g_vt_dict_Language) / sizeof(vt_dict), attr->Value, &ll ) != -1 )
				ns->lang = (vt_lang)ll;
		}

		//
		memset( &ns->defines, 0, sizeof(ns->defines) );
		memset( &ns->globals, 0, sizeof(ns->globals) );
		memset( &ns->runconf, 0, sizeof(ns->runconf) );

		// to load "Symbols"
		ele = xmlGetCurSubEle( hxml, "SymbolTable", 0 );
		if( ele )
		{
			xmlGetParentElement( hxml );
		}

		// to load specified dictionary
		ele = xmlGetCurSubEle( hxml, "Dictionary", 0 );
		if( ele )
		{
			// dict: ExceptionType
			ele = xmlGetCurSubEle( hxml, "ExceptionType", 0 );
			if( ele )
			{
				if( (ns->consts.dict.excption_type = vt_conf_load_dictionary( hxml, ns, &ll )) )
				{
					ns->consts.dict.num_excption_type = (vt_uint)ll;
	#if (VT_DICT_SORT_BY == 2)
					vt_dict_sort_bykey( ns->consts.dict.excption_type, ll );
	#else
					vt_dict_sort_byval( ns->consts.dict.excption_type, ll );
	#endif
					_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_INFO, "succeed to load %lld items of the dictionary \"ExceptionType\"\n", ll );
				}
				xmlGetParentElement( hxml );
			}

			// dict: RequestType
			ele = xmlGetCurSubEle( hxml, "RequestType", 0 );
			if( ele )
			{
				if( (ns->consts.dict.request_type = vt_conf_load_dictionary( hxml, ns, &ll )) )
				{
					ns->consts.dict.num_request_type = (vt_uint)ll;
	#if (VT_DICT_SORT_BY == 2)
					vt_dict_sort_bykey( ns->consts.dict.request_type, ll );
	#else
					vt_dict_sort_byval( ns->consts.dict.request_type, ll );
	#endif
					_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_INFO, "succeed to load %lld items of the dictionary \"RequestType\"\n", ll );
				}
				xmlGetParentElement( hxml );
			}

			// dict: ResponseType
			ele = xmlGetCurSubEle( hxml, "ResponseType", 0 );
			if( ele )
			{
				if( (ns->consts.dict.response_type = vt_conf_load_dictionary( hxml, ns, &ll )) )
				{
					ns->consts.dict.num_response_type = (vt_uint)ll;
	#if (VT_DICT_SORT_BY == 2)
					vt_dict_sort_bykey( ns->consts.dict.response_type, ll );
	#else
					vt_dict_sort_byval( ns->consts.dict.response_type, ll );
	#endif
					_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_INFO, "succeed to load %lld items of the dictionary \"ResponseType\"\n", ll );
				}
				xmlGetParentElement( hxml );
			}

			// dict: EventType
			ele = xmlGetCurSubEle( hxml, "EventType", 0 );
			if( ele )
			{
				if( (ns->consts.dict.event_type = vt_conf_load_dictionary( hxml, ns, &ll )) )
				{
					ns->consts.dict.num_event_type = (vt_uint)ll;
	#if (VT_DICT_SORT_BY == 2)
					vt_dict_sort_bykey( ns->consts.dict.event_type, ll );
	#else
					vt_dict_sort_byval( ns->consts.dict.event_type, ll );
	#endif
					_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_INFO, "succeed to load %lld items of the dictionary \"EventType\"\n", ll );
				}
				else
				{
					ns->consts.dict.event_type = ns->consts.dict.request_type;
					ns->consts.dict.num_event_type = ns->consts.dict.num_request_type;
				}
				xmlGetParentElement( hxml );
			}
			xmlGetParentElement( hxml );
		}

		// to load "Constraints"
		if( vt_conf_load_constraints(hxml, ns) == -1 )
			goto LAB_ERROR;
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_INFO, "succeed to load %lld items of Constraints\n",
						  ns->defines.nconstraint );

		// to load "RunConf"
		vt_conf_load_runconf( hxml, ns );

		if( ns->consts.dict.num_event_type > 0 &&
			(ns->runconf.evtres.intv > 0 || ns->runconf.evtres.max > 0) )
		{
			ns->evttab.ite = _vt_evttab_setup( ns->consts.dict.event_type, ns->consts.dict.num_event_type, &ns->mmphd );
			if( ns->evttab.ite )
				ns->evttab.nite = ns->consts.dict.num_event_type;
		}
		else
			ns->evttab.ite = NULL;

		// to load "DataClasses"

		// to load "NamedDataObjects"

		// to load "TraceRules"
		if( vt_conf_load_rules( hxml, ns ) == -1 )
			goto LAB_ERROR;
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_INFO, "succeed to load %lld rules of %lld RuleEngines\n",
						  ns->defines.nrule, ns->defines.nrulelib );

		// to load "Methods"
		// NOTE: the "TracePointList" has been merged with the level "Methods"
		if( vt_conf_load_methods( hxml, ns ) == -1 )
			goto LAB_ERROR;
		_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_INFO, "succeed to load %lld methos of %lld classes\n",
						  ns->defines.nmethod, ns->defines.nklass );

		// setup fixed dictionary
		ns->consts.dict.grp[VTRACE_DICT_TRACE_MODE-1]  = g_vt_dict_TraceMode;
		ns->consts.dict.ngrp[VTRACE_DICT_TRACE_MODE-1] = sizeof(g_vt_dict_TraceMode) / sizeof(vt_dict);

		ns->consts.dict.grp[VTRACE_DICT_TRACE_STAGE-1]  = g_vt_dict_TraceStage;
		ns->consts.dict.ngrp[VTRACE_DICT_TRACE_STAGE-1] = sizeof(g_vt_dict_TraceStage) / sizeof(vt_dict);

		ns->consts.dict.grp[VTRACE_DICT_TRACE_LEVEL-1]  = g_vt_dict_TraceLevel;
		ns->consts.dict.ngrp[VTRACE_DICT_TRACE_LEVEL-1] = sizeof(g_vt_dict_TraceLevel) / sizeof(vt_dict);

		ns->consts.dict.grp[VTRACE_DICT_TRACE_LANG-1]  = g_vt_dict_Language;
		ns->consts.dict.ngrp[VTRACE_DICT_TRACE_LANG-1] = sizeof(g_vt_dict_Language) / sizeof(vt_dict);

		ns->consts.dict.grp[VTRACE_DICT_TRACE_CAUSE-1]  = g_vt_dict_TraceCause;
		ns->consts.dict.ngrp[VTRACE_DICT_TRACE_CAUSE-1] = sizeof(g_vt_dict_TraceCause) / sizeof(vt_dict);

		ns->consts.dict.grp[VTRACE_DICT_TRACE_KIND-1]  = g_vt_dict_TraceKind;
		ns->consts.dict.ngrp[VTRACE_DICT_TRACE_KIND-1] = sizeof(g_vt_dict_TraceKind) / sizeof(vt_dict);

		ns->consts.dict.grp[VTRACE_DICT_DATA_TYPE-1]  = g_vt_dict_DataType;
		ns->consts.dict.ngrp[VTRACE_DICT_DATA_TYPE-1] = sizeof(g_vt_dict_DataType) / sizeof(vt_dict);

		ns->consts.dict.grp[VTRACE_DICT_JOURNAL_TYPE-1]  = g_vt_dict_JournalType;
		ns->consts.dict.ngrp[VTRACE_DICT_JOURNAL_TYPE-1] = sizeof(g_vt_dict_JournalType) / sizeof(vt_dict);

		ns->consts.dict.grp[VTRACE_DICT_OPERATOR_TYPE-1]  = g_vt_dict_OperatorType;
		ns->consts.dict.ngrp[VTRACE_DICT_OPERATOR_TYPE-1] = sizeof(g_vt_dict_OperatorType) / sizeof(vt_dict);

		ns->consts.dict.grp[VTRACE_DICT_OPERATOR_SET-1]  = g_vt_dict_OperatorSet;
		ns->consts.dict.ngrp[VTRACE_DICT_OPERATOR_SET-1] = sizeof(g_vt_dict_OperatorSet) / sizeof(vt_dict);

		ns->consts.dict.grp[VTRACE_DICT_TRACE_PROTOCOL-1]  = g_vt_dict_Protocol;
		ns->consts.dict.ngrp[VTRACE_DICT_TRACE_PROTOCOL-1] = sizeof(g_vt_dict_Protocol) / sizeof(vt_dict);
	}

	xmlUnload( hxml );
	return 0;

	LAB_ERROR:
	_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "the configure file \"%s\" is not integrity\n", fnm );
	xmlUnload( hxml );
	// other cleaning work
	return -1;
}

// Function:
int vt_ns_initiate( const char *home, vt_namespace *ns )
{
	if( __sync_val_compare_and_swap( &g_vt_conf_initiated, 0, 1 ) == 0 )
	{
#if (VT_DICT_SORT_BY == 2)
		vt_dict_sort_bykey( (vt_dict*)g_vt_dict_TraceMode, sizeof(g_vt_dict_TraceMode) / sizeof(vt_dict) );
		vt_dict_sort_bykey( (vt_dict*)g_vt_dict_TraceStage, sizeof(g_vt_dict_TraceStage) / sizeof(vt_dict) );
		vt_dict_sort_bykey( (vt_dict*)g_vt_dict_TraceLevel, sizeof(g_vt_dict_TraceLevel) / sizeof(vt_dict) );
		vt_dict_sort_bykey( (vt_dict*)g_vt_dict_Language, sizeof(g_vt_dict_Language) / sizeof(vt_dict) );
		vt_dict_sort_bykey( (vt_dict*)g_vt_dict_TraceCause, sizeof(g_vt_dict_TraceCause) / sizeof(vt_dict) );
		vt_dict_sort_bykey( (vt_dict*)g_vt_dict_TraceKind, sizeof(g_vt_dict_TraceKind) / sizeof(vt_dict) );
		vt_dict_sort_bykey( (vt_dict*)g_vt_dict_DataType, sizeof(g_vt_dict_DataType) / sizeof(vt_dict) );
		vt_dict_sort_bykey( (vt_dict*)g_vt_dict_JournalType, sizeof(g_vt_dict_JournalType) / sizeof(vt_dict) );
		vt_dict_sort_bykey( (vt_dict*)g_vt_dict_OperatorType, sizeof(g_vt_dict_OperatorType) / sizeof(vt_dict) );
		vt_dict_sort_bykey( (vt_dict*)g_vt_dict_OperatorSet, sizeof(g_vt_dict_OperatorSet) / sizeof(vt_dict) );
		vt_dict_sort_bykey( (vt_dict*)g_vt_dict_Protocol, sizeof(g_vt_dict_Protocol) / sizeof(vt_dict) );
#else
		vt_dict_sort_byval( (vt_dict*)g_vt_dict_TraceMode, sizeof(g_vt_dict_TraceMode) / sizeof(vt_dict) );
		vt_dict_sort_byval( (vt_dict*)g_vt_dict_TraceStage, sizeof(g_vt_dict_TraceStage) / sizeof(vt_dict) );
		vt_dict_sort_byval( (vt_dict*)g_vt_dict_TraceLevel, sizeof(g_vt_dict_TraceLevel) / sizeof(vt_dict) );
		vt_dict_sort_byval( (vt_dict*)g_vt_dict_Language, sizeof(g_vt_dict_Language) / sizeof(vt_dict) );
		vt_dict_sort_byval( (vt_dict*)g_vt_dict_TraceCause, sizeof(g_vt_dict_TraceCause) / sizeof(vt_dict) );
		vt_dict_sort_byval( (vt_dict*)g_vt_dict_TraceKind, sizeof(g_vt_dict_TraceKind) / sizeof(vt_dict) );
		vt_dict_sort_byval( (vt_dict*)g_vt_dict_DataType, sizeof(g_vt_dict_DataType) / sizeof(vt_dict) );
		vt_dict_sort_byval( (vt_dict*)g_vt_dict_JournalType, sizeof(g_vt_dict_JournalType) / sizeof(vt_dict) );
		vt_dict_sort_byval( (vt_dict*)g_vt_dict_OperatorType, sizeof(g_vt_dict_OperatorType) / sizeof(vt_dict) );
		vt_dict_sort_byval( (vt_dict*)g_vt_dict_OperatorSet, sizeof(g_vt_dict_OperatorSet) / sizeof(vt_dict) );
		vt_dict_sort_byval( (vt_dict*)g_vt_dict_Protocol, sizeof(g_vt_dict_Protocol) / sizeof(vt_dict) );
#endif

		g_vt_dds_md = EVP_md5();
	}

	memset( ns, 0, sizeof(vt_namespace) );

	ns->lock = BsemCreate( SEM_FULL );
	//ns->sem_ctxlnk = BsemCreate( SEM_FULL );

	mmp_create_mempool( 1, &ns->mmphd );
	mmp_create_mempool( 1, &ns->mmphd_dyn );
	vt_symtab_init( &ns->consts.symb, 512 );

	dlk_Init( &ns->ctxlnk );
	//dlk_Init( &ns->evtlnk );

	if( vt_conf_load(home, ns) == 0 )
	{
		if( !ns->runconf.vulner.path.data )
			ns->runconf.vulner.path.data = &g_vt_path_data;
		if( !ns->runconf.vulner.path.meta )
			ns->runconf.vulner.path.meta = &g_vt_path_meta;
		if( !ns->runconf.vulner.path.report )
			ns->runconf.vulner.path.report = &g_vt_path_report;

		// make essential directories
		if( access( ns->runconf.vulner.path.data->cs, F_OK ) != 0 )
		{
			if( mkdir( ns->runconf.vulner.path.data->cs, 0775 ) != 0 )
				_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "can not build the directory \"%s\" with an errno %d\n",
						ns->runconf.vulner.path.data->cs, errno );
		}

		if( access( ns->runconf.vulner.path.meta->cs, F_OK ) != 0 )
		{
			if( mkdir( ns->runconf.vulner.path.meta->cs, 0775 ) != 0 )
				_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "can not build the directory \"%s\" with an errno %d\n",
						ns->runconf.vulner.path.meta->cs, errno );
		}

		if( access( ns->runconf.vulner.path.report->cs, F_OK ) != 0 )
		{
			if( mkdir( ns->runconf.vulner.path.report->cs, 0775 ) != 0 )
				_VT_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "can not build the directory \"%s\" with an errno %d\n",
						ns->runconf.vulner.path.report->cs, errno );
		}

		return 0;
	}

	vt_ns_destroy( ns );
	return -1;
}

//
void vt_ns_destroy( vt_namespace *ns )
{
	vt_long ll;
	vt_context *ctx = (vt_context*)ns->ctxlnk.head;

	while( ctx )
	{
		vt_symtab_release( &ctx->locsym );
		mmp_destroy_mempool( 1, &ctx->mmphd );
		if( ctx->bsem )
			SemDelete( ctx->bsem );
		ctx = ctx->clue._next;
	}

	//
	for( ll=ns->defines.nrulelib-1; ll >= 0; ll-- )
		vt_rulelib_unload( ns->defines.rulelibs[ll] );

	//
	vt_symtab_release( &ns->consts.symb );
	mmp_destroy_mempool( 1, &ns->mmphd );
	mmp_destroy_mempool( 1, &ns->mmphd_dyn );
	if( ns->lock )
		SemDelete( ns->lock );
	//if( ns->sem_ctxlnk )
	//	SemDelete( ns->sem_ctxlnk );
	memset( ns, 0, sizeof(vt_namespace) );
}

//
vt_event* vt_ns_alloc_event( vt_namespace *ns )
{
	vt_event *evt = NULL;

#if 0
	if( ns && BsemTake(ns->lock, -1) )
	{
		evt = mmp_malloc( 1, &ns->mmphd_dyn, sizeof(vt_event) );
		if( evt )
		{
			memset( evt, 0, sizeof(vt_event) );
			evt->bsem = BsemCreate( SEM_FULL );
			if( evt->bsem )
			{
				evt->container = ns;
				dlk_Add( &ns->evtlnk, (dlk_node*)evt, 0 );
			}
			else
			{
				mmp_free( 1, &ns->mmphd_dyn, evt );
				evt = NULL;
			}
		}
		BsemGive( ns->lock );
	}
#else
	evt = mmp_malloc( 1, NULL, sizeof(vt_event) );
	if( evt )
	{
		memset( evt, 0, sizeof(vt_event) );
		evt->bsem = BsemCreate( SEM_FULL );
		if( evt->bsem )
		{
			evt->clue_uni.magic.mark = VTRACE_MAGIC_MARK_CHECK( evt );
			evt->container = ns;
		}
		else
		{
			mmp_free( 1, NULL, evt );
			evt = NULL;
		}
	}
#endif

	return evt;
}

//
void vt_ns_free_event( vt_event *evt )
{
#if 0
	if( evt )
	{
		vt_namespace *ns = (vt_namespace*)evt->container;

		if( evt->bsem )
			SemDelete( evt->bsem );

		if( BsemTake(ns->lock, -1) )
		{
			dlk_Drop( &ns->evtlnk, (dlk_node*)evt, 0 );
			mmp_free( 1, &ns->mmphd_dyn, evt );
			BsemGive( ns->lock );
		}
	}
#else
	if( evt->bsem )
		SemDelete( evt->bsem );
	mmp_free( 1, NULL, evt );
#endif
}

#if 0
//
vt_context* vt_ns_alloc_context( vt_namespace *ns )
{
	vt_context *ctx = NULL;

	if( ns && BsemTake(ns->lock, -1) )
	{
		ctx = mmp_malloc( 1, &ns->mmphd_dyn, sizeof(vt_context)+sizeof(vt_action) );
		if( ctx )
		{
			memset( ctx, 0, sizeof(vt_context) + sizeof(vt_action) );

			ctx->act = (vt_action*)&ctx[1];
			ctx->act->ctx = ctx;

			ctx->bsem = BsemCreate( SEM_FULL );
			if( ctx->bsem )
			{
				ctx->act->clue_evt.magic.mark = VTRACE_MAGIC_MARK_CHECK( ctx->act );
				vt_create_mempool( &ctx->mmphd );
				vt_symtab_init( &ctx->locsym, 64 );

				ctx->ns = ns;
				dlk_Add( &ns->ctxlnk, (dlk_node*)ctx, 0 );
			}
			else
			{
				mmp_free( 1, &ns->mmphd_dyn, ctx );
				ctx = NULL;
			}
		}
		BsemGive( ns->lock );
	}
	return ctx;
}

//
void vt_ns_free_context( vt_context *ctx )
{
	if( ctx )
	{
		vt_symtab_release( &ctx->locsym );
		vt_destroy_mempool( &ctx->mmphd );

		if( ctx->bsem )
			SemDelete( ctx->bsem );

		vt_namespace *ns = ctx->ns;

		if( ns && BsemTake(ns->lock, -1) )
		{
			dlk_Drop( &ns->ctxlnk, (dlk_node*)ctx, 0 );
			mmp_free( 1, &ns->mmphd_dyn, ctx );
			BsemGive( ns->lock );
		}
	}
}
#endif

//
vt_context* vt_alloc_context( vt_event *evt )
{
	vt_context *ctx = mmp_malloc( 1, NULL, sizeof(vt_context)+sizeof(vt_action) );
	if( ctx )
	{
		memset( ctx, 0, sizeof(vt_context) + sizeof(vt_action) );

		ctx->act = (vt_action*)&ctx[1];
		ctx->act->ctx = ctx;

		ctx->bsem = BsemCreate( SEM_FULL );
		if( ctx->bsem )
		{
			ctx->act->clue_evt.magic.mark = VTRACE_MAGIC_MARK_CHECK( ctx->act );
			vt_create_mempool( &ctx->mmphd );
			vt_symtab_init( &ctx->locsym, 64 );
			ctx->ns = evt->container;
		}
		else
		{
			mmp_free( 1, NULL, ctx );
			ctx = NULL;
		}
	}

	return ctx;
}
//
void vt_free_context( vt_context *ctx )
{
	if( ctx )
	{
		vt_symtab_release( &ctx->locsym );
		vt_destroy_mempool( &ctx->mmphd );

		if( ctx->bsem )
			SemDelete( ctx->bsem );

		mmp_free( 1, NULL, ctx );
	}
}
