#!/bin/sh
# $Id: run_test.sh,v 5.0 1990/08/30 08:02:54 ste_cm Rel $
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
rm -f $TMP
