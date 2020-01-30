#pragma once

#include "/usr/include/fcntl.h"

__BEGIN_DECLS

int creat_with_path_length(const char* path, size_t path_length, mode_t);
int open_with_path_length(const char* path, size_t path_length, int options, mode_t);
#define AT_FDCWD -100
int openat_with_path_length(int dirfd, const char* path, size_t path_length, int options, mode_t);

int watch_file(const char* path, size_t path_length);

__END_DECLS
