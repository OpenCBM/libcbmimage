SUBDIRS=$(wildcard */)

SCRIPTS=$(wildcard *.sh)

ifneq ("$(LIB)","")
	$(error "Var LIB mustn't be set for test builds")
endif

ifneq ("$(EXE)","")
all:: exe dep $(SUBDIRS)
else
  ifneq ("$(EXEFILES)","")
all:: $(SUBDIRS) scripts
  endif
endif


include $(RELATIVEPATH)/make/common.mk

exe: $(OUTPUTDIR)/$(EXEDIR)$(EXE)

$(OUTPUTDIR)/$(EXEDIR)$(EXE):: $(EXE).c
	@$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)


clean mrproper:: $(SUBDIRS)

.PHONY: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: scripts $(SCRIPTS)
scripts: $(SCRIPTS)

$(SCRIPTS):
	./$@
