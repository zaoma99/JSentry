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


// Defined Attribute Name( MUST be ordered with CF_ATTR_TYPE )
const char CF_ATTR_NAME[32][48] = {
		"",
		"ConstantValue",
		"Code",
		"StackMapTable",
		"Exceptions",
		"InnerClasses",
		"EnclosingMethod",
		"Synthetic",
		"Signature",
		"SourceFile",
		"SourceDebugExtension",
		"LineNumberTable",
		"LocalVariableTable",
		"LocalVariableTypeTable",
		"Deprecated",
		"RuntimeVisibleAnnotations",
		"RuntimeInvisibleAnnotations",
		"RuntimeVisibleParameterAnnotations",
		"RuntimeInvisibleParameterAnnotations",
		"RuntimeVisibleTypeAnnotations",
		"RuntimeInvisibleTypeAnnotations",
		"AnnotationDefault",
		"BootstrapMethods",
		"MethodParameters",
		"Module",
		"ModulePackages",
		"ModuleMainClass",
		"NestHost",
		"NestMembers",
		"Record",
		"PermittedSubclasses"
};



//
typedef ssize_t (*FUNPTR_CHECK_ATTR)( const CF_AttrInfo*, ssize_t );

static FUNPTR_CHECK_ATTR cf_func_check_attr[] = {
		NULL
};

static const int _FUNC_COUNT_ = sizeof(cf_func_check_attr) / sizeof(FUNPTR_CHECK_ATTR);


// Function:
JVM_ATTR_TYPE cf_get_attr_type( const char *attr_name, int name_len )
{
	int i;

	if( name_len == -1 )
		name_len = strlen( attr_name );

	for( i=1; i < 32; i++ )
	{
		if( strncasecmp(attr_name, CF_ATTR_NAME[i], name_len) == 0 )
			return (JVM_ATTR_TYPE)i;
	}
	return JVM_ATTR_Unknown;
}



// Function:
CF_AttrInfo* cf_seek_attrinfo( const CF_AttrInfo *attr, cf_count_t count,
							   const char *name, int name_len, cf_idx_t seq,
							   const CF_ClassFileMap *cf_map )
{
	CP_Utf8 *utf8;
	cf_count_t c;

	if( name && name_len > 0 )
	{
		// by specified AttributeName and Sequence of same attribute
		for( c=0; c < count; c++ )
		{
			utf8 = (CP_Utf8*)cf_get_cpitem( cf_map, _CF_ENDIAN_FLIP_INDEX(attr->attr_nm_idx) );  // always process ENDIAN
			if( utf8 &&
				utf8->tag == JVM_CONSTANT_Utf8 &&
				utf8->len == name_len &&
				strncmp(name, utf8->b, name_len ) == 0  /* case sensitive */
				)
			{
				if( seq == 0 )
					break;
				seq--;
			}

			// try next
			attr = (CF_AttrInfo*)( (juint8*)attr + sizeof(CF_AttrHead) + _CF_ENDIAN_FLIP_U4(attr->attr_len) );
		}
	}
	else
	{
		// only by sequence, equal to attribute index
		for( c=0; c < count && seq > 0; c++, seq-- )
			attr = (CF_AttrInfo*)( (juint8*)attr + sizeof(CF_AttrHead) + _CF_ENDIAN_FLIP_U4(attr->attr_len) );
	}

	return seq == 0 && c < count ? (CF_AttrInfo*)attr : NULL;
}


//
CF_AttrInfo* cf_match_attrinfo( const CF_AttrInfo **attr, cf_count_t count,
							    const char *name, int name_len, cf_idx_t seq,
							    const CF_ClassFileMap *cf_map, cf_idx_t *idx )
{
	CP_Utf8 *utf8;
	cf_count_t c;

	// by specified AttributeName and Sequence of same attribute
	for( c=0; c < count; c++ )
	{
		utf8 = (CP_Utf8*)cf_get_cpitem( cf_map, _CF_ENDIAN_FLIP_INDEX(attr[c]->attr_nm_idx) );  // always process ENDIAN
		if( utf8 &&
			utf8->tag == JVM_CONSTANT_Utf8 &&
			utf8->len == name_len &&
			strncmp(name, utf8->b, name_len ) == 0  /* case sensitive */
			)
		{
			if( seq == 0 )
			{
				if( idx )
					*idx = c;
				break;
			}
			seq--;
		}
	}

	return seq == 0 && c < count ? (CF_AttrInfo*)attr[c] : NULL;
}


#if 0  // TODO:
// Function:
void cf_traverse_attrinfo( const CF_AttrInfo *attr, cf_count_t count, const CF_ClassFileMap *cf_map,
						   int (*cbf)(CF_AttrInfo*, cf_idx_t lvl, cf_idx_t idx) )
{
	// Two Attribute, Code and Record, allowed to traverse

	CP_Utf8 *utf8;
	cf_count_t c, v;
	JVM_ATTR_TYPE type;
	cf_idx_t fixed_index[2] = {0};

	for( v=c=0; c < count; c++ )
	{
		if( fixed_index[0] == attr->attr_nm_idx )
		{
			type = JVM_ATTR_Code;
		}
		else if( fixed_index[1] == attr->attr_nm_idx )
		{
			type = JVM_ATTR_Record;
		}
		else
		{
			utf8 = (CP_Utf8*)cf_get_cpitem( cf_map, _CF_ENDIAN_FLIP_INDEX(attr->attr_nm_idx) );  // always process ENDIAN
			if( utf8 && utf8->tag == JVM_CONSTANT_Utf8 )
			{
				type = cf_get_attr_type( utf8->b, utf8->len );
				if( type != JVM_ATTR_Code || type != JVM_ATTR_Record )
					type = 0;
			}
			else
				type = 0;
		}
		attr = (CF_AttrInfo*)( (juint8*)attr + sizeof(CF_AttrInfo) + _CF_ENDIAN_FLIP_U4(attr->attr_len) );
	}
}
#endif



// Function: only check length
CF_HIDDEN ssize_t cf_chk_attr( CF_AttrInfo *attr, cf_count_t count, ssize_t len, FILE *hfile, int local_endian )
{
	ssize_t n, sum = 0;

	for( ; len >= (ssize_t)sizeof(CF_AttrHead) && count > 0; --count )
	{
		n = _CF_ENDIAN_FLIP_U4(attr->attr_len) + sizeof(CF_AttrHead);

		// TODO: check attribute name

		len -= n;
		sum += n;
		attr = (CF_AttrInfo*)(((void*)attr) + n);
	}

	if( count <= 0 )
		return len < 0 ? len : sum;

	// seek in the file
	CF_AttrHead ah;
	ssize_t fpos_org = ftell( hfile );

	n = -len;  // offset from current file position

	do
	{
		if( n != 0 && fseek( hfile, n, SEEK_CUR ) != 0 )
			break;

		if( fread( &ah, 1, sizeof(CF_AttrHead), hfile ) != sizeof(CF_AttrHead) || ferror(hfile) )
			break;

		// TODO: check attribute name

		n = _CF_ENDIAN_FLIP_U4(ah.attr_len) + sizeof(CF_AttrHead);
		len -= n;
		//sum += n;
	}while( --count > 0 );

	fseek( hfile, fpos_org, SEEK_SET );  // restore file position

	return count != 0 ? 0 : len;
}


// Function:
ssize_t cf_check_attribute( int type, const CF_AttrInfo *attr, ssize_t len )
{
	return (type > 0 & type < _FUNC_COUNT_) ? cf_func_check_attr[type]( attr, len ) : 0;
}


//
void cf_alter_BootstrapMethod( CF_BootstrapMethod *ite, int num, int cp_off )
{
	int d, n, m;
	for( ; num > 0; num-- )
	{
		d = _CF_ENDIAN_FLIP_INDEX(ite->method_ref) + cp_off;
		ite->method_ref = _CF_ENDIAN_FLIP_INDEX( d );

		m = _CF_ENDIAN_FLIP_COUNT( ite->num_arguments );
		for( n=0; n < m; n++ )
		{
			d = _CF_ENDIAN_FLIP_INDEX(ite->arguments[n]) + cp_off;
			ite->arguments[n] = _CF_ENDIAN_FLIP_INDEX( d );
		}
		ite = (CF_BootstrapMethod*)((void*)ite + m*sizeof(cf_idx_t)+sizeof(CF_BootstrapMethod) );
	}
}


//
void cf_alter_BootstrapMethod_2( CF_BootstrapMethod *ite, int num, cf_idx_t *new_idx )
{
	int d, n, m;
	for( ; num > 0; num-- )
	{
		d = new_idx[_CF_ENDIAN_FLIP_INDEX(ite->method_ref)];
		ite->method_ref = _CF_ENDIAN_FLIP_INDEX( d );

		m = _CF_ENDIAN_FLIP_COUNT( ite->num_arguments );
		for( n=0; n < m; n++ )
		{
			d = new_idx[_CF_ENDIAN_FLIP_INDEX(ite->arguments[n])];
			ite->arguments[n] = _CF_ENDIAN_FLIP_INDEX( d );
		}
		ite = (CF_BootstrapMethod*)((void*)ite + m*sizeof(cf_idx_t)+sizeof(CF_BootstrapMethod) );
	}
}

//
void cf_mark_cpi_BootstrapMethod( CF_BootstrapMethod *ite, int num, cf_idx_t *mask )
{
	int n, m;
	for( ; num > 0; num-- )
	{
		mask[_CF_ENDIAN_FLIP_INDEX(ite->method_ref)] = 1;

		m = _CF_ENDIAN_FLIP_COUNT( ite->num_arguments );
		for( n=0; n < m; n++ )
			mask[_CF_ENDIAN_FLIP_INDEX(ite->arguments[n])] = 1;

		ite = (CF_BootstrapMethod*)((void*)ite + m*sizeof(cf_idx_t)+sizeof(CF_BootstrapMethod) );
	}
}
