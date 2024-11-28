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
#include "classfile_util.h"
#include "cf_bytecode.h"
#include "cf_impl.h"


CF_HIDDEN const juint8 CF_OPC_LEN[256] = CF_OPC_LEN_INITIALIZER;

CF_HIDDEN const char CF_OPC_SYM[256][24] = CF_OPC_SYM_INITIALIZER;

CF_HIDDEN const jint CF_OPC_STK_GROW[256] = CF_OPC_STACK_GROWTH_INITIALIZER;

CF_HIDDEN const juint8 CF_SMFVT_LEN[9] = CF_SMFVT_LEN_INIT;

// keep the order as requirement of the opcode "newarray"
CF_HIDDEN
const juint8 CF_IDX_TO_OPC_RETURN[16] = {
		0, 0, 0,
		CF_OPC_return,   /* void */
		CF_OPC_ireturn,  /* boolean */
		CF_OPC_ireturn,  /* char */
		CF_OPC_freturn,  /* float */
		CF_OPC_dreturn,  /* double */
		CF_OPC_ireturn,  /* byte */
		CF_OPC_ireturn,  /* short */
		CF_OPC_ireturn,  /* int */
		CF_OPC_lreturn,  /* long */
		CF_OPC_areturn,  /* class */
		CF_OPC_areturn,  /* array */
		CF_OPC_areturn,  /* enum ? */
		0
};

//
CF_HIDDEN
const juint8 CF_IDX_TO_OPC_STORE[16] = {
		0, 0, 0,
		0,   /* void */
		CF_OPC_istore,  /* boolean */
		CF_OPC_istore,  /* char */
		CF_OPC_fstore,  /* float */
		CF_OPC_dstore,  /* double */
		CF_OPC_istore,  /* byte */
		CF_OPC_istore,  /* short */
		CF_OPC_istore,  /* int */
		CF_OPC_lstore,  /* long */
		CF_OPC_astore,  /* class */
		CF_OPC_astore,  /* array */
		CF_OPC_astore,  /* enum ? */
		0
};

//
CF_HIDDEN
const juint8 CF_IDX_TO_OPC_LOAD[16] = {
		0, 0, 0,
		0,   /* void */
		CF_OPC_iload,  /* boolean */
		CF_OPC_iload,  /* char */
		CF_OPC_fload,  /* float */
		CF_OPC_dload,  /* double */
		CF_OPC_iload,  /* byte */
		CF_OPC_iload,  /* short */
		CF_OPC_iload,  /* int */
		CF_OPC_lload,  /* long */
		CF_OPC_aload,  /* class */
		CF_OPC_aload,  /* array */
		CF_OPC_aload,  /* enum ? */
		0
};


// Function: locate all CONTROL instructions
CF_InsLoc*  cf_locate_ctrl_instruction( const juint8 *code, int len, CF_InsLoc *loc_ptr, int *loc_len,
										int *last_opc_off, int *last_opc_len, CF_OPC ldc )
{
	register unsigned char c;
	register int p, n, l;
	int L, a0, _p;

	ldc = -(ldc!=CF_OPC_ldc) | CF_OPC_ldc;  // 2022-07-01

	L = *loc_len & !loc_ptr - 1;
	l = n = 0;
	p = _p = 0;
	while( p < len )
	{
		if( (n = cf_opc_len_calcu( &code[p], p )) <= 0 )
			return NULL;

		c = code[p];
		if( (c >= CF_OPC_CTRL_Begin && c <= CF_OPC_CTRL_End) ||
			(c >= CF_OPC_CMP_Begin && c <= CF_OPC_CMP_End) ||
			(c == CF_OPC_ifnull || c == CF_OPC_ifnonnull ||
			 c == CF_OPC_goto_w || c == CF_OPC_jsr_w) ||
			(ldc == c)
			)
		{
			// Control Instructions
			if( l >= L )
			{
				// re-allocate
				L += 32;
				void *ptr = realloc( loc_ptr, sizeof(CF_InsLoc) * L );
				if( !ptr )
					return NULL;
				loc_ptr = ptr;
			}

			loc_ptr[l].opc = c;
			loc_ptr[l].pos = p;
			loc_ptr[l].len = n;
			loc_ptr[l].ext = 0;
			loc_ptr[l].add = 0;
			loc_ptr[l].same = 0;
			loc_ptr[l].last_opc_pos = _p;

			// calculate the "jmpto", as a absolute position
			switch( c )
			{
			case CF_OPC_tableswitch:{
				// choose the maximum offset
		    	a0 = (p + 4) & -4;  // (p + 1 + 3) & -4;
		    	int _m = 1 + _MAKE_INT32_( code[a0+8], code[a0+9], code[a0+10], code[a0+11] )
		    			   - _MAKE_INT32_( code[a0+4], code[a0+5], code[a0+6], code[a0+7] );
		    	if( l+_m+1 > L )
				{
					// re-allocate
					L += _m+1;
					void *ptr = realloc( loc_ptr, sizeof(CF_InsLoc) * L );
					if( !ptr )
						return NULL;
					loc_ptr = ptr;
				}

		    	int _d = l;
		    	loc_ptr[l].jmpto = p + _MAKE_INT32_( code[a0], code[a0+1], code[a0+2], code[a0+3] );  // default is the first
		    	loc_ptr[l].same = 1;

		    	for( a0+=12; --_m >= 0; a0+=4 )
		    	{
		    		loc_ptr[++l] = loc_ptr[_d];
		    		loc_ptr[l].jmpto = p + _MAKE_INT32_( code[a0], code[a0+1], code[a0+2], code[a0+3] );
		    	}
		    	loc_ptr[_d].same = 0;
		    	break;
		    }
			case CF_OPC_lookupswitch:{
				// choose the maximum offset
		    	a0 = (p + 4) & -4;  // (p + 1 + 3) & -4;
		    	int _m = _MAKE_INT32_( code[a0+4], code[a0+5], code[a0+6], code[a0+7] );
		    	if( l+_m+1 > L )
				{
					// re-allocate
					L += _m+1;
					void *ptr = realloc( loc_ptr, sizeof(CF_InsLoc) * L );
					if( !ptr )
						return NULL;
					loc_ptr = ptr;
				}

		    	int _d = l;
		    	loc_ptr[l].jmpto = p + _MAKE_INT32_( code[a0], code[a0+1], code[a0+2], code[a0+3] );  // default is the first
		    	loc_ptr[l].same = 1;

		    	for( a0+=8; --_m >= 0; a0+=8 )
		    	{
		    		loc_ptr[++l] = loc_ptr[_d];
		    		loc_ptr[l].jmpto = p + _MAKE_INT32_( code[a0+4], code[a0+5], code[a0+6], code[a0+7] );
		    	}
		    	loc_ptr[_d].same = 0;
		    	break;
		    }
			case CF_OPC_goto_w:
			case CF_OPC_jsr_w:
				loc_ptr[l].jmpto = p + _MAKE_INT32_( code[p+1], code[p+2], code[p+3], code[p+4] );
				break;
			case CF_OPC_goto:
			case CF_OPC_jsr:
			case CF_OPC_ifeq:
			case CF_OPC_ifne:
			case CF_OPC_iflt:
			case CF_OPC_ifge:
			case CF_OPC_ifgt:
			case CF_OPC_ifle:
			case CF_OPC_if_icmpeq:
			case CF_OPC_if_icmpne:
			case CF_OPC_if_icmplt:
			case CF_OPC_if_icmpge:
			case CF_OPC_if_icmpgt:
			case CF_OPC_if_icmple:
			case CF_OPC_if_acmpeq:
			case CF_OPC_if_acmpne:
			case CF_OPC_ifnull:
			case CF_OPC_ifnonnull:
				a0 = _MAKE_INT16_( code[p+1], code[p+2] );
				loc_ptr[l].jmpto = p + (-(a0>>15)&0xffff0000|a0);
				break;
			case CF_OPC_lcmp:
			case CF_OPC_fcmpl:
			case CF_OPC_fcmpg:
			case CF_OPC_dcmpl:
			case CF_OPC_dcmpg:
			case CF_OPC_ret:
			case CF_OPC_ireturn:
			case CF_OPC_lreturn:
			case CF_OPC_freturn:
			case CF_OPC_dreturn:
			case CF_OPC_areturn:
			case CF_OPC_return:
			case CF_OPC_ldc:  // 2022-07-01
			default:
				loc_ptr[l].jmpto = -1;
				break;
			}

			l++;
		}
		_p = p;
		p += n;
	}

	if( last_opc_off )
		*last_opc_off = _p;
	if( last_opc_len )
		*last_opc_len = n;

	*loc_len = l;
	return loc_ptr;
}



// Function: alter offset as which operand of CONTROL instructions,
//           and alter exception table belongs to attribute "Code" of method
int cf_alter_instruction_offset( CF_InsLoc *locs, int num, int beg_pos, int exit_pos, CF_ExpOff *exptab, int expnum )
{
	// Note: Assumes that "locs" has been setup by cf_locate_ctrl_instruction
	//       Exit Code stays at the head

	register int i, k, p;
	register char d;
	register unsigned c;
	int n, _b, _e;


	for( n=i=0; i < num; i++ )
	{
		// seek instruction "return or [x]return"
		c = locs[i].opc;

		if( (exit_pos > 0 && (c >= CF_OPC_ireturn && c <= CF_OPC_return))
			 || c == CF_OPC_nop	|| c == CF_OPC_ldc  /* 2022-07-01 */
			)
		{
			// "return" will be replaced by "goto_w"
			// or "nop"(reserved placement) will be replaced by "goto_w"
			// or "ldc"(will be changed to ldc_w)

			// modified, 2022-06-23
			d = c==CF_OPC_ldc ? 1 : 4 + (c==CF_OPC_nop);

			locs[i].add = d;
			n += d;

			p = locs[i].pos;

			// try to alter all related offset
			for( k=0; k < num; k++ )
			{
				if( (_e=locs[k].jmpto) >= 0 )  // "jmpto" is a absolute position, means instruction position plus it's offset
				{
					if( (_b=locs[k].pos) < _e )
					{
						if( p<_e & p>_b)
							locs[k].ext += d;
					}
					else if( p<_b & p>_e )
						locs[k].ext -= d;
				}
			}
			goto LAB_ADJ_EXP;
		}
		else if( c == CF_OPC_tableswitch || c == CF_OPC_lookupswitch )
		{
			p = locs[i].pos;
			d = ((p&3) - (p+beg_pos+n&3));  // equal to (4-((A'+1)%4)) - (4-((A+1)%4))
			if( d )
			{
				locs[i].add = d;
				n += d;

				// try to alter all related offset
				for( k=0; k < num; k++ )
				{
					if( (_e=locs[k].jmpto) >= 0 )
					{
						if( (_b=locs[k].pos) < _e )
						{
							if( p<_e & p>=_b )
								locs[k].ext += d;
						}
						else if( p<_b & p>=_e )
							locs[k].ext -= d;
					}
				}
				goto LAB_ADJ_EXP;
			}
		}
		// DO NOT alter other CONTROL instructions
		continue;

		LAB_ADJ_EXP:	// try to adjust exception block
		for( k=0; k < expnum; k++ )
		{
			if( p < exptab[k].start_pc ) {
				exptab[k].start_off += d;
				exptab[k].end_off += d;
			}
			else if( p < exptab[k].end_pc )  // exclude the instruction stays at the end_pc
				exptab[k].end_off += d;

			if( p < exptab[k].handler_pc )
				exptab[k].handler_off += d;
		}
	}

	return n;
}



// Function: re-assemble instruction sequences according to altered locations
int cf_assemble_instruction( const juint8 *src, int srclen,
							 juint8 *dst, int dstsize,
							 CF_InsLoc *locs, int nlocs,
							 int dst_pos, int exit_pos,
							 int *end_opc_off, int *end_opc_len
							 )
{
	// Note: assumes CF_InsLoc(s) have been sorted by ascent

	int i, _p, p, n, l, _l;
	juint8 c;

	i = p = 0;
	l = _l = dst_pos;

	while( p < srclen && l < dstsize )
	{
		_l = l;

		if( i >= nlocs )
		{
			// the last block
			n = srclen - p;
			memcpy( &dst[l], &src[p], n );
			l += n;
			break;
		}

		// block between CONTROLs

		_p = locs[i].pos;
		n = _p - p;
		memcpy( &dst[l], &src[p], n );
		l += n;

		c = locs[i].opc;

		if( locs[i].add == 5 && ((c>=CF_OPC_IF_Begin && c<=CF_OPC_IF_End) || c==CF_OPC_ifnull || c==CF_OPC_ifnonnull) )
		{
			// if-xx => ifnot-xx
			dst[l++] = c + (1|(c&1)-1) * (-(c>=CF_OPC_ifnull) | 1);
			dst[l++] = 0;
			dst[l++] = 5; // skip goto_w

			// insert goto_w
			n = locs[i].ext + _MAKE_INT16_( src[_p+1], src[_p+2] );
			dst[l++] = CF_OPC_goto_w;
			dst[l++] = n >> 24;
			dst[l++] = (n >> 16) & 0xFF;
			dst[l++] = (n >> 8) & 0xFF;
			dst[l++] = n & 0xFF;
		}
		else if( locs[i].add == 4 && c>=CF_OPC_ireturn && c<=CF_OPC_return )
		{
			// return => goto_w
			n = exit_pos < 0 ? 0 : exit_pos - l;
			dst[l++] = CF_OPC_goto_w;
			dst[l++] = n >> 24;
			dst[l++] = (n >> 16) & 0xFF;
			dst[l++] = (n >> 8) & 0xFF;
			dst[l++] = n & 0xFF;
		}
		else if( locs[i].add == 2 && (c == CF_OPC_goto || c == CF_OPC_jsr) )
		{
			// goto => goto_w, or jsr => jsr_w
			n = locs[i].ext + _MAKE_INT16_( src[_p+1], src[_p+2] );
			dst[l++] = c + (CF_OPC_goto_w - CF_OPC_goto);
			dst[l++] = n >> 24;
			dst[l++] = (n >> 16) & 0xFF;
			dst[l++] = (n >> 8) & 0xFF;
			dst[l++] = n & 0xFF;
		}
		else if( c == CF_OPC_nop && locs[i].add == 5 )
		{
			// added, 2022-06-23
			locs[i].jmpto = l;  // save the position in the dst buffer

			// insert hole, will be changed later
			dst[l] = CF_OPC_goto_w;
			dst[l+1] = dst[l+2] = dst[l+3] = dst[l+4] = 0;
			l += 5;
		}
		else if( c == CF_OPC_ldc && locs[i].add == 1 )
		{
			// 2022-07-01
			dst[l++] = CF_OPC_ldc_w;
			dst[l++] = 0;
			dst[l++] = src[_p+1];
		}
		else
		{
			// CONTROL instructions
			if( locs[i].ext || c == CF_OPC_tableswitch || c == CF_OPC_lookupswitch )
			{
				dst[l++] = c;

				// alter offset
				switch( c )
				{
				case CF_OPC_tableswitch:{
					int a = (_p + 4) & -4;
					int m = 1 + _MAKE_INT32_( src[a+8], src[a+9], src[a+10], src[a+11] )
							  - _MAKE_INT32_( src[a+4], src[a+5], src[a+6], src[a+7] );

					//
					l = ((l + 3) & -4);
					memcpy( &dst[l], &src[a], 12 + m * 4 );

					if( locs[i].ext )
					{
						// first item is the default
						n = locs[i].ext + _MAKE_INT32_( src[a], src[a+1], src[a+2], src[a+3] );
						dst[l] = n >> 24;
						dst[l+1] = (n >> 16) & 0xFF;
						dst[l+2] = (n >> 8) & 0xFF;
						dst[l+3] = n & 0xFF;
					}

					l += 12;
					for( a+=12; --m >= 0; a+=4 )
					{
						if( locs[++i].ext )
						{
							n = locs[i].ext + _MAKE_INT32_( src[a], src[a+1], src[a+2], src[a+3] );
							dst[l] = n >> 24;
							dst[l+1] = (n >> 16) & 0xFF;
							dst[l+2] = (n >> 8) & 0xFF;
							dst[l+3] = n & 0xFF;
						}
						l += 4;
					}
					break;
				}
				case CF_OPC_lookupswitch:{
					int a = (_p + 4) & -4;
					int m = _MAKE_INT32_( src[a+4], src[a+5], src[a+6], src[a+7] );

					//
					l = ((l + 3) & -4);
					memcpy( &dst[l], &src[a], 8 + m * 8 );

					if( locs[i].ext )
					{
						// first item is the default
						n = locs[i].ext + _MAKE_INT32_( src[a], src[a+1], src[a+2], src[a+3] );
						dst[l] = n >> 24;
						dst[l+1] = (n >> 16) & 0xFF;
						dst[l+2] = (n >> 8) & 0xFF;
						dst[l+3] = n & 0xFF;
					}

					l += 8;
					for( a+=8; --m >= 0; a+=8 )
					{
						if( locs[++i].ext )
						{
							n = locs[i].ext + _MAKE_INT32_( src[a+4], src[a+5], src[a+6], src[a+7] );
							dst[l+4] = n >> 24;
							dst[l+5] = (n >> 16) & 0xFF;
							dst[l+6] = (n >> 8) & 0xFF;
							dst[l+7] = n & 0xFF;
						}
						l += 8;
					}
					break;
				}
				case CF_OPC_goto_w:
				case CF_OPC_jsr_w:
					n = locs[i].ext + _MAKE_INT32_( src[_p+1], src[_p+2], src[_p+3], src[_p+4] );
					dst[l++] = n >> 24;
					dst[l++] = (n >> 16) & 0xFF;
					dst[l++] = (n >> 8) & 0xFF;
					dst[l++] = n & 0xFF;
					break;
				case CF_OPC_goto:
				case CF_OPC_jsr:
				case CF_OPC_ifeq:
				case CF_OPC_ifne:
				case CF_OPC_iflt:
				case CF_OPC_ifge:
				case CF_OPC_ifgt:
				case CF_OPC_ifle:
				case CF_OPC_if_icmpeq:
				case CF_OPC_if_icmpne:
				case CF_OPC_if_icmplt:
				case CF_OPC_if_icmpge:
				case CF_OPC_if_icmpgt:
				case CF_OPC_if_icmple:
				case CF_OPC_if_acmpeq:
				case CF_OPC_if_acmpne:
				case CF_OPC_ifnull:
				case CF_OPC_ifnonnull:
					n = locs[i].ext + _MAKE_INT16_( src[_p+1], src[_p+2] );
					dst[l++] = (n >> 8) & 0xFF;
					dst[l++] = n & 0xFF;
					break;
				case CF_OPC_lcmp:
				case CF_OPC_fcmpl:
				case CF_OPC_fcmpg:
				case CF_OPC_dcmpl:
				case CF_OPC_dcmpg:
					break;
				default:
					return -1;
				}
			}
			else
			{
				// keep unmodified
				memcpy( &dst[l], &src[_p], locs[i].len );
				l += locs[i].len;
			}
		}

		p = _p + locs[i++].len;
	}

	if( l > _l && end_opc_off && end_opc_len )
	{
		// locate the end opcode
		p = cf_seek_end_opcode( &dst[_l], l-_l, end_opc_len );
		*end_opc_off = p + _l - dst_pos;
	}

	return l;
}


// Function: modify constant pool index used by specific opcode
int cf_modify_cp_index( juint8 *code, int len, int cp_off, int *last_opc_len )
{
	register int p, n, i, _p_;

	_p_ = p = n = 0;
	while( p < len )
	{
		if( (n = cf_opc_len_calcu( &code[p], p )) <= 0 )
			break;

		switch( code[p] )
		{
	    case CF_OPC_ldc             : /* it's impossible to appears in byte-code which is compatible with SABI */
	    	i = code[p+1];
	    	code[p+1] = (cp_off + i) & -!!i;
	    	break;
	    case CF_OPC_ldc_w           :
	    case CF_OPC_ldc2_w          :
	    case CF_OPC_getstatic       :
	    case CF_OPC_putstatic       :
	    case CF_OPC_getfield        :
	    case CF_OPC_putfield        :
	    case CF_OPC_invokevirtual   :
	    case CF_OPC_invokespecial   :
	    case CF_OPC_invokestatic    :
	    case CF_OPC_invokeinterface :
	    case CF_OPC_invokedynamic   :
	    case CF_OPC_new             :
	    case CF_OPC_anewarray       :
	    case CF_OPC_checkcast       :
	    case CF_OPC_instanceof      :
	    case CF_OPC_multianewarray  :
	    	i = _MAKE_INT16_( code[p+1], code[p+2] );
	    	i = (cp_off + i) & -!!i;
	    	code[p+1] = (i>>8)&0xff;
	    	code[p+2] = i&0xff;
	    	break;
	    //default:
		}
		_p_ = p;
		p += n;
	}

	if( last_opc_len )
		*last_opc_len = n;

	return _p_;  // last opcode offset
}


// Function: scan constants used in specified code sequences
int cf_scan_constant_refer( juint8 *code, int len, cf_idx_t *mask, int cp_num, int *last_opc_len, int *last_opc_off )
{
	register int p, n, i, _p_, m;

	_p_ = p = n = m = 0;
	while( p < len )
	{
		if( (n = cf_opc_len_calcu( &code[p], p )) <= 0 )
			break;

		switch( code[p] )
		{
	    case CF_OPC_ldc             :
	    	i = code[p+1];
	    	break;
	    case CF_OPC_ldc_w           :
	    case CF_OPC_ldc2_w          :
	    case CF_OPC_getstatic       :
	    case CF_OPC_putstatic       :
	    case CF_OPC_getfield        :
	    case CF_OPC_putfield        :
	    case CF_OPC_invokevirtual   :
	    case CF_OPC_invokespecial   :
	    case CF_OPC_invokestatic    :
	    case CF_OPC_invokeinterface :
	    case CF_OPC_invokedynamic   :
	    case CF_OPC_new             :
	    case CF_OPC_anewarray       :
	    case CF_OPC_checkcast       :
	    case CF_OPC_instanceof      :
	    case CF_OPC_multianewarray  :
	    	i = _MAKE_INT16_( code[p+1], code[p+2] );
	    	break;
	    default:
	    	i = -1;  // will be ignored
	    	break;
		}

		if( i >= 0 && i < cp_num )  //&& mask[i] == 0 )
		{
    		mask[i] = 1;
    		m++;
		}

		_p_ = p;
		p += n;
	}

	if( last_opc_len )
		*last_opc_len = n;
	if( last_opc_off )
		*last_opc_off = _p_;

	return m;
}


#if 0
// Function: seek local variables in byte code
int cf_seek_local_var( const juint8 *code, int len, cf_idx_t *rdvar, cf_idx_t *wrvar, int varmax  )
{
	register int p, n;
	register juint8 c, w;
	int r = 0;

	p = 0;
	while( p < len && r < varmax )
	{
		if( (n = cf_opc_len_calcu( &code[p], p )) <= 0 )
			break;

		c = code[p];
		if( (w = c==CF_OPC_wide) )
		{
			c = code[++p];
			n--;
		}

		if( rdvar )
		{
			if( c >= CF_OPC_iload_0 && c <= CF_OPC_aload_3 )
				rdvar[r++] = (c - CF_OPC_iload_0) & 3;
			else if( c >= CF_OPC_iload && c <= CF_OPC_aload )
				rdvar[r++] = w ? _MAKE_INT16_( code[p+1], code[p+2] ) : code[p+1];
		}

		if( wrvar )
		{
			if( c >= CF_OPC_istore_0 && c <= CF_OPC_astore_3 )
				wrvar[r++] = (c - CF_OPC_istore_0) & 3;
			else if( c >= CF_OPC_istore && c <= CF_OPC_astore )
				wrvar[r++] = w ? _MAKE_INT16_( code[p+1], code[p+2] ) : code[p+1];
		}

		p += n;
	}
	return r;
}
#endif

// Function:
int cf_seek_loc_var( const juint8 *code, int len, cf_idx_t *idx, juint8 store_opc, cf_idx_t var_num, int *opc_len, int *last_opc_len  )
{
	register int p, n, _n;
	register juint8 c, w, _store_opc2_ = CF_OPC_istore_0 + 4 * (store_opc - CF_OPC_istore);

	_n = 0;
	p = 0;
	while( p < len )
	{
		if( (n = cf_opc_len_calcu( &code[p], p )) <= 0 )
			break;

		c = code[p];
		if( (w = c==CF_OPC_wide) )
		{
			c = code[++p];
			n--;
		}

		if( c >= CF_OPC_istore && c <= CF_OPC_astore_3 )
		{
			if( store_opc == c || ((c - _store_opc2_)&0xff) <= 3 || store_opc==0 )
			{
				if( var_num-- < 1 )
				{
					if( c >= CF_OPC_istore_0 && c <= CF_OPC_astore_3 )
						*idx = (c - CF_OPC_istore_0) & 3;
					else //if( c >= CF_OPC_istore && c <= CF_OPC_astore )
						*idx = w ? _MAKE_INT16_( code[p+1], code[p+2] ) : code[p+1];

					*opc_len = n;
					*last_opc_len = _n;
					return p;
				}
			}
		}
		p += n;
		_n = n;
	}
	return -1;
}

//
int cf_seek_opcode( const juint8 *code, int len, juint8 opc, int *opc_len, int *last_opc_len  )
{
	register int p, n, _n;
	_n = 0;
	p = 0;
	while( p < len )
	{
		if( (n = cf_opc_len_calcu( &code[p], p )) <= 0 )
			break;

		if( code[p] == opc )
		{
			*opc_len = n;
			*last_opc_len = _n;
			return p;
		}

		p += n;
		_n = n;
	}
	return -1;
}

//
int cf_seek_end_opcode( const juint8 *code, int len, int *opc_len  )
{
	register int p, _p, n, _n;
	_n = 0;
	p = _p = 0;
	while( p < len && (n = cf_opc_len_calcu( &code[p], p )) > 0 )
	{
		_p = p;
		_n = n;
		p += n;
	}
	*opc_len = _n;
	return _p;
}


// Function: blend source code and stub code as one integrated sequences
// Arguments:
// src_buf: [0]=source byte-code;
//			[1]=Entry-Code going to be inserted;
//			[2]=Exit-Code going to be inserted;
//			[3]=Try-Catch-Code going to be inserted
//			[4]=Head-Code at which prior to the Entry-Code
// src_len: [x] is a length of src_buf[x]
// dst_buf: destination buffer
// dst_sz : original buffer size of the dst_buf
// exp_tab: exception table
// exp_num: number of try-catch entries
// info   : return some important information about transforming
// flags  : 1=change opcode "ldc" to "ldc_w"
// cpi_tpix: cp index of the "TPIX" if needs to be inserted
// tpix   :
// Return: number of bytes lived in the destination if succeed, otherwise -1 will be returned
// Note:
// 1, Verification is prohibited due to performance requirement
// 2, Regardless of existence of any constants

int cf_bytecode_transform_v2( const juint8 *src_buf[],
						   	  const int    *src_len,
						      juint8      **dst_buf,
						      int           dst_sz,
						      CF_ExpInfo   *exp_tab,
						      int           exp_num,
						      CF_CodeTransformInfo *info,
							  uint32		flags,
							  int			cpi_tpix
						      )
{
	const juint8 *src_ptr = src_buf[0];
	const juint8 *entry_ptr = src_buf[1];
	const juint8 *exit_ptr = src_buf[2];
	const juint8 *catch_ptr = src_buf[3];
	const juint8 *head_ptr = src_buf[4];
	int srclen = src_len[0];
	int entry_len = src_len[1];
	int exit_len = src_len[2];
	int catch_len = src_len[3];
	int head_len = src_len[4];
	juint8 *dst_ptr = *dst_buf;
	CF_InsLoc *insLoc = NULL;
	CF_ExpOff *expoff = NULL;
	CF_CodeTransformInfo info2;
	BOOL chg_ldc = flags & 1;

	int insLocNum;
	int ln, k, v, d;


	// save parameters
	memset( &info2, 0, sizeof(info2) );
	info2.cp_off = info->cp_off;
	info2.btsmtd_off = info->btsmtd_off;
	info2.mtd_name = info->mtd_name;
	info2.mtd_desc = info->mtd_desc;

	//if( cpi_tpix > 0 )
	//	cpi_tpix += info2.cp_off;


	// locate all CONTROL instructions
	insLoc = cf_locate_ctrl_instruction( src_ptr, srclen, NULL, &insLocNum, &k, &v, chg_ldc?CF_OPC_ldc:0 );
	if( insLoc == NULL )
		return -1;

	info2.code_last_opc_off = (juint16)k;
	info2.code_last_opc_len = (juint16)v;

	if( exp_tab && exp_num > 0 )
	{
		expoff = malloc( (k=sizeof(CF_ExpOff) * exp_num) );
		if( !expoff )
			return -1;

		memset( expoff, 0, k );
		for( k=0; k < exp_num; k++ )
		{
			expoff[k].start_pc = _CF_ENDIAN_FLIP_U2( exp_tab[k].start_pc );
			expoff[k].end_pc = _CF_ENDIAN_FLIP_U2( exp_tab[k].end_pc );
			expoff[k].handler_pc = _CF_ENDIAN_FLIP_U2( exp_tab[k].handler_pc );
		}
	}

	// calculate maximum length of destination possible
	ln = srclen + head_len + entry_len + exit_len + catch_len + insLocNum * 4 + 64;

	if( dst_sz < ln )
	{
		dst_sz = _ALIGN_SIZE_( ln, 1024 );
		dst_ptr = realloc( dst_ptr, dst_sz );
		if( !dst_ptr )
		{
			if( expoff )
				free( expoff );
			return -1;
		}
	}


	//
	cf_get_return_type( info2.mtd_desc, &info2.ret_tp_idx );
	info2.ret_slots = CF_SIGNATURE_SLOTS[info2.ret_tp_idx];
	ln = 0;

	// ENTRY block
	if( entry_ptr && entry_len > 0 )
	{
#if 1
		if( head_len > 0 )
		{
			// copy HEAD(2022-07-02)
			// ASSUME there are no any local variables in the HEAD
			memcpy( dst_ptr, head_ptr, head_len );

			// modify constant pool index used by HEAD
			cf_modify_cp_index( dst_ptr, head_len, info2.cp_off, NULL );
			ln = head_len;
		}
#endif

		k = ln;

		///////////////////////////////////////////////////////////////////////////////
		int _opclen, _last_opclen, _opcoff;
		cf_idx_t tpix_var = -1;
		d = 0;

		if( cpi_tpix > 0 )
		{
			// seek the local variable "TPIX"
			_opcoff = cf_seek_loc_var( entry_ptr, entry_len, &tpix_var, CF_OPC_lstore, 0, &_opclen, &_last_opclen );
			if( tpix_var != (cf_idx_t)-1 )
			{
				// insert instructions for assigning of "TPIX"
				dst_ptr[k++] = CF_OPC_ldc2_w;
				dst_ptr[k++] = (cpi_tpix >> 8) & 0xff;
				dst_ptr[k++] = cpi_tpix & 0xff;
				memcpy( dst_ptr+k, entry_ptr+_opcoff, _opclen );
				k += _opclen;

				if( _opcoff == _last_opclen )
				{
					// the TPIX at offset 0 in block ENTRY
					entry_ptr += v = _opcoff + _opclen;
					entry_len -= v;
					d += v;
				}
				else
				{
					// for StackMapTable
					info2.ins_locs2[info2.ins_locs_num2].ext = 1;  // mark as block ENTRY
					info2.ins_locs2[info2.ins_locs_num2].pos = d;
					d += info2.ins_locs2[info2.ins_locs_num2++].add = 3 + _opclen;
				}
			}
		}
		///////////////////////////////////////////////////////////////////////////////

		// copy ENTRY
		memcpy( &dst_ptr[k], entry_ptr, entry_len );

		// discards original assignment instructions(TPIX) if possible
		if( tpix_var != (cf_idx_t)-1 && _opcoff != _last_opclen )
			memset( &dst_ptr[k+_opcoff-_last_opclen], CF_OPC_nop, _opclen + _last_opclen );

		// modify constant pool index used by ENTRY
		info2.entry_last_opc_off = (juint16)(cf_modify_cp_index( &dst_ptr[k], entry_len, info2.cp_off, &v ) + (k - ln));
		info2.entry_last_opc_len = (juint16)v;
		//
		info2.entry_pc = ln; // record the pc of ENTRY
		ln = k + entry_len;
	}

	// reserve 4 bytes for instruction "nop" and "goto <source-code>"
	info2.code_pc = ln += 4 - !ln;

	// EXIT block
	if( exit_ptr && exit_len > 0 )
	{
		juint8 *_dstptr_;
		juint8 x = info2.ret_tp_idx;
		//juint8 s = info2.ret_slots;

		k = ln;

		///////////////////////////////////////////////////////////////////////////////
		int _opclen2, _last_opclen2;
		int _opcoff2 = 0;
		cf_idx_t tpix_var = -1;
		d = 0;

		if( cpi_tpix > 0 )
		{
			// seek the local variable "TPIX"
			_opcoff2 = cf_seek_loc_var( exit_ptr, exit_len, &tpix_var, CF_OPC_lstore, 0, &_opclen2, &_last_opclen2 );
			if( tpix_var != (cf_idx_t)-1 )
			{
				// insert instructions for assigning of "TPIX"
				dst_ptr[k++] = CF_OPC_ldc2_w;
				dst_ptr[k++] = (cpi_tpix >> 8) & 0xff;
				dst_ptr[k++] = cpi_tpix & 0xff;
				memcpy( dst_ptr+k, exit_ptr+_opcoff2, _opclen2 );
				k += _opclen2;

				if( _opcoff2 == _last_opclen2 )
				{
					// the TPIX at offset 0 in block EXIT
					exit_ptr += v = _opcoff2 + _opclen2;
					exit_len -= v;
					d += v;
				}
				else
				{
					// for StackMapTable
					info2.ins_locs2[info2.ins_locs_num2].ext = 2;  // mark as block EXIT
					info2.ins_locs2[info2.ins_locs_num2].pos = d;
					d += info2.ins_locs2[info2.ins_locs_num2++].add = 3 + _opclen2;
				}
			}
		}
		///////////////////////////////////////////////////////////////////////////////

		int _opclen, _last_opclen;
		int _opcoff = 0;
		cf_idx_t retvar = -1;

		info2.exit_ret_off = 0xffff;

		if( info2.ret_slots )
		{
			// seek the local variable going to be returned
			_opcoff = cf_seek_loc_var( exit_ptr, exit_len, &retvar, CF_IDX_TO_OPC_STORE[x],
									   (tpix_var != (cf_idx_t)-1 && exit_len==src_len[2] && CF_IDX_TO_OPC_STORE[x]==CF_OPC_lstore),
									   &_opclen, &_last_opclen );
			if( retvar != (cf_idx_t)-1 )
			{
				// changed, 2022-06-27
				info2.exit_ret_off = k - ln;

				_dstptr_ = dst_ptr + k;

				// insert an opcode "store"(return value)
				if( retvar < 4 )
				{
					// <x>store_<n>
					_dstptr_[0] = (juint8)(CF_OPC_istore_0 + (CF_IDX_TO_OPC_STORE[x] - CF_OPC_istore) * 4 + retvar);
					k++;
				}
				else if( retvar < 256 )
				{
					// <x>store index
					_dstptr_[0] = CF_IDX_TO_OPC_STORE[x];
					_dstptr_[1] = retvar;
					k += 2;
				}
				else
				{
					// wide <x>store index-high index-low
					_dstptr_[0] = CF_OPC_wide;
					_dstptr_[1] = CF_IDX_TO_OPC_STORE[x];
					_dstptr_[2] = retvar>>8;
					_dstptr_[3] = retvar&0xff;
					k += 4;
				}
				// For StackMapTable
				info2.ins_locs2[info2.ins_locs_num2].ext = 2;  // mark as block EXIT
				info2.ins_locs2[info2.ins_locs_num2].pos = d;
				d += info2.ins_locs2[info2.ins_locs_num2++].add = &dst_ptr[k] - _dstptr_;
			}
		}

		// copy EXIT
		memcpy( &dst_ptr[k], exit_ptr, exit_len );

		// mask original assignment instructions(return value) if possible
		if( retvar != (cf_idx_t)-1 )
			memset( &dst_ptr[k+_opcoff-_last_opclen], CF_OPC_nop, _opclen + _last_opclen );

		// mask original assignment instructions(TPIX) if possible
		if( tpix_var != (cf_idx_t)-1 && _opcoff2 != _last_opclen2 )
			memset( &dst_ptr[k+_opcoff2-_last_opclen2], CF_OPC_nop, _opclen2 + _last_opclen2 );

		// modify constant pool index used by EXIT
		info2.exit_last_opc_off = (juint16)(cf_modify_cp_index( &dst_ptr[k], exit_len, info2.cp_off, &v ) + (k - ln));
		info2.exit_last_opc_len = (juint16)v;
		//
		d += exit_len;
		k += exit_len;
		v = dst_ptr[ln + info2.exit_last_opc_off];
		if( v != CF_OPC_athrow && (v < CF_OPC_ireturn || v > CF_OPC_return) )
		{
			// added, 2022-06-27
			_dstptr_ = dst_ptr + k;

			if( retvar != (cf_idx_t)-1 )
			{
				// insert an opcode "load"(return value)
				if( retvar < 4 )
				{
					// <x>load_<n>
					_dstptr_[0] = (juint8)(CF_OPC_iload_0 + (CF_IDX_TO_OPC_LOAD[x] - CF_OPC_iload) * 4 + retvar);
					k++;
				}
				else if( retvar < 256 )
				{
					// <x>load index
					_dstptr_[0] = CF_IDX_TO_OPC_LOAD[x];
					_dstptr_[1] = retvar;
					k += 2;
				}
				else
				{
					// wide <x>load index-high index-low
					_dstptr_[0] = CF_OPC_wide;
					_dstptr_[1] = CF_IDX_TO_OPC_LOAD[x];
					_dstptr_[2] = retvar>>8;
					_dstptr_[3] = retvar&0xff;
					k += 4;
				}
			}
			dst_ptr[k++] = CF_IDX_TO_OPC_RETURN[x];  // always append opcode "[x]return"

			v = &dst_ptr[k] - _dstptr_;
			info2.exit_last_opc_off += info2.exit_last_opc_len + v - 1;
			info2.exit_last_opc_len = CF_OPC_LEN[CF_OPC_return];

			// for StackMapTable
			info2.ins_locs2[info2.ins_locs_num2].ext = 2;  // mark as block EXIT
			info2.ins_locs2[info2.ins_locs_num2].pos = d;
			/*d +=*/ info2.ins_locs2[info2.ins_locs_num2++].add = v;
		}

		// always append a separator "nop"
		dst_ptr[k++] = CF_OPC_nop;

		if( catch_ptr && catch_len > 0 )
		{
			// append a separator "goto 0" between the EXIT and the CATCH
			dst_ptr[k++] = CF_OPC_goto;
			dst_ptr[k++] = 0;
			dst_ptr[k++] = 0;
		}

		info2.exit_pc = ln; // record the pc of the EXIT
		ln = k;
	}


	// CATCH block
	if( catch_ptr && catch_len > 0 )
	{
		k = ln;

		///////////////////////////////////////////////////////////////////////////////
		int _opclen, _last_opclen, _opcoff;
		cf_idx_t tpix_var = -1;
		d = 0;

		if( cpi_tpix > 0 )
		{
			// seek the local variable "TPIX"
			_opcoff = cf_seek_loc_var( catch_ptr, catch_len, &tpix_var, CF_OPC_lstore, 0, &_opclen, &_last_opclen );
			if( tpix_var != (cf_idx_t)-1 )
			{
				// copy instructions before the "lstore"
				memcpy( dst_ptr+k, catch_ptr, (v=_opcoff-_last_opclen) );
				k += v;
				d += v;

				// insert instructions for assigning of "TPIX"
				dst_ptr[k++] = CF_OPC_ldc2_w;
				dst_ptr[k++] = (cpi_tpix >> 8) & 0xff;
				dst_ptr[k++] = cpi_tpix & 0xff;
				memcpy( dst_ptr+k, catch_ptr+_opcoff, _opclen );
				k += _opclen;

				if( _last_opclen != 3 )
				{
					// for StackMapTable
					info2.ins_locs2[info2.ins_locs_num2].ext = 3;  // mark as block CATCH
					info2.ins_locs2[info2.ins_locs_num2].pos = d;
					d += info2.ins_locs2[info2.ins_locs_num2++].add = 3 - _last_opclen;
				}

				// always skip the original "lstore"
				catch_ptr += v = _opcoff + _opclen;
				catch_len -= v;
				d += v;
			}
		}

		///////////////////////////////////////////////////////////////////////////////

		// copy CATCH
		memcpy( &dst_ptr[k], catch_ptr, catch_len );

		// modify constant pool index used by CATCH
		info2.catch_last_opc_off = (juint16)(cf_modify_cp_index( &dst_ptr[k], catch_len, info2.cp_off, &v ) + (k - ln));
		info2.catch_last_opc_len = (juint16)v;
		//
		d += catch_len;
		k += catch_len;
		v = dst_ptr[ln + info2.catch_last_opc_off];
		if( v != CF_OPC_athrow && (v < CF_OPC_ireturn || v > CF_OPC_return) )
		{
			dst_ptr[k++] = CF_OPC_athrow;  // append opcode "athrow"

			info2.catch_last_opc_off += info2.catch_last_opc_len;
			info2.catch_last_opc_len = 1; //CF_OPC_LEN[CF_OPC_athrow];

			// for StackMapTable
			info2.ins_locs2[info2.ins_locs_num2].ext = 3;  // mark as block CATCH
			info2.ins_locs2[info2.ins_locs_num2].pos = d;
			/*d +=*/ info2.ins_locs2[info2.ins_locs_num2++].add = 1; //info2.catch_last_opc_len;
		}

		// always append a separator "nop" between the CATCH and the CODE
		dst_ptr[k++] = CF_OPC_nop;

		info2.catch_pc = ln; // record the pc of CATCH
		ln = k;
	}

	if( ln > 3 )
	{
		if( info2.entry_last_opc_len )
			dst_ptr[info2.code_pc-4] = CF_OPC_nop;  // append a separator "nop" if has the block ENTRY

		if( info2.code_pc != ln )
		{
			// with the EXIT and/or the CATCH
			// instruction: goto the start pc of original code
			juint8 *_dstptr_ = &dst_ptr[info2.code_pc - 3];
			k = ln - info2.code_pc + 3;
			_dstptr_[0] = CF_OPC_goto;
			_dstptr_[1] = (k>>8)&0xff;
			_dstptr_[2] = k&0xff;
		}
		else
		{
			// without the EXIT and the CATCH
			ln -= 3;
		}
	}
	else
		ln = 0;  // without the ENTRY, the EXIT and the CATCH(nothing be inserted)


	// Original CODE block
	info2.code_pc = ln; // record the pc of ORGINAL byte-code

	// alter instructions offset and modify the exception table if possible
	cf_alter_instruction_offset( insLoc, insLocNum, info2.code_pc, info2.exit_pc, expoff, exp_num );

	// transform original instructions
	ln = cf_assemble_instruction( src_ptr, srclen, dst_ptr, dst_sz, insLoc, insLocNum, info2.code_pc, info2.exit_pc, &k, &v );
	if( ln > 0 )
	{
		if( info2.catch_pc )
		{
			info2.catch_beg_pc = info2.code_pc;
			info2.catch_end_pc = ln;
		}

		info2.code_ln = ln - info2.code_pc; // record the length of TRANSFORMED byte-code

		info2.code_last_opc_off = (juint16)k;
		info2.code_last_opc_len = (juint16)v;

		if( expoff )
		{
			// alter exception table
			int pc;
			for( k=0; k < exp_num; k++ )
			{
				pc = expoff[k].start_pc + expoff[k].start_off + info2.code_pc;
				exp_tab[k].start_pc = _CF_ENDIAN_FLIP_U2( pc );

				pc = expoff[k].end_pc + expoff[k].end_off + info2.code_pc;
				exp_tab[k].end_pc = _CF_ENDIAN_FLIP_U2( pc );

				pc = expoff[k].handler_pc + expoff[k].handler_off + info2.code_pc;
				exp_tab[k].handler_pc = _CF_ENDIAN_FLIP_U2( pc );
			}
		}

		*dst_buf = dst_ptr;

		info2.ins_locs = insLoc;
		info2.ins_locs_num = insLocNum;

		memcpy( info, &info2, sizeof(info2) );
	}
	else
	{
		// failed
		if( *dst_buf == NULL )
			free( dst_ptr );  // free destination buffer if it was allocated at the beginning
	}

	if( insLoc && info2.ins_locs == NULL )
		free( insLoc );
	if( expoff )
		free( expoff );

	return ln;
}

int cf_bytecode_transform( const juint8 *src_buf[],
						   const int    *src_len,
						   juint8      **dst_buf,
						   int           dst_sz,
						   CF_ExpInfo   *exp_tab,
						   int           exp_num,
						   CF_CodeTransformInfo *info
						   )
{
	return cf_bytecode_transform_v2( src_buf, src_len, dst_buf, dst_sz, exp_tab, exp_num, info, 0, -1 );
}

//
static void _modify_gotow_offset( juint8 *code, int len, int new_pos, int shift )
{
	register int p, n, off;

	p = n = 0;
	while( p < len )
	{
		if( (n = cf_opc_len_calcu( &code[p], p )) <= 0 )
			break;
		if( code[p] == CF_OPC_goto_w )
		{
			if( !(code[p+1] | code[p+2] | code[p+3] | code[p+4]) )
			{
				off = new_pos - p + shift;
				code[p+1] = (off>>24)&0xff;
				code[p+2] = (off>>16)&0xff;
				code[p+3] = (off>>8)&0xff;
				code[p+4] = off&0xff;
			}
		}
		p += n;
	}
}

// temporary approach for special methods, such as object constructor "<init>" that requires stub code be inserted behind source code
int cf_bytecode_transform_v3( const juint8 *src_buf[],
						   	  const int    *src_len,
						      juint8      **dst_buf,
						      int           dst_sz,
						      CF_ExpInfo   *exp_tab,
						      int           exp_num,
						      CF_CodeTransformInfo *info,
							  uint32		flags,
							  int			cpi_tpix
						      )
{
	const juint8 *src_ptr = src_buf[0];
	const juint8 *entry_ptr = src_buf[1];
	const juint8 *exit_ptr = src_buf[2];
	const juint8 *catch_ptr = src_buf[3];
	const juint8 *head_ptr = src_buf[4];
	int srclen = src_len[0];
	int entry_len = src_len[1];
	int exit_len = src_len[2];
	int catch_len = src_len[3];
	int head_len = src_len[4];
	juint8 *dst_ptr = *dst_buf;
	CF_InsLoc *insLoc = NULL;
	CF_ExpOff *expoff = NULL;
	CF_CodeTransformInfo info2;
	BOOL chg_ldc = flags & 1;

	int insLocNum;
	int ln, k, v, d;


	// save parameters
	memset( &info2, 0, sizeof(info2) );
	info2.cp_off = info->cp_off;
	info2.btsmtd_off = info->btsmtd_off;
	info2.mtd_name = info->mtd_name;
	info2.mtd_desc = info->mtd_desc;


	// locate all CONTROL instructions
	insLoc = cf_locate_ctrl_instruction( src_ptr, srclen, NULL, &insLocNum, &k, &v, chg_ldc?CF_OPC_ldc:0 );
	if( insLoc == NULL )
		return -1;

	info2.code_last_opc_off = (juint16)k;
	info2.code_last_opc_len = (juint16)v;

	if( exp_tab && exp_num > 0 )
	{
		expoff = malloc( (k=sizeof(CF_ExpOff) * exp_num) );
		if( !expoff )
			return -1;

		memset( expoff, 0, k );
		for( k=0; k < exp_num; k++ )
		{
			expoff[k].start_pc = _CF_ENDIAN_FLIP_U2( exp_tab[k].start_pc );
			expoff[k].end_pc = _CF_ENDIAN_FLIP_U2( exp_tab[k].end_pc );
			expoff[k].handler_pc = _CF_ENDIAN_FLIP_U2( exp_tab[k].handler_pc );
		}
	}

	// calculate maximum length of destination possible
	ln = srclen + head_len + entry_len + exit_len + catch_len + insLocNum * 4 + 64;

	if( dst_sz < ln )
	{
		dst_sz = _ALIGN_SIZE_( ln, 1024 );
		dst_ptr = realloc( dst_ptr, dst_sz );
		if( !dst_ptr )
		{
			if( expoff )
				free( expoff );
			return -1;
		}
	}

	//
	cf_get_return_type( info2.mtd_desc, &info2.ret_tp_idx );
	info2.ret_slots = CF_SIGNATURE_SLOTS[info2.ret_tp_idx];
	ln = 0;

	info2.exit_pc = exit_ptr && exit_len > 0 ? srclen : 0;

	// Original CODE block
	info2.code_pc = entry_ptr && entry_len > 0 ? 5 : 0; // record the pc of ORGINAL byte-code

	// alter instructions offset and modify the exception table if possible
	cf_alter_instruction_offset( insLoc, insLocNum, info2.code_pc, info2.exit_pc, expoff, exp_num );

	// transform original instructions
	ln = cf_assemble_instruction( src_ptr, srclen, dst_ptr, dst_sz, insLoc, insLocNum, info2.code_pc, -1, &k, &v );
	if( ln > 0 )
	{
		info2.code_ln = ln - info2.code_pc; // record the length of TRANSFORMED byte-code
		info2.code_last_opc_off = (juint16)k;
		info2.code_last_opc_len = (juint16)v;

		if( expoff )
		{
			// alter exception table
			int pc;
			for( k=0; k < exp_num; k++ )
			{
				pc = expoff[k].start_pc + expoff[k].start_off + info2.code_pc;
				exp_tab[k].start_pc = _CF_ENDIAN_FLIP_U2( pc );

				pc = expoff[k].end_pc + expoff[k].end_off + info2.code_pc;
				exp_tab[k].end_pc = _CF_ENDIAN_FLIP_U2( pc );

				pc = expoff[k].handler_pc + expoff[k].handler_off + info2.code_pc;
				exp_tab[k].handler_pc = _CF_ENDIAN_FLIP_U2( pc );
			}
		}

		info2.ins_locs = insLoc;
		info2.ins_locs_num = insLocNum;
	}
	else
	{
		// failed
		if( *dst_buf == NULL )
			free( dst_ptr );  // free destination buffer if it was allocated at the beginning
		goto LAB_EXIT;
	}

	if( catch_ptr && catch_len > 0 )
	{
		info2.catch_beg_pc = info2.code_pc;
		info2.catch_end_pc = ln;  // Exception Block include only the ORIGINAL CODE

		if( strcmp(info2.mtd_name, "<init>") == 0 )
		{
			k = cf_seek_opcode( &dst_ptr[info2.code_pc], info2.code_ln, CF_OPC_invokespecial, &v, &d );
			if( k > 0 )
				info2.catch_beg_pc += k + v;
		}
	}

	// always insert a separator "nop" behind the source code
	dst_ptr[ln++] = CF_OPC_nop;

	// ENTRY block
	if( entry_ptr && entry_len > 0 )
	{
		if( head_len > 0 )
		{
			// copy HEAD(2022-07-02)
			// ASSUME there are no any local variables in the HEAD
			memcpy( &dst_ptr[ln], head_ptr, head_len );

			// modify constant pool index used by HEAD
			cf_modify_cp_index( &dst_ptr[ln], head_len, info2.cp_off, NULL );
			ln += head_len;
		}

		k = ln;

		///////////////////////////////////////////////////////////////////////////////
		int _opclen, _last_opclen, _opcoff;
		cf_idx_t tpix_var = -1;
		d = 0;

		if( cpi_tpix > 0 )
		{
			// seek the local variable "TPIX"
			_opcoff = cf_seek_loc_var( entry_ptr, entry_len, &tpix_var, CF_OPC_lstore, 0, &_opclen, &_last_opclen );
			if( tpix_var != (cf_idx_t)-1 )
			{
				// insert instructions for assigning of "TPIX"
				dst_ptr[k++] = CF_OPC_ldc2_w;
				dst_ptr[k++] = (cpi_tpix >> 8) & 0xff;
				dst_ptr[k++] = cpi_tpix & 0xff;
				memcpy( dst_ptr+k, entry_ptr+_opcoff, _opclen );
				k += _opclen;

				if( _opcoff == _last_opclen )
				{
					// the TPIX at offset 0 in block ENTRY
					entry_ptr += v = _opcoff + _opclen;
					entry_len -= v;
					d += v;
				}
				else
				{
					// for StackMapTable
					info2.ins_locs2[info2.ins_locs_num2].ext = 1;  // mark as block ENTRY
					info2.ins_locs2[info2.ins_locs_num2].pos = d;
					d += info2.ins_locs2[info2.ins_locs_num2++].add = 3 + _opclen;
				}
			}
		}
		///////////////////////////////////////////////////////////////////////////////

		// copy ENTRY
		memcpy( &dst_ptr[k], entry_ptr, entry_len );

		// discards original assignment instructions(TPIX) if possible
		if( tpix_var != (cf_idx_t)-1 && _opcoff != _last_opclen )
			memset( &dst_ptr[k+_opcoff-_last_opclen], CF_OPC_nop, _opclen + _last_opclen );

		// modify constant pool index used by ENTRY
		info2.entry_last_opc_off = (juint16)(cf_modify_cp_index( &dst_ptr[k], entry_len, info2.cp_off, &v ) + (k - ln));
		info2.entry_last_opc_len = (juint16)v;
		//
		info2.entry_pc = ln - head_len; // record the pc of ENTRY
		ln = k + entry_len;

		//info2.catch_end_pc = ln + 1;  Exception Block desn't include the ENTRY

		//
		juint8 *_dstptr_ = &dst_ptr[ln];
		_dstptr_[0] = CF_OPC_nop;  // append a separator "nop"
		k = 4 - ln;  // -(ln + 1(nop) - 5(goto_w))
		_dstptr_[1] = CF_OPC_goto_w;  // goto the start pc of original code
		_dstptr_[2] = (k>>24)&0xff;
		_dstptr_[3] = (k>>16)&0xff;
		_dstptr_[4] = (k>>8)&0xff;
		_dstptr_[5] = k&0xff;
		ln += 6;

		// instruction: goto the entry pc
		_dstptr_ = &dst_ptr[info2.code_pc - 5]; // 0
		k = info2.entry_pc - 1;  // jump to the separator "nop"
		_dstptr_[0] = CF_OPC_goto_w;
		_dstptr_[1] = (k>>24)&0xff;
		_dstptr_[2] = (k>>16)&0xff;
		_dstptr_[3] = (k>>8)&0xff;
		_dstptr_[4] = k&0xff;
		//_dstptr_[5] = CF_OPC_nop;
	}


	// EXIT block
	if( exit_ptr && exit_len > 0 )
	{
		juint8 *_dstptr_;
		juint8 x = info2.ret_tp_idx;
		//juint8 s = info2.ret_slots;

		k = ln;

		///////////////////////////////////////////////////////////////////////////////
		int _opclen2, _last_opclen2;
		int _opcoff2 = 0;
		cf_idx_t tpix_var = -1;
		d = 0;

		if( cpi_tpix > 0 )
		{
			// seek the local variable "TPIX"
			_opcoff2 = cf_seek_loc_var( exit_ptr, exit_len, &tpix_var, CF_OPC_lstore, 0, &_opclen2, &_last_opclen2 );
			if( tpix_var != (cf_idx_t)-1 )
			{
				// insert instructions for assigning of "TPIX"
				dst_ptr[k++] = CF_OPC_ldc2_w;
				dst_ptr[k++] = (cpi_tpix >> 8) & 0xff;
				dst_ptr[k++] = cpi_tpix & 0xff;
				memcpy( dst_ptr+k, exit_ptr+_opcoff2, _opclen2 );
				k += _opclen2;

				if( _opcoff2 == _last_opclen2 )
				{
					// the TPIX at offset 0 in block EXIT
					exit_ptr += v = _opcoff2 + _opclen2;
					exit_len -= v;
					d += v;
				}
				else
				{
					// for StackMapTable
					info2.ins_locs2[info2.ins_locs_num2].ext = 2;  // mark as block EXIT
					info2.ins_locs2[info2.ins_locs_num2].pos = d;
					d += info2.ins_locs2[info2.ins_locs_num2++].add = 3 + _opclen2;
				}
			}
		}
		///////////////////////////////////////////////////////////////////////////////

		int _opclen, _last_opclen;
		int _opcoff = 0;
		cf_idx_t retvar = -1;

		info2.exit_ret_off = 0xffff;

		if( info2.ret_slots )
		{
			// seek the local variable going to be returned
			_opcoff = cf_seek_loc_var( exit_ptr, exit_len, &retvar, CF_IDX_TO_OPC_STORE[x],
									   (tpix_var != (cf_idx_t)-1 && exit_len==src_len[2] && CF_IDX_TO_OPC_STORE[x]==CF_OPC_lstore),
									   &_opclen, &_last_opclen );
			if( retvar != (cf_idx_t)-1 )
			{
				// changed, 2022-06-27
				info2.exit_ret_off = k - ln;

				_dstptr_ = dst_ptr + k;

				// insert an opcode "store"(return value)
				if( retvar < 4 )
				{
					// <x>store_<n>
					_dstptr_[0] = (juint8)(CF_OPC_istore_0 + (CF_IDX_TO_OPC_STORE[x] - CF_OPC_istore) * 4 + retvar);
					k++;
				}
				else if( retvar < 256 )
				{
					// <x>store index
					_dstptr_[0] = CF_IDX_TO_OPC_STORE[x];
					_dstptr_[1] = retvar;
					k += 2;
				}
				else
				{
					// wide <x>store index-high index-low
					_dstptr_[0] = CF_OPC_wide;
					_dstptr_[1] = CF_IDX_TO_OPC_STORE[x];
					_dstptr_[2] = retvar>>8;
					_dstptr_[3] = retvar&0xff;
					k += 4;
				}
				// For StackMapTable
				info2.ins_locs2[info2.ins_locs_num2].ext = 2;  // mark as block EXIT
				info2.ins_locs2[info2.ins_locs_num2].pos = d;
				d += info2.ins_locs2[info2.ins_locs_num2++].add = &dst_ptr[k] - _dstptr_;
			}
		}

		// copy EXIT
		memcpy( &dst_ptr[k], exit_ptr, exit_len );

		// mask original assignment instructions(return value) if possible
		if( retvar != (cf_idx_t)-1 )
			memset( &dst_ptr[k+_opcoff-_last_opclen], CF_OPC_nop, _opclen + _last_opclen );

		// mask original assignment instructions(TPIX) if possible
		if( tpix_var != (cf_idx_t)-1 && _opcoff2 != _last_opclen2 )
			memset( &dst_ptr[k+_opcoff2-_last_opclen2], CF_OPC_nop, _opclen2 + _last_opclen2 );

		// modify constant pool index used by EXIT
		info2.exit_last_opc_off = (juint16)(cf_modify_cp_index( &dst_ptr[k], exit_len, info2.cp_off, &v ) + (k - ln));
		info2.exit_last_opc_len = (juint16)v;
		//
		d += exit_len;
		k += exit_len;
		v = dst_ptr[ln + info2.exit_last_opc_off];
		if( v != CF_OPC_athrow && (v < CF_OPC_ireturn || v > CF_OPC_return) )
		{
			// added, 2022-06-27
			_dstptr_ = dst_ptr + k;

			if( retvar != (cf_idx_t)-1 )
			{
				// insert an opcode "load"(return value)
				if( retvar < 4 )
				{
					// <x>load_<n>
					_dstptr_[0] = (juint8)(CF_OPC_iload_0 + (CF_IDX_TO_OPC_LOAD[x] - CF_OPC_iload) * 4 + retvar);
					k++;
				}
				else if( retvar < 256 )
				{
					// <x>load index
					_dstptr_[0] = CF_IDX_TO_OPC_LOAD[x];
					_dstptr_[1] = retvar;
					k += 2;
				}
				else
				{
					// wide <x>load index-high index-low
					_dstptr_[0] = CF_OPC_wide;
					_dstptr_[1] = CF_IDX_TO_OPC_LOAD[x];
					_dstptr_[2] = retvar>>8;
					_dstptr_[3] = retvar&0xff;
					k += 4;
				}
			}
			dst_ptr[k++] = CF_IDX_TO_OPC_RETURN[x];  // always append opcode "[x]return"

			v = &dst_ptr[k] - _dstptr_;
			info2.exit_last_opc_off += info2.exit_last_opc_len + v - 1;
			info2.exit_last_opc_len = CF_OPC_LEN[CF_OPC_return];

			// for StackMapTable
			info2.ins_locs2[info2.ins_locs_num2].ext = 2;  // mark as block EXIT
			info2.ins_locs2[info2.ins_locs_num2].pos = d;
			/*d +=*/ info2.ins_locs2[info2.ins_locs_num2++].add = v;
		}

		if( catch_ptr && catch_len > 0 )
		{
			//info2.catch_end_pc = k + 1;  Exception Block desn't include the EXIT

			// append a separator "nop"
			dst_ptr[k++] = CF_OPC_nop;

			// append a separator "goto 0" between the EXIT and the CATCH
			dst_ptr[k++] = CF_OPC_goto;
			dst_ptr[k++] = 0;
			dst_ptr[k++] = 0;
		}

		info2.exit_pc = ln; // record the pc of the EXIT
		ln = k;

		//
		_modify_gotow_offset( &dst_ptr[info2.code_pc], info2.code_ln, info2.exit_pc, -info2.code_pc );
	}


	// CATCH block
	if( catch_ptr && catch_len > 0 )
	{
		k = ln;

		///////////////////////////////////////////////////////////////////////////////
		int _opclen, _last_opclen, _opcoff;
		cf_idx_t tpix_var = -1;
		d = 0;

		if( cpi_tpix > 0 )
		{
			// seek the local variable "TPIX"
			_opcoff = cf_seek_loc_var( catch_ptr, catch_len, &tpix_var, CF_OPC_lstore, 0, &_opclen, &_last_opclen );
			if( tpix_var != (cf_idx_t)-1 )
			{
				// copy instructions before the "lstore"
				memcpy( dst_ptr+k, catch_ptr, (v=_opcoff-_last_opclen) );
				k += v;
				d += v;

				// insert instructions for assigning of "TPIX"
				dst_ptr[k++] = CF_OPC_ldc2_w;
				dst_ptr[k++] = (cpi_tpix >> 8) & 0xff;
				dst_ptr[k++] = cpi_tpix & 0xff;
				memcpy( dst_ptr+k, catch_ptr+_opcoff, _opclen );
				k += _opclen;

				if( _last_opclen != 3 )
				{
					// for StackMapTable
					info2.ins_locs2[info2.ins_locs_num2].ext = 3;  // mark as block CATCH
					info2.ins_locs2[info2.ins_locs_num2].pos = d;
					d += info2.ins_locs2[info2.ins_locs_num2++].add = 3 - _last_opclen;
				}

				// always skip the original "lstore"
				catch_ptr += v = _opcoff + _opclen;
				catch_len -= v;
				d += v;
			}
		}

		///////////////////////////////////////////////////////////////////////////////

		// copy CATCH
		memcpy( &dst_ptr[k], catch_ptr, catch_len );

		// modify constant pool index used by CATCH
		info2.catch_last_opc_off = (juint16)(cf_modify_cp_index( &dst_ptr[k], catch_len, info2.cp_off, &v ) + (k - ln));
		info2.catch_last_opc_len = (juint16)v;
		//
		d += catch_len;
		k += catch_len;
		v = dst_ptr[ln + info2.catch_last_opc_off];
		if( v != CF_OPC_athrow && (v < CF_OPC_ireturn || v > CF_OPC_return) )
		{
			dst_ptr[k++] = CF_OPC_athrow;  // append opcode "athrow"

			info2.catch_last_opc_off += info2.catch_last_opc_len;
			info2.catch_last_opc_len = 1; //CF_OPC_LEN[CF_OPC_athrow];

			// for StackMapTable
			info2.ins_locs2[info2.ins_locs_num2].ext = 3;  // mark as block CATCH
			info2.ins_locs2[info2.ins_locs_num2].pos = d;
			/*d +=*/ info2.ins_locs2[info2.ins_locs_num2++].add = 1; //info2.catch_last_opc_len;
		}

		info2.catch_pc = ln; // record the pc of CATCH
		//info2.catch_end_pc = ln;  //k;
		ln = k;
	}

	*dst_buf = dst_ptr;
	memcpy( info, &info2, sizeof(info2) );

	LAB_EXIT:
	if( insLoc && info2.ins_locs == NULL )
		free( insLoc );
	if( expoff )
		free( expoff );

	return ln;
}



// Function:
int cf_count_smf_delta( CF_Attr_StackMapFrame_Union **smf, int num )
{
	int i, d=0;
	for( i=0; i < num; i++ )
	{
		switch( smf[i]->frame_type )
		{
		case CF_StackMapFrame_SameLocals_1_Stack_Item_Ext:
			d += _CF_ENDIAN_FLIP_U2( smf[i]->same_locals_1_stack_item_frame_ext.delta ) + 1;
			break;
		case CF_StackMapFrame_ChopFrame_Begin:
		case CF_StackMapFrame_ChopFrame_Begin+1:
		case CF_StackMapFrame_ChopFrame_Begin+2:
			d += _CF_ENDIAN_FLIP_U2( smf[i]->chop_frame.delta ) + 1;
			break;
		case CF_StackMapFrame_SameFrame_Ext:
			d += _CF_ENDIAN_FLIP_U2( smf[i]->same_frame_ext.delta ) + 1;
			break;
		case CF_StackMapFrame_AppendFrame_Begin:
		case CF_StackMapFrame_AppendFrame_Begin+1:
		case CF_StackMapFrame_AppendFrame_Begin+2:
			d += _CF_ENDIAN_FLIP_U2( smf[i]->append_frame.delta ) + 1;
			break;
		case CF_StackMapFrame_FullFrame:
			d += _CF_ENDIAN_FLIP_U2( smf[i]->full_frame.delta ) + 1;
			break;
		default:
			if( smf[i]->frame_type >= CF_StackMapFrame_SameFrame_Begin &&
				smf[i]->frame_type <= CF_StackMapFrame_SameFrame_End )
			{
				d += smf[i]->frame_type + 1;
			}
			else if( smf[i]->frame_type >= CF_StackMapFrame_SameLocals_1_Stack_Item_Begin &&
					 smf[i]->frame_type <= CF_StackMapFrame_SameLocals_1_Stack_Item_End )
			{
				d += smf[i]->frame_type - CF_StackMapFrame_SameLocals_1_Stack_Item_Begin + 1;
			}
		}
	}

	return d;
}


// Function:
int cf_alter_smf_delta( CF_Attr_StackMapFrame_Union *smf, int shift, BOOL flip_endian )
{
	switch( smf->frame_type )
	{
	case CF_StackMapFrame_SameLocals_1_Stack_Item_Ext:
		shift += (int)(flip_endian ? _CF_ENDIAN_FLIP_U2(smf->same_locals_1_stack_item_frame_ext.delta) : smf->same_locals_1_stack_item_frame_ext.delta);
		if( shift < 0 || shift > 0xFFFF )
			break;
		smf->same_locals_1_stack_item_frame_ext.delta = _CF_ENDIAN_FLIP_U2( shift );
		return shift;
	case CF_StackMapFrame_ChopFrame_Begin:
	case CF_StackMapFrame_ChopFrame_Begin+1:
	case CF_StackMapFrame_ChopFrame_Begin+2:
		shift += (int)(flip_endian ? _CF_ENDIAN_FLIP_U2(smf->chop_frame.delta) : smf->chop_frame.delta);
		if( shift < 0 || shift > 0xFFFF )
			break;
		smf->chop_frame.delta = _CF_ENDIAN_FLIP_U2( shift );
		return shift;
	case CF_StackMapFrame_SameFrame_Ext:
		shift += (int)(flip_endian ? _CF_ENDIAN_FLIP_U2(smf->same_frame_ext.delta) : smf->same_frame_ext.delta);
		if( shift < 0 || shift > 0xFFFF )
			break;
		smf->same_frame_ext.delta = _CF_ENDIAN_FLIP_U2( shift );
		return shift;
	case CF_StackMapFrame_AppendFrame_Begin:
	case CF_StackMapFrame_AppendFrame_Begin+1:
	case CF_StackMapFrame_AppendFrame_Begin+2:
		shift += (int)(flip_endian ? _CF_ENDIAN_FLIP_U2(smf->append_frame.delta) : smf->append_frame.delta);
		if( shift < 0 || shift > 0xFFFF )
			break;
		smf->append_frame.delta = _CF_ENDIAN_FLIP_U2( shift );
		return shift;
	case CF_StackMapFrame_FullFrame:
		shift += (int)(flip_endian ? _CF_ENDIAN_FLIP_U2(smf->full_frame.delta) : smf->full_frame.delta);
		if( shift < 0 || shift > 0xFFFF )
			break;
		smf->full_frame.delta = _CF_ENDIAN_FLIP_U2( shift );
		return shift;
	default:
		if( smf->frame_type >= CF_StackMapFrame_SameFrame_Begin &&
			smf->frame_type <= CF_StackMapFrame_SameFrame_End )
		{
			shift += (int)smf->frame_type;
			if( shift < CF_StackMapFrame_SameFrame_Begin || shift > CF_StackMapFrame_SameFrame_End )
				break;
			smf->frame_type = shift;
			return shift;
		}
		else if( smf->frame_type >= CF_StackMapFrame_SameLocals_1_Stack_Item_Begin &&
				 smf->frame_type <= CF_StackMapFrame_SameLocals_1_Stack_Item_End )
		{
			shift += (int)smf->frame_type;
			if( shift < CF_StackMapFrame_SameLocals_1_Stack_Item_Begin || shift > CF_StackMapFrame_SameLocals_1_Stack_Item_End )
				break;
			smf->frame_type = shift;
			return shift;
		}
	}

	return -1;
}


// Function: build StackMapTables from byte stream
CF_Attr_StackMapFrame_Union** cf_build_stackmaptab( const CF_Attr_StackMapTable **tabs, int ntab, int *nfrm, int **frm_len_ptr )
{
	const juint8 SMFVT_LEN[] = CF_SMFVT_LEN_INIT;
	CF_Attr_StackMapFrame_Union **frm_ptr = NULL;
	CF_Attr_StackMapFrame_Union *cur_frm;
	CF_Attr_StackMapFrame_VT *smf_vt;
	int *frm_len;
	int i, e, l, p, t, k, num, len, frm_sum;

	num = 0;
	for( i=0; i < ntab; i++ )
	{
		if( tabs[i] )
			num += _CF_ENDIAN_FLIP_COUNT( tabs[i]->num_entry );
	}

	frm_ptr = malloc( (sizeof(void*) + sizeof(int)) * (num+ntab) );
	if( !frm_ptr )
		return NULL;
	frm_len = (int*)&frm_ptr[num+ntab];

	frm_sum = 0;
	for( i=0; i < ntab; i++ )
	{
		if( !tabs[i] )
		{
			frm_len[frm_sum] = 0;
			frm_ptr[frm_sum++] = NULL;  // terminated by null
			continue;
		}

		len = tabs[i]->attr_len;
		len = _CF_ENDIAN_FLIP_U4( len ) - sizeof(cf_count_t);

		num = tabs[i]->num_entry;
		_CF_ENDIAN_SWAP_COUNT( num );

		cur_frm = (CF_Attr_StackMapFrame_Union*)tabs[i]->entry;
		for( e=p=0; e < num && p < len; e++ )
		{
			l = 0;
			switch( cur_frm->frame_type )
			{
			case CF_StackMapFrame_SameLocals_1_Stack_Item_Ext:
				if( cur_frm->same_locals_1_stack_item_frame_ext.stack.tag > CF_SMFVT_uninit )
					break;
				l = sizeof( struct _cf_same_locals_1_stack_item_frame_ext ) - 1 + SMFVT_LEN[cur_frm->same_locals_1_stack_item_frame_ext.stack.tag];
				break;
			case CF_StackMapFrame_ChopFrame_Begin:
			case CF_StackMapFrame_ChopFrame_Begin+1:
			case CF_StackMapFrame_ChopFrame_Begin+2:
				l = sizeof( struct _cf_chop_frame );
				break;
			case CF_StackMapFrame_SameFrame_Ext:
				l = sizeof( struct _cf_same_frame_ext );
				break;
			case CF_StackMapFrame_AppendFrame_Begin:
			case CF_StackMapFrame_AppendFrame_Begin+1:
			case CF_StackMapFrame_AppendFrame_Begin+2:
				l = sizeof( struct _cf_append_frame );
				t = cur_frm->frame_type - CF_StackMapFrame_AppendFrame_Begin;
				for( k=0; k <= t; k++ )
				{
					smf_vt = (CF_Attr_StackMapFrame_VT*)((void*)cur_frm+l);
					if( smf_vt->tag > CF_SMFVT_uninit )
						break;
					l += SMFVT_LEN[smf_vt->tag];
				}
				break;
			case CF_StackMapFrame_FullFrame:
				l = _OFFSET( struct _cf_full_frame, locals );
				t = cur_frm->full_frame.number_of_locals;
				_CF_ENDIAN_SWAP_COUNT( t );
				for( k=0; k < t; k++ )
				{
					smf_vt = (CF_Attr_StackMapFrame_VT*)((void*)cur_frm+l);
					if( smf_vt->tag > CF_SMFVT_uninit )
						break;
					l += SMFVT_LEN[smf_vt->tag];
				}

				//
				t = *((juint16*)((void*)cur_frm + l));
				_CF_ENDIAN_SWAP_COUNT( t );
				l += sizeof( cf_count_t );
				for( k=0; k < t; k++ )
				{
					smf_vt = (CF_Attr_StackMapFrame_VT*)((void*)cur_frm+l);
					if( smf_vt->tag > CF_SMFVT_uninit )
						break;
					l += SMFVT_LEN[smf_vt->tag];
				}
				break;
			default:
				if( cur_frm->frame_type >= CF_StackMapFrame_SameFrame_Begin &&
					cur_frm->frame_type <= CF_StackMapFrame_SameFrame_End )
				{
					l = sizeof( struct _cf_same_frame );
					break;
				}

				if( cur_frm->frame_type >= CF_StackMapFrame_SameLocals_1_Stack_Item_Begin &&
				    cur_frm->frame_type <= CF_StackMapFrame_SameLocals_1_Stack_Item_End &&
					cur_frm->same_locals_1_stack_item_frame.stack.tag <= CF_SMFVT_uninit )
				{
					l = sizeof( struct _cf_same_locals_1_stack_item_frame ) - 1 + SMFVT_LEN[cur_frm->same_locals_1_stack_item_frame.stack.tag];
					break;
				}

				goto LAB_EXIT;
			}

			frm_len[frm_sum] = l;
			frm_ptr[frm_sum++] = cur_frm;
			p += l;
			cur_frm = (CF_Attr_StackMapFrame_Union*)((void*)cur_frm + l);
		}
		frm_len[frm_sum] = 0;
		frm_ptr[frm_sum++] = NULL;  // terminated by null
	}

	LAB_EXIT:
	if( i < ntab)
	{
		free( frm_ptr );
		return NULL;
	}

	*frm_len_ptr = frm_len;
	*nfrm = frm_sum;
	return frm_ptr;
}


//
int cf_change_smfvt2( CF_Attr_StackMapFrame_VT *smfvt, int num, const CF_InsLoc *insLocs, int nlocs, int code_shift, int cpool_shift )
{
	const juint8 SMFVT_LEN[] = CF_SMFVT_LEN_INIT;
	CF_Attr_SMFVT_Union *cur_vt = (CF_Attr_SMFVT_Union*)smfvt;
	int i, k, d, l;
	juint16 u2;

	for( i=l=0; i < num; i++ )
	{
		if( cur_vt->vt_tag == CF_SMFVT_object )
		{
			if( cpool_shift )
			{
				u2 = _CF_ENDIAN_FLIP_INDEX( cur_vt->ObjectVar.obj_idx ) + cpool_shift;
				cur_vt->ObjectVar.obj_idx = _CF_ENDIAN_FLIP_INDEX( u2 );
			}
		}
		else if( cur_vt->vt_tag == CF_SMFVT_uninit )
		{
			d = code_shift;
			u2 = _CF_ENDIAN_FLIP_INDEX( cur_vt->UniVar.offset );
			for( k=0; k < nlocs && insLocs[k].pos <= u2; k++ )
				d += insLocs[k].add;
			u2 += d;
			cur_vt->UniVar.offset = _CF_ENDIAN_FLIP_INDEX( u2 );
		}

		l += SMFVT_LEN[cur_vt->vt_tag];
		cur_vt = (CF_Attr_SMFVT_Union*)((void*)smfvt + l);
	}

	return l;
}


//
inline static void cf_change_smfvt( CF_Attr_StackMapFrame_Union *smf, const CF_InsLoc *insLocs, int nlocs, int code_shift, int cpool_shift )
{
	switch( smf->frame_type )
	{
	case CF_StackMapFrame_SameLocals_1_Stack_Item_Ext:
		cf_change_smfvt2( &smf->same_locals_1_stack_item_frame_ext.stack, 1, insLocs, nlocs, code_shift, cpool_shift );
		break;
	case CF_StackMapFrame_ChopFrame_Begin:
	case CF_StackMapFrame_ChopFrame_Begin+1:
	case CF_StackMapFrame_ChopFrame_Begin+2:
		break;
	case CF_StackMapFrame_SameFrame_Ext:
		break;
	case CF_StackMapFrame_AppendFrame_Begin:
	case CF_StackMapFrame_AppendFrame_Begin+1:
	case CF_StackMapFrame_AppendFrame_Begin+2:
		cf_change_smfvt2( smf->append_frame.locals, smf->frame_type-CF_StackMapFrame_AppendFrame_Begin+1,
						  insLocs, nlocs, code_shift, cpool_shift );
		break;
	case CF_StackMapFrame_FullFrame:{
		int ln = cf_change_smfvt2( smf->full_frame.locals, _CF_ENDIAN_FLIP_U2(smf->full_frame.number_of_locals),
						  	  	   insLocs, nlocs, code_shift, cpool_shift );
		juint16 *num_ptr = (juint16*)((void*)smf->full_frame.locals + ln);
		ln = *num_ptr;
		cf_change_smfvt2( (CF_Attr_StackMapFrame_VT*)&num_ptr[1], _CF_ENDIAN_FLIP_U2(ln),
						  insLocs, nlocs, code_shift, cpool_shift );
		break;
	}
	default:
		/*if( smf->frame_type >= CF_StackMapFrame_SameFrame_Begin &&
			smf->frame_type <= CF_StackMapFrame_SameFrame_End )
		{
			break;
		}*/

		if( smf->frame_type >= CF_StackMapFrame_SameLocals_1_Stack_Item_Begin &&
			smf->frame_type <= CF_StackMapFrame_SameLocals_1_Stack_Item_End )
		{
			cf_change_smfvt2( &smf->same_locals_1_stack_item_frame.stack, 1, insLocs, nlocs, code_shift, cpool_shift );
			break;
		}
	}
}

// Function: alter stack map table
int cf_alter_stackmaptab( CF_Attr_StackMapFrame_Union **smf_ptr, int nfrm, int *smf_len,
						  const CF_InsLoc *insLocs, int nlocs,
						  int code_shift, int cpool_shift, void **ext_buf, int *end_pos
						  )
{
	const juint8 SMFVT_LEN[] = CF_SMFVT_LEN_INIT;
	int i, k, x, l, pos, _pos_, ttlen;  // num
	juint16 delta;
	CF_Attr_StackMapFrame_Union *cur_frm;
	void *buf_ptr = NULL;
	int buf_pos = 0, buf_size=0;

	pos = end_pos ? *end_pos : -1; //code_shift - !code_shift;
	//num = 0;
	ttlen = 0;

	for( i=k=0; i < nfrm && (k < nlocs || !insLocs); i++ )
	{
		if( !(cur_frm = smf_ptr[i]) )
			continue;

		switch( cur_frm->frame_type )
		{
		case CF_StackMapFrame_SameLocals_1_Stack_Item_Ext:
		case CF_StackMapFrame_ChopFrame_Begin:
		case CF_StackMapFrame_ChopFrame_Begin+1:
		case CF_StackMapFrame_ChopFrame_Begin+2:
		case CF_StackMapFrame_SameFrame_Ext:
		case CF_StackMapFrame_AppendFrame_Begin:
		case CF_StackMapFrame_AppendFrame_Begin+1:
		case CF_StackMapFrame_AppendFrame_Begin+2:
		case CF_StackMapFrame_FullFrame:
			delta = _CF_ENDIAN_SWAP_U2(cur_frm->append_frame.delta) + 1;
			_pos_ = pos + delta;
			for( x=0; k < nlocs && insLocs[k].pos < _pos_; k++ )
			{
				if( insLocs[k].pos >= pos )
					x += insLocs[k].add;
			}
			pos = _pos_;
			delta += x - 1 + code_shift;
			cur_frm->append_frame.delta = (juint16)_CF_ENDIAN_FLIP_U2( delta );

			//
			cf_change_smfvt( cur_frm, insLocs, nlocs, code_shift, cpool_shift );
			break;
		default:
			if( cur_frm->frame_type >= CF_StackMapFrame_SameFrame_Begin &&
				cur_frm->frame_type <= CF_StackMapFrame_SameFrame_End )
			{
				delta = cur_frm->same_frame.type + 1;
				_pos_ = pos + delta;
				for( x=0; k < nlocs && insLocs[k].pos < _pos_; k++ )
				{
					if( insLocs[k].pos >= pos )
						x += insLocs[k].add;
				}
				pos = _pos_;

				delta += x - 1 + code_shift;
				if( delta <= CF_StackMapFrame_SameFrame_End )
					cur_frm->same_frame.type = (juint8)delta;
				else
				{
					l = sizeof( struct _cf_same_frame_ext );
					if( buf_pos+l > buf_size )
					{
						void *ptr = realloc( buf_ptr, buf_size+=256 );
						if( !ptr )
							goto LAB_ERROR;
						buf_ptr = ptr;
					}
					cur_frm = (CF_Attr_StackMapFrame_Union*)(buf_ptr + buf_pos);
					buf_pos += l;

					cur_frm->same_frame_ext.type = CF_StackMapFrame_SameFrame_Ext;
					cur_frm->same_frame_ext.delta = (juint16)_CF_ENDIAN_FLIP_U2( delta );
					smf_ptr[i] = cur_frm;
					smf_len[i] = l;
				}
				break;
			}


			if( cur_frm->frame_type >= CF_StackMapFrame_SameLocals_1_Stack_Item_Begin &&
			    cur_frm->frame_type <= CF_StackMapFrame_SameLocals_1_Stack_Item_End )
			{
				delta = cur_frm->same_locals_1_stack_item_frame.type - CF_StackMapFrame_SameLocals_1_Stack_Item_Begin + 1;
				_pos_ = pos + delta;
				for( x=0; k < nlocs && insLocs[k].pos < _pos_; k++ )
				{
					if( insLocs[k].pos >= pos )
						x += insLocs[k].add;
				}
				pos = _pos_;

				delta += x - 1 + code_shift + CF_StackMapFrame_SameLocals_1_Stack_Item_Begin;
				if( delta <= CF_StackMapFrame_SameLocals_1_Stack_Item_End )
					cur_frm->same_locals_1_stack_item_frame.type = (juint8)delta;
				else
				{
					l = sizeof( struct _cf_same_locals_1_stack_item_frame_ext ) + SMFVT_LEN[cur_frm->same_locals_1_stack_item_frame.stack.tag] - 1;
					if( buf_pos+l > buf_size )
					{
						void *ptr = realloc( buf_ptr, buf_size+=256 );
						if( !ptr )
							goto LAB_ERROR;
						buf_ptr = ptr;
					}
					cur_frm = (CF_Attr_StackMapFrame_Union*)(buf_ptr + buf_pos);
					buf_pos += l;

					delta -= CF_StackMapFrame_SameLocals_1_Stack_Item_Begin;
					cur_frm->same_locals_1_stack_item_frame_ext.type = CF_StackMapFrame_SameLocals_1_Stack_Item_Ext;
					cur_frm->same_locals_1_stack_item_frame_ext.delta = (juint16)_CF_ENDIAN_FLIP_U2( delta );
					cur_frm->same_locals_1_stack_item_frame_ext.stack.tag = smf_ptr[i]->same_locals_1_stack_item_frame.stack.tag;
					if( l > (int)sizeof( struct _cf_same_locals_1_stack_item_frame_ext ) )
						*(juint16*)cur_frm->same_locals_1_stack_item_frame_ext.stack.b = *(juint16*)smf_ptr[i]->same_locals_1_stack_item_frame.stack.b;
					smf_ptr[i] = cur_frm;
					smf_len[i] = l;
				}

				//
				cf_change_smfvt( cur_frm, insLocs, nlocs, code_shift, cpool_shift );
				break;
			}
		}

		//num++;
		ttlen += smf_len[i];
		code_shift = 0;
	}

	*ext_buf = buf_ptr;

	if( end_pos )
		*end_pos = pos;

	return ttlen;  //num;

	LAB_ERROR:
	if( buf_ptr )
		free( buf_ptr );
	return -1;
}


// Function:
int cf_fill_smf_full_frame( juint16 delta, const CF_SignExt *locals, int local_num,
		const CF_SignExt *stacks, int stack_num, void *dst_buf, const CF_SignExt *this_var )
{
	int i;
	CF_Attr_SMFVT_Union *vt;
	struct _cf_full_frame *frame = (struct _cf_full_frame*)dst_buf;
	frame->type = CF_StackMapFrame_FullFrame;
	frame->delta = _CF_ENDIAN_FLIP_U2( delta );

	i = local_num + !!this_var;
	frame->number_of_locals = _CF_ENDIAN_FLIP_U2( i );
	vt = (CF_Attr_SMFVT_Union*)frame->locals;

	if( this_var )
	{
		if( this_var == (void*)-1 )
		{
			// uninitiated this
			vt->vt_tag = CF_SMFVT_uninit_this;
			vt = (CF_Attr_SMFVT_Union*)((void*)vt + CF_SMFVT_LEN[CF_SMFVT_uninit_this]);
		}
		else
		{
			// insert "this" at head(2023-02-25)
			vt->vt_tag = CF_SMFVT_object;
			vt->ObjectVar.obj_idx = _CF_ENDIAN_FLIP_U2(this_var->cls_idx);
			vt = (CF_Attr_SMFVT_Union*)((void*)vt + CF_SMFVT_LEN[CF_SMFVT_object]);
		}
	}

	for( i=0; i < local_num; i++ )
	{
		vt->vt_tag = CF_IDX_TO_SMFVT[locals[i].type_idx];
		if( vt->vt_tag == CF_SMFVT_object )
			vt->ObjectVar.obj_idx = _CF_ENDIAN_FLIP_U2(locals[i].cls_idx);
		vt = (CF_Attr_SMFVT_Union*)((void*)vt + CF_SMFVT_LEN[vt->vt_tag]);
	}

	//
	vt = (CF_Attr_SMFVT_Union*)((void*)vt + sizeof(juint16));
	*((juint16*)vt-1) = _CF_ENDIAN_FLIP_U2( stack_num );
	for( i=0; i < stack_num; i++ )
	{
		vt->vt_tag = CF_IDX_TO_SMFVT[stacks[i].type_idx];
		if( vt->vt_tag == CF_SMFVT_object )
			vt->ObjectVar.obj_idx = _CF_ENDIAN_FLIP_U2(stacks[i].cls_idx);
		vt = (CF_Attr_SMFVT_Union*)((void*)vt + CF_SMFVT_LEN[vt->vt_tag]);
	}
	return (void*)vt - dst_buf;
}


//
typedef int (*FUNPTR_INSTRUCTION_EXPLAIN)( const juint8 *code, int pos, const CF_ClassFileMap *cfmap, FILE *fout );

//
CF_UNUSED static  FUNPTR_INSTRUCTION_EXPLAIN cf_func_instruction_explain[256] = {
		NULL
};


// Function: translate byte-code in readable instruction assemble sequences
void cf_bytecode_translate( const juint8 *code, int len, const CF_ClassFileMap *cfmap, FILE *fout )
{
	register int n, p, c, a0;
	CP_Info **cps = cfmap->cptab[0].cps;

	p = 0;
	while( p < len )
	{
		if( (n = cf_opc_len_calcu( &code[p], p )) <= 0 )
			return ;

		c = code[p];
		fprintf( fout, "%5d: %-16.16s", p, CF_OPC_SYM[c] );

		switch( c )
		{
		/* Constants */
	    case CF_OPC_bipush     :
	    	a0 = code[p+1];
	    	fprintf( fout, " %02x%15c// %d", a0, 32, (-(a0>>7)&0xffffff00|a0) );
	    	break;
	    case CF_OPC_sipush     :
	    	a0 = _MAKE_INT16_( code[p+1], code[p+2] );
	    	fprintf( fout, " %04x%13c// %d", a0, 32, (-(a0>>15)&0xffff0000|a0) );
	    	break;
	    case CF_OPC_ldc        :
	    	a0 = code[p+1];
	    	fprintf( fout, " #%-16d// ", a0 );
	    	cf_print_cpinfo( cps[a0], TRUE, fout );
	    	break;
	    case CF_OPC_ldc_w      :
	    case CF_OPC_ldc2_w     :
	    	a0 = _MAKE_INT16_( code[p+1], code[p+2] );
	    	fprintf( fout, " #%-16d// ", a0 );
	    	cf_print_cpinfo( cps[a0], TRUE, fout );
	    	break;
		/* Loads */
	    case CF_OPC_iload      :
	    case CF_OPC_lload      :
	    case CF_OPC_fload      :
	    case CF_OPC_dload      :
	    case CF_OPC_aload      :
		/* Stores */
	    case CF_OPC_istore     :
	    case CF_OPC_lstore     :
	    case CF_OPC_fstore     :
	    case CF_OPC_dstore     :
	    case CF_OPC_astore     :
			fprintf( fout, " [%d]", code[p+1] );
			break;
		/* Math */
	    case CF_OPC_iinc       :
	    	a0 = code[p+2];
			fprintf( fout, " [%d], %d", code[p+1], (-(a0>>7)&0xffffff00|a0) );
			break;
		/* Comparisons */
	    case CF_OPC_ifeq       :
	    case CF_OPC_ifne       :
	    case CF_OPC_iflt       :
	    case CF_OPC_ifge       :
	    case CF_OPC_ifgt       :
	    case CF_OPC_ifle       :
	    case CF_OPC_if_icmpeq  :
	    case CF_OPC_if_icmpne  :
	    case CF_OPC_if_icmplt  :
	    case CF_OPC_if_icmpge  :
	    case CF_OPC_if_icmpgt  :
	    case CF_OPC_if_icmple  :
	    case CF_OPC_if_acmpeq  :
	    case CF_OPC_if_acmpne  :
	    case CF_OPC_ifnull     :  /* Extended */
	    case CF_OPC_ifnonnull  :  /* Extended */
	    	a0 = _MAKE_INT16_( code[p+1], code[p+2] );
	    	a0 = -(a0>>15)&0xffff0000|a0;
	    	fprintf( fout, " %-16d // %d", a0, p+a0 );
	    	break;
		/* Controls */
	    case CF_OPC_goto           :
	    case CF_OPC_jsr            :
	    	a0 = _MAKE_INT16_( code[p+1], code[p+2] );
	    	a0 = -(a0>>15)&0xffff0000|a0;
	    	fprintf( fout, " %-16d // %d", a0, p+a0 );
	    	break;
	    case CF_OPC_ret            :
			fprintf( fout, " [%d]", code[p+1] );
			break;
	    case CF_OPC_tableswitch    :{
	    	a0 = (p + 4) & -4;  // (p + 1 + 3) & -4;
	    	int d = _MAKE_INT32_( code[a0], code[a0+1], code[a0+2], code[a0+3] );
	    	int l = _MAKE_INT32_( code[a0+4], code[a0+5], code[a0+6], code[a0+7] );
	    	int h = _MAKE_INT32_( code[a0+8], code[a0+9], code[a0+10], code[a0+11] );
	    	fprintf( fout, " {  // low=%d, high=%d\n", l, h );
	    	for( a0+=12; l <= h; l++, a0+=4 )
	    		fprintf( fout, "%20d: %d\n", l, p+_MAKE_INT32_( code[a0], code[a0+1], code[a0+2], code[a0+3] ) );
	    	fprintf( fout, "%20s: %d\n%12c}", "default", p+d, 32 );
	    	break;
	    }
	    case CF_OPC_lookupswitch   :{
	    	a0 = (p + 4) & -4;  // (p + 1 + 3) & -4;
	    	int d = _MAKE_INT32_( code[a0], code[a0+1], code[a0+2], code[a0+3] );
	    	int m = _MAKE_INT32_( code[a0+4], code[a0+5], code[a0+6], code[a0+7] );

	    	fprintf( fout, " {  // npairs=%d\n", m );
	    	for( a0+=8; --m >= 0; a0+=8 )
	    		fprintf( fout, "%20d: %d\n",
	    				_MAKE_INT32_( code[a0], code[a0+1], code[a0+2], code[a0+3] ),
						p+_MAKE_INT32_( code[a0+4], code[a0+5], code[a0+6], code[a0+7] ) );
	    	fprintf( fout, "%20s: %d\n%12c}", "default", p+d, 32 );
	    	break;
	    }
		/* References */
	    case CF_OPC_getstatic      :
	    case CF_OPC_putstatic      :
	    case CF_OPC_getfield       :
	    case CF_OPC_putfield       :
	    case CF_OPC_invokevirtual  :
	    case CF_OPC_invokespecial  :
	    case CF_OPC_invokestatic   :
	    	a0 = _MAKE_INT16_( code[p+1], code[p+2] );
	    	fprintf( fout, " #%-16d// ", a0 );
	    	cf_print_cpinfo( cps[a0], TRUE, fout );
	    	break;
	    case CF_OPC_invokeinterface:
	    	a0 = _MAKE_INT16_( code[p+1], code[p+2] );
	    	fprintf( fout, " #%-16d, %d// ", a0, code[p+3] );
	    	cf_print_cpinfo( cps[a0], TRUE, fout );
	    	break;
	    case CF_OPC_invokedynamic  :
	    	a0 = _MAKE_INT16_( code[p+1], code[p+2] );
	    	fprintf( fout, " #%-16d// ", a0 );
	    	cf_print_cpinfo( cps[a0], TRUE, fout );
	    	break;
	    case CF_OPC_new            :
	    case CF_OPC_anewarray      :
	    case CF_OPC_checkcast      :
	    case CF_OPC_instanceof     :
	    	a0 = _MAKE_INT16_( code[p+1], code[p+2] );
	    	fprintf( fout, " #%-16d// ", a0 );
	    	cf_print_cpinfo( cps[a0], TRUE, fout );
	    	break;
	    case CF_OPC_newarray       :
	    	fprintf( fout, " %s", CF_PRIME_TYPE_NAME[code[p+1]] );
	    	break;
		/* Extended */
	    case CF_OPC_wide           :
	    	if( code[p+1] == CF_OPC_iinc )
	    	{
	    		a0 = _MAKE_INT16_( code[p+4], code[p+5] );
				fprintf( fout, "%s [%d], %d", CF_OPC_SYM[CF_OPC_iinc], _MAKE_INT16_( code[p+2], code[p+3] ), (-(a0>>15)&0xffff0000|a0) );
	    	}
	    	else if( code[p+1] == CF_OPC_ret )
	    		fprintf( fout, "%s %d", CF_OPC_SYM[CF_OPC_ret], _MAKE_INT16_( code[p+2], code[p+3] ) );
	    	else
				fprintf( fout, "%s #%d", CF_OPC_SYM[code[p+1]], _MAKE_INT16_( code[p+2], code[p+3] ) );
	    	break;
	    case CF_OPC_multianewarray :
	    	a0 = _MAKE_INT16_( code[p+1], code[p+2] );
	    	fprintf( fout, " #%-16d, %d// ", a0, code[p+3] );
	    	cf_print_cpinfo( cps[a0], TRUE, fout );
	    	break;
	    case CF_OPC_goto_w         :  /* also is Control */
	    case CF_OPC_jsr_w          :  /* also is Control */
	    	a0 = _MAKE_INT32_( code[p+1], code[p+2], code[p+3], code[p+4] );
	    	fprintf( fout, " %-16d // %d", a0, p+a0 );
	    	break;
	    default:
	    	break;
		}
		fprintf( fout, "\n" );
		p += n;
	}
}
