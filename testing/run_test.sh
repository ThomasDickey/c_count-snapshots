#!/bin/sh
# $Id: run_test.sh,v 6.0 1991/10/21 16:06:48 ste_cm Rel $
#
PROG=../bin/lincnt
TMP=/tmp/lincnt$$
rm -f $TMP
#
cat <<EOF/
**
**	Case 1:	Count lines in test-files (which have both unbalanced quotes and
**		"illegal" characters:
EOF/
$PROG test[12].c >$TMP
./show_diffs.sh $TMP normal.ref
#
cat <<EOF/
**
**	Case 2:	Suppressing unbalanced-quote:
EOF/
$PROG -qLEFT test[12].c >$TMP
./show_diffs.sh $TMP quotes.ref
#
cat <<EOF/
**
**	Case 3:	Counting by names given in standard-input:
EOF/
$PROG >$TMP <<EOF/
test1.c
test2.c
EOF/
./show_diffs.sh $TMP list.ref
#
cat <<EOF/
**
**	Case 4:	Counting bulk text piped in standard-input:
EOF/
cat test[12].c | $PROG -v - >$TMP
./show_diffs.sh $TMP cat.ref
#
cat <<EOF/
**
**	Case 5:	Counting history-comments
EOF/
$PROG test3.c >$TMP
./show_diffs.sh $TMP history.ref
#
cat <<EOF/
**
**	Case 6:	Display as a spreadsheet
EOF/
$PROG -t -qLEFT test[123].c >$TMP
./show_diffs.sh $TMP table.ref
#
cat <<EOF/
**
**	Case 7:	Display as a spreadsheet (per-file)
EOF/
$PROG -pt -qLEFT test[123].c >$TMP
./show_diffs.sh $TMP table_p.ref
#
#
rm -f $TMP
