SUBAGENT = dbquery

pkglib_LTLIBRARIES = dbquery.la
dbquery_la_SOURCES = conn.cpp dbquery.cpp direct.cpp poll.cpp
dbquery_la_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
dbquery_la_LDFLAGS = -module -avoid-version -export-symbols ../subagent.sym
dbquery_la_LIBADD = ../../libnxagent/libnxagent.la ../../../libnetxms/libnetxms.la ../../../db/libnxdb/libnxdb.la

EXTRA_DIST = dbquery.vcxproj dbquery.vcxproj.filters dbquery.h Makefile.w32

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
