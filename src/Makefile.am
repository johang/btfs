EXTRACXXFLAGS = -std=c++14 \
                -Wall \
                -Wextra \
                -Wconversion \
                -Wsign-compare \
                -Wsign-conversion \
                -Wno-unused-parameter
bin_PROGRAMS = btfs btfsstat
btfs_SOURCES = btfs.cc btfs.h
btfs_CXXFLAGS = $(EXTRACXXFLAGS) $(FUSE_CFLAGS) $(LIBTORRENT_CFLAGS) $(LIBCURL_CFLAGS)
btfs_LDADD = $(FUSE_LIBS) $(LIBTORRENT_LIBS) $(LIBCURL_LIBS)
btfsstat_SOURCES = btfsstat.cc btfsstat.h
btfsstat_CXXFLAGS = $(EXTRACXXFLAGS)
btfsstat_LDADD =
