## Process this file with automake to produce Makefile.in

EXTRA_DIST = README build-dll
INCLUDES = -I$(top_srcdir)/jnlib -I$(top_srcdir)/intl

noinst_LIBRARIES = libgpapa.a
noinst_PROGRAMS = gpapatest


#libgpapa_a_LDFLAGS =
libgpapa_a_SOURCES = gpapa.c gpapa.h \
		     gpapatypedefs.h \
		     gpapafile.c      gpapafile.h \
		     gpapaintern.c    gpapaintern.h \
		     gpapakey.c       gpapakey.h     \
		     gpapapublickey.c gpapapublickey.h \
		     gpapasecretkey.c gpapasecretkey.h \
		     gpapasignature.c gpapasignature.h \
                     keyserver.c  keyserver.h \
		     rungpg.c  rungpg.h \
		     gpgme.c  gpgme.h \
		     wait.c  wait.h \
		     ops.h \
		     gpgme_types.h \
		     gpgme_io.h \
		     status-table.h \
		     util.c util.h \
		     syshdr.h \
		     posix-io.c \
		     w32-io.c \
		     posix-sema.c \
		     sema.h \
		     w32-sema.c \
		     data.c \
		     errors.c \
		     debug.c \
		     context.h

gpapatest_SOURCES = gpapatest.c
gpapatest_LDADD =  -L . -lgpapa -L ../jnlib -ljnlib -lwsock32 -liberty
