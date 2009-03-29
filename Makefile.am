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

gpadefs.h: Makefile
	rm -f $@ $@.tmp
	$(edit) '$(srcdir)/$@.in' >$@.tmp
	mv $@.tmp $@

gpadefs.h: $(srcdir)/gpadefs.h.in

all: gpadefs.h

# Create a VERSION file which we need for the Prereq: patch file trick.
dist-hook:
	@set -e; echo "$(VERSION)" > $(distdir)/VERSION

