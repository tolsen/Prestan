
#include "config.h"

#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>

#include <ne_props.h>
#include <ne_uri.h>
#include <ne_locks.h>

#include "common.h"

static char *res, *res2;
static ne_lock_store *store;
extern struct timeval g_tv1, g_tv2;

static struct ne_lock reslock, *gotlock = NULL;

static int precond(void)
{
    if (!i_class2) {
	t_context("locking tests skipped,\n"
		  "server does not claim Class 2 compliance");
	return SKIPREST;
    }
    
    return OK;
}

/* Get a lock, store pointer in global 'getlock'. */
int locks(void)
{
    struct timeval tv1, tv2;

    res = ne_concat(i_path, "lockme", NULL);
    CALL(upload_foo("lockme"));

    memset(&reslock, 0, sizeof(reslock));

    ne_fill_server_uri(i_session, &reslock.uri);
    reslock.uri.path = res;

    reslock.depth = NE_DEPTH_ZERO;
    reslock.scope = ne_lockscope_exclusive;
    reslock.type = ne_locktype_write;
    reslock.timeout = 3600;
    reslock.owner = ne_strdup("Prestan test suite");

    /* Lock single */
    SEND_REQUEST3(ne_lock(i_session, &reslock), ne_unlock(i_session, &reslock));
    my_printf("Lock");

    /* Unlock single */
    SEND_REQUEST2(ne_lock(i_session, &reslock), ne_unlock(i_session, &reslock));
    my_printf("UnLock");

    /* Collection Lock */
    res = ne_concat(i_path, "lockme2/", NULL);
    ONV(ne_mkcol(i_session, res),
       ("MKCOL %s %s", res, ne_get_error(i_session)));
    my_mkcol2( res, pget_option.depth);
    reslock.uri.path = res;
    reslock.depth = NE_DEPTH_INFINITE;

    /* Lock Collection */
    SEND_REQUEST3(ne_lock(i_session, &reslock), ne_unlock(i_session, &reslock));
    my_printf("LockCol");


    /* Unlock Collection */
    SEND_REQUEST2(ne_lock(i_session, &reslock), ne_unlock(i_session, &reslock));
    my_printf("UnLockCol");

    ne_delete(i_session, res);

    return OK;
}


