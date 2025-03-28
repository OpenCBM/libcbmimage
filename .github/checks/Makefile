
ifneq ($(shell echo),)
  CMD_EXE = 1
endif

ifdef CMD_EXE

.PHONY: checkstyle

checkstyle:
	$(info INFO: style checks require bash.)

else

.PHONY: checkstyle lineendings lastline spaces noexec

checkstyle: lineendings lastline spaces noexec

lineendings: lineendings.sh
	@./lineendings.sh

lastline: lastline.sh
	@./lastline.sh

spaces: spaces.sh
	@./spaces.sh

noexec: noexec.sh
	@./noexec.sh

endif
