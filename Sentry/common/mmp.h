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

#ifndef _USR_MMP_H_
#define _USR_MMP_H_


#ifdef __cplusplus
extern "C"{
#endif

//#define _MEMPOOL_DEBUG_STATISTIC

__attribute__((visibility("default"))) extern void* mmp_malloc( int sys_mmp, long *mmp, unsigned long size );

__attribute__((visibility("default"))) extern void* mmp_realloc( int sys_mmp, long *mmp, void *ptr, size_t size );

__attribute__((visibility("default"))) extern void mmp_free( int sys_mmp, long *mmp, void *ptr );

__attribute__((visibility("default"))) extern void mmp_free_ex( int sys_mmp, long *mmp, void *ptr, int check );

__attribute__((visibility("default"))) extern int mmp_leave( int sys_mmp, long *mmp, void *ptr );

__attribute__((visibility("default"))) extern int mmp_join( int sys_mmp, long *mmp, void *ptr );

__attribute__((visibility("default"))) extern void mmp_destroy_mempool( int sys_mmp, long *mmphd );

__attribute__((visibility("default"))) extern int mmp_create_mempool( int sys_mmp, long *mmphd );

#ifdef _MEMPOOL_DEBUG_STATISTIC
__attribute__((visibility("default"))) extern size_t mmp_count_size( int sys_mmp, long *mmphd );
#else
#define mmp_count_size(...)	((size_t)0)
#endif


#ifdef __cplusplus
}
#endif

#endif
