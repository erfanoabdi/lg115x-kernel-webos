export KDRV_MODULE_BUILD

ifeq ($(KDRV_MODULE_BUILD), YES)
CONFIG_LG_BUILTIN_KDRIVER := m
endif

ifeq ($(KDRV_PLATFORM_CFG),m14_googletv)
EXTRA_CFLAGS=-fno-pic
endif

obj-$(CONFIG_LG_BUILTIN_KDRIVER) += hma.o
hma-objs += hma_alloc.o
hma-objs += hma_drv.o

