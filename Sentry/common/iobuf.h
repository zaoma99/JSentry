// iobuf.h created by CXX on 8th April 2017

#ifndef __IOBUF_H__
#define __IOBUF_H__

#include <common/pdefine.h>


// 通用的 IO-Buffer
typedef struct
{
	UINT8  *addr;	// 存储区首地址
	size_t size;	// 容量
	size_t num;		// 有效字节数
	size_t pos;		// 当前指针
}IOBUF_T;


// 创建buffer
#define IOBUF_CREATE(_buf, _sz)	{\
	(_buf)->addr = malloc( sizeof(UINT8) * (_sz) ); \
	if( (_buf)->addr ) {	  \
		(_buf)->size = (_sz); \
		(_buf)->num = (_buf)->pos = 0; \
	} \
}

// 释放buffer
#define IOBUF_DESTROY(_buf) {\
	if( (_buf) && (_buf)->addr ){\
		free( (_buf)->addr );	 \
		(_buf)->addr = NULL;	 \
		(_buf)->size = (_buf)->num = (_buf)->pos = 0; \
	}\
}

// 修改内存实际容量( 2022-05-04 )
#define IOBUF_RESIZE(_buf, _sz)	{\
	void *ptr = realloc( sizeof(UINT8) * (_sz) ); \
	if( ptr ) {	  \
		(_buf)->addr = ptr; \
		(_buf)->size = (_sz); \
		if( (_sz) < (_buf)->num ) (_buf)->num = _sz; \
		if( (_buf)->num < (_buf)->pos ) (_buf)->pos = (_buf)->num; \
	} \
}

// 清空buffer
#define IOBUF_CLEAR(_buf)		(_buf)->num = (_buf)->pos = 0;

// 剩余容量
#define IOBUF_CAPACITY(_buf)	( (_buf)->size - (_buf)->num )

// 是否已满
#define IOBUF_ISFULL(_buf)		( (_buf)->num == (_buf)->size )

// 是否为空
#define IOBUF_ISEMPTY(_buf)		( (_buf)->num == 0 )

// num
#define IOBUF_NUM(_buf)			(0,(_buf)->num)

#define IOBUF_SIZE(_buf)		(0,(_buf)->size)

#define IOBUF_HEAD(_buf)		( (_buf)->addr )

#define IOBUF_TAIL(_buf)		( &(_buf)->addr[(_buf)->num] )

#define IOBUF_ADD(_buf, _len)	( (_buf)->num += (_len) )

#define IOBUF_DEC(_buf, _len)	( (_buf)->num -= (_len) )

#define IOBUF_WRITE(_buf, _src, _len)\
	( memcpy( &(_buf)->addr[(_buf)->num], _src, _len ) && ((_buf)->num += _len) )



// pos
#define IOBUF_POS(_buf)		(0,(_buf)->pos)

#define IOBUF_SETTOHDR(_buf)	\
	( (_buf)->num > 0 ? &(_buf)->addr[(_buf)->pos = 0] : NULL )


#define IOBUF_SETTOEND(_buf)	\
	( (_buf)->num > 0 ? &(_buf)->addr[(_buf)->pos = (_buf)->num-1] : NULL )

#define IOBUF_SKIPALL(_buf)		( (_buf)->pos = (_buf)->num )

#define IOBUF_PTR(_buf)			( &(_buf)->addr[(_buf)->pos] )

#define IOBUF_AVAILABLE(_buf)	( (_buf)->num - (_buf)->pos )

#define IOBUF_SEEK(_buf, _off) \
	( (_buf)->num > 0 && (((_buf)->pos += _off) <= (_buf)->num) ? (_buf)->pos : ((_buf)->pos = (_buf)->num) )
	//( (_buf)->pos += _off )
	//( ((_buf)->pos += _off) < (_buf)->num && (_buf)->pos >= 0 ? &(_buf)->addr[(_buf)->pos] : ((_buf)->pos -= _off) & 0x00 )

#define IOBUF_SEEK2(_buf, _off)	( (_buf)->pos += _off )

// reduce
#define IOBUF_REDUCE(_buf)	(\
	  ((_buf)->pos > 0 && (_buf)->pos <= (_buf)->num) && \
	  (memcpy((_buf)->addr, &(_buf)->addr[(_buf)->pos], ((_buf)->num -= (_buf)->pos)) ,  \
	   ((_buf)->pos = 0), 1) \
	   )

#define IOBUF_REDUCE2(_buf)	\
	  ( (memcpy((_buf)->addr, &(_buf)->addr[(_buf)->pos], ((_buf)->num -= (_buf)->pos)) , ((_buf)->pos = 0), 1) )


#endif
