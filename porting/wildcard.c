#ifndef NO_IDENT
static char *Id = "$Id: wildcard.c,v 1.2 1995/05/15 00:30:06 tom Exp $";
#endif

/*
 * wildcard.c - perform wildcard expansion for non-UNIX configurations
 */

#include "system.h"

#if SYS_VMS

#include <stdlib.h>

#include <starlet.h>		/* DEC-C (e.g., sys$parse) */
#include <stdio.h>		/* perror */

#include <rms.h>
#include <descrip.h>
/*#include <unixio.h>*/
#include <string.h>

int	has_wildcard (char *path)
{
	return (strstr(path, "...") != 0
	   ||   strchr(path, '*') != 0
	   ||   strchr(path, '?') != 0);
}

int	expand_wildcard (char *path, int initiate)
{
	static	struct	FAB	zfab;
	static	struct	NAM	znam;
	static	char	my_esa[NAM$C_MAXRSS];	/* expanded: SYS$PARSE */
	static	char	my_rsa[NAM$C_MAXRSS];	/* expanded: SYS$SEARCH */

	if (initiate) {
		zfab = cc$rms_fab;
		zfab.fab$l_fop = FAB$M_NAM;
		zfab.fab$l_nam = &znam;		/* FAB => NAM block	*/
		zfab.fab$l_dna = "*.*;";	/* Default-selection	*/
		zfab.fab$b_dns = strlen(zfab.fab$l_dna);

		zfab.fab$l_fna = path;
		zfab.fab$b_fns = strlen(path);

		znam = cc$rms_nam;
		znam.nam$b_ess = sizeof(my_esa);
		znam.nam$l_esa = my_esa;
		znam.nam$b_rss = sizeof(my_rsa);
		znam.nam$l_rsa = my_rsa;

		if (sys$parse(&zfab) != RMS$_NORMAL) {
			perror(path);
			exit(EXIT_FAILURE);
		}
	}
	if (sys$search(&zfab) == RMS$_NORMAL) {
		strncpy(path, my_rsa, znam.nam$b_rsl)[znam.nam$b_rsl] = '\0';
		return (TRUE);
	}
	return FALSE;
}

#endif	/* VMS */
