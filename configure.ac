# configure.ac for GPA
# Copyright (C) 2000, 2001  Werner Koch
# Copyright (C) 2002, 2003, 2004  Miguel Coca
# Copyright (C) 2005-2016   g10 Code GmbH
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
# To build a release you need to create a tag with the version number
# (git tag -s gpa-1.n.m) and run "./autogen.sh --force".  Please
# bump the version number immediately *after* the release and do
# another commit and push so that the git magic is able to work.
m4_define(mym4_version_major, [0])
m4_define(mym4_version_minor, [10])
m4_define(mym4_version_micro, [0])

# Below is m4 magic to extract and compute the git revision number,
# the decimalized short revision number, a beta version string and a
# flag indicating a development version (mym4_isgit).  Note that the
# m4 processing is done by autoconf and not during the configure run.
m4_define(mym4_version,
          [mym4_version_major.mym4_version_minor.mym4_version_micro])
m4_define([mym4_revision],
          m4_esyscmd([git rev-parse --short HEAD | tr -d '\n\r']))
m4_define([mym4_revision_dec],
          m4_esyscmd_s([echo $((0x$(echo ]mym4_revision[|head -c 4)))]))
m4_define([mym4_betastring],
          m4_esyscmd_s([git describe --match 'gpa-[0-9].*[0-9]' --long|\
                        awk -F- '$3!=0{print"-beta"$3}']))
m4_define([mym4_isgit],m4_if(mym4_betastring,[],[no],[yes]))
m4_define([mym4_full_version],[mym4_version[]mym4_betastring])

AC_INIT([gpa],[mym4_full_version],[https://bugs.gnupg.org])


NEED_GPG_ERROR_VERSION=1.27
NEED_LIBASSUAN_API=2
NEED_LIBASSUAN_VERSION=2.4.2
NEED_GPGME_API=1
NEED_GPGME_VERSION=1.9.0

AC_CONFIG_AUX_DIR([build-aux])
AM_CONFIG_HEADER(config.h)
AC_CONFIG_SRCDIR(src/gpa.c)
AM_INIT_AUTOMAKE([dist-bzip2 no-dist-gzip])

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


#
# Provide information about the build.
#
BUILD_REVISION="mym4_revision"
AC_SUBST(BUILD_REVISION)
AC_DEFINE_UNQUOTED(BUILD_REVISION, "$BUILD_REVISION",
                   [GIT commit id revision used to build this package])

changequote(,)dnl
BUILD_FILEVERSION=`echo "$VERSION" | sed 's/\([0-9.]*\).*/\1./;s/\./,/g'`
changequote([,])dnl
BUILD_FILEVERSION="${BUILD_FILEVERSION}mym4_revision_dec"
AC_SUBST(BUILD_FILEVERSION)

BUILD_TIMESTAMP=`date -u +%Y-%m-%dT%H:%M+0000 2>/dev/null || date`
AC_SUBST(BUILD_TIMESTAMP)
AC_DEFINE_UNQUOTED(BUILD_TIMESTAMP, "$BUILD_TIMESTAMP",
                   [The time this package was configured for a build])




AC_GNU_SOURCE

AH_BOTTOM([
/* We don't want the old assuan codes anymore. */
#define _ASSUAN_ONLY_GPG_ERRORS 1

#include "gpadefs.h"
])


AM_MAINTAINER_MODE
AM_SILENT_RULES

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


#
#  Options to disable features
#
card_manager=yes
AC_MSG_CHECKING([whether to build the card manager])
AC_ARG_ENABLE(card-manager,
              AC_HELP_STRING([--disable-card-manager],
                             [build without the card manager]),
              card_manager=$enableval)
AC_MSG_RESULT($card_manager)
if test "$card_manager" = yes ; then
  AC_DEFINE(ENABLE_CARD_MANAGER, 1, [Include the card manager in the build])
fi
AM_CONDITIONAL(ENABLE_CARD_MANAGER, test "$card_manager" = yes)

keyserver_support=yes
AC_MSG_CHECKING([whether to include keyserver support])
AC_ARG_ENABLE(keyserver-support,
              AC_HELP_STRING([--disable-keyserver-support],
                             [build without keyserver support]),
              keyserver_support=$enableval)
AC_MSG_RESULT($keyserver_support)
if test "$keyserver_support" = yes ; then
  AC_DEFINE(ENABLE_KEYSERVER_SUPPORT, 1,
            [Include keyserver support in the build])
fi
AM_CONDITIONAL(ENABLE_KEYSERVER_SUPPORT, test "$keyserver_support" = yes)


#
# Find the keyserver plugins. Assume that gpgkeys_ldap is always available
#
if test "$keyserver_support" = yes ; then
  AC_PATH_PROGS(GPGKEYS_LDAP, gpg2keys_ldap gpgkeys_ldap,
               [${libexecdir}/gnupg/gpg2keys_ldap],
               [$PATH:/usr/libexec/gnupg:/usr/lib/gnupg2:/usr/lib/gnupg])
  KEYSERVER_HELPERS_DIR=`dirname $GPGKEYS_LDAP`
  if test -z "$KEYSERVER_HELPERS_DIR"; then
       AC_MSG_ERROR([[
***
*** Keyserver helpers not found. They should be contained in the GnuPG package.
***
    ]])
  fi
  AC_DEFINE_UNQUOTED(KEYSERVER_HELPERS_DIR,
                     "$KEYSERVER_HELPERS_DIR", [Path to keyserver helper tools])
fi


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
AM_GNU_GETTEXT_VERSION([0.18.1])
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

_save_libs=$LIBS
_save_cflags=$CFLAGS
LIBS="$LIBS $GPGME_LIBS"
CFLAGS="$CFLAGS $GPGME_CFLAGS"
AC_CHECK_FUNCS([gpgme_data_identify])
LIBS=$_save_libs
CFLAGS="$_save_cflags"


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
AC_MSG_NOTICE([checking for cc features])
if test "$GCC" = yes; then
    if test "$USE_MAINTAINER_MODE" = "yes"; then
        #
        # Enable all kinds of warnings.
        #
        CFLAGS="$CFLAGS -O3 -Wall -Wcast-align -Wshadow -Wstrict-prototypes"
        CFLAGS="$CFLAGS -Wformat -Wno-format-y2k -Wformat-security"

        AC_MSG_CHECKING([if gcc supports -Wno-missing-field-initializers])
        _gcc_cflags_save=$CFLAGS
        CFLAGS="-Wno-missing-field-initializers"
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[])],_gcc_wopt=yes,_gcc_wopt=no)
        AC_MSG_RESULT($_gcc_wopt)
        CFLAGS=$_gcc_cflags_save;
        if test x"$_gcc_wopt" = xyes ; then
          CFLAGS="$CFLAGS -W -Wno-sign-compare -Wno-missing-field-initializers"
        fi

        AC_MSG_CHECKING([if gcc supports -Wdeclaration-after-statement])
        _gcc_cflags_save=$CFLAGS
        CFLAGS="-Wdeclaration-after-statement"
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[])],_gcc_wopt=yes,_gcc_wopt=no)
        AC_MSG_RESULT($_gcc_wopt)
        CFLAGS=$_gcc_cflags_save;
        if test x"$_gcc_wopt" = xyes ; then
          CFLAGS="$CFLAGS -Wdeclaration-after-statement"
        fi

	# We should keep -Wstrict-prototypes here.  However, it causes
	# too many warnings in gtkitemfactory.h:51 (see comment over
	# there).  This might be solved with GTK 3.
        AC_MSG_CHECKING([if gcc supports -Wno-strict-prototypes])
        _gcc_cflags_save=$CFLAGS
        CFLAGS="-Wno-strict-prototypes"
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[])],_gcc_wopt=yes,_gcc_wopt=no)
        AC_MSG_RESULT($_gcc_wopt)
        CFLAGS=$_gcc_cflags_save;
        if test x"$_gcc_wopt" = xyes ; then
          CFLAGS="$CFLAGS -Wno-strict-prototypes"
        fi

    else
        CFLAGS="$CFLAGS -Wall"
    fi

    AC_MSG_CHECKING([if gcc supports -Wno-pointer-sign])
    _gcc_cflags_save=$CFLAGS
    CFLAGS="-Wno-pointer-sign"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[])],_gcc_wopt=yes,_gcc_wopt=no)
    AC_MSG_RESULT($_gcc_wopt)
    CFLAGS=$_gcc_cflags_save;
    if test x"$_gcc_wopt" = xyes ; then
       CFLAGS="$CFLAGS -Wno-pointer-sign"
    fi

    AC_MSG_CHECKING([if gcc supports -Wno-unused-parameter])
    _gcc_cflags_save=$CFLAGS
    CFLAGS="-Wno-unused-parameter"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[])],_gcc_wopt=yes,_gcc_wopt=no)
    AC_MSG_RESULT($_gcc_wopt)
    CFLAGS=$_gcc_cflags_save;
    if test x"$_gcc_wopt" = xyes ; then
       CFLAGS="$CFLAGS -Wno-unused-parameter"
    fi

    # We need to use ms-bitfields.
    if test "$have_w32_system" = yes; then
       CFLAGS="$CFLAGS -mms-bitfields"
    fi
fi


#
# Define name strings
AC_DEFINE_UNQUOTED(GPA_NAME, "GPA", [The name of this program])
AC_DEFINE_UNQUOTED(GPA_LONG_NAME, "GNU Privacy Assistant",
                                   [The spelled out name of this program])


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
src/versioninfo.rc
pixmaps/Makefile
doc/Makefile
])


AC_OUTPUT

echo "
        GPA v${VERSION} has been configured as follows:

        Revision:  mym4_revision  (mym4_revision_dec)
        Platform:  $host
"