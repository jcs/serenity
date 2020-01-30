#pragma once

#include "/usr/include/sys/mman.h"

#define MAP_PURGEABLE 0x80

#define MADV_SET_VOLATILE 0x100
#define MADV_SET_NONVOLATILE 0x200
#define MADV_GET_VOLATILE 0x400

__BEGIN_DECLS

void* mmap_with_name(void* addr, size_t, int prot, int flags, int fd, off_t, const char* name);
int set_mmap_name(void*, size_t, const char*);

__END_DECLS
