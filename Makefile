# Prestan Makefile.  Generated from Makefile.in by configure.
SHELL = /bin/sh

# Installation directories
prefix = /usr/local
exec_prefix = ${prefix}
libexecdir = ${exec_prefix}/libexec
bindir = ${exec_prefix}/bin
datadir = ${prefix}/share

# Toolchain settings
CC = gcc
CFLAGS = -g -O2 -I$(top_srcdir)/libneon
CPPFLAGS = -DHAVE_CONFIG_H  -I${top_builddir} -I$(top_srcdir)/lib -I$(top_srcdir)/src 

LDFLAGS = 
LIBS = -Llibneon -lneon  -lexpat 
# expat may be in LIBOBJS, so must come after $(LIBS) (which has -lneon)
ALL_LIBS = -L. -ltest $(LIBS) $(LIBOBJS)

top_builddir = .
top_srcdir = .



AR = /usr/bin/ar
RANLIB = /usr/bin/ranlib

LIBOBJS = 
TESTOBJS = src/common.o 
HDRS = src/common.h config.h

TESTS = Prestan

URL = http://`hostname`/dav/
CREDS = `whoami` `whoami`
DIR = .
OPTS = 

INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL = /usr/bin/install -c

# Fixme; use $(LIBOBJS) here instead. not happy on many non-GNU makes
# though; not sure why.
ODEPS = subdirs libtest.a 

all: $(TESTS)
	@echo
	@echo "  Now run:"
	@echo ""
	@echo '     make URL=http://dav.server/path/ check'
	@echo ' or  make URL=http://dav.server/path/ CREDS="uname passwd" check'
	@echo ""

#Prestan: Prestan.in
#	@./config.status Prestan


install: $(TESTS) 
	$(INSTALL) -d $(bindir)
	$(INSTALL) -d $(libexecdir)/Prestan
	$(INSTALL_PROGRAM) $(top_builddir)/Prestan $(bindir)/Prestan
	for t in $(TESTS); do \
	  $(INSTALL_PROGRAM) $(top_builddir)/$$t $(libexecdir)/Prestan/$$t; done

Prestan: src/props.o src/basic.o src/locks.o $(ODEPS)
	$(CC) $(LDFLAGS) -o $@ src/props.o src/basic.o src/locks.o $(ALL_LIBS)

props: src/props.o $(ODEPS)
	$(CC) $(LDFLAGS) -o $@ src/props.o $(ALL_LIBS)


locks: src/locks.o $(ODEPS)
	$(CC) $(LDFLAGS) -o $@ src/locks.o $(ALL_LIBS)


subdirs:
	(cd libneon && $(MAKE)) || exit 1

libtest.a: $(TESTOBJS)
	$(AR) cru $@ $(TESTOBJS)
	$(RANLIB) $@

clean:
	rm -f */*.o $(TESTS) libtest.a

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

Makefile: $(top_srcdir)/Makefile.in
	./config.status Makefile

src/common.o: src/common.c $(HDRS)
src/locks.o: src/locks.c $(HDRS)
src/props.o: src/props.c $(HDRS)
src/basic.o: src/basic.c $(HDRS)
