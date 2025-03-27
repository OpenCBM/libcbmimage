ifneq ("$(LIB)","")
all: lib dep
endif

ifneq ("$(EXE)","")
	$(error "Var EXE mustn't be set for lib builds")
endif

include $(RELATIVEPATH)/make/common.mk

lib: $(OUTPUTDIR)/$(EXEDIR)lib$(LIB)

$(OUTPUTDIR)/$(EXEDIR)lib$(LIB): $(OBJECTFILES)
	@ar rcs $@ $(OBJECTFILES)
