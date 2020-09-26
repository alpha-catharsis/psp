# Initial tasks
AC_PREREQ([2.69])
AC_INIT([psp], [1.1.2], [alpha.catharsis@gmail.com])
AC_CONFIG_SRCDIR([src/master/main.c])
AC_CONFIG_HEADERS([src/config.h])
AM_INIT_AUTOMAKE([foreign])
LT_INIT

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC_C99
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_MKDIR_P

# Use the C language and compiler for the following checks
AC_LANG([C])

# Checks for libraries.
AC_CHECK_LIB([m], [floor])
AC_CHECK_LIB([rt], [clock_gettime])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h float.h limits.h memory.h netinet/in.h stddef.h stdint.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT16_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([clock_gettime floor inet_ntoa memmove memset socket sqrt strerror strtol strtoul])

# Final tasks
AC_CONFIG_FILES([Makefile
                 src/Makefile
		 src/common/Makefile
		 src/master/Makefile
		 src/slave/Makefile])
AC_OUTPUT
