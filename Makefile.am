# Makefile.am - Top makefile for GPA.
# Copyright (C) 2000, 2001  Werner Koch
# Copyright (C) 2002, 2003, 2004  Miguel Coca
# Copyright (C) 2005, 2006, 2007, 2008, 2009  g10 Code GmbH
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
AUTOMAKE_OPTIONS = dist-bzip2

SUBDIRS = m4 src po pixmaps
EXTRA_DIST = config.rpath  TODO README-alpha to-utf8 autogen.sh gpa.spec \
 gpa.desktop gpa.png gpadefs.h.in
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
dist-hook:
	@set -e; echo "$(VERSION)" > $(distdir)/VERSION

