#ifndef	lint
static	char	Id[] = "@(#)lincnt.c	1.5 86/11/07 07:54:46";
#endif	lint

/*
 * Title:	lincnt.c
 * Author:	T.E.Dickey
 * Created:	04 Dec 1985
 * Modified:
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

#include	<syscap.h>

#include	<stdio.h>
#include	<ctype.h>

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

static	int	literal;
				/* Running totals: */
static	long	tot_chars = 0,	/* # of characters */
		tot_lines = 0,	/* # of lines	   */
		tot_stmts = 0,	/* # of statements */
		tot_unquo = 0,	/* # of unbalanced quotes (line) */
		tot_uncmt = 0,	/* # of unbalanced comments (file) */
		tot_white = 0,	/* Whitespace size */
		tot_notes = 0;	/* Comment-length  */

static	long	unquo, uncmt;	/* per-file, for running totals */

main (argc, argv)
int	argc;
char	*argv[];
{
register int j;
char	name[256];

	if (argc > 1) {
		for (j = 1; j < argc; j++)
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
		if (tot_unquo)
			PRINTF("%8ld lines with unterminated quotes\n", tot_unquo);
		if (tot_uncmt)
			PRINTF("%8ld files with unterminated comments\n", tot_uncmt);
	}
	return(0);
}

/*
 * Process a single file:
 */
doFile (name)
char	*name;
{
register int c;
register long
	num_lines = tot_lines,
	num_stmts = tot_stmts;

	if (!(File = fopen (name, "r")))	 return;

	uncmt = unquo = 0;
	c = inFile ();
	while (c != EOF) {
		switch (c) {
		case '/':
			c = inFile();
			if (c == '*') c = Comment();
			break;
		case '"':
		case '\'':
			c = String(c);
			break;
		case ';':
			tot_stmts++;
		default:
			c = inFile();
		}
	}
	PRINTF ("%8ld %8ld%c %s\n",
		tot_lines-num_lines, tot_stmts-num_stmts,
		((unquo || uncmt) ? '?' : ' '), name);
	tot_unquo += unquo;
	tot_uncmt += uncmt;
	(void) fclose (File);
}

/*
 * Scan over a quoted string.
 */
String (mark)
char	mark;
{
register int c = inFile();

	literal = TRUE;
	while (c != EOF) {
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
 * Entered immediately after reading "/*", scan over a comment, returning the
 * first character after the comment.
 */
int	Comment ()
{
register int c = inFile();

	tot_white += 3;			/* count "/,*,*,/" as whitespace */
	while (c != EOF) {
		if (isalnum(c))		tot_notes++;
		else if (!isspace(c))	tot_white++;
		if (c == '*') {
			c = inFile();
			if (c == '/')	return (inFile());
		}
		else
			c = inFile();
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

	if (c != EOF) {
		tot_chars++;
		if (c == '\n') tot_lines++;
		if (isspace(c) && !literal)	tot_white++;
	}
	return (c);
}
