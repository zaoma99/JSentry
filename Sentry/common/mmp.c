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

#include "mmp.h"
#include "common/error_user.h"

#define MMP_USRMMU

#define _MEMPOOL_INCSIZE	(4096*2)
// #ifdef _MEMPOOL_CHECK_

#ifdef MMP_USRMMU
#include "advLib/usrMMU.h"
#endif

//#pragma GCC diagnostic ignored "-Wunused-value"


//
struct _mmp_mbnode_t
{
	struct _mmp_mbnode_t *next;
	struct _mmp_mbnode_t *prior;
#ifdef _MEMPOOL_CHECK_
	size_t chk;
#endif
#ifdef _MEMPOOL_DEBUG_STATISTIC
	size_t len;
#endif
};

#define _MMP_CHECK_(_NODE_)     (((size_t)_NODE_) ^ 0xabcd095fcdbacc8a)

//
void* mmp_malloc( int sys_mmp, long *mmp, size_t size )
{
	if( sys_mmp )
	{
		if( size == 0 )
			return NULL;

		void *ptr = malloc( size + sizeof(struct _mmp_mbnode_t) );
		if( !ptr )
			return NULL;

		if( mmp )
		{
			struct _mmp_mbnode_t *nd = (struct _mmp_mbnode_t*)ptr;
			nd->next = NULL;
			nd->prior = (void*)*mmp;
#ifdef _MEMPOOL_CHECK_
			nd->chk = _MMP_CHECK_( nd );
#endif

#ifdef _MEMPOOL_DEBUG_STATISTIC
			nd->len = size;// + sizeof(struct _mmp_mbnode_t);
#endif
			if( *mmp )
				((struct _mmp_mbnode_t*)*mmp)->next = ptr;

			*mmp = (long)ptr;  // always pointer to the tail
		}

		return ptr+sizeof(struct _mmp_mbnode_t);
	}

#ifndef MMP_USRMMU
	errno = errUnsupport;
	return NULL;
#else
	return (void*)UM_Alloc( *(UM_ID*)mmp, size );
#endif
}

//
void* mmp_realloc( int sys_mmp, long *mmp, void *orgptr, size_t size )
{
	if( !orgptr )
		return mmp_malloc( sys_mmp, mmp, size );

	if( sys_mmp )
	{
		if( size == 0 )
			return NULL;

		orgptr -= sizeof(struct _mmp_mbnode_t);
		struct _mmp_mbnode_t orgnd = *(struct _mmp_mbnode_t*)orgptr;

		void *ptr = realloc( orgptr, size + sizeof(struct _mmp_mbnode_t) );
		if( !ptr )
			return NULL;

		if( ptr != orgptr && mmp )
		{
			// re-link
			struct _mmp_mbnode_t *nd = (struct _mmp_mbnode_t*)ptr;
			*nd = orgnd;
			if( orgnd.prior )
				orgnd.prior->next = nd;
			if( orgnd.next )
				orgnd.next->prior = nd;

#ifdef _MEMPOOL_CHECK_
			nd->chk = _MMP_CHECK_( nd );
#endif
			if( *mmp == (long)orgptr )
				*mmp = (long)ptr;
		}
#ifdef _MEMPOOL_DEBUG_STATISTIC
		((struct _mmp_mbnode_t*)ptr)->len = size;// + sizeof(struct _mmp_mbnode_t);
#endif
		return ptr+sizeof(struct _mmp_mbnode_t);
	}

#ifndef MMP_USRMMU
	errno = errUnsupport;
	return NULL;
#else
	errno = errUnsupport;
	return NULL;
#endif
}

//
int mmp_join( int sys_mmp, long *mmp, void *ptr )
{
	if( sys_mmp && mmp )
	{
		ptr -= sizeof(struct _mmp_mbnode_t);
		struct _mmp_mbnode_t *nd = (struct _mmp_mbnode_t*)ptr;

#ifdef _MEMPOOL_CHECK_
		if( nd->chk == _MMP_CHECK_( nd ) )
#endif
		{
			nd->next = NULL;
			nd->prior = (void*)*mmp;

			if( *mmp )
				((struct _mmp_mbnode_t*)*mmp)->next = ptr;

			*mmp = (long)ptr;  // always pointer to the tail
			return 0;
		}
		errno = EINVAL;
	}
	else
		errno = errUnsupport;
	return -1;
}

//
int mmp_detach( int sys_mmp, long *mmp, void *ptr )
{
	if( sys_mmp && mmp )
	{
		struct _mmp_mbnode_t *nd = (struct _mmp_mbnode_t*)(ptr - sizeof(struct _mmp_mbnode_t));
#ifdef _MEMPOOL_CHECK_
		if( nd->chk == _MMP_CHECK_( nd ) )
#endif
		{
			if( nd->prior )
				nd->prior->next = nd->next;

			if( nd->next )
				nd->next->prior = nd->prior;
			else	//if( *mmp == (long)nd )
				*mmp = (long)nd->prior;
			return 0;
		}
		errno = EINVAL;
	}
	else
		errno = errUnsupport;
	return -1;
}

//
void mmp_free_ex( int sys_mmp, long *mmp, void *ptr, int check )
{
	if( sys_mmp )
	{
		if( mmp )
		{
			struct _mmp_mbnode_t *_nd = (struct _mmp_mbnode_t*)*mmp;
			struct _mmp_mbnode_t *nd = (struct _mmp_mbnode_t*)(ptr - sizeof(struct _mmp_mbnode_t));

			if( check )
				while( _nd && _nd != nd ) _nd = _nd->prior;  // check node existing

			if( _nd )
			{
#ifdef _MEMPOOL_CHECK_
				if( _nd->chk == _MMP_CHECK_( _nd ) )
#endif
				{
					if( nd->prior )
						nd->prior->next = nd->next;

					if( nd->next )
						nd->next->prior = nd->prior;
					else	//if( *mmp == (long)nd )
						*mmp = (long)nd->prior;

					free( nd );
				}
#ifdef _MEMPOOL_CHECK_
				else
					printf( "%s: invalid checksum\n", __FUNCTION__ );
#endif
			}
			else
				printf( "%s: invalid pointer, errno=%d\n", __FUNCTION__, errno );
		}
		else
			free( ptr - sizeof(struct _mmp_mbnode_t) );
	}
	else
	{
#ifndef MMP_USRMMU
		errno = errUnsupport;
#else
		UM_Free( (UM_ID)mmp, ptr );
#endif
	}
}

//
void mmp_free( int sys_mmp, long *mmp, void *ptr )
{
	mmp_free_ex( sys_mmp, mmp, ptr, 0 );
}


//
void mmp_destroy_mempool( int sys_mmp, long *mmphd )
{
	if( mmphd && *mmphd )
	{
		if( sys_mmp )
		{
			struct _mmp_mbnode_t *nd2, *nd = (struct _mmp_mbnode_t*)*mmphd;
			while( nd )
			{
				nd2 = nd->prior;
				free( nd );
				nd = nd2;
			}
		}
#ifdef MMP_USRMMU
		else
			UM_DeletePool( *mmphd );
#endif

		*mmphd = 0;
	}
}


#ifdef _MEMPOOL_DEBUG_STATISTIC
//
size_t mmp_count_size( int sys_mmp, long *mmphd )
{
	if( mmphd && *mmphd )
	{
		size_t len_sum = 0;

		if( sys_mmp )
		{
			struct _mmp_mbnode_t *nd = (struct _mmp_mbnode_t*)*mmphd;
			while( nd )
			{
				len_sum += nd->len;
				nd = nd->prior;
			}
		}
#ifdef MMP_USRMMU
		else
			;
#endif
		return len_sum;
	}
	return -1;
}
#endif


//
int mmp_create_mempool( int sys_mmp, long *mmphd )
{
	if( sys_mmp )
	{
		// delete all nodes have been linked in the sys_mem_pool
		mmp_destroy_mempool( 1, mmphd );
		return 0;
	}

#ifdef MMP_USRMMU
	// prepare MEMPOOL
	UM_ID hd = *mmphd;

	if( hd != -1 && hd != 0 )
	{
		if( UM_CheckPool(hd) != 0 )  // test existing of MEMPOOL
		{
			// invalid MEMPOOL
			if( errno == ESRCH )
				hd = -1;
			else
				return -1;  // undefined
		}
		else
		{
			// always delete
			UM_DeletePool( hd );
			hd = -1;
		}
	}

	if( hd == 0 || hd == -1 )
	{
		// create a new MEMPOOL
		*mmphd = hd = UM_CreatePoolEx2( 16, FALSE, FALSE, _MEMPOOL_INCSIZE );
		if( hd == -1 || hd == 0 )
			return -1;
	}
	return 0;
#else
	errno = errUnsupport;
	return -1;
#endif
}


