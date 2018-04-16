# Makefile.am - Top makefile for GPA.
# Copyright (C) 2000, 2001  Werner Koch
# Copyright (C) 2002, 2003, 2004  Miguel Coca
# Copyright (C) 2005-2014  g10 Code GmbH
#
# This file is part of GPA.
#
# GPA is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your
# option) any later version.
#
# GPA is distributed in the hope that it will be useful but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
# License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.

## Process this file with automake to produce Makefile.in

localedir = $(datadir)/locale

logo = gpa-logo.ppm

pkgdata_DATA = $(logo)

EXTRA_DIST = $(logo) gpa.ico gpa-resource.rc versioninfo.rc.in \
	     gpa-marshal.list Signals
BUILT_SOURCES = gpa-marshal.h gpa-marshal.c
MOSTLYCLEANFILES = gpa-marshal.h gpa-marshal.c

GLIB_GENMARSHAL = glib-genmarshal

gpa-marshal.h: gpa-marshal.list
	$(GLIB_GENMARSHAL) --header --prefix=gpa_marshal $< > $@.tmp
	mv $@.tmp $@

gpa-marshal.c: gpa-marshal.list
	$(GLIB_GENMARSHAL) --body --prefix=gpa_marshal $< > $@.tmp
	mv $@.tmp $@

if HAVE_W32_SYSTEM
.rc.o:
	$(WINDRES) `echo $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) | \
	 sed -e 's/-I/--include-dir /g;s/-D/--define /g'` -i $< -o $@

gpa_res_ldflag = -Wl,gpa-resource.o
gpa_deps = gpa-resource.o versioninfo.rc
gpa_w32_sources = w32reg.h w32reg.c w32-afunix.h w32-afunix.c

else
gpa_res_ldflag =
gpa_deps =
gpa_w32_sources =
endif


bin_PROGRAMS = gpa
if HAVE_W32_SYSTEM
 bin_PROGRAMS += launch-gpa
endif

noinst_PROGRAMS = dndtest

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(top_srcdir)/pixmaps
AM_CPPFLAGS += -DLOCALEDIR=\"$(localedir)\"

AM_CFLAGS = $(GPGME_CFLAGS) $(LIBASSUAN_CFLAGS) $(GPG_ERROR_CFLAGS) \
	    $(GTK_CFLAGS)
LDADD = $(GPGME_LIBS) $(LIBASSUAN_LIBS) $(GPG_ERROR_LIBS) \
        $(GTK_LIBS) $(LIBINTL) $(NETLIBS)
gpa_LDFLAGS = $(gpa_res_ldflag)
gpa_DEPENDENCIES = $(gpa_deps)

if HAVE_W32_SYSTEM
launch_gpa_SOURCES = launch-gpa.c
launch_gpa_LDFLAGS = -mwindows
endif

if ENABLE_CARD_MANAGER
gpa_cardman_sources = \
                cardman.c cardman.h \
                cm-object.c cm-object.h \
                cm-openpgp.c cm-openpgp.h \
		cm-geldkarte.c cm-geldkarte.h \
		cm-netkey.c cm-netkey.h \
		cm-dinsig.c cm-dinsig.h \
		cm-unknown.c cm-unknown.h
else
gpa_cardman_sources =
endif

if ENABLE_KEYSERVER_SUPPORT
keyserver_support_sources = \
		server-access.c     	\
		gpaimportserverop.c	\
		gpaimportbykeyidop.c	\
		gpaexportserverop.c
else
keyserver_support_sources =
endif

gpa_SOURCES = \
              get-path.h get-path.c \
	      gpa.c gpa.h i18n.h options.h \
	      gpawindowkeeper.c gpawindowkeeper.h \
	      gtktools.c gtktools.h  \
	      helpmenu.c helpmenu.h	  \
	      icons.c icons.h \
	      gpawidgets.c gpawidgets.h \
	      fileman.c fileman.h \
	      clipboard.h clipboard.c \
	      filesigndlg.c filesigndlg.h \
	      encryptdlg.c encryptdlg.h \
	      verifydlg.c verifydlg.h \
              recipientdlg.c recipientdlg.h \
              selectkeydlg.c selectkeydlg.h \
	      keymanager.c keymanager.h \
	      gpa-key-details.c gpa-key-details.h \
	      gpa-uid-list.c gpa-uid-list.h \
	      gpa-tofu-list.c gpa-tofu-list.h \
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
              certchain.c certchain.h \
	      gpl-text.c gpl-text.h \
	      keyserver.c keyserver.h \
	      hidewnd.c hidewnd.h \
	      keytable.c keytable.h \
	      gpgmetools.h gpgmetools.c \
	      gpgmeedit.h gpgmeedit.c \
	      server-access.h $(keyserver_support_sources) \
	      settingsdlg.h settingsdlg.c \
	      passwddlg.h passwddlg.c \
	      gpacontext.h gpacontext.c \
	      gpaprogressdlg.h gpaprogressdlg.c \
	      gpaprogressbar.h gpaprogressbar.c \
	      gparecvkeydlg.h gparecvkeydlg.c \
	      gpaoperation.h gpaoperation.c \
	      gpastreamop.h gpastreamop.c  \
	      gpastreamencryptop.h gpastreamencryptop.c  \
	      gpastreamsignop.h gpastreamsignop.c  \
	      gpastreamverifyop.h gpastreamverifyop.c  \
	      gpastreamdecryptop.h gpastreamdecryptop.c  \
	      gpafileop.h gpafileop.c \
	      gpafiledecryptop.h gpafiledecryptop.c \
	      gpafileencryptop.h gpafileencryptop.c \
	      gpafilesignop.h gpafilesignop.c \
	      gpafileverifyop.h gpafileverifyop.c \
	      gpafileimportop.h gpafileimportop.c \
	      gpakeyop.h gpakeyop.c \
	      gpakeydeleteop.h gpakeydeleteop.c \
	      gpakeysignop.h gpakeysignop.c \
	      gpakeytrustop.h gpakeytrustop.c \
	      gpakeyexpireop.h gpakeyexpireop.c \
	      gpakeypasswdop.h gpakeypasswdop.c \
	      gpaexportop.h gpaexportop.c \
	      gpaexportfileop.h gpaexportfileop.c \
	      gpaexportclipop.h gpaexportclipop.c \
	      gpaexportserverop.h  \
	      gpaimportop.h gpaimportop.c \
	      gpaimportfileop.h gpaimportfileop.c \
	      gpaimportclipop.h gpaimportclipop.c \
	      gpaimportserverop.h  \
	      gpaimportbykeyidop.h  \
	      gpagenkeyop.h gpagenkeyop.c \
	      gpagenkeyadvop.h gpagenkeyadvop.c \
	      gpagenkeysimpleop.h gpagenkeysimpleop.c \
	      gpagenkeycardop.h gpagenkeycardop.c \
	      gpabackupop.h gpabackupop.c \
	      gpakeyselector.h gpakeyselector.c \
	      gpapinchange.h gpapinchange.c \
	      gpadatebutton.c gpadatebutton.h \
	      gpadatebox.c gpadatebox.h \
	      server.c \
	      filewatch.c \
	      options.c \
	      confdialog.h confdialog.c \
	      gpa-marshal.c gpa-marshal.h \
	      strlist.h strlist.c \
	      gpg-stuff.h gpg-stuff.c \
              format-dn.c format-dn.h \
	      convert.c convert.h \
              membuf.c membuf.h \
	      parsetlv.c parsetlv.h \
	      filetype.c filetype.h \
	      utils.c $(gpa_w32_sources) $(gpa_cardman_sources)

dndtest_SOURCES = dndtest.c
