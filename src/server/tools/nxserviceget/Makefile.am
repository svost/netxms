bin_PROGRAMS = nxserviceget
nxserviceget_SOURCES = nxserviceget.cpp
nxserviceget_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
nxserviceget_LDFLAGS = @EXEC_LDFLAGS@
nxserviceget_LDADD = \
	      ../../libnxsrv/libnxsrv.la \
	      ../../../libnxsl/libnxsl.la \
	      ../../../snmp/libnxsnmp/libnxsnmp.la \
	      ../../../libnetxms/libnetxms.la \
	      @EXEC_LIBS@

EXTRA_DIST = \
	Makefile.w32 \
	nxserviceget.vcxproj nxserviceget.vcxproj.filters
