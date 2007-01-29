
#ifndef INTEROP_H
#define INTEROP_H 1

#include <ne_session.h>
#include <ne_request.h>
#include <ne_basic.h>
#include <ne_socket.h> /* for ne_sock_addr */
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>


/* always use O_BINARY for cygwin/windows compatibility. */
#ifndef O_BINARY
#define O_BINARY (0)
#endif

/* prototype a test function. */
#define TF(x) int x(void)

/* Standard test functions.
 * init: parses and verifies cmd-line args (URL, username/password)
 * test_connect: checks that server is running.
 * open_foo: opens the dummy 'foo' file.
 * begin: opens session 'i_session' to server.
 * options: does an OPTIONS request on i_path, sets i_class2.
 * finish: closes i_session. */

TF(init); TF(begin);
TF(options); TF(options_ping); TF(finish);

/* Standard initialisers for tests[] array: start everything up: */
#define INIT_TESTS T(init), T(begin)

/* And finish everything off */
#define FINISH_TESTS T(finish), T(NULL)

/* The sesssion to use. */
extern ne_session *i_session, *i_session2;

/* server details. */
extern const char *i_hostname;
extern int i_port;
extern ne_sock_addr *i_address;
extern char *i_path;

float g_average, g_std_variance, g_thrput, g_sum_thrput, g_ops;
int g_pid;

typedef struct{
    int synccnt;
    char cur_method[64];
    float *rstlist;
    float *rstlist2;
    short *pause;
}process_share_t;
process_share_t *g_sharep;


extern int i_class2; /* true if server is a class 2 DAV server. */

/* If open_foo() has been called, this is the fd to the 'foo' file. */
extern int i_foo_fd;

/* Upload htdocs/foo to i_path + path */
int upload_foo(const char *path);

/* for method 'method' on 'uri', do operation 'x'. */
#define ONMREQ(method, uri, x) do { int _ret = (x); if (_ret) { t_context("%s on `%s': %s", method, uri, ne_get_error(i_session)); return FAIL; } } while (0)

/* for method 'method' which 'uri1' to 'uri2', do operation 'x'. */
#define ONM2REQ(method, uri1, uri2, x) do { int _ret = (x); if (_ret) { t_context("%s `%s' to `%s': %s", method, uri1, uri2, ne_get_error(i_session)); return FAIL; } } while (0)

/* ONNREQ(msg, x): fails if (x) is non-zero, giving 'msg' followed by
 * neon session error. */
#define ONNREQ(msg, x) do { int _ret = (x); if (_ret) { t_context("%s:\n%s", msg, ne_get_error(i_session)); return FAIL; } } while (0)

/* similarly for second session. */
#define ONNREQ2(msg, x) do { int _ret = (x); if (_ret) { t_context("%s:\n%s", msg, ne_get_error(i_session2)); return FAIL; } } while (0)

#define GETSTATUS (atoi(ne_get_error(i_session)))

/* STATUS(404) returns non-zero if status code is not 404 */
#define STATUS(code) (GETSTATUS != (code))

#define GETSTATUS2 (atoi(ne_get_error((i_session2))))
#define STATUS2(code) (GETSTATUS2 != (code))

#define EDGE (5) 
#define TIMEOUT (60)
#define MIN_PAUSETIME (7)
#define WARMUP_TIME 2

#define DEFAULT_DEPTH	10
#define DEFAULT_WIDTH	100
#define DEFAULT_REQUESTS	100
#define DEFAULT_CONCURRENCY	1
#define DEFAULT_NUMPROPS	10


#define time_process(num) \
{\
	int i, num2;\
	g_average =0; \
	qsort(times1, num, sizeof(float), time_comp);\
	num2 = time_filter(times1, times2, num, times1[num/2], 0.05);\
	for(i=0; i<num2; i++)\
		g_average += times2[i];\
	g_average /= num2;\
	for ( i=0; i<num2; i++)\
	    g_std_variance += (g_average-times2[i])*(g_average-times2[i]);\
	g_std_variance = sqrt(g_std_variance/num2);\
}


#define SEND_REQUEST(METHOD) \
{ \
	int i;\
	for( i=0; i<pget_option.requests; i++){ \
	    	METHOD; \
		times1[i] = latency(g_tv1, g_tv2);  \
	} \
	time_process(pget_option.requests);\
}

#define SEND_REQUEST_TWO(METHOD, METHOD2) \
{ \
	int i;\
	for( i=0; i<pget_option.requests; i++){ \
	    	METHOD; \
		times1[i] = latency(g_tv1, g_tv2);  \
	    	METHOD2; \
		times1[i] += latency(g_tv1, g_tv2);  \
	} \
	time_process(pget_option.requests);\
}

#define SEND_REQUEST_FOUR(METHOD, METHOD2, METHOD3, METHOD4) \
{ \
	int i;\
	for( i=0; i<pget_option.requests; i++){ \
	    	METHOD; \
		times1[i] = latency(g_tv1, g_tv2);  \
	    	METHOD2; \
		times1[i] += latency(g_tv1, g_tv2);  \
	    	METHOD3; \
		times1[i] += latency(g_tv1, g_tv2);  \
	    	METHOD4; \
		times1[i] += latency(g_tv1, g_tv2);  \
	} \
	time_process(pget_option.requests);\
}

#define SEND_REQUEST2(METHOD1, METHOD2) \
{ \
	int i;\
	for( i=0; i<pget_option.requests; i++){ \
		METHOD1; \
	    	METHOD2; \
		times1[i] = latency(g_tv1, g_tv2); \
	} \
	time_process(pget_option.requests);\
}

#define SEND_REQUEST2_THREE(METHOD1, METHOD2_1, METHOD2_2) \
{ \
	int i;\
	for( i=0; i<pget_option.requests; i++){ \
		METHOD1; \
	    	METHOD2_1; \
		times1[i] = latency(g_tv1, g_tv2); \
	    	METHOD2_2; \
		times1[i] += latency(g_tv1, g_tv2); \
	} \
	time_process(pget_option.requests);\
}

#define SEND_REQUEST2_FOUR(METHOD1, METHOD2_1, METHOD2_2, METHOD2_3) \
{ \
	int i;\
	for( i=0; i<pget_option.requests; i++){ \
		METHOD1; \
	    	METHOD2_1; \
		times1[i] = latency(g_tv1, g_tv2); \
	    	METHOD2_2; \
		times1[i] += latency(g_tv1, g_tv2); \
	    	METHOD2_3; \
		times1[i] += latency(g_tv1, g_tv2); \
	} \
	time_process(pget_option.requests);\
}

#define SEND_REQUEST3(METHOD1, METHOD2) \
{ \
	int i;\
	for( i=0; i<pget_option.requests; i++){ \
	    	METHOD1; \
		times1[i] = latency(g_tv1, g_tv2); \
		METHOD2; \
	} \
	time_process(pget_option.requests);\
}

#define SEND_REQUEST3_FOUR(METHOD1_1, METHOD1_2, METHOD1_3, METHOD2) \
{ \
	int i;\
	for( i=0; i<pget_option.requests; i++){ \
	    	METHOD1_1; \
		times1[i] = latency(g_tv1, g_tv2); \
	    	METHOD1_2; \
		times1[i] += latency(g_tv1, g_tv2); \
	    	METHOD1_3; \
		times1[i] += latency(g_tv1, g_tv2); \
		METHOD2; \
	} \
	time_process(pget_option.requests);\
}

int time_comp(const void* tv1, const void* tv2);
int time_filter(float a[], float b[], int nelm, float median, float percentage);
float *times1, *times2;

inline int latency(struct timeval sec, struct timeval usec);
int my_mkcol(char* uri, int depth);
void my_mkcol2(char* uri, int depth);

int put_get1K(void);
int put_get64K(void);
int put_get1024K(void);
int mkcol(void);
int my_copymovedelete(void);

#endif /* INTEROP_H */


#ifndef TESTS_H
#define TESTS_H 1

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <stdio.h>

#define OK 0
#define FAIL 1
#define FAILHARD 2 /* fail and skip all succeeding tests in this suite. */
#define SKIP 3 /* test was skipped because precondition was not met */
#define SKIPREST 4 /* skipped, and skip all succeeding tests in suite */

/* A test function. Must return any of OK, FAIL, FAILHARD, SKIP, or
 * SKIPREST.  May call t_warning() any number of times.  If not
 * returning OK, optionally call t_context to provide an error
 * message. */
typedef int (*test_func)(void);

typedef struct {
    test_func fn; /* the function to test. */
    const char *name; /* the name of the test. */
    int flags;
} ne_test;

struct options_t {
    char *URL;
    char *username;
    char *password;
    char *outfile;
    char methods[32];
    int depth;
    int width;
    int requests;
    int concurrency;
    int numprops;
    int nummethods;
}pget_option; 

/* possible values for flags: */
#define T_CHECK_LEAKS (1) /* check for memory leaks */
#define T_EXPECT_FAIL (2) /* expect failure */

/* array of tests to run: must be defined by each test suite. */
extern ne_test tests[];
extern ne_test tests2[];

/* define a test function which has the same name as the function,
 * and does check for memory leaks. */
#define T(fn) { fn, #fn, T_CHECK_LEAKS }
/* define a test function which is expected to return FAIL. */
#define T_XFAIL(fn) { fn, #fn, T_EXPECT_FAIL | T_CHECK_LEAKS }
/* define a test function which isn't checked for memory leaks. */
#define T_LEAKY(fn) { fn, #fn, 0 }

/* current test number */
extern int test_num;

/* name of test suite */
extern const char *test_suite;

/* Provide result context message. */
void t_context(const char *ctx, ...)
#ifdef __GNUC__
                __attribute__ ((format (printf, 1, 2)))
#endif /* __GNUC__ */
    ;

extern char test_context[];

/* the command-line arguments passed in to the test suite: */
extern char **test_argv;
extern int test_argc;

/* child process should call this. */
//void in_child(void);

/* issue a warning. */
//void t_warning(const char *str, ...)
#ifdef __GNUC__
                __attribute__ ((format (printf, 1, 2)))
#endif /* __GNUC__ */


/* Macros for easily writing is-not-zero comparison tests; the ON*
 * macros fail the function if a comparison is not zero.
 *
 * ONV(x,vs) takes a comparison X, and a printf varargs list for
 * the failure message.
 *  e.g.   ONV(strcmp(bar, "foo"), ("bar was %s not 'foo'", bar))
 *
 * ON(x) takes a comparison X, and uses the line number for the failure
 * message.   e.g.  ONV(strcmp(bar, "foo"))
 *
 * ONN(n, x) takes a comparison X, and a flat string failure message.
 *  e.g.  ONN("foo was wrong", strcmp(bar, "foo")) */

#define ONV(x,vs) do { if ((x)) { t_context vs; return FAIL; } } while (0)
#define ON(x) ONV((x), ("line %d", __LINE__ ))
#define ONN(n,x) ONV((x), (n))

/* ONCMP(exp, act, name): 'exp' is the expected string, 'act' is the
 * actual string for some field 'name'.  Succeeds if strcmp(exp,act)
 * == 0 or both are NULL. */
#define ONCMP(exp, act, ctx, name) do { \
ONV(exp && !act, ("%s: " name " was NULL, expected non-NULL", ctx)); \
ONV(!exp && act, ("%s: " name " was non-NULL, expected NULL", ctx)); \
ONV(exp && strcmp(exp, act), ("%s: " name " was %s not %s", ctx, exp, act)); \
} while (0)

/* return immediately with result of test 'x' if it fails. */
#define CALL(x) do { int t_ret = (x); if (t_ret != OK) return t_ret; } while (0)

/* PRECOND: skip current test if condition 'x' is not true. */
#define PRECOND(x) do { if (!(x)) { return SKIP; } } while (0)


int propinit();
int proppatch(); 
int   propfinddead();
int  propfindlive();
int   put_get1K();
int   wf_put_get1K();
int   put_get64K();
int   put_get1024K();
int   my_single();
int   wf_my_single();
int   my_collection();
int   locks();


/*************************************/

#endif /* TESTS_H */
