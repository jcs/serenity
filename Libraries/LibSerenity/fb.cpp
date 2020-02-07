#ifdef __OpenBSD__
#include "FB.h"
#include <AK/Assertions.h>
#include <AK/LogStream.h>
#include <sys/cdefs.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <dev/wscons/wsconsio.h>

/* XXX yuck - this doesn't live in /usr/include though */
#include "/usr/src/sys/dev/pci/drm/include/uapi/drm/i915_drm.h"

static struct {
    int width;
    int height;
    int pitch;
    size_t size;
    uint64_t res_conn_buf[10];
} drm_screen;

#define MAX_FBS 2

static struct openbsd_fb {
    void *fb;
    int crtc_id;
    struct drm_mode_crtc crtc;
} drm_fbs[MAX_FBS];

#define DRM_DEVICE "/dev/drm0"

int fb_create_buffers(int fd);

int fb_get_size_in_bytes(int fd, size_t* out)
{
    if (!drm_fbs[0].fb)
        fb_create_buffers(fd);

    *out = drm_screen.size;

    return 0;
}

int fb_get_resolution(int fd, FBResolution* info)
{
    if (!drm_fbs[0].fb)
        fb_create_buffers(fd);

    info->pitch = drm_screen.pitch;
    info->width = drm_screen.width;
    info->height = drm_screen.height;
    return 0;
}

int fb_set_resolution(int fd, FBResolution* info)
{
    // we can't actually change the resolution, so just keep it at what it's at
    if (fb_get_resolution(fd, info) != 0)
        return -1;

    // but take this opportunity to put wscons into dumb (non-text) mode to setup for mmap
    int mode = WSDISPLAYIO_MODE_DUMBFB;
    if (ioctl(fd, WSDISPLAYIO_SMODE, &mode) == -1) {
        perror("WSDISPLAYIO_SMODE");
        return -1;
    }

    return 0;
}

void *fb_get_addr(int fd, int index)
{
    if (!drm_fbs[0].fb)
        fb_create_buffers(fd);

    return drm_fbs[index].fb;
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
    struct drm_mode_crtc_page_flip flip;

    if (index >= MAX_FBS)
        return -1;

    if (!drm_fbs[index].fb)
        fb_create_buffers(fd);

    memset(&flip, 0, sizeof(flip));
    flip.fb_id = drm_fbs[index].crtc.fb_id;
    flip.crtc_id = drm_fbs[index].crtc.crtc_id;
    flip.user_data = (u64)drm_fbs[index].fb;
    // TODO: check for DRM_CAP_ASYNC_PAGE_FLIP and use DRM_MODE_PAGE_FLIP_ASYNC
    flip.flags = 0;

    if (ioctl(fd, DRM_IOCTL_MODE_PAGE_FLIP, &flip) != 0) {
        if (errno != EBUSY) {
            perror("DRM_IOCTL_MODE_PAGE_FLIP");
            return -1;
        }
    }

    return 0;
}

int fb_create_buffers(int fd)
{
    int ret = -1;

    memset(&drm_screen, 0, sizeof(drm_screen));
    memset(&drm_fbs, 0, sizeof(drm_fbs));

    if (ioctl(fd, DRM_IOCTL_SET_MASTER, 0) != 0) {
        perror("DRM_IOCTL_SET_MASTER");
        return -1;
    }

    struct drm_mode_card_res res;
    memset(&res, 0, sizeof(res));
    if (ioctl(fd, DRM_IOCTL_MODE_GETRESOURCES, &res) != 0) {
        perror("DRM_IOCTL_MODE_GETRESOURCES");
        return -1;
    }

    for (auto i = 0; i < (int)res.count_connectors; i++) {
        uint64_t res_fb_buf[10] = { 0 }, res_crtc_buf[10] = { 0 }, res_enc_buf[10] = { 0 };
        res.fb_id_ptr = (u64)res_fb_buf;
        res.crtc_id_ptr = (u64)res_crtc_buf;
        res.connector_id_ptr = (u64)drm_screen.res_conn_buf;
        res.encoder_id_ptr = (u64)res_enc_buf;
        if (ioctl(fd, DRM_IOCTL_MODE_GETRESOURCES, &res) != 0) {
            perror("DRM_IOCTL_MODE_GETRESOURCES");
            return -1;
        }

        struct drm_mode_modeinfo conn_mode_buf[20];
        uint64_t conn_prop_buf[20] = { 0 }, conn_propval_buf[20] = { 0 }, conn_enc_buf[20] = { 0 };
        struct drm_mode_get_connector conn;
        memset(&conn_mode_buf, 0, sizeof(conn_mode_buf));
        memset(&conn, 0, sizeof(conn));
        conn.connector_id = (u64)drm_screen.res_conn_buf[i];
        if (ioctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn) != 0) {
            perror("DRM_IOCTL_MODE_GETCONNECTOR");
            continue;
        }

        conn.modes_ptr = (u64)conn_mode_buf;
        conn.props_ptr = (u64)conn_prop_buf;
        conn.prop_values_ptr = (u64)conn_propval_buf;
        conn.encoders_ptr = (u64)conn_enc_buf;
        if (ioctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn) != 0) {
            perror("DRM_IOCTL_MODE_GETCONNECTOR");
            continue;
        }

        if (conn.count_encoders < 1 || conn.count_modes < 1 || !conn.encoder_id || !conn.connection)
            continue;

        for (auto j = 0; j <= 1; j++) {
            struct drm_mode_create_dumb create_dumb;
            struct drm_mode_map_dumb map_dumb;
            struct drm_mode_fb_cmd cmd_dumb;

            memset(&create_dumb, 0, sizeof(create_dumb));

            create_dumb.width = conn_mode_buf[0].hdisplay;
            create_dumb.height = conn_mode_buf[0].vdisplay;
            create_dumb.bpp = 32;
            if (ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb) != 0) {
                perror("DRM_IOCTL_MODE_CREATE_DUMB");
                goto bail;
            }

            cmd_dumb.width = create_dumb.width;
            cmd_dumb.height = create_dumb.height;
            cmd_dumb.bpp = create_dumb.bpp;
            cmd_dumb.pitch = create_dumb.pitch;
            cmd_dumb.depth = create_dumb.bpp;
            cmd_dumb.handle = create_dumb.handle;
            if (ioctl(fd, DRM_IOCTL_MODE_ADDFB, &cmd_dumb) != 0) {
                perror("DRM_IOCTL_MODE_ADDFB");
                goto bail;
            }

            map_dumb.handle = create_dumb.handle;
            if (ioctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map_dumb) != 0) {
                perror("DRM_IOCTL_MODE_MAP_DUMB");
                goto bail;
            }

            drm_screen.width = create_dumb.width;
            drm_screen.height = create_dumb.height;
            drm_screen.pitch = create_dumb.pitch;
            drm_screen.size = create_dumb.size;

            drm_fbs[j].fb = mmap(nullptr, drm_screen.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, map_dumb.offset);

            struct drm_mode_get_encoder enc;
            enc.encoder_id = conn.encoder_id;
            if (ioctl(fd, DRM_IOCTL_MODE_GETENCODER, &enc) != 0) {
                perror("DRM_IOCTL_MODE_GETENCODER");
                munmap(drm_fbs[j].fb, drm_screen.size);
                drm_fbs[j].fb = nullptr;
                goto bail;
            }

            drm_fbs[j].crtc.crtc_id = enc.crtc_id;
            if (ioctl(fd, DRM_IOCTL_MODE_GETCRTC, &drm_fbs[j].crtc) != 0) {
                perror("DRM_IOCTL_MODE_GETCRTC");
                munmap(drm_fbs[j].fb, drm_screen.size);
                drm_fbs[j].fb = nullptr;
                goto bail;
            }

            drm_fbs[j].crtc.fb_id = cmd_dumb.fb_id;
            drm_fbs[j].crtc.set_connectors_ptr = (uint64_t)&drm_screen.res_conn_buf[i];
            drm_fbs[j].crtc.count_connectors = 1;
            drm_fbs[j].crtc.mode = conn_mode_buf[0];
            drm_fbs[j].crtc.mode_valid = 1;
        }

        // we only need one set of buffers
        ret = 0;
        break;
    }

    if (ioctl(fd, DRM_IOCTL_MODE_SETCRTC, &drm_fbs[0].crtc) != 0)
        perror("DRM_IOCTL_MODE_SETCRTC");

bail:
    if (ioctl(fd, DRM_IOCTL_DROP_MASTER, 0) != 0) {
        perror("DRM_IOCTL_DROP_MASTER");
        ASSERT_NOT_REACHED();
    }

    return ret;
}
#endif
