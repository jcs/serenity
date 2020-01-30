SUBDIRS := $(patsubst %/Makefile,%/,$(wildcard */Makefile))

ifeq ($(HOSTED),1)
    SUBDIRS := $(filter-out LibC/ LibM/ LibPthread/,$(SUBDIRS))
endif

include ../Makefile.subdir
