#ifndef	lint
static	char	Id[] = "@(#)lincnt.c	1.8 87/07/28 08:26:57";
#endif	lint

/*
 * Title:	lincnt.c
 * Author:	T.E.Dickey
 * Created:	04 Dec 1985
 * Modified:
 *		01 Jul 1987, test for junky files (i.e., non-ascii characters,
 *			     nested comments, non-graphic characters in quotes),
 *			     added '-v' verbose mode to show running totals &
 *			     status per-file.  Added '-q' option to handle the
 *			     case of unbalanced '"' in a macro (e.g., for a
 *			     common printf-prefix).
 *		07 Nov 1986, added tested for unbalanced quote, comment so we
 *			     can get reasonable counts for ddn-driver, etc.
 *			     Also fixed printf-formats for xenix-port.
 *		23 Apr 1986, treat standard-input as a list of names, not code.
 *		28 Jan 1985, make final 'exit()' with return code.
 *
 * Function:	Count lines and statements in one or more C program files,
 *		giving statistics on percentage commented.
 *
 *		The file(s) are specified as command line arguments.  If no
 *		arguments are given, then the file is read from standard input.
 */

#include	<ptypes.h>

#include	<stdio.h>
#include	<ctype.h>
extern	int	optind;
extern	char	*optarg, *malloc();

#ifdef	SYS3_LLIB
extern	int	exit();
#else
extern	void	exit();
#endif

#define	OCTAL	3		/* # of octal digits permissible in escape */
#define	TRUE	1
#define	FALSE	0
#define	PRINTF	(void) printf
#define	PERCENT(n,s) PRINTF ("%.1f%% s", (100.0*((double)n))/((double)tot_chars));

static	FILE	*File;
static	char	**quotvec;

static	int	literal;
				/* Running totals: */
static	long	tot_chars = 0,	/* # of characters */
		tot_lines = 0,	/* # of lines	   */
		tot_stmts = 0,	/* # of statements */
		tot_unasc = 0,	/* # of illegal (nongraphic) characters */
		tot_unquo = 0,	/* # of unbalanced quotes (line) */
		tot_uncmt = 0,	/* # of unbalanced comments (file) */
		tot_white = 0,	/* Whitespace size */
		tot_notes = 0;	/* Comment-length  */

static	long	num_unquo, unquo,/* per-file, for running totals */
		num_uncmt, uncmt,
		num_unasc, unasc,
		num_chars,
		num_lines,
		num_stmts;

static	int	verbose	= FALSE,/* TRUE iff we echo file, as processed */
		quotdef	= 0,	/* number of tokens we treat as '"' */
		debug	= FALSE,
		newsum;		/* TRUE iff we need a summary */

main (argc, argv)
int	argc;
char	*argv[];
{
register int j;
char	name[256];

	quotvec = (char **)malloc(argc * sizeof(char *));
	while ((j = getopt(argc,argv,"dvq:")) != EOF) switch(j) {
	case 'd':	debug	= TRUE;
			break;
	case 'v':	verbose = TRUE;
			break;
	case 'q':	quotvec[quotdef++] = optarg;
			break;
	default:	usage();
	}

	if (optind < argc) {
		for (j = optind; j < argc; j++)
			doFile (argv[j]);
	}
	else while (gets(name))
		doFile (name);

	if (tot_chars) {
		PRINTF ("--------\n");
		PRINTF ("%8ld characters", tot_chars);
		if ((tot_white != 0L) || (tot_notes != 0L)) {
		long	bot_ratio = (tot_chars - tot_white - tot_notes);
			PRINTF (" (");
			if (tot_white)	PERCENT(tot_white,whitespace);
			if (tot_white && tot_notes) PRINTF(", ");
			if (tot_notes)	PERCENT(tot_notes,comment);
			PRINTF(")");
			if (tot_stmts && bot_ratio) {
				PRINTF (": %.2f", ((float)tot_notes)/bot_ratio);
			}
		}
		PRINTF("\n");
		PRINTF ("%8ld lines\n", tot_lines);
		PRINTF ("%8ld statements\n", tot_stmts);
		if (tot_unasc)
			PRINTF("%8ld ?:illegal characters found\n", tot_unasc);
		if (tot_unquo)
			PRINTF("%8ld \":lines with unterminated quotes\n", tot_unquo);
		if (tot_uncmt)
			PRINTF("%8ld *:unterminated/nested comments\n", tot_uncmt);
	}
	return(0);
}

usage()
{
	PRINTF("usage: lincnt [-v] [-qDEFINE] [files]\n");
	(void)exit(0);
}

/*
 * Process a single file:
 */
doFile (name)
char	*name;
{
register int c;

	num_chars = tot_chars;
	num_lines = tot_lines;
	num_stmts = tot_stmts;

	if (!(File = fopen (name, "r")))	 return;

	newsum = TRUE;
	num_unasc = unasc = 0;
	num_uncmt = uncmt = 0;
	num_unquo = unquo = 0;
	c = inFile ();
	while (c != EOF) {
		switch (c) {
		case '/':
			c = Token(0);
			if (c == '*') c = Comment();
			break;
		case '"':
		case '\'':
			c = String(c);
			break;
		case ';':
			tot_stmts++;
		default:
			c = Token(c);
		}
	}
	(void)Token(0);
	if (verbose) {
		unquo = num_unquo;
		uncmt = num_uncmt;
		unasc = num_unasc;
	}
	Summary();
	PRINTF("%s\n", name);
	tot_unquo += num_unquo;
	tot_uncmt += num_uncmt;
	tot_unasc += num_unasc;
	(void) fclose (File);
}

/*
 * Summarize, line-by-line, or at the end of a file
 */
Summary()
{
	if (newsum) {
		newsum = FALSE;
		PRINTF ("%6ld %5ld%c%c%c|",
			tot_lines-num_lines, tot_stmts-num_stmts,
			(unasc ? '?' : ' '),
			(unquo ? '"' : ' '),
			(uncmt ? '*' : ' '));
		num_unasc += unasc; unasc = 0;
		num_unquo += unquo; unquo = 0;
		num_uncmt += uncmt; uncmt = 0;
	}
}

/*
 * If '-q' option is in effect, append to the token-buffer until a token is
 * complete.  Test the completed token to see if it matches any of the strings
 * we have equated to '"'.  If so, process a string from that point.  Note that
 * there are two special cases which fortuitously work out:
 *	(a) in the "#define" statement, the whitespace-character immediately
 *	    after the token is ignored.
 *	(b) in the usage of the token, the whitespace-character following the
 *	    token is ignored.
 * To provide for the special case of one define providing an alternate name
 * for another, we do the lookup/string processing only if a quote-macro could
 * be expanded there, e.g., if it is followed by a space or tab.
 */
Token(c)
{
static	char	bfr[80];
static	int	len = 0;
register int	j = 0;

	if (quotdef) {
		while (isalnum(c) || c == '_') {
			if (len < sizeof(bfr)-1) bfr[len++] = c;
			c = inFile();
			j++;
		}
		if (len) {
			if (c == ' ' || c == '\t' || c == '(') {
				bfr[len] = 0;
				for (j = 0; j < quotdef; j++) {
					if (!strcmp(quotvec[j],bfr)) {
						c = String('"');
						if (debug) PRINTF("**%c**",c);
						break;
					}
				}
				if (debug) PRINTF("%s\n", bfr);
			}
			len = 0;
		} else if (!j)
			c = inFile();
		bfr[len] = 0;
	} else
		c = inFile();
	return(c);
}

/*
 * Scan over a quoted string.  Except for the special (automatic) case of
 * "@(#)"-sccs strings, flag all nonprinting characters which are found in
 * the string in 'unasc'.  Also, flag places where we have a newline in a
 * string (perhaps because the leading quote was in a macro!).
 */
String (mark)
char	mark;
{
register int c = inFile();
char	*p = "@(#)";			/* permit literal tab here only! */

	literal = TRUE;
	while (c != EOF) {
		if (p != 0) {
			if (*p == '\0') {
				if (!isprint(c) && (c != '\t'))
					unasc++;
			} else if (c != *p++)
				p = 0;
		}
		if ((p == 0) && !isprint(c))
			unasc++;	/* this may duplicate 'unquo'! */
		if (c == '\n') {	/* this is legal, but not likely */
			unquo++;	/* ...assume balance is in macro */
			return (inFile());
		} else if (c == mark) {
			literal = FALSE;
			return (inFile());
		}
		else if (c == '\\')	c = Escape();
		else			c = inFile();
	}
	literal = FALSE;
	return (c);
}

/*
 * Scan over an '\' escape sequence, returning the first character after it.
 * If we start with an octal digit, we may read up to OCTAL of these in a row.
 */
int	Escape ()
{
register int c = inFile(),
	digits = 0;

	if (c != EOF) {
		while (c >= '0' && c <= '7' && digits < OCTAL) {
			digits++;
			if ((c = inFile()) == EOF)	return (c);
		}
		if (!digits) c = inFile();
	}
	return (c);
}

/*
 * Entered immediately after reading '/','*', scan over a comment, returning the
 * first character after the comment.  Flags both unterminated comments and
 * nested comments with 'uncmt'.
 */
int	Comment ()
{
register int c = inFile();
int	d = 0;

	tot_white += 3;			/* count "/,*,*,/" as whitespace */
	while (c != EOF) {
		if (isalnum(c))		tot_notes++;
		else if (!isspace(c))	tot_white++;
		if (c == '*') {
			c = inFile();
			if (c == '/')	return (inFile());
		} else {
			c = inFile();
			if (c == '*' && d == '/')	uncmt++;
		}
		d = c;
	}
	uncmt++;
	return (c);			/* Unterminated comment! */
}

/*
 * Return the next character from the current file, using the global file
 * pointer.
 */
int	inFile ()
{
register int c = fgetc(File);

	if (feof(File) || ferror(File))
		c = EOF;
	else
		c &= 0xff;		/* protect against sign-extension bug */
	if (c != EOF) {
		newsum = TRUE;
		if (!isascii(c) || (!isprint(c) && !isspace(c))) {
			c = '?';		/* protect/flag this */
			unasc++;
		}
		if (verbose) {
			if (num_chars == tot_chars)	Summary();
			putchar(c);
			newsum = TRUE;
		}
		tot_chars++;
		if (c == '\n') {
			tot_lines++;
			if (verbose) Summary();
		}
		if (isspace(c) && !literal)	tot_white++;
	}
	return (c);
}
