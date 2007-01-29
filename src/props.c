
#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#include <ne_request.h>
#include <ne_props.h>
#include <ne_uri.h>

#include "common.h"

#define NS "http://webdav.org/neon/DavTester/"

#define NSPACE(x) ((x) ? (x) : "")

extern char *g_methodlist[];

static const ne_propname props[] = {
    { "DAV:", "getcontentlength" },
    { "DAV:", "getlastmodified" },
    { "DAV:", "displayname" },
    { "DAV:", "resourcetype" },
    { NS, "foo" },
    { NS, "bar" },
    { NULL }
};

#define ELM_resourcetype (NE_ELM_207_UNUSED + 1)
#define ELM_collection (NE_ELM_207_UNUSED + 4)

static const struct ne_xml_elm complex_elms[] = {
    { "DAV:", "resourcetype", ELM_resourcetype, 0 },
    { "DAV:", "collection", ELM_collection, 0 },
    { NULL }
};

struct private {
    int collection;
};

struct results {
    ne_propfind_handler *ph;
    int result;
};


static int prop_ok = 0;
char *prop_uri, *prop_uri2, *prop_uri3;

int propinit(void)
{
    srandom(123456);
    prop_uri = ne_concat(i_path, "prop", NULL);
    
    ne_delete(i_session, prop_uri);

    CALL(upload_foo("prop"));

    prop_ok = 1;
    
    return OK;
}

#define MAXNP 1024

static ne_proppatch_operation pops[MAXNP + 1];
static ne_propname propnames[MAXNP + 1];
static char *values[MAXNP + 1];

extern int numprops, removedprops;
extern struct timeval g_tv1, g_tv2;
extern int *g_intp;

#define PS_VALUE "value goes here"

int proppatch(void)
{
    int n;
    char tmp[100];

    memset(tmp, 0, sizeof(tmp));
    sprintf(tmp, "prop_%d", random());
    prop_uri = ne_concat(i_path, tmp, NULL);
    ne_delete(i_session, prop_uri);
    CALL(upload_foo(tmp));


    memset(tmp, 0, sizeof(tmp));
    sprintf(tmp, "prop_%d", random());
    prop_uri2 = ne_concat(i_path, tmp, NULL);
    ne_delete(i_session, prop_uri2);
    CALL(upload_foo(tmp));

    /* multiple property */
    for (n = 0; n < numprops; n++) {
	sprintf(tmp, "prop%d", n);
	propnames[n].nspace = NS;
	propnames[n].name = ne_strdup(tmp);
	pops[n].name = &propnames[n];
	pops[n].type = ne_propset;
	sprintf(tmp, "value%d", n);
	values[n] = ne_strdup(tmp);
	pops[n].value = values[n];
    }

    memset(&pops[n], 0, sizeof(pops[n]));
    memset(&propnames[n], 0, sizeof(propnames[n]));	   
    values[n] = NULL;

    SEND_REQUEST(ne_proppatch(i_session, prop_uri, pops));
    my_printf("ProppatchMult");

    /* singel property */
    n = 1;	
    memset(&pops[n], 0, sizeof(pops[n]));
    memset(&propnames[n], 0, sizeof(propnames[n]));	   
    values[n] = NULL;

    SEND_REQUEST(ne_proppatch(i_session, prop_uri2, pops));
    my_printf("ProppatchSingle");

    return OK;
}

static void
my_pg_results(void *userdata, const char *uri,
		const ne_prop_result_set *rset)
{
    struct results *r = userdata;

    r->result = FAIL;

}

int propfinddead(void)
{
    int n;
    char tmp[128]; 
    int ret;
    char *prop_uri2;

    struct results r = {0};

    PRECOND(prop_ok);

    r.result = 1;
    t_context("No responses returned");

    for (n = 0; n < numprops; n++) {
	sprintf(tmp, "prop%d", n);
	propnames[n].nspace = NS;
	propnames[n].name = ne_strdup(tmp);
	pops[n].name = &propnames[n];
	pops[n].type = ne_propset;
	sprintf(tmp, "value%d", n);
	values[n] = ne_strdup(tmp);
	pops[n].value = values[n];
    }

    memset(&pops[n], 0, sizeof(pops[n]));
    memset(&propnames[n], 0, sizeof(propnames[n]));	   
    values[n] = NULL;

    prop_uri = ne_concat(i_path, "prop", NULL);
    
    ne_delete(i_session, prop_uri);

    CALL(upload_foo("prop"));

    prop_uri2 = ne_concat(i_path, "prop2", NULL);
    
    ne_delete(i_session, prop_uri2);

    CALL(upload_foo("prop2"));

    ne_proppatch(i_session, prop_uri, pops);
    ne_proppatch(i_session, prop_uri2, pops);


     /* multiple dead properties */
    SEND_REQUEST(ne_simple_propfind(i_session, prop_uri, 
    		NE_DEPTH_ZERO,propnames, my_pg_results, &r));
    my_printf("PropfindDeadMult");


    /* single dead property */
    n = 1;
    memset(&pops[n], 0, sizeof(pops[n]));
    memset(&propnames[n], 0, sizeof(propnames[n]));	   
    values[n] = NULL;

    SEND_REQUEST(ne_simple_propfind(i_session, prop_uri2, 
    		NE_DEPTH_ZERO,propnames, my_pg_results, &r));
    my_printf("PropfindDeadSingle");


    return OK;
}


int propfindlive(void)
{
    int n;
    char tmp[128]; 
    int ret;
    char *prop_uri2;

    struct results r = {0};

    PRECOND(prop_ok);

    r.result = 1;
    t_context("No responses returned");

    sprintf(tmp, "prop_%d", random());
    prop_uri = ne_concat(i_path, tmp, NULL);
    
    ne_delete(i_session, prop_uri);

    CALL(upload_foo(tmp));

    sprintf(tmp, "prop_%d", random());
    prop_uri2 = ne_concat(i_path, tmp, NULL);
    
    ne_delete(i_session, prop_uri2);

    CALL(upload_foo(tmp));

    memset(tmp, 0, sizeof(tmp));


    /* multiple live properties */

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

    memset(&propnames[n], 0, sizeof(propnames[n]));	   

    SEND_REQUEST(ne_simple_propfind(i_session, prop_uri2, 
    			NE_DEPTH_ZERO,propnames, my_pg_results, &r));
    my_printf("PropfindLiveMult");


    /* single live property */
    for (n = 0; n < 1; n++) {
	propnames[n].nspace = "DAV:";
	sprintf(tmp, "creationdate");
	propnames[n].name = ne_strdup(tmp);
	values[n] = ne_strdup("17");;
    }
    memset(&propnames[n], 0, sizeof(propnames[n]));	   


    SEND_REQUEST(ne_simple_propfind(i_session, prop_uri2, 
    			NE_DEPTH_ZERO,propnames, my_pg_results, &r));
    my_printf("PropfindLiveSingle");


    return OK;
}

