/*
 * Copyright (c) 2020 joshua stein <jcs@jcs.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <AK/Vector.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __OpenBSD__
#include <limits.h>
#include <sys/shm.h>
#include <sys/mman.h>

#define EMAXERRNO ELAST
#endif

extern "C" {
int create_shared_buffer(int size, void** buffer)
{
    int key, id = -1;

    for (key = 1; key < INT_MAX; key++) {
        id = shmget(key, size, IPC_CREAT | 0600);
        if (id != -1) {
            *buffer = shmat(id, 0, 0);
            if (*buffer == (void *)-1) {
                perror("shmat");
                ASSERT_NOT_REACHED();
            }
            break;
        }
    }

    if (id == -1) {
        perror("shmget: exhausted");
        ASSERT_NOT_REACHED();
    }

    return key;
}

int share_buffer_with(__attribute__((unused)) int shared_buffer_id, __attribute__((unused)) pid_t peer_pid)
{
    /* TODO: use shmctl? */
    return 0;
}

int share_buffer_globally(__attribute__((unused)) int shared_buffer_id)
{
    /* XXX: use shmctl? */
    return 0;
}

int set_process_icon(__attribute__((unused)) int icon_id)
{
    /* TODO */
    return 0;
}

void* get_shared_buffer(int shared_buffer_id)
{
    int id = shmget(shared_buffer_id, 0, 0);
    if (id == -1) {
        errno = -id;
        return (void*)-1;
    }
    return shmat(id, 0, 0);
}

int release_shared_buffer(__attribute__((unused)) int shared_buffer_id)
{
    /* TODO */
    return 0;
}

int get_shared_buffer_size(__attribute__((unused)) int shared_buffer_id)
{
    /* TODO */
    return 0;
}

int seal_shared_buffer(__attribute__((unused)) int shared_buffer_id)
{
    /* TODO */
    return 0;
}

}
