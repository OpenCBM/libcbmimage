LIB=cbmimage.a

RELATIVEPATH=../

HEADERFILES=$(wildcard *.h)
SOURCEFILES=$(wildcard *.c)

include $(RELATIVEPATH)/make/lib.mk

.PHONY: d t v

d v t:
	@$(MAKE) -C .. $@
