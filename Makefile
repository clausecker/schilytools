#
# This is only the bootstrap makefile.
# The 'real' makefile is called 'SMakefile'. It is used in the special
# case of compiling 'smake' to help to make the compiling as simple
# as possible. Smake first looks for 'SMakefile' and thus the 
# command 'psmake/smake $@' will use 'SMakefile' to read rules.
#
.PHONY: all clean clobber distclean install ibins depend rmdep config TAGS tags tests rmtarget relink

all man lint clean clobber distclean install installman ibins depend rmdep config TAGS tags tests rmtarget relink:
	@echo "NOTICE: Using bootstrap 'Makefile' to make '$@'"
	cd psmake && sh ./MAKE-all
	./psmake/smake -r $@
