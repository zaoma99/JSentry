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
#include "cf_impl.h"
#include "cf_bytecode.h"




// Function: get field or method by specified name_index and/or descriptor_index
CF_HIDDEN const CF_FieldInfo* cf_get_field_method( int which, const CF_ClassFileMap *cf_map,
												   cf_idx_t name_idx, cf_idx_t desc_idx,
												   cf_idx_t *field_idx, ssize_t *field_size )
{
	if( name_idx | desc_idx )
	{
		CF_FieldInfo **fields;
		cf_count_t c, n, m, i;

		cf_count_t tablen;
		const struct _cf_fdtab_item *tab;

		if( which == 1 )
		{
			// choose field
			tablen = cf_map->fdtab_count;
			tab = cf_map->fdtab;
		}
		else
		{
			// choose method
			tablen = cf_map->mdtab_count;
			tab = (struct _cf_fdtab_item*)cf_map->mdtab;
		}

		i = 0;
		for( c=0; c < tablen; c++ )
		{
			fields = tab[c].fields;
			m = tab[c].field_count;
			for( n=0; n < m; n++ )
			{
				if( (!name_idx || fields[n]->name_idx == name_idx) && (!desc_idx || fields[n]->desc_idx == desc_idx) )
				{
					if( field_idx )
						*field_idx = i + n;

					if( field_size )
						*field_size = tab[c].fields_len[n];

					return fields[n];
				}
			}
			i += m;
		}
	}
	return NULL;
}


//
CF_HIDDEN const CF_FieldInfo* cf_get_field_method_by_name( int which, const CF_ClassFileMap *cf_map,
														   const char *name, const char *desc,
														   cf_idx_t *field_idx, ssize_t *field_size,
														   const CP_Utf8 **desc_full )
{
	CP_InfoUnion *cpite[2];
	CF_FieldInfo **fields;
	cf_count_t c, n, m, i;
	cf_count_t tablen;
	const struct _cf_fdtab_item *tab;
	int name_len, desc_len;

	if( !name && !desc )
		return NULL;

	if( which == 1 )
	{
		// choose field
		tablen = cf_map->fdtab_count;
		tab = cf_map->fdtab;
	}
	else
	{
		// choose method
		tablen = cf_map->mdtab_count;
		tab = (struct _cf_fdtab_item*)cf_map->mdtab;
	}

	name_len = name ? strlen( name ) : 0;
	desc_len = desc ? strlen( desc ) : 0;

	i = 0;
	for( c=0; c < tablen; c++ )
	{
		fields = tab[c].fields;
		m = tab[c].field_count;

		for( n=0; n < m; n++ )
		{
			if( name )
			{
				// match name
				cpite[0] = (CP_InfoUnion*)cf_get_cpitem( cf_map, fields[n]->name_idx );
				if( !cpite[0] ||
					cpite[0]->Info.tag != JVM_CONSTANT_Utf8 ||
					name_len != cpite[0]->Utf8.len ||
					strncmp(cpite[0]->Utf8.b, name, cpite[0]->Utf8.len) != 0
					)
					continue;
			}

			if( desc )
			{
				// match full signature besides name and return type if possible
				cpite[1] = (CP_InfoUnion*)cf_get_cpitem( cf_map, fields[n]->desc_idx );
				if( !cpite[1] ||
					cpite[1]->Info.tag != JVM_CONSTANT_Utf8 ||
					desc_len > cpite[1]->Utf8.len ||
					strncmp(cpite[1]->Utf8.b, desc, desc_len) != 0
					)
					continue;

				if( desc[desc_len-1] != JVM_SIGNATURE_ENDFUNC && desc_len != cpite[1]->Utf8.len )
					continue;

				if( desc_full && desc[desc_len-1] == JVM_SIGNATURE_ENDFUNC )
					*desc_full = (CP_Utf8*)cpite[1];
			}

			if( field_idx )
				*field_idx = i + n;

			if( field_size )
				*field_size = tab[c].fields_len[n];

			return fields[n];
		}
		i += m;
	}
	return NULL;
}


// Function: find field by specified name and/or descriptor
CF_HIDDEN const CF_FieldInfo* cf_get_field2( const CF_ClassFileMap *cf_map, const char *name, const char *desc,
											 cf_idx_t *field_idx, ssize_t *field_len )
{
	cf_idx_t name_idx=0, desc_idx=0;

	if( name && !cf_get_cpitem_utf8( cf_map, name, strlen(name), &name_idx) )
		return NULL;
	if( desc && !cf_get_cpitem_utf8( cf_map, desc, strlen(desc), &desc_idx) )
		return NULL;

	return cf_get_field_method( 1, cf_map, name_idx, desc_idx, field_idx, field_len );
}


// Function: get field according to the index
CF_HIDDEN const CF_FieldInfo* cf_get_field3( const CF_ClassFileMap *cf_map, cf_idx_t idx, ssize_t *field_len )
{
	cf_count_t c;
	for( c=0; c < cf_map->fdtab_count; c++ )
	{
		if( cf_map->fdtab[c].field_count > idx )
		{
			if( field_len )
				*field_len = cf_map->fdtab[c].fields_len[idx];

			return cf_map->fdtab[c].fields[idx];
		}
		else
		{
			idx -= cf_map->fdtab[c].field_count;
		}
	}
	return NULL;
}



// Function: find method by specified name and/or descriptor
CF_HIDDEN const CF_MethodInfo* cf_get_method2( const CF_ClassFileMap *cf_map, const char *name, const char *desc,
											   cf_idx_t *method_idx, ssize_t *method_len, const CP_Utf8 **desc_full )
{
	cf_idx_t name_idx=0, desc_idx=0;

	if( name && !cf_get_cpitem_utf8( cf_map, name, strlen(name), &name_idx) )
		return NULL;
	if( desc )
	{
		int n = strlen( desc );
		if( desc[n-1] == JVM_SIGNATURE_ENDFUNC )
		{
			const CP_Utf8 *sign = cf_get_cpitem_utf8_signature( cf_map, desc, strlen(desc), &desc_idx );
			if( !sign )
				return NULL;
			if( desc_full )
				*desc_full = sign;
		}
		else if( !cf_get_cpitem_utf8( cf_map, desc, n, &desc_idx) )
			return NULL;
	}

	return cf_get_field_method( 2, cf_map, name_idx, desc_idx, method_idx, method_len );
}


// Function: get method according to the index
CF_HIDDEN const CF_MethodInfo* cf_get_method3( const CF_ClassFileMap *cf_map, cf_idx_t idx, ssize_t *method_len )
{
	cf_count_t c;
	for( c=0; c < cf_map->mdtab_count; c++ )
	{
		if( cf_map->mdtab[c].method_count > idx )
		{
			if( method_len )
				*method_len = cf_map->mdtab[c].methods_len[idx];

			return cf_map->mdtab[c].methods[idx];
		}
		else
		{
			idx -= cf_map->mdtab[c].method_count;
		}
	}
	return NULL;
}


// Function:
int cf_parse_method_descriptor( const char *desc, int desc_len, CF_SignExt **ite_ptr, const CF_ClassFileMap *cfmap )
{
	//const char primitive_sign[] = "VZCFDBSIJL[E";
	const char *primitive_sign = CF_SIGNATURE_SYMBOL + 3;  // start from void
	const char *c = desc;
	const char *e = desc + desc_len;
	CF_SignExt *ite = NULL;
	int ite_num = -1;
	CP_InfoUnion cpi_hit = {.Class.tag = JVM_CONSTANT_Class};
	juint8 array_dims = 0;
	juint8 epoch = 2;  // parameter list and return value
	juint8 type_idx;
	const char *p, *h;

	if( desc_len >= 3 && *c == JVM_SIGNATURE_FUNC && (e=strchr(c+1, JVM_SIGNATURE_ENDFUNC)) )
	{
		ite_num = 0;

		do{
			while( ++c < e )
			{
				if( *c == JVM_SIGNATURE_ARRAY )
				{
					// array
					if( ++array_dims == 0 )
						break;  // overflow
				}
				else
				{
					if( *c == JVM_SIGNATURE_CLASS || *c == JVM_SIGNATURE_ENUM )
					{
						// reference
						p = strchr( c+1, JVM_SIGNATURE_ENDCLASS );
						type_idx = sizeof(CF_SIGNATURE_SYMBOL) - 3 - !array_dims;  // sizeof(CF_SIGNATURE_SYMBOL) - 2 => sizeof(CF_SIGNATURE_SYMBOL) - 3
					}
					else if( (p = strchr(primitive_sign, *c)) )
					{
						// primitive
						type_idx = array_dims ? sizeof(CF_SIGNATURE_SYMBOL) - 3 : p - CF_SIGNATURE_SYMBOL;  // 14 => sizeof(CF_SIGNATURE_SYMBOL) - 3
						p = c;
					}

					if( !p )
						break;

					if( (ite_num&7) == 0 )
					{
						// allocate
						void *ptr = realloc( ite, sizeof(CF_SignExt)*(ite_num+8) );
						if( !ptr )
							break;
						ite = (CF_SignExt*)ptr;
					}

					ite[ite_num].sym = (char*)(h = c - array_dims);
					ite[ite_num].len = p - h + 1;
					ite[ite_num].cls_idx = 0;
					ite[ite_num].type_idx = type_idx;
					ite[ite_num].res[0] = 0;
					if( p > h )
					{
						if( cfmap )
						{
							// seek the class be specified by this symbol
							if( array_dims > 0 )
								cf_get_cpitem_xx( cfmap, &cpi_hit, &ite[ite_num].cls_idx, h, ite[ite_num].len );
							else
								cf_get_cpitem_xx( cfmap, &cpi_hit, &ite[ite_num].cls_idx, h+1, ite[ite_num].len-2 );

							if( ite[ite_num].cls_idx == 0 )
							{
								ite[ite_num].res[0] = 1 + (epoch == 1);  // set flag 1 if it is parameters, or set 2 if it is return value
								cf_get_cpitem_xx( cfmap, &cpi_hit, &ite[ite_num].cls_idx, "java/lang/Object", sizeof("java/lang/Object")-1 );
							}

#if 0
							cpi_hit.Class.name_idx = 0;
							if( array_dims > 0 )
								cf_get_cpitem_utf8( cfmap, h, ite[ite_num].len, &cpi_hit.Class.name_idx );
							else
								cf_get_cpitem_utf8( cfmap, h+1, ite[ite_num].len-2, &cpi_hit.Class.name_idx );

							if( cpi_hit.Class.name_idx == 0 )
							{
								// replace with the base class(2023-02-25)
								cf_get_cpitem_utf8( cfmap, "java/lang/Object", sizeof("java/lang/Object")-1, &cpi_hit.Class.name_idx );
							}

							if( cpi_hit.Class.name_idx != 0 )
								cf_get_cpitem_ex( cfmap, &cpi_hit, &ite[ite_num].cls_idx );
#endif
						}
						c = p;
					}
					ite_num++;
					array_dims = 0;
				}
			}

			if( c != e || array_dims != 0 )
				break;
			e = desc + desc_len;  // try to extract "return type"
		}while( --epoch > 0 );

		if( epoch == 0 )
		{
			*ite_ptr = ite;
		}
		else
		{
			ite_num = -1;
			*ite_ptr = NULL;
			if( ite )
				free( ite );
		}
	}

	return ite_num;
}


// Function: check validation of stub method with standard .class format
static int cf_verify_stub_method( const CF_Attr_Code *attr_code, ssize_t code_len,
								  const juint8 *blkptr[], int *blklen, cf_idx_t *catch_type )
{
	// exception table of the target "Code"
	CF_ExpInfo *exptab_ptr = (CF_ExpInfo*)(attr_code->code.byte + code_len + sizeof(cf_count_t));
	cf_count_t exptab_len = *((cf_count_t*)exptab_ptr - 1);
	_CF_ENDIAN_SWAP_COUNT( exptab_len );
	if( exptab_len != 3 )
		return -1;

	int p;

	blkptr[0] = attr_code->code.byte + (p = _CF_ENDIAN_FLIP_U2( exptab_ptr[0].start_pc ));
	blklen[0] = (int)_CF_ENDIAN_FLIP_U2(exptab_ptr[0].end_pc) - p;
	p = blkptr[0][blklen[0]];
	blklen[0] += ((p >= CF_OPC_ireturn) & (p <= CF_OPC_return)) | (p == CF_OPC_athrow);  // include the instruction "return" if possible

	blkptr[1] = attr_code->code.byte + (p = _CF_ENDIAN_FLIP_U2( exptab_ptr[1].start_pc ));
	blklen[1] = (int)_CF_ENDIAN_FLIP_U2(exptab_ptr[1].end_pc) - p;
	p = blkptr[1][blklen[1]];
	blklen[1] += ((p >= CF_OPC_ireturn) & (p <= CF_OPC_return)) | (p == CF_OPC_athrow);  // include the instruction "return" if possible

	// CAUTION: the CATCH code always is behind the clause "catch"
	p = _CF_ENDIAN_FLIP_U2( exptab_ptr[2].handler_pc );  // CAUTION: here is the handler_pc, not the start_pc
	blkptr[2] = &attr_code->code.byte[p];
	blklen[2] = code_len - p; // till to the end

	*catch_type = _CF_ENDIAN_FLIP_INDEX( exptab_ptr[2].catch_type );

	return 0;
}


//
static void _cf_change_1_stack( int delta, CF_Attr_StackMapFrame_Union *smf_cur, int *smf_cur_len )
{
	if( smf_cur->frame_type == CF_StackMapFrame_FullFrame )
	{
		// cut the last "stack item"(assume it is a Throwable)
		int j;
		CF_Attr_StackMapFrame_VT *smf_vt;
		int l = _OFFSET( struct _cf_full_frame, locals );
		int t = _CF_ENDIAN_FLIP_COUNT( smf_cur->full_frame.number_of_locals );
		for( j=0; j < t; j++ )
		{
			smf_vt = (CF_Attr_StackMapFrame_VT*)((void*)smf_cur+l);
			l += CF_SMFVT_LEN[smf_vt->tag];
		}

		juint16 *stk_num = (juint16*)((void*)smf_cur + l);
		t = _CF_ENDIAN_FLIP_U2( *stk_num ) - 1;
		if( t >= 0 )
		{
			*stk_num = _CF_ENDIAN_FLIP_U2( t );
			smf_cur->full_frame.delta = _CF_ENDIAN_FLIP_U2( delta );
			*smf_cur_len -= 3;  // only reduce the length
		}
	}
	else if( delta <= CF_StackMapFrame_SameFrame_End )
	{
		// changed to "same_frame"
		smf_cur->same_frame.type = delta;
		*smf_cur_len = sizeof( struct _cf_same_frame );
	}
	else
	{
		// changed to "same_frame_extended"
		smf_cur->same_frame_ext.type = CF_StackMapFrame_SameFrame_Ext;
		smf_cur->same_frame_ext.delta = _CF_ENDIAN_FLIP_U2( delta );
		*smf_cur_len = sizeof( struct _cf_same_frame_ext );
	}
}


//
static void _cf_change_fullframe_vt_object( CF_Attr_StackMapFrame_Union *smf_cur, const CF_SignExt *locals, int local_num, const CF_SignExt *this_var )
{
	if( smf_cur->frame_type == CF_StackMapFrame_FullFrame )
	{
		int j;
		CF_Attr_StackMapFrame_VT *smf_vt;
		int l = _OFFSET( struct _cf_full_frame, locals );
		int t = _CF_ENDIAN_FLIP_COUNT( smf_cur->full_frame.number_of_locals );

		if( t > 0 )
		{
			if( this_var )
			{
				smf_vt = (CF_Attr_StackMapFrame_VT*)((void*)smf_cur+l);
				l += CF_SMFVT_LEN[smf_vt->tag];
				if( smf_vt->tag == CF_SMFVT_object )
				{
					((CF_Attr_SMFVT_Object*)smf_vt)->obj_idx = _CF_ENDIAN_FLIP_U2( this_var->cls_idx );
				}
				t--;
			}
			if( t > local_num )
				t = local_num;

			for( j=0; j < t; j++ )
			{
				smf_vt = (CF_Attr_StackMapFrame_VT*)((void*)smf_cur+l);
				l += CF_SMFVT_LEN[smf_vt->tag];
				//
				if( smf_vt->tag == CF_SMFVT_object )
				{
					((CF_Attr_SMFVT_Object*)smf_vt)->obj_idx = _CF_ENDIAN_FLIP_U2( locals[j].cls_idx );
				}
			}
		}
	}
}


// Function: simply algorithm for generating of StackMapTable
static  int cf_blend_stkmaptab( CF_Attr_StackMapFrame_Union **smf_src,
								int *smf_src_lens,
								int smf_src_num,
								CF_Attr_StackMapFrame_Union **smf_dst,
								int *smf_dst_lens,
								void **ext_buf_ret,
								const CF_ExpInfo *stub_exptab,
								const juint8 *bytecode,
								CF_CodeTransformInfo *info )
{
	CF_Attr_StackMapFrame_Union *smf_cur;
	CF_ExpInfo expinfo[3];
	int m, smf_num[2], ext_buf_len;
	int i=-1, pos = -1, j;
	int _i_[3], _delta_[3], _d_;//, _ofst_[3];
	void *ext_buf;


	// seek the last frame of each try-catch-block
	for( m=j=0; j < 3; j++ )
	{
		expinfo[j].start_pc = _CF_ENDIAN_FLIP_U2( stub_exptab[j].start_pc );
		expinfo[j].end_pc = _CF_ENDIAN_FLIP_U2( stub_exptab[j].end_pc );
		expinfo[j].handler_pc = _CF_ENDIAN_FLIP_U2( stub_exptab[j].handler_pc );

		//
		for( ; m < info->ins_locs_num2 && info->ins_locs2[m].ext == j+1; m++ )
		{
			info->ins_locs2[m].pos += expinfo[j].start_pc;
		}

		// check stub's delta and save the last frame's delta in each block
		for( ++i; (smf_cur=smf_src[i]); ++i )
		{
			switch( smf_cur->frame_type )
			{
			case CF_StackMapFrame_SameLocals_1_Stack_Item_Ext:
				pos += (_delta_[j]=_CF_ENDIAN_FLIP_U2( smf_cur->same_locals_1_stack_item_frame_ext.delta )) + 1;
				break;
			case CF_StackMapFrame_ChopFrame_Begin:
			case CF_StackMapFrame_ChopFrame_Begin+1:
			case CF_StackMapFrame_ChopFrame_Begin+2:
				pos += _CF_ENDIAN_FLIP_U2( smf_cur->chop_frame.delta ) + 1;
				break;
			case CF_StackMapFrame_SameFrame_Ext:
				pos += _CF_ENDIAN_FLIP_U2( smf_cur->same_frame_ext.delta ) + 1;
				break;
			case CF_StackMapFrame_AppendFrame_Begin:
			case CF_StackMapFrame_AppendFrame_Begin+1:
			case CF_StackMapFrame_AppendFrame_Begin+2:
				pos += _CF_ENDIAN_FLIP_U2( smf_cur->append_frame.delta ) + 1;
				break;
			case CF_StackMapFrame_FullFrame:
				pos += (_delta_[j]=_CF_ENDIAN_FLIP_U2( smf_cur->full_frame.delta )) + 1;
				break;
			default:
				if( smf_cur->frame_type >= CF_StackMapFrame_SameFrame_Begin &&
					smf_cur->frame_type <= CF_StackMapFrame_SameFrame_End )
				{
					pos += smf_cur->frame_type + 1;
				}
				else if( smf_cur->frame_type >= CF_StackMapFrame_SameLocals_1_Stack_Item_Begin &&
						 smf_cur->frame_type <= CF_StackMapFrame_SameLocals_1_Stack_Item_End )
				{
					pos += (_delta_[j]=smf_cur->frame_type - CF_StackMapFrame_SameLocals_1_Stack_Item_Begin) + 1;
				}
			}

			if( pos >= expinfo[j].handler_pc )
				break;
		}

		if( !smf_cur || pos > expinfo[j].handler_pc )
			return -1;  // exception

		_i_[j] = i;
		//_ofst_[j] = pos;
	}

	// count smf number of stub
	for( ++i; smf_src[i]; ++i );
	//
	smf_num[0] = i + 1;
	smf_num[1] = smf_src_num - smf_num[0];

	// adjust stub's stack frame table
	ext_buf_len = cf_alter_stackmaptab( smf_src, i, smf_src_lens, info->ins_locs2, info->ins_locs_num2, 0, 0, &ext_buf, NULL );
	if( ext_buf_len == -1 )
		return -1;

	ext_buf_len &= -!!ext_buf;  // set zero if no malloc occurred on the ext_buf (ext_buf is null)
	ext_buf = realloc( ext_buf, ext_buf_len + 4096 );
	if( !ext_buf )
		return -1;

	*ext_buf_ret = ext_buf;
	//ext_buf_len = 0;
	m = 0;
	_d_ = 0;

	// =========================================================================================================================================//
	// process ENTRY
	if( info->code_pc > 0 )
	{
		if( info->entry_last_opc_len > 0 )
		{
			// with the ENTRY
			// copy all entry's frames
			for( i=_i_[0], j=0; j <= i; j++ )
			{
				smf_dst[m] = smf_src[j];
				smf_dst_lens[m++] = smf_src_lens[j];
			}

			// changed the last "frame"(assume it is always a 'same_locals_1_stack_item_frame' or 'full_frame')
			j = bytecode[info->entry_pc + info->entry_last_opc_off];
			//_d_ = ((j >= CF_OPC_ireturn) && (j <= CF_OPC_areturn)) || (j == CF_OPC_athrow) || (j == CF_OPC_pop) || (j == CF_OPC_pop2);
			_d_ = j != CF_OPC_return && j != CF_OPC_goto && j != CF_OPC_goto_w;	// skip opcode if it is a terminator besides the 'return',
																				// because there is maybe jumped from a transfer instruction

			_delta_[0] = info->entry_pc + info->entry_last_opc_off
										+ (-_d_ & info->entry_last_opc_len)
										- (cf_count_smf_delta(smf_dst, m-1) - (m>1))
										- (m>1);

			if( _delta_[0] < 0 && !_d_ )
			{
				_delta_[0] = info->entry_last_opc_len - !!m;
				_d_ = 1;
			}

			if( _delta_[0] >= 0 )
			{
				_cf_change_1_stack( _delta_[0], smf_dst[m-1], &smf_dst_lens[m-1] );

				if( !_d_ )
				{
					// insert one frame "same_frame"(skip the 'return')
					smf_dst[m] = (CF_Attr_StackMapFrame_Union*)(ext_buf + ext_buf_len);
					smf_dst[m]->same_frame.type = CF_StackMapFrame_SameFrame_Begin + info->exit_last_opc_len - 1;
					ext_buf_len += smf_dst_lens[m] = sizeof( struct _cf_same_frame );
					m++;
				}
			}
			else
				m--;  // cut the last one

			if( info->exit_pc>0 || info->catch_pc>0 )
			{
				// insert one frame "same_frame"(skip the 'nop', reach at the separator 'goto code_pc')
				smf_dst[m] = (CF_Attr_StackMapFrame_Union*)(ext_buf + ext_buf_len);
				smf_dst[m]->same_frame.type = CF_StackMapFrame_SameFrame_Begin;
				ext_buf_len += smf_dst_lens[m] = sizeof( struct _cf_same_frame );
				m++;
			}
		}
		// always has a separator "nop" or "goto xx" after block ENTRY
		_d_ = info->exit_pc>0 || info->catch_pc>0 ? CF_OPC_LEN[CF_OPC_goto] : CF_OPC_LEN[CF_OPC_nop];
	}

	// =========================================================================================================================================//
	// process EXIT
	if( info->exit_pc > 0 )
	{
		// joint the block EXIT
		smf_dst[m] = (CF_Attr_StackMapFrame_Union*)(ext_buf + ext_buf_len);
		if( info->ret_slots > 0 )
		{
			// with a return value
			// insert one frame "same_locals_1_stack_item_frame" of return value
			CF_Attr_SMFVT_Union *vt = (CF_Attr_SMFVT_Union*)&smf_dst[m]->same_locals_1_stack_item_frame.stack;
			vt->vt_tag = CF_IDX_TO_SMFVT[info->ret_tp_idx];
			if( vt->vt_tag == CF_SMFVT_object )
			{
				// the last item is a return object
				vt->ObjectVar.obj_idx = info->mtd_sign_tab_len > 0 ? info->mtd_sign_tab[info->mtd_sign_tab_len-1].cls_idx : 0;
				_CF_ENDIAN_SWAP_U2( vt->ObjectVar.obj_idx );
			}
			smf_dst[m]->same_locals_1_stack_item_frame.type = CF_StackMapFrame_SameLocals_1_Stack_Item_Begin + _d_ - !!m;
			ext_buf_len += smf_dst_lens[m] = _OFFSET( struct _cf_same_locals_1_stack_item_frame, stack ) + CF_SMFVT_LEN[vt->vt_tag];
			m++;

#if 0 // do not need any more
			if( info->exit_ret_off != 0xffff )
			{
				// insert one frame "append_frame"
				j = cf_opc_len_calcu( &bytecode[info->exit_pc + info->exit_ret_off], 0 ) - 1;
				smf_dst[m] = (CF_Attr_StackMapFrame_Union*)(ext_buf + ext_buf_len);
				smf_dst[m]->append_frame.type = CF_StackMapFrame_AppendFrame_Begin;
				smf_dst[m]->append_frame.delta = _CF_ENDIAN_FLIP_U2(j);
				smf_dst[m]->append_frame.locals[0].tag = CF_IDX_TO_SMFVT[info->ret_tp_idx];
				ext_buf_len += smf_dst_lens[m] = _OFFSET( struct _cf_append_frame, locals ) + CF_SMFVT_LEN[CF_IDX_TO_SMFVT[info->ret_tp_idx]];
				m++;
			}
#endif
		}
		else
		{
			// without return value
			// insert one frame "same_frame"
			smf_dst[m]->same_frame.type = CF_StackMapFrame_SameFrame_Begin + _d_ - !!m;
			ext_buf_len += smf_dst_lens[m] = sizeof( struct _cf_same_frame );
			m++;
		}
		_d_ = 0;

		if( _i_[1] - _i_[0] > 1 )
		{
			// copy EXIT's frames besides the first one
			for( i=_i_[1], j=_i_[0]+2; j <= i; j++ )  // skip one which between the ENTRY end EXIT(clause "catch")
			{
				smf_dst[m] = smf_src[j];
				smf_dst_lens[m++] = smf_src_lens[j];
			}
			//i -= _i_[0];  // i - (_i_[0] + 2) + 1 + 1/*append before*/, number of frame belong to block EXIT

			// changed the last "frame"(assume it is always a 'same_locals_1_stack_item_frame' or 'full_frame')
			j = bytecode[info->exit_pc + info->exit_last_opc_off];
			_d_ = j != CF_OPC_return;	// skip opcode if it is a terminator besides the 'return',
										// because there is maybe jumped from a transfer instruction

			_delta_[1] = info->exit_pc + info->exit_last_opc_off
									   + (-_d_ & info->exit_last_opc_len)
									   - (cf_count_smf_delta(smf_dst, m-1) - 1/*(m>1)*/)
									   - 1/*(i>1)*/;

			if( _delta_[1] < 0 && !_d_ )
			{
				_delta_[1] = info->exit_last_opc_len - 1;
				_d_ = 1;
			}

			if( _delta_[1] >= 0 )
			{
				_cf_change_1_stack( _delta_[1], smf_dst[m-1], &smf_dst_lens[m-1] );

				if( !_d_ )
				{
					// insert one frame "same_frame"(skip the 'return')
					smf_dst[m] = (CF_Attr_StackMapFrame_Union*)(ext_buf + ext_buf_len);
					smf_dst[m]->same_frame.type = CF_StackMapFrame_SameFrame_Begin + info->exit_last_opc_len - 1;
					ext_buf_len += smf_dst_lens[m] = sizeof( struct _cf_same_frame );
					m++;
				}
			}
			else
				m--;  // cut the last one


			if( info->catch_pc > 0 )
			{
				// insert one frame "same_frame"(skip the 'nop', reach at the separator 'goto 0')
				smf_dst[m] = (CF_Attr_StackMapFrame_Union*)(ext_buf + ext_buf_len);
				smf_dst[m]->same_frame.type = CF_StackMapFrame_SameFrame_Begin;
				ext_buf_len += smf_dst_lens[m] = sizeof( struct _cf_same_frame );
				m++;
			}
			// always has a separator "nop" or "goto 0" after block EXIT
			_d_ = info->catch_pc > 0 ? CF_OPC_LEN[CF_OPC_goto] : CF_OPC_LEN[CF_OPC_nop];
		}
	}

	// =========================================================================================================================================//
	// process CATCH
	if( info->catch_pc > 0 )
	{
		// joint the block CATCH
		// insert one frame "same_locals_1_stack_item_frame" of thrown object
		smf_dst[m] = (CF_Attr_StackMapFrame_Union*)(ext_buf + ext_buf_len);
		smf_dst[m]->same_locals_1_stack_item_frame.type = CF_StackMapFrame_SameLocals_1_Stack_Item_Begin + _d_ - !!m;
		CF_Attr_SMFVT_Object *vt = (CF_Attr_SMFVT_Object*)&smf_dst[m]->same_locals_1_stack_item_frame.stack;
		vt->tag = CF_SMFVT_object;
		vt->obj_idx = _CF_ENDIAN_FLIP_INDEX( info->catch_type );
		ext_buf_len += smf_dst_lens[m] = _OFFSET( struct _cf_same_locals_1_stack_item_frame, stack ) + CF_SMFVT_LEN[CF_SMFVT_object];
		m++;

#if 0	// do not need any more
		// insert one frame "append_frame"
		_d_ = cf_opc_len_calcu( &bytecode[info->catch_pc], 0 );
		j = _d_ - 1;
		smf_dst[m] = (CF_Attr_StackMapFrame_Union*)(ext_buf + ext_buf_len);
		smf_dst[m]->append_frame.type = CF_StackMapFrame_AppendFrame_Begin;
		smf_dst[m]->append_frame.delta = _CF_ENDIAN_FLIP_U2(j);
		*(CF_Attr_SMFVT_Object*)smf_dst[m]->append_frame.locals = *vt;
		ext_buf_len += smf_dst_lens[m] = _OFFSET( struct _cf_append_frame, locals ) + CF_SMFVT_LEN[CF_SMFVT_object];
		m++;
#endif
		// copy catch's frames
		smf_num[0]--;
		i = m;
		for( j=_i_[2]+1; j < smf_num[0]; j++ )  // skip the frames before the clause "catch()"
		{
			smf_dst[m] = smf_src[j];
			smf_dst_lens[m++] = smf_src_lens[j];
		}
		smf_num[0]++;

		//if( i != m )
		//	_delta_[2] = cf_count_smf_delta( &smf_dst[m-1], 1 ) - 1;
		//_d_ = info->code_pc - (info->catch_pc + _d_ + cf_count_smf_delta( smf_dst+i, m-i ) + 1);

		// always has a separator "nop" after block CATCH
		_d_ = info->code_pc - 1 - (info->catch_pc + cf_count_smf_delta(smf_dst+i, m-i));
	}

	if( _d_ > 1 )
	{
		// insert one frame "full_frame"
		smf_dst[m] = (CF_Attr_StackMapFrame_Union*)(ext_buf + ext_buf_len);
		ext_buf_len += smf_dst_lens[m] = cf_fill_smf_full_frame( _d_-1, info->mtd_sign_tab, info->mtd_sign_tab_len-1, NULL, 0, smf_dst[m], info->this_var );
		m++;
	}

	if( m > 0 )
	{
		// at this location, assume here always is a separator "nop"

		// insert one frame "same_frame"
		smf_dst[m] = (CF_Attr_StackMapFrame_Union*)(ext_buf + ext_buf_len);
		smf_dst[m]->same_frame.type = CF_StackMapFrame_SameFrame_Begin;
		ext_buf_len += smf_dst_lens[m] = sizeof( struct _cf_same_frame );
		m++;
	}

	// =========================================================================================================================================//
	// process ORIGINAL CODE
	if( smf_src[i = smf_num[0]] )
	{
		// copy frames of target code
		j = smf_num[1] - 1;
		memcpy( &smf_dst[m], &smf_src[i], sizeof(void*)*j );
		memcpy( &smf_dst_lens[m], &smf_src_lens[i], sizeof(int)*j );

		if( m > 0 )
		{
			if( cf_alter_smf_delta( smf_dst[m], -1, TRUE ) == -1 )
				memcpy( smf_dst+m, smf_dst+m+1, --j );  // remove the first one

			for( i=0; i < m; i++ )
			{
				if( smf_dst[i]->frame_type == CF_StackMapFrame_FullFrame )
					_cf_change_fullframe_vt_object( smf_dst[i], info->mtd_sign_tab, info->mtd_sign_tab_len-1, info->this_var );
			}
		}
		m += j;
	}

	return m;
}


// another version as cf_blend_stkmaptab, flip stack frame order against the cf_blend_stkmaptab
static int cf_blend_stkmaptab_v2( CF_Attr_StackMapFrame_Union **smf_src,
								  int *smf_src_lens,
								  int smf_src_num,
								  CF_Attr_StackMapFrame_Union **smf_dst,
								  int *smf_dst_lens,
								  void **ext_buf_ret,
								  const CF_ExpInfo *stub_exptab,
								  const juint8 *bytecode,
								  CF_CodeTransformInfo *info )
{
	CF_Attr_StackMapFrame_Union *smf_cur;
	CF_ExpInfo expinfo[3];
	int m, _m_, smf_num[2], ext_buf_len;
	int i=-1, pos = -1, j;
	int _i_[3], _delta_[3], _d_;//, _ofst_[3];
	void *ext_buf;
	BOOL is_init_mtd;


	// seek the last frame of each try-catch-block
	for( m=j=0; j < 3; j++ )
	{
		expinfo[j].start_pc = _CF_ENDIAN_FLIP_U2( stub_exptab[j].start_pc );
		expinfo[j].end_pc = _CF_ENDIAN_FLIP_U2( stub_exptab[j].end_pc );
		expinfo[j].handler_pc = _CF_ENDIAN_FLIP_U2( stub_exptab[j].handler_pc );

		//
		for( ; m < info->ins_locs_num2 && info->ins_locs2[m].ext == j+1; m++ )
		{
			info->ins_locs2[m].pos += expinfo[j].start_pc;
		}

		// check stub's delta and save the last frame's delta in each block
		for( ++i; (smf_cur=smf_src[i]); ++i )
		{
			switch( smf_cur->frame_type )
			{
			case CF_StackMapFrame_SameLocals_1_Stack_Item_Ext:
				pos += (_delta_[j]=_CF_ENDIAN_FLIP_U2( smf_cur->same_locals_1_stack_item_frame_ext.delta )) + 1;
				break;
			case CF_StackMapFrame_ChopFrame_Begin:
			case CF_StackMapFrame_ChopFrame_Begin+1:
			case CF_StackMapFrame_ChopFrame_Begin+2:
				pos += _CF_ENDIAN_FLIP_U2( smf_cur->chop_frame.delta ) + 1;
				break;
			case CF_StackMapFrame_SameFrame_Ext:
				pos += _CF_ENDIAN_FLIP_U2( smf_cur->same_frame_ext.delta ) + 1;
				break;
			case CF_StackMapFrame_AppendFrame_Begin:
			case CF_StackMapFrame_AppendFrame_Begin+1:
			case CF_StackMapFrame_AppendFrame_Begin+2:
				pos += _CF_ENDIAN_FLIP_U2( smf_cur->append_frame.delta ) + 1;
				break;
			case CF_StackMapFrame_FullFrame:
				pos += (_delta_[j]=_CF_ENDIAN_FLIP_U2( smf_cur->full_frame.delta )) + 1;
				break;
			default:
				if( smf_cur->frame_type >= CF_StackMapFrame_SameFrame_Begin &&
					smf_cur->frame_type <= CF_StackMapFrame_SameFrame_End )
				{
					pos += smf_cur->frame_type + 1;
				}
				else if( smf_cur->frame_type >= CF_StackMapFrame_SameLocals_1_Stack_Item_Begin &&
						 smf_cur->frame_type <= CF_StackMapFrame_SameLocals_1_Stack_Item_End )
				{
					pos += (_delta_[j]=smf_cur->frame_type - CF_StackMapFrame_SameLocals_1_Stack_Item_Begin) + 1;
				}
			}

			if( pos >= expinfo[j].handler_pc )
				break;
		}

		if( !smf_cur || pos > expinfo[j].handler_pc )
			return -1;  // exception

		_i_[j] = i;
		//_ofst_[j] = pos;
	}

	// count smf number of stub
	for( ++i; smf_src[i]; ++i );
	//
	smf_num[0] = i + 1;
	smf_num[1] = smf_src_num - smf_num[0];

	// adjust stub's stack frame table
	ext_buf_len = cf_alter_stackmaptab( smf_src, i, smf_src_lens, info->ins_locs2, info->ins_locs_num2, 0, 0, &ext_buf, NULL );
	if( ext_buf_len == -1 )
		return -1;

	ext_buf_len &= -!!ext_buf;  // set zero if no malloc occurred on the ext_buf (ext_buf is null)
	ext_buf = realloc( ext_buf, ext_buf_len + 4096 );
	if( !ext_buf )
		return -1;

	*ext_buf_ret = ext_buf;
	//ext_buf_len = 0;
	m = 0;
	_d_ = 0;
	is_init_mtd = strcmp(info->mtd_name, "<init>") == 0;

	if( info->entry_last_opc_len > 0 )
	{
		// insert one frame "same_frame" at the first separator "nop" before the ORIGINAL CODE
		smf_dst[m] = (CF_Attr_StackMapFrame_Union*)(ext_buf + ext_buf_len);
		smf_dst[m]->same_frame.type = CF_StackMapFrame_SameFrame_Begin + CF_OPC_LEN[CF_OPC_goto_w];
		ext_buf_len += smf_dst_lens[m] = sizeof( struct _cf_same_frame );
		m++;
	}

	// =========================================================================================================================================//
	// process ORIGINAL CODE
	i = smf_num[0];
	j = smf_num[1] - 1;
	if( j > 0 )
	{
		// copy frames of target code
		memcpy( &smf_dst[m], &smf_src[i], sizeof(void*)*j );
		memcpy( &smf_dst_lens[m], &smf_src_lens[i], sizeof(int)*j );

		if( m > 0 )
			cf_alter_smf_delta( smf_dst[m], -1, TRUE );
		m += j;
	}
	_m_ = m;

	// insert one frame "full_frame" at the separator "nop" after the ORIGINAL CODE
	_d_ = info->code_pc + info->code_ln - cf_count_smf_delta( smf_dst, m );
	smf_dst[m] = (CF_Attr_StackMapFrame_Union*)(ext_buf + ext_buf_len);
	ext_buf_len += smf_dst_lens[m] = cf_fill_smf_full_frame( _d_, info->mtd_sign_tab, info->mtd_sign_tab_len-1, NULL, 0, smf_dst[m],
										  (is_init_mtd && info->entry_last_opc_len > 0 ? (void*)-1 : info->this_var) );
	m++;

	_d_ = CF_OPC_LEN[CF_OPC_nop];

	// =========================================================================================================================================//
	// process ENTRY
	if( info->entry_last_opc_len > 0 )
	{
		// with the ENTRY
		// copy all entry's frames
		for( i=_i_[0], j=0; j <= i; j++ )
		{
			smf_dst[m] = smf_src[j];
			smf_dst_lens[m++] = smf_src_lens[j];
		}

		// changed the last "frame"(assume it is always a 'same_locals_1_stack_item_frame' or 'full_frame')
		j = bytecode[info->entry_pc + info->entry_last_opc_off];
		_d_ = j != CF_OPC_return && j != CF_OPC_goto && j != CF_OPC_goto_w;	// skip opcode if it is a terminator besides the 'return',
																			// because there is maybe jumped from a transfer instruction

		_delta_[0] = info->entry_pc + info->entry_last_opc_off
									+ (-_d_ & info->entry_last_opc_len)
									- (cf_count_smf_delta(smf_dst, m-1) - (m>1))
									- (m>1);

		if( _delta_[0] < 0 && !_d_ )
		{
			_delta_[0] = info->entry_last_opc_len - !!m;
			_d_ = 1;
		}

		if( _delta_[0] >= 0 )
		{
			_cf_change_1_stack( _delta_[0], smf_dst[m-1], &smf_dst_lens[m-1] );

			if( !_d_ && (info->exit_pc>0 || info->catch_pc>0) )
			{
				// insert one frame "same_frame"(skip the 'return')
				smf_dst[m] = (CF_Attr_StackMapFrame_Union*)(ext_buf + ext_buf_len);
				smf_dst[m]->same_frame.type = CF_StackMapFrame_SameFrame_Begin + info->exit_last_opc_len - 1;
				ext_buf_len += smf_dst_lens[m] = sizeof( struct _cf_same_frame );
				m++;
			}
		}
		else
			m--;  // cut the last one

		if( info->exit_pc>0 || info->catch_pc>0 )
		{
			// insert one frame "same_frame"(skip the 'nop', reach at the separator 'goto code_pc')
			smf_dst[m] = (CF_Attr_StackMapFrame_Union*)(ext_buf + ext_buf_len);
			smf_dst[m]->same_frame.type = CF_StackMapFrame_SameFrame_Begin;
			ext_buf_len += smf_dst_lens[m] = sizeof( struct _cf_same_frame );
			m++;
			_d_ = CF_OPC_LEN[CF_OPC_goto_w];
		}
		else
			_d_ = 0;
	}

	// =========================================================================================================================================//
	// process EXIT
	if( info->exit_pc > 0 )
	{
		// joint the block EXIT
		smf_dst[m] = (CF_Attr_StackMapFrame_Union*)(ext_buf + ext_buf_len);
		if( info->ret_slots > 0 )
		{
			// with a return value
			if( is_init_mtd && info->entry_last_opc_len > 0 )
			{
				// insert one frame "full_frame"
				CF_SignExt stk_1 = {
						.sym = NULL,
						.len = 0,
						.type_idx = info->ret_tp_idx,
						.cls_idx = 0
				};

				if( CF_IDX_TO_SMFVT[stk_1.type_idx] == CF_SMFVT_object )
					stk_1.cls_idx = info->mtd_sign_tab_len > 0 ? info->mtd_sign_tab[info->mtd_sign_tab_len-1].cls_idx : 0;

				ext_buf_len += smf_dst_lens[m] = cf_fill_smf_full_frame( _d_-!!m, info->mtd_sign_tab, info->mtd_sign_tab_len-1,
																		 &stk_1, 1, smf_dst[m], info->this_var );
			}
			else
			{
				// insert one frame "same_locals_1_stack_item_frame" of return value
				CF_Attr_SMFVT_Union *vt = (CF_Attr_SMFVT_Union*)&smf_dst[m]->same_locals_1_stack_item_frame.stack;
				vt->vt_tag = CF_IDX_TO_SMFVT[info->ret_tp_idx];
				if( vt->vt_tag == CF_SMFVT_object )
				{
					// the last item is a return object
					vt->ObjectVar.obj_idx = info->mtd_sign_tab_len > 0 ? info->mtd_sign_tab[info->mtd_sign_tab_len-1].cls_idx : 0;
					_CF_ENDIAN_SWAP_U2( vt->ObjectVar.obj_idx );
				}
				smf_dst[m]->same_locals_1_stack_item_frame.type = CF_StackMapFrame_SameLocals_1_Stack_Item_Begin + _d_ - !!m;
				ext_buf_len += smf_dst_lens[m] = _OFFSET( struct _cf_same_locals_1_stack_item_frame, stack ) + CF_SMFVT_LEN[vt->vt_tag];
			}
		}
		else
		{
			// without return value
			if( is_init_mtd && info->entry_last_opc_len > 0 )
			{
				// insert one frame "full_frame"
				ext_buf_len += smf_dst_lens[m] = cf_fill_smf_full_frame( _d_-!!m, info->mtd_sign_tab, info->mtd_sign_tab_len-1,
																		 NULL, 0, smf_dst[m], info->this_var );
			}
			else
			{
				// insert one frame "same_frame"
				smf_dst[m]->same_frame.type = CF_StackMapFrame_SameFrame_Begin + _d_ - !!m;
				ext_buf_len += smf_dst_lens[m] = sizeof( struct _cf_same_frame );
			}
		}
		m++;
		_d_ = 0;

		if( _i_[1] - _i_[0] > 1 )
		{
			// copy EXIT's frames besides the first one
			for( i=_i_[1], j=_i_[0]+2; j <= i; j++ )  // skip one which between the ENTRY end EXIT(clause "catch")
			{
				smf_dst[m] = smf_src[j];
				smf_dst_lens[m++] = smf_src_lens[j];
			}
			//i -= _i_[0];  // i - (_i_[0] + 2) + 1 + 1/*append before*/, number of frame belong to block EXIT

			// changed the last "frame"(assume it is always a 'same_locals_1_stack_item_frame' or 'full_frame')
			j = bytecode[info->exit_pc + info->exit_last_opc_off];
			_d_ = j != CF_OPC_return;	// skip opcode if it is a terminator besides the 'return',
										// because there is maybe jumped from a transfer instruction

			_delta_[1] = info->exit_pc + info->exit_last_opc_off
									   + (-_d_ & info->exit_last_opc_len)
									   - (cf_count_smf_delta(smf_dst, m-1) - 1/*(m>1)*/)
									   - 1/*(i>1)*/;

			if( _delta_[1] < 0 && !_d_ )
			{
				_delta_[1] = info->exit_last_opc_len - 1;
				_d_ = 1;
			}

			if( _delta_[1] >= 0 )
			{
				_cf_change_1_stack( _delta_[1], smf_dst[m-1], &smf_dst_lens[m-1] );

				if( !_d_ && info->catch_pc > 0 )
				{
					// insert one frame "same_frame"(skip the 'return')
					smf_dst[m] = (CF_Attr_StackMapFrame_Union*)(ext_buf + ext_buf_len);
					smf_dst[m]->same_frame.type = CF_StackMapFrame_SameFrame_Begin + info->exit_last_opc_len - 1;
					ext_buf_len += smf_dst_lens[m] = sizeof( struct _cf_same_frame );
					m++;
				}
			}
			else
				m--;  // cut the last one

			if( info->catch_pc>0 )
			{
				// insert one frame "same_frame"(skip the 'nop', reach at the separator 'goto 0')
				smf_dst[m] = (CF_Attr_StackMapFrame_Union*)(ext_buf + ext_buf_len);
				smf_dst[m]->same_frame.type = CF_StackMapFrame_SameFrame_Begin;
				ext_buf_len += smf_dst_lens[m] = sizeof( struct _cf_same_frame );
				m++;
				_d_ = CF_OPC_LEN[CF_OPC_goto];
			}
			else
				_d_ = 0;
		}
	}

	// =========================================================================================================================================//
	// process CATCH
	if( info->catch_pc > 0 )
	{
		// joint the block CATCH
		smf_dst[m] = (CF_Attr_StackMapFrame_Union*)(ext_buf + ext_buf_len);
		/*if( is_init_mtd )
		{
			// insert one frame "full_frame"
			CF_SignExt stk_1 = {
					.sym = NULL,
					.len = 0,
					.type_idx = cf_get_signature_index( JVM_SIGNATURE_CLASS ),
					.cls_idx = info->catch_type
			};
			ext_buf_len += smf_dst_lens[m] = cf_fill_smf_full_frame( _d_-!!m, info->mtd_sign_tab, info->mtd_sign_tab_len-1,
																	 &stk_1, 1, smf_dst[m], info->this_var );
		}
		else*/
		{
			// insert one frame "same_locals_1_stack_item_frame" of thrown object
			smf_dst[m]->same_locals_1_stack_item_frame.type = CF_StackMapFrame_SameLocals_1_Stack_Item_Begin + _d_ - !!m;
			CF_Attr_SMFVT_Object *vt = (CF_Attr_SMFVT_Object*)&smf_dst[m]->same_locals_1_stack_item_frame.stack;
			vt->tag = CF_SMFVT_object;
			vt->obj_idx = _CF_ENDIAN_FLIP_INDEX( info->catch_type );
			ext_buf_len += smf_dst_lens[m] = _OFFSET( struct _cf_same_locals_1_stack_item_frame, stack ) + CF_SMFVT_LEN[CF_SMFVT_object];
		}
		m++;

		// copy catch's frames
		smf_num[0]--;
		i = m;
		for( j=_i_[2]+1; j < smf_num[0]; j++ )  // skip the frames before the clause "catch()"
		{
			smf_dst[m] = smf_src[j];
			smf_dst_lens[m++] = smf_src_lens[j];
		}
		smf_num[0]++;

		//
		//_d_ = info->code_pc - 1 - (info->catch_pc + cf_count_smf_delta(smf_dst+i, m-i));
	}

	for( i=_m_; i < m; i++ )
	{
		if( smf_dst[i]->frame_type == CF_StackMapFrame_FullFrame )
			_cf_change_fullframe_vt_object( smf_dst[i], info->mtd_sign_tab, info->mtd_sign_tab_len-1, info->this_var );
	}

	return m;
}


// Function: merge critical attribute "BootstrapMethods" if possible(migrated from the cf_transform_class)
static CF_Attr_BootstrapMethod* _cf_merge_bootstrap_methods( cf_idx_t attr_btsmtd_idx,
															 CF_Attr_BootstrapMethod *attr_btsmtd,
															 CF_Attr_BootstrapMethod *stub_attr_btsmtd,
															 int cp_off,
															 CF_ClassFileMap *cfmap )
{
	if( !stub_attr_btsmtd )
		return NULL;

	int k, n, m;
	cf_count_t cnt = _CF_ENDIAN_FLIP_COUNT( stub_attr_btsmtd->num_methods );

	if( !attr_btsmtd )
	{
		k = cfmap->attrtab[0].attr_count+1;
		n = 0;
	}
	else
	{
		k = 0;
		n = _CF_ENDIAN_FLIP_U4(attr_btsmtd->attr_len) - sizeof(cf_count_t);
		cnt += _CF_ENDIAN_FLIP_COUNT( attr_btsmtd->num_methods );
	}

	m = _CF_ENDIAN_FLIP_U4(stub_attr_btsmtd->attr_len) - sizeof(cf_count_t);

	void *ptr = cf_malloc( cfmap->auxi.use_sysheap, &cfmap->auxi.mmphd,
					 	   _OFFSET(CF_Attr_BootstrapMethod, methods)+n+m+sizeof(void*)*k );
	if( !ptr )
		return NULL;

	CF_Attr_BootstrapMethod *new_btsmtd = (CF_Attr_BootstrapMethod*)( ptr + sizeof(void*)*k );

	memcpy( (void*)new_btsmtd->methods.ite, (void*)attr_btsmtd->methods.ite, n );
	memcpy( (void*)new_btsmtd->methods.ite+n, (void*)stub_attr_btsmtd->methods.ite, m );

	// alert cpool index of BootstrapMethod
	cf_alter_BootstrapMethod( (CF_BootstrapMethod*)((void*)new_btsmtd->methods.ite+n),
							  _CF_ENDIAN_FLIP_COUNT(stub_attr_btsmtd->num_methods), cp_off );

	n += m + sizeof(cf_count_t);
	new_btsmtd->attr_len = _CF_ENDIAN_FLIP_U4( n );
	new_btsmtd->num_methods = _CF_ENDIAN_FLIP_COUNT( cnt );
	if( attr_btsmtd )
	{
		// update
		new_btsmtd->attr_nm_idx = attr_btsmtd->attr_nm_idx;
	}
	else
	{
		// append
		cf_get_cpitem_utf8( cfmap, "BootstrapMethods", 16, &new_btsmtd->attr_nm_idx );
		_CF_ENDIAN_SWAP_INDEX( new_btsmtd->attr_nm_idx );

		// change the original attribute table
		attr_btsmtd_idx = k - 1;
		memcpy( ptr, (void*)cfmap->attrtab[0].attr, attr_btsmtd_idx*sizeof(void*) );
		cfmap->attrtab[0].attr = (CF_AttrInfo**)ptr;
		cfmap->attrtab[0].attr_count = (cf_count_t)k;
	}
	cfmap->attrtab[0].attr[attr_btsmtd_idx] = (CF_AttrInfo*)new_btsmtd;


	// it is not necessary to free old attr_btsmtd immediately
	return new_btsmtd;
}


//
static ssize_t _cf_clear_attr_variable( CF_AttrInfo *code_attr, void *new_method, ssize_t *new_method_len, CF_ClassFileMap *cfmap )
{
	int i, d;
	ssize_t ll, l, ds = 0;
	void *ptr;
	CF_Attr_LocalVariableTable *loctab;
	const char *attr_name[] = {"LocalVariableTable", "LocalVariableTypeTable"};

	cf_count_t code_attr_cnt = *((cf_count_t*)code_attr - 1);
	_CF_ENDIAN_SWAP_COUNT( code_attr_cnt );

	for( i=1; i >= 0; i-- )
	{
		loctab = (CF_Attr_LocalVariableTable*)
						cf_seek_attrinfo( code_attr, code_attr_cnt, attr_name[i], strlen(attr_name[i]), 0, cfmap );
		if( loctab && loctab->locals_len )
		{
			l = _CF_ENDIAN_FLIP_U4( loctab->attr_len );
			ptr = (void*)loctab + l + sizeof(CF_AttrHead);
			ll = *new_method_len - (ptr - (void*)new_method);
			memmove( (void*)loctab->locals, ptr, ll );

			d = (sizeof(CF_Attr_LocalVariableTable) - sizeof(CF_AttrHead));
			loctab->attr_len = _CF_ENDIAN_FLIP_U4( d );
			loctab->locals_len = 0;

			l -= d;
			ds += l;
			*new_method_len -= l;
		}
	}
	return ds;
}

//
static ssize_t _cf_clear_attr_linenumber( CF_AttrInfo *code_attr, void *new_method, ssize_t *new_method_len, CF_ClassFileMap *cfmap )
{
	int d = 0;
	ssize_t ll, l=0;
	void *ptr;
	CF_Attr_LineNumberTable *tab;
	const char attr_name[] = "LineNumberTable";

	cf_count_t code_attr_cnt = *((cf_count_t*)code_attr - 1);
	_CF_ENDIAN_SWAP_COUNT( code_attr_cnt );

	tab = (CF_Attr_LineNumberTable*)
					cf_seek_attrinfo( code_attr, code_attr_cnt, attr_name, sizeof(attr_name)-1, 0, cfmap );
	if( tab && tab->lines )
	{
		l = _CF_ENDIAN_FLIP_U4( tab->attr_len );
		ptr = (void*)tab + l + sizeof(CF_AttrHead);
		ll = *new_method_len - (ptr - (void*)new_method);
		memmove( (void*)tab->lntab, ptr, ll );

		d = (sizeof(CF_Attr_LineNumberTable) - sizeof(CF_AttrHead));
		tab->attr_len = _CF_ENDIAN_FLIP_U4( d );
		tab->lines = 0;

		l -= d;
		*new_method_len -= l;
	}
	return l;
}


// Function: transform specified method according to SABI
CF_MethodInfo* cf_transform_method( const CF_MethodInfo *method, ssize_t method_len, CF_ClassFileMap *cfmap,
									const CF_MethodInfo *stub, ssize_t stub_len, const CF_ClassFileMap *stub_cfmap,
									ssize_t *new_method_len, const char *method_name, const char *method_desc,
									uint8 eecm, int cpi_tpix, CF_SignExt *retval_sign )
{
	// Note: assumes byte order of attribute is big endian, others is local endian

	const CF_Attr_Code *method_code, *stub_code;
	const juint8 *srcbuf[5] = {NULL};
	int srclen[5] = {0};
	juint8 *dstbuf = NULL;
	int dstlen;
	ssize_t len;


	// seek the attribute "Code" of the stub
	stub_code = (CF_Attr_Code*)cf_seek_attrinfo( stub->attr.ite, stub->attr_count, "Code", 4, 0, stub_cfmap );
	if( !stub_code )
		return NULL;
	len = stub_code->code_len;
	_CF_ENDIAN_SWAP_U4( len );

	// try to verify validation of the stub
	cf_idx_t stub_catch_type;
	if( cf_verify_stub_method(stub_code, len, &srcbuf[1], &srclen[1], &stub_catch_type) != 0 )
	{
		// try another format as following:
		// | impdep1 | ENTER_LEN | impdep1 | EXIT_LEN | impdep1 | CATCH_LEN | CATCH_TYPE | ENTRY BYTECODE | EXIT BYTECODE | CATCH BYTECODE | CRC32 |
		// 0         1           3         4          6         7           9            11               N               M                X       X+4
		return NULL;  // has been disabled

		if( cf_check_stubcode( stub_code->code.byte, (int)len, &srcbuf[1], &srclen[1], &stub_catch_type ) != 0 )
			return NULL;

		// HEAD, equal to the ENTRY block
		srcbuf[4] = srcbuf[1];
		srclen[4] = srclen[1];
	}
	else
	{
		// HEAD, prior to the ENTRY block
		srcbuf[4] = stub_code->code.byte;
		srclen[4] = (int)(size_t)(srcbuf[1] - srcbuf[4]);
	}
	// now, both of srcbuf[1~4] and srclen[1~4] have been set by verify function

	// EEC Mask(2022-06-27)
	if( eecm & 1 )
		srcbuf[1] = NULL, srclen[1] = 0;
	if( eecm & 2 )
		srcbuf[2] = NULL, srclen[2] = 0;
	if( eecm & 4 )
		srcbuf[3] = NULL, srclen[3] = 0;


	// seek the attribute "Code" of the target method
	method_code = (CF_Attr_Code*)cf_seek_attrinfo( method->attr.ite, method->attr_count, "Code", 4, 0, cfmap );
	if( !method_code )
		return NULL;

	srcbuf[0] = method_code->code.byte;
	srclen[0] = method_code->code_len;
	_CF_ENDIAN_SWAP_U4( srclen[0] );


	// exception table of the target "Code"
	CF_ExpInfo *exptab_ptr = (CF_ExpInfo*)(method_code->code.byte + srclen[0] + sizeof(cf_count_t));
	cf_count_t exptab_len = *((cf_count_t*)exptab_ptr - 1);
	_CF_ENDIAN_SWAP_COUNT( exptab_len );

	// parse method descriptor
	CF_SignExt this_var;
	CF_SignExt *mtd_sign_tab = NULL;
	int mtd_sign_tab_len = cf_parse_method_descriptor( method_desc, strlen(method_desc), &mtd_sign_tab, cfmap );
	if( !mtd_sign_tab )
		return NULL;

	// transform byte-code
	CF_CodeTransformInfo info = {
			.cp_off = cfmap->cptab[0].cp_count-1,
			.btsmtd_off = 0,
			.mtd_name = method_name,
			.mtd_desc = method_desc,
			.ins_locs = NULL,
			.this_var = NULL
	};

	if( strcmp(method_name, "<init>") == 0 )
		dstlen = cf_bytecode_transform_v3( srcbuf, srclen, &dstbuf, 0, exptab_ptr, exptab_len, &info, 0, cpi_tpix );
	else
		dstlen = cf_bytecode_transform_v2( srcbuf, srclen, &dstbuf, 0, exptab_ptr, exptab_len, &info, 0, cpi_tpix );
	if( dstlen <= 0 )
	{
		free( mtd_sign_tab );
		return NULL;
	}

	if( mtd_sign_tab_len > 0 && retval_sign )
	{
		if( mtd_sign_tab[mtd_sign_tab_len-1].res[0] == 2 )
		{
			// use new cp-item if value type of return is missing
			mtd_sign_tab[mtd_sign_tab_len-1].cls_idx = retval_sign->cls_idx;
			*retval_sign = mtd_sign_tab[mtd_sign_tab_len-1];
		}
	}

	info.mtd_sign_tab = mtd_sign_tab;
	info.mtd_sign_tab_len = mtd_sign_tab_len;

	if( !method->access.STATIC )
	{
		// 2023-02-25
		info.this_var = &this_var;
		this_var.cls_idx = cfmap->this_class;
		this_var.sym = NULL;
		this_var.len = 0;
		this_var.type_idx = sizeof(CF_SIGNATURE_SYMBOL) - 4;  // Class

		const CP_InfoUnion *cpi = (const CP_InfoUnion*)cf_get_cpitem( cfmap, cfmap->this_class );
		if( cpi )
		{
			cpi = (const CP_InfoUnion*)cf_get_cpitem( cfmap, cpi->Class.name_idx );
			if( cpi )
			{
				this_var.sym = (char*)cpi->Utf8.b;
				this_var.len = cpi->Utf8.len;
			}
		}
	}

	// allocate memory for the new method
	int diff = (dstlen - srclen[0]) + (sizeof(CF_ExpInfo) & -!!srclen[3]);
	len = method_len + diff;
	CF_MethodInfo *new_method = (CF_MethodInfo*)cf_malloc(
						cfmap->auxi.use_sysheap, &cfmap->auxi.mmphd, len+1024 );  // always reserves 1024 bytes for potential transformation
	if( new_method )
	{
		// pack new method up

		ssize_t m = (ssize_t)method_code - (ssize_t)method;
		CF_Attr_Code *attr_code_new = (CF_Attr_Code*)((void*)new_method + m);  // new method's "Code" attribute
		CF_ExpInfo *exptab_new = (CF_ExpInfo*)(attr_code_new->code.byte + dstlen + sizeof(cf_count_t));  // "Exception Table" of "Code" of new method
		cf_count_t *exptab_new_len = ((cf_count_t*)exptab_new - 1);

		// contents, before the attribute "Code"
		memcpy( (void*)new_method, (void*)method, m );

		// contents, attribute "Code"
		cf_count_t cnt[4] = {
			stub_code->max_stack,
			stub_code->max_locals,
			method_code->max_stack,
			method_code->max_locals
		};
		_CF_ENDIAN_SWAP_COUNT( cnt[0] );
		_CF_ENDIAN_SWAP_COUNT( cnt[1] );
		_CF_ENDIAN_SWAP_COUNT( cnt[2] );
		_CF_ENDIAN_SWAP_COUNT( cnt[3] );

		cnt[0] += info.stk_add;
		//if( cnt[0] > cnt[2] )
			cnt[2] += cnt[0]; // max_stack, choose greater one

		cnt[1] += info.var_add;
		if( cnt[1] > cnt[3] )
			cnt[3] = cnt[1]; // max locals, choose greater one

		// head of "Code"
		m = method_code->attr_len;
		m = _CF_ENDIAN_FLIP_U4( m ) + diff;
		attr_code_new->attr_len = _CF_ENDIAN_FLIP_U4( m );
		attr_code_new->attr_nm_idx = method_code->attr_nm_idx;
		attr_code_new->max_stack = _CF_ENDIAN_FLIP_COUNT( cnt[2] );
		attr_code_new->max_locals = _CF_ENDIAN_FLIP_COUNT( cnt[3] );

		// byte-code of the "Code"
		attr_code_new->code_len = _CF_ENDIAN_FLIP_U4( dstlen );
		memcpy( attr_code_new->code.byte, dstbuf, dstlen );

		// exception table of the "Code"
		m = exptab_len;
		if( srclen[3] )
		{
			// append exception table entry
			info.catch_type = stub_catch_type + cfmap->cptab[0].cp_count - 1;
			exptab_new[m].catch_type = _CF_ENDIAN_FLIP_U2( info.catch_type );
			exptab_new[m].handler_pc = _CF_ENDIAN_FLIP_U2( info.catch_pc );
			exptab_new[m].end_pc = _CF_ENDIAN_FLIP_U2( info.catch_end_pc );
			exptab_new[m].start_pc = _CF_ENDIAN_FLIP_U2( info.catch_beg_pc );
			m++;
		}
		cnt[0] = (cf_count_t)m;  // cnt[0] is exception table length

		if( (*exptab_new_len = _CF_ENDIAN_FLIP_COUNT( m )) )
			memcpy( exptab_new, exptab_ptr, (m = sizeof(CF_ExpInfo)*exptab_len) );


		// remained contents, after the attribute "Code", and attribute table of "Code"

		m += _OFFSET(CF_Attr_Code, exp_tab) + srclen[0];  // m stays at the attr_count of the "Code" of original method

		do{
			// transform "StackMapTable"
			ssize_t _ln = _CF_ENDIAN_FLIP_U4( stub_code->code_len ) + _OFFSET(CF_Attr_Code, code);  // _ln stays at the exception_count of the "Code"

			// seek the attribute table of the "Code" of original method
			CF_AttrInfo *code_attr = (CF_AttrInfo*)((void*)method_code + m + sizeof(cf_count_t));
			cf_count_t code_attr_len = *((cf_count_t*)code_attr - 1);
			_CF_ENDIAN_SWAP_COUNT( code_attr_len );

			// seek the exception table of the "Code" of stub
			CF_ExpInfo *stub_exptab = (CF_ExpInfo*)((void*)stub_code + _ln + sizeof(cf_count_t));
			cf_count_t stub_exptab_n = *((cf_count_t*)stub_exptab - 1);  // it is three, be guaranteed by the cf_verify_stub_method
			_CF_ENDIAN_SWAP_COUNT( stub_exptab_n );

			// seek the attribute table of the "Code" of stub
			CF_AttrInfo *code_attr2 = (CF_AttrInfo*)((cf_count_t*)&stub_exptab[stub_exptab_n] + 1);
			cf_count_t code_attr_len2 = *((cf_count_t*)code_attr2 - 1);
			_CF_ENDIAN_SWAP_COUNT( code_attr_len2 );

			// seek "StackMapTable"
			CF_Attr_StackMapTable *smt[3];
			smt[1] = (CF_Attr_StackMapTable*)cf_seek_attrinfo( code_attr, code_attr_len, "StackMapTable", sizeof("StackMapTable")-1, 0, cfmap );
			smt[2] = (CF_Attr_StackMapTable*)cf_seek_attrinfo( code_attr2, code_attr_len2, "StackMapTable", sizeof("StackMapTable")-1, 0, stub_cfmap );

			// clone "StackMapTable" of stub
			_ln = smt[2] ? _CF_ENDIAN_FLIP_U4(smt[2]->attr_len) + sizeof(CF_AttrHead) : 0;
			smt[0] = _ln ? (CF_Attr_StackMapTable*)malloc( _ln ) : NULL;
			if( smt[0] )
			{
				memcpy( smt[0], smt[2], _ln );

				int smf_num, *smf_lens;
				CF_Attr_StackMapFrame_Union** smf_ptr;

				smf_ptr = cf_build_stackmaptab( (const CF_Attr_StackMapTable**)smt, 2, &smf_num, &smf_lens );
				if( smf_ptr )
				{
					// stub's SMFs before the original's

					int A, B;
					void *ext_buf[2] = {NULL};

					for( A=0; A < smf_num && smf_ptr[A]; A++ );
					B = smf_num - A - 2;
					cf_alter_stackmaptab( smf_ptr, A, smf_lens, NULL, 0, 0, cfmap->cptab[0].cp_count-1, &ext_buf[0], NULL );
					cf_alter_stackmaptab( &smf_ptr[A+1], B, &smf_lens[A+1], info.ins_locs, info.ins_locs_num, 0, 0, &ext_buf[1], NULL );

					CF_Attr_StackMapFrame_Union** smf_dst = malloc( (sizeof(void*)+sizeof(int)) * (smf_num+10) );
					if( smf_dst )
					{
						void *_extbuf_ = NULL;
						int *smf_dst_lens = (int*)&smf_dst[smf_num+4];

						if( strcmp(method_name, "<init>") == 0 )
							B = cf_blend_stkmaptab_v2( smf_ptr, smf_lens, smf_num, smf_dst, smf_dst_lens,
													   &_extbuf_, stub_exptab, attr_code_new->code.byte, &info );
						else
							B = cf_blend_stkmaptab( smf_ptr, smf_lens, smf_num, smf_dst, smf_dst_lens,
													&_extbuf_, stub_exptab, attr_code_new->code.byte, &info );
						if( B > 0 )
						{
							ssize_t ttlen = 0;
							for( A=0; A < B; A++ )
								ttlen += smf_dst_lens[A];

							if( smt[1] )
								_ln = ttlen - (_CF_ENDIAN_FLIP_U4( smt[1]->attr_len ) - sizeof(cf_count_t));
							else
								_ln = ttlen + sizeof(CF_Attr_StackMapTable);

							if( _ln > 1024 )
							{
								// reallocate
								void *new_ptr = cf_malloc( cfmap->auxi.use_sysheap, &cfmap->auxi.mmphd, len+_ln );
								if( !new_ptr )
								{
									// failed
									cf_free( cfmap->auxi.use_sysheap, &cfmap->auxi.mmphd, new_method );
									new_method = NULL;
									goto LAB_BEFORE_EXIT;
								}
								memcpy( new_ptr, new_method, len );
								cf_free( cfmap->auxi.use_sysheap, &cfmap->auxi.mmphd, new_method );
								new_method = new_ptr;
							}
							len += _ln;

							// change the length of "Code" of new method
							_ln += _CF_ENDIAN_FLIP_U4(attr_code_new->attr_len);
							attr_code_new->attr_len = _CF_ENDIAN_FLIP_U4( _ln );

							//
							cf_count_t *new_attr_count = (cf_count_t*)&exptab_new[cnt[0]];
							void *ptr_d = &new_attr_count[1];
							const void *ptr_s = (void*)method_code + m + sizeof( cf_count_t );
							*new_attr_count = *((cf_count_t*)ptr_s - 1);

							// assemble new StackMapTable( the first attribute of "Code" )
							CF_Attr_StackMapTable *new_smt = (CF_Attr_StackMapTable*)ptr_d;
							if( !smt[1] )
							{
								// append attribute "StackMapTable"
								A = _CF_ENDIAN_FLIP_COUNT( *new_attr_count ) + 1;
								*new_attr_count = _CF_ENDIAN_FLIP_COUNT( A );
								//
								if( !cf_get_cpitem_utf8( cfmap, "StackMapTable", sizeof("StackMapTable")-1, (cf_idx_t*)&A ) )
									A = _CF_ENDIAN_FLIP_INDEX(smt[2]->attr_nm_idx) + cfmap->cptab[0].cp_count - 1;
								new_smt->attr_nm_idx = _CF_ENDIAN_FLIP_INDEX( A );
							}
							else
								new_smt->attr_nm_idx = smt[1]->attr_nm_idx;
							new_smt->num_entry = _CF_ENDIAN_FLIP_COUNT(B);
							ttlen += sizeof(cf_count_t);
							new_smt->attr_len = _CF_ENDIAN_FLIP_U4(ttlen);
							ptr_d += sizeof(CF_Attr_StackMapTable);
							// copy entries
							for( A=0; A < B; A++ )
							{
								memcpy( ptr_d, (void*)smf_dst[A], smf_dst_lens[A] );
								ptr_d += smf_dst_lens[A];
							}

							// copy remained contents
							if( smt[1] )
							{
								// contents, before the StackMapTable
								_ln = (void*)smt[1] - ptr_s;
								memcpy( ptr_d, ptr_s, _ln );

								// contents, after the StackMapTable
								ptr_d += _ln;
								ptr_s = (void*)smt[1] + sizeof(CF_AttrHead) + _CF_ENDIAN_FLIP_U4( smt[1]->attr_len );
							}
							_ln = method_len - ((ssize_t)ptr_s - (ssize_t)method);
							memcpy( ptr_d, ptr_s, _ln );

							*new_method_len = len;
							m = 0;
						}

						LAB_BEFORE_EXIT:
						if( _extbuf_ )
							free( _extbuf_ );
						free( smf_dst );
					}
					else
					{
						// failed
						cf_free( cfmap->auxi.use_sysheap, &cfmap->auxi.mmphd, new_method );
						new_method = NULL;
					}

					if( ext_buf[0] )
						free( ext_buf[0] );
					if( ext_buf[1] )
						free( ext_buf[1] );
					free( smf_ptr );
					// CAUTION: DO NOT need to free the smf_lens
				}
				/*else
				{
					// failed
					cf_free( cfmap->auxi.use_sysheap, &cfmap->auxi.mmphd, new_method );
					new_method = NULL;
				}*/
				free( smt[0] );
			}
			else if( _ln )
			{
				// failed
				cf_free( cfmap->auxi.use_sysheap, &cfmap->auxi.mmphd, new_method );
				new_method = NULL;
			}
		}while(0);

		if( new_method )
		{
			if( m )
			{
				// without blending of "StackMapTable"
				method_len -= ((ssize_t)method_code - (ssize_t)method) + m;
				memcpy( (void*)&exptab_new[cnt[0]], (void*)method_code+m, method_len );
				*new_method_len = len;
			}

			// to modify other attributes of "Code" (temporary!!!)
			ssize_t ds;

			// LocalVariableTable, and LocalVariableTypeTable
			ds = _cf_clear_attr_variable( (CF_AttrInfo*)((cf_count_t*)&exptab_new[cnt[0]] + 1), /* code attribute */
										  new_method, new_method_len, cfmap );

			// LineNumberTable
			ds += _cf_clear_attr_linenumber( (CF_AttrInfo*)((cf_count_t*)&exptab_new[cnt[0]] + 1),
											  new_method, new_method_len, cfmap );

			if( ds > 0 )
			{
				ds = _CF_ENDIAN_FLIP_U4( attr_code_new->attr_len ) - ds;
				attr_code_new->attr_len = _CF_ENDIAN_FLIP_U4( ds );
			}

			// seek "RuntimeVisibleTypeAnnotations"
			// seek "RuntimeInvisibleTypeAnnotations"
		}
	}

	free( mtd_sign_tab );
	free( dstbuf );
	if( info.ins_locs )
		free( info.ins_locs );
	//if( info.ins_locs2 )
	//	free( info.ins_locs2 );

	return new_method;
}



/*
// Function:
static int _count_cpitems( const char *methods_name[], const char *methods_desc[], int methods_num,
						   const char *stubs_name[], const char *stubs_desc[], char *flags, cf_idx_t *idx_map,
						   CF_ClassFileMap *cfmap, const CF_ClassFileMap *stub_cfmap )
{
	int i, n;
	cf_idx_t mtd_idx, stub_idx;
	ssize_t mtd_len, stub_len;
	const CF_MethodInfo *mtd, *stub;

	for( i=n=0; i < methods_num; i++ )
	{
		if( (flags[i] & 1) == 0 )
		{
			mtd = cf_get_method2( cfmap, methods_name[i], methods_desc[i], &mtd_idx, &mtd_len );
			if( !mtd )
				continue;
			stub = cf_get_method2( stub_cfmap, stubs_name[i], stubs_desc[i], &stub_idx, &stub_len );
			if( !stub )
				continue;
			// TODO:
		}
	}

	return n;
}
*/


// Function: transform methods specified by the methods_name and the methods_desc;
//           and, combines both constants pool
int cf_transform_class( const char *methods_name[], const char *methods_desc[], int methods_num,
						const char *stubs_name[], const char *stubs_desc[], char *flags, long *tpix,
						const char *klass_name, char *mtd_mask,
						CF_ClassFileMap *cfmap, const CF_ClassFileMap *stub_cfmap )
{
	// check constant pool length
	int i, n, m, t;
	void **ptr;

	if( cfmap->cptab[0].cp_count + stub_cfmap->cptab[0].cp_count + (2 * methods_num * !!tpix) > 0xFFFF )
		return -1;

	cf_count_t mdtab_idx = CF_MAP_ORGTAB_IDX( mdtab, cfmap );
	if( !cfmap->mdtab[mdtab_idx].methods )
	{
		// allocate method table for backup
		m = cfmap->mdtab[0].method_count;
		ptr = cf_malloc( cfmap->auxi.use_sysheap, &cfmap->auxi.mmphd, sizeof(void*)*m*2 );
		if( !ptr )
			return -1;
		memset( ptr, 0, sizeof(void*)*m*2 );
		cfmap->mdtab[mdtab_idx].methods = (CF_MethodInfo**)ptr;
		cfmap->mdtab[mdtab_idx].methods_len = (ssize_t*)&ptr[m];
		//cfmap->mdtab[mdtab_idx].method_count = m;
	}

	// seek the attribute "BootstrapMethods" of the target class file
	cf_idx_t attr_btsmtd_idx;
	CF_Attr_BootstrapMethod *attr_btsmtd = (CF_Attr_BootstrapMethod*)cf_match_attrinfo(
					(const CF_AttrInfo**)cfmap->attrtab[0].attr, cfmap->attrtab[0].attr_count,
					"BootstrapMethods", sizeof("BootstrapMethods")-1, 0, cfmap, &attr_btsmtd_idx );

	//
	cf_count_t cptab_idx = CF_MAP_ORGTAB_IDX( cptab, cfmap );

	// clone constants pool of stub
	if( cf_clone_constants_pool( &stub_cfmap->cptab[0], &cfmap->cptab[cptab_idx], cfmap ) == -1 )
		return -1;

	// combines both constants pool
	n = cfmap->cptab[0].cp_count + cfmap->cptab[cptab_idx].cp_count - 1;  // exclude the cps[0]
	ptr = cf_malloc( cfmap->auxi.use_sysheap, &cfmap->auxi.mmphd, sizeof(CP_Info**) * (n + ((2 + 2 * !!tpix) * methods_num)) + 1 );
	if( !ptr )
		return -1;

	// for TPIX(2023-02-25)
	CP_Long *cp_tpix = NULL;
	if( tpix )
	{
		cp_tpix = cf_malloc( cfmap->auxi.use_sysheap, &cfmap->auxi.mmphd, sizeof(CP_Long)*methods_num );
		if( !cp_tpix )
			return -1;
	}

	// re-new "this class" of stub_code
	CP_Utf8 *cp_utf8 = NULL;
	i = ((CP_Class*)cfmap->cptab[cptab_idx].cps[stub_cfmap->this_class])->name_idx;
	t = ((CP_Class*)cfmap->cptab[0].cps[cfmap->this_class])->name_idx;
	if( cfmap->cptab[cptab_idx].cps[i]->tag == JVM_CONSTANT_Utf8 && cfmap->cptab[0].cps[t]->tag == JVM_CONSTANT_Utf8 )
	{
		m = ((CP_Utf8*)cfmap->cptab[0].cps[t])->len + sizeof(CP_Utf8);
		cp_utf8 = cf_malloc( cfmap->auxi.use_sysheap, &cfmap->auxi.mmphd, m );
		if( cp_utf8 )
		{
			memcpy( cp_utf8, cfmap->cptab[0].cps[t],  m );
			m -= sizeof(CP_Utf8) + ((CP_Utf8*)cfmap->cptab[cptab_idx].cps[i])->len;
			cfmap->cptab[cptab_idx].cps[i] = (CP_Info*)cp_utf8;
			cfmap->cptab[cptab_idx].cps_size += m;
		}
	}

	//
	i = cfmap->cptab[0].cp_count;

	// alter constants index
	cf_alter_cp_index( &cfmap->cptab[cptab_idx].cps[1], n-i, i-1,
					   (attr_btsmtd ? _CF_ENDIAN_FLIP_COUNT(attr_btsmtd->num_methods) : 0) );

	// re-mapping "this class" of stub_code if possible
	if( !cp_utf8 )
		((CP_Class*)cfmap->cptab[cptab_idx].cps[stub_cfmap->this_class])->name_idx = ((CP_Class*)cfmap->cptab[0].cps[cfmap->this_class])->name_idx;

	//
	memcpy( ptr, cfmap->cptab[0].cps, sizeof(CP_Info*) * i );
	memcpy( &ptr[i], &cfmap->cptab[cptab_idx].cps[1], sizeof(CP_Info*) * (n-i) );

	// delete old cp table be cloned from stub map
	cf_free( cfmap->auxi.use_sysheap, &cfmap->auxi.mmphd, cfmap->cptab[cptab_idx].cps );

	// update cptab[cptab_idx]
	cfmap->cptab[cptab_idx].cps = (CP_Info**)ptr;
	cfmap->cptab[cptab_idx].cp_count = n;
	cfmap->cptab[cptab_idx].cps_size += cfmap->cptab[0].cps_size;

	//
	cf_idx_t mtd_idx, stub_idx;
	ssize_t mtd_len, stub_len, new_mtd_len;
	const CF_MethodInfo *mtd, *stub;
	CF_MethodInfo *new_mtd;

	const CP_Utf8 *desc_full, *stub_desc_full;
	size_t _mtd_desc_sz = 0;
	char *_mtd_desc = NULL;
	juint8 _ret_typ_[2];
	int tpix_slot;
	CF_SignExt retval_sign;
	CP_Class *cp_class;
	int k, nc = 0;
	ssize_t add_len = 0;
	int sum = 0;

	//n = cfmap->cptab[cptab_idx].cp_count;
	//ptr = cfmap->cptab[cptab_idx].cps;
	m = n;
	for( i=t=0; i < methods_num; i++ )
	{
		if( (flags[i] & 1) == 0 )
		{
			desc_full = NULL;
			mtd = cf_get_field_method_by_name( 2, cfmap, methods_name[i], methods_desc[i], &mtd_idx, &mtd_len, &desc_full );
			if( !mtd || mtd->access.ABSTRACT || (mtd_mask && mtd_mask[mtd_idx]=='1') )
				continue;

			stub = cf_get_field_method_by_name( 2, stub_cfmap, stubs_name[i], stubs_desc[i], &stub_idx, &stub_len, &stub_desc_full );
			if( !stub )
				continue;

			if( mtd->access.STATIC && !stub->access.STATIC )
			{
				_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR,
						"JVAS_TRANSFORM_CLASS: Unmatched method access flag 'STATIC' between the origin \"%s%s\" and the stub \"%s%s\""
						"; In the class \"%s\"\n",
						methods_name[i], methods_desc[i], stubs_name[i], stubs_desc[i], klass_name );
				continue;
			}

			if( desc_full )
			{
				if( _mtd_desc_sz <= strlen(methods_desc[i]) )
				{
					if( _mtd_desc )
						free( _mtd_desc );

					_mtd_desc_sz = ALIGN_SIZE( _mtd_desc_sz+desc_full->len, 1024 );
					_mtd_desc = malloc( _mtd_desc_sz );
					if( !_mtd_desc )
						break;
				}
				memcpy( _mtd_desc, desc_full->b, desc_full->len );
				_mtd_desc[desc_full->len] = 0;
				cf_get_return_type( _mtd_desc, &_ret_typ_[0] );
			}
			else
			{
				cf_get_return_type( methods_name[i], &_ret_typ_[0] );
			}

			// to check match of return type
			if( stub_desc_full )
				cf_get_return_type2( stub_desc_full->b, stub_desc_full->len, &_ret_typ_[1] );
			else
				cf_get_return_type( stubs_desc[i], &_ret_typ_[1] );

			if( CF_SIGNATURE_SYMBOL[_ret_typ_[0]] == JVM_SIGNATURE_ARRAY )
				_ret_typ_[0] = cf_get_signature_index( JVM_SIGNATURE_CLASS );

			if( CF_SIGNATURE_SYMBOL[_ret_typ_[1]] == JVM_SIGNATURE_ARRAY )
				_ret_typ_[1] = cf_get_signature_index( JVM_SIGNATURE_CLASS );

			if( _ret_typ_[0] != _ret_typ_[1] )
			{
				_DEBUG_PRINT_( stdout, SABI_JOURNAL_WARN,
							   "JVAS_TRANSFORM_CLASS: Unmatched return type between the real method \"%s%s\" and the stub \"%s%s\""
							   "; In the class \"%s\"\n",
								methods_name[i], (desc_full ? _mtd_desc : methods_desc[i]),
								stubs_name[i], stubs_desc[i], klass_name );
			}

			//
			//memset( &retval_sign, 0, sizeof(retval_sign) );
			retval_sign.sym = NULL;
			tpix_slot = cp_tpix && (flags[i]&0x08) ? n : -1;
			retval_sign.cls_idx = n + ((tpix_slot != -1) << 1);

			new_mtd = cf_transform_method( mtd, mtd_len, cfmap, stub, stub_len, stub_cfmap, &new_mtd_len,
										   methods_name[i], (desc_full ? _mtd_desc : methods_desc[i]),
										   (flags[i]>>4)&7,
										   tpix_slot,
										   &retval_sign );
			flags[i] |= 0x80;

			if( new_mtd )
			{
				// change method table
				cfmap->mdtab[mdtab_idx].methods[mtd_idx] = (CF_MethodInfo*)mtd;
				cfmap->mdtab[mdtab_idx].methods_len[mtd_idx] = mtd_len;
				cfmap->mdtab[0].methods[mtd_idx] = new_mtd;
				cfmap->mdtab[0].methods_len[mtd_idx] = new_mtd_len;
				//flags[i] |= 0x80;
				if( mtd_mask )
					mtd_mask[mtd_idx]++;// = '1';


				if( cp_tpix )
				{
					// setup TPIX(2023-02-25)
					cp_tpix[t].tag = JVM_CONSTANT_Long;
					cp_tpix[t].i64 = tpix[i];  // keep as local byte order
					ptr[n++] = &cp_tpix[t++];
					ptr[n++] = NULL;
				}

				if( retval_sign.sym && retval_sign.len > 0 )
				{
					// new class of return value
					// prevent duplicate
					for( k=m; k < n; k++ )
					{
						if( ptr[k] && ((CP_Info*)ptr[k])->tag == JVM_CONSTANT_Class )
						{
							k = ((CP_Class*)ptr[k])->name_idx;
							if( ((CP_Utf8*)ptr[k])->len == retval_sign.len &&
								strncmp( ((CP_Utf8*)ptr[k])->b, retval_sign.sym, retval_sign.len ) == 0 )
							{
								break;
							}
						}
					}

					if( k >= n )
					{
						// try to append it
						k = sizeof(CP_Class) + sizeof(CP_Utf8) + retval_sign.len;
						cp_class = (CP_Class*)cf_malloc( cfmap->auxi.use_sysheap, &cfmap->auxi.mmphd, k+1 );
						if( !cp_class )
							break;
						cp_utf8 = (CP_Utf8*)&cp_class[1];
						//
						cp_class->tag = JVM_CONSTANT_Class;
						cp_class->name_idx = n + 1;
						ptr[n++] = cp_class;
						//
						cp_utf8->tag = JVM_CONSTANT_Utf8;
						if( retval_sign.sym[0] == 'L' )
						{
							// object reference
							cp_utf8->len = retval_sign.len - 2;
							memcpy( cp_utf8->b, retval_sign.sym+1, retval_sign.len-2 ); // besides 'L' and ';'
							cp_utf8->b[retval_sign.len-2] = 0;
							add_len += k - 2;
						}
						else
						{
							// maybe an array reference or others(full string)
							cp_utf8->len = retval_sign.len;
							memcpy( cp_utf8->b, retval_sign.sym, retval_sign.len );
							cp_utf8->b[retval_sign.len] = 0;
							add_len += k;
						}
						ptr[n++] = cp_utf8;
						nc++;
					}
				}
				sum++;
			}
		}
	}

	if( sum > 0 )
	{
		// exchange constants pool
		struct _cf_cptab_item _tmp = cfmap->cptab[cptab_idx];
		cfmap->cptab[cptab_idx] = cfmap->cptab[0];
		cfmap->cptab[0] = _tmp;
		cfmap->cptab[0].cp_count = n; // += (t + nc) * 2;
		cfmap->cptab[0].cps_size += t * sizeof(CP_Long) + add_len;

		if( _mtd_desc )
			free( _mtd_desc );

		// merge critical attribute "BootstrapMethods" if possible
		CF_Attr_BootstrapMethod *stub_attr_btsmtd = (CF_Attr_BootstrapMethod*)cf_match_attrinfo(
									(const CF_AttrInfo**)stub_cfmap->attrtab[0].attr, stub_cfmap->attrtab[0].attr_count,
									"BootstrapMethods", sizeof("BootstrapMethods")-1, 0, stub_cfmap, NULL );
		if( stub_attr_btsmtd )
		{
			if( !_cf_merge_bootstrap_methods( attr_btsmtd_idx, attr_btsmtd, stub_attr_btsmtd, cfmap->cptab[cptab_idx].cp_count-1, cfmap ) )
				return -1;
		}
	}

	return sum;
}

// Function: add a new field/method


// Function: delete a exists field/method


// Function: rename a exists field/method


// Function: add an attribute to field/method


// Function: delete an attribute from field/method



