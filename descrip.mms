# $Id: descrip.mms,v 7.1 1994/07/18 01:27:43 tom Exp $
# MMS script for C-LineCount program.
#
PROGRAM	= C_COUNT
.INCLUDE PORTUNIX_ROOT:[SUPPORT]PROGRAM_RULES
[-.BIN]$(PROGRAM).OBJ : $(I)PORTUNIX.H
