AUTOMAKE_OPTIONS = 1.7 foreign
SUBDIRS = src
EXTRA_DIST = win32/postbuild.bat win32/shrike.sln win32/shrike.vcproj COPYING

dist-zip: distdir
	zip -rq $(distdir).zip $(distdir)
	$(am__remove_distdir)

dist dist-all: distdir
	$(AMTAR) chof - $(distdir) | GZIP=$(GZIP_ENV) gzip -c >$(distdir).tar.gz
	zip -rq $(distdir).zip $(distdir)
	$(am__remove_distdir)
