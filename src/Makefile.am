## Process this file with automake to produce Makefile.in

tip_files = gpa_tips.en gpa_tips.de

EXTRA_DIST = $(tip_files)

bin_PROGRAMS = gpa

INCLUDES = -I$(top_srcdir)/jnlib -I$(top_srcdir)/gpapa	-I$(top_srcdir)/intl
LDADD = -L ../gpapa -lgpapa -L ../jnlib -ljnlib

gpa_SOURCES = gpa.c gpa.h \
	      filemenu.c filemenu.h \
	      gpawindowkeeper.c gpawindowkeeper.h \
	      gtktools.c gtktools.h  \
	      help.c helpmenu.c helpmenu.h	  \
	      keysmenu.c keysmenu.h	    \
	      optionsmenu.c optionsmenu.h \
	      icons.xpm

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)
	for i in $(tip_files); do \
	    $(INSTALL_DATA) $(srcdir)/$$i $(DESTDIR)$(pkgdatadir)/$$i; \
	done

