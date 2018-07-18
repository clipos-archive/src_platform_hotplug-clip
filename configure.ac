# SPDX-License-Identifier: GPL-2.0
# Copyright © 2009-2018 ANSSI. All Rights Reserved.
dnl Process this file with autoconf to produce a configure script.
AC_INIT(hotplug/hotplug.c)

dnl Checks for programs.
AC_PROG_CC

[
if test "x$GCC" = "xyes"; then
  CFLAGS="$CFLAGS -Wall -Wextra -Werror \
	-Wstrict-prototypes -Wmissing-prototypes \
	-Wcast-qual -Wcast-align -Wpointer-arith \
	-Wnested-externs -Wno-error=sign-compare"
fi
]

if test -z "$PKG_CONFIG"; then
	PKG_CONFIG=pkg-config
fi
AC_SUBST(PKG_CONFIG)
if "$PKG_CONFIG" --version > /dev/null 2>&1 ; then
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
  AC_MSG_ERROR([*** $PKG_CONFIG missing ***

    \'$PKG_CONFIG --version\' failed to run - make sure it is inside one of the
    directories in your PATH environment variable!

    pkg-config comes with the developement packages for GTK+-2.2, available at:

    with the developement packages for GTK+-2.2, available at:

    http://www.gtk.org])
fi

AC_DEFUN([CLIP_REQUIRE], [
AC_MSG_CHECKING(that $1 (version >= $2) is installed)
if "$PKG_CONFIG" --atleast-version=$2 $1 ; then
  LIBS="$LIBS `$PKG_CONFIG --libs $1`"
  CFLAGS="$CFLAGS `$PKG_CONFIG --cflags $1`"
  AC_MSG_RESULT(yes) 
else
  AC_MSG_RESULT(no)
  if "$PKG_CONFIG" --exists $1 ; then
      AC_MSG_ERROR(Current version is only `"$PKG_CONFIG" "$1" --modversion`)
  else
      AC_MSG_ERROR(Package is not installed)
  fi
fi
])

CLIP_REQUIRE(clip, 1.2.3)


dnl Check for debug option
AC_ARG_ENABLE(debug,   
		AC_HELP_STRING([--enable-debug], [debugging output]),
			[debug=$enableval], [debug=no])
AC_SUBST(debug)

AC_OUTPUT(Makefile 
	common/Makefile
	device-admind/Makefile 
	eventd/Makefile
	hotplug/Makefile
	input-attach/Makefile
	input-gettype/Makefile
	input-notifier/Makefile
	scripts/Makefile 
	img/Makefile)
