AC_DEFUN([AX_CHECK_HIPBLAS],[

# Check if an hipBLAS implementation is installed.
AC_ARG_WITH(hipblas,
[AS_HELP_STRING([--with-hipblas,--with-hipblas=PATH],
                [search in system directories or specify prefix directory for installed hipBLAS package])])
AC_ARG_WITH(hipblas-include,
[AS_HELP_STRING([--with-hipblas-include=PATH],
                [specify directory for installed hipBLAS include files])])
AC_ARG_WITH(hipblas-lib,
[AS_HELP_STRING([--with-hipblas-lib=PATH],
                [specify directory for the installed hipBLAS library])])

# Search for hipBLAS by default
AS_IF([test "$with_hipblas" != yes],[
  olibdir=$with_hipblas/lib

  hipblasinc="-I$with_hipblas/include"
  hipblaslib="-L$olibdir -Wl,-rpath,$olibdir"
])

AS_IF([test "x$with_hipblas_include" != x],[
  hipblasinc="-I$with_hipblas_include"
])
AS_IF([test "x$with_hipblas_lib" != x],[
  hipblaslib="-L$with_hipblas_lib -Wl,-rpath,$with_hipblas_lib"
])

# Tests if provided headers and libraries are usable and correct
# Check headers using hipconfig flags
AX_VAR_PUSHVALUE([CPPFLAGS],[$CPPFLAGS $hipflags])
AX_VAR_PUSHVALUE([CFLAGS])
AX_VAR_PUSHVALUE([LDFLAGS],[$LDFLAGS $hipblaslib])
AX_VAR_PUSHVALUE([LIBS],[])

AC_CHECK_HEADERS([hipblas.h], [hipblas=yes], [hipblas=no])

search_libs="hipblas"
required_libs=""

m4_foreach([function],
           [hipblasGemmEx],
           [
             AS_IF([test "$hipblas" = "yes"],[
               AC_SEARCH_LIBS(function,
                              [$search_libs],
                              [hipblas=yes],
                              [hipblas=no],
                              [$required_libs])dnl
             ])
           ])dnl

hipblaslibs=$LIBS

AX_VAR_POPVALUE([CPPFLAGS])
AX_VAR_POPVALUE([CFLAGS])
AX_VAR_POPVALUE([LDFLAGS])
AX_VAR_POPVALUE([LIBS])

AS_IF([test "$hipblas" != "yes"],[
    AC_MSG_ERROR([
------------------------------
hipBLAS path was not correctly specified.
Please, check that the provided directories are correct.
------------------------------])
])

AC_SUBST([hipblasinc])
AC_SUBST([hipblaslibs])

])dnl AX_CHECK_HIPBLAS

