/* $Id: system.h,v 7.4 1995/05/14 23:50:36 tom Exp $ */

#ifdef HAVE_CONFIG_H

# include "config.h"
# define SYS_UNIX 1

#else
	/* provide values for non-UNIX systems */
# if defined(vms)
#  define SYS_VMS 1
# endif

#endif /* HAVE_CONFIG_H */

#ifndef ANSI_PROTOS
#define ANSI_PROTOS 1
#endif

#ifndef HAVE_GETOPT_H
#define HAVE_GETOPT_H 1
#endif

#ifndef HAVE_MALLOC_H
#define HAVE_MALLOC_H 0
#endif

#ifndef HAVE_STDLIB_H
#define HAVE_STDLIB_H 1
#endif

#ifndef HAVE_STRING_H
#define HAVE_STRING_H 1
#endif

#ifndef SYS_MSDOS
#define SYS_MSDOS 0
#endif

#ifndef SYS_UNIX
#define SYS_UNIX 0
#endif

#ifndef SYS_VMS
#define SYS_VMS 0
#endif

	/* Macros that ought to be defined on every system */
#if ANSI_PROTOS
#define ARGS(p) p
#define _ARG(type,name) type name
#define _DCL(type,name)
#define NO_ARGS void
#else
#define ARGS(p) ()
#define _ARG(type,name) name
#define _DCL(type,name) type name;
#define NO_ARGS
#endif

#ifdef lint
#define typeCalloc(type,elts) (type *)(elts)
#else
#define typeCalloc(type,elts) (type *)calloc(elts,sizeof(type))
#endif

#ifndef TRUE
#define TRUE  (1)
#define FALSE (0)
#endif

#undef EOS
#define EOS	'\0'
