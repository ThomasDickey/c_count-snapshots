# $Id: descrip.mms,v 7.0 1990/05/10 14:44:38 ste_cm Rel $
# MMS script for C-LineCount program.
#
PROGRAM	= LINCNT
.INCLUDE PORTUNIX_ROOT:[SUPPORT]PROGRAM_RULES
[-.BIN]$(PROGRAM).OBJ : $(I)PORTUNIX.H
