#ifndef	lint
static	char	Id[] = "@(#)lincnt.c	1.3 86/10/07 13:35:08";
#endif	lint

/*
 * Title:	lincnt.c
 * Author:	T.E.Dickey
 * Created:	04 Dec 1985
 * Modified:	23 Apr 1986, treat standard-input as a list of names, not code.
 *		28 Jan 1985, make final 'exit()' with return code.
 *
 * Function:	Count lines and statements in one or more C program files,
 *		giving statistics on percentage commented.
 *
 *		The file(s) are specified as command line arguments.  If no
 *		arguments are given, then the file is read from standard input.
 */

#include	<stdio.h>
#include	<ctype.h>
extern	void	exit();

#define	OCTAL	3		/* # of octal digits permissible in escape */
#define	TRUE	1
#define	FALSE	0
#define	PRINTF	(void) printf
#define	PERCENT(n,s) PRINTF ("%.1f%% s", (100.0*n)/tot_chars);

static	FILE	*File;

static	int	literal;
				/* Running totals: */
static	long	tot_chars = 0,	/* # of characters */
		tot_lines = 0,	/* # of lines	   */
		tot_stmts = 0,	/* # of statements */
		tot_white = 0,	/* Whitespace size */
		tot_notes = 0;	/* Comment-length  */

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
		PRINTF ("%8d characters", tot_chars);
		if (tot_white || tot_notes) {
			PRINTF (" (");
			if (tot_white)	PERCENT(tot_white,whitespace);
			if (tot_white && tot_notes) PRINTF(", ");
			if (tot_notes)	PERCENT(tot_notes,comment);
			PRINTF(")");
			if (tot_stmts && tot_notes)
				PRINTF (": %.2f", (tot_notes*1.)
					/(tot_chars-tot_white-tot_notes));
		}
		PRINTF("\n");
		PRINTF ("%8d lines\n", tot_lines);
		PRINTF ("%8d statements\n", tot_stmts);
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
	PRINTF ("%8d %8d %s\n", tot_lines-num_lines, tot_stmts-num_stmts, name);
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
		if (c == mark) {
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
