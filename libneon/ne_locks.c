/* 
   WebDAV Class 2 locking operations
   Copyright (C) 1999-2003, Joe Orton <joe@manyfish.co.uk>

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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include <ctype.h> /* for isdigit() */

#include "ne_alloc.h"

#include "ne_request.h"
#include "ne_xml.h"
#include "ne_locks.h"
#include "ne_uri.h"
#include "ne_basic.h"
#include "ne_props.h"
#include "ne_207.h"
#include "ne_i18n.h"

#define HOOK_ID "http://webdav.org/neon/hooks/webdav-locking"

/* A list of lock objects. */
struct lock_list {
    struct ne_lock *lock;
    struct lock_list *next, *prev;
};

struct ne_lock_store_s {
    struct lock_list *locks;
    struct lock_list *cursor; /* current position in 'locks' */
};

struct lh_req_cookie {
    const ne_lock_store *store;
    struct lock_list *submit;
};

/* Context for PROPFIND/lockdiscovery callbacks */
struct discover_ctx {
    ne_session *session;
    ne_lock_result results;
    void *userdata;
};

/* Context for handling LOCK response */
struct lock_ctx {
    struct ne_lock active; /* activelock */
    char *token; /* the token we're after. */
    int found;
};

/* Element ID's start at HIP_ELM_UNUSED and work upwards */

#define NE_ELM_LOCK_FIRST (NE_ELM_207_UNUSED)

#define NE_ELM_lockdiscovery (NE_ELM_LOCK_FIRST)
#define NE_ELM_activelock (NE_ELM_LOCK_FIRST + 1)
#define NE_ELM_lockscope (NE_ELM_LOCK_FIRST + 2)
#define NE_ELM_locktype (NE_ELM_LOCK_FIRST + 3)
#define NE_ELM_depth (NE_ELM_LOCK_FIRST + 4)
#define NE_ELM_owner (NE_ELM_LOCK_FIRST + 5)
#define NE_ELM_timeout (NE_ELM_LOCK_FIRST + 6)
#define NE_ELM_locktoken (NE_ELM_LOCK_FIRST + 7)
#define NE_ELM_lockinfo (NE_ELM_LOCK_FIRST + 8)
#define NE_ELM_write (NE_ELM_LOCK_FIRST + 9)
#define NE_ELM_exclusive (NE_ELM_LOCK_FIRST + 10)
#define NE_ELM_shared (NE_ELM_LOCK_FIRST + 11)

static const struct ne_xml_elm lock_elms[] = {
#define A(x) { "DAV:", #x, NE_ELM_ ## x, NE_XML_COLLECT } /* ANY */
#define D(x) { "DAV:", #x, NE_ELM_ ## x, 0 }               /* normal */
#define C(x) { "DAV:", #x, NE_ELM_ ## x, NE_XML_CDATA }   /* (#PCDATA) */
#define E(x) { "DAV:", #x, NE_ELM_ ## x, 0 /* LEAF */ }    /* EMPTY */
    D(lockdiscovery), D(activelock),
    D(prop),
    D(lockscope), D(locktype), C(depth), A(owner), C(timeout), D(locktoken),
    /* no lockentry */
    D(lockinfo), D(lockscope), D(locktype),
    E(write), E(exclusive), E(shared),
    C(href),
#undef A
#undef D
#undef C
#undef E
    { NULL, 0, 0 }
};

static const ne_propname lock_props[] = {
    { "DAV:", "lockdiscovery" },
    { NULL }
};

/* this simply registers the accessor for the function. */
static void lk_create(ne_request *req, void *session, 
		       const char *method, const char *uri)
{
    struct lh_req_cookie *lrc = ne_malloc(sizeof *lrc);
    lrc->store = session;
    lrc->submit = NULL;
    ne_set_request_private(req, HOOK_ID, lrc);
}

static void lk_pre_send(ne_request *r, void *userdata, ne_buffer *req)
{
    struct lh_req_cookie *lrc = ne_get_request_private(r, HOOK_ID);

    if (lrc->submit != NULL) {
	struct lock_list *item;

	/* Add in the If header */
	ne_buffer_zappend(req, "If:");
	for (item = lrc->submit; item != NULL; item = item->next) {
	    char *uri = ne_uri_unparse(&item->lock->uri);
	    ne_buffer_concat(req, " <", uri, "> (<",
			     item->lock->token, ">)", NULL);
	    ne_free(uri);
	}
	ne_buffer_zappend(req, EOL);
    }
}

/* Insert 'lock' into lock list *list. */
static void insert_lock(struct lock_list **list, struct ne_lock *lock)
{
    struct lock_list *item = ne_malloc(sizeof *item);
    if (*list != NULL) {
	(*list)->prev = item;
    }
    item->prev = NULL;
    item->next = *list;
    item->lock = lock;
    *list = item;
}

static void free_list(struct lock_list *list, int destroy)
{
    struct lock_list *next;

    while (list != NULL) {
	next = list->next;
	if (destroy)
	    ne_lock_destroy(list->lock);
	ne_free(list);
	list = next;
    }
}

static void lk_destroy(ne_request *req, void *userdata)
{
    struct lh_req_cookie *lrc = ne_get_request_private(req, HOOK_ID);
    free_list(lrc->submit, 0);
    ne_free(lrc);
}

void ne_lockstore_destroy(ne_lock_store *store)
{
    free_list(store->locks, 1);
    ne_free(store);
}

ne_lock_store *ne_lockstore_create(void)
{
    return ne_calloc(sizeof(ne_lock_store));
}

#define CURSOR_RET(s) ((s)->cursor?(s)->cursor->lock:NULL)

struct ne_lock *ne_lockstore_first(ne_lock_store *store)
{
    store->cursor = store->locks;
    return CURSOR_RET(store);
}

struct ne_lock *ne_lockstore_next(ne_lock_store *store)
{
    store->cursor = store->cursor->next;
    return CURSOR_RET(store);
}

void ne_lockstore_register(ne_lock_store *store, ne_session *sess)
{
    /* Register the hooks */
    ne_hook_create_request(sess, lk_create, store);
    ne_hook_pre_send(sess, lk_pre_send, store);
    ne_hook_destroy_request(sess, lk_destroy, store);
}

/* Submit the given lock for the given URI */
static void submit_lock(struct lh_req_cookie *lrc, struct ne_lock *lock)
{
    struct lock_list *item;

    /* Check for dups */
    for (item = lrc->submit; item != NULL; item = item->next) {
	if (strcasecmp(item->lock->token, lock->token) == 0)
	    return;
    }

    insert_lock(&lrc->submit, lock);
}

struct ne_lock *ne_lockstore_findbyuri(ne_lock_store *store,
				       const ne_uri *uri)
{
    struct lock_list *cur;

    for (cur = store->locks; cur != NULL; cur = cur->next) {
	if (ne_uri_cmp(&cur->lock->uri, uri) == 0) {
	    return cur->lock;
	}
    }

    return NULL;
}

void ne_lock_using_parent(ne_request *req, const char *path)
{
    struct lh_req_cookie *lrc = ne_get_request_private(req, HOOK_ID);
    ne_uri u;
    struct lock_list *item;
    char *parent;

    if (lrc == NULL)
	return;
    
    parent = ne_path_parent(path);
    if (parent == NULL)
	return;
    
    u.authinfo = NULL;
    ne_fill_server_uri(ne_get_session(req), &u);

    for (item = lrc->store->locks; item != NULL; item = item->next) {

	/* Only care about locks which are on this server. */
	u.path = item->lock->uri.path;
	if (ne_uri_cmp(&u, &item->lock->uri))
	    continue;
	
	/* This lock is needed if it is an infinite depth lock which
	 * covers the parent, or a lock on the parent itself. */
	if ((item->lock->depth == NE_DEPTH_INFINITE && 
	     ne_path_childof(item->lock->uri.path, parent)) ||
	    ne_path_compare(item->lock->uri.path, parent) == 0) {
	    NE_DEBUG(NE_DBG_LOCKS, "Locked parent, %s on %s\n",
		     item->lock->token, item->lock->uri.path);
	    submit_lock(lrc, item->lock);
	}
    }

    u.path = parent; /* handy: makes u.path valid and ne_free(parent). */
    ne_uri_free(&u);
}

void ne_lock_using_resource(ne_request *req, const char *uri, int depth)
{
    struct lh_req_cookie *lrc = ne_get_request_private(req, HOOK_ID);
    struct lock_list *item;
    int match;

    if (lrc == NULL)
	return;	

    /* Iterate over the list of stored locks to see if any of them
     * apply to this resource */
    for (item = lrc->store->locks; item != NULL; item = item->next) {
	
	match = 0;
	
	if (depth == NE_DEPTH_INFINITE &&
	    ne_path_childof(uri, item->lock->uri.path)) {
	    /* Case 1: this is a depth-infinity request which will 
	     * modify a lock somewhere inside the collection. */
	    NE_DEBUG(NE_DBG_LOCKS, "Has child: %s\n", item->lock->token);
	    match = 1;
	} 
	else if (ne_path_compare(uri, item->lock->uri.path) == 0) {
	    /* Case 2: this request is directly on a locked resource */
	    NE_DEBUG(NE_DBG_LOCKS, "Has direct lock: %s\n", item->lock->token);
	    match = 1;
	}
	else if (item->lock->depth == NE_DEPTH_INFINITE && 
		 ne_path_childof(item->lock->uri.path, uri)) {
	    /* Case 3: there is a higher-up infinite-depth lock which
	     * covers the resource that this request will modify. */
	    NE_DEBUG(NE_DBG_LOCKS, "Is child of: %s\n", item->lock->token);
	    match = 1;
	}
	
	if (match) {
	    submit_lock(lrc, item->lock);
	}
    }

}

void ne_lockstore_add(ne_lock_store *store, struct ne_lock *lock)
{
    insert_lock(&store->locks, lock);
}

void ne_lockstore_remove(ne_lock_store *store, struct ne_lock *lock)
{
    struct lock_list *item;

    /* Find the lock */
    for (item = store->locks; item != NULL; item = item->next)
	if (item->lock == lock)
	    break;
    
    if (item->prev != NULL) {
	item->prev->next = item->next;
    } else {
	store->locks = item->next;
    }
    if (item->next != NULL) {
	item->next->prev = item->prev;
    }
    ne_free(item);
}

struct ne_lock *ne_lock_copy(const struct ne_lock *lock)
{
    struct ne_lock *ret = ne_calloc(sizeof *ret);

    ret->uri.path = ne_strdup(lock->uri.path);
    ret->uri.host = ne_strdup(lock->uri.host);
    ret->uri.scheme = ne_strdup(lock->uri.scheme);
    ret->uri.port = lock->uri.port;
    ret->token = ne_strdup(lock->token);
    ret->depth = lock->depth;
    ret->type = lock->type;
    ret->scope = lock->scope;
    if (lock->owner) ret->owner = ne_strdup(lock->owner);
    ret->timeout = lock->timeout;

    return ret;
}

struct ne_lock *ne_lock_create(void)
{
    struct ne_lock *lock = ne_calloc(sizeof *lock);
    lock->depth = NE_DEPTH_ZERO;
    lock->type = ne_locktype_write;
    lock->scope = ne_lockscope_exclusive;
    lock->timeout = NE_TIMEOUT_INVALID;
    return lock;
}

void ne_lock_free(struct ne_lock *lock)
{
    ne_uri_free(&lock->uri);
    NE_FREE(lock->owner);
    NE_FREE(lock->token);
}

void ne_lock_destroy(struct ne_lock *lock)
{
    ne_lock_free(lock);
    ne_free(lock);
}

int ne_unlock(ne_session *sess, const struct ne_lock *lock)
{
    ne_request *req = ne_request_create(sess, "UNLOCK", lock->uri.path);
    int ret;
    
    ne_print_request_header(req, "Lock-Token", "<%s>", lock->token);
    
    /* UNLOCK of a lock-null resource removes the resource from the
     * parent collection; so an UNLOCK may modify the parent
     * collection. (somewhat counter-intuitive, and not easily derived
     * from 2518.) */
    ne_lock_using_parent(req, lock->uri.path);

    ret = ne_request_dispatch(req);
    
    if (ret == NE_OK && ne_get_status(req)->klass == 2) {
	ret = NE_OK;    
    } else {
	ret = NE_ERROR;
    }

    ne_request_destroy(req);
    
    return ret;
}

static int check_context(void *ud, ne_xml_elmid parent, ne_xml_elmid child)
{
    NE_DEBUG(NE_DBG_XML, "ne_locks: check_context %d in %d\n", child, parent);
    switch (parent) {
    case NE_ELM_root:
	/* TODO: for LOCK requests only...
	 * shouldn't allow this for PROPFIND really */
	if (child == NE_ELM_prop)
	    return NE_XML_VALID;
	break;	    
    case NE_ELM_prop:
	if (child == NE_ELM_lockdiscovery)
	    return NE_XML_VALID;
	break;
    case NE_ELM_lockdiscovery:
	if (child == NE_ELM_activelock)
	    return NE_XML_VALID;
	break;
    case NE_ELM_activelock:
	switch (child) {
	case NE_ELM_lockscope:
	case NE_ELM_locktype:
	case NE_ELM_depth:
	case NE_ELM_owner:
	case NE_ELM_timeout:
	case NE_ELM_locktoken:
	    return NE_XML_VALID;
	default:
	    break;
	}
	break;
    case NE_ELM_lockscope:
	switch (child) {
	case NE_ELM_exclusive:
	case NE_ELM_shared:
	    return NE_XML_VALID;
	default:
	    break;
	}
    case NE_ELM_locktype:
	if (child == NE_ELM_write)
	    return NE_XML_VALID;
	break;
	/* ... depth is PCDATA, owner is COLLECT, timeout is PCDATA */
    case NE_ELM_locktoken:
	if (child == NE_ELM_href)
	    return NE_XML_VALID;
	break;
    }
    return NE_XML_DECLINE;
}

static int parse_depth(const char *depth)
{
    if (strcasecmp(depth, "infinity") == 0) {
	return NE_DEPTH_INFINITE;
    } else if (isdigit(depth[0])) {
	return atoi(depth);
    } else {
	return -1;
    }
}

static long parse_timeout(const char *timeout)
{
    if (strcasecmp(timeout, "infinite") == 0) {
	return NE_TIMEOUT_INFINITE;
    } else if (strncasecmp(timeout, "Second-", 7) == 0) {
	long to = strtol(timeout+7, NULL, 10);
	if (to == LONG_MIN || to == LONG_MAX)
	    return NE_TIMEOUT_INVALID;
	return to;
    } else {
	return NE_TIMEOUT_INVALID;
    }
}

static void discover_results(void *userdata, const char *href,
			     const ne_prop_result_set *set)
{
    struct discover_ctx *ctx = userdata;
    struct ne_lock *lock = ne_propset_private(set);
    const ne_status *status = ne_propset_status(set, &lock_props[0]);

    /* Require at least that the lock has a token. */
    if (lock->token) {
	if (status && status->klass != 2) {
	    ctx->results(ctx->userdata, NULL, lock->uri.path, status);
	} else {
	    ctx->results(ctx->userdata, lock, lock->uri.path, NULL);
	}
    }
    else if (status) {
	ctx->results(ctx->userdata, NULL, href, status);
    }
	
    NE_DEBUG(NE_DBG_LOCKS, "End of response for %s\n", href);
}

static int 
end_element_common(struct ne_lock *l, const struct ne_xml_elm *elm,
		   const char *cdata)
{
    switch (elm->id){ 
    case NE_ELM_write:
	l->type = ne_locktype_write;
	break;
    case NE_ELM_exclusive:
	l->scope = ne_lockscope_exclusive;
	break;
    case NE_ELM_shared:
	l->scope = ne_lockscope_shared;
	break;
    case NE_ELM_depth:
	NE_DEBUG(NE_DBG_LOCKS, "Got depth: %s\n", cdata);
	l->depth = parse_depth(cdata);
	if (l->depth == -1) {
	    return -1;
	}
	break;
    case NE_ELM_timeout:
	NE_DEBUG(NE_DBG_LOCKS, "Got timeout: %s\n", cdata);
	l->timeout = parse_timeout(cdata);
	if (l->timeout == NE_TIMEOUT_INVALID) {
	    return -1;
	}
	break;
    case NE_ELM_owner:
	l->owner = strdup(cdata);
	break;
    case NE_ELM_href:
	l->token = strdup(cdata);
	break;
    }
    return 0;
}

/* End-element handler for lock discovery PROPFIND response */
static int 
end_element_ldisc(void *userdata, const struct ne_xml_elm *elm, 
		  const char *cdata) 
{
    struct ne_lock *lock = ne_propfind_current_private(userdata);

    return end_element_common(lock, elm, cdata);
}

static int lk_startelm(void *userdata, const struct ne_xml_elm *elm,
		       const char **atts)
{
    struct lock_ctx *ctx = userdata;

    /* Lock-Token header is a MUST requirement: if we don't know
     * the response lock, bail out. */
    if (ctx->token == NULL)
	return -1;

    if (elm->id == NE_ELM_activelock && !ctx->found) {
	/* a new activelock */
	ne_lock_free(&ctx->active);
	memset(&ctx->active, 0, sizeof ctx->active);
    }
    
    return 0;
}

/* End-element handler for LOCK response */
static int
lk_endelm(void *userdata, const struct ne_xml_elm *elm, const char *cdata)
{
    struct lock_ctx *ctx = userdata;

    if (ctx->found)
	return 0;

    if (end_element_common(&ctx->active, elm, cdata))
	return -1;

    if (elm->id == NE_ELM_activelock) {
	if (ctx->active.token && strcmp(ctx->active.token, ctx->token) == 0) {
	    ctx->found = 1;
	}
    }

    return 0;
}

static void *ld_alloc(void *userdata, const char *href)
{
    struct discover_ctx *ctx = userdata;
    struct ne_lock *lk = ne_lock_create();

    if (ne_uri_parse(href, &lk->uri) != 0) {
	ne_lock_destroy(lk);
	return NULL;
    }
    
    if (!lk->uri.host)
	ne_fill_server_uri(ctx->session, &lk->uri);

    return lk;
}

static void ld_free(void *userdata, void *lock)
{
    if (lock) ne_lock_destroy(lock);
}

/* Discover all locks on URI */
int ne_lock_discover(ne_session *sess, const char *uri, 
		     ne_lock_result callback, void *userdata)
{
    ne_propfind_handler *handler;
    struct discover_ctx ctx = {0};
    int ret;
    
    ctx.results = callback;
    ctx.userdata = userdata;
    ctx.session = sess;

    handler = ne_propfind_create(sess, uri, NE_DEPTH_ZERO);

    ne_propfind_set_private(handler, ld_alloc, ld_free, &ctx);
    
    ne_xml_push_handler(ne_propfind_get_parser(handler), lock_elms, 
			check_context, NULL, end_element_ldisc, handler);
    
    ret = ne_propfind_named(handler, lock_props, discover_results, &ctx);
    
    ne_propfind_destroy(handler);

    return ret;
}

static void add_timeout_header(ne_request *req, long timeout)
{
    if (timeout == NE_TIMEOUT_INFINITE) {
	ne_add_request_header(req, "Timeout", "Infinite");
    } 
    else if (timeout != NE_TIMEOUT_INVALID && timeout > 0) {
	ne_print_request_header(req, "Timeout", "Second-%ld", timeout);
    }
    /* just ignore it if timeout == 0 or invalid. */
}

/* Parse a Lock-Token response header. */
static void get_ltoken_hdr(void *ud, const char *value)
{
    struct lock_ctx *ctx = ud;
    
    if (value[0] == '<') value++;
    ctx->token = ne_strdup(value);
    ne_shave(ctx->token, ">");
}

int ne_lock(ne_session *sess, struct ne_lock *lock) 
{
    ne_request *req = ne_request_create(sess, "LOCK", lock->uri.path);
    ne_buffer *body = ne_buffer_create();
    ne_xml_parser *parser = ne_xml_create();
    int ret, parse_failed;
    struct lock_ctx ctx;

    memset(&ctx, 0, sizeof ctx);

    ne_xml_push_handler(parser, lock_elms, check_context, 
			lk_startelm, lk_endelm, &ctx);
    
    /* Create the body */
    ne_buffer_concat(body, "<?xml version=\"1.0\" encoding=\"utf-8\"?>" EOL
		    "<lockinfo xmlns='DAV:'>" EOL " <lockscope>",
		    lock->scope==ne_lockscope_exclusive?
		    "<exclusive/>":"<shared/>",
		    "</lockscope>" EOL
		    "<locktype><write/></locktype>", NULL);

    if (lock->owner) {
	ne_buffer_concat(body, "<owner>", lock->owner, "</owner>" EOL, NULL);
    }
    ne_buffer_zappend(body, "</lockinfo>" EOL);

    ne_set_request_body_buffer(req, body->data, ne_buffer_size(body));
    ne_add_response_body_reader(req, ne_accept_2xx, 
				ne_xml_parse_v, parser);
    ne_add_request_header(req, "Content-Type", NE_XML_MEDIA_TYPE);
    ne_add_depth_header(req, lock->depth);
    add_timeout_header(req, lock->timeout);
    
    ne_add_response_header_handler(req, "Lock-Token", get_ltoken_hdr, &ctx);

    /* TODO: 
     * By 2518, we need this only if we are creating a lock-null resource.
     * Since we don't KNOW whether the lock we're given is a lock-null
     * or not, we cover our bases.
     */
    ne_lock_using_parent(req, lock->uri.path);
    /* This one is clearer from 2518 sec 8.10.4. */
    ne_lock_using_resource(req, lock->uri.path, lock->depth);

    ret = ne_request_dispatch(req);

    ne_buffer_destroy(body);
    parse_failed = !ne_xml_valid(parser);
    
    if (ret == NE_OK && ne_get_status(req)->klass == 2) {
	if (ctx.token == NULL) {
	    ret = NE_ERROR;
	    ne_set_error(sess, _("No Lock-Token header given"));
	}
	else if (parse_failed) {
	    ret = NE_ERROR;
	    ne_set_error(sess, ne_xml_get_error(parser));
	}
	else if (ne_get_status(req)->code == 207) {
	    ret = NE_ERROR;
	    /* TODO: set the error string appropriately */
	}
	else if (ctx.found) {
	    /* it worked: copy over real lock details if given. */
	    NE_FREE(lock->token);
	    lock->token = ctx.token;
	    ctx.token = NULL;
	    if (ctx.active.timeout != NE_TIMEOUT_INVALID)
		lock->timeout = ctx.active.timeout;
	    lock->scope = ctx.active.scope;
	    lock->type = ctx.active.type;
	    if (ctx.active.depth >= 0)
		lock->depth = ctx.active.depth;
	    if (ctx.active.owner) {
		NE_FREE(lock->owner);
		lock->owner = ctx.active.owner;
		ctx.active.owner = NULL;
	    }
	} else {
	    ret = NE_ERROR;
	    ne_set_error(sess, _("Response missing activelock for %s"), 
			 ctx.token);
	}
    } else {
	ret = NE_ERROR;
    }

    if (ctx.token)
	ne_free(ctx.token);
    ne_lock_free(&ctx.active);

    ne_request_destroy(req);
    ne_xml_destroy(parser);

    return ret;
}

int ne_lock_refresh(ne_session *sess, struct ne_lock *lock)
{
    ne_request *req = ne_request_create(sess, "LOCK", lock->uri.path);
    ne_xml_parser *parser = ne_xml_create();
    int ret, parse_failed;

    /* Handle the response and update *lock appropriately. */
    ne_xml_push_handler(parser, lock_elms, check_context, 
			 NULL, lk_endelm, lock);
    
    ne_add_response_body_reader(req, ne_accept_2xx, 
				ne_xml_parse_v, parser);

    /* Probably don't need to submit any other lock-tokens for this
     * resource? If it's an exclusive lock, then there can be no other
     * locks covering the resource. If it's a shared lock, then this
     * lock (which is being refreshed) is sufficient to modify the
     * resource state? */
    ne_print_request_header(req, "If", "(<%s>)", lock->token);
    add_timeout_header(req, lock->timeout);

    ret = ne_request_dispatch(req);

    parse_failed = !ne_xml_valid(parser);
    
    if (ret == NE_OK && ne_get_status(req)->klass == 2) {
	if (parse_failed) {
	    ret = NE_ERROR;
	    ne_set_error(sess, ne_xml_get_error(parser));
	}
	else if (ne_get_status(req)->code == 207) {
	    ret = NE_ERROR;
	    /* TODO: set the error string appropriately */
	}
    } else {
	ret = NE_ERROR;
    }

    ne_request_destroy(req);
    ne_xml_destroy(parser);

    return ret;
}
