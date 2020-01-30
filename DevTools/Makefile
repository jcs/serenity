SUBDIRS := $(patsubst %/Makefile,%/,$(wildcard */Makefile))

ifeq ($(HOSTED),1)
    SUBDIRS := $(filter-out ProfileViewer/,$(SUBDIRS))
endif

include ../Makefile.subdir
