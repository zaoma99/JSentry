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

#include "vt_api.h"


VT_LOCAL int g_vt_print_type_size[31] = VT_TYPE_SIZE_INITIATOR;

//
void vt_print_primitive( FILE *fd, const char *fmt, const char *suffix,
						 int type, vt_size len, vt_value *val )
{
	switch( type )
	{
	case VT_TYPE_SI8:
		fprintf( fd, fmt?fmt:"%c%s", val->c, suffix );
		break;
	case VT_TYPE_UI8:
		fprintf( fd, fmt?fmt:"%u%s", val->b, suffix );
		break;
	case VT_TYPE_SI16:
		fprintf( fd, fmt?fmt:"%d%s", val->s, suffix );
		break;
	case VT_TYPE_UI16:
		fprintf( fd, fmt?fmt:"%u%s", val->w, suffix );
		break;
	case VT_TYPE_SI32:
		fprintf( fd, fmt?fmt:"%d%s", val->i, suffix );
		break;
	case VT_TYPE_UI32:
		fprintf( fd, fmt?fmt:"%u%s", val->u, suffix );
		break;
	case VT_TYPE_SI64:
	case VT_TYPE_TIME:
		fprintf( fd, fmt?fmt:"%lld%s", val->l, suffix );
		break;
	case VT_TYPE_UI64:
		fprintf( fd, fmt?fmt:"%llu%s", val->q, suffix );
		break;
	case VT_TYPE_FLOAT:
		fprintf( fd, fmt?fmt:"%f%s", val->f, suffix );
		break;
	case VT_TYPE_DOUBLE:
		fprintf( fd, fmt?fmt:"%e%s", val->d, suffix );
		break;
	case VT_TYPE_POINTER:
		fprintf( fd, fmt?fmt:"%p%s", val->p, suffix );
		break;
	case VT_TYPE_CLOCK:{
		vt_clock *clk = (vt_clock*)val;
		fprintf( fd, "{sec:%lld, nsec:%lld}%s", clk->tv_sec, clk->tv_nsec, suffix );
		break;
	}
	case VT_TYPE_IDENTIFIER:{
		vt_ident *ident = (vt_ident*)val;
		fprintf( fd, "{L:0x%llx, H:0x%llx}%s", ident->low, ident->high, suffix );
		break;
	}
	case VT_TYPE_CSTRING:{
		if( len > 0 )
		{
			/*char s[256];
			if( len >= sizeof(s) )
				len = sizeof(s) - 1;
			memcpy( s, (char*)val->p, len );
			s[len] = 0;
			fprintf( fd, "\"%s\"%s", s, suffix );
			*/
			fprintf( fd, "\"" );
			fwrite( val->p, 1, len>256?256:len, fd );
			fprintf( fd, "\"%s", suffix );
		}
		break;
	}
	default:
		if( len <= sizeof(vt_value) )
			fprintf( fd, fmt?fmt:"%p%s", val->p, suffix );
		break;
	}
}

//
void vt_print_capacity( FILE *fd, const char *level, const char *suffix, const vt_capa *caps )
{
	fprintf( fd, "capacity:{%s"
			 "sot_max: %llu,%s"
			 "mem_max: %llu,%s"
			 "dsk_max: %llu,%s"
			 "cputm_max: %llu,%s"
			 "timeout: %lld,%s"
			 "fd_max: %d,%s"
			 "rep_max: %d,%s"
			 "freq: %d,%s"
			 "doj_dec: %d,%s"
			 "flags: 0x%llx}%s",
			 level,
			 caps->sot_max, level,
			 caps->mem_max, level,
			 caps->dsk_max, level,
			 caps->live_max, level,
			 caps->exec_tmo, level,
			 caps->fd_max, level,
			 caps->rep_max, level,
			 caps->freq, level,
			 caps->doj_dec, level,
			 caps->flag,
			 suffix );
}

//
void vt_print_tlv( FILE *fd, const char *level, const char *suffix,
				   const vt_tlv *tlv, vt_long ntlv )
{
	vt_long i;
	vt_size ln;
	vt_value *val;
	const char *comma[] = {", ", ""};

	for( i=0; i < ntlv; i++ )
	{
		val = (vt_value*)vt_tlv_parse( (vt_tlv*)&tlv[i], &ln );

		fprintf( fd, "%stlv:{t:0x%04x, l:%llu, v:", comma[i==0], tlv[i].tag, ln );

		switch( tlv[i].type1 )
		{
		case VT_TYPE_ARRAY:{
			int tp = tlv[i].type2;
			if( VT_TYPE_IS_PREMITIVE(tp) )
			{
				fprintf( fd, "[" );
				vt_size n, z = g_vt_print_type_size[tp];
				char *ptr = val->p;
				ln %= 21;
				for( n=0; n < ln; n++ )
				{
					vt_print_primitive( fd, NULL, ", ", tp, z, (void*)ptr );
					ptr += z;
				}
				fprintf( fd, "\b\b]" );
			}
			break;
		}
		case VT_TYPE_CLASS:
		case VT_TYPE_BSTRING:{
			fprintf( fd, "[" );
			vt_size n;
			char *ptr = val->p;
			ln %= 257;
			for( n=0; n < ln; n++ )
			{
				vt_print_primitive( fd, "%02x", ", ", VT_TYPE_BYTE, 1, (void*)ptr );
				ptr++;
			}
			fprintf( fd, "\b\b]" );
			break;
		}
		default:
			vt_print_primitive( fd, NULL, comma[i==0], tlv[i].type1, ln, val );
			break;
		}

		fprintf( fd, "}%s\t\t", level );
	}
	fprintf( fd, "%s\t", suffix );
}

//
void vt_print_data_sequence( FILE *fd, const char *name,
							 const char *level, const char *suffix,
							 const vt_data_sequence *datseq )
{
	vt_long n = datseq->ndat;

	fprintf( fd, "%s:{%s\t"
			 "ndat: %lld,%s\t"
			 "uids: [",
			 name,
			 level,
			 n, level );

	n %= 101;  // limit 100

	vt_long i;
	vt_ident *uids = datseq->uids;
	vt_cstr  *names = datseq->names;
	vt_tlv   *datas = datseq->datas;
	const char *comma[] = {", ", ""};

	if( uids )
	{
		for( i=0; i < n; i++ )
			fprintf( fd, "%s{0x%llx, 0x%llx}", comma[i==0], uids[i].low, uids[i].high );
	}

	fprintf( fd, "],%s\tnames: [", level );
	if( names )
	{
		//char nm[128];
		//int ln;
		for( i=0; i < n; i++ )
		{
			// always copy string due to it is possible missing the terminal '\0'
			/*ln = names[i].cs ? names[i].len : 0;
			if( ln >= sizeof(nm) )
				ln = sizeof(nm) - 1;
			memcpy( nm, names[i].cs, ln );
			nm[ln] = 0;
			fprintf( fd, "%s\"%s\"", comma[i==0], nm );
			*/
			fprintf( fd, "%s\"", comma[i==0] );
			fwrite( names[i].cs, 1, names[i].len>=128?128:names[i].len, fd );
			fprintf( fd, "\"" );
		}
	}

	fprintf( fd, "],%s\tdatas: [", level );
	if( datas )
	{
		vt_print_tlv( fd, level, level, datas, n );
	}

	fprintf( fd, "]%s}%s", level, suffix );
}

//
void vt_print_event_entity( FILE *fd, const char *level, const char *suffix, const vt_event_entity *entity )
{
	fprintf( fd, "entity:{%s"
			 "type: %u,%s"
			 "opt: %u,%s"
			 "ident: [0x%llx, 0x%llx],%s"
			 "from: [0x%llx, 0x%llx],%s"
			 "dest: [0x%llx, 0x%llx],%s",
			 level,
			 entity->type, level,
			 entity->_idx_, level,
			 entity->ident.low, entity->ident.high, level,
			 entity->from.low, entity->from.high, level,
			 entity->dest.low, entity->dest.high, level );

	vt_print_data_sequence( fd, "request", level, "", &entity->req.datseq );

	fprintf( fd, "%s}%s", level, suffix );
}

//
void vt_print_event( FILE *fd, vt_thread init_thread, int mode,
					 const vt_capa *caps, const vt_event_entity *entity )
{
	fprintf( fd,
			"\n==========================================\n"
			"EventCreate:{\n\t"
			"init_thread: %p,\n\t"
			"mode: %d,\n\t",
			init_thread, mode );

	if( caps )
		vt_print_capacity( fd, "\n\t\t", ",\n\t", caps );

	vt_print_event_entity( fd, "\n\t\t", "\n", entity );

	fprintf( fd, "}\n" );
}

//
void vt_print_callstack( FILE *fd, const char *level, const char *suffix, vt_data_callstack *cstk )
{
	fprintf( fd, "callstack:{%s"
			"thread: %p,%s"
			"nframe: %d,%s"
			"frames: [",
			level,
			cstk->thread, level,
			cstk->num, level );

	vt_int i;
	vt_call_frame *frm = cstk->frame;

	for( i=0; i < cstk->num; i++ )
	{
		if( frm[i].full_path && frm[i].path_len > 0 )
		{
			fprintf( fd, "%s\"", level );
			fwrite( frm[i].full_path, 1, frm[i].path_len, fd );
			fprintf( fd, ":%lld\"", frm[i].pos );
		}
	}

	fprintf( fd, "]%s}%s", level, suffix );
}

//
void vt_print_exception( FILE *fd, const char *level, const char *suffix, vt_data_exception *exp )
{
	fprintf( fd, "exception:{%s"
			"type: %d,%s"
			"message: \"",
			level, exp->type, level );

	fwrite( exp->msgstr, 1, exp->msglen>=4096?4096:exp->msglen, fd );

	fprintf( fd, "\",%s", level );

	vt_print_callstack( fd, level, level, &exp->backtrace );

	fprintf( fd, "}%s", suffix );
}

//
void vt_print_journal( FILE *fd, const char *suffix, vt_data_journal *log )
{
	fprintf( fd, "journal:\"%02x#",log->type );

	fwrite( log->source, 1, log->srclen, fd );

	fprintf( fd, "#" );
	fwrite( log->message, 1, log->msglen, fd );

	fprintf( fd, "\"%s", suffix );
}

//
void vt_print_sot_entity( FILE *fd, const char *level, const char *suffix,
						  vt_sot_entity *entity, vt_bool isException )
{
	if( isException )
		vt_print_exception( fd, level, level, &entity->expt );
	else
		vt_print_data_sequence( fd, "args", level, level, &entity->invk.args.datseq );

	// callstack
	vt_print_callstack( fd, level, level, &entity->cstk );

	// watch
	vt_print_data_sequence( fd, "watch", level, level, &entity->watch.args.datseq );

	// journal
	vt_print_journal( fd, level, &entity->log );
}

//
void vt_print_sot( FILE *fd,
		   	   	   vt_long  actref,
				   vt_ulong tpidx,
				   vt_stage stage,
				   vt_cause cause,
				   vt_sot_entity *entity )
{
	fprintf( fd,
			"\n==========================================\n"
			"SOTAppend:{\n\t"
			"actref: %lld,\n\t"
			"tpidx: %lld,\n\t"
			"stage: %d,\n\t"
			"cause: %d,\n\t",
			actref, tpidx, stage, cause	);

	vt_print_sot_entity( fd, "\n\t\t", "\n", entity, (stage==VTRACE_STAGE_CATCH || stage==VTRACE_STAGE_CRASH) );

	fprintf( fd, "}\n" );
}

//
void vt_print_calc_confidence( FILE *fd,
							   vt_long actref,
							   vt_long evtref,
							   vt_data_sequence *datseq )
{
	fprintf( fd,
			"\n==========================================\n"
			"CalculateConfidence:{\n\t"
			"actref: %lld,\n\t"
			"evtref: %lld,\n\t",
			actref, evtref );

	vt_print_data_sequence( fd, "fed", "\n\t\t", "\n", datseq );

	fprintf( fd, "}\n" );
}


// Function: print namespace configures
void vt_ns_print_methods( FILE *fd, vt_namespace *ns )
{
#if 0
	vt_method_struct **methods;
	vt_class_struct **klass = ns->defines.klasses;
	vt_long i, j, a, b, m, n = ns->defines.nklass;
	vt_cstr *cs;
	vt_dict *dict[4];
	vt_data_class **datcls;

	for( i=0; i < n; i++ )
	{
		dict[0] = vt_dict_match_val2( ns->consts.dict.language, ns->consts.dict.num_language, klass[j]->lang );
		fprintf( fd,
				 "\n==========================================\n"
				 "Class: {name:\"%s\",lang:\"%s\",\n\t",
				 ((vt_cstr*)klass[i]->name)->cs, (dict[0] ? dict[0]->key : "") );

		m = klass[i]->method_num;
		methods = klass[i]->methods;

		for( j=0; j < m; j++ )
		{
			dict[0] = vt_dict_match_val2( ns->consts.dict.operator_type, ns->consts.dict.num_operator_type, methods[j]->inop );
			dict[1] = vt_dict_match_val2( ns->consts.dict.operator_set, ns->consts.dict.num_operator_set, methods[j]->opset );

			fprintf( fd, "m:{id:%x, decl:\"%s%s\", pos:%ld, op:\"%s\", group:\"%s\",\n\t\t",
					 methods[j]->id,
					 (methods[j]->name ? ((vt_cstr*)methods[j]->name)->cs : ""),
					 (methods[j]->decl ? ((vt_cstr*)methods[j]->decl)->cs : ""),
					 /*(methods[j]->alias ? ((vt_cstr*)methods[j]->alias)->cs : ""),*/
					 methods[j]->pos,
					 (dict[0] ? dict[0]->key : ""),
					 (dict[1] ? dict[1]->key : "")
					 );
			// parameters
			datcls = methods[j]->arglst.args;
			a = methods[j]->arglst.argn;
			for( b=0; b < a; b++ )
			{
				fprintf( fd, "",
						 datcls[b]-> );
			}
		}

		fprintf( fd, "}\n" );
	}
#endif
}

//
void vt_ns_print_conf( FILE *fd, vt_namespace *ns )
{

}
