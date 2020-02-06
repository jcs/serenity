OBJS = \
    Project.o \
    ProjectFile.o \
    TerminalWrapper.o \
    FindInFilesWidget.o \
    ProcessStateWidget.o \
    FormEditorWidget.o \
    FormWidget.o \
    Editor.o \
    EditorWrapper.o \
    Locator.o \
    Tool.o \
    CursorTool.o \
    WidgetTool.o \
    WidgetTreeModel.o \
    main.o

PROGRAM = HackStudio

LIB_DEPS = GUI Web VT Protocol Markdown Gfx IPC Thread Pthread Core JS

ifeq ($(HOSTED),1)
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),OpenBSD)
        LDFLAGS += -lkvm
    endif
endif

include ../../Makefile.common
