# This Makefile.am is in the public domain
pcfiles = \
       gnunetarm.pc \
       gnunetats.pc \
       gnunetblock.pc \
       gnunetconsensus.pc \
       gnunetconversation.pc \
       gnunetcore.pc \
       gnunetdatacache.pc \
       gnunetdatastore.pc \
       gnunetdht.pc \
       gnunetdns.pc \
       gnunetenv.pc \
       gnunetfragmentation.pc \
       gnunetfs.pc \
       gnunetgns.pc \
       gnunethello.pc \
       gnunetidentity.pc \
       gnunetcadet.pc \
       gnunetmicrophone.pc \
       gnunetmysql.pc \
       gnunetnamestore.pc \
       gnunetnat.pc \
       gnunetnse.pc \
       gnunetpeerinfo.pc \
       gnunetregex.pc \
       gnunetrevocation.pc \
       gnunetrps.pc \
       gnunetscalarproduct.pc \
       gnunetset.pc \
       gnunetspeaker.pc \
       gnunetstatistics.pc \
       gnunettestbed.pc \
       gnunettesting.pc \
       gnunettransport.pc \
       gnunetutil.pc \
       gnunetvpn.pc

all-local: $(pcfiles)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(pcfiles)

EXTRA_DIST = \
       gnunetarm.pc.in \
       gnunetats.pc.in \
       gnunetblock.pc.in \
       gnunetconsensus.pc.in \
       gnunetconversation.pc.in \
       gnunetcore.pc.in \
       gnunetdatacache.pc.in \
       gnunetdatastore.pc.in \
       gnunetdht.pc.in \
       gnunetdns.pc.in \
       gnunetenv.pc.in \
       gnunetfragmentation.pc.in \
       gnunetfs.pc.in \
       gnunetgns.pc.in \
       gnunethello.pc.in \
       gnunetidentity.pc.in \
       gnunetcadet.pc.in \
       gnunetmicrophone.pc.in \
       gnunetmysql.pc.in \
       gnunetnamestore.pc.in \
       gnunetnat.pc.in \
       gnunetnse.pc.in \
       gnunetpeerinfo.pc.in \
       gnunetpostgres.pc.in \
       gnunetregex.pc.in \
       gnunetrevocation.pc.in \
       gnunetrps.pc.in \
       gnunetscalarproduct.pc.in \
       gnunetset.pc.in \
       gnunetspeaker.pc.in \
       gnunetstatistics.pc.in \
       gnunettestbed.pc.in \
       gnunettesting.pc.in \
       gnunettransport.pc.in \
       gnunetutil.pc.in \
       gnunetvpn.pc.in

CLEANFILES = $(pcfiles)
AM_CPPFLAGS = -I$(top_srcdir)/src/include
