
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/mman.h>
#include <config.h>
#include <ne_props.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include <ne_uri.h>
#include <ne_auth.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "ne_utils.h"
#include "ne_socket.h"
#include "getopt.h"
#include "common.h"
#include "config.h"

extern int proppatch(void);

int g_echo = 1;
int l_msize;
char *l_p;

int numprops, removedprops;
extern struct timeval g_tv1, g_tv2;

int i_class2 = 0;

volatile int *g_intp;
ne_session *i_session, *i_session2, *tmp_session;

const char *i_hostname;
int i_port;
ne_sock_addr *i_address;
char *i_path;

static int use_secure = 0;

const char *i_username = NULL, *i_password;

static char *htdocs_root = NULL;

static char *proxy_hostname = NULL;
static int proxy_port;

int i_foo_fd;


static char dots[] = "...................";
char* Result_Header[] = {"Prestan, Version 2.0.3",
			"Copyright(c) 2003 Teng Xu, GRASE Research Group at UCSC", "http://www.soe.ucsc.edu/research/labs/grase",
			NULL};

const static struct option longopts[] = {
    { "htdocs", required_argument, NULL, 'd' },
    { "help", no_argument, NULL, 'h' },
    { "proxy", required_argument, NULL, 'p' },
#if 0
    { "colour", no_argument, NULL, 'c' },
    { "no-colour", no_argument, NULL, 'n' },
#endif
    { NULL }
};


#define MAXCHILD 256
pid_t childpid[MAXCHILD];

int time_comp(const void *t1, const void *t2)
{
    return (*(float*)t1 > *(float*)t2);
}

/* Return: number of valid elements */
int time_filter(float a[], float b[], int nelm, float median, float percentage)
{
    int i, j;
    float bound;

    bound = median * percentage;
    for( i=j=0; i<nelm; i++){
	if ( abs((int)(a[i]-median))< bound){
	    b[j] = a[i];
	    j++;	
	}
    }
    return j;
}

static int open_foo(void)
{
    char *foofn = ne_concat(htdocs_root, "/foo", NULL);
    i_foo_fd = open(foofn, O_RDONLY | O_BINARY);
    if (i_foo_fd < 0) {
	t_context("could not open %s: %s", foofn, strerror(errno));
	return FAILHARD;
    }
    return OK;
}

static int test_connect(void)
{
    const ne_inet_addr *ia;
    ne_socket *sock = NULL;
    unsigned int port = proxy_hostname ? proxy_port : i_port;

    for (ia = ne_addr_first(i_address); ia && !sock; 
	 ia = ne_addr_next(i_address))
	sock = ne_sock_connect(ia, port);
    
    if (sock == NULL) {
	t_context("connection refused by `%s' port %d",
		  i_hostname, port);
	return FAILHARD;
    }
    ne_sock_close(sock);
    return OK;
}

static int test_resolve(const char *hostname, const char *name)
{
    i_address = ne_addr_resolve(hostname, 0);
    if (ne_addr_result(i_address)) {
	char buf[256];
	t_context("%s hostname `%s' lookup failed: %s", name, hostname,
		  ne_addr_error(i_address, buf, sizeof buf));
	return FAILHARD;
    }
    return OK;
}

int init(void)
{
    ne_uri u = {0}, proxy = {0};
    int optc, n;
    char *proxy_url = NULL;
    char str[64], *src;
    int i;


    if ((times1=malloc(sizeof(float)*pget_option.requests)) == NULL){
	perror("malloc(times1) :");
	exit(-1);
    }	
    if ((times2=malloc(sizeof(float)*pget_option.requests)) == NULL){
	perror("malloc(times2) :");
	exit(-1);
    }	
	
    while ((optc = getopt_long(test_argc, test_argv, 
			       "d:hp", longopts, NULL)) != -1) {
	switch (optc) {
	case 'd':
	    htdocs_root = optarg;
	    break;
	case 'p':
	    proxy_url = optarg;
	    break;
	default:
	    exit(1);
	}
    }

    n = test_argc - optind;

    if (n == 0 || n > 3 || n == 2) {
	exit(1);
    }

    if (htdocs_root == NULL)
	htdocs_root = "htdocs";

    if (ne_uri_parse(test_argv[optind], &u)) {
	t_context("couldn't parse server URL `%s'",
		  test_argv[optind]);
	return FAILHARD;
    }       

    if (proxy_url) {
	if (ne_uri_parse(proxy_url, &proxy)) {
	    t_context("couldn't parse proxy URL `%s'", proxy_url);
	    return FAILHARD;
	}
	if (proxy.scheme && strcmp(proxy.scheme, "http") != 0) {
	    t_context("cannot use scheme `%s' for proxy", proxy.scheme);
	    return FAILHARD;
	}
	if (proxy.port > 0) {
	    proxy_port = proxy.port;
	} else {
	    proxy_port = 8080;
	}
	proxy_hostname = proxy.host;
    }		      

    if (u.scheme && strcmp(u.scheme, "https") == 0)
	use_secure = 1;

    i_hostname = u.host;
    if (u.port > 0) {
	i_port = u.port;
    } else {
	if (use_secure) {
	    i_port = 443;
	} else {
	    i_port = 80;
	}
    }
    if (ne_path_has_trailing_slash(u.path)) {
	i_path = u.path;
    } else {
	i_path = ne_concat(u.path, "/", NULL);
    }

    i_path = ne_path_escape(i_path);
    
    if (n > 2) {
	i_username = test_argv[optind+1];
	i_password = test_argv[optind+2];
	
	if (strlen(i_username) >= NE_ABUFSIZ) {
	    t_context("username must be <%d chars", NE_ABUFSIZ);
	    return FAILHARD;
	}

	if (strlen(i_password) >= NE_ABUFSIZ) {
	    t_context("password must be <%d chars", NE_ABUFSIZ);
	    return FAILHARD;
	}
    }
    
    if (proxy_hostname)
	CALL(test_resolve(proxy_hostname, "proxy server"));
    else
	CALL(test_resolve(i_hostname, "server"));

    CALL(open_foo());

    CALL(test_connect());

    printf("Done\n");
    return OK;
}

static int auth(void *ud, const char *realm, int attempt,
		char *username, char *password)
{
    strcpy(username, i_username);
    strcpy(password, i_password);
    return attempt;
}

static void i_pre_send(ne_request *req, void *userdata, ne_buffer *hdr)
{
    char buf[BUFSIZ];
    const char *name = userdata;
    
    ne_snprintf(buf, BUFSIZ, "%s: %s: %d (%s)\r\n", 
		name, test_suite, test_num, tests[test_num].name);
    
    ne_buffer_zappend(hdr, buf);
}

/* Allow all certificates. */
static int ignore_verify(void *ud, int fs, const ne_ssl_certificate *cert)
{
    return 0;
}

static int init_session(ne_session *sess)
{
    if (proxy_hostname) {
	ne_session_proxy(sess, proxy_hostname, proxy_port);
    }

    ne_set_useragent(i_session, "davtest/" PACKAGE_VERSION);

    if (i_username) {
	ne_set_server_auth(sess, auth, NULL);
    }

    if (use_secure) {
	if (!ne_supports_ssl()) {
	    t_context("No SSL support, reconfigure using --with-ssl");
	    return FAILHARD;
	} else {
	    ne_ssl_set_verify(sess, ignore_verify, NULL);
	}
    }
    
    return OK;
}    

int warmup(void)
{
    char str[64], *src;
    struct timeval start, cur;
    int old_requests;

	g_echo = 0;

	memset(str, 0, 64);
	memset(str, '.', 40);
	src = "Server Warming Up";
	strncpy(str,src,strlen(src));

	printf("\n%s", str);

	fflush(stdout);

	gettimeofday(&start, NULL);
	gettimeofday(&cur, NULL);
	old_requests = pget_option.requests;
	pget_option.requests = 10;
	while ( (cur.tv_sec - start.tv_sec) < WARMUP_TIME){

		proppatch();
		propfinddead();
		propfindlive();
		put_get1K();
		put_get64K();
		//put_get1024K();
		my_single();
		my_collection();
		locks();

		gettimeofday(&cur, NULL);
	}

	pget_option.requests = old_requests;
	printf("Done\n");
	g_echo = 1;
}

int begin(void)
{
    const char *scheme = use_secure?"https":"http";
    char *space;
    static const char blanks[] = "          ";
    static const char stars[] = "**********************************";
    

    i_session = ne_session_create(scheme, i_hostname, i_port);
    CALL(init_session(i_session));
    ne_hook_pre_send(i_session, i_pre_send, "X-Prestan");


    space = ne_concat(i_path, "davtest/", NULL);
    ne_delete(i_session, space);
    if (ne_mkcol(i_session, space)) {
	t_context("Could not create new collection `%s' for tests: %s\n"
	  "Server must allow `MKCOL %s' for tests to proceed", 
	  space, ne_get_error(i_session), space);
	return FAILHARD;
    }
    free(i_path);
    i_path = space;    
    
    warmup();

    printf("\nStart Testing %s:\n\n",pget_option.URL) ;
    printf("\n%s%s\n", blanks, stars);
    printf("\n%s* Number of Requests\t\t%d\n", blanks, pget_option.requests);
    printf("\n%s* Number of Dead Properties\t%d\n", 
    			blanks, pget_option.numprops);
    printf("\n%s* Depth of Collection\t\t%d\n", blanks, pget_option.depth);
    printf("\n%s* Width of Collection\t\t%d\n", blanks, pget_option.width);
    printf("\n%s* Type of Methods\t\t%s\n", blanks, pget_option.methods);
    printf("\n%s%s\n", blanks, stars);
    printf("\n\n");
    
    return OK;
}

int finish(void)
{
    ne_delete(i_session, i_path);
    ne_session_destroy(i_session);
    printf("\n\n");
    return OK;
}

int upload_foo(const char *path)
{
    char *uri = ne_concat(i_path, path, NULL);
    int ret;
    /* i_foo_fd is rewound automagically by ne_request.c */
    ret = ne_put(i_session, uri, i_foo_fd);
    free(uri);
    if (ret)
	t_context("PUT of `%s': %s", uri, ne_get_error(i_session));
    return ret;
}

inline int
latency(struct timeval tv1, struct timeval  tv2)
{
    return ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec));
}


int
my_mkcol(char* uri, int depth)
{
	char myuri[128];
	int i, ret;

	if ( depth > 1){
		memset(myuri, 0, sizeof(myuri));
		strcpy( myuri, uri);
		sprintf( myuri, "%ssub/", myuri);
    		if (ret=ne_mkcol(i_session, myuri)){
		    printf("my_mkcol failed 1\n");
		    return ret;
		}
		my_mkcol( myuri, depth-1);
	}else if ( depth == 1)
		for ( i=0; i<pget_option.width; i++){
			memset(myuri, 0, sizeof(myuri));
			strcpy( myuri, uri);
			sprintf( myuri, "%ssub%d/", myuri, i);
    			if (ret=ne_mkcol(i_session, myuri)){
			    return ret;
		    		printf("my_mkcol failed 2\n");
			}
		}

	return ret;
}


/* 
 * create a hierarchy of 10 levels deep, NBOTTOM files at bottom level
 */
void
my_mkcol2(char* uri, int depth)
{
	char tmp[256] = "/tmp/Davtest-XXXXXX";
	char myuri[128];
	int i, fd;

	if ( (fd=mkstemp(tmp)) < 0){
		perror("mkstemp() :");
		return ;
	}

	for( i=0; i<4; i++)
		write(fd, tmp, sizeof(tmp));

	if ( depth > 1){
		memset(myuri, 0, sizeof(myuri));
		strcpy( myuri, uri);
		sprintf( myuri, "%ssub/", myuri);
    		if (ne_mkcol(i_session, myuri)){
			printf("ne_mkcol failed\n");
			return;
		}
		# Return error if myuri + "sub/" is > 128
		if (strlen(myuri) >= sizeof(myuri) - 4) {
			printf("\nERROR: max depth reached.\n");	
			exit(1);
		} else {
			my_mkcol2( myuri, depth-1);
		}
	}else if ( depth == 1)
		for ( i=0; i<pget_option.width; i++){
			memset(myuri, 0, sizeof(myuri));
			strcpy( myuri, uri);
			sprintf( myuri, "%sfile%d", myuri, i);
    			if (ne_put(i_session, myuri, fd)){
				printf("ne_put failed\n");
				return;
			}
		}

	unlink(tmp);

	return;
}

void
my_mkcol2_proppatch(char* uri, int depth, const ne_propname pops[])
{
	char tmp[256] = "/tmp/Davtest-XXXXXX";
	char myuri[128];
	int i, fd;

	if ( (fd=mkstemp(tmp)) < 0){
		perror("mkstemp() :");
		return ;
	}

	for( i=0; i<4; i++)
		write(fd, tmp, sizeof(tmp));

	if ( depth > 1){
		memset(myuri, 0, sizeof(myuri));
		strcpy( myuri, uri);
		sprintf( myuri, "%ssub/", myuri);
    		ONV(ne_mkcol(i_session, myuri), 
			("MKCOL %s: %s", myuri, ne_get_error(i_session)));
		ONMREQ("PROPPATCH",myuri, ne_proppatch(i_session, myuri, pops));
		my_mkcol2( myuri, depth-1);
	}else if ( depth == 1)
		for ( i=0; i<pget_option.width; i++){
			memset(myuri, 0, sizeof(myuri));
			strcpy( myuri, uri);
			sprintf( myuri, "%sfile%d", myuri, i);
    			ONV(ne_put(i_session, myuri, fd),
				("PUT of %s: %s", myuri, ne_get_error(i_session)));
		}

	return;
}


void my_printf(char *src)
{
	char tmp[64];

    if ( g_echo ){	

	memset(tmp, 0, 64);
	memset(tmp, '.', 30);
	strncpy(tmp, src, strlen(src));
	printf("\n%s Rsp = %.0f [us]\n", tmp, g_average);

    }
}



char test_context[BUFSIZ];
int have_context = 0;

static FILE *child_debug, *debug;

char **test_argv;
int test_argc;

const char *test_suite;
int test_num;

/* statistics for all tests so far */
static int passes = 0, fails = 0, skipped = 0, warnings = 0;

/* per-test globals: */
static int warned, aborted = 0;
static const char *test_name; /* current test name */

static int use_colour = 0;

/* resource for ANSI escape codes:
 * http://www.isthe.com/chongo/tech/comp/ansi_escapes.html */
#define COL(x) do { if (use_colour) printf("\033[" x "m"); } while (0)

#define NOCOL COL("00")

void t_context(const char *context, ...)
{
    va_list ap;
    va_start(ap, context);
    ne_vsnprintf(test_context, BUFSIZ, context, ap);
    va_end(ap);
    have_context = 1;
}

void t_warning(const char *str, ...)
{
    va_list ap;
    COL("43;01"); printf("WARNING:"); NOCOL;
    putchar(' ');
    va_start(ap, str);
    vprintf(str, ap);
    va_end(ap);
    warnings++;
    warned++;
    putchar('\n');
}    

#define TEST_DEBUG \
(NE_DBG_HTTP | NE_DBG_SOCKET | NE_DBG_HTTPBODY | NE_DBG_HTTPAUTH | \
 NE_DBG_LOCKS | NE_DBG_XMLPARSE | NE_DBG_XML | NE_DBG_SSL)

#define W(m) write(0, m, strlen(m))


void Usage( const char *prog) {
    printf("Usage: %s [http://]hostname[:port]/path [username password] [options]\n", prog);
    printf("Option: \n"
	   "  -r, --Requests	Number of repeat requests (Default: 10)\n"
	   "  -p, --Properties	Number of dead properties (Default: 10)\n"
	   "  -d, --Depth		Depth of collection (Default: 10) \n"
	   "  -w, --Width		Width of collection at the bottom level(Default: 100) \n"
	   "  -o, --Output		Output file\n"
	   "  -m, --Methods		Type of Web Methods (WebDAV / WebFolder, Default: WebDAV)\n"
	   );
    printf("\nExample: %s http://dav.cse.ucsc.edu:81/basic test1 test1 -r 20 -p 20 -m WebFolder \n\n", prog);
}


int read_options(int argc, char *argv[]) {
    int optc;
    
    static const struct option opts[] = {
	{ "help", no_argument, NULL, 'h' },
	{ "username", required_argument, NULL, 'u' },
	{ "password", required_argument, NULL, 'p' },
	{ "numprops", required_argument, NULL, 'n' },
	{ "outfile", required_argument, NULL, 'o' },
	{ "depth", required_argument, NULL, 'd' },
	{ "width", required_argument, NULL, 'w' },
	{ "requests", required_argument, NULL, 'r' },
	{ "quite", no_argument, NULL, 'q' },
	{ 0, 0, 0, 0 }
    };
    
    if (argc < 2) 
	return -1;
    
    memset(&pget_option, 0, sizeof(pget_option));

    pget_option.URL = argv[1];

    
    pget_option.depth = DEFAULT_DEPTH;
    strcpy(pget_option.methods, "WebDAV");
    pget_option.width = DEFAULT_WIDTH;
    pget_option.requests = DEFAULT_REQUESTS;
    pget_option.numprops = DEFAULT_NUMPROPS;


    while ((optc = getopt_long(argc, argv, "p:o:d:w:r:m:hq", opts, NULL)) != -1) {
	switch (optc) {
	case '?': 
	case 'h': Usage(argv[0]); exit(-1);
	case 'p': pget_option.numprops = atoi(optarg); break;
	case 'r': pget_option.requests = atoi(optarg); break;
	case 'm': strcpy(pget_option.methods,optarg); break;
	case 'd': pget_option.depth = atoi(optarg); break;
	case 'w': pget_option.width = atoi(optarg); break;
	case 'o': pget_option.outfile = optarg; break;
	default:
	    printf("Try `%s --help' for more information.\n", argv[1]);
	    return -1;
	}
    }

    return 0;
}


int main(int argc, char *argv[])
{
    int n, fd, i;
    char *p;
    ne_test *testp;
    
    if ( read_options(argc, argv) == -1){
	Usage(argv[0]);
	exit(-1);	
    }

    if ( pget_option.outfile ){
	if ((fd=open(pget_option.outfile, O_CREAT | O_TRUNC | O_RDWR, 0660)) < 0){
	    perror("open() :");
	    return -1;
	}   

	close(STDOUT_FILENO);
	dup2(fd, STDOUT_FILENO);
    }


    test_argc = argc;
    test_argv = argv;


    if (tests[0].fn == NULL) {
	printf("-> no tests found in `%s'\n", test_suite);
	return -1;
    }

    if (ne_sock_init()) {
	COL("43;01"); printf("WARNING:"); NOCOL;
	printf(" Socket library initalization failed.\n");
    }

    printf("\n");	
    printf("\n");	
    i = 0;
    p = Result_Header[i];
    while (  p != NULL ){
    	printf("%s\n", p);
	i++;
    	p = Result_Header[i];
    }


   testp = &tests[0];
   if ( !strcmp(pget_option.methods, "WebFolder") )
    	testp = &tests2[0];

    for ( n=0; !aborted && testp[n].fn != NULL; n++) {
	int result;


	test_name = testp[n].name;
	have_context = 0;
	test_num = n;
	warned = 0;
	fflush(stdout);

	/* run the test. */
	result = testp[n].fn();


        if (testp[n].flags & T_EXPECT_FAIL) {
            if (result == OK) {
                t_context("test passed but expected failure");
                result = FAIL;
            } else if (result == FAIL)
                result = OK;
        }

	/* align the result column if we've had warnings. */
	if (warned) {
	    printf("    %s ", dots);
	}

    }

    /* discount skipped tests */
    if (skipped) {
	printf("-> %d %s.\n", skipped,
	       skipped==1?"test was skipped":"tests were skipped");
	n -= skipped;
	if (passes + fails != n) {
	    printf("-> ARGH! Number of test results does not match "
		   "number of tests.\n"
		   "-> ARGH! Test Results are INRELIABLE.\n");
	}
    }

    ne_sock_exit();
    
    return fails;
}

ne_test tests[] = 
{
   INIT_TESTS,

   T(propinit), 
   T(proppatch), 
   T(propfinddead),
   T(propfindlive),

   T(put_get1K),
   T(put_get64K),
   T(put_get1024K),
   T(my_single),
   T(my_collection),

   T(locks),

   FINISH_TESTS
};

ne_test tests2[] = 
{
   INIT_TESTS,

   T(wf_put_get1K),
   T(wf_my_single),

   FINISH_TESTS
};

