
#include "config.h"

#include <sys/types.h>
#include <sys/time.h>

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <fcntl.h>

#include <ne_request.h>
#include <ne_string.h>
#include <ne_props.h>
#include <ne_uri.h>
#include <ne_locks.h>

#include "common.h"

static char buff[1024];
static struct ne_lock reslock;

extern struct timeval g_tv1, g_tv2;

/* BINARYMODE() enables binary file I/O on cygwin. */
#ifdef __CYGWIN__
#define BINARYMODE(fd) do { setmode(fd, O_BINARY); } while (0)
#else
#define BINARYMODE(fd) if (0)
#endif

#define MAXNP 1024
static ne_propname propnames[MAXNP+1];
static char *values[MAXNP+1];


static char *create_temp(const char *contents, int fsize)
{
    char tmp[256] = "/tmp/Davtest-XXXXXX";
    char str[32]= "This is Prestan test file.\n";
    int fd;
    int i;


    for ( i=0; i<32; i++)
	strcpy(&buff[i*32], str);

    fd = mkstemp(tmp);
    BINARYMODE(fd);

    for( i=0; i<fsize; i++)
    	write(fd, buff, sizeof(buff));

    close(fd);

    return ne_strdup(tmp);
}

static const char *test_contents = ""
"This is\n"
"a test file.\n"
"for Prestan\n"
"testing.\n";

static char *pg_uri = NULL;

static int do_put_get(const char *segment, int fsize)
{
    char *fn, tmp[] = "/tmp/Davtest-XXXXXX", *uri;
    char str[64];
    int fd, res;
    
    uri = ne_concat(i_path, segment, NULL);
    fn = create_temp(test_contents, fsize);
    fd = open(fn, O_RDONLY | O_BINARY);

    /* PUT METHOD */
    SEND_REQUEST(ne_put(i_session, uri, fd));
    memset(str, 0, sizeof(str));
    sprintf(str, "Put%dK", fsize);
    my_printf(str);

    close(fd);

    /* GET METHOD*/
    fd = mkstemp(tmp);
    BINARYMODE(fd);

    SEND_REQUEST(ne_get(i_session, uri, fd));
    memset(str, 0, sizeof(str));
    sprintf(str, "Get%dK", fsize);
    my_printf(str);
 
    close(fd);

    /* Clean up. */
    unlink(fn);
    unlink(tmp);

    return OK;
}


int put_get1K(void)
{
    return do_put_get("res", 1);
}

int put_get64K(void)
{
    return do_put_get("res", 64);
}

int put_get1024K(void)
{
    return do_put_get("res", 1024);
}


int
my_single(void)
{
   char *dest, *dest2, *uri;

   uri = ne_concat(i_path, "move", NULL);
   dest = ne_concat(i_path, "movedest", NULL);
   dest2 = ne_concat(i_path, "movedest2", NULL);

   /* single resource testing */
   CALL(upload_foo("move"));

   /* copy */
   SEND_REQUEST(ne_copy(i_session, 1, NE_DEPTH_INFINITE, uri, dest));
   my_printf("Copy");
 

   /* move */
   SEND_REQUEST2(ne_copy(i_session, 1, NE_DEPTH_INFINITE, uri, dest),
	       ne_move(i_session, 1, dest, dest2));
   my_printf("Move");

   /* delete */
   SEND_REQUEST2(ne_copy(i_session, 1, NE_DEPTH_INFINITE, uri, dest),
   	ne_delete(i_session, dest));
   my_printf("Delete");
} 

int
my_collection(void)
{
   char *dest, *dest2, *uri;


   dest = ne_concat(i_path, "dest/", NULL);
   dest2 = ne_concat(i_path, "dest2/", NULL);
   uri = ne_concat(i_path, "coll/", NULL);


   /* 			 *
    * Collection Testing * 
    *			 */
    /* Mkcol */
   SEND_REQUEST3(ne_mkcol(i_session, uri), ne_delete(i_session, uri));
   my_printf("MkCol");
 

   /* copycol */
   uri = ne_concat(i_path, "coll2/", NULL);
   ONV( ne_mkcol(i_session, uri),
	("MKCOL %s: %s", uri, ne_get_error(i_session)));
   my_mkcol2(uri, pget_option.depth);
 

   SEND_REQUEST(ne_copy(i_session, 1, NE_DEPTH_INFINITE, uri, dest));
   my_printf("CopyCol");
  
  /* movecol */
   SEND_REQUEST2(ne_copy(i_session, 1, NE_DEPTH_INFINITE, uri, dest), 
   				ne_move(i_session, 1, dest, dest2));
   my_printf("MoveCol");

  /* deletecol */
   SEND_REQUEST2(ne_copy(i_session, 1, NE_DEPTH_INFINITE, uri, dest), 
   						ne_delete(i_session, dest));
   my_printf("DeleteCol");

   ne_delete(i_session, uri);
}



static int wf_do_put_get(const char *segment, int fsize)
{
    char *fn, tmp[] = "/tmp/Davtest-XXXXXX", *uri;
    char str[64];
    int fd, res;
    
    uri = ne_concat(i_path, segment, NULL);
    fn = create_temp(test_contents, fsize);
    fd = open(fn, O_RDONLY | O_BINARY);

    /* PUT METHOD */
    SEND_REQUEST(ne_put(i_session, uri, fd));
    SEND_REQUEST_TWO(ne_head(i_session, uri), ne_put(i_session, uri, fd));

    memset(str, 0, sizeof(str));
    sprintf(str, "Put%dK", fsize);
    my_printf(str);

    close(fd);

    /* GET METHOD*/
    fd = mkstemp(tmp);
    BINARYMODE(fd);

    SEND_REQUEST(ne_get(i_session, uri, fd));
    memset(str, 0, sizeof(str));
    sprintf(str, "Get%dK", fsize);
    my_printf(str);
 
    close(fd);

    /* Clean up. */
    unlink(fn);
    unlink(tmp);

    return OK;
}


int wf_put_get1K(void)
{
    return wf_do_put_get("res", 1);
}


int
wf_my_single(void)
{
   char *dest, *dest2, *uri, *uri2, *res;
   ne_server_capabilities caps = {0};
   char tmp[100], buffer[128];
   char str1[32], str2[32];
    char *fn, tmp2[] = "/tmp/Davtest-XXXXXX";
   int n, fd, fsize;


	/*
	 * Prepare the properties
	 */

    for (n = 0; n <10 ; n++) {
	propnames[n].nspace = "DAV:";
	propnames[n].name = ne_strdup(tmp);
	values[n] = ne_strdup("17");;
    }

    sprintf(tmp, "creationdate");
    propnames[0].name = ne_strdup(tmp);
	
    sprintf(tmp, "getlastmodified");
    propnames[1].name = ne_strdup(tmp);

    sprintf(tmp, "resourcetype");
    propnames[2].name = ne_strdup(tmp);

    sprintf(tmp, "supportedlock");
    propnames[3].name = ne_strdup(tmp);

    sprintf(tmp, "lockdiscovery");
    propnames[4].name = ne_strdup(tmp);

    sprintf(tmp, "getcontentlength");
    propnames[5].name = ne_strdup(tmp);

    sprintf(tmp, "getetag");
    propnames[6].name = ne_strdup(tmp);

    sprintf(tmp, "getcontentlanguage");
    propnames[7].name = ne_strdup(tmp);

    sprintf(tmp, "getcontenttype");
    propnames[8].name = ne_strdup(tmp);

    sprintf(tmp, "supportedlock");
    propnames[9].name = ne_strdup(tmp);

    sprintf(tmp, "source");
    propnames[10].name = ne_strdup(tmp);

	/* ** */

   dest = ne_concat(i_path, "movedest", NULL);
   dest2 = ne_concat(i_path, "movedest2", NULL);
   uri = ne_concat(i_path, "coll/", NULL);
   uri2 = ne_concat(i_path, "", NULL);

   /* single resource testing */
   CALL(upload_foo("move"));

    /* Mkcol */
   SEND_REQUEST3_FOUR(ne_simple_propfind(i_session, uri2, NE_DEPTH_ZERO, 
			propnames,NULL, NULL),
		ne_mkcol(i_session, uri), 
    		ne_simple_propfind(i_session, uri, NE_DEPTH_ZERO,
			propnames, NULL, NULL),
		ne_delete(i_session, uri));

   my_printf("MkCol");


   uri = ne_concat(i_path, "move", NULL);

   /* copy */
   SEND_REQUEST_TWO(ne_simple_propfind(i_session, dest, NE_DEPTH_ZERO,
			propnames, NULL, NULL),
		ne_copy(i_session, 1, NE_DEPTH_INFINITE, uri, dest));

   my_printf("Copy");
 

   /* move */
   SEND_REQUEST2_THREE(ne_copy(i_session, 1, NE_DEPTH_INFINITE, uri, dest),
		ne_simple_propfind(i_session, dest2, NE_DEPTH_ZERO,
			propnames, NULL, NULL),
	       ne_move(i_session, 1, dest, dest2));

   my_printf("Move");

   /* delete */
   SEND_REQUEST2_THREE(ne_copy(i_session, 1, NE_DEPTH_INFINITE, uri, dest),
		ne_simple_propfind(i_session, dest, NE_DEPTH_ZERO,
			propnames, NULL, NULL),
   		ne_delete(i_session, dest));

   my_printf("Delete");


    /* Co_Authoring Methods */
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

   /* open */
    fd = mkstemp(tmp2);
    BINARYMODE(fd);

    SEND_REQUEST3_FOUR(
		ne_options(i_session, i_path, &caps),
		ne_lock(i_session, &reslock), 
		ne_get(i_session, res, fd),
		ne_unlock(i_session, &reslock));

    my_printf("Open");

   /* close */
    fn = create_temp(test_contents, 1024);
    fd = open(fn, O_RDONLY | O_BINARY);

    SEND_REQUEST2_FOUR(
		ne_lock(i_session, &reslock), 
		ne_put(i_session, res, fd),
		ne_lock(i_session, &reslock), 
		ne_unlock(i_session, &reslock));
    my_printf("Close");

   /* mount */
    strcpy(str1, "/_vti_inf.html");
    strcpy(str2, "/_vti_bin/shtml.exe/_vti_rpc");
    SEND_REQUEST_FOUR(
		ne_options(i_session, i_path, &caps),
		ne_get(i_session, str1, fd),
		ne_post(i_session, str2, fd, buffer),
		ne_simple_propfind(i_session, i_path, NE_DEPTH_ZERO,
			propnames, NULL, NULL)
		);

   my_printf("Mount");
} 

