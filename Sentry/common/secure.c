
#include <bso/bso_socket.h>
#include "secure.h"


#ifdef _LINUX
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>

// for libcrypto.a's requirement of dladdr@@GLIBC_2.0
void dladdr_dummy( void )
{
	dladdr( NULL, (void*)-1 );
}

#else
#pragma comment( lib, "" )
#endif




#if defined( _USE_SSL_ )
int as_ssl_lib_init( void )
{
	//SSLeay_add_ssl_algorithms();

	SSL_library_init(); // always success

	//SSL_load_error_strings();

	return 0;
}


//
SSL_CTX* as_ssl_ctx_new( int cs )
{
	const SSL_METHOD *method;
	SSL_CTX *ctx;


	if( cs == 0x03 )
		method = SSLv23_method();
	else if( cs & 0x01 )
		method = SSLv23_server_method();
	else if( cs & 0x02 )
		method = SSLv23_client_method();
	else
		return NULL;

	if( !method )
	{
		//ERR_print_errors_fp( stderr );
		return NULL;
	}

	ctx = SSL_CTX_new( method );
	if( !ctx )
	{
		//ERR_print_errors_fp( stderr );
		return NULL;
	}


	SSL_CTX_set_mode( ctx, SSL_MODE_ENABLE_PARTIAL_WRITE );
	SSL_CTX_clear_mode( ctx, SSL_MODE_AUTO_RETRY );
	SSL_CTX_set_quiet_shutdown( ctx, 0 );

	return ctx;
}


//
void as_ssl_ctx_free( SSL_CTX **ctx )
{
	if( ctx && *ctx )
	{
		SSL_CTX_free( *ctx );
		*ctx = NULL;
	}
}
#endif

//
int as_ssl_send( SSL *ssl, int sock, const char *buf, int len )
{
	int ret;

#if defined( _USE_SSL_ )
	if( ssl )
	{
		ret = SSL_write( ssl, buf, len );
		if( ret == 0 )
		{
			ret = SOCKET_ERROR;
		}
		else if( ret < 1 )
		{
			ret = SSL_get_error( ssl, ret );
			ret = ( ret == SSL_ERROR_WANT_READ  || ret == SSL_ERROR_WANT_WRITE ) ? 0 : SOCKET_ERROR;
		}
	}
	else
	{
		ret = send( sock, buf, len, 0 );
	}

#else
	ret = send( sock, buf, len, 0 );
#endif

	return ret;
}


//
int as_ssl_recv( SSL *ssl, int sock, char *buf, int sz )
{
	int ret;

	SetLastError( 0 );

#if defined( _USE_SSL_ )
	if( ssl )
	{
		ret = SSL_read( ssl, buf, sz );
		if( ret == 0 )
		{
			ret = SOCKET_ERROR;
		}
		else if( ret < 1 )
		{
			ret = SSL_get_error( ssl, ret );
			ret = ( ret == SSL_ERROR_WANT_READ  || ret == SSL_ERROR_WANT_WRITE ) ? 0 : SOCKET_ERROR;
		}
	}
	else
	{
		ret = recv( sock, buf, sz, 0 );
		if( ret == 0 )
		{
			//if( GetLastError() == 0 )
			//	SetLastError( EWOULDBLOCK );

			//SetLastError( ECONNRESET );
			ret = SOCKET_ERROR;
		}
	}

#else
	ret = recv( sock, buf, sz, 0 );
	if( ret == 0 )
	{
		//if( GetLastError() == 0 )
		//	SetLastError( EWOULDBLOCK );

		//SetLastError( ECONNRESET );
		ret = SOCKET_ERROR;
	}
#endif

	return ret;
}


