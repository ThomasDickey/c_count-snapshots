/*
 * Title:	c_count.c
 * Author:	T.E.Dickey
 * Created:	04 Dec 1985
 * Modified:
 *		14 Dec 2013, minor fix for Quoted(); if the first character of
 *			     a quoted string was a blank, it was added to the
 *			     blanks category rather than to code.
 *		24 Aug 2013, ifdef to assume MinGW does globbing.
 *		16 Jul 2010, fix strict compiler warnings, e.g., with const.
 *		08 Jan 2006, correct bookkeeping for unterminated blocks.
 *		17 Jul 2005, show statement numbers at the beginning of a
 *			     statement with the -v option, rather than at
 *			     the terminating ';'.
 *		16 Jul 2005, modify treatment of -v option so it buffers data
 *			     to allow reporting the current line, rather than
 *			     the previous line.  Originally a debugging option,
 *			     it is useful in itself for making formatted
 *			     listings.
 *		26 Jun 2005, add -n option.  Correct treatment of "#" in
 *			     quotes (could be confused with preprocessor).
 *			     Correct treatment of blanks in quoted string in
 *			     preprocessor statement (was counted in statement).
 *			     Modify check for SCCS tag to allow it to be past
 *			     the beginning of a string.  Improve parsing so
 *			     numbers are not treated as tokens (for average
 *			     length computation).  Add parsing for \x escapes.
 *		22 Nov 2002, Convert to ANSI C, indent'd.  Fix a bug in -q/-d
 *			     logic.  Parse filename after #include as a string.
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
 *		28 Jan 1986, make final 'exit()' with return code.
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
#include "patchlev.h"

#ifndef	NO_IDENT
static const char Id[] = "$Id: c_count.c,v 7.59 2013/12/14 13:45:23 tom Exp $";
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

#define UCH(c)     ((unsigned char)(c))
#ifndef isascii
#define isascii(c) (UCH(c) < 128)
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

#define	ESC_OCTAL	10	/* # of octal digits permissible in escape */
#define	ESC_HEX		8	/* # of hex digits permissible in escape */

#ifndef isoctal
#define isoctal(c) ((c) >= '0' && (c) <= '7')
#endif

#define isCurly(c) ((c) == '{' || (c) == '}')

#define PRINTF  	(void)printf
#define	DEBUG		if (debug) PRINTF

#define	TOKEN(c)	((c) == '_' || (c) == '$' || isalpha(c))
#define TOKEN2(c)	(TOKEN(c) || isdigit(c))

#define	NUMBER(c)	(isdigit(c) || (c) == '.')
#define NUMBER2(c)	(NUMBER(c) || strchr("+-eEpPlLuUxX", c) != 0)

#define LVL_WEIGHT(n)	if (opt_blok) One.lvl_weights += (n) * (One.nesting_lvl+1)

#if defined(WIN32) && (defined(__MINGW32__) || defined(__MINGW64__))
int _dowildard = -1;
#endif

/*
 * To make the verbose listing look nice (without doing tab-conversion), we
 * want to have the columns for statements, lines, error flags add up to
 * a multiple of 8.
 */
#define DIGITS_LINES	6
#define DIGITS_STMTS	5
#define DIGITS_FLAGS	((DIGITS_LINES + 1 + DIGITS_STMTS + 8) % 8)

/* do not count curly-braces as part of statements, for numbering purposes */
#define CHARS_STMTS(p) ((p)->chars_code - (p)->chars_curly)

static int inFile(void);
static int Comment(int cpp);
static int Escape(void);
static int Quoted(int mark);

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
    long chars_curly;
    /* line-counts */
    long lines_total;		/* # of lines */
    long lines_blank;
    long lines_code;
    long lines_inline;		/* in-line comments */
    long lines_notes;		/* all other comments */
    long lines_rlogs;		/* RCS history comments */
    long lines_prepro;
    /* flags */
    long flags_unquo;		/* lines with unterminated quotes */
    long flags_uncmt;		/* unterminated/nested comments */
    long flags_unasc;		/* illegal characters found */
    long flags_unlvl;		/* unterminated blocks */
    long flags_2long;		/* too-long identifiers */
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

static STATS Old;		/* previous data for summarize() */

enum PSTATE {
    pCode,
    pComment,
    pPreprocessor
};
static enum PSTATE pstate;

static int within_stmt;		/* nonzero within statements */

static int verbose = FALSE;	/* TRUE iff we echo file, as processed */
static int quotdef = 0;		/* number of tokens we treat as '"' */
static int jargon = FALSE;
static int per_file = FALSE;
static int debug = FALSE;
static int opt_all = -1;
static int opt_blok = FALSE;	/* "-b" option */
static int opt_line = FALSE;	/* "-l" option */
static int opt_char = FALSE;	/* "-c" option */
static int opt_name = FALSE;	/* "-i" option */
static int opt_stat = FALSE;	/* "-s" option */
static int opt_summary = TRUE;	/* "-n" option */
static int spreadsheet = FALSE;
static int cms_history = FALSE;
static int files_total = 0;
static int limit_name = 32;
static int read_last;
static int wrote_last;
static int newsum;		/* TRUE iff we need a summary */

/* buffer line for verbose-mode */
static char *big_line = 0;
static size_t big_used = 0;
static size_t big_size = 0;

static const char *comma = ",";
static const char *dashes = "----------------";

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
per_cent(const char *text, long num, long den)
{
    double value;
    if (spreadsheet) {
	PRINTF("%ld%s", num, comma);
	return;
    }
    if (num == 0 || den == 0)
	value = 0.0;
    else
	value = RoundUp(((double) num * 100.0) / (double) den, 10.0);
    PRINTF("%6ld\t%-24s%6.1f %%\n", num, text, value);
}

static void
show_a_flag(const char *text, long flag)
{
    if (spreadsheet)
	PRINTF("%ld%s", flag, comma);
    else if (flag)
	PRINTF("%6ld\t%s\n", flag, text);
}

static void
ratio(const char *text, long num, long den)
{
    if (den == 0)
	den = 1;
    if (spreadsheet) {
	PRINTF("%.2f%s", (float) num / (float) den, comma);
	return;
    }
    PRINTF("%6.2f\tratio of %s\n", (float) num / (float) den, text);
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
	       (1.0 * (double) p->words_length) / (double) p->words_total);
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
    char errors[80];
    int c = 0;

    newsum = FALSE;
    if (spreadsheet) {
	PRINTF("%ld%s%ld%s",
	       p->lines_total, comma,
	       p->stmts_total, comma);
    } else {
	int showed_stmts = FALSE;

	/* show line- and statement-numbers */
	PRINTF("%*ld ",
	       DIGITS_LINES, p->lines_total + (mark && !name));
	if ((p->stmts_total != Old.stmts_total) || !mark || name) {
	    PRINTF("%*ld", DIGITS_STMTS, p->stmts_total);
	    showed_stmts = TRUE;
	} else {
	    PRINTF("%*s", DIGITS_STMTS, " ");
	}

	/* show block-level */
	if (verbose > 1 || (opt_blok && !opt_all)) {
	    if ((name || !mark)
		|| (mark && (p->top_lvl_blk != Old.top_lvl_blk)))
		PRINTF("  %5ld ", p->top_lvl_blk);
	    else
		PRINTF("%8s", " ");
	    if (name || !mark)
		PRINTF(" %5ld  ", p->max_blk_lvl + 1);
	    else if (mark && (p->nesting_lvl != Old.nesting_lvl))
		PRINTF(" %5ld  ", p->nesting_lvl + 1);
	    else
		PRINTF("%8s", " ");
	}

	/* show single-character flags */
	if (p->flags_unasc != Old.flags_unasc)
	    errors[c++] = '?';
	if (p->flags_unquo != Old.flags_unquo)
	    errors[c++] = '"';
	if (p->flags_uncmt != Old.flags_uncmt)
	    errors[c++] = '*';
	if (p->flags_unlvl != 0)
	    errors[c++] = '+';
	if (p->flags_2long != Old.flags_2long)
	    errors[c++] = '>';

	while (c < DIGITS_FLAGS - 2) {
	    errors[c++] = ' ';
	}
	/* If there is room, mark the last flag showing if the line contained
	 * code or a preprocessor-line.  Normally this shows continuation
	 * lines.
	 */
	if (!showed_stmts && CHARS_STMTS(p) != CHARS_STMTS(&Old)) {
	    errors[c++] = '.';
	} else {
	    errors[c++] = ' ';
	}

	if (c > DIGITS_FLAGS)
	    c = DIGITS_FLAGS;
	errors[c] = EOS;
	PRINTF("%s%c",
	       errors,
	       (mark ? '|' : ' '));
    }
    Old = *p;
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
    ADD(chars_curly);

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

    if (All.max_blk_lvl < One.max_blk_lvl)
	All.max_blk_lvl = One.max_blk_lvl;
    One.max_blk_lvl = 0;

    if (One.nesting_lvl > 0)
	All.flags_unlvl += One.nesting_lvl;
    One.nesting_lvl = 0;

    files_total++;
}

/*
 * Treat an included-filename as a string.
 */
static int
IncludeFile(int c)
{
    while (c == ' ' || c == '\t')
	c = inFile();
    if (c == '"')
	c = Quoted(c);
    else if (c == '<')
	c = Quoted('>');
    return c;
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
    static size_t used = 0;
    static size_t have = 0;

    enum PSTATE bstate = pstate;
    int j = 0;

    if (bfr == 0)
	bfr = malloc(have = 80);

    if (TOKEN(c)) {
	int word_length = 0;
	One.words_total++;
	do {
	    if (used + 2 >= have)
		bfr = realloc(bfr, have *= 2);
	    bfr[used++] = (char) c;
	    word_length++;
	    c = inFile();
	} while (TOKEN2(c));
	bfr[used] = EOS;

	DEBUG("name\t%s\n", bfr);

	One.words_length += word_length;
	if (word_length >= limit_name)
	    One.flags_2long++;

	if (pstate == pPreprocessor && !strcmp(bfr, "include")) {
	    c = IncludeFile(c);
	} else if (quotdef) {
	    if (c == ' ' || c == '\t' || c == '(') {
		for (j = 0; j < quotdef; j++) {
		    if (!strcmp(quotvec[j], bfr)) {
			c = Quoted('"');
			DEBUG("**%c**\n", c);
			break;
		    }
		}
	    }
	}
    } else if (NUMBER(c)) {
	do {
	    if (used + 2 >= have)
		bfr = realloc(bfr, have *= 2);
	    bfr[used++] = (char) c;
	    c = inFile();
	} while (NUMBER2(c));
	bfr[used] = EOS;

	DEBUG("number\t%s\n", bfr);
    } else {			/* punctuation */
	if (debug) {
	    if (isgraph(c) && strchr("{};/\\", c) == 0)
		DEBUG("char\t%c\n", c);
	}
	c = inFile();
    }
    used = 0;

    if ((c == '\n') && pstate == pCode && bstate != pCode)
	DEBUG("\n");

#ifdef NO_LEAKS2
    if (bfr != 0) {
	free(bfr);
	bfr = 0;
	have = 0;
    }
#endif
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
	within_stmt = 0;
	bstate = pCode;
	memset(&Old, 0, sizeof(Old));
	had_note =
	    had_code = FALSE;
	cnt_code = 0;
	read_last = EOS;
	is_blank = TRUE;
    } else {
	newsum = TRUE;
	if (!isascii(ch) || (!isprint(ch) && !isspace(ch))) {
	    ch = '?';		/* protect/flag this */
	    One.flags_unasc++;
	}

	/* If requested, show the file.  But avoid showing a
	 * carriage return unless it is embedded in the line.
	 */
	if (verbose) {
	    if (big_used + 4 >= big_size) {
		big_line = realloc(big_line, big_size *= 2);
		if (big_line == 0) {
		    perror("realloc");
		    exit(EXIT_FAILURE);
		}
	    }
	    if (wrote_last == '\r' && ch != '\n') {
		big_line[big_used - 1] = '^';
		big_line[big_used++] = 'M';
	    }
	    big_line[big_used++] = (char) ch;
	    big_line[big_used] = EOS;
	    if (ch == '\n') {
		Summary(TRUE);
		fputs(big_line, stdout);
		big_line[big_used = 0] = EOS;
	    }
	}
	wrote_last = ch;

	One.chars_total++;

	if (ch == '#' && is_blank && !literal) {
	    bstate = pPreprocessor;
	    pstate = pPreprocessor;
	} else if (ch == '\n') {
	    if (cnt_code) {
		had_code += (pstate != pComment);
		if (pstate == pComment)
		    had_note = TRUE;
		cnt_code = 0;
	    }
	    One.lines_total++;
	    if (is_blank) {
		One.lines_blank++;
	    } else {
		if (pstate == pPreprocessor
		    || bstate == pPreprocessor) {
		    One.lines_prepro++;
		    if (had_note) {
			One.lines_inline++;
		    }
		} else if (had_code) {
		    One.lines_code++;
		    if (had_note) {
			One.lines_inline++;
		    }
		} else if (had_note || pstate == pComment) {
		    One.lines_notes++;
		}
		had_code =
		    had_note = FALSE;
	    }
	    is_blank = TRUE;
	    if (pstate == pPreprocessor && read_last != '\\') {
		bstate = pCode;
		pstate = pCode;
	    }
	}

	if (isspace(ch)) {
	    if (cnt_code) {
		had_code += (pstate == pCode);
		cnt_code = 0;
	    }
	    if (literal) {
		if (pstate == pCode) {
		    One.chars_code++;
		    LVL_WEIGHT(1);
		} else {
		    One.chars_prepro++;
		}
	    } else {
		One.chars_blank++;
	    }
	} else {
	    is_blank = FALSE;
	    switch (pstate) {
	    case pComment:
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
	    case pPreprocessor:
		One.chars_prepro++;
		break;
	    case pCode:
		if (!isspace(ch)) {
		    if (within_stmt) {
			within_stmt++;
		    } else if (!isCurly(ch) && !literal) {
			within_stmt = TRUE;
			One.stmts_total++;
		    }
		}
		cnt_code++;
		One.chars_code++;
		LVL_WEIGHT(1);
		break;
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

    pstate = pCode;
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
	    else
		DEBUG("char\t%c\n", c);
	    break;
	case '"':
	case '\'':
	    c = Quoted(c);
	    break;
	case '{':
	    DEBUG("char\t%c\n", c);
	    if (pstate == pCode) {
		One.chars_curly++;
		One.nesting_lvl++;
		if (One.nesting_lvl >
		    One.max_blk_lvl)
		    One.max_blk_lvl = One.nesting_lvl;
	    }
	    c = Token(c);
	    break;
	case '}':
	    DEBUG("char\t%c\n", c);
	    if (pstate == pCode) {
		One.chars_curly++;
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
	    DEBUG("char\t%c\n", c);
	    within_stmt = 0;
	    if (pstate == pCode) {
		if (One.nesting_lvl == 0) {
		    One.top_lvl_blk++;
		}
		topblock = FALSE;
	    }
	    c = Token(c);
	    break;
	case ',':
	    DEBUG("char\t%c\n", c);
	    topblock = FALSE;
	    c = Token(c);
	    break;
	default:
	    if (pstate == pCode) {
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

    Old.stmts_total = 0;	/* force # of statements to display */

    if (per_file && spreadsheet) {
	show_totals(&One);
    } else if (!per_file) {
	summarize(&One, TRUE, TRUE);
    }
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
Quoted(int mark)
{
    static const char *sccs_tag = "@(#)";
    const char *p = sccs_tag;	/* permit literal tab here only! */
    int c;

    DEBUG("string\t%c", (mark == '>') ? '<' : mark);
    literal = TRUE;
    c = inFile();
    while (c != EOF) {
	if (c == '\n') {	/* this is legal, but not likely */
	    One.flags_unquo++;	/* ...assume balance is in macro */
	    break;
	} else {
	    if (!isprint(c)) {
		if (c != '\t' || *p != '\0')
		    One.flags_unasc++;
	    }
	    if (*p != '\0') {
		if (c != *p++)
		    p = sccs_tag;
	    }
	    if (c == mark) {
		DEBUG("%c", c);
		literal = FALSE;
		c = inFile();
		break;
	    } else if (c == '\\') {
		c = Escape();
	    } else {
		DEBUG("%c", c);
		c = inFile();
	    }
	}
    }
    literal = FALSE;
    DEBUG("\n");
    return (c);
}

/*
 * Scan over an '\' escape sequence, returning the first character after it.
 * If we start with an octal digit, we may read up to ESC_OCTAL of these in a
 * row.
 */
static int
Escape(void)
{
    int c = inFile();
    int digits = 0;

    if (c != EOF) {
	if (isoctal(c)) {
	    while (isoctal(c) && digits < ESC_OCTAL) {
		digits++;
		if ((c = inFile()) == EOF)
		    return (c);
	    }
	} else if (c == 'x') {
	    c = inFile();
	    while (isxdigit(c) && digits < ESC_HEX) {
		digits++;
		if ((c = inFile()) == EOF)
		    return (c);
	    }
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
	if (isalnum(UCH(*lo))) {
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
#define	SKIP(p)	while (isspace(UCH(*p)))	p++;

static int
strbcmp(char *a, char *b)
{
    int cmp;

    while (*a && *b) {
	if (isspace(UCH(*a)) && isspace(UCH(*b))) {
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
    static const char *CMS_ = "DEC/CMS REPLACEMENT HISTORY,";
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
	buffer[len++] = (char) c;
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
		if (!strncmp(d, "$Log", (size_t) 4)
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
		if (!strncmp(d, "Revision", (size_t) 8)) {
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

    if (within_stmt == 2) {
	One.stmts_total--;
	within_stmt = 0;
    }

    if (pstate == pCode) {
	One.chars_code -= 2;
	LVL_WEIGHT(-2);
    } else {
	One.chars_prepro -= 2;
    }
    One.chars_ignore += 2;	/* ignore the comment-delimiter */

    pstate = pComment;
    c = filter_history(TRUE);
    while (c != EOF) {
	if (c_plus_plus) {
	    if (c == '\n') {
		if (save_st == pPreprocessor)
		    DEBUG("\n");
		pstate = save_st;
		return (c);
	    }
	    c = filter_history(FALSE);
	} else {
	    if (c == '*') {
		c = filter_history(FALSE);
		if (c == '/') {
		    pstate = save_st;
		    c = inFile();
		    if (c == '\n' && save_st == pPreprocessor)
			DEBUG("\n");
		    return c;
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

#define OPTIONS "\
b\
c\
d\
i\
j\
l\
n\
o:\
p\
q:\
s\
t\
V\
v\
w:\
"

static void
usage(void)
{
    static const char *tbl[] =
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
	," -n        do not print summary-statistics"
	," -o file   specify alternative output-file"
	," -p        per-file statistics"
	," -q DEFINE tells c_count that the given name is an unbalanced quote"
	," -s        specialized statistics"
	," -t        generate output for spreadsheet"
	," -V        print the version"
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

#if defined(WIN32) && !(defined(__MINGW32__) || defined(__MINGW64__))
    _setargv(&argc, &argv);
#endif
    quotvec = typeCalloc(char *, (size_t) argc);
    while ((j = getopt(argc, argv, OPTIONS)) != EOF) {
	switch (j) {
	case 'b':
	    opt_all = FALSE;
	    opt_blok = TRUE;
	    break;
	case 'c':
	    opt_all = FALSE;
	    opt_char = TRUE;
	    break;
	case 'd':
	    debug = TRUE;
	    break;
	case 'i':
	    opt_all = FALSE;
	    opt_name = TRUE;
	    break;
	case 'j':
	    jargon = TRUE;
	    break;
	case 'l':
	    opt_all = FALSE;
	    opt_line = TRUE;
	    break;
	case 'n':
	    opt_summary = FALSE;
	    break;
	case 'o':
	    if (!freopen(optarg, "w", stdout))
		usage();
	    break;
	case 'p':
	    per_file = TRUE;
	    break;
	case 'q':
	    quotvec[quotdef++] = optarg;
	    break;
	case 's':
	    opt_all = FALSE;
	    opt_stat = TRUE;
	    break;
	case 't':
	    spreadsheet = TRUE;
	    break;
	case 'V':
	    PRINTF("c_count version %d.%d\n", RELEASE, PATCHLEVEL);
	    return EXIT_SUCCESS;
	case 'v':
	    verbose++;
	    if (big_line == 0)
		big_line = malloc(big_size = 1024);
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

	} else {
	    PRINTF("LINES%sSTATEMENTS%s", comma, comma);
	}
	PRINTF("FILENAME\n");
    }

    if (optind < argc) {
	for (j = optind; j < argc; j++)
	    doFile(argv[j]);
    } else {
	while (fgets(name, (int) sizeof(name) - 1, stdin)) {
	    size_t len = strlen(name);
	    if (len != 0 && name[--len] == '\n')
		name[len] = EOS;
	    doFile(name);
	}
    }

    if (!spreadsheet && files_total && opt_summary) {
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
#ifdef NO_LEAKS
    if (big_line != 0)
	free(big_line);
    free(quotvec);
#endif

    return EXIT_SUCCESS;
}
