# $Id: Makefile,v 7.0 1993/04/28 09:40:00 ste_cm Rel $
# make-file for testing LINCNT

TOP	= ../..
include $(TOP)/td_lib/support/td_lib.mk

####### (Standard Lists) #######################################################
SCRIPTS	=\
	run_test.sh	run_test.com\
	showdiff.sh

REF_FILES = \
	cat.ref \
	history.ref \
	list.ref \
	normal.ref \
	quotes.ref \
	table.ref \
	table_p.ref

TST_FILES = \
	test1.c \
	test2.c \
	test3.c

SOURCES	= Makefile descrip.mms README $(SCRIPTS) $(REF_FILES) $(TST_FILES)

####### (Standard Productions) #################################################
all:	$(SOURCES) $(ALL)	; @echo 'Type "make run_test"'
clean:				; rm -f $(CLEAN)
clobber:	clean		; rm -f $(ALL)
destroy:			; $(DESTROY)
sources:	$(SOURCES)
lint.out:
lincnt.out:
run_test:	$(SOURCES)	; $(RUN_TESTS)

####### (Details of Productions) ###############################################
$(SOURCES):			; $(GET) $@
