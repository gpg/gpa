
SUBDIRS = m4  intl src po pixmaps
EXTRA_DIST = config.rpath  TODO README-alpha to-utf8 autogen.sh gpa.spec gpa-logo-48x48.png
DISTCLEANFILES = gpadefs.h 


# Create a VERSION file which we need for the Prereq: patch file trick.
dist-hook:
	@set -e; echo "$(VERSION)" > $(distdir)/VERSION


ACLOCAL_AMFLAGS = -I m4
