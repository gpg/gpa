## Process this file with automake to produce Makefile.in

tip_files = gpa_tips.en gpa_tips.de
logo = gpa_logo.ppm

pkgdata_DATA = $(tip_files) $(logo)

EXTRA_DIST = $(tip_files) $(logo)

bin_PROGRAMS = gpa

INCLUDES = -I$(top_srcdir)/intl -I$(top_srcdir)/pixmaps
LDADD = @INTLLIBS@

gpa_SOURCES = gpa.c gpa.h i18n.h options.h \
	      gpawindowkeeper.c gpawindowkeeper.h \
	      gtktools.c gtktools.h  \
	      helpmenu.c helpmenu.h	  \
	      icons.c icons.h \
	      gpawidgets.c gpawidgets.h \
	      fileman.c fileman.h \
	      filesigndlg.c filesigndlg.h \
	      encryptdlg.c encryptdlg.h \
	      verifydlg.c verifydlg.h \
	      keyring.c keyring.h \
	      ownertrustdlg.c ownertrustdlg.h \
	      keysigndlg.c keysigndlg.h \
	      keyimportdlg.c keyimportdlg.h \
	      keyexportdlg.c keyexportdlg.h \
	      keygendlg.c keygendlg.h \
	      keygenwizard.c keygenwizard.h \
              qdchkpwd.c qdchkpwd.h \
	      keyeditdlg.c keyeditdlg.h \
	      expirydlg.c expirydlg.h \
	      keydeletedlg.c keydeletedlg.h \
	      keylist.c keylist.h \
	      siglist.c siglist.h \
	      gpasubkeylist.c gpasubkeylist.h \
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
	      settingsdlg.h settingsdlg.c \
	      passwddlg.h passwddlg.c \
	      gpacontext.h gpacontext.c \
	      gpaprogressdlg.h gpaprogressdlg.c \
	      gpaoperation.h gpaoperation.c \
	      gpafileop.h gpafileop.c \
	      gpafiledecryptop.h gpafiledecryptop.c \
	      gpafileencryptop.h gpafileencryptop.c \
	      gpafilesignop.h gpafilesignop.c \
	      gpafileverifyop.h gpafileverifyop.c \
	      gpakeyop.h gpakeyop.c \
	      gpakeydeleteop.h gpakeydeleteop.c \
	      gpakeysignop.h gpakeysignop.c \
	      gpakeytrustop.h gpakeytrustop.c \
	      gpakeyselector.h gpakeyselector.c \
	      options.c gpa.rc
