dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(onetouch, 1.0)


AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([-Wall])

AC_PROG_CC_C99
AS_IF([test "x$ac_cv_prog_cc_c99" = xno], [AC_MSG_ERROR([compiler does not support C99])])

AC_DEFINE([_POSIX_C_SOURCE], [199309L])
AC_SEARCH_LIBS([clock_gettime], [rt])  

AC_CONFIG_FILES(Makefile src/Makefile test/Makefile)
AC_OUTPUT

