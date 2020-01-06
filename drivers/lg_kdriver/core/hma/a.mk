
#
# hma library description
#
libs += libhma.so
ver_libhma.so := 0.0
objs_libhma.so += libhma.o
#LDFLAGS_libhma.so += -llogs

headers_libhma.so += .:libhma.h

#libs += libhma.a
#objs_libhma.a += libhma.o

apps += test_hma
objs_test_hma += test.o
LDFLAGS_test_hma += -lhma


$(eval $(call build_doxygen_document,hma))

