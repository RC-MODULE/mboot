COBJS-libfdt += fdt.o fdt_ro.o fdt_rw.o fdt_strerror.o fdt_sw.o fdt_wip.o

COBJS-$(CONFIG_OF_LIBFDT) += $(addprefix lib/libfdt/,$(COBJS-libfdt))
COBJS-$(CONFIG_FIT) += $(COBJS-libfdt)
