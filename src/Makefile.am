## Process this file with automake to produce Makefile.in

bin_PROGRAMS = gpa

INCLUDES = -I$(top_srcdir)/jnlib -I$(top_srcdir)/gpapa	-I$(top_srcdir)/intl
LDADD = -L ../gpapa -lgpapa -L ../jnlib -ljnlib

gpa_SOURCES = gpa.c gpa.h \
	      filemenu.c filemenu.h \
              gpawindowkeeper.c gpawindowkeeper.h \
	      gtktools.c gtktools.h  \
	      helpmenu.c helpmenu.h	   \
	      keysmenu.c keysmenu.h	    \
	      optionsmenu.c optionsmenu.h


