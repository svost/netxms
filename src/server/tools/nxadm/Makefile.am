bin_PROGRAMS = nxadm
nxadm_SOURCES = comm.cpp nxadm.cpp
nxadm_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
nxadm_LDFLAGS = @EXEC_LDFLAGS@
nxadm_LDADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la ../../../snmp/libnxsnmp/libnxsnmp.la \
	      @SERVER_LIBS@ @EXEC_LIBS@

EXTRA_DIST = \
	Makefile.w32 \
	nxadm.vcxproj nxadm.vcxproj.filters \
	nxadm.h
