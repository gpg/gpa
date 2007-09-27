## Process this file with automake to produce Makefile.in

tip_files = gpa_tips.en gpa_tips.de
logo = gpa_logo.ppm 

pkgdata_DATA = $(tip_files) $(logo)

EXTRA_DIST = $(tip_files) $(logo) gpa.ico gpa-resource.rc

if HAVE_W32_SYSTEM
.rc.o:
	$(WINDRES) `echo $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) | \
	 sed -e 's/-I/--include-dir /g;s/-D/--define /g'` -i $< -o $@

gpa_res_ldflag = -Wl,gpa-resource.o
gpa_deps = gpa-resource.o
gpa_w32_sources = w32-afunix.c w32-afunix.h
else
gpa_res_ldflag =
gpa_deps =
gpa_w32_sources =
endif


bin_PROGRAMS = gpa
noinst_PROGRAMS = dndtest confdialog

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(top_srcdir)/pixmaps
AM_CFLAGS = $(GPGME_CFLAGS) $(LIBASSUAN_CFLAGS) $(GPG_ERROR_CFLAGS) \
	    $(GTK_CFLAGS)
LDADD = $(GPGME_LIBS) $(LIBASSUAN_LIBS) $(GPG_ERROR_LIBS) \
        $(GTK_LIBS) $(LIBINTL) $(NETLIBS)
gpa_LDFLAGS = $(gpa_res_ldflag)
gpa_DEPENDENCIES = $(gpa_deps)

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
	      hidewnd.c hidewnd.h \
	      keytable.c keytable.h \
	      gpgmetools.h gpgmetools.c \
	      gpgmeedit.h gpgmeedit.c \
	      server_access.h server_access.c \
	      settingsdlg.h settingsdlg.c \
	      passwddlg.h passwddlg.c \
	      gpacontext.h gpacontext.c \
	      gpaprogressdlg.h gpaprogressdlg.c \
	      gparecvkeydlg.h gparecvkeydlg.c \
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
	      gpakeyexpireop.h gpakeyexpireop.c \
	      gpakeypasswdop.h gpakeypasswdop.c \
	      gpaexportop.h gpaexportop.c \
	      gpaexportfileop.h gpaexportfileop.c \
	      gpaexportclipop.h gpaexportclipop.c \
	      gpaexportserverop.h gpaexportserverop.c \
	      gpaimportop.h gpaimportop.c \
	      gpaimportfileop.h gpaimportfileop.c \
	      gpaimportclipop.h gpaimportclipop.c \
	      gpaimportserverop.h gpaimportserverop.c \
	      gpagenkeyop.h gpagenkeyop.c \
	      gpagenkeyadvop.h gpagenkeyadvop.c \
	      gpagenkeysimpleop.h gpagenkeysimpleop.c \
	      gpabackupop.h gpabackupop.c \
	      gpakeyselector.h gpakeyselector.c \
	      server.c \
	      options.c $(gpa_w32_sources)


dndtest_SOURCES = dndtest.c

confdialog_SOURCES = confdialog.c








