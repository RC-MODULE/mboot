ifdef CONFIG_USB_ETHER
COBJS-y += ether.o epautoconf.o config.o usbstring.o
else
ifdef CONFIG_USB_DEVICE
COBJS-y += core.o
COBJS-y += ep0.o
COBJS-$(CONFIG_OMAP1510) += omap1510_udc.o
COBJS-$(CONFIG_OMAP1610) += omap1510_udc.o
COBJS-$(CONFIG_MPC885_FAMILY) += mpc8xx_udc.o
COBJS-$(CONFIG_PXA27X) += pxa27x_udc.o
COBJS-$(CONFIG_SPEARUDC) += spr_udc.o
endif
endif
