#ifndef	lint
static	char	Id[] = "$Id: c_count.c,v 3.0 1990/05/21 13:33:11 ste_cm Rel $";
#endif	lint

/*
 * Title:	lincnt.c
 * Author:	T.E.Dickey
 * Created:	04 Dec 1985
 * $Log: c_count.c,v $
 * Revision 3.0  1990/05/21 13:33:11  ste_cm
 * BASELINE Mon Jun 11 15:15:17 1990 -- added options a la a.count
 *
 *		Revision 2.3  90/05/21  13:33:11  dickey
 *		corrected final (per-file) call on 'Summary()'
 *		
 *		Revision 2.2  90/05/21  12:14:07  dickey
 *		rewrote, adding options to make this behave like "a.count"
 *		
 *		Revision 2.1  90/05/14  16:36:55  dickey
 *		added "-t" (spreadsheet/table) option
 *		
 *		Revision 2.0  90/05/10  16:01:43  ste_cm
 *		BASELINE Fri May 11 10:36:36 1990
 *		
 *		Revision 1.19  90/05/10  16:01:43  dickey
 *		ported to VAX/VMS 5.3 (expand wildcards, added "-o" option).
 *		made usage-message more verbose
 *		
 *		Revision 1.18  89/10/17  11:03:46  dickey
 *		assume "//" begins C++ inline comments
 *		
 *		Revision 1.17  89/10/05  13:04:20  dickey
 *		lint (apollo SR10 "string" defs)
 *		
 *		Revision 1.16  89/07/21  10:51:16  dickey
 *		sccs2rcs keywords
 *		
 *		21 Jul 1989, permit use of "-" to indicate standard input.
 *		15 Aug 1988, use 'vecalloc()' rather than 'malloc()'
 *		01 Jun 1988, added token-length statistic
 *		01 Jul 1987, test for junky files (i.e., non-ascii characters,
 *			     nested comments, non-graphic characters in
 *			     quotes), added '-v' verbose mode to show running
 *			     totals & status per-file.  Added '-q' option to
 *			     handle the case of unbalanced '"' in a macro
 *			     (e.g., for a common printf-prefix).
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

#ifdef	vms
#define	DIR_PTYPES
#endif

#define	STR_PTYPES
#include	"ptypes.h"
#include	<ctype.h>
extern	int	optind;
extern	char	*optarg,
		**vecalloc();

#define	OCTAL	3		/* # of octal digits permissible in escape */
#define	DEBUG	if (debug) PRINTF
#define	TOKEN(c)	((c) == '_' || isalnum(c))

static	FILE	*File;
static	char	**quotvec;

static	int	literal;	/* true when permitting literal-tab */

typedef	struct	{
		long	chars_total,	/* # of characters */
			chars_blank,
			chars_code,
			chars_ignore,
			chars_notes,
			chars_prepro;
		long	lines_total,	/* # of lines */
			lines_blank,
			lines_code,
			lines_inline,	/* in-line comments */
			lines_notes,	/* all other comments */
			lines_prepro;
		long	flags_unquo,
			flags_uncmt,
			flags_unasc;
		long	stmts_total;
		long	words_total,	/* # of tokens */
			words_length;	/* total length of tokens */
	} STATS;

static	STATS	All, One;	/* total, per-file stats */

static	long	old_unquo,
		old_unasc,
		old_uncmt;

typedef	enum	PSTATE	{code, comment, preprocessor};
static	enum	PSTATE	pstate;

static	int	verbose	= FALSE,/* TRUE iff we echo file, as processed */
		quotdef	= 0,	/* number of tokens we treat as '"' */
		jargon	= FALSE,
		per_file= FALSE,
		debug	= FALSE,
		opt_line,
		opt_char,
		opt_name,
		opt_stat,
		spreadsheet = FALSE,
		newsum;		/* TRUE iff we need a summary */

static	char	*comma	= ",";
static	char	*dashes = "----------------";

/************************************************************************
 *	local procedures						*
 ************************************************************************/
#ifdef	sun
static	double	roundup(value,parts)
	double	value, parts;
	{
		long	temp = value * parts * 10.0;
		if ((temp % 10) == 5)	temp++;
		return (temp / (parts * 10.0));
	}
#else
#define	roundup(value,parts)	value
#endif

static
new_summary()
{
	if (!spreadsheet)
		PRINTF ("\n");
}

static
per_cent(text, num, den)
long	num, den;
char	*text;
{
	double	value;
	if (spreadsheet) {
		PRINTF("%d%s", num, comma);
		return;
	}
	if (num == 0 || den == 0)
		value = 0.0;
	else
		value = roundup((num * 100.0) / den, 10.0);
	PRINTF("%6ld\t%-24s %5.1f %%\n", num, text, value);
}

static
show_a_flag(text,flag)
char	*text;
long	flag;
{
	if (spreadsheet)
		PRINTF("%s%ld", comma, flag);
	else if (flag)
		PRINTF("%6ld\t%s\n", flag, text);
}

static
ratio(text, num, den)
char	*text;
long	num, den;
{
	if (den == 0) den = 1;
	if (spreadsheet) {
		PRINTF("%.2f%s", (float)(num) / den, comma);
		return;
	}
	PRINTF("%6.2f\tratio of %s\n", (float)(num) / den, text);
}

static
summarize_lines(p)
STATS	*p;
{
	auto	long	den = p->lines_total;

	new_summary();
	per_cent("lines had comments",
		p->lines_notes + p->lines_inline, den);
	/* patch
	if (!spreadsheet)
		PRINTF("%6d\tcomments are preprocessor-style\n", lines_c_meta);
	*/
	per_cent("comments are inline",	  p->lines_inline,	-den);
	per_cent("lines were blank",	  p->lines_blank,	den);
	per_cent("lines for preprocessor",p->lines_prepro,	den);
	per_cent("lines containing code", p->lines_code,	den);
	per_cent(jargon ?
		 "total lines (PSS)" :
		 "total lines",
		p->lines_notes + p->lines_blank +
		p->lines_prepro + p->lines_code,
		den);
}

static
summarize_chars(p)
STATS	*p;
{
	auto	long	den = p->chars_total;

	new_summary();
	per_cent("comment-chars",	  p->chars_notes,	den);
	per_cent("nontext-comment-chars", p->chars_ignore,	den);
	per_cent("whitespace-chars",	  p->chars_blank,	den);
	per_cent("preprocessor-chars",	  p->chars_prepro,	den);
	per_cent("statement-chars",	  p->chars_code,	den);
	per_cent("total characters",
		p->chars_blank + p->chars_notes + p->chars_ignore +
		p->chars_prepro + p->chars_code,
		den);
}

static
summarize_names(p)
STATS	*p;
{
	new_summary();
	if (spreadsheet) {
		PRINTF("%ld%s%ld%s",
			p->words_total, comma,
			p->words_length, comma);
		return;
	}
	if (p->words_total)
		PRINTF("%6ld\ttokens, average length %.2f\n",
			p->words_total,
			(1.0 * p->words_length) / p->words_total);
}

static
summarize_stats(p)
STATS	*p;
{
	new_summary();
	ratio("comment:code", p->chars_notes, p->chars_prepro + p->chars_code);
	show_a_flag("?:illegal characters found",	p->flags_unasc);
	show_a_flag("\":lines with unterminated quotes",p->flags_unquo);
	show_a_flag("*:unterminated/nested comments",	p->flags_uncmt);
}

static
show_totals(p)
STATS	*p;
{
	if (opt_line)	summarize_lines(p);
	if (opt_char)	summarize_chars(p);
	if (opt_name)	summarize_names(p);
	if (opt_stat)	summarize_stats(p);
}

static
summarize(p,mark)
STATS	*p;
{
	newsum = FALSE;
	if (spreadsheet) {
		PRINTF ("%ld%s%ld%s",
			p->lines_total,	comma,
			p->stmts_total,	comma);
	} else {
		PRINTF ("%6ld %5ld%c%c%c%c",
			p->lines_total,
			p->stmts_total,
			(p->flags_unasc != old_unasc ? '?' : ' '),
			(p->flags_unquo != old_unquo ? '"' : ' '),
			(p->flags_uncmt != old_uncmt ? '*' : ' '),
			(mark  ? '|' : ' '));
	}
	old_unasc = p->flags_unasc;
	old_uncmt = p->flags_uncmt;
	old_unquo = p->flags_unquo;
}

static
Summary(mark)
{
	if (newsum)
		summarize(&One,mark);
}

#define	ADD(m)	All.m += One.m; One.m = 0

static
add_totals()
{
	ADD(chars_total);
	ADD(chars_blank);
	ADD(chars_code);
	ADD(chars_ignore);
	ADD(chars_notes);
	ADD(chars_prepro);

	ADD(lines_total);
	ADD(lines_blank);
	ADD(lines_code);
	ADD(lines_inline);
	ADD(lines_notes);
	ADD(lines_prepro);

	ADD(flags_unquo);
	ADD(flags_uncmt);
	ADD(flags_unasc);

	ADD(stmts_total);

	ADD(words_total);
	ADD(words_length);
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
static
Token(c)
{
static	char	bfr[80];
static	int	len = 0;
register int	j = 0;

	if (quotdef) {
		One.words_total++;
		while (TOKEN(c)) {
			One.words_length++;
			if (len < sizeof(bfr)-1) bfr[len++] = c;
			c = inFile();
			j++;
		}
		if (len) {
			if (c == ' ' || c == '\t' || c == '(') {
				bfr[len] = EOS;
				for (j = 0; j < quotdef; j++) {
					if (!strcmp(quotvec[j],bfr)) {
						c = String('"');
						DEBUG("**%c**",c);
						break;
					}
				}
				DEBUG("%s\n", bfr);
			}
			len = 0;
		} else if (!j)
			c = inFile();
		bfr[len] = 0;
	} else {
		if (TOKEN(c)) {
			One.words_total++;
			DEBUG("'");
			do {
				One.words_length++;
				DEBUG("%c", c);
				c = inFile();
			} while (TOKEN(c));
			DEBUG("'\n");
		} else	/* punctuation */
			c = inFile();
	}
	return(c);
}

/*
 * Process a single file:
 */
static
doFile (name)
char	*name;
{
register int c;

#ifdef	vms
	if (vms_iswild(name)) {		/* expand wildcards */
		auto	DIR		*dirp;
		auto	struct	direct	*dp;
		auto	char		pattern[BUFSIZ];

		name = strcpy(pattern, name);
		if (!strchr(name, ';'))
			(void)strcat(name, ";");

		if (dirp = opendir(name)) {
			while (dp = readdir(dirp))
				doFile(dp->d_name);
			closedir(dirp);
		} else {
			perror(name);
			exit(FAIL);
		}
		return;
	}
#endif

	pstate = code;
	if (!strcmp(name, "-"))
		File = stdin;
	else if (!(File = fopen (name, "r")))
		return;

	newsum = TRUE;
	c = inFile ();

	while (c != EOF) {
		switch (c) {
		case '/':
			c = Token(EOS);
			if (c == '*')
				c = Comment(FALSE);
			else if (c == '/')
				c = Comment(TRUE);
			break;
		case '"':
		case '\'':
			c = String(c);
			break;
		case ';':
			One.stmts_total++;
		default:
			c = Token(c);
		}
	}
	(void)Token(EOS);
	(void)fclose(File);

	if (per_file && spreadsheet) {
		show_totals(&One);
	} else if (!per_file)
		summarize(&One,TRUE);
	PRINTF("%s\n", name);
	if (per_file && !spreadsheet) {
		show_totals(&One);
		PRINTF("\n");
	}
	add_totals();
}

/*
 * Scan over a quoted string.  Except for the special (automatic) case of
 * "@(#)"-sccs strings, flag all nonprinting characters which are found in
 * the string in 'unasc'.  Also, flag places where we have a newline in a
 * string (perhaps because the leading quote was in a macro!).
 */
static
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
					One.flags_unasc++;
			} else if (c != *p++)
				p = 0;
		}
		if ((p == 0) && !isprint(c))
			One.flags_unasc++; /* this may duplicate 'unquo'! */
		if (c == '\n') {	/* this is legal, but not likely */
			One.flags_unquo++; /* ...assume balance is in macro */
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
static
int
Escape ()
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
 * Entered immediately after reading '/','*', scan over a comment, returning
 * the first character after the comment.  Flags both unterminated comments
 * and nested comments with 'uncmt'.
 */
static
int
Comment (c_plus_plus)
{
	register int	c;
	auto	int	d = 0;
	auto	enum	PSTATE	save_st = pstate;

	if (pstate == code)
		One.chars_code -= 2;
	else
		One.chars_prepro -= 2;
	One.chars_ignore += 2;		/* ignore the comment-delimiter */

	pstate = comment;
	c = inFile();
	while (c != EOF) {
		if (c_plus_plus) {
			if (c == '\n') {
				pstate = save_st;
				return(c);
			}
			c = inFile();
		} else {
			if (c == '*') {
				c = inFile();
				if (c == '/') {
					pstate = save_st;
					return (inFile());
				}
			} else {
				c = inFile();
				if (c == '*' && d == '/')
					One.flags_uncmt++;
			}
		}
		d = c;
	}
	One.flags_uncmt++;
	return (c);			/* Unterminated comment! */
}

/*
 * Return the next character from the current file, using the global file
 * pointer.
 */
static
int
inFile ()
{
	static	int	last_c;
	static	int	is_blank;	/* true til we get nonblank on line */
	static	int	had_note,
			had_code;
register int c = fgetc(File);

	if (One.chars_total == 0) {
		old_unquo =
		old_unasc =
		old_uncmt = 0;
		had_note  =
		had_code  = FALSE;
		last_c    = EOS;
		is_blank  = TRUE;
	}

	if (feof(File) || ferror(File))
		c = EOF;
	else
		c &= 0xff;		/* protect against sign-extension bug */
	if (c != EOF) {
		if (verbose && (!One.chars_total || last_c == '\n'))
			Summary(TRUE);
		newsum = TRUE;
		if (!isascii(c) || (!isprint(c) && !isspace(c))) {
			c = '?';		/* protect/flag this */
			One.flags_unasc++;
		}
		if (verbose) {
#ifdef	putc
			c = putchar((unsigned char)c);
#else
			c = putchar(c);
#endif
		}
		One.chars_total++;
		if (c == '#' && is_blank)
			pstate = preprocessor;
		else if (c == '\n') {
			One.lines_total++;
			if (is_blank)
				One.lines_blank++;
			else {
				if (pstate == preprocessor) {
					One.lines_prepro++;
					if (had_note)
						One.lines_inline++;
				} else if (had_code) {
					One.lines_code++;
					if (had_note)
						One.lines_inline++;
				} else if (had_note)
					One.lines_notes++;
				had_code =
				had_note = FALSE;
			}
			is_blank = TRUE;
			if (pstate == preprocessor && last_c != '\\')
				pstate = code;
		}
		if (isspace(c)) {
			if (literal)
				One.chars_code++;
			else
				One.chars_blank++;
		} else {
			is_blank = FALSE;
			switch (pstate) {
			case comment:
				had_note = TRUE;
				if (isalnum(c))
					One.chars_notes++;
				else
					One.chars_ignore++;
				break;
			case preprocessor:
				One.chars_prepro++;
				break;
			default:
				had_code = TRUE;
				One.chars_code++;
			}
		}
	}
	last_c = c;
	return (c);
}


usage()
{
	static	char	*tbl[] = {
 "Usage: lincnt [options] [files]"
,""
,"If no files are specified as arguments, a list of filenames is read from the"
,"standard input.  The special name \"-\" denotes a file which is read from the"
,"standard input."
,""
,"Options:"
," -c        character-statistics"
," -d        debug (shows tokens as they are parsed)"
," -i        identifier-statistics"
," -j        annotate summary in technical format"
," -l        line-statistics"
," -o file   specify alternative output-file"
," -p        per-file statistics"
," -q DEFINE tells lincnt that the given name is an unbalanced quote"
," -s        specialized statistics"
," -t        generate output for spreadsheet"
," -v        verbose (shows lines as they are counted)"
	};
	register int	j;
	for (j = 0; j < sizeof(tbl)/sizeof(tbl[0]); j++)
		FPRINTF(stderr, "%s\n", tbl[j]);
	(void)exit(FAIL);
}

/************************************************************************
 *	main procedure							*
 ************************************************************************/

main (argc, argv)
int	argc;
char	*argv[];
{
	register int j;
	auto	char	name[BUFSIZ];
	auto	int	opt_all = -1;

	quotvec = vecalloc((unsigned)(argc * sizeof(char *)));
	while ((j = getopt(argc,argv,"cdijlo:pq:stv")) != EOF) switch(j) {
	case 'l':	opt_all = FALSE; opt_line = TRUE; break;
	case 'c':	opt_all = FALSE; opt_char = TRUE; break;
	case 'i':	opt_all = FALSE; opt_name = TRUE; break;
	case 's':	opt_all = FALSE; opt_stat = TRUE; break;

	case 'd':	debug	= TRUE;	break;
	case 'j':	jargon	= TRUE;	break;
	case 'p':	per_file= TRUE;	break;
	case 'v':	verbose = TRUE;	break;
	case 'o':	if (!freopen(optarg, "w", stdout))
				usage();
			break;
	case 'q':	quotvec[quotdef++] = optarg;
			break;
	case 't':	spreadsheet = TRUE;	break;
	default:	usage();
	}

	if (opt_all == -1)
		opt_line = opt_char = opt_name = opt_stat = TRUE;

	if (spreadsheet) {
		if (per_file) {
			if (opt_line)
				PRINTF("%s%s%s%s%s%s%s%s%s%s%s%s",
					"L-COMMENT",	comma,
					"L-INLINE",	comma,
					"L-BLANK",	comma,
					"L-CPP",	comma,
					"L-CODE",	comma,
					"L-TOTAL",	comma);
			if (opt_char)
				PRINTF("%s%s%s%s%s%s%s%s%s%s%s%s",
					"C-COMMENT",	comma,
					"C-IGNORE",	comma,
					"C-BLANK",	comma,
					"C-CPP",	comma,
					"C-CODE",	comma,
					"C-TOTAL",	comma);
			if (opt_name)
				PRINTF("%s%s%s%s",
					"W-TOTAL",	comma,
					"W-LENGTH",	comma);
			if (opt_stat)
				PRINTF("%s%s%s%s%s%s%s%s",
					"CODE:COMMENT",	comma,
					"ILLEGAL-CHARS", comma,
					"ILLEGAL-QUOTES", comma,
					"ILLEGAL-COMMENTS", comma);

		} else
			PRINTF("STATEMENTS%sLINES%s", comma, comma);
		PRINTF("FILENAME\n");
	}

	if (optind < argc) {
		for (j = optind; j < argc; j++)
			doFile (argv[j]);
	}
	else while (gets(name))
		doFile (name);

	if (!spreadsheet && All.chars_total) {
		if (!per_file) {
			PRINTF ("%s\n", dashes);
			summarize(&All,FALSE);
			PRINTF("%s\n",
				jargon ?
				"physical source statements/logical source statements" :
				"total lines/statements");
		} else {
			PRINTF("Grand total\n");
			PRINTF ("%s\n", dashes);
		}
		show_totals(&All);
		PRINTF("\n");
	}
	(void)exit(SUCCESS);
	/*NOTREACHED*/
}
