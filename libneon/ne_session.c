/* 
   HTTP session handling
   Copyright (C) 1999-2003, Joe Orton <joe@manyfish.co.uk>
   Portions are:
   Copyright (C) 1999-2000 Tommi Komulainen <Tommi.Komulainen@iki.fi>

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

#include "config.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef NEON_SSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pkcs12.h>
#include <openssl/x509v3.h>
#endif

#include "ne_session.h"
#include "ne_alloc.h"
#include "ne_utils.h"
#include "ne_i18n.h"
#include "ne_string.h"

#include "ne_private.h"

#ifdef NEON_SSL
static int provide_client_cert(SSL *ssl, X509 **cert, EVP_PKEY **pkey);
#endif

/* Destroy a a list of hooks. */
static void destroy_hooks(struct hook *hooks)
{
    struct hook *nexthk;

    while (hooks) {
	nexthk = hooks->next;
	ne_free(hooks);
	hooks = nexthk;
    }
}

#ifdef NEON_SSL
/* Free client certificate stored in session. */
static void free_client_cert(ne_session *sess)
{
    if (sess->client_key)
        EVP_PKEY_free(sess->client_key);
    if (sess->client_cert) 
        X509_free(sess->client_cert);
    sess->client_key = NULL;
    sess->client_cert = NULL;
}
#endif

void ne_session_destroy(ne_session *sess) 
{
    struct hook *hk;

    NE_DEBUG(NE_DBG_HTTP, "ne_session_destroy called.\n");

    /* Run the destroy hooks. */
    for (hk = sess->destroy_sess_hooks; hk != NULL; hk = hk->next) {
	ne_destroy_sess_fn fn = (ne_destroy_sess_fn)hk->fn;
	fn(hk->userdata);
    }
    
    destroy_hooks(sess->create_req_hooks);
    destroy_hooks(sess->pre_send_hooks);
    destroy_hooks(sess->post_send_hooks);
    destroy_hooks(sess->destroy_req_hooks);
    destroy_hooks(sess->destroy_sess_hooks);
    destroy_hooks(sess->private);

    NE_FREE(sess->server.hostname);
    NE_FREE(sess->server.hostport);
    if (sess->server.address) ne_addr_destroy(sess->server.address);
    if (sess->proxy.address) ne_addr_destroy(sess->proxy.address);
    NE_FREE(sess->proxy.hostname);
    NE_FREE(sess->scheme);
    NE_FREE(sess->user_agent);

    if (sess->connected) {
	ne_close_connection(sess);
    }

#ifdef NEON_SSL
    if (sess->ssl_context)
	SSL_CTX_free(sess->ssl_context);

    if (sess->ssl_sess)
	SSL_SESSION_free(sess->ssl_sess);

    if (sess->server_cert)
	X509_free(sess->server_cert);

    free_client_cert(sess);
#endif

    ne_free(sess);
}

int ne_version_pre_http11(ne_session *s)
{
    return !s->is_http11;
}

/* Stores the "hostname[:port]" segment */
static void set_hostport(struct host_info *host, unsigned int defaultport)
{
    size_t len = strlen(host->hostname);
    host->hostport = ne_malloc(len + 10);
    strcpy(host->hostport, host->hostname);
    if (host->port != defaultport)
	ne_snprintf(host->hostport + len, 9, ":%u", host->port);
}

/* Stores the hostname/port in *info, setting up the "hostport"
 * segment correctly. */
static void
set_hostinfo(struct host_info *info, const char *hostname, unsigned int port)
{
    NE_FREE(info->hostport);
    NE_FREE(info->hostname);
    info->hostname = ne_strdup(hostname);
    info->port = port;
}

ne_session *ne_session_create(const char *scheme,
			      const char *hostname, unsigned int port)
{
    ne_session *sess = ne_calloc(sizeof *sess);

    NE_DEBUG(NE_DBG_HTTP, "HTTP session to %s://%s:%d begins.\n",
	     scheme, hostname, port);

    strcpy(sess->error, "Unknown error.");

    /* use SSL if scheme is https */
    sess->use_ssl = !strcmp(scheme, "https");
    
    /* set the hostname/port */
    set_hostinfo(&sess->server, hostname, port);
    set_hostport(&sess->server, sess->use_ssl?443:80);

#ifdef NEON_SSL
    if (sess->use_ssl) {
	sess->ssl_context = SSL_CTX_new(SSLv23_client_method());
	/* set client cert callback. */
	SSL_CTX_set_client_cert_cb(sess->ssl_context, provide_client_cert);
    }
#endif

    sess->scheme = ne_strdup(scheme);

    /* Default expect-100 to OFF. */
    sess->expect100_works = -1;
    return sess;
}

void ne_session_proxy(ne_session *sess, const char *hostname,
		      unsigned int port)
{
    sess->use_proxy = 1;
    set_hostinfo(&sess->proxy, hostname, port);
}

void ne_set_error(ne_session *sess, const char *format, ...)
{
    va_list params;

    va_start(params, format);
    ne_vsnprintf(sess->error, sizeof sess->error, format, params);
    va_end(params);
}


void ne_set_progress(ne_session *sess, 
		     ne_progress progress, void *userdata)
{
    sess->progress_cb = progress;
    sess->progress_ud = userdata;
}

void ne_set_status(ne_session *sess,
		     ne_notify_status status, void *userdata)
{
    sess->notify_cb = status;
    sess->notify_ud = userdata;
}

void ne_set_expect100(ne_session *sess, int use_expect100)
{
    if (use_expect100) {
	sess->expect100_works = 1;
    } else {
	sess->expect100_works = -1;
    }
}

void ne_set_persist(ne_session *sess, int persist)
{
    sess->no_persist = !persist;
}

void ne_set_read_timeout(ne_session *sess, int timeout)
{
    sess->rdtimeout = timeout;
}

#define AGENT " neon/" NEON_VERSION

void ne_set_useragent(ne_session *sess, const char *token)
{
    if (sess->user_agent) ne_free(sess->user_agent);
    sess->user_agent = malloc(sizeof AGENT + strlen(token));
    strcat(strcpy(sess->user_agent, token), AGENT);
}

const char *ne_get_server_hostport(ne_session *sess)
{
    return sess->server.hostport;
}

const char *ne_get_scheme(ne_session *sess)
{
    return sess->scheme;
}

void ne_fill_server_uri(ne_session *sess, ne_uri *uri)
{
    uri->host = ne_strdup(sess->server.hostname);
    uri->port = sess->server.port;
    uri->scheme = ne_strdup(sess->scheme);
}

const char *ne_get_error(ne_session *sess)
{
    return ne_strclean(sess->error);
}

void ne_close_connection(ne_session *sess)
{
    if (sess->connected) {
	NE_DEBUG(NE_DBG_SOCKET, "Closing connection.\n");
	ne_sock_close(sess->socket);
	sess->socket = NULL;
	NE_DEBUG(NE_DBG_SOCKET, "Connection closed.\n");
    } else {
	NE_DEBUG(NE_DBG_SOCKET, "(Not closing closed connection!).\n");
    }
    sess->connected = 0;
}

void ne_ssl_set_verify(ne_session *sess, ne_ssl_verify_fn fn, void *userdata)
{
    sess->ssl_verify_fn = fn;
    sess->ssl_verify_ud = userdata;
}

void ne_ssl_provide_ccert(ne_session *sess, 
			  ne_ssl_provide_fn fn, void *userdata)
{
    sess->ssl_provide_fn = fn;
    sess->ssl_provide_ud = userdata;
}

void ne_ssl_keypw_prompt(ne_session *sess, ne_ssl_keypw_fn fn, void *ud)
{
    sess->ssl_keypw_fn = fn;
    sess->ssl_keypw_ud = ud;
}

char *ne_ssl_readable_dname(const ne_ssl_dname *dn)
{
    int flag = 0;
    ne_buffer *buf = ne_buffer_create();

#define DO(f) do { \
if (f) { ne_buffer_concat(buf, flag?", ":"", f, NULL); flag = 1; } \
} while (0)

    DO(dn->organizationalUnit);
    DO(dn->organization);
    DO(dn->locality);
    DO(dn->state);
    DO(dn->country);

#undef DO

    return ne_buffer_finish(buf);
}

#ifdef NEON_SSL

SSL_CTX *ne_ssl_get_context(ne_session *sess)
{
    sess->ssl_context->references++;
    return sess->ssl_context;
}

/* Map a server cert verification into a string. */
static void verify_err(ne_session *sess, int failures)
{
    struct {
	int bit;
	const char *str;
    } reasons[] = {
	{ NE_SSL_NOTYETVALID, N_("not yet valid") },
	{ NE_SSL_EXPIRED, N_("Server certificate has expired") },
	{ NE_SSL_CNMISMATCH, N_("Certificate hostname mismatch") },
	{ NE_SSL_UNKNOWNCA, N_("issuer not trusted") },
	{ 0, NULL }
    };
    int n, flag = 0;

    strcpy(sess->error, _("Server certificate verification failed: "));

    for (n = 0; reasons[n].bit; n++) {
	if (failures & reasons[n].bit) {
	    if (flag) strncat(sess->error, ", ", sizeof sess->error);
	    strncat(sess->error, _(reasons[n].str), sizeof sess->error);
	    flag = 1;
	}
    }

}

/* enough to store a single field. */
#define ATTBUFSIZ (1028)

/* Get an attribute out of 'name', using 'dump' as a temporary
 * buffer. */
static const char *getx509field(X509_NAME *name, int nid, 
				ne_buffer *dump)
{
    char *buf;
    int ret;
    
    /* make sure we have 1K of space. */
    ne_buffer_grow(dump, dump->used + ATTBUFSIZ);
    buf = dump->data + dump->used;
    ret = X509_NAME_get_text_by_NID(name, nid, buf, ATTBUFSIZ);
    if (ret < 1) {
	return NULL;
    } else {
	dump->used += (size_t)ret + 1; /* +1 for \0 */
	return buf;
    }
}

/* Return malloc-allocated string representation of given ASN.1 time
 * structure.  TODO: would be better to parse out the raw ASN.1 string
 * and give that to the application in some form which is localisable
 * e.g. time_t.  */
static char *asn1time_to_string(ASN1_TIME *tm)
{
    char buf[64];
    BIO *bio;
    
    strncpy(buf, _("[invalid date]"), sizeof buf);
    
    bio = BIO_new(BIO_s_mem());
    if (bio) {
	if (ASN1_TIME_print(bio, tm))
	    BIO_read(bio, buf, sizeof buf);
	BIO_free(bio);
    }
  
    return ne_strdup(buf);
}

/* Return non-zero if hostname from certificate (cn) matches hostname
 * used for session (hostname). TODO: could do more advanced wildcard
 * matching using fnmatch() here, if fnmatch is present. */
static int match_hostname(char *cn, const char *hostname)
{
    const char *dot;
    NE_DEBUG(NE_DBG_SSL, "Match %s on %s...\n", cn, hostname);
    dot = strchr(hostname, '.');
    if (dot == NULL) {
	char *pnt = strchr(cn, '.');
	/* hostname is not fully-qualified; unqualify the cn. */
	if (pnt != NULL) {
	    *pnt = '\0';
	}
    }
    else if (strncmp(cn, "*.", 2) == 0) {
	hostname = dot + 1;
	cn += 2;
    }
    return !strcasecmp(cn, hostname);
}

/* Fills in the friendly DN structure 'dn' from given X509 name 'xn',
 * using 'dump' as temporary storage. */
static void make_dname(ne_ssl_dname *dn, X509_NAME *xn, ne_buffer *dump)
{
    dn->country = getx509field(xn, NID_countryName, dump);
    dn->state = getx509field(xn, NID_stateOrProvinceName, dump);
    dn->locality = getx509field(xn, NID_localityName, dump);
    dn->organization = getx509field(xn, NID_organizationName, dump);
    dn->organizationalUnit = getx509field(xn, 
					  NID_organizationalUnitName, dump);
    dn->commonName = getx509field(xn, NID_commonName, dump);
}

/* Check certificate identity.  Returns zero if identity matches; 1 if
 * identity does not match, or <0 on error (session error string
 * set). */
static int check_identity(ne_session *sess, X509 *cert)
{
    STACK_OF(GENERAL_NAME) *names;
    int match = 0, found = 0;
    
    names = X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL);
    if (names) {
	/* Got a subject alt. name extension. */
	int n;

	for (n = 0; n < sk_GENERAL_NAME_num(names) && !match; n++) {
	    GENERAL_NAME *nm = sk_GENERAL_NAME_value(names, n);
	    
	    /* only care about this if it is a DNS name. */
	    if (nm->type == GEN_DNS) {
		/* TODO: convert this string to UTF8 or something. */
		char *name = ne_strndup(nm->d.ia5->data, nm->d.ia5->length);
		match = match_hostname(name, sess->server.hostname);
		ne_free(name);
		found = 1;
	    }
	}
        /* free the whole stack. */
        sk_GENERAL_NAME_pop_free(names, GENERAL_NAME_free);
    }
    
    /* Check against the commonName if no DNS alt. names were found,
     * as per RFC2818. */
    if (!found) {
	X509_NAME *subj = X509_get_subject_name(cert);
	X509_NAME_ENTRY *entry;
	ASN1_STRING *str;
	int idx = -1, lastidx;
	char *name;

	/* find the most specific commonName attribute. */
	do {
	    lastidx = idx;
	    idx = X509_NAME_get_index_by_NID(subj, NID_commonName, lastidx);
	} while (idx >= 0);
	
	if (lastidx < 0) {
	    ne_set_error(sess, _("Server certificate was missing "
				 "commonName attribute in subject name"));
	    return -1;
	}

	/* extract the string from the entry */
	entry = X509_NAME_get_entry(subj, lastidx);
	str = X509_NAME_ENTRY_get_data(entry);

	name = ne_strndup(str->data, str->length);
	match = match_hostname(name, sess->server.hostname);
	ne_free(name);
    }

    NE_DEBUG(NE_DBG_SSL, "Identity match: %s\n", match ? "good" : "bad");
    return match ? 0 : 1;
}

/* Verifies an SSL server certificate. */
static int check_certificate(ne_session *sess, SSL *ssl, X509 *cert)
{
    X509_NAME *subj = X509_get_subject_name(cert);
    X509_NAME *issuer = X509_get_issuer_name(cert);
    ASN1_TIME *notBefore = X509_get_notBefore(cert);
    ASN1_TIME *notAfter = X509_get_notAfter(cert);
    int ret, failures = 0;
    long result;

    /* check expiry dates */
    if (X509_cmp_current_time(notBefore) >= 0)
	failures |= NE_SSL_NOTYETVALID;
    else if (X509_cmp_current_time(notAfter) <= 0)
	failures |= NE_SSL_EXPIRED;

    /* Check certificate was issued to this server. */
    ret = check_identity(sess, cert);
    if (ret < 0) return NE_ERROR;
    else if (ret > 0) failures |= NE_SSL_CNMISMATCH;

    /* get the result of the cert verification out of OpenSSL */
    result = SSL_get_verify_result(ssl);

    NE_DEBUG(NE_DBG_SSL, "Verify result: %ld = %s\n", result,
	     X509_verify_cert_error_string(result));

#if NE_DEBUGGING
    if (ne_debug_mask & NE_DBG_SSL) {
	STACK_OF(X509) *chain = SSL_get_peer_cert_chain(ssl);
	int n, top = chain?sk_X509_num(chain):0;

	NE_DEBUG(NE_DBG_SSL, "Peer chain depth is %d\n", top);
	
	for (n = 0; n < top; n++) {
	    X509 *c = sk_X509_value(chain, n);
	    NE_DEBUG(NE_DBG_SSL, "Cert #%d:\n", n);
	    X509_print_fp(ne_debug_stream, c);
	}
    }
#endif

    switch (result) {
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
    case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
    case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
	/* TODO: and probably more result codes here... */
	failures |= NE_SSL_UNKNOWNCA;
	break;
    case X509_V_ERR_CERT_NOT_YET_VALID:
    case X509_V_ERR_CERT_HAS_EXPIRED:
	/* ignore these, since we've already noticed them . */
	break;
    case X509_V_OK:
	/* it's okay. */
	break;
    default:
	/* TODO: tricky to handle the 30-odd failure cases OpenSSL
	 * presents here (see x509_vfy.h), and present a useful API to
	 * the application so it in turn can then present a meaningful
	 * UI to the user.  The only thing to do really would be to
	 * pass back the error string, but that's not localisable.  So
	 * just fail the verification here - better safe than
	 * sorry. */
	ne_set_error(sess, _("Certificate verification error: %s"),
		     X509_verify_cert_error_string(result));
	return NE_ERROR;
    }

    if (sess->ssl_verify_fn && failures) {
	ne_ssl_certificate c;
	ne_ssl_dname sdn = {0}, idn = {0};
	ne_buffer *dump = ne_buffer_ncreate(ATTBUFSIZ * 2);
	char *from, *until;
	
	/* Do the gymnatics to retrieve attributes out of the
	 * X509_NAME, store them in a temporary buffer (dump), and set
	 * the structure fields up to pass to the verify callback.
	 * Using a temp buffer means that this can be done with only a
	 * few malloc() calls and only one ne_free(). */

	dump->used = 0; /* ignore the initial \0 */

	make_dname(&sdn, subj, dump);
	make_dname(&idn, issuer, dump);
	
	c.subject = &sdn;
	c.issuer = &idn;
	c.from = from = asn1time_to_string(notBefore);
	c.until = until = asn1time_to_string(notAfter);

	if (sess->ssl_verify_fn(sess->ssl_verify_ud, failures, &c)) {
	    ne_set_error(sess, _("Certificate verification failed"));
	    ret = NE_ERROR;
	} else {
	    ret = NE_OK;
	}

	ne_buffer_destroy(dump);
	ne_free(from);
	ne_free(until);

    } else if (failures != 0) {
	verify_err(sess, failures);
	ret = NE_ERROR;
    } else {
	/* well, okay then if you insist. */
	ret = NE_OK;
    }

    return ret;
}

/* Callback invoked when the SSL server requests a client certificate.  */
static int provide_client_cert(SSL *ssl, X509 **cert, EVP_PKEY **pkey)
{
    ne_session *sess = SSL_get_app_data(ssl);

    if (!sess->client_key && sess->ssl_provide_fn) {
	ne_ssl_dname dn;
	ne_buffer *buf;
	X509 *peer = SSL_get_peer_certificate(ssl);
	X509_NAME *subject;	

	if (!peer) {
	    NE_DEBUG(NE_DBG_SSL, 
		     "Peer subject unspecified; cannot provide c.cert\n");
	    return 0;
	}

	subject = X509_get_subject_name(peer);

	buf = ne_buffer_ncreate(2048);
	make_dname(&dn, subject, buf);

	NE_DEBUG(NE_DBG_SSL, "Calling client certificate provider...\n");
	sess->ssl_provide_fn(sess->ssl_provide_ud, sess, &dn);
        ne_buffer_destroy(buf);
        X509_free(peer);
    }

    if (sess->client_key && sess->client_cert) {
	NE_DEBUG(NE_DBG_SSL, "Supplying client certificate.\n");
	sess->client_cert->references++;
	sess->client_key->references++;
	*cert = sess->client_cert;
	*pkey = sess->client_key;
	return 1;
    } else {
	NE_DEBUG(NE_DBG_SSL, "No client certificate supplied.\n");
	return 0;
    }
}

/* For internal use only. */
int ne_negotiate_ssl(ne_request *req)
{
    ne_session *sess = ne_get_session(req);
    SSL *ssl;
    X509 *cert;

    NE_DEBUG(NE_DBG_SSL, "Doing SSL negotiation.\n");

    if (ne_sock_use_ssl_os(sess->socket, sess->ssl_context, 
			   sess->ssl_sess, &ssl, sess)) {
	if (sess->ssl_sess) {
	    /* remove cached session. */
	    SSL_SESSION_free(sess->ssl_sess);
	    sess->ssl_sess = NULL;
	}
	ne_set_error(sess, _("SSL negotiation failed: %s"),
		     ne_sock_error(sess->socket));
	return NE_ERROR;
    }	
    
    cert = SSL_get_peer_certificate(ssl);
    if (cert == NULL) {
	ne_set_error(sess, _("SSL server did not present certificate"));
	return NE_ERROR;
    }

    if (sess->server_cert) {
	int cmp = X509_cmp(cert, sess->server_cert);
	X509_free(cert);
	if (cmp) {
	    /* This could be a MITM attack: fail the request. */
	    ne_set_error(sess, _("Server certificate changed: "
				 "connection intercepted?"));
	    X509_free(sess->server_cert);
	    sess->server_cert = NULL;
	    return NE_ERROR;
	} 
	/* certificate has already passed verification: no need to
	 * verify it again. */
    } else {
	/* new connection: verify the cert. */
	if (check_certificate(sess, ssl, cert)) {
	    NE_DEBUG(NE_DBG_SSL, "SSL certificate checks failed: %s\n",
		     sess->error);
	    X509_free(cert);
	    return NE_ERROR;
	}
	/* cache the cert. */
	sess->server_cert = cert;
    }
    
    if (!sess->ssl_sess) {
	/* store the session. */
	sess->ssl_sess = SSL_get1_session(ssl);
    }

    if (sess->notify_cb) {
	sess->notify_cb(sess->notify_ud, ne_conn_secure, SSL_get_version(ssl));
    }

    return NE_OK;
}

int ne_ssl_load_ca(ne_session *sess, const char *file)
{
    return !SSL_CTX_load_verify_locations(sess->ssl_context, file, NULL);
}

int ne_ssl_load_default_ca(ne_session *sess)
{
    return !SSL_CTX_set_default_verify_paths(sess->ssl_context);
}

static int privkey_prompt(char *buf, int len, int rwflag, void *userdata)
{
    ne_session *sess = userdata;
    
    if (sess->ssl_keypw_fn(sess->ssl_keypw_ud, buf, len))
	return -1;

    /* Obscurely OpenSSL requires the callback to return the length of
     * the password, this seems a bit weird so we don't expose this in
     * the neon API. */
    return strlen(buf);
}

int ne_ssl_load_pkcs12(ne_session *sess, const char *fn)
{
    /* you are lost in a maze of twisty crypto algorithms... */
    PKCS12 *p12;
    FILE *fp;
    int ret;
    char *password = NULL, buf[BUFSIZ];

    fp = fopen(fn, "r");
    if (fp == NULL) {
	ne_strerror(errno, buf, sizeof buf);
	ne_set_error(sess, _("Could not open file `%s': %s"), fn, buf);
	return -1;
    }

    p12 = d2i_PKCS12_fp(fp, NULL);

    fclose(fp);
    
    if (p12 == NULL) {
	ne_set_error(sess, _("Could not read certificate from file `%s'"),
		     fn);
	return -1;
    }

    if (sess->ssl_keypw_fn) {
	if (sess->ssl_keypw_fn(sess->ssl_keypw_ud, buf, sizeof buf) == 0)
	    password = buf;
    }

    free_client_cert(sess);
	
    ret = PKCS12_parse(p12, password, 
		       &sess->client_key, &sess->client_cert, NULL);
    PKCS12_free(p12);

    if (ret != 1) {
	ne_set_error(sess,
		     _("Error parsing certificate (incorrect password?): %s"),
		     ERR_reason_error_string(ERR_get_error()));
	return -1;
    }

    return 0;
}

int ne_ssl_load_pem(ne_session *sess, const char *cert, const char *key)
{
    FILE *fp;
    char err[200];

    fp = fopen(cert, "r");
    if (fp == NULL) {
	ne_strerror(errno, err, sizeof err);
	ne_set_error(sess, _("Could not open file `%s': %s"), cert, err);
	return -1;
    }

    free_client_cert(sess);

    sess->client_cert = PEM_read_X509(fp, NULL, privkey_prompt, sess);
    if (sess->client_cert == NULL) {
	ne_set_error(sess, _("Could not read certificate: %s"),
		     ERR_reason_error_string(ERR_get_error()));
	fclose(fp);
	return -1;
    }

    if (key != NULL) {
	fclose(fp);
	fp = fopen(key, "r");
	if (fp == NULL) {
	    ne_strerror(errno, err, sizeof err);
	    ne_set_error(sess, _("Could not open private key file `%s': %s"),
			 key, err);
	    return -1;
	}
    }

    sess->client_key = PEM_read_PrivateKey(fp, NULL, privkey_prompt, sess);
    fclose(fp);
    if (sess->client_key == NULL) {
	ne_set_error(sess, _("Could not parse private key: %s"),
		     ERR_reason_error_string(ERR_get_error()));
	return -1;
    }
    
    return 0;
}

X509 *ne_ssl_server_cert(ne_session *sess)
{
    return sess->server_cert;
}

#else

#define STUB(sess) ne_set_error(sess, _("SSL is not supported")); return NE_ERROR

/* Stubs to make the library have the same ABI whether or not SSL
 * support is enabled. */
int ne_negotiate_ssl(ne_request *req) { STUB(ne_get_session(req)); }
int ne_ssl_load_ca(ne_session *sess, const char *file) { STUB(sess); }
int ne_ssl_load_default_ca(ne_session *sess) { STUB(sess); }
int ne_ssl_load_pkcs12(ne_session *sess, const char *fn) { STUB(sess); }
int ne_ssl_load_pem(ne_session *sess, const char *cert, const char *key) { STUB(sess); }

#endif


