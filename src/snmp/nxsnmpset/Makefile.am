bin_PROGRAMS = nxsnmpset
nxsnmpset_SOURCES = nxsnmpset.cpp
nxsnmpset_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
nxsnmpset_LDFLAGS = @EXEC_LDFLAGS@
nxsnmpset_LDADD = ../../libnetxms/libnetxms.la ../libnxsnmp/libnxsnmp.la @EXEC_LIBS@

EXTRA_DIST = \
	Makefile.w32 \
	nxsnmpset.vcxproj nxsnmpset.vcxproj.filters
