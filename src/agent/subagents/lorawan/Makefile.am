SUBAGENT = lorawan

pkglib_LTLIBRARIES = lorawan.la
lorawan_la_SOURCES = commproc.cpp lslink.cpp main.cpp mqtt_client.cpp
lorawan_la_CPPFLAGS=-I@top_srcdir@/include @MQTT_CPPFLAGS@ -I@top_srcdir@/build
lorawan_la_LDFLAGS = -module -avoid-version -export-symbols ../subagent.sym @MQTT_LDFLAGS@
lorawan_la_LIBADD = ../../libnxagent/libnxagent.la ../../../libnetxms/libnetxms.la ../../../libnxmb/libnxmb.la -lcurl @MQTT_LIBS@

EXTRA_DIST = lorawan.h Makefile.w32

if !STATIC_BUILD
install-exec-hook:
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
