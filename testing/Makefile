# $Id: Makefile,v 6.1 1992/10/19 08:35:46 dickey Exp $
# make-file for testing LINCNT

TOP	= ../..
include $(TOP)/cm_library/support/cm_library.mk

####### (Standard Lists) #######################################################
SCRIPTS	=\
	run_tests.sh	run_tests.com\
	show_diffs.sh

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
all:	$(SOURCES) $(ALL)	; @echo 'Type "make run_tests"'
clean:				; rm -f $(CLEAN)
clobber:	clean		; rm -f $(ALL)
destroy:			; $(DESTROY)
sources:	$(SOURCES)
lint.out:
lincnt.out:
run_tests:	$(SOURCES)	; $(RUN_TESTS)

####### (Details of Productions) ###############################################
$(SOURCES):			; $(GET) $@
