## Process this file with automake to produce Makefile.in

tip_files = gpa_tips.en gpa_tips.de
gtkrc = gtkrc

EXTRA_DIST = $(tip_files) $(gtkrc)

bin_PROGRAMS = gpa

INCLUDES = -I$(top_srcdir)/jnlib -I$(top_srcdir)/gpapa	-I$(top_srcdir)/intl
LDADD = -L ../gpapa -lgpapa -L ../jnlib -ljnlib @INTLLIBS@

gpa_SOURCES = gpa.c gpa.h \
	      keysmenu.c keysmenu.h	    \
	      filemenu.c filemenu.h \
	      gpawindowkeeper.c gpawindowkeeper.h \
	      gtktools.c gtktools.h  \
	      help.c help.h helpmenu.c helpmenu.h	  \
	      optionsmenu.c optionsmenu.h \
	      icons.c icons.h icons.xpm \
	      gpawidgets.c gpawidgets.h \
	      fileman.c fileman.h \
	      filesigndlg.c filesigndlg.h \
	      encryptdlg.c encryptdlg.h \
	      keyring.c keyring.h \
	      ownertrustdlg.c ownertrustdlg.h \
	      keysigndlg.c keysigndlg.h \
	      passphrasedlg.c passphrasedlg.h \
	      keyreceivedlg.c keyreceivedlg.h \
	      keyexportdlg.c keyexportdlg.h \
	      keygendlg.c keygendlg.h \
	      keygenwizard.c keygenwizard.h \
	      keylist.c keylist.h \
	      siglist.c siglist.h \
	      gpawizard.c gpawizard.h \
	      gpapastrings.c gpapastrings.h\
		  w32reg.c w32reg.h\
		  simple-gettext.c


install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)
	for i in $(tip_files); do \
	    $(INSTALL_DATA) $(srcdir)/$$i $(DESTDIR)$(pkgdatadir)/$$i; \
	done
	$(INSTALL_DATA) $(srcdir)/$(gtkrc) $(DESTDIR)$(pkgdatadir)/$(gtkrc);

