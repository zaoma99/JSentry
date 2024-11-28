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


//
int cf_print_access( cf_acc_t acc, FILE *fout )
{
	return
	fprintf( fout,
			"PUBLIC:%d, PRIVATE:%d, PROTECTED:%d, STATIC:%d, FINAL:%d, SYNCHRONIZED/SUPER:%d, VOLATILE/BRIDGE:%d, VARARGS/TRANSIENT:%d,"
			"NATIVE:%d, INTERFACE:%d, ABSTRACT:%d, STRICT:%d, SYNTHETIC:%d, ANNOTATION:%d, ENUM:%d, MODULE:%d",
			acc.PUBLIC, acc.PRIVATE, acc.PROTECTED, acc.STATIC, acc.FINAL, acc.SYNCH, acc.VOLATILE, acc.VARARGS,
			acc.NATIVE, acc.INTERFACE, acc.ABSTRACT, acc.STRICT, acc.SYNTHETIC, acc.ANNO, acc.ENUM, acc.MODULE
			);
}


//
int cf_print_field_method_info( int which, const CF_FieldInfo **field, cf_count_t count, const CF_ClassFileMap *cf_map, FILE *fout )
{
	CP_InfoConstPtr cp0, cp1;
	cf_acc_t acc;
	char name[256], desc[1024];
	cf_count_t c;

	char field_method_name[][12] = { "", "Field", "Method" };

	for( c=0; c < count; c++ )
	{
		acc.flags = field[c]->access.flags;
		cp0.Info = cf_get_cpitem( cf_map, field[c]->name_idx );
		cp1.Info = cf_get_cpitem( cf_map, field[c]->desc_idx );

		name[0] = 0;
		cf_copy_utf8( cp0.Utf8, name, sizeof(name) );
		desc[0] = 0;
		cf_copy_utf8( cp1.Utf8, desc, sizeof(desc) );

		fprintf( fout, "%s:{access:{"
				"PUBLIC:%d, PRIVATE:%d, PROTECTED:%d, STATIC:%d, FINAL:%d, SYNCHRONIZED/SUPER:%d, VOLATILE/BRIDGE:%d, VARARGS/TRANSIENT:%d,"
				"NATIVE:%d, INTERFACE:%d, ABSTRACT:%d, STRICT:%d, SYNTHETIC:%d, ANNOTATION:%d, ENUM:%d, MODULE:%d},"
				"name_index:%d, name:%s, desc_index:%d, desc:%s, attr_count:%d, attr_info:[",
				field_method_name[which],
				acc.PUBLIC, acc.PRIVATE, acc.PROTECTED, acc.STATIC, acc.FINAL, acc.SYNCH, acc.VOLATILE, acc.VARARGS,
				acc.NATIVE, acc.INTERFACE, acc.ABSTRACT, acc.STRICT, acc.SYNTHETIC, acc.ANNO, acc.ENUM, acc.MODULE,
				field[c]->name_idx, name, field[c]->desc_idx, desc, field[c]->attr_count );

		cf_print_attrinfo( field[c]->attr.ite, field[c]->attr_count, cf_map, fout );

		fprintf( fout, "]}\n" );
	}
	return 0;
}

int cf_print_fieldinfo( const CF_FieldInfo **field, cf_count_t count, const CF_ClassFileMap *cf_map, FILE *fout )
{
	return cf_print_field_method_info( 1, field, count, cf_map, fout );
}

int cf_print_methodinfo( const CF_MethodInfo **method, cf_count_t count, const CF_ClassFileMap *cf_map, FILE *fout )
{
	return cf_print_field_method_info( 2, (const CF_FieldInfo**)method, count, cf_map, fout );
}


//
int cf_print_interface( const cf_idx_t *if_idx, cf_count_t count, const CF_ClassFileMap *cf_map, FILE *fout )
{
	CP_InfoConstPtr cp, cp1;
	cf_count_t c;
	char name[512];


	for( c=0; c < count; c++ )
	{
		if( !(cp.Info = cf_get_cpitem(cf_map, if_idx[c])) || cp.Class->tag != JVM_CONSTANT_Class)
			continue;

		if( !(cp1.Info = cf_get_cpitem( cf_map, cp.Class->name_idx )) || cp1.Utf8->tag != JVM_CONSTANT_Utf8 )
			continue;

		name[0] = 0;
		cf_copy_utf8( cp1.Utf8, name, sizeof(name) );

		fprintf( fout, "Class:{index:%d, name:%s}\n", cp.Class->name_idx, name );
	}
	return 0;
}


//
void cf_translate_method( const CF_MethodInfo *method, const CF_ClassFileMap *cf_map, FILE *fout )
{
	const char *sym_static[] = { "static ", "" };
	const char *sym_final[] = { "final ", "" };
	//const char *sym_abstract[] = { "abstract ", "" };
	const char *sym_interface[] = { "interface ", "" };
	const char *sym_ppp[] = { "", "public ", "private ", "", "protected " };

	const CP_InfoUnion **cps = (const CP_InfoUnion**)cf_map->cptab[0].cps;
	const CF_AttrInfo *attr;
	cf_count_t i, j, idx;
	char str[1024];

	// get attribute "CODE"
	attr = method->attr.ite;
	for( j=0; j < method->attr_count; j++ )
	{
		idx = _CF_ENDIAN_FLIP_INDEX( attr->attr_nm_idx );
		if( idx &&
			cps[idx]->Utf8.tag == JVM_CONSTANT_Utf8 &&
			cps[idx]->Utf8.len == 4 &&
			strncmp(cps[idx]->Utf8.b, CF_ATTR_NAME[CF_ATTR_Code], 4) == 0
			)
		{
			juint32 code_len = ((CF_Attr_Code*)attr)->code_len;
			_CF_ENDIAN_SWAP_U4( code_len );

			// show name
			str[0] = 0;
			cf_copy_utf8( &cps[method->name_idx]->Utf8, str, sizeof(str) );
			fprintf( fout, "%s%s%s%s%s",
					 sym_ppp[method->access.flags&7],
					 sym_final[!method->access.FINAL],
					 sym_static[!method->access.STATIC],
					 sym_interface[!method->access.INTERFACE],
					 str );
			// show descriptor
			str[0] = 0;
			cf_copy_utf8( &cps[method->desc_idx]->Utf8, str, sizeof(str) );
			fprintf( fout, "%s\n Code:\n", str );

			// exception table
			CF_ExpInfo *exptab_ptr = (CF_ExpInfo*)(((CF_Attr_Code*)attr)->code.byte + code_len + sizeof(cf_count_t));
			cf_count_t exptab_len = *((cf_count_t*)exptab_ptr - 1);
			_CF_ENDIAN_SWAP_COUNT( exptab_len );

			// show instructions
			cf_bytecode_translate( ((CF_Attr_Code*)attr)->code.byte, (int)code_len, cf_map, fout );

			// show exception table
			if( exptab_len )
			{
				fprintf( fout, " Exception Table:\n        From       To    Handler    Type\n" );

				for( i=0; i < exptab_len; i++ )
				{
					str[0] = 0;
					idx = _CF_ENDIAN_FLIP_INDEX( exptab_ptr[i].catch_type );
					if( idx )
					{
						idx = cps[idx]->Class.name_idx;
						cf_copy_utf8( &cps[idx]->Utf8, str, sizeof(str) );
					}

					fprintf( fout, "    %8d %8d   %8d    Class %s\n",
							_CF_ENDIAN_FLIP_U2(exptab_ptr[i].start_pc),
							_CF_ENDIAN_FLIP_U2(exptab_ptr[i].end_pc),
							_CF_ENDIAN_FLIP_U2(exptab_ptr[i].handler_pc),
							str[0] ? str : "any" );
				}
			}

			break;
		}
		attr = (const CF_AttrInfo*)((void*)attr + attr->attr_len + sizeof(CF_AttrHead));
	}
}


//
void cf_translate_methodEx( const char *method_name, const CF_ClassFileMap *cf_map, FILE *fout )
{
	CP_InfoUnion **cps = (CP_InfoUnion**)cf_map->cptab[0].cps;
	CF_MethodInfo **methods = cf_map->mdtab[0].methods;
	int name_len = method_name ? strlen(method_name) : 0;

	cf_count_t i, idx;
	for( i=0; i < cf_map->mdtab[0].method_count; i++ )
	{
		if( methods[i]->access.NATIVE )
			continue;

		if( name_len > 0 )
		{
			idx = methods[i]->name_idx;
			if( cps[idx]->Utf8.tag != JVM_CONSTANT_Utf8 ||
				cps[idx]->Utf8.len != name_len ||
				strncmp(cps[idx]->Utf8.b, method_name, name_len) != 0
				)
				continue;  // not matched
		}

		fprintf( fout, "\nMethod No.%d:\n", i );
		cf_translate_method( methods[i], cf_map, fout );
	}
}
