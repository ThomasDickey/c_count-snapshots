$! $Id: run_test.dcl,v 2.0 1990/05/11 09:45:05 ste_cm Rel $
$	verify = F$VERIFY(0)
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
$ nomsg
$	set file/protection=(o:rwed) 'p1;*
$	delete 'p1;*
$ msg
$	set on
$	return
$ endsubroutine
