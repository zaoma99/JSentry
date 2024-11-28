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
#include <stdlib.h>
#include <string.h>

#include "journal.h"
#include "sys/taskLib.h"
#include "facility/UTIMER.h"


#ifdef ___LINUX___
#define FUNC_HIDDEN 	__attribute__((visibility("hidden")))
#else
#define FUNC_HIDDEN
#endif


#include <sys/ioctl.h>
#include <sys/stat.h>


// change standard input, output, error
FUNC_HIDDEN int ChangeStdioe( const char *_path, const char *prefix, FILE **fdin, FILE **fdout, FILE **fderr, int FIFO )
{
	struct stat fsta;
	int n;
	int pathlen = (_path?strlen(_path):0) + (prefix?strlen(prefix):0) + MAX_PATH;
	char *path = malloc( pathlen + 1 );

	if( !path )
		return -1;

	struct tm t2;
	time_t t1 = time( NULL );
	localtime_r( &t1, &t2 );

	// change stdio, stdout, stderr

	if( fdin )
	{
		// STDIN
		snprintf( path, pathlen, "%s/%sSTDIN_%02d%s", _path, prefix, t2.tm_mday, FIFO?"_FF":"" );
		if( FIFO )
		{
			remove( path );
			mkfifo( path, 0666 );
		}
		*fdin =  freopen( path,  "w+b", *fdin );
	}

	if( fdout )
	{
		// STDOUT
		snprintf( path, pathlen, "%s/%sSTDOUT_%02d%s", _path, prefix, t2.tm_mday, FIFO?"_FF":"" );
		if( FIFO )
		{
			remove( path );
			mkfifo( path, 0666 );
		}
		*fdout = freopen( path, "w+b", *fdout );

		if( fstat( fileno(*fdout), &fsta ) == 0 && (fsta.st_mode & __S_IFIFO) )
		{
			n = 1;
			n = ioctl( fileno(*fdout), FIONBIO, &n );
			setvbuf( *fdout, NULL, _IOLBF, 0 );  // set Line-buffered standard output FIFO, 2021-11-10
		}
	}

	if( fderr )
	{
		// STDERR
		snprintf( path, pathlen, "%s/%sSTDERR_%02d%s", _path, prefix, t2.tm_mday, FIFO?"_FF":"" );
		if( FIFO )
		{
			remove( path );
			mkfifo( path, 0666 );
		}
		*fderr = freopen( path, "w+b", *fderr );

		if( fstat( fileno(*fderr), &fsta ) == 0 && (fsta.st_mode & __S_IFIFO) )
		{
			n = 1;
			n = ioctl( fileno(*fderr), FIONBIO, &n );
		}
	}

	free( path );

	return 0;
}


//
static BOOL BackupLogfile( jour_env_t *env )
{
	char tt[MAX_PATH+1];
	time_t t;
	struct tm t2;
	localtime_r( &t, &t2 );

	snprintf( tt, MAX_PATH, "%s/bkup_day%02d.txt", env->path, t2.tm_mday );

	// 2021-12-28
	if( env->chgfd )
	{
		ChangeStdioe( "", "", NULL, env->hOut ? &env->hOut : NULL, env->hErr ? &env->hErr : NULL, env->fifo );
	}

	return LOG_Backup( env->hLogFile, tt, FALSE );
}


// 内存日志分发任务
FUNC_HIDDEN TASK_RET tJournalDispatch( TASK_ARG arg )
{
	static char txtline[LOG_TXTLINE_MAX+1];
	long tlen;
	jour_env_t *me = (jour_env_t*)arg;
	LOG_MEMHD *hMsg = (LOG_MEMHD*)me->hLogMem;
	FILE *hOut = me->hOut;

#ifdef ___LINUX___
	utTimer tmr_1;

	utStartTimer( &tmr_1, 86400000 );
#else

	UINT32    tmr_1 = 0;
#endif


	fprintf( hOut, "tJournalDispatch begin \n" );

	while( !me->Exit )
	{
		tlen = RQ_Get( &hMsg->hBuf, (UCHAR*)txtline, LOG_TXTLINE_MAX );
		if( tlen > 0 )
		{
			txtline[tlen] = '\0';


#ifdef _INCLUDE_VT
			// output via VT
			VT_Print( VT_INFO_TYPE_RAW, "", "%s", txtline );
#endif

			// output via local standard output device(terminal or file)
			// NOTE: MUST call printf rather than other output function such as fwrite. Because the printf is a multi-thread safe function.
			fprintf( hOut, "%s", txtline );
		}
		else
		{
#ifdef ___LINUX___
			// wait something to print
			LOG_SyncWait( hMsg, 10000 );
			if( me->Exit )
				break;

			utDoTimer( &tmr_1 );
			if( utIsOverflow(&tmr_1) )
#else
			Sleep( MAIN_LOG_FREQUENCY );
			tmr_1 += MAIN_LOG_FREQUENCY;
			if( tmr_1 >= 86400000 )
#endif
			{
				// 备份日志文件( 次/24小时)
				BackupLogfile( me );


#ifdef ___LINUX___
				utStartTimer( &tmr_1, 86400000 );
#else
				tmr_1 = 0;
#endif
			}
		}
	}

	fprintf( hOut, "tJournalDispatch end \n" );
	return 0;
}
