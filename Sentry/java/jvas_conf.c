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
#include "jvas.h"
#include "sabi.h"
#include "facility/xmlAnalyzer.h"

extern int g_jvas_journal_mask;


#define _IS_SPACE_CHAR(_C_)	\
		( (_C_) == '\r' || (_C_) == '\n' || \
		  (_C_) == '\t' || (_C_) == '\b' || \
		  (_C_) == '\f' || (_C_) == 32 )

#define _IS_SEPA_CHAR(_C_)	( (_C_) == '=' || (_C_) == '\0' || _IS_SPACE_CHAR(_C_) )

//
static int _ParseTextLine( char *txt, const char **argv, int argn )
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
}


//
/*static __attribute__((unused)) const char * _seek_argument( int argc, const char **argv, const char *argname )
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
}*/

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


// Function: load run configures
JVAS_HIDDEN int _jvas_load_config( const char *home, JVAS_Conf *confPtr )
{
	HXML hxml;
	XML_ELEMENT *ele;
	XML_ATTRIBUTE *attr;
	char fnm[1024];


	snprintf( fnm, MAX_PATH, "%s/"_JVAS_CONFPATH"/"_JVAS_CONFNAME, home?home:"." );

	if( xmlLoad(fnm, &hxml) != XML_ERR_OK )
	{
		_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "Can not load configure file \"%s\"\n", fnm );
		return -1;
	}

	if( !xmlGetElement( hxml, "JVAS" ) )
	{
		_JVAS_DEBUG_PRINT_( stdout, SABI_JOURNAL_ERROR, "Can not seek the root node \"JVAS\"\n" );
		goto LAB_ERROR;
	}

	int m, n;

	// RUN
	confPtr->run.mode = 0;
	ele = xmlGetCurSubEle( hxml, "RUN", 0 );
	if( ele )
	{
		// NONTRIVIAL-AGENT
		confPtr->run.nontrival = 2;  // default is "AUTO"
		ele = xmlGetCurSubEle( hxml, "NONTRIVIAL-AGENT", 0 );
		if( ele )
		{
			if( stricmp( ele->Value, "yes" ) == 0 || stricmp( ele->Value, "true" ) == 0 )
				confPtr->run.nontrival = 1;
			else if( stricmp( ele->Value, "no" ) == 0 || stricmp( ele->Value, "false" ) == 0 )
				confPtr->run.nontrival = 0;

			xmlGetParentElement( hxml );
		}

		//
		ele = xmlGetCurSubEle( hxml, "JOURNAL", 0 );
		if( ele )
		{
			char *cc;
			attr = xmlGetEleAttr( ele, "mask" );
			if( attr && attr->Value[0] )
				g_jvas_journal_mask = (int)strtol( attr->Value, &cc, 0 );
			xmlGetParentElement( hxml );
		}

		// MODE
		ele = xmlGetCurSubEle( hxml, "MODE", 0 );
		if( ele )
		{
			attr = xmlGetEleAttr( ele, "distribute" );
			if( attr && stricmp(attr->Value, "true")==0 )
				confPtr->run.mode |= SABI_MODE_DISTRIBUTED;

			attr = xmlGetEleAttr( ele, "independent" );
			if( attr && stricmp(attr->Value, "true")==0 )
				confPtr->run.mode |= SABI_MODE_INDEPENDENT;

			attr = xmlGetEleAttr( ele, "silent" );
			if( attr && stricmp(attr->Value, "true")==0 )
				confPtr->run.mode |= SABI_MODE_SILENT;

			attr = xmlGetEleAttr( ele, "quiet" );
			if( attr && stricmp(attr->Value, "true")==0 )
				confPtr->run.mode |= SABI_MODE_QUIET;

			attr = xmlGetEleAttr( ele, "auto" );
			if( attr && stricmp(attr->Value, "true")==0 )
				confPtr->run.mode |= SABI_MODE_AUTO;

			const char *argv[10];
			n = _ParseTextLine( ele->Value, argv, 10 );
			if( _seek_argument2(n, argv, "TRACE") )
				confPtr->run.mode |= SABI_MODE_TRACE;

			if( _seek_argument2(n, argv, "ASPP") || _seek_argument2(n, argv, "RASP") )
				confPtr->run.mode |= SABI_MODE_ASPP;

			if( _seek_argument2(n, argv, "IAST") || _seek_argument2(n, argv, "TEST") )
				confPtr->run.mode |= SABI_MODE_TEST;

			if( _seek_argument2(n, argv, "OFG") )
				confPtr->run.mode |= SABI_MODE_OFG;

			if( _seek_argument2(n, argv, "DFG") )
				confPtr->run.mode |= SABI_MODE_DFG;

			xmlGetParentElement( hxml );
		}

		// JAVA
		ele = xmlGetCurSubEle( hxml, "JAVA", 0 );
		if( ele )
		{
			ele = xmlGetCurSubEle( hxml, "ClassPath", 0 );
			if( ele )
			{
				if( (n=strlen(ele->Value)) < sizeof(confPtr->run.java.classpath) )
					strcpy( confPtr->run.java.classpath, ele->Value );
				xmlGetParentElement( hxml );
			}

			ele = xmlGetCurSubEle( hxml, "LibraryPath", 0 );
			if( ele )
			{
				if( (n=strlen(ele->Value)) < sizeof(confPtr->run.java.libpath) )
					strcpy( confPtr->run.java.libpath, ele->Value );
				xmlGetParentElement( hxml );
			}
			xmlGetParentElement( hxml );
		}

		//
		ele = xmlGetCurSubEle( hxml, "SENTRY-LIBPATH", 0 );
		if( ele )
		{
			ele = xmlGetCurSubEle( hxml, "JVAS", 0 );
			if( ele )
			{
				if( (n=strlen(ele->Value)) < sizeof(confPtr->run.jvas_libpath) )
					strcpy( confPtr->run.jvas_libpath, ele->Value );
				xmlGetParentElement( hxml );
			}

			ele = xmlGetCurSubEle( hxml, "JVAS-SWI", 0 );
			if( ele )
			{
				if( (n=strlen(ele->Value)) < sizeof(confPtr->run.jvas_swi_libpath) )
					strcpy( confPtr->run.jvas_swi_libpath, ele->Value );
				xmlGetParentElement( hxml );
			}

			xmlGetParentElement( hxml );
		}
		xmlGetParentElement( hxml );
	}

	// ESSENTIAL-JAVA-CLASSES
	ele = xmlGetCurSubEle( hxml, "ESSENTIAL-JAVA-CLASSES", 0 );
	if( ele )
	{
		attr = xmlGetEleAttr( ele, "package" );
		if( attr && (n=strlen(attr->Value)) < sizeof(confPtr->esse_class.package) )
		{
			strcpy( confPtr->esse_class.package, attr->Value );
			if( n > 0 && n < NAME_MAX && confPtr->esse_class.package[n-1] != '/' )
			{
				confPtr->esse_class.package[n++] = '/';
				confPtr->esse_class.package[n] = '\0';
			}
		}

		attr = xmlGetEleAttr( ele, "home" );
		if( attr && (n=strlen(attr->Value)) < sizeof(confPtr->esse_class.home) )
		{
			strcpy( confPtr->esse_class.home, attr->Value );
			if( n > 0 && n < NAME_MAX && confPtr->esse_class.home[n-1] != '/' )
			{
				confPtr->esse_class.home[n++] = '/';
				confPtr->esse_class.home[n] = '\0';
			}
		}

		//
		confPtr->esse_class.classes = NULL;
		confPtr->esse_class.count = 0;
		m = xmlGetCurSubEleNum( hxml, "CLASS" );
		if( m > 0 )
		{
			if( m > _JVAS_CONF_ESSE_CLASS_MAX )
				m = _JVAS_CONF_ESSE_CLASS_MAX;
			confPtr->esse_class.classes = malloc( sizeof(struct jvas_class_conf) * m );
			if( confPtr->esse_class.classes )
			{
				memset( confPtr->esse_class.classes, 0, sizeof(struct jvas_class_conf) * m );

				int ln, s;
				struct jvas_class_conf tmpcls = {.name=NULL};

				for( n=s=0; n < m; n++ )
				{
					if( (ele=xmlGetCurSubEle( hxml, "CLASS", n )) )
					{
						if( (ln = strlen( ele->Value )) > 0 )
						{
							if( !(confPtr->esse_class.classes[s].name = malloc( ln+1 )) )
								goto LAB_ERROR;
							strcpy( confPtr->esse_class.classes[s].name, ele->Value );

							attr = xmlGetEleAttr( ele, "must" );
							confPtr->esse_class.classes[s].bits.must = (!attr || stricmp(attr->Value, "true")==0);

							attr = xmlGetEleAttr( ele, "sabi" );
							confPtr->esse_class.classes[s].bits.sabi = (attr && stricmp(attr->Value, "true")==0);

							attr = xmlGetEleAttr( ele, "define-if-possible" );
							confPtr->esse_class.classes[s].bits.defi = (attr && stricmp(attr->Value, "true")==0);

							attr = xmlGetEleAttr( ele, "main" );
							if( attr && stricmp(attr->Value, "true")==0 )
							{
								confPtr->esse_class.classes[s].bits.main = 1;
								if( n > 0 && tmpcls.name == NULL )
								{
									// swap to the head of class table(keep it at the head)
									tmpcls = confPtr->esse_class.classes[0];
									confPtr->esse_class.classes[0] = confPtr->esse_class.classes[s];
									confPtr->esse_class.classes[s] = tmpcls;
								}
							}
							s++;
						}
						xmlGetParentElement( hxml );
					}
				}// for
				confPtr->esse_class.count = s;
			}// if( confPtr->esse_class.classes )
		}
		xmlGetParentElement( hxml );
	}


	// CLASS-LOADER-SEARCH
	confPtr->class_loader.jars = NULL;
	confPtr->class_loader.jar_count = 0;
	ele = xmlGetCurSubEle( hxml, "CLASS-LOADER-SEARCH", 0 );
	if( ele )
	{
		// JAR
		m = xmlGetCurSubEleNum( hxml, "JAR" );
		if( m > 0 )
		{
			if( m > _JVAS_CONF_LDSRC_JAR_MAX )
				m = _JVAS_CONF_LDSRC_JAR_MAX;
			confPtr->class_loader.jars = malloc( sizeof(struct jvas_jar_conf) * m );
			if( confPtr->class_loader.jars )
			{
				memset( confPtr->class_loader.jars, 0, sizeof(struct jvas_jar_conf) * m );

				int ln, s;
				for( n=s=0; n < m; n++ )
				{
					if( (ele=xmlGetCurSubEle( hxml, "JAR", n )) )
					{
						if( (ln = strlen( ele->Value )) > 0 )
						{
							confPtr->class_loader.jars[s].name = malloc( ln+1 );
							if( !confPtr->class_loader.jars[s].name )
								goto LAB_ERROR;
							strcpy( confPtr->class_loader.jars[s].name, ele->Value );

							attr = xmlGetEleAttr( ele, "must" );
							confPtr->class_loader.jars[s].bits.must = (!attr || stricmp(attr->Value, "true")==0);

							attr = xmlGetEleAttr( ele, "Bootstrap" );
							confPtr->class_loader.jars[s].bits.bootld = (!attr || stricmp(attr->Value, "true")==0);

							attr = xmlGetEleAttr( ele, "System" );
							confPtr->class_loader.jars[s].bits.sysld = (!attr || stricmp(attr->Value, "true")==0);

							s++;
						}
						xmlGetParentElement( hxml );
					}
				}
				confPtr->class_loader.jar_count = s;
			}
		}
		xmlGetParentElement( hxml );
	}


	// CLASS-TRANSFORM
	ele = xmlGetCurSubEle( hxml, "CLASS-TRANSFORM", 0 );
	if( ele )
	{
		attr = xmlGetEleAttr( ele, "define-home" );
		if( attr && (n=strlen(attr->Value)) < sizeof(confPtr->tc.home) )
		{
			strcpy( confPtr->tc.home, attr->Value );
			if( n > 0 && n < NAME_MAX && confPtr->tc.home[n-1] != '/' )
			{
				confPtr->tc.home[n++] = '/';
				confPtr->tc.home[n] = '\0';
			}
		}

		attr = xmlGetEleAttr( ele, "preload" );
		confPtr->tc.preload = !attr || stricmp(attr->Value, "true")==0;

		attr = xmlGetEleAttr( ele, "delay" );
		confPtr->tc.delay = attr ? strtol(attr->Value, NULL, 0) : 0;

		xmlGetParentElement( hxml );
	}

	xmlUnload( hxml );
	return 0;

	LAB_ERROR:
	xmlUnload( hxml );
	return -1;
}


// Function: free resource of configures pointed to by the confPtr
JVAS_HIDDEN void _jvas_free_conf( JVAS_Conf *confPtr )
{
	int n;

	if( confPtr->esse_class.classes )
	{
		for( n=confPtr->esse_class.count-1; n >= 0; n-- )
		{
			if( confPtr->esse_class.classes[n].name )
				free( confPtr->esse_class.classes[n].name );
		}
		free( confPtr->esse_class.classes );
	}

	//
	if( confPtr->class_loader.jars )
	{
		for( n=confPtr->class_loader.jar_count-1; n >= 0; n-- )
		{
			if( confPtr->class_loader.jars[n].name )
				free( confPtr->class_loader.jars[n].name );
		}
		free( confPtr->class_loader.jars );
	}

	memset( confPtr, 0, sizeof(JVAS_Conf) );
}


// Function: print configures
JVAS_HIDDEN void _jvas_print_conf( const JVAS_Conf *confPtr )
{
	int n, m;
	char sep[][2] = {",", ""};
	char blsym[][6] = {"True", "False"};
	//char flgsym[][24] = {"", "must", "main", "define-if-possible", "bootstrap", "system"};
	char str[100];

	n = 0;
	if( confPtr->run.mode & SABI_MODE_TRACE )
		strcpy( str+n, "TRACE+" ), n += 6;
	if( confPtr->run.mode & SABI_MODE_ASPP )
		strcpy( str+n, "ASPP+" ), n += 8;
	if( confPtr->run.mode & SABI_MODE_OFG )
		strcpy( str+n, "OFG+" ), n += 4;
	if( confPtr->run.mode & SABI_MODE_DFG )
		strcpy( str+n, "DFG+" ), n += 4;
	if( confPtr->run.mode & SABI_MODE_TEST )
		strcpy( str+n, "TEST+" ), n += 5;
	str[n-1] = 0;

	printf( "RUN:{\n\tNONTRIVIAL:%s,\n\tJournalMask:%x,\n\tMODE:{behavior:%s, independent:%s, silent:%s, quiet:%s, distribute:%s, auto:%s}\n\t"
			"JAVA:{classpath:%s, libpath:%s}\n\tSENTRY-LIBPATH:{jvas:%s, jvas_swi:%s}\n}\n",
			(confPtr->run.nontrival == 0 ? "No" : confPtr->run.nontrival == 1 ? "Yes" : "Auto"), g_jvas_journal_mask, str,
			blsym[!(confPtr->run.mode&SABI_MODE_INDEPENDENT)], blsym[!(confPtr->run.mode&SABI_MODE_SILENT)],
			blsym[!(confPtr->run.mode&SABI_MODE_QUIET)], blsym[!(confPtr->run.mode&SABI_MODE_DISTRIBUTED)],
			blsym[!(confPtr->run.mode&SABI_MODE_AUTO)],
			confPtr->run.java.classpath, confPtr->run.java.libpath,
			confPtr->run.jvas_libpath, confPtr->run.jvas_swi_libpath );


	//
	printf( "ESSENTIAL-JAVA-CLASSES:{\n\tpackage:%s, \n\tclass_count:%d, \n\tclasses:[\n", confPtr->esse_class.package, (m=confPtr->esse_class.count) );
	for( n=0; n < m; n++ )
		printf( "\t\t{name:%s, must:%s, sabi:%s, main:%s, define-if-possible:%s}%s\n",
				confPtr->esse_class.classes[n].name,
				blsym[!confPtr->esse_class.classes[n].bits.must],
				blsym[!confPtr->esse_class.classes[n].bits.sabi],
				blsym[!confPtr->esse_class.classes[n].bits.main],
				blsym[!confPtr->esse_class.classes[n].bits.defi],
				sep[n+1==m] );
	printf( "\t]\n}\n" );


	//
	printf( "CLASS-LOADER-SEARCH:{JAR_COUNT:%d, JAR:[\n", (m=confPtr->class_loader.jar_count) );
	for( n=0; n < m; n++ )
		printf( "\t{name:%s, bootstrap:%s, system:%s}%s\n", confPtr->class_loader.jars[n].name,
				blsym[!confPtr->class_loader.jars[n].bits.bootld],
				blsym[!confPtr->class_loader.jars[n].bits.sysld],
				sep[n+1==m] );
	printf( "\t]\n}\n" );

	//
	printf( "CLASS-TRANSFORM:{preload:%s, define-home:\"%s\"}\n", blsym[!confPtr->tc.preload], confPtr->tc.home );
}
