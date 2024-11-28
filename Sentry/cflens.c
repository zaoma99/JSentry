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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "java/classfile_util.h"
#include "java/cf_bytecode.h"
#include "java/cf_impl.h"
#include "java/jvas_tc.h"
#include "common/sabi.h"
#include "common/vtrace.h"
#include "common/vt_api.h"

#include "facility/xmlAnalyzer.h"



#ifdef _EXE

JVAS_HIDDEN int g_jvas_journal_mask = 0xFF;


#define _IS_SPACE_CHAR(_C_)	\
		( (_C_) == '\r' || (_C_) == '\n' || \
		  (_C_) == '\t' || (_C_) == '\b' || \
		  (_C_) == '\f' || (_C_) == 32 )

#define _IS_SEPA_CHAR(_C_)	( (_C_) == '=' || (_C_) == '\0' || _IS_SPACE_CHAR(_C_) )

//
/*static __attribute__((unused)) int _ParseTextLine( char *txt, char space, char **argv, int argn )
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
		while( *s != 0 && *s != space ) s++;
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


#if 0
static int _test( void )
{
	// FOR DEBUG
	if( !xmlInitiate() )
		return -1;

	jvas_tc_base tcb;
	int r = jvas_tc_base_open( &tcb );

	r = jvas_tc_load( "conf", &tcb );
	jvas_tc_print( &tcb );
	const jvas_tc_def *tcdef = jvas_tc_find_define( &tcb, "String", 1, 1 );
	r = jvas_tc_find_method( tcdef, "String", NULL );
	r = jvas_tc_find_method( tcdef, "concat", "(Ljava/lang/String;)Ljava/lang/String;" );
	jvas_tc_free( &tcb );
	jvas_tc_base_close( &tcb, 0 );
	xmlRelease();
	return 0;
}
#endif

//
static int _transform( CF_ClassFileMap *cfmap, const char *cfgfile, const char *klass_name, long tpix_base, const char *outfile )
{
	if( !xmlInitiate() )
		return -1;

	char *fn;
	char **ptr;
	int ln;
	jvas_tc_base tcb;
	const jvas_tc_def *tcdef;
	int ret = -1;

	if( jvas_tc_base_open( &tcb ) != 0 )
		goto LAB_EXIT0;

	if( jvas_tc_read_def( cfgfile, &tcb ) != 0 )
		goto LAB_EXIT1;

	//jvas_tc_print( &tcb );

	tcdef = jvas_tc_find_define( &tcb, klass_name, 0, 1 );
	if( !tcdef )
	{
		printf( "there are no valid transformation configures\n" );
		goto LAB_EXIT2;
	}

	ln = strlen(tcdef->stub.name) + 10;
	fn = malloc( ln + (sizeof(void*) * 4 * tcdef->klass.method_count + ((sizeof(char)+sizeof(long))*tcdef->klass.method_count)) );
	if( !fn )
	{
		printf( "failed to allocate memory, errno=%d\n", errno );
		goto LAB_EXIT2;
	}
	ptr = (char**)(fn + ln);

	strcpy( fn, tcdef->stub.name );
	strcpy( fn+ln-10, ".class" );

	CF_ClassFileMap cfmap_stub = {0};
	CF_MAP_SET_FALG( cfmap_stub, blksz_abap, 1 );
	CF_MAP_SET_FALG( cfmap_stub, use_sysheap, 1 );
	CF_MAP_LOCAL_ENDIAN_IDX( cfmap_stub );
	CF_MAP_LOCAL_ENDIAN_VAL( cfmap_stub );

	if( cf_file2map( fn, &cfmap_stub, 0 ) == 0 )
	{
		if( jvas_tc_prefix_stub( &cfmap_stub, NULL ) == cfmap_stub.mdtab[0].method_count )
		{
			ln = tcdef->klass.method_count;
			struct jvas_class_method *mtd = tcdef->klass.method_table;
			struct jvas_class_method *mtd2 = tcdef->stub.method_table;

			const char **methods_name = (const char**)ptr;  //[] = {"<init>", "concat"};
			const char **methods_desc = &methods_name[ln];  //[] = {"(Ljava/lang/String;)V", "(Ljava/lang/String;)Ljava/lang/String;"};
			const char **stubs_name   = &methods_desc[ln];  //[] = {"String", "concat"};
			const char **stubs_desc   = &stubs_name[ln];  //[] = {"(Ljava/lang/String;Ljava/lang/String;)V", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;"};
			char *flags = (char*)&stubs_desc[ln];  //[sizeof(methods_name)/sizeof(char*)];
			long *tpix = tpix_base >= 0 ? (long*)&flags[ln] : NULL;

			int i;
			for( i=0; i < ln; i++ )
			{
				methods_name[i] = mtd[i].desc->name;
				methods_desc[i] = mtd[i].desc->sign;
				stubs_name[i] = mtd2[i].desc->name;
				stubs_desc[i] = mtd2[i].desc->sign;
				flags[i] = mtd[i].desc->dis;
				if( !flags[i] )
					flags[i] |= (mtd[i].desc->mod & 0x70) | (mtd[i].desc->flag & 0x08);
				if( tpix )
					tpix[i] = tpix_base++;
			}

			if( cf_transform_class( methods_name, methods_desc, ln, stubs_name, stubs_desc, flags, tpix, klass_name, cfmap, &cfmap_stub ) > 0 )
			{
				if( tcdef->callee_map_num > 0 )
				{
					// re-mapping callee
					jvas_tc_remap_callee( tcdef, cfmap );
				}


				//cf_map2file( &cf_map, "String2.class", CF_FORMAT_BIN );

				ssize_t sz = -cf_map2buffer( cfmap, NULL, -1, CF_FORMAT_BIN );
				if( sz > 0 )
				{
					void *buf = malloc(sz);
					if( buf )
					{
						//ssize_t ln =
						cf_map2buffer( cfmap, buf, sz, CF_FORMAT_BIN );

						if( outfile && outfile[0] )
						{
							FILE *hf = fopen( outfile, "wb" );
							if( hf )
							{
								fwrite( buf, 1, sz, hf );
								fclose( hf );
							}
							else
								printf( "failed to open file \"%s\"\n", outfile );
						}
						ret = 0;

						free(buf);
					}
					else
						printf( "failed to allocate memory, errno=%d\n", errno );
				}
				else
					printf( "cf_map2buffer failed\n" );
			}
			else
				printf( "nothing be transformed\n" );
		}
		else
			printf( "failed to pre-process the stub class file\n" );

		cf_free_map( &cfmap_stub );
	}

	free( fn );

	LAB_EXIT2:
	jvas_tc_free( &tcb );

	LAB_EXIT1:
	jvas_tc_base_close( &tcb, 0 );

	LAB_EXIT0:
	xmlRelease();
	return ret;
}


#ifdef _EXE
static const char _CMDLINE_SYNTAX_[] =
		"usage: -f=class-file {[-a all information] | "
		"[-c=method-name(translate method bytecode)] | "
		"[-t=class transformation configure(used for class transformation)] | "
		"[-k=class full name(used for class transformation)] |"
		"[-s=stub class file verification(pre-process)] |"
		"[-i=ask insert a TPIX] |"
		"[-n=new class file]}\n";

//
int main( int argc, const char **argv )
{
	if( argc < 2 )
		goto LAB_ERROR;


	const char *filepath = _seek_argument( argc, argv, "-f" );
	const char *all =_seek_argument2( argc, argv, "-a" );
	const char *code = _seek_argument2( argc, argv, "-c" );
	const char *transform = _seek_argument( argc, argv, "-t" );
	const char *newfile = _seek_argument( argc, argv, "-n" );
	const char *stub_verify = _seek_argument2( argc, argv, "-s" );
	const char *klass = _seek_argument( argc, argv, "-k" );
	const char *tpix = _seek_argument( argc, argv, "-i" );

	if( !all && !code && !transform || !filepath )
		goto LAB_ERROR;

	CF_ClassFileMap cf_map = {0};
	CF_MAP_SET_FALG( cf_map, blksz_abap, 1 );
	CF_MAP_SET_FALG( cf_map, use_sysheap, 1 );
	CF_MAP_LOCAL_ENDIAN_IDX( cf_map );
	CF_MAP_LOCAL_ENDIAN_VAL( cf_map );

	if( cf_file2map( filepath, &cf_map, CF_FORMAT_BIN ) == 0 )
	{
		if( stub_verify )
		{
			jvas_tc_prefix_stub( &cf_map, newfile );
		}
		else if( transform )
		{
			if( _transform( &cf_map, transform, klass, tpix?strtol(tpix, NULL, 0):-1, newfile ) != 0 )
				exit(-1);
		}

		if( all )
			cf_print_map_ex( &cf_map, stdout );

		if( code )
		{
			code = code[0]=='=' && code[1] ? &code[1] : NULL;
			cf_translate_methodEx( code, &cf_map, stdout );
		}

		cf_free_map( &cf_map );
		exit(0);
	}

	exit(-1);

	LAB_ERROR:
	printf( "\e[1;31m%s\e[0m\n", _CMDLINE_SYNTAX_ );
	exit(-1);
}

#endif  // _EXE

