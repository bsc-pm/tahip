AC_DEFUN([AX_CHECK_HIP],[

#Check if an HIP implementation is installed.
AC_ARG_WITH(hip,
[AS_HELP_STRING([--with-hip,--with-hip=PATH],
                [search in system directories or specify prefix directory for installed HIP package])])
AC_ARG_WITH(hip-bin,
[AS_HELP_STRING([--with-hip-bin=PATH],
                [specify directory for installed HIP bin files])])
AC_ARG_WITH(hip-lib,
[AS_HELP_STRING([--with-hip-lib=PATH],
                [specify directory for the installed HIP library])])

# Search for HIP by default
AS_IF([test "$with_hip" != yes],[
  olibdir=$with_hip/lib
  hipbin=$with_hip/bin

  hiplib="-L$olibdir -Wl,-rpath,$olibdir"
])

AS_IF([test "x$with_hip_bin" != x],[
  hipbin="-I$with_hip_bin"
])
AS_IF([test "x$with_hip_lib" != x],[
  hiplib="-L$with_hip_lib -Wl,-rpath,$with_hip_lib"
])

hipflags=$($hipbin/hipconfig --cpp_config)

# Tests if provided headers and libraries are usable and correct
# Check headers using hipconfig flags
AX_VAR_PUSHVALUE([CPPFLAGS],[$CPPFLAGS $hipflags])
AX_VAR_PUSHVALUE([CFLAGS])
AX_VAR_PUSHVALUE([LDFLAGS],[$LDFLAGS $hiplib])
AX_VAR_PUSHVALUE([LIBS],[])

AC_CHECK_HEADERS([hip/hip_runtime.h hip/hip_runtime_api.h], [hip=yes], [hip=no])
search_libs="amdhip64"
required_libs=""

m4_foreach([function],
           [hipInit,
            hipStreamCreate,
            hipLaunchKernel,
	    hipMemcpyAsync],
           [
             AS_IF([test "$hip" = "yes"],[
               AC_SEARCH_LIBS(function,
                              [$search_libs],
                              [hip=yes],
                              [hip=no],
                              [$required_libs])dnl
             ])
           ])dnl

hiplibs=$LIBS

AX_VAR_POPVALUE([CPPFLAGS])
AX_VAR_POPVALUE([CFLAGS])
AX_VAR_POPVALUE([LDFLAGS])
AX_VAR_POPVALUE([LIBS])

AS_IF([test "$hip" != "yes"],[
    AC_MSG_ERROR([
------------------------------
HIP path was not correctly specified.
Please, check that the provided directories are correct.
------------------------------])
])

AC_SUBST([hip])
AC_SUBST([hipinc])
AC_SUBST([hiplib])
AC_SUBST([hiplibs])
AC_SUBST([hipflags])

])dnl AX_CHECK_HIP

