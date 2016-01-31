# -*-Makefile-*-

OPT_LEVEL=0
DEBUG_ENABLED=yes

SED_SCRIPT='s,[^0-9a-zA-Z],_,g'
UNAME_S=$(shell uname -s)
UNAME_R=$(shell uname -r)
OSBRAND=$(shell echo $(UNAME_S) | sed $(SED_SCRIPT))
OS_REV=$(shell echo $(UNAME_S)_$(UNAME_R) | sed $(SED_SCRIPT))

include $(wildcard Makefile.$(OSBRAND) Makefile.$(OS_REV))

# Do NOT export COMPILER_PATH as you will get weirdo errors like this that will take you eons to triage:
#   /usr/bin/c++     -fPIC -Wall -Werror -Wsynth -Wno-comment -Wreturn-type cycledetector.cpp -c -o cycledetector.o
#   In file included from /usr/include/boost/config/platform/linux.hpp:14,
#                    from /usr/include/boost/config.hpp:53,
#                    from cycledetector.cpp:8:
#   /usr/include/c++/4.3/cstdlib:73:25: error: stdlib.h: No such file or directory
#   
#### Don't do this!!! --> export COMPILER_PATH

CXXEXT=cpp
CEXT=c

#EXEC_FILE=mainc.exe
EXEC_FILE=main.exe
EXEC_FILE_OBJS=$(patsubst %.exe,%.o,$(EXEC_FILE))
EXEC_FILE_PREPROC_FILE=$(patsubst %.exe,%.E,$(EXEC_FILE))
EXEC_FILE_DEP_FILE=$(patsubst %.exe,%.d,$(EXEC_FILE))
EXEC_FILE_ASM_FILE=$(patsubst %.exe,%.s,$(EXEC_FILE))

ALL_OBJECTS=$(EXEC_FILE)

# Sigh. I had to add CC_FLAGS_EXTRA_PRE and CC_FLAGS_EXTRA_POST
# variables to be able to add -Wno-unknown-pragmas at the very end
# to undo the effects of -Wall:
CC_FLAGS = \
  $(CC_FLAGS_EXTRA_PRE) \
  $(CC_DEPS_FLAGS) \
  $(OPT_FLAGS)  \
  $(CXX_INC_SPEC) \
  $(POSITION_INDEP_FLAGS) \
  $(EXCEPTION_HANDLING_POLICY) \
  $(CXX_ERROR_SPEC) \
  $(CXX_MISC_FLAGS) \
  $(CC_FLAGS_EXTRA_POST)

CC_DUMP_ASM_COMMAND   = $(CXX_PATH) $(CC_FLAGS) -S -dA -dp -fverbose-asm $< 

# for doing tracebacks:
#TRACEBACK_LIB=-lbfd -liberty
TRACEBACK_LIB=

EXTRA_CXX_POSTO_LINK_FLAGS = 

STRIP_LINK_CMD = 
ifdef DO_SYMBOL_STRIPPING
  STRIP_LINK_CMD = && strip --strip-all $@ && file $@
endif

CC_LINK_BASE = $(CXX_PATH) \
  $(CC_FLAGS) \
  $^ \
  -L. \
  $(CXX_LINK_SPEC) \
  $(EXTRA_CXX_PREO_LINK_FLAGS) \
  -o $@ \
  $(EXTRA_CXX_POSTO_LINK_FLAGS) \
  $(TRACEBACK_LIB)

CC_EXEC_LINK_COMMAND = $(CC_LINK_BASE)

CC_SLIB_LINK_COMMAND = $(CC_LINK_BASE) \
  -shared

CC_COMPILE_COMMAND    = $(CXX_PATH) $(CC_FLAGS) $< -c -o $@

C_LINK_COMMAND       = $(CXX_PATH) $(CC_FLAGS) $< -c -o $@

all: $(ALL_OBJECTS)

.PHONY: run
run: $(ALL_OBJECTS)
	time $(EXEC_FILE) $(RUN_ARGUMENTS)

runrpath: $(ALL_OBJECTS)
	mkdir -p rpath_test_subdir; cd rpath_test_subdir; ../$(EXEC_FILE) $(RUN_ARGUMENTS)

runnohup: $(ALL_OBJECTS)
	sh -c "nohup $(EXEC_FILE) $(RUN_ARGUMENTS) </dev/null >nohup.out 2>&1 &"; sleep 2; ps -ef --forest --cols=3000 ; sleep 5

ldd: $(ALL_OBJECTS)
	ldd -v $(EXEC_FILE)

stdinFile=$(wildcard ./stdinFile)

ifeq "$(stdinFile)" ""
runstdin: $(EXEC_FILE)
	$(EXEC_FILE)
else
runstdin: $(EXEC_FILE)
	$(EXEC_FILE) <$(stdinFile)
endif

runexec: $(EXEC_FILE)
	$(EXEC_FILE)

rungdb: $(ALL_OBJECTS)
	gdb $(EXEC_FILE)

# run this from a shell, not from emacs compile mode -- its interactive
runadb: $(ALL_OBJECTS)
	adb -P adb $(EXEC_FILE)

$(EXEC_FILE) : $(EXEC_FILE_OBJS)
	$(SILENT_COMPILE)$(CC_EXEC_LINK_COMMAND) $(STRIP_LINK_CMD)

################################################################################
# The main compile rule:
#
# Using tips in http://stackoverflow.com/a/8027542 we define:
%.o : %.$(CXXEXT)
	$(SILENT_COMPILE)$(CC_COMPILE_COMMAND)
	@if [ -f $*.d ]; then \
          cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	  	-e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d; \
	fi
# Now conditionally include the generated .P file
-include *.P
################################################################################





%.s : %.$(CXXEXT)
	$(SILENT_COMPILE)$(CC_DUMP_ASM_COMMAND)

clean: 
	rm -f $(EXEC_FILE_DEP_FILE) $(ALL_OBJECTS) *.o $(EXEC_FILE_ASM_FILE) *.d *.P

%.E : %.$(CXXEXT)
	CC_FLAGS_EXTRA_PRE="-E"; export CC_FLAGS_EXTRA_PRE; \
		rm -f $(basename $(EXEC_FILE)).o; \
		$(MAKE) $(basename $(EXEC_FILE)).o && \
		mv $(basename $(EXEC_FILE)).o $@

preproc : $(EXEC_FILE_PREPROC_FILE)

dump_asm: $(EXEC_FILE_ASM_FILE)

ver:
	@$(CC_COMPILE_COMMAND) --version
