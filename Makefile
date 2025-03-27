.PHONY: all lib mrproper clean testlib tests app doxygen cleandoxy valgrind

MAKE_OPTS=--no-print-directory
#MAKE_OPTS+=-s

DIRS=\
	lib \
	tests \
#

all: lib app tests

.PHONY: d t v

t:
	@$(MAKE) mrproper
	@$(MAKE)

app: lib
	@$(MAKE) $(MAKE_OPTS) -C app/

testlib:
	@$(MAKE) $(MAKE_OPTS) CBMIMAGE_TESTLIB=1 -C lib/

tests: testlib app
	@$(MAKE) $(MAKE_OPTS) CBMIMAGE_TESTLIB=1 -C tests/
	@$(MAKE) $(MAKE_OPTS) CBMIMAGE_TESTLIB=1 -C tests/ $@

lib:
	@$(MAKE) $(MAKE_OPTS) -C $@

cleandoxy:: $(wildcard output/doxygen/)
	@test -z "$^" || rm -r -- $^

mrproper:: $(wildcard *~) $(wildcard */*~) $(wildcard */*/*~) $(wildcard */*/*/*~)
	@test -z "$^" || rm -- $^

clean mrproper::
	@for A in $(DIRS); do $(MAKE) $(MAKE_OPTS) -C $$A $@; done
	@$(MAKE) $(MAKE_OPTS) CBMIMAGE_TESTLIB=1 -C lib $@

doxygen d: output/ cleandoxy
	doxygen make/Doxyfile

valgrind v:
	DO_VALGRIND=1 make

output/:
	@mkdir -p "$@"
