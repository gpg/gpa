
SUBDIRS = intl jnlib gpapa src po
EXTRA_DIST = TODO README-alpha to-utf8 autogen.sh gpa.spec
DISTCLEANFILES = gpadefs.h 


# Create a VERSION file which we need for the Prereq: patch file trick.
dist-hook:
	@set -e; echo "$(VERSION)" > $(distdir)/VERSION

