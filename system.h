/* $Id: system.h,v 7.1 1995/05/13 23:57:05 tom Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
	/* provide values for non-UNIX systems */
#endif

#ifndef ANSI_PROTOS
#define ANSI_PROTOS 1
#endif

#if ANSI_PROTOS
#define ARGS(p) p
#else
#define ARGS(p) ()
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
