
SUBDIRS = m4  intl src po pixmaps
EXTRA_DIST = config.rpath  TODO README-alpha to-utf8 autogen.sh gpa.spec gpa.desktop gpa.png
DISTCLEANFILES = gpadefs.h 

desktopdir = $(datadir)/applications/
desktop_DATA = gpa.desktop

icondir = $(datadir)/pixmaps
icon_DATA = gpa.png

pkgdata_DATA = gpa.png

# Create a VERSION file which we need for the Prereq: patch file trick.
dist-hook:
	@set -e; echo "$(VERSION)" > $(distdir)/VERSION


ACLOCAL_AMFLAGS = -I m4
