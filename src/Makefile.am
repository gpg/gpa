## Process this file with automake to produce Makefile.in

bin_PROGRAMS = gpa


gpa_SOURCES = gpa.c gpa.h \
	      gpafile.c gpafile.h \
	      filemenu.c filemenu.h \
	      gtktools.c gtktools.h  \
	      helpmenu.c helpmenu.h	   \
	      keysmenu.c keysmenu.h	    \
	      optionsmenu.c optionsmenu.h

LDADD = -L ../../pgg/src -lpgg
INCLUDES = -I ../../pgg/src

