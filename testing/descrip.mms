# $Id: descrip.mms,v 5.0 1990/08/30 08:05:35 ste_cm Rel $
# mms-file for testing LINCNT
#

.INCLUDE PORTUNIX_ROOT:[SUPPORT]LIBRARY_RULES

####### (Development) ##########################################################

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

RUN_TESTS :	$(SCRIPTS)
	@run_tests

####### (Details of Productions) ###############################################
