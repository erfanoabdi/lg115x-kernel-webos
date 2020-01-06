
libs += liblogs.so
ver_liblogs.so := 0.0
objs_liblogs.so += log.o
objs_liblogs.so += logm.o

headers_liblogs.so += .:log.h
headers_liblogs.so += .:logm.h

CFLAGS_liblogs.so += -I$(src)/../../include

ifeq ($(USE_BIONIC_LIBC), YES)
LDFLAGS_liblogs.so +=
else
LDFLAGS_liblogs.so += -lpthread
endif

apps += logpiped
objs_logpiped += logpiped.o
LDFLAGS_logpiped += -llogs

apps += test_logfunnel
objs_test_logfunnel += test.o
LDFLAGS_test_logfunnel += -llogs

#apps += buffer2text

ifeq ($(src),)
$(if $(MAKECMDGOALS),$(MAKECMDGOALS),all):
	$(MAKE) -C ../../../../ -f buildrule/root.mk USEMAKEFILE=$(shell pwd)/a.mk $@
endif

