/* 
   HTTP session handling
   Copyright (C) 1999-2002, Joe Orton <joe@manyfish.co.uk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA

*/

#ifndef NE_SESSION_H
#define NE_SESSION_H 1

#include <sys/types.h>

#ifdef NEON_SSL
#include <openssl/ssl.h>
#endif

#include "ne_uri.h" /* for ne_uri */
#include "ne_defs.h"

BEGIN_NEON_DECLS

typedef struct ne_session_s ne_session;

/* Create a session to the given server, using the given scheme.  If
 * "https" is passed as the scheme, SSL will be used to connect to the
 * server. */
ne_session *ne_session_create(const char *scheme,
			      const char *hostname, unsigned int port);

/* Finish an HTTP session */
void ne_session_destroy(ne_session *sess);

/* Prematurely force the connection to be closed for the given
 * session. */
void ne_close_connection(ne_session *sess);

/* Set the proxy server to be used for the session. */
void ne_session_proxy(ne_session *sess,
		      const char *hostname, unsigned int port);

/* Set protocol options for session:
 *   expect100: Defaults to OFF
 *   persist:   Defaults to ON
 *
 * expect100: When set, send the "Expect: 100-continue" request header
 * with requests with bodies.
 *
 * persist: When set, use a persistent connection. (Generally,
 * you don't want to turn this off.)
 * */
void ne_set_expect100(ne_session *sess, int use_expect100);
void ne_set_persist(ne_session *sess, int persist);

/* Progress callback. */
typedef void (*ne_progress)(void *userdata, off_t progress, off_t total);

/* Set a progress callback for the session. */
void ne_set_progress(ne_session *sess, 
		     ne_progress progress, void *userdata);

/* Store an opaque context for the session, 'priv' is returned by a
 * call to ne_session_get_private with the same ID. */
void ne_set_session_private(ne_session *sess, const char *id, void *priv);
void *ne_get_session_private(ne_session *sess, const char *id);

typedef enum {
    ne_conn_namelookup, /* lookup up hostname (info = hostname) */
    ne_conn_connecting, /* connecting to host (info = hostname) */
    ne_conn_connected, /* connected to host (info = hostname) */
    ne_conn_secure /* connection now secure (info = crypto level) */
} ne_conn_status;

typedef void (*ne_notify_status)(void *userdata, 
				 ne_conn_status status,
				 const char *info);


/* Set a status notification callback for the session, to report
 * connection status. */
void ne_set_status(ne_session *sess,
		   ne_notify_status status, void *userdata);


/* A distinguished name; unique identifier for some entity. Any of
 * these fields other than commonName might be NULL. */
typedef struct {
    const char *country, *state, *locality, *organization;
    const char *organizationalUnit;
    /* commonName gives the hostname to which the certificate was
     * issued; will never by NULL. */
    const char *commonName;
} ne_ssl_dname;

/* Returns a single-line string representing a distinguished name,
 * intended to be human-readable (e.g. "Acme Ltd., Norfolk, GB").
 * Return value is malloc-allocated and must be free'd by the
 * caller. */
char *ne_ssl_readable_dname(const ne_ssl_dname *dn);

/* An SSL certificate (simplified), giving the name of the entity
 * which the certificate identifies, and the name of the entity which
 * issued (and signed) the certificate; the certificate authority.  A
 * certificate is only valid for a certain period, as given by the
 * "from" and "until" times. */
typedef struct {
    const ne_ssl_dname *subject, *issuer;
    const char *from, *until;
} ne_ssl_certificate;

/* Certificate verification problems. */

/* certificate is not yet valid: */
#define NE_SSL_NOTYETVALID (1<<0)
/* certificate has expired: */
#define NE_SSL_EXPIRED (1<<1)
/* hostname (a.k.a commonName) to which the certificate was issued
 * does not match the hostname of the server: this could mean that the
 * connection has been intercepted and is being attacked, or that the
 * server has not been configured properly. */
#define NE_SSL_CNMISMATCH (1<<2)
/* the certificate authority which issued this certificate is not
 * trusted. */
#define NE_SSL_UNKNOWNCA (1<<3)

/* The bitmask of known failure bits: if (failures & ~NE_SSL_FAILMASK)
 * is non-zero, an unrecognized failure is given, and the verification
 * should be failed. */
#define NE_SSL_FAILMASK (0x0f)

/* A callback which is used when server certificate verification is
 * needed.  The reasons for verification failure are given in the
 * 'failures' parameter, which is a binary OR of one or more of the
 * above NE_SSL_* values. failures is guaranteed to be non-zero.  The
 * callback must return zero to accept the certificate: a non-zero
 * return value will fail the SSL negotiation. */
typedef int (*ne_ssl_verify_fn)(void *userdata, int failures,
				const ne_ssl_certificate *cert);

/* Install a callback to handle server certificate verification.  This
 * is required when the CA certificate is not known for the server
 * certificate, or the server cert has other verification problems. */
void ne_ssl_set_verify(ne_session *sess, ne_ssl_verify_fn fn, void *userdata);

/* Add a file containing CA certificates in PEM-encoded format.
 * Returns non-zero on error. */
int ne_ssl_load_ca(ne_session *sess, const char *file);

/* Load default set of trusted CA certificates provided by the SSL
 * library (if any). Returns non-zero on error. */
int ne_ssl_load_default_ca(ne_session *sess);

/* Private key password prompt: must copy a NUL-terminated password
 * into 'pwbuf', which is of length 'len', and return zero. */
typedef int (*ne_ssl_keypw_fn)(void *userdata, char *pwbuf, size_t len);

/* Register a callback function which will be invoked if a client
 * certificate is loaded which uses an encrypted private key, and a
 * password is required to decrypt it. */
void ne_ssl_keypw_prompt(ne_session *sess, ne_ssl_keypw_fn fn, void *ud);

/* Utility functions, which load a client certificate file using
 * PKCS#12 or PEM encoding, for the session.  Return non-zero on
 * error, with session error set.  The private key password callback
 * may be used, and these functions may fail if a password callback
 * has not been registered.
 *
 * These functions may be used to pre-emptively register a client
 * certificate. Alternatively, they can be called only when the server
 * requests a client certificate, by using them from a callback
 * registered using ne_ssl_provide_ccert.  */
int ne_ssl_load_pkcs12(ne_session *sess, const char *fn);
/* For ne_ssl_load_pem, the private key may be in a separate file to
 * the certificate: if both key and cert are in the same file, pass
 * keyfn as NULL. */
int ne_ssl_load_pem(ne_session *sess, const char *certfn, const char *keyfn);

/* Callback used to supply a client certificate on demand.  The
 * distinguished name of the certificate presented by the server
 * 'server'.  Typically the callback will use ne_ssl_load_pkcs12 or
 * ne_ssl_load_pem. */
typedef void (*ne_ssl_provide_fn)(void *userdata, ne_session *sess,
				  const ne_ssl_dname *server);

/* Register a function to be called when the server requests a client
 * certificate. */
void ne_ssl_provide_ccert(ne_session *sess,
			  ne_ssl_provide_fn fn, void *userdata);

#ifdef NEON_SSL
/* WARNING: use of the functions inside this ifdef means that your
 * application cannot be linked against a neon library which was built
 * without SSL support. */

/* Get the OpenSSL context used to instatiate SSL connections.  Will
 * return NULL if SSL is not being used for this session. The
 * reference count of the SSL_CTX is increased, so the caller must
 * SSL_CTX_free it after use.  */
SSL_CTX *ne_ssl_get_context(ne_session *sess);

/* Returns a pointer to the certificate returned by the server. */
X509 *ne_ssl_server_cert(ne_session *req);
#endif

/* Set the timeout (in seconds) used when reading from a socket.  The
 * timeout value must be greater than zero. */
void ne_set_read_timeout(ne_session *sess, int timeout);

/* Sets the user-agent string. neon/VERSION will be appended, to make
 * the full header "User-Agent: product neon/VERSION".
 * If this function is not called, the User-Agent header is not sent.
 * The product string must follow the RFC2616 format, i.e.
 *       product         = token ["/" product-version]
 *       product-version = token
 * where token is any alpha-numeric-y string [a-zA-Z0-9]* */
void ne_set_useragent(ne_session *sess, const char *product);

/* Determine if next-hop server claims HTTP/1.1 compliance. Returns:
 *   0 if next-hop server does NOT claim HTTP/1.1 compliance
 *   non-zero if next-hop server DOES claim HTTP/1.1 compliance
 * Not that the "next-hop" server is the proxy server if one is being
 * used, otherwise, the origin server.
 */
int ne_version_pre_http11(ne_session *sess);

/* Returns the 'hostport' URI segment for the end-server, e.g.
 *    "my.server.com:8080"    or    "www.server.com" 
 *  (port segment is ommitted if == 80) 
 */
const char *ne_get_server_hostport(ne_session *sess);

/* Returns the URL scheme being used for the current session.
 * Does NOT include a trailing ':'. 
 * Example returns: "http" or "https".
 */
const char *ne_get_scheme(ne_session *sess);

/* Sets uri->host, uri->scheme, uri->port members ONLY of given URI
 * structure; host and scheme are malloc-allocated. */
void ne_fill_server_uri(ne_session *sess, ne_uri *uri);

/* Set the error string for the session; takes printf-like format
 * string. */
void ne_set_error(ne_session *sess, const char *format, ...)
#ifdef __GNUC__
                __attribute__ ((format (printf, 2, 3)))
#endif /* __GNUC__ */
;

/* Retrieve the error string for the session */
const char *ne_get_error(ne_session *sess);

END_NEON_DECLS

#endif /* NE_SESSION_H */
