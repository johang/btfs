bin_PROGRAMS = btfs
btfs_SOURCES = btfs.cc btfs.h
btfs_CPPFLAGS = -Wall -Wextra -Wconversion -Wsign-compare -Wsign-conversion -Wno-unused-parameter $(FUSE_CFLAGS) $(LIBTORRENT_CFLAGS) $(LIBCURL_CFLAGS)
btfs_LDADD = $(FUSE_LIBS) $(LIBTORRENT_LIBS) $(LIBCURL_LIBS)
