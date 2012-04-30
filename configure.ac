# configure.ac for GPA
# Copyright (C) 2000, 2001  Werner Koch
# Copyright (C) 2002, 2003, 2004  Miguel Coca
# Copyright (C) 2005, 2006, 2007, 2008, 2009  g10 Code GmbH
#
# This file is part of GPA.
#
# GPA is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your
# option) any later version.
#
# GPA is distributed in the hope that it will be useful but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
# License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.
#
# (Process this file with autoconf to produce a configure script.)
AC_PREREQ(2.61)
min_automake_version="1.10"

# The version number goes here.
#
# Remember to change the version number immediately *after* a release.
# Set my_issvn to "yes" for non-released code.  Remember to run an
# "svn up" and "autogen.sh --force" right before creating a distribution.
m4_define(my_version, [0.9.2])
m4_define(my_issvn, yes)

m4_define([svn_revision], m4_esyscmd([printf "%d" $(svn info 2>/dev/null \
          | sed -n '/^Revision:/ s/[^0-9]//gp'|head -1)]))
m4_define([git_revision], m4_esyscmd([git branch -v 2>/dev/null \
          | awk '/^\* / {printf "%s",$3}']))
m4_define([my_full_version], [my_version[]m4_if(my_issvn,[yes],
          [m4_if(git_revision,[],[-svn[]svn_revision],[-git[]git_revision])])])
AC_INIT([gpa],[my_full_version],[http://bugs.gnupg.org])

NEED_GPG_ERROR_VERSION=1.4
NEED_LIBASSUAN_API=2
NEED_LIBASSUAN_VERSION=1.1.0
NEED_GPGME_API=1
NEED_GPGME_VERSION=1.2.0


AM_CONFIG_HEADER(config.h)
AC_CONFIG_SRCDIR(src/gpa.c)
AM_INIT_AUTOMAKE($PACKAGE_NAME, $PACKAGE_VERSION)

BUILD_REVISION=svn_revision
PACKAGE=$PACKAGE_NAME
VERSION=$PACKAGE_VERSION
AC_SUBST(PACKAGE)
AC_SUBST(VERSION)
AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE", [Name of this package])
AC_DEFINE_UNQUOTED(VERSION, "$VERSION", [Version of this package])

AC_DEFINE_UNQUOTED(PACKAGE_BUGREPORT, "$PACKAGE_BUGREPORT",
                                      [address for reporting bugs])

have_gpg_error=no
have_gpgme=no
have_libassuan=no


# Generate extended version information
if test "$have_w32_system" = yes; then
   BUILD_TIMESTAMP=`date --iso-8601=minutes`
   changequote(,)dnl
   BUILD_FILEVERSION=`echo "$VERSION" | sed 's/\([0-9.]*\).*/\1./;s/\./,/g'`
   changequote([,])dnl
   BUILD_FILEVERSION="${BUILD_FILEVERSION}${BUILD_REVISION}"
fi
AC_SUBST(BUILD_REVISION)
AC_SUBST(BUILD_TIMESTAMP)
AC_SUBST(BUILD_FILEVERSION)
AC_DEFINE_UNQUOTED(BUILD_REVISION, "$BUILD_REVISION",
                   [Subversion revision used to build this package])


AC_GNU_SOURCE

AH_BOTTOM([
/* We don't want the old assuan codes anymore. */
#define _ASSUAN_ONLY_GPG_ERRORS 1

#include "gpadefs.h"
])


AM_MAINTAINER_MODE

dnl Check for libraries
AC_CHECK_LIB(m, sin)
CHECK_ZLIB
AC_CHECK_FUNCS([strsep stpcpy])

development_version=no
# Allow users to append something to the version string (other than -cvs)
# without flagging it as development version.  The user version parts is
# considered everything after a dash.
if test "$development_version" != yes; then
  changequote(,)dnl
  tmp_pat='[a-zA-Z]'
  changequote([,])dnl
  if echo "$VERSION" | sed 's/-.*//' | grep "$tmp_pat" >/dev/null ||
     echo "$VERSION" | grep -- "-cvs$" >/dev/null; then
    development_version=yes
  fi
fi
if test "$development_version" = yes; then
    AC_DEFINE(IS_DEVELOPMENT_VERSION,1,
            [Defined if this is not a regular release])
fi


# Define HAVE_W32_SYSTEM as an alternative to the other macros which
# might led the naive reader assume that W32 is actually a win.  Also
# needed later to enable certain gcc options.
have_w32_system=no
case "${host}" in
    *-mingw32*) have_w32_system=yes ;;
esac
if test "$have_w32_system" = yes; then
   AC_DEFINE(HAVE_W32_SYSTEM,1, [Defined if we run on a W32 API based system])
fi
AM_CONDITIONAL(HAVE_W32_SYSTEM, test "$have_w32_system" = yes)



dnl
dnl Checks for programs
dnl
AM_MISSING_PROG(ACLOCAL, aclocal, $missing_dir)
AM_MISSING_PROG(AUTOCONF, autoconf, $missing_dir)
AM_MISSING_PROG(AUTOMAKE, automake, $missing_dir)
AM_MISSING_PROG(AUTOHEADER, autoheader, $missing_dir)
AM_MISSING_PROG(MAKEINFO, makeinfo, $missing_dir)
AC_PROG_CC
AC_ISC_POSIX
AC_STDC_HEADERS
AC_PROG_RANLIB
AC_CHECK_TOOL(WINDRES, windres, :)

if test -z "$GPG"; then
  AC_PATH_PROG(GPG, gpg)
  if test -z "$GPG"; then
       AC_MSG_ERROR([[
***
*** GnuPG not found. Please install GnuPG first.
*** See   http://www.gnupg.org/download.html
***
  ]])
  fi
fi

dnl Find the keyserver plugins. Assume that gpgkeys_ldap is always available
AC_PATH_PROG(GPGKEYS_LDAP, gpgkeys_ldap, [${libexecdir}/gnupg/gpgkeys_ldap],
             [$PATH:/usr/libexec/gnupg/:/usr/lib/gnupg/])
KEYSERVER_HELPERS_DIR=`dirname $GPGKEYS_LDAP`
if test -z "$KEYSERVER_HELPERS_DIR"; then
       AC_MSG_ERROR([[
***
*** Keyserver helpers not found. They should be contained in the GnuPG package.
***
  ]])
fi

AC_DEFINE_UNQUOTED(GPG_PATH, "$GPG", [Path to the gpg binary.])
AC_DEFINE_UNQUOTED(KEYSERVER_HELPERS_DIR,
		   "$KEYSERVER_HELPERS_DIR", [Path to keyserver helper tools])

dnl
dnl Checks for compiler features
dnl


dnl
dnl Checks for libraries
dnl
NETLIBS=
if test "$have_w32_system" = yes; then
   NETLIBS="${NETLIBS} -lws2_32"
fi
AC_SUBST(NETLIBS)


AM_PO_SUBDIRS
AM_GNU_GETTEXT_VERSION([0.17])
AM_GNU_GETTEXT([external],[need-ngettext])


dnl Where is the GTK+ toolkit
AM_PATH_GTK_2_0(2.10.0,, AC_MSG_ERROR(Cannot find GTK+ 2.0))


#
# The GnuPG shared error codes library.
#
AM_PATH_GPG_ERROR("$NEED_GPG_ERROR_VERSION",
                  have_gpg_error=yes,have_gpg_error=no)
AC_DEFINE(GPG_ERR_SOURCE_DEFAULT, 12,
          [The default error source for GPA.])

#
# Libassuan is the IPC library used for the server mode
#
AM_PATH_LIBASSUAN("$NEED_LIBASSUAN_API:$NEED_LIBASSUAN_VERSION",
                  have_libassuan=yes,have_libassuan=no)

# We need this for GPGME
AC_SYS_LARGEFILE

#
# Depending on the OS we need to test for different versions of gpgme.
#
_AM_PATH_GPGME_CONFIG
if test "$have_w32_system" = yes; then
  AM_PATH_GPGME_GLIB("$NEED_GPGME_API:$NEED_GPGME_VERSION",
                     have_gpgme=yes,have_gpgme=no)
  GPGME_LIBS="$GPGME_GLIB_LIBS"
  GPGME_CFLAGS="$GPGME_GLIB_CFLAGS"
  AC_SUBST(GPGME_CFLAGS)
  AC_SUBST(GPGME_LIBS)
else
  AM_PATH_GPGME("$NEED_GPGME_API:$NEED_GPGME_VERSION",
                have_gpgme=yes,have_gpgme=no)
fi

dnl The tests below are not anymore used because we now depend on a
dnl gpgme which has all these features.  However, I keep the code here
dnl for future work.
dnl
dnl _save_libs=$LIBS
dnl _save_cflags=$CFLAGS
dnl LIBS="$LIBS $GPGME_LIBS"
dnl CFLAGS="$CFLAGS $GPGME_CFLAGS"
dnl AC_CHECK_FUNCS([gpgme_op_assuan_transact])
dnl AC_CHECK_FUNCS([gpgme_io_write])
dnl # Fixme:  We should write a test based on gpgme_subkey_t
dnl #         We might even want to put such a test into gpgme.m4.
dnl AC_CHECK_MEMBERS([struct _gpgme_subkey.card_number],,,[#include <gpgme.h>])
dnl LIBS=$_save_libs
dnl CFLAGS="$_save_cflags"
dnl # We build the card manager only if we have a decent libgpgme.  */
dnl if test "x$ac_cv_func_gpgme_op_assuan_transact" = "xyes"; then
dnl     AC_DEFINE(BUILD_CARD_MANAGER, 1,
dnl               [Defined if the card manager shall be build.])
dnl fi
dnl AM_CONDITIONAL(BUILD_CARD_MANAGER,
dnl                test "x$ac_cv_func_gpgme_op_assuan_transact" = "xyes")


#
# Checks for header files.
#
AC_MSG_NOTICE([checking for header files])
AC_CHECK_HEADERS([locale.h])

#
# Checks for typedefs and structures
#

GNUPG_CHECK_TYPEDEF(byte, HAVE_BYTE_TYPEDEF)
GNUPG_CHECK_TYPEDEF(ushort, HAVE_USHORT_TYPEDEF)
GNUPG_CHECK_TYPEDEF(ulong, HAVE_ULONG_TYPEDEF)
GNUPG_CHECK_TYPEDEF(u16, HAVE_U16_TYPEDEF)
GNUPG_CHECK_TYPEDEF(u32, HAVE_U32_TYPEDEF)

#
# Check for library functions
#

# See whether libc supports the Linux inotify interface
case "${host}" in
    *-*-linux*)
        AC_CHECK_FUNCS([inotify_init])
        ;;
esac


#
# Set extra compiler flags
#
if test "$GCC" = yes; then
    if test "$USE_MAINTAINER_MODE" = "yes"; then
        # FIXME:
	#
        # Should have -Wshadow here too, but it causes too many warnings
        # along with GTK+, due to:
        #
        # http://gcc.gnu.org/cgi-bin/gnatsweb.pl?cmd=view&pr=529&database=gcc
        #
        # Add it again if that bug is ever fixed.
	#
	# The same goes to -Wstrict-prototypes, as it causes too many warnings
	# in gtkitemfactory.h:51 (see comment in the code for the reason).
	#
	# All those warnings make it difficult to tell apart real GPA
	# warnings.
	#
	# CFLAGS="$CFLAGS -Wall -Wcast-align -Wshadow -Wstrict-prototypes"
        CFLAGS="$CFLAGS -Wall -Wcast-align"
    else
        CFLAGS="$CFLAGS -Wall"
    fi
    if test "$have_w32_system" = yes; then
       CFLAGS="$CFLAGS -mms-bitfields"
    fi
fi


#
# Print errors here so that they are visible all
# together and the user can acquire them all together.
#
die=no
if test "$have_gpg_error" = "no"; then
   die=yes
   AC_MSG_NOTICE([[
***
*** You need libgpg-error to build this program.
**  This library is for example available at
***   ftp://ftp.gnupg.org/gcrypt/libgpg-error
*** (at least version $NEED_GPG_ERROR_VERSION is required.)
***]])
fi
if test "$have_gpgme" = "no"; then
   die=yes
   AC_MSG_NOTICE([[
***
*** You need gpgme to build this program.
**  This library is for example available at
***   ftp://ftp.gnupg.org/gcrypt/gpgme/
*** (at least version $NEED_GPGME_VERSION is required.)
***]])
fi
if test "$have_libassuan" = "no"; then
   die=yes
   AC_MSG_NOTICE([[
***
*** You need libassuan to build this program.
*** This library is for example available at
***   ftp://ftp.gnupg.org/gcrypt/libassuan/
*** (at least version $NEED_LIBASSUAN_VERSION is required).
***]])
fi
if test "$die" = "yes"; then
    AC_MSG_ERROR([[
***
*** Required libraries not found. Please consult the above messages
*** and install them before running configure again.
***]])
fi


AC_CONFIG_FILES([ m4/Makefile
Makefile
po/Makefile.in
src/Makefile
pixmaps/Makefile
doc/Makefile
])


AC_OUTPUT
