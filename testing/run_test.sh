: '$Id: run_test.sh,v 1.1 1989/10/02 09:39:50 dickey Exp $'
#
PROG=../bin/lincnt
TMP=/tmp/lincnt$$
rm -f $TMP
#
cat <<EOF/
**
**	Count lines in test-files (which have both unbalanced quotes and
**	"illegal" characters:
EOF/
$PROG test[12].c >$TMP
cat  $TMP
diff $TMP normal.ref
#
cat <<EOF/
**
**	Suppressing unbalanced-quote:
EOF/
$PROG -qLEFT test[12].c >$TMP
cat  $TMP
diff $TMP quotes.ref
#
cat <<EOF/
**
**	Counting by names given in standard-input:
EOF/
$PROG >$TMP <<EOF/
test1.c
test2.c
EOF/
cat  $TMP
diff $TMP list.ref
#
cat <<EOF/
**
**	Counting bulk text piped in standard-input:
EOF/
cat test[12].c | $PROG -v - >$TMP
cat  $TMP
diff $TMP cat.ref
#
rm -f $TMP
