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

#ifndef _USR_JOURNAL_H_
#define _USR_JOURNAL_H_

#include <stdio.h>
#include "sys/taskLib.h"
#include "facility/log_recorder.h"


typedef struct
{
	BOOL    Exit;
	LOG_HD *hLogFile;
	LOG_HD *hLogMem;
	FILE   *hOut;
	FILE   *hErr;
	int     fifo :1;
	int     chgfd:1;
	char    path[MAX_PATH+1];
}jour_env_t;

#define JOURNAL_ENV_BACKUP

#ifdef __cplusplus
extern "C"{
#endif


extern TASK_RET tJournalDispatch( TASK_ARG arg );

extern int ChangeStdioe( const char *path, const char *prefix, FILE **fdin, FILE **fdout, FILE **fderr, int FIFO );

#ifdef __cplusplus
}
#endif

#endif
