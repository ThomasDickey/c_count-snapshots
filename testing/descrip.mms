# $Id: descrip.mms,v 7.0 1993/05/05 14:44:13 ste_cm Rel $
# mms-file for testing LINCNT
#

.INCLUDE PORTUNIX_ROOT:[SUPPORT]LIBRARY_RULES

####### (Development) ##########################################################

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
.LAST:
	-remove -f *.dia;*

ALL :
	@ write sys$output "done with $@"

CLEAN :
	-remove -f *.log

CLOBBER :	CLEAN
	@ write sys$output "done with $@"
DESTROY :
	-remove -fv *.*;*

RUN_TEST :	$(SCRIPTS)
	@run_test

####### (Details of Productions) ###############################################
