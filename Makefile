SUBDIRS += \
	Libraries \
	AK \
	DevTools \
	Servers

SUBDIRS += \
	Applications \
	Kernel \
	MenuApplets \
	Shell \
	Userland

SUBDIRS += \
	Games \
	Demos

ifneq (, $(shell which ccache 2>/dev/null))
  export PRE_CXX=ccache
endif

ifeq ($(HOSTED),1)
    SUBDIRS := $(filter-out Kernel Userland,$(SUBDIRS))
endif

include Makefile.subdir

all: subdirs

.PHONY: test
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
test: 
else
test:
	$(QUIET) flock AK/Tests -c "$(MAKE) -C AK/Tests clean all && $(MAKE) -C AK/Tests clean"
endif

