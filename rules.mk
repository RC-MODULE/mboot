#
# This Makefile is to be included in board's main Makefile. It defines rules_all
# and rules_clean based the following variables:
#
# Cross-compiler prefix, like in Linux kernel
ifndef CROSS_COMPILE
$(error CROSS_COMPILE is not set in user`s environment)
endif
# Space-separated list of all *o files, which are board-specific. It should
# include all arch- and cpu-specific code.
ifndef BOARD_OBJS
$(error BOARD_OBJS is not set in user`s environment)
endif
# Per-board config.h containing various CONFIG_* defines
ifndef BOARD_CONFIG
$(error BOARD_CONFIG is not set in Makefile.board)
endif
# Per-board linker script
ifndef BOARD_LDSCRIPT
$(error BOARD_LDSCRIPT is not set in Makefile.board)
endif
# Per-board .text offset
ifndef BOARD_TEXT_BASE
$(error BOARD_TEXT_BASE is not set in Makefile.board)
endif
# The name of *elf file
ifndef BOARD_ELF
$(error BOARD_ELF is not set in Makefile.board)
endif
# The name of *bin file
ifndef BOARD_BIN
BOARD_BIN = $(BOARD_ELF).bin
endif

.PHONY: rules_all rules_clean

rules_all: $(BOARD_ELF) $(BOARD_BIN)

rules_clean:
	-@rm $(BOARD_ELF) $(BOARD_ELF).config $(BOARD_BIN) >/dev/null
	-@find -name '*\.o' -exec rm '{}' ';'
	-@find -name '*\.t' -exec rm '{}' ';'
	-@find -name '*\.d' -exec rm '{}' ';'

rules.mk: $(BOARD_ELF).config $(shell find arch board common drivers net lib fs -name Makefile)

$(BOARD_ELF).config: $(BOARD_CONFIG)
	$(CPP) -Iinclude -DDO_DEPS_ONLY -dM $(BOARD_CONFIG) | \
	    sed -n -f tools/scripts/define2mk.sed > $@

include $(BOARD_ELF).config

VPATH = $(shell find common drivers net lib fs -type d)

COBJS-y += $(BOARD_OBJS)
include common/Makefile
include drivers/Makefile
include net/Makefile
include lib/Makefile

AS = $(CROSS_COMPILE)as
CC = $(CROSS_COMPILE)gcc
CPP = $(CROSS_COMPILE)cpp
LD = $(CROSS_COMPILE)ld
AR = $(CROSS_COMPILE)ar
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

CTAGS = ctags

CPPFLAGS = \
	$(BOARD_CPPFLAGS) \
	-DTEXT_BASE=$(BOARD_TEXT_BASE) \
	-DMBOOT_VERSION="\"mboot$(shell ./tools/setlocalversion)\"" \
	-DMBOOT_DATE="\"$(shell LANG=C date)\"" \
	-D__KERNEL__ \
	-nostdinc \
	-include $(BOARD_CONFIG) \
	-isystem $(shell $(CC) -print-file-name=include) \
	-I include

CFLAGS = \
	$(BOARD_CFLAGS) \
	-fno-common \
	-fno-builtin \
	-ffreestanding \
	-Wall \
	-Werror \
	-g \
	-O0

ASFLAGS = $(CFLAGS)

LDFLAGS = \
	$(BOARD_LDFLAGS) \
	-L $(shell dirname `$(CC) $(CFLAGS) -print-libgcc-file-name`) \
	-lgcc

DEPS = $(COBJS-y:.o=.d)

rules.mk: $(DEPS)

%.d: %.S
	$(CC) -M $(CFLAGS) $(CPPFLAGS) $< \
		| sed 's@\(.*\)\.o[ :]*@$(@:.d=.o) $@ : @g' > $@; \
        [ -s $@ ] || rm -f $@

%.d: %.c
	$(CC) -M $(CFLAGS) $(CPPFLAGS) $< \
		| sed 's@\(.*\)\.o[ :]*@$(@:.d=.o) $@ : @g' > $@; \
        [ -s $@ ] || rm -f $@

tags: $(DEPS)
	cat $^ | sed 's/^.*://g' | sed 's/[\\]//g' | sed 's/ \+/\n/g' | sort -u | ctags -L -

-include $(DEPS)

$(BOARD_ELF): $(COBJS-y)
	$(LD) -T $(BOARD_LDSCRIPT) -Ttext=$(BOARD_TEXT_BASE) \
		--start-group $^ --end-group $(LDFLAGS) -o $@

$(BOARD_BIN): $(BOARD_ELF)
	$(OBJCOPY) -v -O binary $< $@

