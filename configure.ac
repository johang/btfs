AC_PREREQ([2.69])
AC_INIT(btfs, 1.0, johan.gunnarsson@gmail.com)
AC_CONFIG_SRCDIR([src/btfs.cc])

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CXX

# Checks for libraries.
PKG_CHECK_MODULES(FUSE, fuse >= 2.9.2)
PKG_CHECK_MODULES(LIBTORRENT, libtorrent-rasterbar >= 0.16.18)

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_OFF_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([memset mkdir realpath strdup])

AC_OUTPUT(Makefile src/Makefile)
