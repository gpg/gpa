## Process this file with automake to produce Makefile.in

tip_files = gpa_tips.en gpa_tips.de
gtkrc = gtkrc
logo = gpa_logo.ppm

pkgdata_DATA = $(tip_files) $(gtkrc) $(logo)

EXTRA_DIST = $(tip_files) $(gtkrc)  $(logo)

bin_PROGRAMS = gpa

INCLUDES = -I$(top_srcdir)/jnlib -I$(top_srcdir)/intl
LDADD = -L ../jnlib -ljnlib @INTLLIBS@

gpa_SOURCES = gpa.c gpa.h i18n.h options.h \
	      gpawindowkeeper.c gpawindowkeeper.h \
	      gtktools.c gtktools.h  \
	      helpmenu.c helpmenu.h	  \
	      optionsmenu.c optionsmenu.h \
	      icons.c icons.h icons.xpm \
	      gpawidgets.c gpawidgets.h \
              gpafilesel.c gpafilesel.h \
	      fileman.c fileman.h \
	      filesigndlg.c filesigndlg.h \
	      encryptdlg.c encryptdlg.h \
	      verifydlg.c verifydlg.h \
	      keyring.c keyring.h \
	      ownertrustdlg.c ownertrustdlg.h \
	      keysigndlg.c keysigndlg.h \
	      keyimportdlg.c keyimportdlg.h \
	      keyimpseldlg.c keyimpseldlg.h \
	      keyexportdlg.c keyexportdlg.h \
	      keygendlg.c keygendlg.h \
	      keygenwizard.c keygenwizard.h \
              qdchkpwd.c qdchkpwd.h \
	      keyeditdlg.c keyeditdlg.h \
	      expirydlg.c expirydlg.h \
	      keydeletedlg.c keydeletedlg.h \
	      keylist.c keylist.h \
	      siglist.c siglist.h \
	      gpawizard.c gpawizard.h \
	      gpapastrings.c gpapastrings.h\
	      gpa_license.c gpa_license.h \
	      keyserver.c keyserver.h \
              w32reg.c w32reg.h \
              simple-gettext.c \
	      hidewnd.c hidewnd.h \
	      keytable.c keytable.h \
	      gpgmetools.h gpgmetools.c \
	      gpgmeedit.h gpgmeedit.c \
	      server_access.h server_access.c \
	      gpa.rc
