OBJS = \
    ProcessModel.o \
    DevicesModel.o \
    ProcessTableView.o \
    MemoryStatsWidget.o \
    GraphWidget.o \
    ProcessStacksWidget.o \
    ProcessMemoryMapWidget.o \
    ProcessFileDescriptorMapWidget.o \
    NetworkStatisticsWidget.o \
    ProcessUnveiledPathsWidget.o \
    main.o

PROGRAM = SystemMonitor

LIB_DEPS = GUI Gfx Protocol PCIDB IPC Thread Pthread Core

ifeq ($(HOSTED),1)
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),OpenBSD)
        LDFLAGS += -lkvm
    endif
endif

include ../../Makefile.common
