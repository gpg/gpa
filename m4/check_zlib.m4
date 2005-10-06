# check_zlib.m4 - autoconf macro to detect zlib.
# Copyright (C) 2005 g10 Code GmbH
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This file is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


dnl zlib test
AC_DEFUN([CHECK_ZLIB], [
AC_ARG_WITH(zlib, AC_HELP_STRING([--with-zlib=PATH],
[Look for zlib library installed in PATH/lib and headers in
PATH/include rather than default include and library paths. (Use an
absolute path)]),
    CPPFLAGS="$CPPFLAGS -I${withval}/include"
    LDFLAGS="$LDFLAGS -L${withval}/lib",
)

AC_CHECK_LIB(z, compress,
	LIBS="$LIBS -lz", 
	AC_ERROR([GPA requires zlib (http://gzip.org/zlib or install Debian package zlib1g-dev)]))

AC_SUBST(LIBZ)
])
