AC_PREREQ([2.69])
AC_INIT(btfs, 2.22, johan.gunnarsson@gmail.com, btfs, https://github.com/johang/btfs)
AC_CONFIG_SRCDIR([src/btfs.cc])

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CXX

# Checks for libraries.
PKG_CHECK_MODULES(FUSE, fuse >= 2.8.0)
PKG_CHECK_MODULES(LIBTORRENT, libtorrent-rasterbar >= 1.0.0)
PKG_CHECK_MODULES(LIBCURL, libcurl >= 7.22.0)

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_OFF_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([memset memcpy strcmp mkdir strdup realpath getxattr dirname basename])

# Check for unportable pthread_setname_np()
AC_CHECK_LIB(pthread, pthread_setname_np)

# Check if -latomic is needed.
AC_SEARCH_LIBS(__atomic_load, atomic)

AC_OUTPUT(Makefile src/Makefile scripts/Makefile man/Makefile)
