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
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#include "jvas_tc.h"
#include "cf_impl.h"
#include "cf_bytecode.h"

#include "vt_api.h"

#include "facility/xmlAnalyzer.h"

JVAS_LOCAL long g_jvas_tc_tpix_base = 0;
JVAS_LOCAL long g_transform_sequence = 0;  // FOR DEBUG

JVAS_EXTERN jvas_tc_base g_jvas_tc_base;  // from jvas.c


#if 0
#define _IS_SPACE_CHAR(_C_)	\
		( (_C_) == '\r' || (_C_) == '\n' || \
		  (_C_) == '\t' || (_C_) == '\b' || \
		  (_C_) == '\f' || (_C_) == 32 )

#define _IS_SEPA_CHAR(_C_)	( (_C_) == '=' || (_C_) == '\0' || _IS_SPACE_CHAR(_C_) )

//
/*static int _ParseTextLine( char *txt, const char **argv, int argn )
{
	int n;
	char *s;


	s = txt;
	n = 0;
	while( n < argn )
	{
		// left trim
		while( _IS_SPACE_CHAR( *s ) ) s++;
		if( *s == 0 )
			break;

		argv[n++] = s++;

		// seek the end of current argument(!_IS_BLANK_CHAR( *s ))
		while( *s != 0 && *s != ',' && *s != '+' && *s != ';' ) s++;
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
}*/


//
static const char * _seek_argument( int argc, const char **argv, const char *argname )
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
static const char * _seek_argument2( int argc, const char **argv, const char *argname )
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
#endif


#define _CONST_CLASS_NAME_OBJECT	("L"CF_CONST_CLASS_NAME_OBJECT";")

//
inline static char* _make_stub_declaration( const char *target_name, const char *target_sign,
											const char *stub_name, const char *klass_name,
											const char *stub_method_prefix,
											char **stub_sign, int flags )
{
	if( flags & 2 )
		klass_name = CF_CONST_CLASS_NAME_OBJECT;

	int n;
	int ln1 = strlen( target_sign );
	int ln2 = stub_name && stub_name[0] ? strlen( stub_name ) : strlen( target_name ) + (stub_method_prefix ? strlen(stub_method_prefix)+1 : 0);
	int ln3 = flags&1 ? strlen( klass_name ) + 2 : 0;
	int sz = ALIGN_SIZE(ln1 + ln2 + ln3 + 2, 1024 );
	char *nm = malloc( sz );
	if( !nm )
		return NULL;

	if( stub_name && stub_name[0] )
		strcpy( nm, stub_name );
	else if( stub_method_prefix )
		sprintf( nm, "%s_%s", stub_method_prefix, target_name);
	else
		strcpy( nm, target_name );

	n = ln2 + 1; // include the '\0'
	nm[n++] = JVM_SIGNATURE_FUNC;

	if( ln3 )
	{
		nm[n] = JVM_SIGNATURE_CLASS;
		strcpy( &nm[n+1], klass_name );
		n += ln3;
		nm[n-1] = JVM_SIGNATURE_ENDCLASS;
	}

	juint8 array_dims = 0;
	const char *c = target_sign;
	const char *e = target_sign + ln1;
	const char *primitive_sign = CF_SIGNATURE_SYMBOL + 3;  // start from void
	const char *p;
	int m;
	BOOL chg;

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
				//type_idx = sizeof(CF_SIGNATURE_SYMBOL) - 3 - !array_dims;
			}
			else if( *c == JVM_SIGNATURE_ENDFUNC )
			{
				nm[n++] = JVM_SIGNATURE_ENDFUNC;
				continue;
			}
			else if( (p = strchr(primitive_sign, *c)) )
			{
				// primitive
				//type_idx = array_dims ? sizeof(CF_SIGNATURE_SYMBOL) - 3 : p - CF_SIGNATURE_SYMBOL;
				p = c;
			}
			if( !p )
				break;

			if( array_dims && (flags&4) )
			{
				// array => Object
				m = sizeof( _CONST_CLASS_NAME_OBJECT ) - 1;
				chg = TRUE;
			}
			else if( p > c && (flags&2) )
			{
				// reference => Object
				m = sizeof( _CONST_CLASS_NAME_OBJECT ) - 1 + array_dims;
				chg = TRUE;
			}
			else
			{
				// keep origin
				m = p - c + 1 + array_dims;
				chg = FALSE;
			}

			if( sz <= n+m )
			{
				// resize the buffer "nm"
				sz = ALIGN_SIZE(sz+m+2, 1024);
				void *ptr = realloc( nm, sz );
				if( !ptr )
					break;
				nm = ptr;
			}

			if( chg )
			{
				if( array_dims && (flags&4)==0 )
				{
					// save array signature
					memset( &nm[n], JVM_SIGNATURE_ARRAY, array_dims );
					n += array_dims;
				}

				// convert to Object
				strcpy( &nm[n], _CONST_CLASS_NAME_OBJECT );
				n += sizeof( _CONST_CLASS_NAME_OBJECT ) - 1;
			}
			else
			{
				// keep origin
				memcpy( &nm[n], c-array_dims, m );
				n += m;
			}
			array_dims = 0;
			c = p;
		}
	}

	if( c != e || array_dims != 0 )
	{
		free( nm );
		return NULL;
	}

	nm[n] = 0;
	*stub_sign = nm + ln2 + 1;
	return nm;
}


//
inline static char* _make_stub_declaration2( const char *target_name, const char *target_sign,
											 const char *stub_name, const char *klass_name,
											 const char *stub_method_prefix,
											 char **stub_sign, BOOL insert_this )
{
	char *nm, *si;
	int n0 = stub_name && stub_name[0] ? strlen( stub_name ) : strlen( target_name ) + (stub_method_prefix ? strlen(stub_method_prefix)+1 : 0);
	int n1 = insert_this ? strlen( klass_name ) + 2 : 0;
	int n = n0 + n1 + strlen( target_sign ) + 2;
	nm = malloc( n );
	if( !nm )
		return NULL;

	si = nm + n0 + 1;
	if( stub_name && stub_name[0] )
		strcpy( nm, stub_name );
	else if( stub_method_prefix )
		sprintf( nm, "%s_%s", stub_method_prefix, target_name );
	else
		strcpy( nm, target_name );

	if( insert_this )
	{
		si[1] = JVM_SIGNATURE_CLASS;
		strcpy( &si[2], klass_name );
		si[n1] = JVM_SIGNATURE_ENDCLASS;
	}
	si[0] = JVM_SIGNATURE_FUNC;
	strcpy( si+n1+1, target_sign+1 );

	*stub_sign = si;
	return nm;
}


//
inline static char* _declaration_to_name_sign( const char *decl_name, char **sign, const char *klass_ident )
{
	char *c = strchr( decl_name, JVM_SIGNATURE_FUNC );
	if( !c )
		return NULL;

	ssize_t d = c - decl_name;
	ssize_t n = -(klass_ident && strncmp(klass_ident, decl_name, d) == 0 && klass_ident[d] == 0) & 6;

	char *name = malloc(strlen(decl_name) + 2 + (n - d & -(n!=0)) );
	if( !name )
		return NULL;

	if( n )
		strcpy( name, "<init>" );
	else
		strncpy( name, decl_name, d ), name[n=d] = 0;

	*sign = name + n + 1;
	strcpy( *sign, decl_name+d );
	return name;
}

//
static CF_ClassFileMap* _load_cfmap( const char *file_path )
{
	CF_ClassFileMap *cfmap = (CF_ClassFileMap*)malloc( sizeof(CF_ClassFileMap) );
	if( cfmap )
	{
		memset( cfmap, 0, sizeof(CF_ClassFileMap) );
		CF_MAP_SET_FALG( *cfmap, blksz_abap, 1 );
		CF_MAP_SET_FALG( *cfmap, use_sysheap, CF_MAP_HEAP_DEFAULT );
		CF_MAP_LOCAL_ENDIAN_IDX( *cfmap );
		CF_MAP_LOCAL_ENDIAN_VAL( *cfmap );

		if( cf_file2map( file_path, cfmap, CF_FORMAT_BIN ) == 0 )
			return cfmap;

		free( cfmap );
	}
	return NULL;
}

//
__unused__ static int _load_cfmap2( const juint8 *src, ssize_t len, CF_ClassFileMap *cfmap, BOOL light_mode )
{
	memset( cfmap, 0, sizeof(CF_ClassFileMap) );
	CF_MAP_SET_FALG( *cfmap, blksz_abap, 1 );
	CF_MAP_SET_FALG( *cfmap, use_sysheap, CF_MAP_HEAP_DEFAULT );
	CF_MAP_LOCAL_ENDIAN_IDX( *cfmap );
	CF_MAP_LOCAL_ENDIAN_VAL( *cfmap );

	return cf_buffer2map( src, len, cfmap, CF_FORMAT_BIN | (light_mode<<30) );
}

//
int jvas_tc_base_open( jvas_tc_base *tcb )
{
	/*if( !tcb )
	{
		SetLastError( errInvParam );
		return -1;
	}*/

	tcb->sem = MsemCreate( FALSE );
	if( !tcb->sem )
	{
		SetLastError( errSema );
		return -1;
	}

	dlk_Init( &tcb->lnk );

	int i;
	tcb->count_class_transformed = 0;
	for( i=sizeof(tcb->class_transformed)/sizeof(vt_symtab)-1; i>=0; i-- )
		symtab_init( &tcb->class_transformed[i], 128 );

	return 0;
}

//
void jvas_tc_base_close( jvas_tc_base *tcb, BOOL bfree )
{
	//if( !tcb || !tcb->sem )
	//	return ;

	SemDelete( tcb->sem );
	tcb->sem = NULL;

	if( bfree )
		dlk_Release( &tcb->lnk );
	else
		dlk_Init( &tcb->lnk );

	int i;
	for( i=sizeof(tcb->class_transformed)/sizeof(vt_symtab)-1; i>=0; i-- )
		symtab_release( &tcb->class_transformed[i] );
}

//
int jvas_tc_base_add( jvas_tc_base *tcb, jvas_tc_def *node, BOOL check )
{
	/*if( !tcb ||
		!tcb->sem ||
		!node )
	{
		SetLastError( errInvParam );
		return -1;
	}*/

	if( !MsemTake(tcb->sem, WAIT_FOREVER) )
	{
		SetLastError( errSema );
		return -1;
	}

	int r = dlk_Add( &tcb->lnk, (dlk_node*)node, check ) != STATUS_OK;

	SetLastError( r ? errAgain : errSuccess );
	MsemGive( tcb->sem );
	return -r;
}

//
int jvas_tc_base_drop( jvas_tc_base *tcb, jvas_tc_def *node, BOOL check )
{
	/*if( !tcb ||
		!tcb->sem ||
		!node )
	{
		SetLastError( errInvParam );
		return -1;
	}*/

	if( !MsemTake(tcb->sem, WAIT_FOREVER) )
	{
		SetLastError( errSema );
		return -1;
	}

	int r = dlk_Drop( &tcb->lnk, (dlk_node*)node, check ) != STATUS_OK;

	// DO NOT free !!!

	SetLastError( r ? errNotExist : errSuccess );
	MsemGive( tcb->sem );
	return -r;
}


//
int jvas_tc_read_defines( const char *fn, jvas_tc_base *tcb, BOOL load_stub )
{
	HXML hxml;

	if( xmlLoad(fn, &hxml) != XML_ERR_OK )
	{
		_JVAS_TC_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "Can not load xml file \"%s\"\n", fn );
		return -1;
	}

	if( !xmlGetElement( hxml, "CLASS-TRANSFORMATION" ) )
	{
		_JVAS_TC_DEBUG_PRINT_( stdout, SABI_JOURNAL_INFO, "Can not seek the root node \"CLASS-TRANSFORMATION\" from the file \"%s\"\n", fn );
		xmlUnload( hxml );
		return -1;
	}

	XML_ELEMENT *ele, *son[2];
	XML_ATTRIBUTE *attr, *attrs[6];
	int m, n, d, mtd_num, callee_map_num;
	jvas_tc_def *tcdef = NULL;
	char *mtd_prefix = NULL;

	// CLASS
	m = xmlGetCurSubEleNum( hxml, "CLASS" );
	if( m < 1 )
		goto LAB_ERROR;

	for( n=0; n < m; n++ )
	{
		if( (ele=xmlGetCurSubEle( hxml, "CLASS", n )) )
		{
			mtd_num = xmlGetCurSubEleNum( hxml, "METHOD" );
			if( mtd_num < 1 )
			{
				printf( "without methods\n" );
				goto LAB_NEXT;
			}

			if( !tcdef )
			{
				// allocate new node
				tcdef = (jvas_tc_def*)malloc( sizeof(jvas_tc_def)+sizeof(ssize_t) );
				if( !tcdef )
					goto LAB_ERROR;
			}

			memset( tcdef, 0, sizeof(jvas_tc_def)+sizeof(ssize_t) );

#if _JVAS_TC_TPIDX_MANAGE_EN == TRUE
			tcdef->nref_ptr = (void*)tcdef + sizeof(jvas_tc_def);
#endif

			// CLASS's SON: Package
			d = 0;
			son[0] = xmlGetCurSubEle( hxml, "Package", 0 );
			if( son[0] )
			{
				if( son[0]->Value[0] )
				{
					p_strtrim( son[0]->Value );
					d = strlen( son[0]->Value );
				}
				xmlGetParentElement( hxml );
			}

			// CLASS's attr: Name
			attr = xmlGetEleAttr( ele, "Name" );
			if( !attr || attr->Value[0] == 0 )
				goto LAB_NEXT;

			tcdef->klass.name = realloc( tcdef->klass.name, strlen(attr->Value) + d + 2 + 6 );  // 2('/','\0'), 6(".class")
			if( !tcdef->klass.name )
				goto LAB_ERROR;

			if( d > 0 )
			{
				strcpy( tcdef->klass.name, son[0]->Value );
				if( son[0]->Value[d-1] != '/' )
					tcdef->klass.name[d++] = '/';
			}
			tcdef->klass_ident = tcdef->klass.name+d;
			strcpy( tcdef->klass_ident, attr->Value );


			// CLASS's attr: On
			attr = xmlGetEleAttr( ele, "On" );
			tcdef->klass.bits.dis = attr && stricmp(attr->Value, "yes") != 0 && stricmp(attr->Value, "true") != 0;
			tcdef->klass.bits.lazy = !tcdef->klass.bits.dis ? 0 : attr && (stricmp(attr->Value, "lazy") == 0 || stricmp(attr->Value, "delay") == 0);

			// CLASS's attr: others


			// CLASS's SON: STUB
			ele = xmlGetCurSubEle( hxml, "STUB", 0 );
			if( ele )
			{
				// STUB's SON: Package
				d = 0;
				son[0] = xmlGetCurSubEle( hxml, "Package", 0 );
				if( son[0] )
				{
					if( son[0]->Value[0] )
					{
						p_strtrim( son[0]->Value );
						d = strlen( son[0]->Value );
					}
					xmlGetParentElement( hxml );
				}

				// STUB's attr: Name
				attr = xmlGetEleAttr( ele, "Name" );
				if( !attr || attr->Value[0] == 0 )
					goto LAB_OUT_STUB;

				tcdef->stub.name = realloc( tcdef->stub.name, strlen(attr->Value)+d+2+6 );
				if( !tcdef->stub.name )
					goto LAB_ERROR;

				if( d > 0 )
				{
					strcpy( tcdef->stub.name, son[0]->Value );
					if( son[0]->Value[d-1] != '/' )
						tcdef->stub.name[d++] = '/';
				}
				tcdef->stub_ident = tcdef->stub.name+d;
				strcpy( tcdef->stub_ident, attr->Value );

				// STUB's attr: others

				// STUB's SON: MethodPrefix
				mtd_prefix = NULL;
				ele = xmlGetCurSubEle( hxml, "MethodPrefix", 0 );
				if( ele )
				{
					if( ele->Value[0] )
						p_strtrim( ele->Value );
					if( ele->Value[0] )
						mtd_prefix = ele->Value;

					xmlGetParentElement( hxml );
				}

				//
				/*ele = xmlGetCurSubEle( hxml, "JAR", 0 );
				if( ele )
				{
					xmlGetParentElement( hxml );
				}*/

				LAB_OUT_STUB:
				xmlGetParentElement( hxml );
			}
			else
				goto LAB_NEXT;


			// CLASS's SON: METHOD
			d = (sizeof(struct jvas_class_method)+sizeof(struct jvas_method_desc)) * mtd_num * 2;
			struct jvas_class_method *mtd = (struct jvas_class_method*)malloc( d );
			if( !mtd )
				goto LAB_ERROR;

			struct jvas_method_desc *desc = (struct jvas_method_desc*)&mtd[mtd_num*2];
			struct jvas_class_method *mtd2 = &mtd[mtd_num];
			struct jvas_method_desc *desc2 = &desc[mtd_num];

			tcdef->klass.method_count = mtd_num;
			tcdef->klass.method_table = mtd;
			tcdef->stub.method_count = mtd_num;
			tcdef->stub.method_table = mtd2;
			memset( (void*)mtd, 0, d );

			int i, a, d, sn;
			char *_name, *_sign;

			for( i=a=0; i < mtd_num; i++ )
			{
				if( (ele = xmlGetCurSubEle( hxml, "METHOD", i )) )
				{
					attr = xmlGetEleAttr( ele, "Target" );
					attrs[0] = xmlGetEleAttr( ele, "Stub" );
					attrs[1] = xmlGetEleAttr( ele, "On" );
					//attrs[2] = xmlGetEleAttr( ele, "Static" );
					attrs[2] = xmlGetEleAttr( ele, "Flag" );
					attrs[3] = xmlGetEleAttr( ele, "Mode" );
					attrs[4] = xmlGetEleAttr( ele, "EECM" );
					//attrs[5] = xmlGetEleAttr( ele, "TPIX" );
					if( attr && attrs[0] )
					{
						mtd[a].desc = &desc[a];
						mtd[a].klass = &tcdef->klass;

						mtd2[a].desc = &desc2[a];
						mtd2[a].klass = &tcdef->stub;

						d = xmlUnescape( attr->Value, attr->Value, -1, TRUE );
						attr->Value[d] = 0;

						//desc[a].tpix = -1;

						_name = _declaration_to_name_sign( attr->Value, &_sign, tcdef->klass_ident );
						if( !_name )
							goto LAB_ERROR;

						// eliminate duplication
						sn = strrchr( _sign, JVM_SIGNATURE_ENDFUNC ) - _sign + 1;
						for( d=a-1;
							 d >= 0
							 && ((strcmp(_name, desc[d].name)!=0 || strncmp(_sign, desc[d].sign, sn)!=0));
							 d-- );
						if( d >= 0 )
							goto LAB_NEXT_ELEM;  // duplication, ignore it
						desc[a].name = _name;
						desc[a].sign = _sign;

#if 0
						if( attrs[2] && stricmp(attrs[2]->Value, "yes") != 0 && stricmp(attrs[2]->Value, "true") != 0 )
							attrs[2] = NULL; // is non-static method

						if( !attrs[0]->Value[0] && attrs[2] )
							desc2[a] = desc[a]; // static method, and the stub's declaration is same to the target
						else if( attrs[0]->Value[0] && strchr(attrs[0]->Value, JVM_SIGNATURE_FUNC) )
							desc2[a].name = _declaration_to_name_sign( attrs[0]->Value, &desc2[a].sign, NULL );  // full stub declaration
						else
						{
							// non-static method, or the stub's declaration is different with the target
							desc2[a].name = _make_stub_declaration2( desc[a].name, desc[a].sign,
												attrs[0]->Value, tcdef->klass.name, mtd_prefix,
												&desc2[a].sign, attrs[2]==NULL );
						}
#endif

						d = attrs[2] ? strtol(attrs[2]->Value, NULL, 0) & 0xff : 0; // Flag

						if( strchr(attrs[0]->Value, JVM_SIGNATURE_FUNC) )
							desc2[a].name = _declaration_to_name_sign( attrs[0]->Value, &desc2[a].sign, NULL );  // assume it is full stub declaration
						else if( (d&3)==0 && !attrs[0]->Value[0] )
							desc2[a] = desc[a]; // the stub's declaration is same to the target
						else if( (d&2)==0 )
							// generate a stub declaration comply with the target(change method name if required, insert "this" if required)
							desc2[a].name = _make_stub_declaration2( desc[a].name, desc[a].sign,
												attrs[0]->Value, tcdef->klass.name, mtd_prefix, &desc2[a].sign, (d&1) );
						else
							// generate a stub declaration comply with the target(convert all objects to the "java/lang/Object")
							desc2[a].name = _make_stub_declaration( desc[a].name, desc[a].sign,
												attrs[0]->Value, tcdef->klass.name, mtd_prefix, &desc2[a].sign, (d&7) );

						if( !desc2[a].name )
							goto LAB_ERROR;

						if( attrs[3] && attrs[3]->Value[0] )
						{
							if( stricmp( attrs[3]->Value, "standard") == 0 )
								desc[a].mod = 1;
							else if( stricmp( attrs[3]->Value, "anywhere") == 0 )
								desc[a].mod = 2;
							else if( stricmp( attrs[3]->Value, "mixed") == 0 )
								desc[a].mod = 3;
							else
								desc[a].mod = 0; // undefined
						}
						else
							desc[a].mod = 1; // default is "standard"

						//desc[a].sta = !!attrs[2];
						desc[a].flag = d;
						desc[a].dis = attrs[1] && stricmp(attrs[1]->Value, "yes") != 0 && stricmp(attrs[1]->Value, "true") != 0;

						// added, 2022-06-27
						if( attrs[4] )
							desc[a].mod |= (strtol( attrs[4]->Value, NULL, 0 ) << 4) & 0x70;

						// other attributes

#if _JVAS_TC_TPIDX_MANAGE_EN == TRUE
						desc[a].ctxid = 0;
#endif
						a++;
					}
					LAB_NEXT_ELEM:
					xmlGetParentElement( hxml );
				}
			}// for

			tcdef->klass.method_count = a;
			tcdef->stub.method_count = a;
			if( a > 0 && load_stub )
			{
				// try to load stub class file
				d = strlen(tcdef->stub.name);
				strcpy( &tcdef->stub.name[d], ".class" );  // 在加载配置时已经分配了足够的内存用来存放".class"
				tcdef->stub_cfmap = _load_cfmap( tcdef->stub.name );
				tcdef->stub.name[d] = 0;

				//
				if( tcdef->stub_cfmap )
				{
					if( jvas_tc_prefix_stub( tcdef->stub_cfmap, NULL ) != tcdef->stub_cfmap->mdtab[0].method_count )
					{
						cf_free_map( tcdef->stub_cfmap );
						free( tcdef->stub_cfmap );
						tcdef->stub_cfmap = NULL;
					}
				}
			}

			// try to load callee mapping
			callee_map_num = xmlGetCurSubEleNum( hxml, "CALLEE-MAPPING" );
			if( callee_map_num > 0 )
			{
				jvas_tc_name_map *name_map;
				jvas_tc_callee_map *map = (jvas_tc_callee_map*)malloc( (d=sizeof(jvas_tc_callee_map) * callee_map_num) );
				if( !map )
					goto LAB_ERROR;
				memset( map, 0, d );

				for( i=a=0; i < callee_map_num; i++ )
				{
					if( !(ele = xmlGetCurSubEle( hxml, "CALLEE-MAPPING", i )) )
						continue;

					attr = xmlGetEleAttr( ele, "Class" );
					attrs[0] = xmlGetEleAttr( ele, "On" );
					if( !attr || !attr->Value[0] )
						goto LAB_NEXT_ELEM2;
					if( attrs[0] && stricmp(attrs[0]->Value, "yes") != 0 && stricmp(attrs[0]->Value, "true") != 0 )
						goto LAB_NEXT_ELEM2;
					m = xmlGetCurSubEleNum( hxml, "Callee" );
					if( m < 1 )
						goto LAB_NEXT_ELEM2;

					// copy class name
					map[a].klass_name = malloc( strlen(attr->Value) + 1 );
					if( !map[a].klass_name )
						goto LAB_ERROR;
					strcpy( map[a].klass_name, attr->Value );

					// load Callees
					name_map = (jvas_tc_name_map*)malloc( (d=sizeof(jvas_tc_name_map) * m) );
					if( !name_map )
						goto LAB_ERROR;
					memset( name_map, 0, d );

					for( n=d=0; n < m; n++ )
					{
						if( !(ele = xmlGetCurSubEle( hxml, "Callee", n )) )
							continue;

						attr = xmlGetEleAttr( ele, "From" );
						attrs[0] = xmlGetEleAttr( ele, "To" );
						attrs[1] = xmlGetEleAttr( ele, "On" );
						if( attr && attr->Value[0] )
						{
							if( !attrs[1] || stricmp(attrs[1]->Value, "yes") == 0 || stricmp(attrs[1]->Value, "true") == 0 )
							{
								name_map[d].name = _declaration_to_name_sign( attr->Value, &name_map[d].sign, NULL );
								if( !name_map[d].name )
									goto LAB_ERROR;
								if( attrs[0] && attrs[0]->Value[0] )
								{
									name_map[d].name2 = malloc( strlen(attrs[0]->Value)+1 );
									if( !name_map[d].name2 )
										goto LAB_ERROR;
									strcpy( name_map[d].name2, attrs[0]->Value );
								}
								//else
								//	name_map[d].name2 = NULL;
								d++;
							}
						}
						LAB_NEXT_ELEM2:
						xmlGetParentElement( hxml );
					} // for(n)

					if( (ele = xmlGetCurSubEle( hxml, "NewClass", 0 )) )
					{
						if( ele->Value[0] )
						{
							map[a].new_klass_name = malloc( strlen(ele->Value) + 1 );
							if( !map[a].new_klass_name )
								goto LAB_ERROR;
							strcpy( map[a].new_klass_name, ele->Value );
						}
						xmlGetParentElement( hxml );
					}
					//else
					//	map[a].new_klass_name = NULL;
					map[a].callee_num = d;
					map[a].callees = name_map;
					a++;

					xmlGetParentElement( hxml );
				} // for(i)
				tcdef->callee_map = map;
				tcdef->callee_map_num = a;
			}

			tcdef->sem = MsemCreate( FALSE );
			if( !tcdef->sem )
				goto LAB_ERROR;

#if _JVAS_TC_TPIDX_MANAGE_EN == TRUE
			*tcdef->nref_ptr = 1;
#endif

			// add to the base
			jvas_tc_base_add( tcb, tcdef, FALSE );
			tcdef = NULL;

			LAB_NEXT:
			xmlGetParentElement( hxml );
		}
	}// for

	xmlUnload( hxml );
	return 0;


	LAB_ERROR:
	d = errno;
	if( tcdef )
	{
		jvas_tc_free_def( tcdef );
		free( tcdef );
	}
	xmlUnload( hxml );
	errno = d;
	return -1;
}

int jvas_tc_read_def( const char *fn, jvas_tc_base *tcb )
{
	return jvas_tc_read_defines( fn, tcb, FALSE );
}

// Function:
int jvas_tc_loadEx( const char *home, jvas_tc_base *tcb, BOOL load_stub )
{
	DIR *dir = opendir( home );
	if( !dir )
		return -1;

	struct dirent de, *deptr;
	struct stat st;
	char str[512], *path;
	int t, sum = -1;

	strcpy( str, home );
	path = str + strlen( str );
	*path++ = '/';

	if( MsemTake(tcb->sem, WAIT_FOREVER) )
	{
		while( readdir_r(dir, &de, &deptr) == 0 && deptr )
		{
			strcpy( path, de.d_name );
			if( access(str, R_OK) == 0 &&
				stat(str, &st) == 0 &&
				((t=(st.st_mode & S_IFMT)) == S_IFREG || t == S_IFLNK) )
			{
				if( jvas_tc_read_defines( str, tcb, load_stub ) != -1 )
				{
					sum++;
				}
			}
		}

		MsemGive( tcb->sem );
	}

	closedir( dir );
	return sum + (sum != -1);
}

int jvas_tc_load( const char *home, jvas_tc_base *tcb )
{
	return jvas_tc_loadEx( home, tcb, FALSE );
}

#if _JVAS_TC_TPIDX_MANAGE_EN == TRUE
// Function:
jvas_tc_def* jvas_tc_dup_def( jvas_tc_base *tcb, jvas_tc_def *src, const char *kname )
{
	// allocate new node
	jvas_tc_def *new_tcdef = (jvas_tc_def*)malloc( sizeof(jvas_tc_def) );
	if( !new_tcdef )
		return NULL;

	memcpy( new_tcdef, src, sizeof(jvas_tc_def) );

	int ln = strlen( kname );
	const char *cc = strrchr( kname, '/' );
	if( !cc )
		cc = kname;

	int ident_ln = ln - (cc - kname);
	new_tcdef->klass.name = malloc( ln + 8 + ident_ln + strlen(src->klass.name) + 2 );
	if( !new_tcdef->klass.name )
	{
		free( new_tcdef );
		return NULL;
	}

	//
	strcpy( new_tcdef->klass.name, kname );
	//
	new_tcdef->klass_ident = new_tcdef->klass.name+ln+8;
	strcpy( new_tcdef->klass_ident, cc+1 );
	//
	new_tcdef->ifname = new_tcdef->klass_ident + ident_ln;
	strcpy( new_tcdef->ifname, src->klass.name );
	//
	__sync_add_and_fetch( new_tcdef->nref_ptr, 1 );
	jvas_tc_base_add( tcb, new_tcdef, FALSE );
	return new_tcdef;
}
#endif

// Function:
void jvas_tc_free_def( jvas_tc_def *tcdef )
{
#if _JVAS_TC_TPIDX_MANAGE_EN == TRUE
	if( __sync_sub_and_fetch( tcdef->nref_ptr, 1 ) <= 0 )
#endif
	{
		if( tcdef->klass.method_table )
		{
			struct jvas_class_method *mtd = tcdef->klass.method_table;
			struct jvas_class_method *mtd2 = tcdef->stub.method_table;
			int n = tcdef->stub.method_count - 1;
			for( ; n >= 0; n-- )
			{
				if( mtd2[n].desc->name != mtd[n].desc->name )
					free( mtd2[n].desc->name );
			}

			n = tcdef->klass.method_count - 1;
			for( ; n >= 0; n-- )
			{
				if( mtd[n].desc->name )
					free( mtd[n].desc->name );
			}
			free( tcdef->klass.method_table );
		}

		if( tcdef->stub.name )
			free( tcdef->stub.name );


		if( tcdef->callee_map )
		{
			jvas_tc_name_map *name_map;
			jvas_tc_callee_map *map = tcdef->callee_map;
			int m, n = tcdef->callee_map_num - 1;
			for( ; n >= 0; n-- )
			{
				if( (name_map = map[n].callees) )
				{
					m = map[n].callee_num - 1;
					for( ; m >= 0; m-- )
					{
						if( name_map[m].name )
							free( name_map[m].name );
						if( name_map[m].name2 )
							free( name_map[m].name2 );
					}
					free( name_map );
				}
				if( map[n].klass_name )
					free( map[n].klass_name );
				if( map[n].new_klass_name )
					free( map[n].new_klass_name );
			}
			free( map );
		}


		if( tcdef->stub_cfmap )
		{
			cf_free_map( tcdef->stub_cfmap );
			free( tcdef->stub_cfmap );
		}

		if( tcdef->sem )
			SemDelete( tcdef->sem );
	}

	if( tcdef->klass.name )
		free( tcdef->klass.name );

	memset( tcdef, 0, sizeof(jvas_tc_def) );
}


// Function:
void jvas_tc_free( jvas_tc_base *tcb )
{
	jvas_tc_def *tcdef, *tmp;

	if( MsemTake(tcb->sem, WAIT_FOREVER) )
	{
		tcdef = (jvas_tc_def*)tcb->lnk.head;
		while( tcdef )
		{
			jvas_tc_free_def( tcdef );
			tmp = tcdef;
			tcdef = (jvas_tc_def*)tcdef->node.next;
			free( tmp );
		}
		memset( &tcb->lnk, 0, sizeof(dlk_link) );
		MsemGive( tcb->sem );
	}
}

// Function:
const jvas_tc_def* jvas_tc_find_define( const jvas_tc_base *tcb, const char *klass_name, BOOL onlyname, BOOL lock )
{
	jvas_tc_def *tcdef;

	if( !lock || MsemTake(tcb->sem, WAIT_FOREVER) )
	{
		tcdef = (jvas_tc_def*)tcb->lnk.head;

		if( klass_name )
		{
			if( onlyname )
				while( tcdef && strcmp(tcdef->klass_ident, klass_name) != 0 ) tcdef = (jvas_tc_def*)tcdef->node.next;
			else
				while( tcdef && strcmp(tcdef->klass.name, klass_name) != 0 ) tcdef = (jvas_tc_def*)tcdef->node.next;
		}

		if( lock )
			MsemGive( tcb->sem );
		return tcdef;
	}
	return NULL;
}

//
const jvas_tc_def* jvas_tc_find_define_2( const jvas_tc_base *tcb, const char *klass_name, int name_len, BOOL onlyname, BOOL lock )
{
	jvas_tc_def *tcdef;

	if( !lock || MsemTake(tcb->sem, WAIT_FOREVER) )
	{
		tcdef = (jvas_tc_def*)tcb->lnk.head;

		if( klass_name )
		{
			if( onlyname )
				while( tcdef && (strlen(tcdef->klass_ident) != name_len || strncmp(tcdef->klass_ident, klass_name, name_len) != 0) )
					tcdef = (jvas_tc_def*)tcdef->node.next;
			else
				while( tcdef && (strlen(tcdef->klass.name) != name_len || strncmp(tcdef->klass.name, klass_name, name_len) != 0) )
					tcdef = (jvas_tc_def*)tcdef->node.next;
		}

		if( lock )
			MsemGive( tcb->sem );
		return tcdef;
	}
	return NULL;
}

// Function:
int jvas_tc_find_method( const jvas_tc_def *tcdef, const char *name, const char *sign )
{
	struct jvas_class_method *mtd = tcdef->klass.method_table;
	//struct jvas_class_method *mtd2 = tcdef->stub.method_table;
	int n = tcdef->klass.method_count - 1;

	for( ; n >= 0; n-- )
	{
		if( (!name || strcmp(mtd[n].desc->name, name) == 0) &&
			(!sign || strcmp(mtd[n].desc->sign, sign) == 0)
			)
			return n;
	}
	return -1;
}


// Function:
int jvas_tc_find_callee_map( const jvas_tc_def *tcdef, const char *klass_name )
{
	int n = tcdef->callee_map_num;
	jvas_tc_callee_map *map = tcdef->callee_map;

	while( --n>=0 && strcmp(map[n].klass_name, klass_name)!=0 );
	return n;
}

//
int jvas_tc_find_callee_map2( const jvas_tc_def *tcdef, const char *klass_name, int name_len )
{
	int n = tcdef->callee_map_num;
	jvas_tc_callee_map *map = tcdef->callee_map;

	while( --n>=0 && (strncmp(map[n].klass_name, klass_name, name_len)!=0 || map[n].klass_name[name_len]!=0) );
	return n;
}

// Function:
int jvas_tc_find_callee( jvas_tc_callee_map *map, const char *name, const char *sign )
{
	int n = map->callee_num;
	jvas_tc_name_map *name_map = map->callees;

	while( --n>=0 &&
			(strcmp(name_map[n].name, name)!=0 || (sign && strcmp(name_map[n].sign, sign)!=0)) );
	return n;
}

//
int jvas_tc_find_callee2( jvas_tc_callee_map *map, const char *name, int name_len, const char *sign, int sign_len )
{
	int n = map->callee_num;
	jvas_tc_name_map *name_map = map->callees;

	while( --n>=0 &&
			((strncmp(name_map[n].name, name, name_len)!=0 || name_map[n].name[name_len]!=0) ||
			 (sign && (strncmp(name_map[n].sign, sign, sign_len)!=0 || name_map[n].sign[sign_len]!=0))) );
	return n;
}

// only called by the jvas_tc_remap_callee
static int _add_callee_map_match( const jvas_tc_callee_map *callee_map, jvas_tc_callee_map ***matched_ptr, int *num_ptr )
{
	jvas_tc_callee_map **_map_match = *matched_ptr;
	const char *klass_name = callee_map->klass_name;
	int i = -1;
	int num = *num_ptr;
	while( ++i < num && strcmp(_map_match[i]->klass_name, klass_name)!=0 );
	if( i >= num )
	{
		// append
		if( !(num & 31) )
		{
			// reallocate
			_map_match = (jvas_tc_callee_map**)realloc( _map_match, sizeof(void*) * (num+32) );
			if( !_map_match )
				return -1;
			*matched_ptr = _map_match;
		}
		_map_match[num] = (jvas_tc_callee_map*)callee_map;
		*num_ptr = num+1;
	}
	return i;
}

// only called by the jvas_tc_remap_callee
static int _add_callee_name_match( const jvas_tc_name_map *callee_name, jvas_tc_name_map ***matched_ptr, int *num_ptr )
{
	jvas_tc_name_map **_name_match = *matched_ptr;
	const char *name = callee_name->name;
	const char *sign = callee_name->sign;
	int i = -1;
	int num = *num_ptr;
	while( ++i < num && (strcmp(_name_match[i]->name, name)!=0 || strcmp(_name_match[i]->sign, sign)!=0) );
	if( i >= num )
	{
		// append
		if( !(num & 31) )
		{
			// reallocate
			_name_match = (jvas_tc_name_map**)realloc( _name_match, sizeof(void*) * (num+32) );
			if( !_name_match )
				return -1;
			*matched_ptr = _name_match;
		}
		_name_match[num] = (jvas_tc_name_map*)callee_name;
		*num_ptr = num+1;
	}
	return i;
}

// only called by the jvas_tc_remap_callee
inline static int _add_cpitem_match( UINT64 idx, UINT64 **matched_ptr, int num )
{
	UINT64 *_cp = *matched_ptr;
	if( !(num & 31) )
	{
		// reallocate
		_cp = (UINT64*)realloc( _cp, sizeof(UINT64) * (num+32) );
		if( !_cp )
			return -1;
		*matched_ptr = _cp;
	}
	_cp[num++] = idx;
	return num;
}

// synchronizing TPIX with the VTrace
static void _sync_tpix( const char *kname, const char *ifname, int methods_num,
						const long *tpix, const char **methods_name, const char **methods_desc,
						const char *flags, const char *mask )
{
	int i;
	const char *sign;
	char *cc, _sign[256];
	char *_buf = _sign;
	ssize_t sz, _sz = sizeof( _sign );

	for( i=0; i < methods_num; i++ )
	{
		if( tpix[i] > 0 && mask[i] && (flags[i] & 0x80) )
		{
			sign = methods_desc[i];
			cc = strrchr( sign, JVM_SIGNATURE_ENDFUNC );
			if( cc && cc[1] != 0 )
			{
				// ignore the "return type"
				if( (sz=cc - sign + 1) >= _sz )
				{
					_sz = ALIGN_SIZE( sz+1, 256 );
					cc = malloc( _sz );
					if( !cc )
					{
						_JVAS_TC_DEBUG_PRINT_( stdout, SABI_JOURNAL_CRIT, "%s, failed to allocate memory, errno=%d\n", __FUNCTION__, errno );
						break;
					}
					if( _buf != _sign )
						free( _buf );
					_buf = cc;
				}
				memcpy( _buf, sign, sz );
				_buf[sz] = 0;
				sign = _buf;
			}
			//
			vt_tpix_sync( tpix[i], methods_name[i], sign, kname, ifname, NULL );
		}
	}

	if( _buf != _sign )
		free( _buf );
}

// only called by the jvas_tc_remap_callee
static int _do_changes( void **ptrs, int *nums )
{
	jvas_tc_callee_map **_callee_map_match = (jvas_tc_callee_map**)ptrs[0];
	jvas_tc_name_map **_name_match = (jvas_tc_name_map**)ptrs[1];
	UINT64 *_cp_match = (UINT64*)ptrs[2];
	int _callee_map_match_num = nums[0];
	int _name_match_num = nums[1];
	int _cp_match_num = nums[2];

	jvas_tc_def *tcdef = (jvas_tc_def*)ptrs[3];
	CF_ClassFileMap *cfmap = (CF_ClassFileMap*)ptrs[4];
	int i, n, m, a, k;
	ssize_t sz;
	void *p;
	CP_InfoUnion **new_cps, *add_cpi, *add_utf8;

	// count length and number of constants going to be appended
	sz = 0;
	for( i=0; i < _callee_map_match_num; i++ )
		sz += sizeof(CP_Utf8) + strlen(_callee_map_match[i]->new_klass_name ? _callee_map_match[i]->new_klass_name : tcdef->stub.name);

	for( i=n=0; i < _name_match_num; i++ )
	{
		if( _name_match[i]->name2 )
		{
			n++;
			sz += sizeof(CP_Utf8) + strlen(_name_match[i]->name2);
		}
	}
	sz += _callee_map_match_num * sizeof(CP_Class) + n * sizeof(CP_NameAndType);

	// allocate memory for new constants
	p = cf_malloc( cfmap->auxi.use_sysheap, &cfmap->auxi.mmphd, sz );
	if( !p )
		return -1;

	// allocate memory for constants pool index table
	m = cfmap->cptab[0].cp_count + ((_callee_map_match_num + n) << 1);
	new_cps = (CP_InfoUnion**)cf_malloc( cfmap->auxi.use_sysheap, &cfmap->auxi.mmphd, sizeof(void**)*m );
	if( !new_cps )
	{
		cf_free( cfmap->auxi.use_sysheap, &cfmap->auxi.mmphd, p );
		return -1;
	}

	// keep old entries
	k = cfmap->cptab[0].cp_count;
	memcpy( new_cps, cfmap->cptab[0].cps, sizeof(CP_Info*) * k );

	// fill new constants
	// layout: | Class | NameAndType | Utf8 |
	a = k + _callee_map_match_num + n;  // begin position of Utf8
	add_utf8 = (CP_InfoUnion*)(p + _callee_map_match_num * sizeof(CP_Class) + n * sizeof(CP_NameAndType));

	// New Classes
	add_cpi = (CP_InfoUnion*)p;
	for( i=0; i < _callee_map_match_num; i++ )
	{
		new_cps[k] = add_cpi;
		add_cpi->Class.tag = JVM_CONSTANT_Class;
		add_cpi->Class.name_idx = a;
		add_cpi = (CP_InfoUnion*)( (void*)add_cpi + sizeof(CP_Class) );

		// Utf8
		new_cps[a] = add_utf8;
		add_utf8->Utf8.tag = JVM_CONSTANT_Utf8;
		if( _callee_map_match[i]->new_klass_name )
		{
			add_utf8->Utf8.len = strlen( _callee_map_match[i]->new_klass_name );
			memcpy( add_utf8->Utf8.b, _callee_map_match[i]->new_klass_name, add_utf8->Utf8.len );
		}
		else
		{
			add_utf8->Utf8.len = strlen( tcdef->stub.name );
			memcpy( add_utf8->Utf8.b, tcdef->stub.name, add_utf8->Utf8.len );
		}
		add_utf8 = (CP_InfoUnion*)( (void*)add_utf8 + sizeof(CP_Utf8) + add_utf8->Utf8.len );
		a++;
		k++;
	}

	// New NameAndTypes
	CP_InfoUnion **old_cps = (CP_InfoUnion**)cfmap->cptab[0].cps;
	for( i=0; i < _name_match_num; i++ )
	{
		if( _name_match[i]->name2 )
		{
			new_cps[k] = add_cpi;
			add_cpi->NameType.tag = JVM_CONSTANT_NameAndType;
			add_cpi->NameType.name_idx = a;
			add_cpi->NameType.desc_idx = old_cps[_name_match[i]->cpidx]->NameType.desc_idx;
			add_cpi = (CP_InfoUnion*)( (void*)add_cpi + sizeof(CP_NameAndType) );

			// Utf8
			new_cps[a] = add_utf8;
			add_utf8->Utf8.tag = JVM_CONSTANT_Utf8;
			add_utf8->Utf8.len = strlen( _name_match[i]->name2 );
			memcpy( add_utf8->Utf8.b, _name_match[i]->name2, add_utf8->Utf8.len );
			add_utf8 = (CP_InfoUnion*)( (void*)add_utf8 + sizeof(CP_Utf8) + add_utf8->Utf8.len );

			_name_match[i]->cpidx = k;  // change to new NameAndType
			a++;
			k++;
		}
	}

	// change References
	int x1, x2, x3;
	k = cfmap->cptab[0].cp_count;
	for( i=0; i < _cp_match_num; i++ )
	{
		x1 = _cp_match[i] >> 48;
		x2 = (_cp_match[i] >> 32) & 0xFFFF;
		x3 = _cp_match[i] & 0xFFFF;
		new_cps[x3]->Method.class_idx = k + x1;
		new_cps[x3]->Method.nm_tp_idx = _name_match[x2]->cpidx;
	}

	// delete old cp table
	cf_free( cfmap->auxi.use_sysheap, &cfmap->auxi.mmphd, cfmap->cptab[0].cps );

	// update cptab[0]
	cfmap->cptab[0].cps = (CP_Info**)new_cps;
	cfmap->cptab[0].cp_count = m;
	cfmap->cptab[0].cps_size += sz;
	return m;
}

// Function:
int jvas_tc_remap_callee( const jvas_tc_def *tcdef, CF_ClassFileMap *cfmap )
{
	int i, x1, x2, x3;
	int num = cfmap->cptab[0].cp_count;
	CP_InfoUnion **cps = (CP_InfoUnion**)cfmap->cptab[0].cps;

	jvas_tc_callee_map **_callee_map_match = NULL;
	jvas_tc_name_map **_name_match = NULL;
	UINT64 *_cp_match = NULL;
	int _callee_map_match_num = 0;
	int _name_match_num = 0;
	int _cp_match_num = 0;
	int ret = -1;

	for( i=1; i < num; i++ )
	{
		if( cps[i] &&
			(cps[i]->Info.tag == JVM_CONSTANT_Methodref ||
			 cps[i]->Info.tag == JVM_CONSTANT_Fieldref  ||
			 cps[i]->Info.tag == JVM_CONSTANT_InterfaceMethodref)
			)
		{
			if( (x1=cps[i]->Method.class_idx) < num &&
				(x2=cps[i]->Method.nm_tp_idx) < num )
			{
				if( cps[x1]->Info.tag == JVM_CONSTANT_Class &&
					cps[x2]->Info.tag == JVM_CONSTANT_NameAndType )
				{
					if( (x1=cps[x1]->Class.name_idx) < num &&
						(x3=cps[x2]->NameType.desc_idx) < num &&
						(x2=cps[x2]->NameType.name_idx) < num )
					{
						if( cps[x1]->Info.tag == JVM_CONSTANT_Utf8 &&
							cps[x2]->Info.tag == JVM_CONSTANT_Utf8 &&
							cps[x3]->Info.tag == JVM_CONSTANT_Utf8 )
						{
							x1 = jvas_tc_find_callee_map2( tcdef, cps[x1]->Utf8.b, cps[x1]->Utf8.len );
							if( x1 >= 0 )
							{
								x2 = jvas_tc_find_callee2( &tcdef->callee_map[x1], cps[x2]->Utf8.b, cps[x2]->Utf8.len,
															cps[x3]->Utf8.b, cps[x3]->Utf8.len );
								if( x2 >= 0 )
								{
									// record matches
									x2 = _add_callee_name_match( &tcdef->callee_map[x1].callees[x2], &_name_match, &_name_match_num );
									if( x2 < 0 )
										break;
									_name_match[x2]->cpidx = cps[i]->Method.nm_tp_idx;

									x1 = _add_callee_map_match( &tcdef->callee_map[x1], &_callee_map_match, &_callee_map_match_num );
									if( x1 < 0 )
										break;

									_cp_match_num = _add_cpitem_match( (((UINT64)x1<<48) | ((UINT64)x2<<32) | i), &_cp_match, _cp_match_num );
									if( _cp_match_num < 0 )
										break;
								}
							}
						}
					}
				}
			}
		}
	}

	if( _callee_map_match_num > 0 && _name_match_num > 0 && _cp_match_num > 0 )
	{
		// do changes, append constants, change indexes
		void *ptrs[] = {(void*)_callee_map_match, (void*)_name_match, (void*)_cp_match, (void*)tcdef, (void*)cfmap};
		int nums[] = {_callee_map_match_num, _name_match_num, _cp_match_num};
		ret = _do_changes( ptrs, nums );
	}

	if( _callee_map_match )
		free( _callee_map_match );
	if( _name_match )
		free( _name_match );
	if( _cp_match )
		free( _cp_match );

	return ret;
}

// Function:
int jvas_tc_transform2( JTIENV_PTR jtienv, jvas_tc_def *tcdef,
						const char *kname, char **mtd_mask,
						const juint8 *src, jint srclen,
						juint8 **dst, jint *dstlen,
						const char *outfile,
						CF_ClassFileMap *src_cfmap,
						CF_ClassFileMap *ret_cfmap )
{
	int ln;

	if( !tcdef->stub_cfmap )
	{
		// try to load the stub class file
		if( tcdef->sem && MsemTake(tcdef->sem, WAIT_FOREVER) )
		{
			ln = strlen(tcdef->stub.name);
			strcpy( &tcdef->stub.name[ln], ".class" );  // 在加载配置时已经分配了足够的内存用来存放".class"
			tcdef->stub_cfmap = _load_cfmap( tcdef->stub.name );
			tcdef->stub.name[ln] = 0;

			if( tcdef->stub_cfmap )
			{
				if( jvas_tc_prefix_stub( tcdef->stub_cfmap, NULL ) != tcdef->stub_cfmap->mdtab[0].method_count )
				{
					cf_free_map( tcdef->stub_cfmap );
					free( tcdef->stub_cfmap );
					tcdef->stub_cfmap = NULL;
				}
			}
			MsemGive( tcdef->sem );
		}
		if( !tcdef->stub_cfmap )
		{
			_JVAS_TC_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "%s, failed to load the stub class file \"%s\"\n", __FUNCTION__, tcdef->stub.name );
			return -1;
		}
	}

	//
	ln = tcdef->klass.method_count;
	void **ptr = (void**)malloc( (sizeof(void*) * 4 * ln) + ((sizeof(short)+sizeof(long)) * ln) );
	if( !ptr )
		return -1;

	// try to load the class file
	int r = -1;
	CF_ClassFileMap _cfmap;
	if( !src_cfmap )
	{
		memset( &_cfmap, 0, sizeof(CF_ClassFileMap) );
		CF_MAP_SET_FALG( _cfmap, blksz_abap, 1 );
		CF_MAP_SET_FALG( _cfmap, use_sysheap, CF_MAP_HEAP_DEFAULT );
		CF_MAP_LOCAL_ENDIAN_IDX( _cfmap );
		CF_MAP_LOCAL_ENDIAN_VAL( _cfmap );
		if( cf_buffer2map( src, srclen, &_cfmap, CF_FORMAT_BIN ) != 0 )
		{
			_JVAS_TC_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "%s, failed to load the class file \"%s\"\n", __FUNCTION__, kname );
			free( ptr );
			return -1;
		}
		src_cfmap = &_cfmap;
	}

	//
	char *_mtd_mask = NULL;
	if( mtd_mask && *mtd_mask == NULL )
	{
		//_mtd_mask = cf_malloc( src_cfmap->auxi.use_sysheap, &src_cfmap->auxi.mmphd,  src_cfmap->mdtab[0].method_count+1 );
		_mtd_mask = malloc( (r=src_cfmap->mdtab[0].method_count)+1 );
		if( _mtd_mask )
		{
			memset( _mtd_mask, '0', r );
			_mtd_mask[r] = 0;
			*mtd_mask = _mtd_mask;
		}
	}

	if(1)  //if( (src_cfmap->access.ABSTRACT /*| src_cfmap->access.INTERFACE*/) == 0 )
	{
		struct jvas_class_method *mtd = tcdef->klass.method_table;
		struct jvas_class_method *mtd2 = tcdef->stub.method_table;

		const char **methods_name = (const char**)ptr;
		const char **methods_desc = &methods_name[ln];
		const char **stubs_name   = &methods_desc[ln];
		const char **stubs_desc   = &stubs_name[ln];
		char *flags = (char*)&stubs_desc[ln];
		char *mask = (char*)&flags[ln];
		long *tpix = (long*)&mask[ln];

		int i;
		for( i=0; i < ln; i++ )
		{
			methods_name[i] = mtd[i].desc->name;
			methods_desc[i] = mtd[i].desc->sign;
			stubs_name[i] = mtd2[i].desc->name;
			stubs_desc[i] = mtd2[i].desc->sign;
			flags[i] = mtd[i].desc->dis;
			if( !flags[i] )
			{
				mask[i] = 0;
				flags[i] = (mtd[i].desc->mod & 0x70) | (mtd[i].desc->flag & 0x08);
#if 0
				if( mtd[i].desc->ctxid > 0 )
				{
					// has occupied one, reuse it
					tpix[i] = mtd[i].desc->ctxid;
				}
				else
				{
					// allocate new one
					tpix[i] = __sync_add_and_fetch( &g_jvas_tc_tpix_base, 1 );
					((struct jvas_method_desc*)mtd[i].desc)->ctxid = tpix[i];
				}
#else
				char _c, *cc = strrchr( methods_desc[i], JVM_SIGNATURE_ENDFUNC );
				if( cc )
				{
					// ignore "return type"
					_c = cc[1];
					cc[1] = 0;
				}

				tpix[i] = vt_tpix_query( methods_name[i], methods_desc[i], kname, NULL );

				if( cc )
					cc[1] = _c;

				if( tpix[i] <= 0 )
				{
					// allocate a new index for this method
					tpix[i] = __sync_add_and_fetch( &g_jvas_tc_tpix_base, 1 );
					mask[i] = 1;
				}
#endif
			}
			else
				tpix[i] = -1;
		}

		if( cf_transform_class( methods_name, methods_desc, ln, stubs_name, stubs_desc, flags, tpix,
								kname, _mtd_mask, src_cfmap, tcdef->stub_cfmap ) > 0 )
		{
			if( tcdef->callee_map_num > 0 )
			{
				// re-mapping callee
				jvas_tc_remap_callee( tcdef, src_cfmap );
			}

			//
			ssize_t sz = -cf_map2buffer( src_cfmap, NULL, -1, CF_FORMAT_BIN );
			if( sz > 0 )
			{
				void *buf = NULL;
				if( jtienv )
					JTI_Allocate( jtienv, sz, &buf );  // managed by JVM
				//else
				//	buf = malloc( sz );

				if( buf )
				{
					cf_map2buffer( src_cfmap, buf, sz, CF_FORMAT_BIN );

					if( outfile && outfile[0] )
					{
						FILE *hf = fopen( outfile, "wb" );
						if( hf )
						{
							fwrite( buf, 1, sz, hf );
							fclose( hf );
						}
						else
							_JVAS_TC_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "%s, failed to create file \"%s\"\n", __FUNCTION__, outfile );
					}

					*dst = buf;
					*dstlen = (jint)sz;
					r = 0;

					_JVAS_TC_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT,
							"%s, succeed to transform the class file \"%s\", with the stub \"%s\", seq=%ld, new size=%zd, original size=%d\n",
							__FUNCTION__, kname, tcdef->stub.name,  __sync_add_and_fetch(&g_transform_sequence, 1), sz, srclen );

#ifndef _EXE
					// synchronizing TPIX with the VTrace
#if _JVAS_TC_TPIDX_MANAGE_EN == TRUE
					_sync_tpix( kname, tcdef->ifname, ln,
								tpix, methods_name, methods_desc, flags, mask );
#else
					_sync_tpix( kname, (strcmp(kname, tcdef->klass.name) == 0 ? NULL : tcdef->klass.name), ln,
								tpix, methods_name, methods_desc, flags, mask );
#endif
#endif
				}
				else
					_JVAS_TC_DEBUG_PRINT_( stdout, SABI_JOURNAL_CRIT, "%s, failed to allocate memory, errno=%d\n", __FUNCTION__, errno );
			}
			else
				_JVAS_TC_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "%s, cf_map2buffer failed, \"%s\"\n", __FUNCTION__, kname );
		}
		else
			_JVAS_TC_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "%s, nothing to be transformed, \"%s\"\n", __FUNCTION__, kname );
	}
	else
		_JVAS_TC_DEBUG_PRINT_( stdout, SABI_JOURNAL_INFO, "%s, nothing to do because it is Interface or Abstract, \"%s\"\n", __FUNCTION__, kname );

	if( ret_cfmap )
		memcpy( ret_cfmap, src_cfmap, sizeof(CF_ClassFileMap) );
	else if( src_cfmap == &_cfmap )
		cf_free_map( &_cfmap );

	free( ptr );
	return r;
}


// Function: only re-mapping callee
int jvas_tc_transform3( JTIENV_PTR jtienv, jvas_tc_def *tcdef,
						const juint8 *src, jint srclen,
						juint8 **dst, jint *dstlen, const char *outfile,
						CF_ClassFileMap *src_cfmap, CF_ClassFileMap *ret_cfmap )
{
	// try to load the class file
	int ret = -1;
	CF_ClassFileMap _cfmap;

	if( !src_cfmap )
	{
		memset( &_cfmap, 0, sizeof(CF_ClassFileMap) );
		CF_MAP_SET_FALG( _cfmap, blksz_abap, 1 );
		CF_MAP_SET_FALG( _cfmap, use_sysheap, CF_MAP_HEAP_DEFAULT );
		CF_MAP_LOCAL_ENDIAN_IDX( _cfmap );
		CF_MAP_LOCAL_ENDIAN_VAL( _cfmap );
		if( cf_buffer2map( src, srclen, &_cfmap, CF_FORMAT_BIN ) != 0 )
		{
			_JVAS_TC_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "%s, failed to load the class file \"%s\"\n", __FUNCTION__, tcdef->klass.name );
			return -1;
		}
		src_cfmap = &_cfmap;
	}

	if(1)  //if( (src_cfmap->access.ABSTRACT /*| src_cfmap->access.INTERFACE*/) == 0 )
	{
		// re-mapping callee
		if( jvas_tc_remap_callee( tcdef, src_cfmap ) > 0 )
		{
			ssize_t sz = -cf_map2buffer( src_cfmap, NULL, -1, CF_FORMAT_BIN );
			if( sz > 0 )
			{
				void *buf = NULL;
				if( jtienv )
					JTI_Allocate( jtienv, sz, &buf );  // managed by JVM
				else
					buf = malloc( sz );

				if( buf )
				{
					cf_map2buffer( src_cfmap, buf, sz, CF_FORMAT_BIN );

					if( outfile && outfile[0] )
					{
						FILE *hf = fopen( outfile, "wb" );
						if( hf )
						{
							fwrite( buf, 1, sz, hf );
							fclose( hf );
						}
						else
							_JVAS_TC_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "%s, failed to create file \"%s\"\n", __FUNCTION__, outfile );
					}

					*dst = buf;
					*dstlen = (jint)sz;
					ret = 0;

					_JVAS_TC_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT,
							"%s, succeed to re-mapping callee of the class file \"%s\", new class file size=%zd\n",
							__FUNCTION__, tcdef->klass.name, sz );
				}
				else
					_JVAS_TC_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "%s, failed to allocate memory, errno=%d\n", __FUNCTION__, errno );
			}
			else
				_JVAS_TC_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "%s, cf_map2buffer failed\n", __FUNCTION__ );
		}
		else
			_JVAS_TC_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "%s, nothing to be re-mapping\n", __FUNCTION__ );

		if( ret_cfmap )
			memcpy( ret_cfmap, src_cfmap, sizeof(CF_ClassFileMap) );
		else if( src_cfmap == &_cfmap )
			cf_free_map( &_cfmap );
	}

	return ret;
}


#ifndef _EXE
int jvas_tc_transform( JTIENV_PTR jtienv, const char *name, const juint8 *src, jint srclen, juint8 **dst, jint *dstlen )
{
	int i, r;
	const CP_InfoUnion *cpite;
	cf_idx_t idx;
	jvas_tc_def *tcdef;
	char *mtd_mask = NULL;
	char *_name_ = NULL;
	CF_ClassFileMap cf_map = {.iftab_count = 0, .auxi.mmphd=0};

	if( !name )
	{
		// name is null sometimes
		// try to get class name at first

		// load class file
		if( _load_cfmap2( src, srclen, &cf_map, TRUE ) != 0 )
			return -1;

		// get class name
		if( cf_map.this_class < cf_map.cptab[0].cp_count &&
			cf_map.cptab[0].cps[cf_map.this_class]->tag == JVM_CONSTANT_Class )
		{
			i = ((CP_Class*)cf_map.cptab[0].cps[cf_map.this_class])->name_idx;
			if( i < cf_map.cptab[0].cp_count &&
				cf_map.cptab[0].cps[i]->tag == JVM_CONSTANT_Utf8 )
			{
				cpite = (CP_InfoUnion*)cf_map.cptab[0].cps[i];
				_name_ = malloc( cpite->Utf8.len + 1 );
				if( _name_ )
				{
					strncpy( _name_, cpite->Utf8.b, cpite->Utf8.len );
					_name_[cpite->Utf8.len] = 0;
					name = _name_;
				}
			}
		}

		if( !name )
		{
			cf_free_map( &cf_map );
			return -1;
		}
	}

	//
	r = -1;
	tcdef = (jvas_tc_def*)jvas_tc_find_define( &g_jvas_tc_base, name, FALSE, FALSE );  // DO NOT lock
	if( tcdef )
	{
		if( tcdef->klass.bits.dis )
			goto LAB_EXIT;

		//char _temp_name_[512];
		//snprintf( _temp_name_, 512, "tc_log/%s_%s.class", strrchr(name, '/')+1, tcdef->klass_ident );  // FOR DEBUG

		if( tcdef->klass.method_count > 0 )
			r = jvas_tc_transform2( jtienv, tcdef, name, &mtd_mask, src, srclen, dst, dstlen,
									NULL /*_temp_name_*/, NULL, (cf_map.auxi.mmphd == 0 ? &cf_map : NULL) );
		else if( tcdef->callee_map_num > 0 )
			r = jvas_tc_transform3( jtienv, tcdef, src, srclen, dst, dstlen, NULL, NULL,
									(cf_map.auxi.mmphd == 0 ? &cf_map : NULL) );
		if( r != 0 )
			goto LAB_EXIT;

		// try to next stage
		src = *dst;
		srclen = *dstlen;
	}

	if( cf_map.auxi.mmphd == 0 )
		_load_cfmap2( src, srclen, &cf_map, TRUE );  // load class file

	if( cf_map.iftab_count > 0 )  //&& (cf_map.access.ABSTRACT /*| cf_map.access.INTERFACE*/) == 0 )
	{
		// try to find definition by interface-name if possible
		for( i=cf_map.iftab[0].if_count-1; i >= 0 ; i-- )
		{
			idx = cf_map.iftab[0].ifs[i];
			if( idx >= cf_map.cptab[0].cp_count ||
				(cpite=(CP_InfoUnion*)cf_map.cptab[0].cps[idx])->Info.tag != JVM_CONSTANT_Class )
				continue;

			idx = cpite->Class.name_idx;
			if( idx >= cf_map.cptab[0].cp_count ||
				(cpite=(CP_InfoUnion*)cf_map.cptab[0].cps[idx])->Info.tag != JVM_CONSTANT_Utf8 )
				continue;
			//
			tcdef = (jvas_tc_def*)jvas_tc_find_define_2( &g_jvas_tc_base, cpite->Utf8.b, cpite->Utf8.len, FALSE, FALSE );  // DO NOT lock
			if( tcdef && !tcdef->klass.bits.dis )
			{
#if _JVAS_TC_TPIDX_MANAGE_EN == TRUE
				// duplicate transform-definition at first
				tcdef = jvas_tc_dup_def( &g_jvas_tc_base, tcdef, name );
				if( !tcdef )
					break;
#endif
				//char _temp_name_[512];
				//snprintf( _temp_name_, 512, "tc_log/%s_%s_%d.class", strrchr(name, '/')+1, tcdef->klass_ident, i );  // FOR DEBUG

				*dst = NULL;
				*dstlen = 0;
				if( tcdef->klass.method_count > 0 )
					r = jvas_tc_transform2( jtienv, tcdef, name, &mtd_mask, src, srclen, dst, dstlen, NULL /*_temp_name_*/, NULL, NULL );
				else if( tcdef->callee_map_num > 0 )
					r = jvas_tc_transform3( jtienv, tcdef, src, srclen, dst, dstlen, NULL, NULL, NULL );
				if( r != 0 )
					break;
				src = *dst;
				srclen = *dstlen;
			}
		}
	}

	if( r != -1 && *dstlen > 0 )
	{
		// register class name, mark it as "transformed"
		//if( MsemTake(g_jvas_tc_base.sem, WAIT_FOREVER) )
		{
			ssize_t n = strlen( name );
			i = jvas_tc_transformed_rank( name );
			if( NULL == symtab_find(&g_jvas_tc_base.class_transformed[i], name, n) )
			{
				symtab_add( &g_jvas_tc_base.class_transformed[i], name, n );
				__sync_fetch_and_add( &g_jvas_tc_base.count_class_transformed, 1 );
				//printf( "*_* %zd, %s: %s\n", g_jvas_tc_base.count_class_transformed, name, mtd_mask );
			}
			//MsemGive( g_jvas_tc_base.sem );
		}
	}

	LAB_EXIT:
	if( cf_map.auxi.mmphd != 0 )
		cf_free_map( &cf_map );

	if( mtd_mask )
		free( mtd_mask );

	if( _name_ )
		free( _name_ );

	return r;
}

//
void jvas_tc_restore( const char *name )
{
	//if( MsemTake(g_jvas_tc_base.sem, WAIT_FOREVER) )
	{
		ssize_t n = strlen( name );
		int i = jvas_tc_transformed_rank( name );
		if( symtab_find(&g_jvas_tc_base.class_transformed[i], name, n) )
		{
			__sync_fetch_and_sub( &g_jvas_tc_base.count_class_transformed, 1 );
			//printf( "@^@ %zd, %s\n", g_jvas_tc_base.count_class_transformed, name );
		}
		//MsemGive( g_jvas_tc_base.sem );
	}
}
#endif


// Function:　ask JVM to re-transform classes specified by the "tcb"
int jvas_tc_retransform_classes( JTIENV_PTR jtienv, JNIENV_PTR jnienv, const jvas_tc_base *tcb, BOOL load_class, BOOL lazy )
{
	int ret = -1;

	if( tcb->lnk.num > 0 && MsemTake(tcb->sem, WAIT_FOREVER) )
	{
		jclass *cls = (jclass*)malloc( sizeof(jclass) * tcb->lnk.num );
		if( cls )
		{
			jint ld_cls_num;
			char **ld_cls_sign = NULL;
			jclass *ld_cls = NULL;
			jvmtiError er = JNICall( jtienv, GetLoadedClasses, &ld_cls_num, &ld_cls );
			if( er == JVMTI_ERROR_NONE && ld_cls )
			{
				// count interesting classes
				jint ncls = 0;
				jvas_tc_def *tcdef = (jvas_tc_def*)tcb->lnk.head;
				for( ; tcdef; tcdef=(jvas_tc_def*)tcdef->node.next )
				{
					if( lazy )
					{
						if( !tcdef->klass.bits.lazy )
							continue;
						tcdef->klass.bits.dis = 0;
					}

					if( tcdef->klass.bits.dis )
						continue;

					// first, try to find the class by name
					cls[ncls] = load_class ? JNICall( jnienv, FindClass, tcdef->klass.name ) : NULL;
					if( cls[ncls] )
					{
						ncls++;
						_JVAS_TC_DEBUG_PRINT_( stdout, SABI_JOURNAL_INFO, "try to retransform the class \"%s\"\n", tcdef->klass.name );
					}
					else
					{
						// second, try to match signature of loaded classes
						int i, nmln = strlen(tcdef->klass.name);
						char *sign, *name=tcdef->klass.name;

						if( !ld_cls_sign )
						{
							// try to load signatures of all loaded classes
							ld_cls_sign = malloc( sizeof(char*)*ld_cls_num );
							if( !ld_cls_sign )
								break;  // exit while
							memset( ld_cls_sign, 0, sizeof(char*)*ld_cls_num );

							for( i=0; i < ld_cls_num; i++ )
								JNICall( jtienv, GetClassSignature, ld_cls[i], &ld_cls_sign[i], NULL );
						}

						// match signatures
						for( i=0; i < ld_cls_num; i++ )
						{
							if( ld_cls[i] && (sign=ld_cls_sign[i]) && sign[0] == JVM_SIGNATURE_CLASS )
							{
								if( strncmp(sign+1, name, nmln) == 0 && sign[nmln+1] == JVM_SIGNATURE_ENDCLASS )
								{
									cls[ncls++] = ld_cls[i];
									ld_cls[i] = NULL;
									_JVAS_TC_DEBUG_PRINT_( stdout, SABI_JOURNAL_INFO, "try to retransform the class \"%s\"\n", ld_cls_sign[i] );
									break;
								}
							}
						}

						if( i >= ld_cls_num )
							_JVAS_TC_DEBUG_PRINT_( stdout, SABI_JOURNAL_MT, "%s, undefined class \"%s\"\n", __FUNCTION__, tcdef->klass.name );
					}
				}

				if( ncls > 0 )
				{
					// send request "re-transform"
					_JVAS_TC_DEBUG_PRINT_( stdout, SABI_JOURNAL_EVENT, "%s, retransforming %d\n", __FUNCTION__, ncls );
					ret = JNICall( jtienv, RetransformClasses, ncls, cls );

					// delete local reference explicitly
					for( --ncls; ncls >= 0; --ncls )
						JNICall( jnienv, DeleteLocalRef, cls[ncls] );
				}
				//
				for( --ld_cls_num; ld_cls_num >= 0; --ld_cls_num )
				{
					if( ld_cls[ld_cls_num] )
						JNICall( jnienv, DeleteLocalRef, ld_cls[ld_cls_num] );

					if( ld_cls_sign && ld_cls_sign[ld_cls_num] )
						JTI_Deallocate( jtienv, ld_cls_sign[ld_cls_num] );
				}
				JTI_Deallocate( jtienv, ld_cls );
			}
			free( cls );
		}
		MsemGive( tcb->sem );
	}

	return ret;
}

//
int jvas_tc_retransform_classes2( JTIENV_PTR jtienv, JNIENV_PTR jnienv, const jvas_tc_base *tcb )
{
	int ret = -1;

	if( tcb->count_class_transformed > 0 && MsemTake(tcb->sem, WAIT_FOREVER) )
	{
		jint ld_cls_num;
		jclass *ld_cls = NULL;
		jclass *_cls;
		char *_sign;
		int i, n, m;
		jvmtiError er = JNICall( jtienv, GetLoadedClasses, &ld_cls_num, &ld_cls );
		if( er == JVMTI_ERROR_NONE && ld_cls )
		{
			_cls = malloc( sizeof(jclass) * tcb->count_class_transformed );
			if( _cls )
			{
				for( i=n=0; i < ld_cls_num; i++ )
				{
					if( JNICall( jtienv, GetClassSignature, ld_cls[i], &_sign, NULL ) == JVMTI_ERROR_NONE &&
						_sign && _sign[0] == JVM_SIGNATURE_CLASS )
					{
						if( symtab_find(&tcb->class_transformed[jvas_tc_transformed_rank(_sign)], &_sign[1], strlen(_sign)-2) )
						{
							_cls[n++] = ld_cls[i];
							_JVAS_TC_DEBUG_PRINT_( stdout, SABI_JOURNAL_INFO, "try to retransform the class \"%s\"\n", _sign );
						}
					}
					JTI_Deallocate( jtienv, _sign );
				}
				printf( ">>>>>>>>>>>>>>> RETRANSFORM ALL CLASSES, %d/%d\n", n, ld_cls_num );
				//ret = JNICall( jtienv, RetransformClasses, n, _cls );

				for( i=0; i < n; i+=m )
				{
					if( (m = n - i) > 100 )
						m = 100;
					JNICall( jtienv, RetransformClasses, m, &_cls[i] );
				}
				free( _cls );
			}
			//
			for( --ld_cls_num; ld_cls_num >= 0; --ld_cls_num )
				JNICall( jnienv, DeleteLocalRef, ld_cls[ld_cls_num] );
			JTI_Deallocate( jtienv, ld_cls );
		}
		MsemGive( tcb->sem );
	}

	return ret;
}

//
int jvas_tc_retransform_all_classes( JTIENV_PTR jtienv, JNIENV_PTR jnienv, const jvas_tc_base *tcb )
{
	int ret = -1;

	if( MsemTake(tcb->sem, WAIT_FOREVER) )
	{
		jint ld_cls_num;
		jclass *ld_cls = NULL;
		jclass *_cls;
		char *_sign;
		int i, n, m;
		jboolean bl;
		jvmtiError er = JNICall( jtienv, GetLoadedClasses, &ld_cls_num, &ld_cls );
		if( er == JVMTI_ERROR_NONE && ld_cls )
		{
			_cls = malloc( sizeof(jclass) * ld_cls_num );
			if( _cls )
			{
				for( i=n=0; i < ld_cls_num; i++ )
				{
					if( !ld_cls[i] ||
						JNICall( jtienv, IsModifiableClass, ld_cls[i], &bl ) != JVMTI_ERROR_NONE ||
						bl == JNI_FALSE )
						continue;
					if( JNICall( jtienv, IsArrayClass, ld_cls[i], &bl ) != JVMTI_ERROR_NONE ||
						bl == JNI_TRUE )
						continue;
					/*if( JNICall( jtienv, IsInterface, ld_cls[i], &bl ) != JVMTI_ERROR_NONE ||
						bl == JNI_TRUE )
						continue;*/
					JNICall( jtienv, GetClassModifiers, ld_cls[i], &m );
					if( (m & (CF_ACC_SYNTHETIC /*| CF_ACC_ABSTRACT | CF_ACC_INTERFACE*/)) )
						continue;
					JNICall( jtienv, GetClassSignature, ld_cls[i], &_sign, NULL );
					if( strstr(_sign, "/LambdaForm$") )
					{
						JTI_Deallocate( jtienv, _sign );
						continue;
					}
					JTI_Deallocate( jtienv, _sign );
					_cls[n++] = ld_cls[i];
				}
				printf( ">>>>>>>>>>>>>>> RETRANSFORM ALL CLASSES, %d/%d\n", n, ld_cls_num );
				//ret = JNICall( jtienv, RetransformClasses, n, _cls );

				for( i=0; i < n; i+=m )
				{
					if( (m = n - i) > 100 )
						m = 100;
					JNICall( jtienv, RetransformClasses, m, &_cls[i] );
				}
				free( _cls );
			}
			//
			for( --ld_cls_num; ld_cls_num >= 0; --ld_cls_num )
			{
				if( ld_cls[ld_cls_num] )
					JNICall( jnienv, DeleteLocalRef, ld_cls[ld_cls_num] );
			}
			JTI_Deallocate( jtienv, ld_cls );
		}
		MsemGive( tcb->sem );
	}

	return ret;
}


// Function:
void jvas_tc_print_define( const jvas_tc_def *tcdef )
{
	const char *blsym[] = { "no", "yes" };
	const char *mdsym[] = { "undef", "standard", "anywhere", "mixed" };
	int i, j, n, m;

	printf( "\n\t<CLASS Name=\"%s\" On=\"%s\">\n\t\t"
			"<Package>%s</Package>\n\t\t",
			tcdef->klass_ident, blsym[!tcdef->klass.bits.dis], tcdef->klass.name );
	printf( "<STUB Name=\"%s\">\n\t\t\t"
			"<Package>%s</Package>\n\t\t</STUB>\n", tcdef->stub_ident, tcdef->stub.name );

	struct jvas_class_method *mtd = tcdef->klass.method_table;
	struct jvas_class_method *mtd2 = tcdef->stub.method_table;
	n = tcdef->klass.method_count;
	for( i=0; i < n; i++ )
	{
		printf( "\t\t<METHOD On=\"%s\" Mode=\"%s\" EECM=\"%d\" Flags=\"%x\" Target=\"%s%s\" Stub=\"%s%s\"></METHOD>\n",
				blsym[!mtd[i].desc->dis],
				mdsym[mtd[i].desc->mod & 7],
				(mtd[i].desc->mod>>4) & 7,
				mtd[i].desc->flag,
				mtd[i].desc->name, mtd[i].desc->sign,
				(mtd2[i].desc->name != mtd[i].desc->name?mtd2[i].desc->name:""),
				(mtd2[i].desc->sign != mtd[i].desc->sign?mtd2[i].desc->sign:"")
				);
	}

	//
	jvas_tc_name_map *name_map;
	jvas_tc_callee_map *map = tcdef->callee_map;
	n = tcdef->callee_map_num;
	for( i=0; i < n; i++ )
	{
		name_map = map[i].callees;
		m = map[i].callee_num;

		printf( "\t\t<CALLEE-MAPPING Class=\"%s\" On=\"yes\">\n"
				"\t\t\t<NewClass>%s</NewClass>\n",
				map[i].klass_name, (map[i].new_klass_name ? map[i].new_klass_name : "") );
		for( j=0; j < m; j++ )
		{
			printf( "\t\t\t<Callee On=\"yes\" From=\"%s%s\" To=\"%s\"></Callee>\n",
					name_map[j].name, name_map[j].sign,
					name_map[j].name2 ? name_map[j].name2 : "" );
		}
		printf( "\t\t</CALLEE-MAPPING>\n" );
	}

	printf( "\t</CLASS>\n" );
}


// Function:
void jvas_tc_print( const jvas_tc_base *tcb )
{
	if( MsemTake(tcb->sem, WAIT_FOREVER) )
	{
		printf( "<CLASS-TRANSFORMATION>\n" );

		jvas_tc_def *tcdef = (jvas_tc_def*)tcb->lnk.head;
		while( tcdef )
		{
			jvas_tc_print_define( tcdef );
			tcdef = (jvas_tc_def*)tcdef->node.next;
		}
		printf( "</CLASS-TRANSFORMATION>\n" );

		MsemGive( tcb->sem );
	}
}


// Function: pre-process stub-method for accelerating progress of transformation in future(such as instruction-change, etc.)
CF_MethodInfo* jvas_tc_prefix_stub_mtd( const CF_MethodInfo *stub, ssize_t stub_len,
										CF_ClassFileMap *stub_cfmap,
										ssize_t *new_stub_len, const char *stub_desc )
{
	// Note: assumes byte order of attribute is big endian, others is local endian

	const CF_Attr_Code *stub_code;
	const juint8 *srcbuf[5] = {NULL};
	int srclen[5] = {0};
	juint8 *dstbuf = NULL;
	int dstlen;
	ssize_t len;


	// seek the attribute "Code" of the stub
	stub_code = (CF_Attr_Code*)cf_seek_attrinfo( stub->attr.ite, stub->attr_count, "Code", 4, 0, stub_cfmap );
	if( !stub_code )
		return NULL;

	srcbuf[0] = stub_code->code.byte;
	srclen[0] = stub_code->code_len;
	_CF_ENDIAN_SWAP_U4( srclen[0] );

	// exception table of the target "Code"
	CF_ExpInfo *exptab_ptr = (CF_ExpInfo*)(stub_code->code.byte + srclen[0] + sizeof(cf_count_t));
	cf_count_t exptab_len = *((cf_count_t*)exptab_ptr - 1);
	_CF_ENDIAN_SWAP_COUNT( exptab_len );

	// transform byte-code
	CF_CodeTransformInfo info = {
			.cp_off = 0,
			.btsmtd_off = 0,
			.mtd_name = NULL,
			.mtd_desc = stub_desc,
			.ins_locs = NULL
	};

	dstlen = cf_bytecode_transform_v2( srcbuf, srclen, &dstbuf, 0, exptab_ptr, exptab_len, &info, 1, -1 );  // change "ldc" to "ldc_w"
	if( dstlen <= 0 )
		return NULL;

	// allocate memory for the new stub method
	int diff = dstlen - srclen[0];
	len = stub_len + diff;
	CF_MethodInfo *new_stub = (CF_MethodInfo*)cf_malloc(
					stub_cfmap->auxi.use_sysheap, &stub_cfmap->auxi.mmphd, len+1024 );  // always reserves 1024 bytes for potential transformation
	if( new_stub )
	{
		// pack new method up
		ssize_t m = (ssize_t)stub_code - (ssize_t)stub;
		CF_Attr_Code *attr_code_new = (CF_Attr_Code*)((void*)new_stub + m);  // new method's "Code" attribute
		CF_ExpInfo *exptab_new = (CF_ExpInfo*)(attr_code_new->code.byte + dstlen + sizeof(cf_count_t));  // "Exception Table" of "Code" of new method
		cf_count_t *exptab_new_len = ((cf_count_t*)exptab_new - 1);

		// contents, before the attribute "Code"
		memcpy( (void*)new_stub, (void*)stub, m );

		// head of "Code"
		m = stub_code->attr_len;
		m = _CF_ENDIAN_FLIP_U4( m ) + diff;
		attr_code_new->attr_len = _CF_ENDIAN_FLIP_U4( m );
		attr_code_new->attr_nm_idx = stub_code->attr_nm_idx;
		attr_code_new->max_stack = stub_code->max_stack;
		attr_code_new->max_locals = stub_code->max_locals;

		// byte-code of the "Code"
		attr_code_new->code_len = _CF_ENDIAN_FLIP_U4( dstlen );
		memcpy( attr_code_new->code.byte, dstbuf, dstlen );

		// exception table of the "Code"
		if( (*exptab_new_len = _CF_ENDIAN_FLIP_COUNT( exptab_len )) )
			memcpy( exptab_new, exptab_ptr, (m=sizeof(CF_ExpInfo)*exptab_len) );
		else
			m = 0;

		// remained contents, after the attribute "Code", and attribute table of "Code"

		m += _OFFSET(CF_Attr_Code, exp_tab) + srclen[0];  // m stays at the attr_count of the "Code"

		CF_AttrInfo *code_attr = (CF_AttrInfo*)((void*)stub_code + m + sizeof(cf_count_t));
		cf_count_t code_attr_len = *((cf_count_t*)code_attr - 1);
		_CF_ENDIAN_SWAP_COUNT( code_attr_len );

		// "StackMapTable"
		const CF_Attr_StackMapTable *smt =
					(CF_Attr_StackMapTable*)cf_seek_attrinfo( code_attr, code_attr_len, "StackMapTable", sizeof("StackMapTable")-1, 0, stub_cfmap );
		if( smt )
		{
			int smf_num, *smf_lens, ttlen=0;
			CF_Attr_StackMapFrame_Union** smf_ptr = cf_build_stackmaptab( &smt, 1, &smf_num, &smf_lens );
			if( smf_ptr )
			{
				void *ext_buf = NULL;
				ttlen = cf_alter_stackmaptab( smf_ptr, --smf_num, smf_lens, info.ins_locs, info.ins_locs_num, 0, 0, &ext_buf, NULL );
				if( ttlen > 0 )
				{
					if( ext_buf )
					{
						// length of "StackMapTable" has been changed
						int smt_attr_len = _CF_ENDIAN_FLIP_U4( smt->attr_len );
						ssize_t _ln = ttlen - (smt_attr_len - sizeof(cf_count_t));
						if( _ln > 1024 )
						{
							// reallocate
							void *new_ptr = cf_malloc( stub_cfmap->auxi.use_sysheap, &stub_cfmap->auxi.mmphd, len+_ln );
							if( !new_ptr )
							{
								free( ext_buf );
								free( smf_ptr );
								ttlen = 0;
								goto LAB_BEFORE_EXIT;
							}

							memcpy( new_ptr, new_stub, len );
							cf_free( stub_cfmap->auxi.use_sysheap, &stub_cfmap->auxi.mmphd, new_stub );
							new_stub = new_ptr;
						}
						len += _ln;

						// change the length of "Code"
						_ln += _CF_ENDIAN_FLIP_U4(attr_code_new->attr_len);
						attr_code_new->attr_len = _CF_ENDIAN_FLIP_U4( _ln );

						//
						cf_count_t *new_attr_count = (cf_count_t*)&exptab_new[exptab_len];
						void *ptr_d = &new_attr_count[1];
						const void *ptr_s = (void*)stub_code + m + sizeof( cf_count_t );
						*new_attr_count = *((cf_count_t*)ptr_s - 1);

						// assemble new StackMapTable
						CF_Attr_StackMapTable *new_smt = (CF_Attr_StackMapTable*)ptr_d;
						new_smt->attr_nm_idx = smt->attr_nm_idx;
						new_smt->num_entry = smt->num_entry;
						ttlen += sizeof(cf_count_t);
						new_smt->attr_len = _CF_ENDIAN_FLIP_U4(ttlen);
						ptr_d += sizeof(CF_Attr_StackMapTable);
						// entries
						int i;
						for( i=0; i < smf_num; i++ )
						{
							memcpy( ptr_d, (void*)smf_ptr[i], smf_lens[i] );
							ptr_d += smf_lens[i];
						}

						// copy contents, before the StackMapTable
						_ln = (void*)smt - ptr_s;
						memcpy( ptr_d, ptr_s, _ln );

						// copy contents, after the StackMapTable
						ptr_d += _ln;
						ptr_s = (void*)smt + sizeof(CF_AttrHead) + smt_attr_len;
						_ln = stub_len - ((ssize_t)ptr_s - (ssize_t)stub);
						memcpy( ptr_d, ptr_s, _ln );

						//
						free( ext_buf );
					}
					else
					{
						// length of "StackMapTable" is not changed
						stub_len -= ((ssize_t)stub_code - (ssize_t)stub) + m;
						memcpy( (void*)&exptab_new[exptab_len], (void*)stub_code+m, stub_len );
					}
				}
				free( smf_ptr );
				// CAUTION: DO NOT need to free the smf_lens
			}

			LAB_BEFORE_EXIT:
			if( ttlen == 0 )
			{
				// failed
				cf_free( stub_cfmap->auxi.use_sysheap, &stub_cfmap->auxi.mmphd, new_stub );
				new_stub = NULL;
				//len = 0;
			}
		}
		else
		{
			// without "StackMapTable"
			stub_len -= ((ssize_t)stub_code - (ssize_t)stub) + m;
			memcpy( (void*)&exptab_new[exptab_len], (void*)stub_code+m, stub_len );
		}
		*new_stub_len = len;
	}

	free( dstbuf );
	if( info.ins_locs )
		free( info.ins_locs );

	return new_stub;
}


// Function:
int jvas_tc_prefix_stub( CF_ClassFileMap *stub_cfmap, const char *fn )
{
	int c = (int)stub_cfmap->mdtab[0].method_count - 1;
	CF_MethodInfo **mtds = stub_cfmap->mdtab[0].methods;
	ssize_t *mtd_lens = stub_cfmap->mdtab[0].methods_len;
	CF_MethodInfo *new_mtd;
	ssize_t new_mtd_len;
	int num = 0;

	for( ; c >= 0; c-- )
	{
		new_mtd = jvas_tc_prefix_stub_mtd( mtds[c], mtd_lens[c], stub_cfmap, &new_mtd_len, "" );
		if( new_mtd )
		{
			mtds[c] = new_mtd;
			mtd_lens[c] = new_mtd_len;
			num++;
		}
	}

	if( fn && num > 0 )
	{
		cf_map2file( stub_cfmap, fn, CF_FORMAT_BIN );
	}
	return num;
}
