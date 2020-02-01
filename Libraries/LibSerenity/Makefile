AK_OBJS = \
    ../../AK/FileSystemPath.o \
    ../../AK/FlyString.o \
    ../../AK/JsonParser.o \
    ../../AK/JsonValue.o \
    ../../AK/LogStream.o \
    ../../AK/MappedFile.o \
    ../../AK/SharedBuffer.o \
    ../../AK/String.o \
    ../../AK/StringBuilder.o \
    ../../AK/StringImpl.o \
    ../../AK/StringUtils.o \
    ../../AK/StringView.o \
    ../../AK/URL.o \
    ../../AK/Utf8View.o

LIBC_OBJS = \
    unistd.o \
    serenity.o \
    mman.o \
    fcntl.o \
    pthread.o \
    fb.o \
    sched.o

OBJS = $(AK_OBJS) $(LIBC_OBJS)

LIBRARY = libserenity.a

all: $(LIBRARY)

include ../../Makefile.common
