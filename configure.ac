dnl
dnl Configure script source for GPA
dnl
dnl (Process this file with autoconf to produce a configure script.)
AC_REVISION($Revision$)dnl
AC_PREREQ(2.52)

# The version number goes here. 
#
# The CVS version is usually the next intended release version with
# the string "-cvs" appended.  The reason for this is that tests for a
# specific feature can already be done under the assumption that the
# CVS vesiuon is the most recent one in a branch.
#
AC_INIT(gpa, 0.5.1, [bug-gpa@gnupg.org])

ALL_LINGUAS="de fr zh_TW.Big5 pt_BR"
AM_CONFIG_HEADER(config.h)
AC_CONFIG_SRCDIR(src/gpa.c)
AM_INIT_AUTOMAKE($PACKAGE_NAME, $PACKAGE_VERSION)

PACKAGE=$PACKAGE_NAME
VERSION=$PACKAGE_VERSION
AC_SUBST(PACKAGE)
AC_SUBST(VERSION)
AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE", [Name of this package])
AC_DEFINE_UNQUOTED(VERSION, "$VERSION", [Version of this package])

AC_DEFINE_UNQUOTED(PACKAGE_BUGREPORT, "$PACKAGE_BUGREPORT",
                                      [address for reporting bugs])

AH_TOP([
/* We need this, because some autoconf tests rely on this (e.g. stpcpy)
   and it should be used for new programs anyway */
#define _GNU_SOURCE  1
])
AM_MAINTAINER_MODE

dnl Check for libraries
AC_CHECK_LIB(m, sin)
AC_CHECK_LIB(z, compress)

dnl
dnl Setup some stuff depending on host/target.
dnl
try_gettext=yes
GPG=
case "${target}" in
    *-*-mingw32)
        try_gettext=no
dnl     AC_DEFINE(USE_ONLY_8DOT3,1,
dnl               [set this to limit filenames to the 8.3 format])
        AC_DEFINE(HAVE_DRIVE_LETTERS,1,
                  [defined if we must run on a stupid file system])
        AC_DEFINE(HAVE_DOSISH_SYSTEM,1,
                  [defined if we run on some of the PCDOS like systems 
                   (DOS, Windoze. OS/2) with special properties like
                    no file modes])
        AC_DEFINE(USE_SIMPLE_GETTEXT,1,
                  [because the Unix gettext has too much overhead on
                   MingW32 systems and these systems lack Posix functions,
                   we use a simplified version of gettext])
	AC_CHECK_LIB(wsock32, connect)
	AC_CHECK_LIB(iberty, getopt_long)
        GPG='c:\\gnupg\\gpg.exe'
        ;;
    *)
       ;;
esac

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
AC_DEFINE_UNQUOTED(GPG_PATH, "$GPG")


dnl
dnl Checks for compiler features
dnl



dnl
dnl Checks for libraries
dnl

if test "$try_gettext" = yes; then
AM_GNU_GETTEXT
else
USE_NLS=no
USE_INCLUDED_LIBINTL=no
BUILD_INCLUDED_LIBINTL=no
AC_SUBST(USE_NLS)
AC_SUBST(USE_INCLUDED_LIBINTL)
AC_SUBST(BUILD_INCLUDED_LIBINTL)
fi

dnl Where is the GTK+ toolkit
AM_PATH_GTK_2_0(2.0.0,
            [LIBS="$LIBS $GTK_LIBS"
            CFLAGS="$CFLAGS $GTK_CFLAGS -DGTK_ENABLE_BROKEN"],
            AC_MSG_ERROR(Cannot find GTK+ 2.0))

dnl
dnl Checks for header files
dnl
dnl These are needed by libjnlib
AC_CHECK_FUNCS(memicmp stpcpy strlwr strtoul memmove stricmp)


dnl
dnl Checks for typedefs and structures
dnl

GNUPG_CHECK_TYPEDEF(byte, HAVE_BYTE_TYPEDEF)
GNUPG_CHECK_TYPEDEF(ushort, HAVE_USHORT_TYPEDEF)
GNUPG_CHECK_TYPEDEF(ulong, HAVE_ULONG_TYPEDEF)
GNUPG_CHECK_TYPEDEF(u16, HAVE_U16_TYPEDEF)
GNUPG_CHECK_TYPEDEF(u32, HAVE_U32_TYPEDEF)
# We should not use them in this software;
# However jnlib/types.h needs them - so we take the easy way.
AC_CHECK_SIZEOF(unsigned short, 2)
AC_CHECK_SIZEOF(unsigned int, 4)
AC_CHECK_SIZEOF(unsigned long, 4)
if test "$ac_cv_sizeof_unsigned_short" = "0" \
   || test "$ac_cv_sizeof_unsigned_int" = "0" \
   || test "$ac_cv_sizeof_unsigned_long" = "0"; then
    AC_MSG_WARN([Hmmm, something is wrong with the sizes - using defaults]);
fi



if test "$GCC" = yes; then
    if test "$USE_MAINTAINER_MODE" = "yes"; then
        CFLAGS="$CFLAGS -Wall -Wcast-align -Wshadow -Wstrict-prototypes"
    else
        CFLAGS="$CFLAGS -Wall"
    fi
fi

AC_CONFIG_COMMANDS(gpadefs.h,[[
cat >gpadefs.tmp <<GPAEOF
/* Generated automatically by configure */
#ifdef HAVE_DRIVE_LETTERS
  #define GPA_LOCALEDIR "c:/lib/gnupg/locale"
  #define GPA_DATADIR   "c:/lib/gnupg"
  #define GPG_PROGRAM   "c:/gnupg/gpg"
#else
  #define GPA_LOCALEDIR "${prefix}/${DATADIRNAME}/locale"
  #define GPA_DATADIR   "${datadir}/gpa"
  #define GPG_PROGRAM   "${GPG}"
#endif
GPAEOF
if cmp -s gpadefs.h gpadefs.tmp 2>/dev/null; then
    echo "gpadefs.h is unchanged"
    rm -f gpadefs.tmp
else
    rm -f gpadefs.h
    mv gpadefs.tmp gpadefs.h
    echo "gpadefs.h created"
fi
]],[[
prefix=$prefix
exec_prefix=$exec_prefix
libdir=$libdir
datadir=$datadir
DATADIRNAME=$DATADIRNAME
GPG=$GPG
]])

AC_CONFIG_FILES([
Makefile
intl/Makefile
po/Makefile.in
jnlib/Makefile
gpapa/Makefile
src/Makefile
])


AC_OUTPUT