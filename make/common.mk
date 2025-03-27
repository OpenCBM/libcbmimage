ifeq ("$(CBMIMAGE_TESTLIB)","")
  OUTPUTDIR=$(RELATIVEPATH)output
  EXEDIR=
  ifeq ("$(EXE)","")
    EXEDIR=
  else
    EXEDIR=$(EXE)/
  endif
  LDFLAGS+=-L$(OUTPUTDIR) -lcbmimage
  CFLAGS+=-I$(RELATIVEPATH)/lib
else
  OUTPUTDIR=$(RELATIVEPATH)output/testlib
  CFLAGS += -DCBMIMAGE_TESTLIB=$(CBMIMAGE_TESTLIB)
  ifeq ("$(EXE)","")
    EXEDIR=
  else
    EXEDIR=$(EXE)/
  endif
  LDFLAGS+=-L$(OUTPUTDIR) -lcbmimage

endif

DEPDIR=$(OUTPUTDIR)/$(EXEDIR).dep/


CFLAGS += -D_FORTIFY_SOURCE=2 -fstack-protector -I$(RELATIVEPATH)/include -I$(RELATIVEPATH)/lib

#CFLAGS_DEP = -MG -MP additionally?
CFLAGS_DEP = -MM -MT $(OUTPUTDIR)/$(@:.d=.o) -MT $(DEPDIR)$@ -MF $(DEPDIR)$@ $(CFLAGS)

# for debugging purposes:
ifneq ($(CBMIMAGE_DUMP_DEBUG),)
CFLAGS += -ggdb -O0
endif

.PHONY: all lib mrproper clean testdir tests

OBJECTFILES=$(patsubst %,$(OUTPUTDIR)/$(EXEDIR)%,$(patsubst %.c,%.o,$(SOURCEFILES)))

OBJECTFILES_DEP=$(patsubst %.c,%.d,$(SOURCEFILES))

ifneq ("$(EXEFILES)","")

all tests dep clean mrproper::
	@for A in $(EXEFILES); do $(MAKE) EXEFILES= EXE=$$A $(MAKECMDGOALS); done

else

$(SOURCEFILES): $(OUTPUTDIR) $(OUTPUTDIR)/$(EXEDIR)

clean mrproper:: $(wildcard $(OUTPUTDIR)/)
	@test -z "$^" || rm -r -- $^

mrproper:: $(wildcard */*/*~) $(wildcard */*~) $(wildcard *~)
	@test -z "$^" || rm -- $^

$(DEPDIR) $(OUTPUTDIR) $(OUTPUTDIR)/$(EXEDIR)::
	@mkdir -p "$@"

%.d: %.c $(DEPDIR)
	@$(CC) $(CFLAGS_DEP) $< $(LDFLAGS)

$(OUTPUTDIR)/$(EXEDIR)%.o: %.c
	@$(CC) -c $(CFLAGS) -o $@ $< $(LDFLAGS)

tests: $(OUTPUTDIR)/$(EXEDIR)$(EXE)
	@echo Executing test $(OUTPUTDIR)/$(EXEDIR)$(EXE)
	@$(RELATIVEPATH)make/perform_test "$(shell pwd)/${EXE}.c" $(OUTPUTDIR)/$(EXEDIR)$(EXE)

dep: $(OBJECTFILES_DEP) $(DEPDIR)

ifneq ("$(wildcard $(DEPDIR)*.d)","")
include $(wildcard $(DEPDIR)*.d)
endif

endif
