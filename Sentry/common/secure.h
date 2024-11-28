/*
 * dsc_secure.h
 *
 *  Created on: May 9, 2017
 *      Author: root
 */

#ifndef _AS_SECURE_H_
#define _AS_SECURE_H_


//#include <crypt/h/xxtea.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

//#define _USE_SSL_


// make a digital digest signature
#define AS_DSA_MAKE_DDS( _md_ctx, _md, _src, _srclen, _out, _outlen ) (\
		EVP_DigestInit_ex( _md_ctx, _md, NULL ) && \
		EVP_DigestUpdate( _md_ctx, _src, _srclen ) && \
		EVP_DigestFinal_ex( _md_ctx, _out, _outlen ) )


// make user's token
#define AS_DSA_MAKE_TOKEN(_md_ctx, _license, _unm, _pwd, _expires, _token, _len) {\
	EVP_DigestInit_ex( _md_ctx, EVP_sha256(), NULL );  \
	EVP_DigestUpdate( _md_ctx, _license, strlen(_license) );   \
	EVP_DigestUpdate( _md_ctx, _unm, strlen(_unm) );   \
	EVP_DigestUpdate( _md_ctx, _pwd, strlen(_pwd) );   \
	EVP_DigestUpdate( _md_ctx, _expires, strlen(_expires) );   \
	EVP_DigestFinal_ex( _md_ctx, _token, _len );	   \
	}

#define AS_RAND_KEY_GET( _buf_, _len_ )\
		RAND_bytes( _buf_, _len_ )



extern int as_ssl_lib_init( void );

extern SSL_CTX* as_ssl_ctx_new( int cs );

extern void as_ssl_ctx_free( SSL_CTX **ctx );

extern int as_ssl_send( SSL *ssl, int sock, const char *buf, int len );

extern int as_ssl_recv( SSL *ssl, int sock, char *buf, int sz );

#endif /* _AS_SECURE_H_ */
