AC_INIT([libgetline], [1.0])
AC_CONFIG_SRCDIR([src/libgetline.c])
AC_CONFIG_SRCDIR([include/libgetline.h])
AC_CONFIG_SRCDIR([include/libgetlineP.h])
AC_CONFIG_HEADERS([include/config.h])
AC_CHECK_HEADERS([stdlib.h string.h stddef.h limits.h errno.h stdio.h unistd.h sys/types.h sys/stat.h fcntl.h], [], [AC_MSG_ERROR("Header-file not found.")])
AC_CHECK_HEADER([stdinta.h], [], [AC_DEFINE(["SIZE_MAX"], [(~(size_t)0)])]) 
AC_PROG_CC
AC_OUTPUT
