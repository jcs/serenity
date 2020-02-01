#ifdef __OpenBSD__
#include "FB.h"
#include <AK/LogStream.h>
#include <sys/cdefs.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <dev/wscons/wsconsio.h>

int fb_get_size_in_bytes(int fd, size_t* out)
{
    struct wsdisplay_fbinfo wsinfo;
    unsigned int linebytes;

    if (ioctl(fd, WSDISPLAYIO_LINEBYTES, &linebytes) == -1) {
    	perror("WSDISPLAYIO_LINEBYTES failed");
        return -1;
    }

    if (ioctl(fd, WSDISPLAYIO_GINFO, &wsinfo) == -1) {
    	perror("WSDISPLAYIO_GINFO failed");
        return -1;
    }

    switch (wsinfo.depth) {
    case 1:
    case 4:
    case 8:
        *out = linebytes * wsinfo.height;
        break;
    case 16:
        if (linebytes == wsinfo.width)
            *out = wsinfo.width * wsinfo.height * sizeof(short);
        else
            *out = linebytes * wsinfo.height;
        break;
    case 24:
        if (linebytes == wsinfo.width)
            *out = wsinfo.width * wsinfo.height * 3;
        else
            *out = linebytes * wsinfo.height;
        break;
    case 32:
        if (linebytes == wsinfo.width)
            *out = wsinfo.width * wsinfo.height * sizeof(int);
        else
            *out = linebytes * wsinfo.height;
        break;
    default:
        dbg() << "unsupported depth " << wsinfo.depth;
        perror("unsupported depth");
        return -1;
    }

    return 0;
}

int fb_get_resolution(int fd, FBResolution* info)
{
    struct wsdisplay_fbinfo wsinfo;
    unsigned int linebytes;

    if (ioctl(fd, WSDISPLAYIO_LINEBYTES, &linebytes) == -1) {
    	perror("WSDISPLAYIO_LINEBYTES failed");
        return -1;
    }

    if (ioctl(fd, WSDISPLAYIO_GINFO, &wsinfo) == -1)
        return -1;

    info->pitch = linebytes;
    info->width = wsinfo.width;
    info->height = wsinfo.height;
    return 0;
}

int fb_set_resolution(int fd, FBResolution* info)
{
    // we can't actually change the resolution, so just keep it at what it's at
    if (fb_get_resolution(fd, info) != 0)
        return -1;

    // but take this opportunity to put wscons into dumb (non-text) mode to setup for mmap
    int mode = WSDISPLAYIO_MODE_DUMBFB;
    if (ioctl(fd, WSDISPLAYIO_SMODE, &mode) == -1)
        return -1;

    return 0;
}

int fb_get_buffer(int fd, int* index)
{
    dbg() << "fb_get_buffer not supported";
    (void)fd;
    (void)index;
    return -1;
}

int fb_set_buffer(int fd, int index)
{
    dbg() << "fb_set_buffer not supported";
    (void)fd;
    (void)index;
    return -1;
}
#endif
