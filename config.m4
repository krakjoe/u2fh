PHP_ARG_WITH(u2fh, for u2fh support,
[  --with-u2fh             Include u2fh support])

if test "$PHP_U2FH" != "no"; then
  SEARCH_PATH="/usr/local /usr" 
  SEARCH_FOR="/include/u2f-host/u2f-host.h"
  if test -r $PHP_U2FH/$SEARCH_FOR; then
    U2FH_DIR=$PHP_U2FH
  else
    AC_MSG_CHECKING([for u2f files in default path])
    for i in $SEARCH_PATH ; do
      if test -r $i/$SEARCH_FOR; then
        U2FH_DIR=$i
        AC_MSG_RESULT(found in $i)
      fi
    done
  fi

  if test -z "$U2FH_DIR"; then
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([Please reinstall the u2fh and u2fs distribution])
  fi

  # --with-u2fh -> add include path
  PHP_ADD_INCLUDE($U2FH_DIR/include)
  PHP_ADD_INCLUDE($U2FH_DIR/include/u2f-host)

  LIBNAME=u2f-host # you may want to change this
  LIBSYMBOL=u2fh_global_done # you most likely want to change this 

  PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  [
    PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $U2FH_DIR/$PHP_LIBDIR, U2FH_SHARED_LIBADD)
    AC_DEFINE(HAVE_U2FHLIB,1,[ ])
  ],[
    AC_MSG_ERROR([wrong u2f-host lib version or lib not found])
  ],[
    -L$U2FH_DIR/$PHP_LIBDIR -lm
  ])
  dnl
  PHP_SUBST(U2FH_SHARED_LIBADD)

  PHP_NEW_EXTENSION(u2fh, u2fh.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi
