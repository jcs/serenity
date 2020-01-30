SUBDIRS := $(patsubst %/Makefile,%/,$(wildcard */Makefile))

ifeq ($(HOSTED),1)
    SUBDIRS := $(filter-out DynamicLink/,$(SUBDIRS))
endif

include ../Makefile.subdir
