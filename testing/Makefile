# $Id: Makefile,v 5.0 1990/08/30 08:05:08 ste_cm Rel $
# make-file for testing LINCNT
#

####### (Development) ##########################################################
GET	= checkout
DATE	= @ echo '**** '`date` >>$@

####### (Standard Lists) #######################################################
SCRIPTS	=\
	run_tests.sh	run_tests.com\
	show_diffs.sh

REF_FILES = \
	cat.ref \
	history.ref \
	list.ref \
	normal.ref \
	quotes.ref

TST_FILES = \
	test1.c \
	test2.c \
	test3.c

SOURCES	= Makefile descrip.mms README $(SCRIPTS) $(REF_FILES) $(TST_FILES)

####### (Standard Productions) #################################################
all:		sources		; @echo 'Type "make run_tests"'
clean:				; rm -f *.out
clobber:	clean
destroy:
	sh -c 'for i in *;do case $$i in RCS);; *) rm -f $$i;;esac;done;exit 0'
sources:	$(SOURCES)
lint.out:
lincnt.out:
run_tests:	$(SOURCES)	; sh -c '$@.sh 2>&1 | tee -a $@.out'

####### (Details of Productions) ###############################################
$(SOURCES):			; $(GET) $@
