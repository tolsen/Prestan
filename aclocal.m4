# aclocal.m4 generated automatically by aclocal 1.6.3 -*- Autoconf -*-

# Copyright 1996, 1997, 1998, 1999, 2000, 2001, 2002
# Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

# Copyright (C) 1998-2002 Joe Orton <joe@manyfish.co.uk>    -*- autoconf -*-
#
# This file is free software; you may copy and/or distribute it with
# or without modifications, as long as this notice is preserved.
# This software is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even
# the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
# PURPOSE.

# The above license applies to THIS FILE ONLY, the neon library code
# itself may be copied and distributed under the terms of the GNU
# LGPL, see COPYING.LIB for more details

# This file is part of the neon HTTP/WebDAV client library.
# See http://www.webdav.org/neon/ for the latest version. 
# Please send any feedback to <neon@webdav.org>

#
# Usage:
#
#      NEON_LIBRARY
# or   NEON_BUNDLED(srcdir, [ACTIONS-IF-BUNDLED], [ACTIONS-IF-NOT_BUNDLED]) 
# or   NEON_VPATH_BUNDLED(srcdir, builddir, 
#			  [ACTIONS-IF-BUNDLED], [ACTIONS-IF-NOT-BUNDLED])
#
#   where srcdir is the location of bundled neon 'src' directory.
#   If using a VPATH-enabled build, builddir is the location of the
#   build directory corresponding to srcdir.
#
#   If a bundled build *is* being used, ACTIONS-IF-BUNDLED will be
#   evaluated. These actions should ensure that 'make' is run
#   in srcdir, and that one of NEON_NORMAL_BUILD or NEON_LIBTOOL_BUILD 
#   is called.
#
# After calling one of the above macros, if the NEON_NEED_XML_PARSER
# variable is set to "yes", then you must configure an XML parser
# too. You can do this your own way, or do it easily using the
# NEON_XML_PARSER() macro. Example usage for where we have bundled the
# neon sources in a directory called libneon, and bundled expat
# sources in a directory called 'expat'.
#
#   NEON_BUNDLED(libneon, [
#	NEON_XML_PARSER(expat)
#	NEON_NORMAL_BUILD
#   ])
#
# Alternatively, for a simple standalone app with neon as a
# dependancy, use just:
#
#   NEON_LIBRARY
# 
# and rely on the user installing neon correctly.
#
# You are free to configure an XML parser any other way you like,
# but the end result must be, either expat or libxml will get linked
# in, and HAVE_EXPAT or HAVE_LIBXML is defined appropriately.
#
# To set up the bundled build environment, call 
#
#    NEON_NORMAL_BUILD
# or
#    NEON_LIBTOOL_BUILD
# 
# depending on whether you are using libtool to build, or not.
# Both these macros take an optional argument specifying the set
# of object files you wish to build: if the argument is not given,
# all of neon will be built.

AC_DEFUN([NEON_BUNDLED],[

neon_bundled_srcdir=$1
neon_bundled_builddir=$1

NEON_COMMON_BUNDLED([$2], [$3])

])

AC_DEFUN([NEON_VPATH_BUNDLED],[

neon_bundled_srcdir=$1
neon_bundled_builddir=$2
NEON_COMMON_BUNDLED([$3], [$4])

])

AC_DEFUN([NEON_COMMON_BUNDLED],[

AC_PREREQ(2.50)

AC_ARG_WITH(included-neon,
AC_HELP_STRING([--with-included-neon], [force use of included neon library]),
[neon_force_included="$withval"], [neon_force_included="no"])

NEON_COMMON

# The colons are here so there is something to evaluate
# in case the argument was not passed.
if test "$neon_force_included" = "yes"; then
	:
	$1
else
	:
	$2
fi

])

dnl Not got any bundled sources:
AC_DEFUN([NEON_LIBRARY],[

AC_PREREQ(2.50)
neon_force_included=no
neon_bundled_srcdir=
neon_bundled_builddir=

NEON_COMMON

])

AC_DEFUN([NEON_VERSIONS], [

# Define the current versions.
NEON_VERSION_MAJOR=0
NEON_VERSION_MINOR=24
NEON_VERSION_RELEASE=0
NEON_VERSION_TAG=-dev

NEON_VERSION="${NEON_VERSION_MAJOR}.${NEON_VERSION_MINOR}.${NEON_VERSION_RELEASE}${NEON_VERSION_TAG}"

# libtool library interface versioning.  Release policy dictates that
# for neon 0.x.y, each x brings an incompatible interface change, and
# each y brings no interface change, and since this policy has been
# followed since 0.1, x == CURRENT, y == RELEASE, 0 == AGE.  For
# 1.x.y, this will become N + x == CURRENT, y == RELEASE, x == AGE,
# where N is constant (and equal to CURRENT + 1 from the final 0.x
# release)
NEON_INTERFACE_VERSION="${NEON_VERSION_MINOR}:${NEON_VERSION_RELEASE}:0"

AC_DEFINE_UNQUOTED(NEON_VERSION, "${NEON_VERSION}", 
	[Define to be the neon version string])
AC_DEFINE_UNQUOTED(NEON_VERSION_MAJOR, [(${NEON_VERSION_MAJOR})],
	[Define to be major number of neon version])
AC_DEFINE_UNQUOTED(NEON_VERSION_MINOR, [(${NEON_VERSION_MINOR})],
	[Define to be minor number of neon version])

])

dnl Define the minimum required version
AC_DEFUN([NEON_REQUIRE], [
neon_require_major=$1
neon_require_minor=$2
])

dnl Check that the external library found in a given location
dnl matches the min. required version (if any).  Requires that
dnl NEON_CONFIG be set the the full path of a valid neon-config
dnl script
dnl
dnl Usage:
dnl    NEON_CHECK_VERSION(ACTIONS-IF-OKAY, ACTIONS-IF-FAILURE)
dnl
AC_DEFUN([NEON_CHECK_VERSION], [
if test "x$neon_require_major" = "x"; then
    # Nothing to check.
    ne_goodver=yes
    ne_libver="(version unknown)"
else
    # Check whether the library is of required version
    ne_save_LIBS="$LIBS"
    ne_save_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS `$NEON_CONFIG --cflags`"
    LIBS="$LIBS `$NEON_CONFIG --libs`"
    ne_libver=`$NEON_CONFIG --version | sed -e "s/neon //g"`
    # Check whether it's possible to link against neon
    AC_CACHE_CHECK([linking against neon], [ne_cv_lib_neon],
    AC_TRY_LINK_FUNC([ne_version_match],
	[ne_cv_lib_neon=yes], [ne_cv_lib_neon=no]))
    if test "$ne_cv_lib_neon" = "yes"; then
       # Now check whether the neon library version is satisfactory
       AC_CACHE_CHECK([neon library version], [ne_cv_lib_neonver],
       AC_TRY_RUN([#include <ne_utils.h>
int main(int argc, char **argv) {
return ne_version_match($neon_require_major, $neon_require_minor);
}], ne_cv_lib_neonver=yes, ne_cv_lib_neonver=no))
    fi
    ne_goodver=$ne_cv_lib_neonver
    LIBS=$ne_save_LIBS
    CFLAGS=$ne_save_CFLAGS
fi
if test "$ne_goodver" = "yes"; then
    AC_MSG_NOTICE([using neon library $ne_libver])
    $1
else
    AC_MSG_NOTICE([incompatible neon library version $ne_libver: wanted $neon_require_major.$neon_require_minor])
    $2
fi])

dnl NEON_CHECK_SUPPORT(feature, var)
AC_DEFUN([NEON_CHECK_SUPPORT], [
if $NEON_CONFIG --support $1 >/dev/null; then
   neon_$1_message="supported by neon"
   $2=yes
else
   neon_$1_message="not supported by neon"
   $2=no
fi
])

AC_DEFUN([NEON_USE_EXTERNAL], [
# Configure to use an external neon, given a neon-config script
# found at $NEON_CONFIG.
neon_prefix=`$NEON_CONFIG --prefix`
NEON_CHECK_VERSION([
    CFLAGS="$CFLAGS `$NEON_CONFIG --cflags`"
    NEON_LIBS="$NEON_LIBS `$NEON_CONFIG --libs`"
    neon_library_message="library in ${neon_prefix} (`$NEON_CONFIG --version`)"
    neon_xml_parser_message="using whatever neon uses"
    NEON_CHECK_SUPPORT([ssl], [NEON_SUPPORTS_SSL])
    NEON_CHECK_SUPPORT([zlib], [NEON_SUPPORTS_ZLIB])
    neon_got_library=yes
], [neon_got_library=no])
])

AC_DEFUN([NEON_COMMON],[

AC_REQUIRE([NEON_COMMON_CHECKS])

NEON_VERSIONS

AC_ARG_WITH(neon,
[  --with-neon[[=DIR]]       specify location of neon library],
[case $withval in
yes|no) neon_force_external=$withval; neon_ext_path= ;;
*) neon_force_external=yes; neon_ext_path=$withval ;;
esac;], [
neon_force_external=no
neon_ext_path=
])

if test "$neon_force_included" = "no"; then
    # There is no included neon source directory, or --with-included-neon
    # wasn't given (so we're not forced to use it).

    # Default to no external neon.
    neon_got_library=no
    if test "x$neon_ext_path" = "x"; then
	AC_PATH_PROG([NEON_CONFIG], neon-config, none)
	if test "x${NEON_CONFIG}" = "xnone"; then
	    AC_MSG_NOTICE([no external neon library found])
	elif test -x "${NEON_CONFIG}"; then
	    NEON_USE_EXTERNAL
	else
	    AC_MSG_NOTICE([ignoring non-executable ${NEON_CONFIG}])
	fi
    else
	AC_MSG_CHECKING([for neon library in $neon_ext_path])
	NEON_CONFIG="$neon_ext_path/bin/neon-config"
	if test -x ${NEON_CONFIG}; then
	    AC_MSG_RESULT([found])
	    NEON_USE_EXTERNAL
	else
	    AC_MSG_RESULT([not found])
	    # ...will fail since force_external=yes
	fi
    fi

    if test "$neon_got_library" = "no"; then 
	if test $neon_force_external = yes; then
	    AC_MSG_ERROR([could not use external neon library])
	elif test -n "$neon_bundled_srcdir"; then
	    # Couldn't find external neon, forced to use bundled sources
	    neon_force_included="yes"
	else
	    # Couldn't find neon, and don't have bundled sources
	    AC_MSG_ERROR(could not find neon)
	fi
    fi
fi

# This isn't a simple 'else' branch, since neon_force_included
# is set to yes if the search fails.

if test "$neon_force_included" = "yes"; then
    AC_MSG_NOTICE([using bundled neon ($NEON_VERSION)])
    NEON_BUILD_BUNDLED="yes"
    LIBNEON_SOURCE_CHECKS
    CFLAGS="$CFLAGS -I$neon_bundled_srcdir"
    NEON_LIBS="-L$neon_bundled_builddir -lneon $NEON_LIBS"
    NEON_NEED_XML_PARSER=yes
    neon_library_message="included libneon (${NEON_VERSION})"
else
    # Don't need to configure an XML parser
    NEON_NEED_XML_PARSER=no
    NEON_BUILD_BUNDLED="yes"
fi

AC_SUBST(NEON_BUILD_BUNDLED)

])

dnl AC_SEARCH_LIBS done differently. Usage:
dnl   NE_SEARCH_LIBS(function, libnames, [extralibs], [actions-if-not-found],
dnl                            [actions-if-found])
dnl Tries to find 'function' by linking againt `-lLIB $NEON_LIBS' for each
dnl LIB in libnames.  If link fails and 'extralibs' is given, will also
dnl try linking against `-lLIB extralibs $NEON_LIBS`.
dnl Once link succeeds, `-lLIB [extralibs]` is prepended to $NEON_LIBS, and
dnl `actions-if-found' are executed, if given.
dnl If link never succeeds, run `actions-if-not-found', if given, else
dnl give an error and fail configure.
AC_DEFUN([NE_SEARCH_LIBS], [

AC_CACHE_CHECK([for library containing $1], [ne_cv_libsfor_$1], [
AC_TRY_LINK_FUNC($1, [ne_cv_libsfor_$1="none needed"], [
ne_save_LIBS=$LIBS
ne_cv_libsfor_$1="not found"
for lib in $2; do
    LIBS="$ne_save_LIBS -l$lib $NEON_LIBS"
    AC_TRY_LINK_FUNC($1, [ne_cv_libsfor_$1="-l$lib"; break])
    m4_if($3, [], [], dnl If $3 is specified, then...
              [LIBS="$ne_save_LIBS -l$lib $3 $NEON_LIBS"
               AC_TRY_LINK_FUNC($1, [ne_cv_libsfor_$1="-l$lib $3"; break])])
done
LIBS=$ne_save_LIBS])])

if test "$ne_cv_libsfor_$1" = "not found"; then
   m4_if($4, [], [AC_MSG_ERROR([could not find library containing $1])], [$4])
elif test "$ne_cv_libsfor_$1" != "none needed"; then 
   NEON_LIBS="$ne_cv_libsfor_$1 $NEON_LIBS"
   $5
fi])

dnl Check for presence and suitability of zlib library
AC_DEFUN([NEON_ZLIB], [

AC_ARG_WITH(zlib, AC_HELP_STRING([--without-zlib], [disable zlib support]),
ne_use_zlib=$withval, ne_use_zlib=yes)

NEON_SUPPORTS_ZLIB=no
AC_SUBST(NEON_SUPPORTS_ZLIB)

if test "$ne_use_zlib" = "yes"; then
    AC_CHECK_HEADER(zlib.h, [
  	AC_CHECK_LIB(z, inflate, [ 
	    NEON_LIBS="$NEON_LIBS -lz"
	    NEON_CFLAGS="$NEON_CFLAGS -DNEON_ZLIB"
	    NEON_SUPPORTS_ZLIB=yes
	    neon_zlib_message="found in -lz"
	], [neon_zlib_message="zlib not found"])
    ], [neon_zlib_message="zlib not found"])
else
    neon_zlib_message="zlib disabled"
fi
])

AC_DEFUN([NE_MACOSX], [
# Check for Darwin, which needs extra cpp and linker flags.
AC_CACHE_CHECK([for Darwin], ne_cv_os_macosx, [
case `uname -s 2>/dev/null` in
Darwin) ne_cv_os_macosx=yes ;;
*) ne_cv_os_macosx=no ;;
esac])
if test $ne_cv_os_macosx = yes; then
  CPPFLAGS="$CPPFLAGS -no-cpp-precomp"
  LDFLAGS="$LDFLAGS -flat_namespace"
fi
])

AC_DEFUN([NEON_COMMON_CHECKS], [

# These checks are done whether or not the bundled neon build
# is used.

AC_REQUIRE([AC_PROG_CC])
AC_REQUIRE([AC_PROG_CC_STDC])
AC_REQUIRE([AC_LANG_C])
AC_REQUIRE([AC_ISC_POSIX])
AC_REQUIRE([AC_C_INLINE])
AC_REQUIRE([AC_C_CONST])
AC_REQUIRE([AC_TYPE_SIZE_T])
AC_REQUIRE([AC_TYPE_OFF_T])

AC_REQUIRE([NE_MACOSX])

AC_REQUIRE([AC_PROG_MAKE_SET])

AC_REQUIRE([AC_HEADER_STDC])

AC_CHECK_HEADERS([errno.h stdarg.h string.h stdlib.h])

NEON_FORMAT(size_t,,u) dnl size_t is unsigned; use %u formats
NEON_FORMAT(off_t)
NEON_FORMAT(ssize_t)

])

AC_DEFUN([NEON_FORMAT_PREP], [
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
if test "$GCC" = "yes"; then
  AC_CACHE_CHECK([for gcc -Wformat -Werror sanity], ne_cv_cc_werror, [
  # See whether a simple test program will compile without errors.
  ne_save_CPPFLAGS=$CPPFLAGS
  CPPFLAGS="$CPPFLAGS -Wformat -Werror"
  AC_TRY_COMPILE([#include <sys/types.h>
  #include <stdio.h>], [int i = 42; printf("%d", i);], 
  [ne_cv_cc_werror=yes], [ne_cv_cc_werror=no])
  CPPFLAGS=$ne_save_CPPFLAGS])
  ne_fmt_trycompile=$ne_cv_cc_werror
else
  ne_fmt_trycompile=no
fi
])

dnl NEON_FORMAT(TYPE[, HEADERS[, [SPECIFIER]])
dnl
dnl This macro finds out which modifier is needed to create a
dnl printf format string suitable for printing integer type TYPE (which
dnl may be an int, long, or long long).
dnl The default specifier is 'd', if SPECIFIER is not given.  
dnl TYPE may be defined in HEADERS; sys/types.h is always used first.
AC_DEFUN([NEON_FORMAT], [

AC_REQUIRE([NEON_FORMAT_PREP])

AC_CHECK_SIZEOF($1, [$2])

dnl Work out which specifier character to use
m4_ifdef([ne_spec], [m4_undefine([ne_spec])])
m4_if($#, 3, [m4_define(ne_spec,$3)], [m4_define(ne_spec,d)])

AC_CACHE_CHECK([how to print $1], [ne_cv_fmt_$1], [
ne_cv_fmt_$1=none
if test $ne_fmt_trycompile = yes; then
  oflags="$CPPFLAGS"
  # Consider format string mismatches as errors
  CPPFLAGS="$CPPFLAGS -Wformat -Werror"
  dnl obscured for m4 quoting: "for str in d ld qd; do"
  for str in ne_spec l]ne_spec[ q]ne_spec[; do
    AC_TRY_COMPILE([#include <sys/types.h>
$2
#include <stdio.h>], [$1 i = 1; printf("%$str", i);], 
	[ne_cv_fmt_$1=$str; break])
  done
  CPPFLAGS=$oflags
else
  # Best guess. Don't have to be too precise since we probably won't
  # get a warning message anyway.
  case $ac_cv_sizeof_$1 in
  $ac_cv_sizeof_int) ne_cv_fmt_$1="ne_spec" ;;
  $ac_cv_sizeof_long) ne_cv_fmt_$1="l]ne_spec[" ;;
  $ac_cv_sizeof_long_long) ne_cv_fmt_$1="ll]ne_spec[" ;;
  esac
fi
])

if test "x$ne_cv_fmt_$1" = "xnone"; then
  AC_MSG_ERROR([format string for $1 not found])
fi

AC_DEFINE_UNQUOTED([NE_FMT_]translit($1, a-z, A-Z), "$ne_cv_fmt_$1", 
	[Define to be printf format string for $1])
])

dnl Wrapper for AC_CHECK_FUNCS; uses libraries from $NEON_LIBS.
AC_DEFUN([NE_CHECK_FUNCS], [
ne_save_LIBS=$LIBS
LIBS="$LIBS $NEON_LIBS"
AC_CHECK_FUNCS($@)
LIBS=$ne_save_LIBS])

dnl Checks needed when compiling the neon source.
AC_DEFUN([LIBNEON_SOURCE_CHECKS], [

dnl Run all the normal C language/compiler tests
AC_REQUIRE([NEON_COMMON_CHECKS])

dnl Needed for building the MD5 code.
AC_REQUIRE([AC_C_BIGENDIAN])
dnl Is strerror_r present; if so, which variant
AC_REQUIRE([AC_FUNC_STRERROR_R])

AC_CHECK_HEADERS([strings.h sys/time.h limits.h sys/select.h arpa/inet.h \
	signal.h sys/socket.h netinet/in.h netdb.h])

AC_REQUIRE([NE_SNPRINTF])

AC_REPLACE_FUNCS(strcasecmp)

AC_CHECK_FUNCS(signal setvbuf setsockopt stpcpy)

# Unixware 7 can only link gethostbyname with -lnsl -lsocket
# Pick up -lsocket first, then the gethostbyname check will work.
NE_SEARCH_LIBS(socket, socket inet)
NE_SEARCH_LIBS(gethostbyname, nsl)

# Enable getaddrinfo() support only if all the necessary functions
# are found.
ne_enable_gai=yes
NE_CHECK_FUNCS(getaddrinfo gai_strerror inet_ntop,,[ne_enable_gai=no; break])
if test $ne_enable_gai = yes; then
   AC_DEFINE(USE_GETADDRINFO, 1, [Define if getaddrinfo() should be used])
else
   # Checks for non-getaddrinfo() based resolver interfaces.
   NE_SEARCH_LIBS(hstrerror, resolv,,[:])
   NE_CHECK_FUNCS(hstrerror)
   # Older Unixes don't declare h_errno.
   AC_CHECK_DECL(h_errno,,,[#define _XOPEN_SOURCE_EXTENDED 1
#include <netdb.h>])
fi

AC_CHECK_MEMBERS(struct tm.tm_gmtoff,,
AC_MSG_WARN([no timezone handling in date parsing on this platform]),
[#include <time.h>])

ifdef([neon_no_zlib], [
    neon_zlib_message="zlib disabled"
    NEON_SUPPORTS_ZLIB=no
], [
    NEON_ZLIB()
])

# Conditionally enable ACL support
AC_MSG_CHECKING([whether to enable ACL support in neon])
if test "x$neon_no_acl" = "xyes"; then
    AC_MSG_RESULT(no)
else
    AC_MSG_RESULT(yes)
    NEON_EXTRAOBJS="$NEON_EXTRAOBJS ne_acl"
fi

NEON_SSL()
NEON_SOCKS()

AC_SUBST(NEON_CFLAGS)
AC_SUBST(NEON_LIBS)

])

dnl Call to put lib/snprintf.o in LIBOBJS and define HAVE_SNPRINTF_H
dnl if snprintf isn't in libc.

AC_DEFUN([NEON_REPLACE_SNPRINTF], [
# Check for snprintf
AC_CHECK_FUNC(snprintf,,[
	AC_DEFINE(HAVE_SNPRINTF_H, 1, [Define if need to include snprintf.h])
	AC_LIBOBJ(lib/snprintf)])
])

dnl turn off webdav, boo hoo.
AC_DEFUN([NEON_WITHOUT_WEBDAV], [
neon_no_webdav=yes
neon_no_acl=yes
NEON_NEED_XML_PARSER=no
neon_xml_parser_message="none needed"
])

dnl Turn off zlib support
AC_DEFUN([NEON_WITHOUT_ZLIB], [
define(neon_no_zlib, yes)
])

AC_DEFUN([NEON_WITHOUT_ACL], [
# Turn off ACL support
neon_no_acl=yes
])

dnl Common macro to NEON_LIBTOOL_BUILD and NEON_NORMAL_BUILD
dnl Sets NEONOBJS appropriately if it has not already been set.
dnl 
dnl NOT FOR EXTERNAL USE: use LIBTOOL_BUILD or NORMAL_BUILD.
dnl

AC_DEFUN([NEON_COMMON_BUILD], [

# Using the default set of object files to build.
# Add the extension to EXTRAOBJS
ne="$NEON_EXTRAOBJS"
NEON_EXTRAOBJS=
for o in $ne; do
	NEON_EXTRAOBJS="$NEON_EXTRAOBJS $o.$NEON_OBJEXT"
done	

AC_MSG_CHECKING(whether to enable WebDAV support in neon)

dnl Did they want DAV support?
if test "x$neon_no_webdav" = "xyes"; then
  # No WebDAV support
  AC_MSG_RESULT(no)
  NEONOBJS="$NEONOBJS \$(NEON_BASEOBJS)"
  NEON_CFLAGS="$NEON_CFLAGS -DNEON_NODAV"
  NEON_SUPPORTS_DAV=no
  AC_DEFINE(NEON_NODAV, 1, [Enable if built without WebDAV support])
else
  # WebDAV support
  NEON_SUPPORTS_DAV=yes
  NEONOBJS="$NEONOBJS \$(NEON_DAVOBJS)"
  # Turn on DAV locking please then.
  AC_DEFINE(USE_DAV_LOCKS, 1, [Support WebDAV locking through the library])

  AC_MSG_RESULT(yes)

fi

AC_SUBST(NEON_TARGET)
AC_SUBST(NEON_OBJEXT)
AC_SUBST(NEONOBJS)
AC_SUBST(NEON_EXTRAOBJS)
AC_SUBST(NEON_LINK_FLAGS)
AC_SUBST(NEON_SUPPORTS_DAV)

])

# The libtoolized build case:
AC_DEFUN([NEON_LIBTOOL_BUILD], [

NEON_TARGET=libneon.la
NEON_OBJEXT=lo

NEON_COMMON_BUILD($#, $*)

])

dnl Find 'ar' and 'ranlib', fail if ar isn't found.
AC_DEFUN([NE_FIND_AR], [

# Search in /usr/ccs/bin for Solaris
ne_PATH=$PATH:/usr/ccs/bin
AC_PATH_TOOL(AR, ar, notfound, $ne_PATH)
if test "x$AR" = "xnotfound"; then
   AC_MSG_ERROR([could not find ar tool])
fi
AC_PATH_TOOL(RANLIB, ranlib, :, $ne_PATH)

])

# The non-libtool build case:
AC_DEFUN([NEON_NORMAL_BUILD], [

NEON_TARGET=libneon.a
NEON_OBJEXT=o

AC_REQUIRE([NE_FIND_AR])

NEON_COMMON_BUILD($#, $*)

])

AC_DEFUN([NE_SNPRINTF], [
AC_CHECK_FUNCS(snprintf vsnprintf,,[
   ne_save_LIBS=$LIBS
   LIBS="$LIBS -lm"    # Always need -lm
   AC_CHECK_LIB(trio, trio_vsnprintf,
   [AC_CHECK_HEADERS(trio.h,,
    AC_MSG_ERROR([trio installation problem? libtrio found but not trio.h]))
    AC_MSG_NOTICE(using trio printf replacement library)
    NEON_LIBS="$NEON_LIBS -ltrio -lm"
    NEON_CFLAGS="$NEON_CFLAGS -DNEON_TRIO"],
   [AC_MSG_NOTICE([no vsnprintf/snprintf detected in C library])
    AC_MSG_ERROR([Install the trio library from http://daniel.haxx.se/trio/])])
   LIBS=$ne_save_LIBS
   break
])])

dnl Usage: NE_CHECK_SSLVER(variable, version-string, version-hex)
dnl Define 'variable' to 'yes' if OpenSSL version is >= version-hex
AC_DEFUN([NE_CHECK_SSLVER], [
AC_CACHE_CHECK([OpenSSL version is >= $2], $1, [
AC_EGREP_CPP(good, [#include <openssl/opensslv.h>
#if OPENSSL_VERSION_NUMBER >= $3
good
#endif], [$1=yes], [$1=no])])])

dnl Less noisy replacement for PKG_CHECK_MODULES
AC_DEFUN([NE_PKG_CONFIG], [

AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test "$PKG_CONFIG" = "no"; then
   : Not using pkg-config
   $4
else
   AC_CACHE_CHECK([for $2 pkg-config data], ne_cv_pkg_$2,
     [if $PKG_CONFIG $2; then
        ne_cv_pkg_$2=yes
      else
        ne_cv_pkg_$2=no
      fi])

   if test "$ne_cv_pkg_$2" = "yes"; then
      $1_CFLAGS=`$PKG_CONFIG --cflags $2`
      $1_LIBS=`$PKG_CONFIG --libs $2`
      : Using provided pkg-config data
      $3
   else
      : No pkg-config for $2 provided
      $4
   fi
fi])

dnl Check for OpenSSL
AC_DEFUN([NEON_SSL], [

AC_ARG_WITH(ssl, [AC_HELP_STRING([--with-ssl], [enable OpenSSL support])])

AC_ARG_WITH(egd,
[[  --with-egd[=PATH]       enable EGD support [using EGD socket at PATH]]])

case $with_ssl in
yes)

   NE_PKG_CONFIG(NE_SSL, openssl,
    [AC_MSG_NOTICE(using SSL library configuration from pkg-config)
     CPPFLAGS="$CPPFLAGS ${NE_SSL_CFLAGS}"
     NEON_LIBS="$NEON_LIBS ${NE_SSL_LIBS}"],
    [# libcrypto may require -ldl if using the OpenSSL ENGINE branch
     NE_SEARCH_LIBS(RSA_new, crypto, -ldl)
     NE_SEARCH_LIBS(SSL_library_init, ssl)])

   AC_CHECK_HEADERS(openssl/ssl.h openssl/opensslv.h,,
   [AC_MSG_ERROR([OpenSSL headers not found, cannot enable SSL support])])

   # Enable EGD support if using 0.9.7 or newer
   NE_CHECK_SSLVER(ne_cv_lib_ssl097, 0.9.7, 0x00907000L)
   if test "$ne_cv_lib_ssl097" = "yes"; then
      AC_MSG_NOTICE([OpenSSL >= 0.9.7; EGD support not needed in neon])
      neon_ssl_message="OpenSSL (0.9.7 or later)"
   else
      # Fail if OpenSSL is older than 0.9.6
      NE_CHECK_SSLVER(ne_cv_lib_ssl096, 0.9.6, 0x00906000L)
      if test "$ne_cv_lib_ssl096" != "yes"; then
         AC_MSG_ERROR([OpenSSL 0.9.6 or later is required])
      fi
      neon_ssl_message="OpenSSL (0.9.6 or later)"

      case "$with_egd" in
      yes|no) ne_cv_lib_sslegd=$with_egd ;;
      /*) ne_cv_lib_sslegd=yes
          AC_DEFINE_UNQUOTED([EGD_PATH], "$with_egd", 
			     [Define to specific EGD socket path]) ;;
      *) # Guess whether EGD support is needed
         AC_CACHE_CHECK([whether to enable EGD support], [ne_cv_lib_sslegd],
	 [if test -r /dev/random || test -r /dev/urandom; then
	    ne_cv_lib_sslegd=no
	  else
	    ne_cv_lib_sslegd=yes
	  fi])
	 ;;
      esac
      if test "$ne_cv_lib_sslegd" = "yes"; then
        AC_MSG_NOTICE([EGD support enabled for seeding OpenSSL PRNG])
        AC_DEFINE([ENABLE_EGD], 1, [Define if EGD should be supported])
      fi
   fi

   NEON_SUPPORTS_SSL=yes
   NEON_CFLAGS="$NEON_CFLAGS -DNEON_SSL"
   ;;
*) # Default to off; only create crypto-enabled binaries if requested.
   neon_ssl_message="No SSL support"
   NEON_SUPPORTS_SSL=no
   ;;
esac
AC_SUBST(NEON_SUPPORTS_SSL)
])

dnl Adds an --enable-warnings argument to configure to allow enabling
dnl compiler warnings
AC_DEFUN([NEON_WARNINGS],[

AC_REQUIRE([AC_PROG_CC]) dnl so that $GCC is set

AC_ARG_ENABLE(warnings,
AC_HELP_STRING(--enable-warnings, [enable compiler warnings]))

if test "$enable_warnings" = "yes"; then
   case $GCC:`uname` in
   yes:*)
      CFLAGS="$CFLAGS -Wall -ansi-pedantic -Wmissing-declarations -Winline -Wshadow -Wreturn-type -Wsign-compare -Wundef -Wpointer-arith -Wcast-align -Wbad-function-cast -Wimplicit-prototypes"
      if test -z "$with_ssl" -o "$with_ssl" = "no"; then
	 # OpenSSL headers fail strict prototypes checks
	 CFLAGS="$CFLAGS -Wstrict-prototypes"
      fi
      ;;
   no:OSF1) CFLAGS="$CFLAGS -check -msg_disable returnchecks -msg_disable alignment -msg_disable overflow" ;;
   no:IRIX) CFLAGS="$CFLAGS -fullwarn" ;;
   no:UnixWare) CFLAGS="$CFLAGS -v" ;;
   *) AC_MSG_WARN([warning flags unknown for compiler on this platform]) ;;
   esac
fi
])

dnl Adds an --disable-debug argument to configure to allow disabling
dnl debugging messages.
dnl Usage:
dnl  NEON_WARNINGS([actions-if-debug-enabled], [actions-if-debug-disabled])
dnl
AC_DEFUN([NEON_DEBUG], [

AC_ARG_ENABLE(debug,
AC_HELP_STRING(--disable-debug,[disable runtime debugging messages]))

# default is to enable debugging
case $enable_debug in
no) AC_MSG_NOTICE([debugging is disabled])
$2 ;;
*) AC_MSG_NOTICE([debugging is enabled])
AC_DEFINE(NE_DEBUGGING, 1, [Define to enable debugging])
$1
;;
esac])

dnl Macro to optionally enable socks support
AC_DEFUN([NEON_SOCKS], [

AC_ARG_WITH([socks], AC_HELP_STRING([--with-socks],[use SOCKSv5 library]))

if test "$with_socks" = "yes"; then
  ne_save_LIBS=$LIBS

  AC_CHECK_HEADERS(socks.h,
    [AC_CHECK_LIB(socks5, connect,
      [AC_MSG_NOTICE([SOCKSv5 support enabled])],
      [AC_MSG_ERROR([could not find libsocks5 for SOCKS support])])],
    [AC_MSG_ERROR([could not find socks.h for SOCKS support])])

  CFLAGS="$CFLAGS -DNEON_SOCKS"
  NEON_LIBS="$NEON_LIBS -lsocks5"
  LIBS=$ne_save_LIBS

fi])

AC_DEFUN([NEON_WITH_LIBS], [
AC_ARG_WITH([libs],
[[  --with-libs=DIR[:DIR2...] look for support libraries in DIR/{bin,lib,include}]],
[case $with_libs in
yes|no) AC_MSG_ERROR([--with-libs must be passed a directory argument]) ;;
*) ne_save_IFS=$IFS; IFS=:
   for dir in $with_libs; do
     ne_add_CPPFLAGS="$ne_add_CPPFLAGS -I${dir}/include"
     ne_add_LDFLAGS="$ne_add_LDFLAGS -L${dir}/lib"
     ne_add_PATH="${ne_add_PATH}${dir}/bin:"
   done
   IFS=$ne_save_IFS
   CPPFLAGS="${ne_add_CPPFLAGS} $CPPFLAGS"
   LDFLAGS="${ne_add_LDFLAGS} $LDFLAGS"
   PATH=${ne_add_PATH}$PATH ;;
esac])])

# isc-posix.m4 serial 2 (gettext-0.11.2)
dnl Copyright (C) 1995-2002 Free Software Foundation, Inc.
dnl This file is free software, distributed under the terms of the GNU
dnl General Public License.  As a special exception to the GNU General
dnl Public License, this file may be distributed as part of a program
dnl that contains a configuration script generated by Autoconf, under
dnl the same distribution terms as the rest of that program.

# This file is not needed with autoconf-2.53 and newer.  Remove it in 2005.

# This test replaces the one in autoconf.
# Currently this macro should have the same name as the autoconf macro
# because gettext's gettext.m4 (distributed in the automake package)
# still uses it.  Otherwise, the use in gettext.m4 makes autoheader
# give these diagnostics:
#   configure.in:556: AC_TRY_COMPILE was called before AC_ISC_POSIX
#   configure.in:556: AC_TRY_RUN was called before AC_ISC_POSIX

undefine([AC_ISC_POSIX])

AC_DEFUN([AC_ISC_POSIX],
  [
    dnl This test replaces the obsolescent AC_ISC_POSIX kludge.
    AC_CHECK_LIB(cposix, strerror, [LIBS="$LIBS -lcposix"])
  ]
)

# Copyright (C) 2001-2002 Joe Orton <joe@manyfish.co.uk>    -*- autoconf -*-
#
# This file is free software; you may copy and/or distribute it with
# or without modifications, as long as this notice is preserved.
# This software is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even
# the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
# PURPOSE.

# The above license applies to THIS FILE ONLY, the neon library code
# itself may be copied and distributed under the terms of the GNU
# LGPL, see COPYING.LIB for more details

# This file is part of the neon HTTP/WebDAV client library.
# See http://www.webdav.org/neon/ for the latest version. 
# Please send any feedback to <neon@webdav.org>

# Tests needed for the neon-test common test code.

AC_DEFUN([NEON_TEST], [

AC_REQUIRE([NEON_COMMON_CHECKS])

AC_REQUIRE([AC_TYPE_PID_T])
AC_REQUIRE([AC_HEADER_TIME])

dnl NEON_XML_PARSER may add things (e.g. -I/usr/local/include) to 
dnl CPPFLAGS which make "gcc -Werror" fail in NEON_FORMAT; suggest
dnl this macro is used first.
AC_BEFORE([$0], [NEON_XML_PARSER])

AC_CHECK_HEADERS(sys/time.h)

AC_CHECK_FUNCS(pipe isatty usleep)

AC_REQUIRE([NE_FIND_AR])

NEON_FORMAT(time_t, [
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif])

])

# Copyright (C) 1998-2002 Joe Orton <joe@manyfish.co.uk>    -*- autoconf -*-
#
# This file is free software; you may copy and/or distribute it with
# or without modifications, as long as this notice is preserved.
# This software is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even
# the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
# PURPOSE.

# The above license applies to THIS FILE ONLY, the neon library code
# itself may be copied and distributed under the terms of the GNU
# LGPL, see COPYING.LIB for more details

# This file is part of the neon HTTP/WebDAV client library.
# See http://www.webdav.org/neon/ for the latest version. 
# Please send any feedback to <neon@webdav.org>

# Check for XML parser, supporting libxml 2.x and expat 1.95.x,
# or a bundled copy of expat.
#  *  Bundled expat if a directory name argument is passed
#     -> expat dir must contain minimal expat sources, i.e.
#        xmltok, xmlparse sub-directories.  See sitecopy/cadaver for
#	 examples of how to do this.
#
# Usage: 
#  NEON_XML_PARSER()
# or
#  NEON_XML_PARSER(expat-dir)

dnl Find expat: run $1 if found, else $2
AC_DEFUN([NE_XML_EXPAT], [
AC_CHECK_HEADER(expat.h,
  [AC_CHECK_LIB(expat, XML_SetXmlDeclHandler, [
    AC_DEFINE(HAVE_EXPAT, 1, [Define if you have expat])
    neon_xml_parser_message="expat"
    NEON_LIBS="$NEON_LIBS -lexpat"
    neon_xml_parser=expat
  ], [$1])])
])

dnl Find libxml2: run $1 if found, else $2
AC_DEFUN([NE_XML_LIBXML2], [
AC_CHECK_PROG(XML2_CONFIG, xml2-config, xml2-config)
if test -n "$XML2_CONFIG"; then
    neon_xml_parser_message="libxml `$XML2_CONFIG --version`"
    AC_DEFINE(HAVE_LIBXML, 1, [Define if you have libxml])
    # xml2-config in some versions erroneously includes -I/include
    # in the --cflags output.
    CPPFLAGS="$CPPFLAGS `$XML2_CONFIG --cflags | sed 's| -I/include||g'`"
    NEON_LIBS="$NEON_LIBS `$XML2_CONFIG --libs | sed 's|-L/usr/lib ||g'`"
    AC_CHECK_HEADERS(libxml/xmlversion.h libxml/parser.h,,[
      AC_MSG_ERROR([could not find parser.h, libxml installation problem?])])
    neon_xml_parser=libxml2
else
    $1
fi
])

dnl Configure for a bundled expat build.
AC_DEFUN([NE_XML_BUNDLED_EXPAT], [

AC_REQUIRE([AC_C_BIGENDIAN])
# Define XML_BYTE_ORDER for expat sources.
if test $ac_cv_c_bigendian = "yes"; then
  ne_xml_border=21
else
  ne_xml_border=12
fi

# mini-expat doesn't pick up config.h
CPPFLAGS="$CPPFLAGS -DXML_BYTE_ORDER=$ne_xml_border -DXML_DTD -I$1/xmlparse -I$1/xmltok"

# Use the bundled expat sources
AC_LIBOBJ($1/xmltok/xmltok)
AC_LIBOBJ($1/xmltok/xmlrole)
AC_LIBOBJ($1/xmlparse/xmlparse)
AC_LIBOBJ($1/xmlparse/hashtable)

AC_DEFINE(HAVE_EXPAT)

AC_DEFINE(HAVE_XMLPARSE_H, 1, [Define if using expat which includes xmlparse.h])

])

AC_DEFUN([NEON_XML_PARSER], [

dnl Switches to force choice of library
AC_ARG_WITH([libxml2],
AC_HELP_STRING([--with-libxml2], [force use of libxml 2.x]))
AC_ARG_WITH([expat], 
AC_HELP_STRING([--with-expat], [force use of expat]))

dnl Flag to force choice of included expat, if available.
ifelse($#, 1, [
AC_ARG_WITH([included-expat],
AC_HELP_STRING([--with-included-expat], [use bundled expat sources]),,
with_included_expat=no)],
with_included_expat=no)

if test "$NEON_NEED_XML_PARSER" = "yes"; then
  # Find an XML parser
  neon_xml_parser=none

  # Forced choice of expat:
  case $with_expat in
  yes) NE_XML_EXPAT([AC_MSG_ERROR([expat library not found, cannot proceed])]) ;;
  no) ;;
  */libexpat.la) 
       # Special case for Subversion
       ne_expdir=`echo $with_expat | sed 's:/libexpat.la$::'`
       AC_DEFINE(HAVE_EXPAT)
       CPPFLAGS="$CPPFLAGS -I$ne_expdir"
       # no dependency on libexpat => crippled libneon, so do partial install
       ALLOW_INSTALL=lib
       neon_xml_parser=expat
       neon_xml_parser_message="expat in $ne_expdir"
       ;;
  /*) AC_MSG_ERROR([--with-expat does not take a directory argument]) ;;
  esac

  # If expat wasn't specifically enabled and libxml was:
  if test "${neon_xml_parser}-${with_libxml}-${with_included_expat}" = "none-yes-no"; then
     NE_XML_LIBXML2(
      [AC_MSG_ERROR([libxml2.x library not found, cannot proceed])])
  fi

  # Otherwise, by default search for libxml2 then expat:
  if test "${neon_xml_parser}-${with_included_expat}" = "none-no"; then
     NE_XML_LIBXML2([NE_XML_EXPAT([:])])
  fi

  # If an XML parser still has not been found, fail or use the bundled expat
  if test "$neon_xml_parser" = "none"; then
    m4_if($1, [], 
       [AC_MSG_ERROR([no XML parser was found: expat or libxml 2.x required])],
       [# Configure the bundled copy of expat
        NE_XML_BUNDLED_EXPAT($1)
	neon_xml_parser_message="bundled expat in $1"])
  fi

  AC_MSG_NOTICE([XML parser used: $neon_xml_parser_message])
fi

])

