/*
 * Title:	c_count.c
 * Author:	T.E.Dickey
 * Created:	04 Dec 1985
 * Modified:
 *		22 Nov 2002, Convert to ANSI C, indent'd
 *		27 Feb 2001, expand wildcards on WIN32 with _setargv().
 *			     Handle ^M^J line-endings for MSDOS, etc.
 *		11 Jan 1999, add check for files w/o trailing newline.
 *		02 Jul 1998, add -w option, to set threshold for too-long
 *			     identifiers.  Implement check for mismatched
 *			     braces.
 *		08 Oct 1997, integrate -b option better, showing block and
 *			     level number if -v option is repeated.
 *		04 Oct 1997, add top-level block and blocklevel counts.
 *		25 Apr 1997, correct missing transition in comment-parsing
 *			     state that caused incorrect line-count.  Modify
 *			     display of line/statement #'s to be more readable.
 *		18 Dec 1996, allow '$' in tokens.  Correct handling of C++
 *			     comments (were always treated as inline because
 *			     of incorrect state processing in inFile()).
 *		13 May 1995, split-off from td_lib.
 *		28 Jul 1994, show totals even for empty file.
 *		17 Jul 1994, renamed from 'lincnt', for clearer meaning.
 *		23 Sep 1993, gcc warnings
 *		16 Oct 1991, header-label for spreadsheet had "STATEMENTS" and
 *			     "LINES" interchanged (fixed).  Also, converted to
 *			     ANSI.
 *		23 May 1991, apollo sr10.3 cpp complains about endif-tags
 *		30 Aug 1990, history could be RCS or CMS (corrected message)
 *		30 Aug 1990, added 'filter_history()' procedure, which filters
 *			     out the history-comments generated by RCS or
 *			     DEC/CMS from the total for "normal" comments.  Only
 *			     "normal" comments are shown in the comment:code
 *			     ratio.  If specific statistics are requested (e.g.,
 *			     "-s") and the "-t" option is set, coerce "-p" as
 *			     well.
 *		29 Aug 1990, corrected format of 'show_flag()'
 *		21 May 1990, corrected final (per-file) call on 'Summary()'
 *		05 May 1990, rewrote, adding options to make this behave like
 *			     "a.count".
 *		14 May 1990, added "-t" (spreadsheet/table) option
 *		10 May 1990, ported to VAX/VMS 5.3 (expand wildcards, added "-o"
 *			     option).  Made usage-message more verbose
 *		17 Oct 1989, assume "//" begins C++ inline comments
 *		05 Oct 1989, lint (apollo SR10 "string" defs)
 *		21 Jul 1989, permit use of "-" to indicate standard input.
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
 *
 * TODO:	Make RCS-history filtering work with C++ comments.
 *
 * TODO:	Make RCS-history filtering smart enough to handle blank lines
 *		in the history-comments (as opposed to blank lines between
 *		successive revisions).
 */

#include "system.h"

#ifndef	NO_IDENT
static const char Id[] = "$Header: /users/source/archives/c_count.vcs/RCS/c_count.c,v 7.31 2002/11/22 23:55:49 tom Exp $";
#endif

#include <stdio.h>
#include <ctype.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if HAVE_MALLOC_H
#include <malloc.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#if HAVE_GETOPT_H
#include <getopt.h>
#else
# if !HAVE_GETOPT_HEADER
extern int getopt(int argc, char **argv, char *opts);
extern int optind;
extern char *optarg;
# endif
#endif

#if !HAVE_STRCHR		/* normally in <string.h> */
#define strchr index
#endif

#ifndef EXIT_SUCCESS		/* normally in <stdlib.h> */
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 0
#endif

#if defined(SYS_MSDOS) || defined(WIN32)
#define FOPEN_MODE "rb"
#else
#define FOPEN_MODE "r"
#endif

#define	OCTAL	3		/* # of octal digits permissible in escape */

#define PRINTF  (void)printf
#define	DEBUG	if (debug) PRINTF
#define	TOKEN(c)	((c) == '_' || (c) == '$' || isalnum(c))
#define LVL_WEIGHT(n) if (opt_blok) One.lvl_weights += (n) * (One.nesting_lvl+1)

static int inFile(void);
static int Comment(int cpp);
static int Escape(void);
static int String(int mark);

static FILE *File;
static char **quotvec;

static int literal;		/* true when permitting literal-tab */

typedef struct {
    /* character-counts */
    long chars_total;		/* # of characters */
    long chars_blank;
    long chars_code;
    long chars_ignore;
    long chars_notes;
    long chars_rlogs;
    long chars_prepro;
    /* line-counts */
    long lines_total;		/* # of lines */
    long lines_blank;
    long lines_code;
    long lines_inline;		/* in-line comments */
    long lines_notes;		/* all other comments */
    long lines_rlogs;		/* RCS history comments */
    long lines_prepro;
    /* flags */
    long flags_unquo;
    long flags_uncmt;
    long flags_unasc;
    long flags_unlvl;
    long flags_2long;
    /* statement-counts */
    long stmts_total;
    long stmts_latch;
    /* miscellaneous */
    long nesting_lvl;		/* {} nesting level */
    long top_lvl_blk;		/* top-level blocks */
    long max_blk_lvl;		/* maximum {} level */
    long lvl_weights;		/* count(code * (level+1)) */
    long words_total;		/* # of tokens */
    long words_length;		/* total length of tokens */
} STATS;

static STATS All, One;		/* total, per-file stats */

static long old_block;
static long old_level;
static long old_stmts;
static long old_unquo;
static long old_unasc;
static long old_2long;
static long old_unlvl;
static long old_uncmt;

enum PSTATE {
    code,
    comment,
    preprocessor
};
static enum PSTATE pstate;

static int verbose = FALSE;	/* TRUE iff we echo file, as processed */
static int quotdef = 0;		/* number of tokens we treat as '"' */
static int jargon = FALSE;
static int per_file = FALSE;
static int debug = FALSE;
static int opt_all = -1;
static int opt_blok;
static int opt_line;
static int opt_char;
static int opt_name;
static int opt_stat;
static int spreadsheet = FALSE;
static int cms_history;
static int files_total;
static int limit_name = 32;
static int read_last;
static int wrote_last;
static int newsum;		/* TRUE iff we need a summary */

static char *comma = ",";
static char *dashes = "----------------";

/************************************************************************
 *	local procedures						*
 ************************************************************************/
#if PRINT_ROUNDS_DOWN
/*
 * When I compared output on Apollo SR10.1 with SunOS 4.1.1, I found that
 * sometimes the SunOS printf would round down (e.g., 0.05% would be rendered
 * as 0.0%).  This code is used to round up so that I'd get the same numbers on
 * different machines.
 */
static double
RoundUp(double value, double parts)
{
    long temp = value * parts * 10.0;
    if ((temp % 10) == 5)
	temp++;
    return (temp / (parts * 10.0));
}
#else
#define	RoundUp(value,parts)	value
#endif

static void
new_summary(void)
{
    if (!spreadsheet)
	PRINTF("\n");
}

static void
per_cent(char *text, long num, long den)
{
    double value;
    if (spreadsheet) {
	PRINTF("%ld%s", num, comma);
	return;
    }
    if (num == 0 || den == 0)
	value = 0.0;
    else
	value = RoundUp((num * 100.0) / den, 10.0);
    PRINTF("%6ld\t%-24s%6.1f %%\n", num, text, value);
}

static void
show_a_flag(char *text, long flag)
{
    if (spreadsheet)
	PRINTF("%ld%s", flag, comma);
    else if (flag)
	PRINTF("%6ld\t%s\n", flag, text);
}

static void
ratio(char *text, long num, long den)
{
    if (den == 0)
	den = 1;
    if (spreadsheet) {
	PRINTF("%.2f%s", (float) (num) / den, comma);
	return;
    }
    PRINTF("%6.2f\tratio of %s\n", (float) (num) / den, text);
}

static void
summarize_lines(STATS * p)
{
    long den = p->lines_total;

    new_summary();
    per_cent("lines had comments",
	     p->lines_notes + p->lines_inline, den);
    if (p->lines_rlogs || spreadsheet)
	per_cent("lines had history", p->lines_rlogs, den);
    per_cent("comments are inline", p->lines_inline, -den);
    per_cent("lines were blank", p->lines_blank, den);
    per_cent("lines for preprocessor", p->lines_prepro, den);
    per_cent("lines containing code", p->lines_code, den);
    per_cent(jargon ?
	     "total lines (PSS)" :
	     "total lines",
	     p->lines_notes + p->lines_rlogs + p->lines_blank +
	     p->lines_prepro + p->lines_code,
	     den);
}

static void
summarize_chars(STATS * p)
{
    long den = p->chars_total;

    new_summary();
    per_cent("comment-chars", p->chars_notes, den);
    if (p->chars_rlogs || spreadsheet)
	per_cent("history-chars", p->chars_rlogs, den);
    per_cent("nontext-comment-chars", p->chars_ignore, den);
    per_cent("whitespace-chars", p->chars_blank, den);
    per_cent("preprocessor-chars", p->chars_prepro, den);
    per_cent("statement-chars", p->chars_code, den);
    per_cent("total characters",
	     p->chars_blank +
	     p->chars_notes + p->chars_rlogs + p->chars_ignore +
	     p->chars_prepro + p->chars_code,
	     den);
}

static void
summarize_names(STATS * p)
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

static void
summarize_stats(STATS * p)
{
    new_summary();
    ratio("comment:code", p->chars_notes, p->chars_prepro + p->chars_code);
    show_a_flag("?:illegal characters found", p->flags_unasc);
    show_a_flag("\":lines with unterminated quotes", p->flags_unquo);
    show_a_flag("*:unterminated/nested comments", p->flags_uncmt);
    show_a_flag("+:unterminated blocks", p->flags_unlvl);
    show_a_flag(">:too-long identifiers", p->flags_2long);
}

static void
summarize_bloks(STATS * p)
{
    new_summary();
    if (spreadsheet) {
	PRINTF("%ld%s%ld%s",
	       p->top_lvl_blk, comma,
	       p->max_blk_lvl, comma);
	ratio("blocklevel:code", p->lvl_weights, p->chars_code);
	return;
    }
    PRINTF("%6ld\ttop-level blocks/statements\n",
	   p->top_lvl_blk);
    PRINTF("%6ld\tmaximum blocklevel\n",
	   p->max_blk_lvl + 1);
    ratio("blocklevel:code", p->lvl_weights, p->chars_code);
}

static void
show_totals(STATS * p)
{
    if (opt_line)
	summarize_lines(p);
    if (opt_char)
	summarize_chars(p);
    if (opt_name)
	summarize_names(p);
    if (opt_stat)
	summarize_stats(p);
    if (opt_blok)
	summarize_bloks(p);
}

static void
summarize(STATS * p, int mark, int name)
{
    char errors[8];
    int c = 0;

    newsum = FALSE;
    if (spreadsheet) {
	PRINTF("%ld%s%ld%s",
	       p->lines_total, comma,
	       p->stmts_total, comma);
    } else {
	PRINTF("%6ld ",
	       p->lines_total + (mark && !name));
	if ((p->stmts_total != old_stmts) || !mark || name) {
	    PRINTF("%5ld", p->stmts_total);
	} else {
	    PRINTF("%5s", " ");
	}
	if (verbose > 1 || (opt_blok && !opt_all)) {
	    if ((name || !mark)
		|| (mark && (p->top_lvl_blk != old_block)))
		PRINTF("  %5ld ", p->top_lvl_blk);
	    else
		PRINTF("%8s", " ");
	    if (name || !mark)
		PRINTF(" %5ld  ", p->max_blk_lvl + 1);
	    else if (mark && (p->nesting_lvl != old_level))
		PRINTF(" %5ld  ", p->nesting_lvl + 1);
	    else
		PRINTF("%8s", " ");
	}
	if (p->flags_unasc != old_unasc)
	    errors[c++] = '?';
	if (p->flags_unquo != old_unquo)
	    errors[c++] = '"';
	if (p->flags_uncmt != old_uncmt)
	    errors[c++] = '*';
	if (p->flags_unlvl != old_unlvl)
	    errors[c++] = '+';
	if (p->flags_2long != old_2long)
	    errors[c++] = '>';
	errors[c] = EOS;
	PRINTF("%-3.3s%c",
	       errors,
	       (mark ? '|' : ' '));
    }
    old_stmts = 0;
    old_2long = p->flags_2long;
    old_block = p->top_lvl_blk;
    old_level = p->nesting_lvl;
    old_stmts = p->stmts_total;
    old_unasc = p->flags_unasc;
    old_uncmt = p->flags_uncmt;
    old_unlvl = p->flags_unlvl;
    old_unquo = p->flags_unquo;
}

static void
Summary(int mark)
{
    if (newsum)
	summarize(&One, mark, FALSE);
}

#define	ADD(m)	All.m += One.m; One.m = 0

static void
add_totals(void)
{
    ADD(chars_total);
    ADD(chars_blank);
    ADD(chars_code);
    ADD(chars_ignore);
    ADD(chars_notes);
    ADD(chars_rlogs);
    ADD(chars_prepro);

    ADD(lines_total);
    ADD(lines_blank);
    ADD(lines_code);
    ADD(lines_inline);
    ADD(lines_notes);
    ADD(lines_rlogs);
    ADD(lines_prepro);

    ADD(flags_unquo);
    ADD(flags_uncmt);
    ADD(flags_unasc);
    ADD(flags_unlvl);
    ADD(flags_2long);

    ADD(stmts_total);

    ADD(words_total);
    ADD(words_length);

    ADD(top_lvl_blk);
    ADD(lvl_weights);

    if (One.nesting_lvl) {
	One.flags_unlvl += One.nesting_lvl;
	One.nesting_lvl = 0;
    }
    if (All.max_blk_lvl < One.max_blk_lvl)
	All.max_blk_lvl = One.max_blk_lvl;
    One.max_blk_lvl = 0;

    files_total++;
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
static int
Token(int c)
{
    static char *bfr;
    static unsigned used = 0;
    static unsigned have = 0;

    int j = 0;

    if (quotdef) {
	if (bfr == 0) {
	    bfr = malloc(have = 80);
	}
	One.words_total++;
	while (TOKEN(c)) {
	    One.words_length++;
	    if (used + 2 >= have) {
		bfr = realloc(bfr, have *= 2);
	    }
	    bfr[used++] = c;
	    c = inFile();
	    j++;
	}
	if (used) {
	    if (c == ' ' || c == '\t' || c == '(') {
		bfr[used] = EOS;
		for (j = 0; j < quotdef; j++) {
		    if (!strcmp(quotvec[j], bfr)) {
			c = String('"');
			DEBUG("**%c**", c);
			break;
		    }
		}
		DEBUG("%s\n", bfr);
	    }
	    used = 0;
	} else if (j == 0) {
	    c = inFile();
	}
	bfr[used] = 0;
    } else {
	if (TOKEN(c)) {
	    int word_length = 0;
	    One.words_total++;
	    DEBUG("'");
	    do {
		word_length++;
		DEBUG("%c", c);
		c = inFile();
	    } while (TOKEN(c));
	    DEBUG("'\n");
	    One.words_length += word_length;
	    if (word_length >= limit_name)
		One.flags_2long++;
	} else {		/* punctuation */
	    c = inFile();
	}
    }
    return (c);
}

/*
 * Count things related to the given character (-1 resets us to the beginning).
 */
static void
countChar(int ch)
{
    static int is_blank;	/* true til we get nonblank on line */
    static int had_note;
    static int had_code;
    static int cnt_code;
    static enum PSTATE bstate;

    if (ch < 0) {
	bstate = code;
	old_block =
	    old_level =
	    old_stmts =
	    old_unquo =
	    old_unlvl =
	    old_2long =
	    old_unasc =
	    old_uncmt = 0;
	had_note =
	    had_code = FALSE;
	cnt_code = 0;
	read_last = EOS;
	is_blank = TRUE;
    } else {
	if (verbose && (!One.chars_total || read_last == '\n'))
	    Summary(TRUE);
	newsum = TRUE;
	if (!isascii(ch) || (!isprint(ch) && !isspace(ch))) {
	    ch = '?';		/* protect/flag this */
	    One.flags_unasc++;
	}

	/* If requested, show the file.  But avoid showing a
	 * carriage return unless it is embedded in the line.
	 */
	if (verbose) {
	    if (ch != '\r') {
		if (ch != '\n' && wrote_last == '\r')
		    (void) putchar('\r');
		(void) putchar(ch);
	    }
	}
	wrote_last = ch;

	One.chars_total++;

	if (ch == '#' && is_blank) {
	    bstate = preprocessor;
	    pstate = preprocessor;
	} else if (ch == '\n') {
	    if (cnt_code) {
		had_code += (pstate != comment);
		if (pstate == comment)
		    had_note = TRUE;
		cnt_code = 0;
	    }
	    One.lines_total++;
	    if (is_blank) {
		One.lines_blank++;
	    } else {
		if (pstate == preprocessor
		    || bstate == preprocessor) {
		    One.lines_prepro++;
		    if (had_note) {
			One.lines_inline++;
		    }
		} else if (had_code) {
		    One.lines_code++;
		    if (had_note) {
			One.lines_inline++;
		    }
		} else if (had_note || pstate == comment) {
		    One.lines_notes++;
		}
		had_code =
		    had_note = FALSE;
	    }
	    is_blank = TRUE;
	    if (pstate == preprocessor && read_last != '\\') {
		bstate = code;
		pstate = code;
	    }
	}

	if (isspace(ch)) {
	    if (cnt_code) {
		had_code += (pstate == code);
		cnt_code = 0;
	    }
	    if (literal) {
		One.chars_code++;
		LVL_WEIGHT(1);
	    } else {
		One.chars_blank++;
	    }
	} else {
	    is_blank = FALSE;
	    switch (pstate) {
	    case comment:
		if (cnt_code) {
		    had_code += (cnt_code > 2);
		    cnt_code = 0;
		}
		had_note = TRUE;
		if (isalnum(ch))
		    One.chars_notes++;
		else
		    One.chars_ignore++;
		break;
	    case preprocessor:
		One.chars_prepro++;
		break;
	    default:
		cnt_code++;
		One.chars_code++;
		LVL_WEIGHT(1);
	    }
	}
	read_last = ch;
    }
}

/*
 * Process a single file:
 */
static void
doFile(char *name)
{
    int c;
    int topblock = FALSE;

#if !SYS_UNIX && !defined(WIN32)
    if (has_wildcard(name)) {	/* expand wildcards? */
	char expanded[BUFSIZ];
	int count = 0;
	(void) strcpy(expanded, name);
	while (expand_wildcard(expanded, !count++))
	    doFile(expanded);
	return;
    }
    /* trim trailing blanks */
    for (c = strlen(name); c > 0; c--) {
	if (isspace(name[--c]))
	    name[c] = EOS;
	else
	    break;
    }
#endif

    pstate = code;
    if (!strcmp(name, "-"))
	File = stdin;
    else if (!(File = fopen(name, FOPEN_MODE)))
	return;

    newsum = TRUE;
    cms_history = FALSE;
    wrote_last = -1;
    c = inFile();

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
	case '{':
	    if (pstate == code) {
		One.nesting_lvl++;
		if (One.nesting_lvl >
		    One.max_blk_lvl)
		    One.max_blk_lvl = One.nesting_lvl;
	    }
	    c = Token(c);
	    break;
	case '}':
	    if (pstate == code) {
		One.nesting_lvl--;
		if (One.nesting_lvl == 0) {
		    topblock = TRUE;
		} else if (One.nesting_lvl < 0) {
		    One.flags_unlvl += One.nesting_lvl;
		    One.nesting_lvl = 0;
		}
	    }
	    c = Token(c);
	    break;
	case ';':
	    One.stmts_total++;
	    if (pstate == code) {
		if (One.nesting_lvl == 0) {
		    One.top_lvl_blk++;
		}
		topblock = FALSE;
	    }
	    c = Token(c);
	    break;
	case ',':
	    topblock = FALSE;
	    c = Token(c);
	    break;
	default:
	    if (pstate == code) {
		if (One.nesting_lvl == 0
		    && topblock) {
		    One.top_lvl_blk++;
		}
		topblock = FALSE;
	    }
	    c = Token(c);
	    break;
	}
    }
    (void) Token(EOS);
    (void) fclose(File);

    if (wrote_last != '\n') {
	(void) countChar('\n');	/* fake a newline */
	One.chars_total--;
	One.chars_blank--;
    }

    old_stmts = 0;		/* force # of statements to display */

    if (per_file && spreadsheet) {
	show_totals(&One);
    } else if (!per_file)
	summarize(&One, TRUE, TRUE);
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
static int
String(int mark)
{
    int c = inFile();
    char *p = "@(#)";		/* permit literal tab here only! */

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
	    One.flags_unasc++;	/* this may duplicate 'unquo'! */
	if (c == '\n') {	/* this is legal, but not likely */
	    One.flags_unquo++;	/* ...assume balance is in macro */
	    return (inFile());
	} else if (c == mark) {
	    literal = FALSE;
	    return (inFile());
	} else if (c == '\\')
	    c = Escape();
	else
	    c = inFile();
    }
    literal = FALSE;
    return (c);
}

/*
 * Scan over an '\' escape sequence, returning the first character after it.
 * If we start with an octal digit, we may read up to OCTAL of these in a row.
 */
static int
Escape(void)
{
    int c = inFile();
    int digits = 0;

    if (c != EOF) {
	while (c >= '0' && c <= '7' && digits < OCTAL) {
	    digits++;
	    if ((c = inFile()) == EOF)
		return (c);
	}
	if (!digits)
	    c = inFile();
    }
    return (c);
}

/*
 * Identify comments which we disregard because they were generated by the
 * RCS history mechanism.
 */
static void
Disregard(char *lo, char *hi)
{
    while (lo <= hi) {
	if (isalnum(*lo)) {
	    One.chars_notes -= 1;
	    One.chars_rlogs += 1;
	}
	lo++;
    }
    One.lines_notes -= 1;
    One.lines_rlogs += 1;
}

/*
 * Compare strings ignoring blanks (TD_LIB)
 */
#define	SKIP(p)	while (isspace(*p))	p++;

static int
strbcmp(char *a, char *b)
{
    int cmp;

    while (*a && *b) {
	if (isspace(*a) && isspace(*b)) {
	    SKIP(a);
	    SKIP(b);
	} else if ((cmp = (*a++ - *b++)) != EOS)
	    return (cmp);
    }
    SKIP(a);
    SKIP(b);
    return (*a - *b);
}

/*
 * Read characters within a comment, keeping track to find RCS or DEC/CMS
 * history comments.
 *
 * RCS history comments consist of an RCS identifier "Log", followed by zero
 * or more groups of lines beginning with the keyword "Revision". Each line in
 * the group is prefixed by the same string (the RCS "comment" prefix).  The
 * groups are separated by a line containing only the comment-prefix.
 *
 * DEC/CMS history comments consist of one or more comment lines between
 * comments containing the string shown below.  These comments (due to the
 * nature of CMS's history substitution) must be self-contained on a line,
 * unlike the RCS history.
 */
static int
filter_history(int first)
{
    enum HSTATE {
	unknown,
	cms,
	rlog,
	revision,
	contents
    };
    static char *CMS_ = "DEC/CMS REPLACEMENT HISTORY,";
    static enum HSTATE hstate = unknown;
    static char buffer[BUFSIZ];
    static char prefix[BUFSIZ];
    static size_t len;

    char *s, *d, *t;

    int c = inFile();

    if (first) {
	buffer[len = 0] = EOS;
	prefix[0] = EOS;
	hstate = cms_history ? cms : unknown;
    } else if (len < sizeof(buffer)) {
	buffer[len++] = c;
	buffer[len] = EOS;
    }
    if (c == '\n') {
	/* try to find CMS bracketing comment */
	len = strlen(CMS_);
	for (s = buffer; *s; s++) {
	    if (!strncmp(s, CMS_, len)) {
		cms_history = !cms_history;
		hstate = cms;
		break;
	    }
	}

	/* ignore all comments within DEC/CMS bracketing */
	if (hstate == cms) {
	    Disregard(buffer, buffer + strlen(buffer));

	    /* try to find RCS identifier "Log" */
	} else if (hstate == unknown) {
	    s = buffer;
	    while ((d = strchr(s, '$')) != NULL) {
		s = d + 1;
		if (!strncmp(d, "$Log", 4)
		    && (t = strchr(s, '$')) != 0) {
		    hstate = rlog;
		    Disregard(d, t);
		    break;
		}
	    }
	    /* try to find "Revision" after comment-prefix */
	} else if (hstate == rlog) {
	    s = buffer;
	    while ((d = strchr(s, 'R')) != NULL) {
		s = d + 1;
		if (!strncmp(d, "Revision", 8)) {
		    size_t len2 = (size_t) (d - buffer);
		    strcpy(prefix, buffer)[len2] = EOS;
		    hstate = revision;
		    s += strlen(s);
		    Disregard(d, s - 2);
		}
	    }
	} else if (hstate == revision
		   || hstate == contents) {
	    if (!strncmp(buffer, prefix, (size_t) strlen(prefix))) {
		d = buffer + strlen(prefix);
		s = d + strlen(d);
		hstate = (*d == '\n') ? rlog : contents;
		Disregard(d, s - 2);
	    } else if (!strbcmp(buffer, prefix)) {
		hstate = rlog;	/* assume user trimmed spaces */
	    } else
		hstate = unknown;
	}
	buffer[len = 0] = EOS;
    }
    return (c);
}

/*
 * Entered immediately after reading '/','*', scan over a comment, returning
 * the first character after the comment.  Flags both unterminated comments
 * and nested comments with 'uncmt'.
 */
static int
Comment(int c_plus_plus)
{
    int c;
    int d = 0;
    enum PSTATE save_st = pstate;

    if (pstate == code) {
	One.chars_code -= 2;
	LVL_WEIGHT(-2);
    } else {
	One.chars_prepro -= 2;
    }
    One.chars_ignore += 2;	/* ignore the comment-delimiter */

    pstate = comment;
    c = filter_history(TRUE);
    while (c != EOF) {
	if (c_plus_plus) {
	    if (c == '\n') {
		pstate = save_st;
		return (c);
	    }
	    c = filter_history(FALSE);
	} else {
	    if (c == '*') {
		c = filter_history(FALSE);
		if (c == '/') {
		    pstate = save_st;
		    return (inFile());
		}
	    } else {
		c = filter_history(FALSE);
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
static int
inFile(void)
{
    int c = fgetc(File);

    if (feof(File) || ferror(File)) {
	c = EOF;
	if (read_last != '\n')
	    One.flags_unasc++;
    } else {
	c &= 0xff;		/* protect against sign-extension bug */
    }
    if (One.chars_total == 0) {
	countChar(-1);
    }

    if (c != EOF) {
	countChar(c);
    }
    return (c);
}

static void
usage(void)
{
    static char *tbl[] =
    {
	"Usage: c_count [options] [files]"
	,""
	,"If no files are specified as arguments, a list of filenames is read from the"
	,"standard input.  The special name \"-\" denotes a file which is read from the"
	,"standard input."
	,""
	,"Options:"
	," -b        block-statistics"
	," -c        character-statistics"
	," -d        debug (shows tokens as they are parsed)"
	," -i        identifier-statistics"
	," -j        annotate summary in technical format"
	," -l        line-statistics"
	," -o file   specify alternative output-file"
	," -p        per-file statistics"
	," -q DEFINE tells c_count that the given name is an unbalanced quote"
	," -s        specialized statistics"
	," -t        generate output for spreadsheet"
	," -v        verbose (shows lines as they are counted)"
	," -w LEN    set threshold for too-long identifiers (default 32)"
    };
    unsigned j;
    for (j = 0; j < sizeof(tbl) / sizeof(tbl[0]); j++)
	(void) fprintf(stderr, "%s\n", tbl[j]);
    (void) exit(EXIT_FAILURE);
}

/************************************************************************
 *	main procedure							*
 ************************************************************************/

int
main(int argc, char **argv)
{
    int j;
    char name[BUFSIZ];

#ifdef WIN32
    _setargv(&argc, &argv);
#endif
    quotvec = typeCalloc(char *, (size_t) argc);
    while ((j = getopt(argc, argv, "bcdijlo:pq:stvw:")) != EOF) {
	switch (j) {
	case 'l':
	    opt_all = FALSE;
	    opt_line = TRUE;
	    break;
	case 'b':
	    opt_all = FALSE;
	    opt_blok = TRUE;
	    break;
	case 'c':
	    opt_all = FALSE;
	    opt_char = TRUE;
	    break;
	case 'i':
	    opt_all = FALSE;
	    opt_name = TRUE;
	    break;
	case 's':
	    opt_all = FALSE;
	    opt_stat = TRUE;
	    break;

	case 'd':
	    debug = TRUE;
	    break;
	case 'j':
	    jargon = TRUE;
	    break;
	case 'p':
	    per_file = TRUE;
	    break;
	case 'v':
	    verbose++;
	    break;
	case 'o':
	    if (!freopen(optarg, "w", stdout))
		usage();
	    break;
	case 'q':
	    quotvec[quotdef++] = optarg;
	    break;
	case 't':
	    spreadsheet = TRUE;
	    break;
	case 'w':
	    limit_name = atoi(optarg);
	    break;
	default:
	    usage();
	}
    }

    if (opt_all == -1)
	opt_blok = opt_line = opt_char = opt_name = opt_stat = TRUE;
    else if (spreadsheet)
	per_file = TRUE;

    if (spreadsheet) {
	if (per_file) {
	    if (opt_line)
		PRINTF("%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
		       "L-COMMENT", comma,
		       "L-HISTORY", comma,
		       "L-INLINE", comma,
		       "L-BLANK", comma,
		       "L-CPP", comma,
		       "L-CODE", comma,
		       "L-TOTAL", comma);
	    if (opt_char)
		PRINTF("%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
		       "C-COMMENT", comma,
		       "C-HISTORY", comma,
		       "C-IGNORE", comma,
		       "C-BLANK", comma,
		       "C-CPP", comma,
		       "C-CODE", comma,
		       "C-TOTAL", comma);
	    if (opt_name)
		PRINTF("%s%s%s%s",
		       "W-TOTAL", comma,
		       "W-LENGTH", comma);
	    if (opt_stat)
		PRINTF("%s%s%s%s%s%s%s%s%s%s%s%s",
		       "CODE:COMMENT", comma,
		       "ILLEGAL-CHARS", comma,
		       "ILLEGAL-QUOTES", comma,
		       "ILLEGAL-COMMENTS", comma,
		       "ILLEGAL-BLOCKS", comma,
		       "ILLEGAL-NAMES", comma);
	    if (opt_blok)
		PRINTF("%s%s%s%s%s%s",
		       "TOP-BLOCKS", comma,
		       "MAX-NESTING", comma,
		       "AVG-NESTING", comma);

	} else
	    PRINTF("LINES%sSTATEMENTS%s", comma, comma);
	PRINTF("FILENAME\n");
    }

    if (optind < argc) {
	for (j = optind; j < argc; j++)
	    doFile(argv[j]);
    } else {
	while (fgets(name, sizeof(name) - 1, stdin)) {
	    size_t len = strlen(name);
	    if (len != 0 && name[--len] == '\n')
		name[len] = EOS;
	    doFile(name);
	}
    }

    if (!spreadsheet && files_total) {
	if (verbose > 1 || (opt_blok && !opt_all))
	    PRINTF("%s", dashes);
	if (!per_file) {
	    PRINTF("%s\n", dashes);
	    summarize(&All, FALSE, FALSE);
	    PRINTF("%s",
		   jargon ?
		   "physical source statements/logical source statements" :
		   "total lines/statements");
	    if (opt_blok && !opt_all)
		PRINTF(", top-level blocks, maximum nesting");
	    PRINTF("\n");
	} else {
	    PRINTF("Grand total\n");
	    PRINTF("%s\n", dashes);
	}
	show_totals(&All);
	PRINTF("\n");
    }
    (void) exit(EXIT_SUCCESS);
    /*NOTREACHED */
}
