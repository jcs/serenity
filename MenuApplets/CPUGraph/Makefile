OBJS = main.o

PROGRAM = CPUGraph.MenuApplet

LIB_DEPS = GUI IPC Gfx Thread Pthread Core

ifeq ($(HOSTED),1)
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),OpenBSD)
        LDFLAGS += -lkvm
    endif
endif

include ../../Makefile.common
