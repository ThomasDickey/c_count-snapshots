$! $Id: run_test.dcl,v 3.0 1990/06/08 13:20:28 ste_cm Rel $
$	verify = F$VERIFY(0)
$	set := set
$	set symbol/scope=(nolocal,noglobal)
$	path = F$ENVIRONMENT("procedure")
$	path = F$EXTRACT(0,F$LOCATE("]",path)+1,path)
$	prog = "$ ''F$EXTRACT(0,F$LENGTH(path)-1,path)'.-.bin]lincnt"
$	temp = "sys$scratch:lincnt.tmp"
$	call remove 'temp
$
$	type sys$input
**
**	Count lines in test-files (which have both unbalanced quotes and
**	"illegal" characters:
$	eod
$
$	prog -o 'temp test1.c test2.c
$	call display normal
$
$	type sys$input
**
**	Suppressing unbalanced-quote:
$	eod
$
$	prog -o 'temp -q"LEFT" test1.c test2.c
$	call display quotes
$
$	type sys$input
**
**	Counting by names given in standard-input:
$	eod
$
$	prog -o 'temp
test1.c
test2.c
$	eod
$	call display list
$
$	type sys$input
**
**	Counting bulk text piped in standard-input:
$	eod
$
$	pipe = "sys$scratch:lincnt.tst"
$	call   remove  'pipe
$	copy   test1.c 'pipe
$	append test2.c 'pipe;
$
$	define/user_mode sys$input 'pipe
$	prog -o 'temp -v "-"
$! note the quoted '-' so that DCL didn't think it was continuation
$	call display cat
$
$	call remove  'pipe
$	verify = F$VERIFY(0)
$	exit
$
$ display: subroutine
$	type  'temp
$	test = "''path'''p1'.ref"
$	list = "sys$scratch:lincnt.dif"
$	diff/output='list 'temp 'test
$	if $severity .ne. 1 then type 'list
$	call remove 'temp
$	call remove 'list
$ endsubroutine
$
$ remove: subroutine
$	set noon
$	set message/noident/notext/nosever/nofacil
$	set file/protection=(o:rwed) 'p1;*
$	delete 'p1;*
$	set message/ident/text/sever/facil
$	set on
$	return
$ endsubroutine
