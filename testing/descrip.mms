# $Id: descrip.mms,v 3.0 1990/05/11 10:12:14 ste_cm Rel $
# mms-file for testing LINCNT
#

.INCLUDE PORTUNIX_ROOT:[SUPPORT]LIBRARY_RULES

####### (Development) ##########################################################

####### (Standard Lists) #######################################################
SCRIPTS	=\
	run_tests.sh	run_tests.com

REF_FILES = \
	cat.ref \
	list.ref \
	normal.ref \
	quotes.ref

TST_FILES = \
	test1.c \
	test2.c

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
