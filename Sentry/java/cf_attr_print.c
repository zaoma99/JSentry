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


// default
static int _print_attr_Default( JVM_ATTR_TYPE type, const CF_AttrInfo *attr, const CF_ClassFileMap *cf_map, int local_endian, FILE *fout )
{
	fprintf( fout, "Attr:{name:%s, len:%d}\n", CF_ATTR_NAME[type], _CF_ENDIAN_FLIP_U4(attr->attr_len) );
	return 0;
}


// JVM_ATTR_ConstantValue
static int _print_attr_ConstantValue( JVM_ATTR_TYPE type, const CF_Attr_Constant *attr, const CF_ClassFileMap *cf_map, int local_endian, FILE *fout )
{
	CP_InfoConstPtr cpi;
	cf_idx_t idx = _CF_ENDIAN_FLIP_INDEX( attr->val_idx );

	cpi.Info = cf_get_cpitem( cf_map, idx );
	fprintf( fout, "Attr:{name:%s, len:%d, const_idx:%d, const_value:{",
			CF_ATTR_NAME[JVM_ATTR_ConstantValue], _CF_ENDIAN_FLIP_U4(attr->attr_len), idx );
	cf_print_cpinfo( cpi.Info, local_endian, fout );
	fprintf( fout, "}}\n" );
	return 0;
}


// JVM_ATTR_Code
static int _print_attr_Code( JVM_ATTR_TYPE type, const CF_Attr_Code *attr, const CF_ClassFileMap *cf_map, int local_endian, FILE *fout )
{
	juint32 code_len = _CF_ENDIAN_FLIP_U4( attr->code_len );

	CF_ExpInfo *exptab_ptr = (CF_ExpInfo*)(attr->code.byte + code_len + sizeof(cf_count_t));
	cf_count_t exptab_len = *((cf_count_t*)exptab_ptr - 1);
	_CF_ENDIAN_SWAP_COUNT( exptab_len );

	CF_AttrInfo *attrtab_ptr = (CF_AttrInfo*)((cf_count_t*)&exptab_ptr[exptab_len] + 1);
	cf_count_t attrtab_len = *((cf_count_t*)attrtab_ptr -1);
	_CF_ENDIAN_SWAP_COUNT( attrtab_len );

	fprintf( fout, "Attr:{name:%s, len:%d, max_stack:%d, max_locals:%d, code_len:%u, code:[], exptab_len:%d, exptab:[",
			CF_ATTR_NAME[JVM_ATTR_Code], _CF_ENDIAN_FLIP_U4(attr->attr_len), _CF_ENDIAN_FLIP_INDEX(attr->max_stack),
			_CF_ENDIAN_FLIP_INDEX(attr->max_locals), code_len, exptab_len );

	char spc[][4] = {", ", ""};
	cf_count_t i;
	for( i=0; i < exptab_len; i++ )
		fprintf( fout, "{start_pc:%d, end_pc:%d, handler_pc:%d, catch_type:%d}%s",
				_CF_ENDIAN_FLIP_U2(exptab_ptr[i].start_pc), _CF_ENDIAN_FLIP_U2(exptab_ptr[i].end_pc),
				_CF_ENDIAN_FLIP_U2(exptab_ptr[i].handler_pc), _CF_ENDIAN_FLIP_INDEX(exptab_ptr[i].catch_type),
				 spc[i+1==exptab_len] );

	fprintf( fout, "], attr_count:%d, attr:[", attrtab_len );
	if( attrtab_len )
		cf_print_attrinfo( attrtab_ptr, attrtab_len, cf_map, fout );  // recursion

	fprintf( fout, "]}\n" );
	return 0;
}



// JVM_ATTR_StackMapTable
static int _print_attr_StackMapTable( JVM_ATTR_TYPE type, const CF_Attr_StackMapTable *attr, const CF_ClassFileMap *cf_map, int local_endian, FILE *fout )
{
	char spc[][4] = {", ", ""};
	juint8 *byte = (juint8*)&attr->entry;
	jint32 i, n = _CF_ENDIAN_FLIP_U4(attr->attr_len);

	fprintf( fout, "Attr:{name:%s, len:%d, entries_number:%d, entries:[",
			CF_ATTR_NAME[JVM_ATTR_StackMapTable], n, _CF_ENDIAN_FLIP_COUNT(attr->num_entry) );

	n -= sizeof(cf_count_t);
	for( i=0; i < n; i++ )
		fprintf( fout, "%02x%s", byte[i], spc[i+1==n] );

	fprintf( fout, "]}\n" );
	return 0;
}


// JVM_ATTR_Exceptions
static int _print_attr_Exception( JVM_ATTR_TYPE type, const CF_Attr_Exception *attr, const CF_ClassFileMap *cf_map, int local_endian, FILE *fout )
{
	cf_count_t i, n = _CF_ENDIAN_FLIP_COUNT( attr->exp_tab_len );
	char spc[][4] = {", ", ""};

	fprintf( fout, "Attr:{name:%s, len:%d, exptab_len:%d, exptab:[",
			CF_ATTR_NAME[JVM_ATTR_Exceptions], _CF_ENDIAN_FLIP_U4(attr->attr_len), n );

	for( i=0; i < n; i++ )
		fprintf( fout, "%d%s", _CF_ENDIAN_FLIP_U2(attr->exp_tab.ite[i]), spc[i+1==n] );

	fprintf( fout, "]}\n" );
	return 0;
}


// JVM_ATTR_InnerClasses
static int _print_attr_InnerClasses( JVM_ATTR_TYPE type, const CF_Attr_InnerClasses *attr, const CF_ClassFileMap *cf_map, int local_endian, FILE *fout )
{
	cf_count_t i, n = _CF_ENDIAN_FLIP_COUNT( attr->class_num );
	char spc[][4] = {", ", ""};

	fprintf( fout, "Attr:{name:%s, len:%d, num_classes:%d, classes:[",
			CF_ATTR_NAME[JVM_ATTR_InnerClasses], _CF_ENDIAN_FLIP_U4(attr->attr_len), n );

	for( i=0; i < n; i++ )
	{
		fprintf( fout, "{inner_class_idx:%d, outer_class_idx:%d, inner_name_idx:%d, inner_class_acc:%04x}%s",
				_CF_ENDIAN_FLIP_INDEX(attr->cls[i].inner_class_idx), _CF_ENDIAN_FLIP_INDEX(attr->cls[i].outer_class_idx),
				_CF_ENDIAN_FLIP_INDEX(attr->cls[i].inner_name_idx), _CF_ENDIAN_FLIP_INDEX(attr->cls[i].inner_class_access.flags),
				spc[i+1==n] );
	}
	fprintf( fout, "]}\n" );
	return 0;
}


// JVM_ATTR_EnclosingMethod
static int _print_attr_EnclosingMethod( JVM_ATTR_TYPE type, const CF_Attr_EnclosingMethod *attr, const CF_ClassFileMap *cf_map, int local_endian, FILE *fout )
{
	fprintf( fout, "Attr:{name:%s, len:%d, class_idx:%d, method_idx:%d}\n",
			 CF_ATTR_NAME[JVM_ATTR_EnclosingMethod], _CF_ENDIAN_FLIP_U4(attr->attr_len),
			 _CF_ENDIAN_FLIP_INDEX( attr->class_idx ), _CF_ENDIAN_FLIP_INDEX( attr->method_idx ) );
	return 0;
}


// JVM_ATTR_Signature
static int _print_attr_Signature( JVM_ATTR_TYPE type, const CF_Attr_Signature *attr, const CF_ClassFileMap *cf_map, int local_endian, FILE *fout )
{
	CP_InfoConstPtr cpi;
	cf_idx_t idx = _CF_ENDIAN_FLIP_INDEX( attr->signature_idx );

	cpi.Info = cf_get_cpitem( cf_map, idx );
	fprintf( fout, "Attr:{name:%s, len:%d, signature_idx:%d, signature:{",
			CF_ATTR_NAME[JVM_ATTR_Signature], _CF_ENDIAN_FLIP_U4(attr->attr_len), idx );
	cf_print_cpinfo( cpi.Info, local_endian, fout );
	fprintf( fout, "}}\n" );
	return 0;
}


// JVM_ATTR_SourceFile
static int _print_attr_SourceFile( JVM_ATTR_TYPE type, const CF_Attr_SourceFile *attr, const CF_ClassFileMap *cf_map, int local_endian, FILE *fout )
{
	CP_InfoConstPtr cpi;
	cf_idx_t idx = _CF_ENDIAN_FLIP_INDEX( attr->srcfile_idx );

	cpi.Info = cf_get_cpitem( cf_map, idx );
	fprintf( fout, "Attr:{name:%s, len:%d, srcfile_idx:%d, srcfile:{",
			CF_ATTR_NAME[JVM_ATTR_SourceFile], _CF_ENDIAN_FLIP_U4(attr->attr_len), idx );
	cf_print_cpinfo( cpi.Info, local_endian, fout );
	fprintf( fout, "}}\n" );
	return 0;
}


// JVM_ATTR_LineNumberTable
static int _print_attr_LineNumberTable( JVM_ATTR_TYPE type, const CF_Attr_LineNumberTable *attr, const CF_ClassFileMap *cf_map, int local_endian, FILE *fout )
{
	cf_count_t i, n = _CF_ENDIAN_FLIP_COUNT( attr->lines );
	char spc[][4] = {", ", ""};

	fprintf( fout, "Attr:{name:%s, len:%d, line_number_table_len:%d, line_number_table:[",
			CF_ATTR_NAME[JVM_ATTR_LineNumberTable], _CF_ENDIAN_FLIP_U4(attr->attr_len), n );

	for( i=0; i < n; i++ )
		fprintf( fout, "{start_pc:%d, number:%d}%s",
				_CF_ENDIAN_FLIP_U2(attr->lntab[i].start_pc), _CF_ENDIAN_FLIP_U2(attr->lntab[i].line_num),
				 spc[i+1==n] );

	fprintf( fout, "]}\n" );
	return 0;
}


// JVM_ATTR_LocalVariableTable
static int _print_attr_LocalVariableTable_ex( JVM_ATTR_TYPE type, const CF_Attr_LocalVariableTable *attr, const CF_ClassFileMap *cf_map, int local_endian, FILE *fout )
{
	cf_count_t i, n = _CF_ENDIAN_FLIP_COUNT( attr->locals_len );
	char spc[][4] = {", ", ""};
	char desc_or_type[][12] = { "desc_idx", "type_idx" };

	fprintf( fout, "Attr:{name:%s, len:%d, locals_count:%d, locals:[",
			CF_ATTR_NAME[type], _CF_ENDIAN_FLIP_U4(attr->attr_len), n );

	for( i=0; i < n; i++ )
		fprintf( fout, "{start_pc:%d, length:%d, name_idx:%d, %s:%d, index_of_array:%d}%s",
				_CF_ENDIAN_FLIP_U2(attr->locals[i].start_pc), _CF_ENDIAN_FLIP_U2(attr->locals[i].length),
				_CF_ENDIAN_FLIP_U2(attr->locals[i].name_idx), desc_or_type[type==JVM_ATTR_LocalVariableTypeTable],
				_CF_ENDIAN_FLIP_INDEX(attr->locals[i].desc_idx),
				_CF_ENDIAN_FLIP_U2(attr->locals[i].index), spc[i+1==n] );

	fprintf( fout, "]}\n" );
	return 0;
}

#define _print_attr_LocalVariableTable	_print_attr_LocalVariableTable_ex
#define _print_attr_LocalVariableTypeTable	_print_attr_LocalVariableTable_ex


// JVM_ATTR_BootstrapMethods
static int _print_attr_BootstrapMethod( JVM_ATTR_TYPE type, const CF_Attr_BootstrapMethod *attr, const CF_ClassFileMap *cf_map, int local_endian, FILE *fout )
{
	const CF_BootstrapMethod *ite;
	cf_count_t i, j, m, n = _CF_ENDIAN_FLIP_COUNT( attr->num_methods );
	char spc[][4] = {", ", ""};

	fprintf( fout, "Attr:{name:%s, len:%d, num_bootstrap_methods:%d, bootstrap_methods:[",
			CF_ATTR_NAME[JVM_ATTR_BootstrapMethods], _CF_ENDIAN_FLIP_U4(attr->attr_len), n );

	ite = attr->methods.ite;

	for( i=0; i < n; i++ )
	{
		m = _CF_ENDIAN_FLIP_COUNT( ite->num_arguments );
		fprintf( fout, "{method_ref:%d, num_arguments:%d, arguments:[",
				_CF_ENDIAN_FLIP_INDEX(ite->method_ref), m );

		for( j=0; j < m; j++ )
			fprintf( fout, "%d%s", _CF_ENDIAN_FLIP_INDEX(ite->arguments[j]), spc[j+1==m] );

		fprintf( fout, "]}%s", spc[i+1==n] );

		ite = (CF_BootstrapMethod*)((void*)ite + m*sizeof(cf_idx_t)+sizeof(CF_BootstrapMethod) );
	}

	fprintf( fout, "]}\n" );
	return 0;
}


// JVM_ATTR_MethodParameters
static int _print_attr_MethodParameters( JVM_ATTR_TYPE type, const CF_Attr_MethodParameters *attr, const CF_ClassFileMap *cf_map, int local_endian, FILE *fout )
{
	cf_count_t i, n = attr->param_count;
	char spc[][4] = {", ", ""};

	fprintf( fout, "Attr:{name:%s, len:%d, parameters_count:%d, parameters:[",
			CF_ATTR_NAME[JVM_ATTR_MethodParameters], _CF_ENDIAN_FLIP_U4(attr->attr_len), n );

	for( i=0; i < n; i++ )
		fprintf( fout, "{namde_index:%d, access:%04X}%s",
				_CF_ENDIAN_FLIP_INDEX(attr->param[i].name_idx), _CF_ENDIAN_FLIP_ACCES(attr->param[i].access.flags),
				 spc[i+1==n] );

	fprintf( fout, "]}\n" );
	return 0;
}


// JVM_ATTR_Module
static int _print_attr_Module( JVM_ATTR_TYPE type, const CF_Attr_Module *attr, const CF_ClassFileMap *cf_map, int local_endian, FILE *fout )
{
	cf_count_t i, n = _CF_ENDIAN_FLIP_COUNT( attr->requires_count );
	char spc[][4] = {", ", ""};

	fprintf( fout, "Attr:{name:%s, len:%d, module:{name_idx:%d, flags:%04X, ver_idx:%d}, requires_count:%d, requires:[",
			CF_ATTR_NAME[JVM_ATTR_Module], _CF_ENDIAN_FLIP_U4(attr->attr_len),
			_CF_ENDIAN_FLIP_U4(attr->module_name_idx), _CF_ENDIAN_FLIP_U4(attr->module_flags),
			_CF_ENDIAN_FLIP_U4(attr->module_ver_idx), n );

	// requires
	for( i=0; i < n; i++ )
		fprintf( fout, "{name_idx:%d, flags:%04X, ver_idx:%d}%s",
				_CF_ENDIAN_FLIP_INDEX(attr->requires[i].requires_name_idx),
				_CF_ENDIAN_FLIP_INDEX(attr->requires[i].requires_flags),
				_CF_ENDIAN_FLIP_INDEX(attr->requires[i].requires_ver_idx),
				spc[i+1==n] );


	// exports
	cf_count_t j, m;
	cf_count_t *count_ptr = (cf_count_t*)&attr->requires[n];
	struct _cf_module_export *export_ptr = (struct _cf_module_export*)&count_ptr[1];
	n = *count_ptr;
	_CF_ENDIAN_SWAP_COUNT( n );

	fprintf( fout, "],\n exports_count:%d, exports:[", n );

	for( i=0; i < n; i++ )
	{
		fprintf( fout, "{exports_idx:%d, flags:%04X, exports_to_count:%d, exports_to_index:[",
				_CF_ENDIAN_FLIP_INDEX(export_ptr[i].exports_index),
				_CF_ENDIAN_FLIP_INDEX(export_ptr[i].exports_flags),
				(m = _CF_ENDIAN_FLIP_COUNT(export_ptr[i].exports_to_count)) );

		for( j=0; j < m; j++ )
			fprintf( fout, "%d%s", _CF_ENDIAN_FLIP_INDEX(export_ptr[i].exports_to_index[j]), spc[j+1==n] );
		fprintf( fout, "]}%s", spc[i+1==n] );
	}

	// exports
	count_ptr = (cf_count_t*)&export_ptr[n];
	struct _cf_module_open *open_ptr = (struct _cf_module_open*)&count_ptr[1];
	n = *count_ptr;
	_CF_ENDIAN_SWAP_COUNT( n );

	fprintf( fout, "],\n opens_count:%d, opens:[", n );

	for( i=0; i < n; i++ )
	{
		fprintf( fout, "{opens_idx:%d, flags:%04X, opens_to_count:%d, opens_to_index:[",
				_CF_ENDIAN_FLIP_INDEX(open_ptr[i].opens_index),
				_CF_ENDIAN_FLIP_INDEX(open_ptr[i].opens_flags),
				(m = _CF_ENDIAN_FLIP_COUNT(open_ptr[i].opens_to_count)) );

		for( j=0; j < m; j++ )
			fprintf( fout, "%d%s", _CF_ENDIAN_FLIP_INDEX(open_ptr[i].opens_to_index[j]), spc[j+1==n] );
		fprintf( fout, "]}%s", spc[i+1==n] );
	}


	// uses
	count_ptr = (cf_count_t*)&open_ptr[n];
	cf_idx_t *uses_ptr = &count_ptr[1];
	n = *count_ptr;
	_CF_ENDIAN_SWAP_COUNT( n );

	fprintf( fout, "],\n uses_count:%d, uses:[", n );

	for( i=0; i < n; i++ )
		fprintf( fout, "%d%s", _CF_ENDIAN_FLIP_INDEX(uses_ptr[i]), spc[i+1==n] );

	// provides
	count_ptr = &uses_ptr[n];
	struct _cf_module_provide *provides_ptr = (struct _cf_module_provide*)&count_ptr[1];
	n = *count_ptr;
	_CF_ENDIAN_SWAP_COUNT( n );

	fprintf( fout, "],\n provides_count:%d, provides:[", n );

	for( i=0; i < n; i++ )
	{
		fprintf( fout, "{provides_idx:%d, provides_with_count:%d, provides_with_index:[",
				_CF_ENDIAN_FLIP_INDEX(provides_ptr[i].provides_index),
				(m = _CF_ENDIAN_FLIP_COUNT(provides_ptr[i].provides_with_count)) );

		for( j=0; j < m; j++ )
			fprintf( fout, "%d%s", _CF_ENDIAN_FLIP_INDEX(provides_ptr[i].provides_with_index[j]), spc[j+1==n] );
		fprintf( fout, "]}%s", spc[i+1==n] );
	}

	fprintf( fout, "]}\n" );

	return 0;
}


// JVM_ATTR_ModulePackages
static int _print_attr_ModulePackages( JVM_ATTR_TYPE type, const CF_Attr_ModulePackages *attr, const CF_ClassFileMap *cf_map, int local_endian, FILE *fout )
{
	cf_count_t i, n = _CF_ENDIAN_FLIP_COUNT( attr->package_count );
	CP_InfoConstPtr cpi;
	cf_idx_t idx;
	char spc[][4] = {", ", ""};
	char name[512];

	fprintf( fout, "Attr:{name:%s, len:%d, packages_count:%d, packages:[",
			CF_ATTR_NAME[JVM_ATTR_ModulePackages], _CF_ENDIAN_FLIP_U4(attr->attr_len), n );

	for( i=0; i < n; i++ )
	{
		idx = _CF_ENDIAN_FLIP_INDEX( attr->package_idx[i] );

		name[0] = 0;
		cpi.Info = cf_get_cpitem( cf_map, idx );
		if( cpi.Info && cpi.Package->tag == JVM_CONSTANT_Package )
		{
			if( (cpi.Info = cf_get_cpitem(cf_map, _CF_ENDIAN_FLIP_INDEX(cpi.Package->name_idx))) )
				cf_copy_utf8( cpi.Utf8, name, sizeof(name) );
		}

		fprintf( fout, "{index:%d, name:%s}%s", idx, name, spc[i+1==n] );
	}

	fprintf( fout, "]}\n" );
	return 0;
}


// JVM_ATTR_ModuleMainClass
static int _print_attr_ModuleMainClass( JVM_ATTR_TYPE type, const CF_Attr_ModuleMainClass *attr, const CF_ClassFileMap *cf_map, int local_endian, FILE *fout )
{
	char name[256];
	CP_InfoConstPtr cpi;
	cf_idx_t idx = _CF_ENDIAN_FLIP_INDEX( attr->main_class_idx );

	name[0] = 0;
	cpi.Info = cf_get_cpitem( cf_map, idx );
	if( cpi.Info && cpi.Class->tag == JVM_CONSTANT_Class )
	{
		if( (cpi.Info = cf_get_cpitem(cf_map, _CF_ENDIAN_FLIP_INDEX(cpi.Class->name_idx))) )
			cf_copy_utf8( cpi.Utf8, name, sizeof(name) );
	}

	fprintf( fout, "Attr:{name:%s, len:%d, main_class_idx:%d, main_class_name:%s}\n",
			CF_ATTR_NAME[JVM_ATTR_ModuleMainClass], _CF_ENDIAN_FLIP_U4(attr->attr_len), idx, name );

	return 0;
}


// JVM_ATTR_PermittedSubclasses
static int _print_attr_PermittedSubclasses( JVM_ATTR_TYPE type, const CF_Attr_PermittedSubclasses *attr, const CF_ClassFileMap *cf_map, int local_endian, FILE *fout )
{
	cf_count_t i, n = _CF_ENDIAN_FLIP_COUNT( attr->num_class );
	char spc[][4] = {", ", ""};

	fprintf( fout, "Attr:{name:%s, len:%d, number_classes:%d, classes:[",
			CF_ATTR_NAME[JVM_ATTR_PermittedSubclasses], _CF_ENDIAN_FLIP_U4(attr->attr_len), n );

	for( i=0; i < n; i++ )
		fprintf( fout, "%d%s", _CF_ENDIAN_FLIP_INDEX(attr->cls[i]), spc[i+1==n] );

	fprintf( fout, "]}\n" );
	return 0;
}



//
typedef int (*FUNPTR_PRINT_ATTRINFO)( JVM_ATTR_TYPE, const CF_AttrInfo*, const CF_ClassFileMap *cf_map, int, FILE* );

static FUNPTR_PRINT_ATTRINFO cf_func_print_attrinfo[] = {
		(FUNPTR_PRINT_ATTRINFO)NULL,
		(FUNPTR_PRINT_ATTRINFO)_print_attr_ConstantValue,
		(FUNPTR_PRINT_ATTRINFO)_print_attr_Code,
		(FUNPTR_PRINT_ATTRINFO)_print_attr_StackMapTable,
		(FUNPTR_PRINT_ATTRINFO)_print_attr_Exception,
		(FUNPTR_PRINT_ATTRINFO)_print_attr_InnerClasses,
		(FUNPTR_PRINT_ATTRINFO)_print_attr_EnclosingMethod,
		(FUNPTR_PRINT_ATTRINFO)_print_attr_Default,			/* Synthetic */
		(FUNPTR_PRINT_ATTRINFO)_print_attr_Signature,
		(FUNPTR_PRINT_ATTRINFO)_print_attr_SourceFile,
		(FUNPTR_PRINT_ATTRINFO)_print_attr_Default,			/*SourceDebugExtension*/
		(FUNPTR_PRINT_ATTRINFO)_print_attr_LineNumberTable,
		(FUNPTR_PRINT_ATTRINFO)_print_attr_LocalVariableTable,
		(FUNPTR_PRINT_ATTRINFO)_print_attr_LocalVariableTypeTable,
		(FUNPTR_PRINT_ATTRINFO)_print_attr_Default, /* Deprecated */
		(FUNPTR_PRINT_ATTRINFO)_print_attr_Default, /* RuntimeVisibleAnnotations */
		(FUNPTR_PRINT_ATTRINFO)_print_attr_Default, /* RuntimeInvisibleAnnotations */
		(FUNPTR_PRINT_ATTRINFO)_print_attr_Default, /* RuntimeVisibleParameterAnnotations */
		(FUNPTR_PRINT_ATTRINFO)_print_attr_Default, /* RuntimeInvisibleParameterAnnotations */
		(FUNPTR_PRINT_ATTRINFO)_print_attr_Default, /* RuntimeVisibleTypeAnnotations */
		(FUNPTR_PRINT_ATTRINFO)_print_attr_Default, /* RuntimeInvisibleTypeAnnotations */
		(FUNPTR_PRINT_ATTRINFO)_print_attr_Default, /* AnnotationDefault */
		(FUNPTR_PRINT_ATTRINFO)_print_attr_BootstrapMethod,
		(FUNPTR_PRINT_ATTRINFO)_print_attr_MethodParameters,
		(FUNPTR_PRINT_ATTRINFO)_print_attr_Module,
		(FUNPTR_PRINT_ATTRINFO)_print_attr_ModulePackages,
		(FUNPTR_PRINT_ATTRINFO)_print_attr_ModuleMainClass,
		(FUNPTR_PRINT_ATTRINFO)_print_attr_Default, /* NestHost */
		(FUNPTR_PRINT_ATTRINFO)_print_attr_Default, /* NestMembers */
		(FUNPTR_PRINT_ATTRINFO)_print_attr_Default, /* Record */
		(FUNPTR_PRINT_ATTRINFO)_print_attr_PermittedSubclasses,
};

static const int _FUNC_PRINT_COUNT_ = sizeof(cf_func_print_attrinfo) / sizeof(FUNPTR_PRINT_ATTRINFO);


// Function:
int cf_print_attrinfo( const CF_AttrInfo *attr, cf_count_t attr_count, const CF_ClassFileMap *cf_map, FILE *fout )
{
	int n, r = 0;
	char name[64];
	cf_idx_t idx;
	cf_count_t c;
	juint32 attr_len;
	JVM_ATTR_TYPE tp;
	CP_InfoConstPtr cpi;

	int local_endian = (cf_map->auxi.local_endian_value<<1 | cf_map->auxi.local_endian_index) & 0x03;  // DO NOT affect attributes

	for( c=0; c < attr_count; c++ )
	{
		idx = _CF_ENDIAN_FLIP_INDEX( attr->attr_nm_idx );
		attr_len = attr->attr_len;
		_CF_ENDIAN_SWAP_U4( attr_len );

		if( !(cpi.Info = cf_get_cpitem( cf_map, idx )) )
		{
			printf( "invalid attribute name index(%d)\n", idx );
			goto LAB_NEXT;
		}

		n = cpi.Utf8->len;
		if( cpi.Utf8->tag != JVM_CONSTANT_Utf8 )
		{
			printf( "invalid attribute name(tag=%d, len=%d)\n", cpi.Utf8->tag, n );
			goto LAB_NEXT;
		}

		tp = cf_get_attr_type( cpi.Utf8->b, n );
		if( tp == JVM_ATTR_Unknown )
		{
			if( n >= sizeof(name) )
				n = sizeof(name) - 1 ;
			strncpy( name, cpi.Utf8->b, n );
			name[n] = 0;
			printf( "invalid attribute name(%s)\n", name );
			goto LAB_NEXT;
		}

		if( tp < _FUNC_PRINT_COUNT_ && cf_func_print_attrinfo[tp] )
		{
			cf_func_print_attrinfo[tp]( tp, attr, cf_map, local_endian, fout );
			r++;
		}

		LAB_NEXT:
		// try next attribute
		attr = (CF_AttrInfo*)( ((void*)attr) + attr_len + sizeof(CF_AttrHead) );
	}

	return r;
}


// Function:
void cf_print_attrtab( const CF_ClassFileMap *cf_map, FILE *fout )
{
	cf_count_t n, c;

	for( n=0; n < cf_map->attrtab_count; n++ )
	{
		for( c=0; c < cf_map->attrtab[n].attr_count; c++ )
		{
			cf_print_attrinfo( cf_map->attrtab[n].attr[c], 1, cf_map, fout );
		}
	}
}

