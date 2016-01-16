AC_PREREQ([2.69])
AC_INIT(btfs, 2.5, johan.gunnarsson@gmail.com)
AC_CONFIG_SRCDIR([src/btfs.cc])

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CXX

# Checks for libraries.
PKG_CHECK_MODULES(FUSE, fuse >= 2.8.0)
PKG_CHECK_MODULES(LIBTORRENT, libtorrent-rasterbar >= 0.16.0)
PKG_CHECK_MODULES(LIBCURL, libcurl >= 7.22.0)

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_OFF_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([memset mkdir realpath strdup])

AC_OUTPUT(Makefile src/Makefile scripts/Makefile)
