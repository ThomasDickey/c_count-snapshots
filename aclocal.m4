dnl $Id: aclocal.m4,v 7.3 2002/11/22 22:03:32 tom Exp $
dnl Process this file with autoconf to produce a configure script.
dnl ---------------------------------------------------------------------------
dnl ---------------------------------------------------------------------------
dnl Copy non-preprocessor flags to $CFLAGS, preprocessor flags to $CPPFLAGS
dnl The second parameter if given makes this macro verbose.
AC_DEFUN([CF_ADD_CFLAGS],
[
cf_new_cflags=
cf_new_cppflags=
for cf_add_cflags in $1
do
	case $cf_add_cflags in #(vi
	-undef|-nostdinc*|-I*|-D*|-U*|-E|-P|-C) #(vi
		case "$CPPFLAGS" in
		*$cf_add_cflags) #(vi
			;;
		*) #(vi
			cf_new_cppflags="$cf_new_cppflags $cf_add_cflags"
			;;
		esac
		;;
	*)
		cf_new_cflags="$cf_new_cflags $cf_add_cflags"
		;;
	esac
done

if test -n "$cf_new_cflags" ; then
	ifelse($2,,,[CF_VERBOSE(add to \$CFLAGS $cf_new_cflags)])
	CFLAGS="$CFLAGS $cf_new_cflags"
fi

if test -n "$cf_new_cppflags" ; then
	ifelse($2,,,[CF_VERBOSE(add to \$CPPFLAGS $cf_new_cppflags)])
	CPPFLAGS="$CPPFLAGS $cf_new_cppflags"
fi

])dnl
dnl ---------------------------------------------------------------------------
dnl This is adapted from the macros 'fp_PROG_CC_STDC' and 'fp_C_PROTOTYPES'
dnl in the sharutils 4.2 distribution.
AC_DEFUN([CF_ANSI_CC_CHECK],
[
AC_CACHE_CHECK(for ${CC-cc} option to accept ANSI C, cf_cv_ansi_cc,[
cf_cv_ansi_cc=no
cf_save_CFLAGS="$CFLAGS"
cf_save_CPPFLAGS="$CPPFLAGS"
# Don't try gcc -ansi; that turns off useful extensions and
# breaks some systems' header files.
# AIX			-qlanglvl=ansi
# Ultrix and OSF/1	-std1
# HP-UX			-Aa -D_HPUX_SOURCE
# SVR4			-Xc
# UnixWare 1.2		(cannot use -Xc, since ANSI/POSIX clashes)
for cf_arg in "-DCC_HAS_PROTOS" \
	"" \
	-qlanglvl=ansi \
	-std1 \
	-Ae \
	"-Aa -D_HPUX_SOURCE" \
	-Xc
do
	CF_ADD_CFLAGS($cf_arg)
	AC_TRY_COMPILE(
[
#ifndef CC_HAS_PROTOS
#if !defined(__STDC__) || (__STDC__ != 1)
choke me
#endif
#endif
],[
	int test (int i, double x);
	struct s1 {int (*f) (int a);};
	struct s2 {int (*f) (double a);};],
	[cf_cv_ansi_cc="$cf_arg"; break])
done
CFLAGS="$cf_save_CFLAGS"
CPPFLAGS="$cf_save_CPPFLAGS"
])

if test "$cf_cv_ansi_cc" != "no"; then
if test ".$cf_cv_ansi_cc" != ".-DCC_HAS_PROTOS"; then
	CF_ADD_CFLAGS($cf_cv_ansi_cc)
else
	AC_DEFINE(CC_HAS_PROTOS)
fi
fi
])dnl
dnl ---------------------------------------------------------------------------
dnl For programs that must use an ANSI compiler, obtain compiler options that
dnl will make it recognize prototypes.  We'll do preprocessor checks in other
dnl macros, since tools such as unproto can fake prototypes, but only part of
dnl the preprocessor.
AC_DEFUN([CF_ANSI_CC_REQD],
[AC_REQUIRE([CF_ANSI_CC_CHECK])
if test "$cf_cv_ansi_cc" = "no"; then
	AC_ERROR(
[Your compiler does not appear to recognize prototypes.
You have the following choices:
	a. adjust your compiler options
	b. get an up-to-date compiler
	c. use a wrapper such as unproto])
fi
])dnl
dnl ---------------------------------------------------------------------------
dnl Check if the compiler supports useful warning options.  There's a few that
dnl we don't use, simply because they're too noisy:
dnl
dnl	-Wconversion (useful in older versions of gcc, but not in gcc 2.7.x)
dnl	-Wredundant-decls (system headers make this too noisy)
dnl	-Wtraditional (combines too many unrelated messages, only a few useful)
dnl	-Wwrite-strings (too noisy, but should review occasionally)
dnl	-pedantic
dnl
AC_DEFUN([CF_GCC_WARNINGS],
[
if test "$GCC" = yes
then
	cat > conftest.$ac_ext <<EOF
#line __oline__ "configure"
int main(int argc, char *argv[[]]) { return (argv[[argc-1]] == 0) ; }
EOF
	AC_CHECKING([for $CC warning options])
	cf_save_CFLAGS="$CFLAGS"
	EXTRA_CFLAGS="-W -Wall"
	cf_warn_CONST=""
	test "$with_ext_const" = yes && cf_warn_CONST="Wwrite-strings"
	for cf_opt in \
		Wbad-function-cast \
		Wcast-align \
		Wcast-qual \
		Winline \
		Wmissing-declarations \
		Wmissing-prototypes \
		Wnested-externs \
		Wpointer-arith \
		Wshadow \
		Wstrict-prototypes \
		Wundef $cf_warn_CONST
	do
		CFLAGS="$cf_save_CFLAGS $EXTRA_CFLAGS -$cf_opt"
		if AC_TRY_EVAL(ac_compile); then
			test -n "$verbose" && AC_MSG_RESULT(... -$cf_opt)
			EXTRA_CFLAGS="$EXTRA_CFLAGS -$cf_opt"
			test "$cf_opt" = Wcast-qual && EXTRA_CFLAGS="$EXTRA_CFLAGS -DXTSTRINGDEFINES"
		fi
	done
	rm -f conftest*
	CFLAGS="$cf_save_CFLAGS"
fi
AC_SUBST(EXTRA_CFLAGS)
])dnl
dnl ---------------------------------------------------------------------------
dnl Check for getopt's variables which are commonly defined in stdlib.h or
dnl (nonstandard) in getopt.h
AC_DEFUN([CF_GETOPT_HEADER],
[
AC_CACHE_CHECK(for header declaring getopt variables,cf_cv_getopt_header,[
cf_cv_getopt_header=none
for cf_header in stdio.h stdlib.h getopt.h
do
AC_TRY_COMPILE([
#include <$cf_header>],
[int x = optind; char *y = optarg],
[cf_cv_getopt_header=$cf_header
 break])
done
])
case $cf_cv_getopt_header in #(vi
getopt.h) #(vi
	AC_DEFINE(HAVE_GETOPT_H)
	AC_DEFINE(HAVE_GETOPT_HEADER)
	;;
stdlib.h) #(vi
	AC_DEFINE(HAVE_STDLIB_H)
	AC_DEFINE(HAVE_GETOPT_HEADER)
	;;
esac
])dnl