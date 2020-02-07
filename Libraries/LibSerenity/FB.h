#pragma once

#include <sys/cdefs.h>
#include <sys/types.h>

__BEGIN_DECLS

struct FBResolution {
    int pitch;
    int width;
    int height;
};

int fb_get_size_in_bytes(int, size_t*);
int fb_get_resolution(int, FBResolution*);
int fb_set_resolution(int, FBResolution*);
int fb_get_buffer(int, int*);
int fb_set_buffer(int, int);

#ifdef __OpenBSD__
void *fb_get_addr(int, int);
#endif

__END_DECLS
