# Makefile.am - Top makefile for GPA.
# Copyright (C) 2000, 2001  Werner Koch
# Copyright (C) 2002, 2003, 2004  Miguel Coca
# Copyright (C) 2005-2013  g10 Code GmbH
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

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = m4 src po pixmaps doc

EXTRA_DIST = 								\
	autogen.sh autogen.rc TODO to-utf8 gpa.spec 			\
	gpa.desktop gpa.png gpadefs.h.in      				\
	po/ChangeLog-2011 pixmaps/ChangeLog-2011 ChangeLog-2011 	\
	m4/ChangeLog-2011 src/ChangeLog-2011 st-fonts/ChangeLog-2011

DISTCLEANFILES = gpadefs.h

desktopdir = $(datadir)/applications/
desktop_DATA = gpa.desktop

icondir = $(datadir)/pixmaps
icon_DATA = gpa.png

pkgdata_DATA = gpa.png

edit = sed \
        -e 's|@datadir[@]|$(pkgdatadir)|g' \
        -e 's|@prefix[@]|$(prefix)|g'

# FIXME: Let configure do this.
gpadefs.h: Makefile
	rm -f $@ $@.tmp
	$(edit) '$(srcdir)/$@.in' >$@.tmp
	mv $@.tmp $@

gpadefs.h: $(srcdir)/gpadefs.h.in

all: gpadefs.h

# Create a VERSION file which we need for the Prereq: patch file trick.
dist-hook: gen-ChangeLog
	@set -e; echo "$(VERSION)" > $(distdir)/VERSION

gen_start_date = 2011-12-12T00:00:00
.PHONY: gen-ChangeLog
gen-ChangeLog:
	if test -d $(top_srcdir)/.git; then				\
	  (cd $(top_srcdir) &&                      			\
	    ./build-aux/gitlog-to-changelog				\
	    --append-dot --tear-off					\
	    --amend=build-aux/git-log-fix				\
	    --since=$(gen_start_date) ) > $(distdir)/cl-t;		\
          cat $(top_srcdir)/build-aux/git-log-footer >> $(distdir)/cl-t;\
	  rm -f $(distdir)/ChangeLog;					\
	  mv $(distdir)/cl-t $(distdir)/ChangeLog;			\
	fi
