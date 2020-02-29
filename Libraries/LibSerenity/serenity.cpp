/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef __serenity__
#include <Kernel/Syscall.h>
#endif
#include <errno.h>
#include <serenity.h>

#ifdef __OpenBSD__
#include <AK/Assertions.h>
#include <err.h>
#include <limits.h>
#include <sys/shm.h>
#include <sys/mman.h>

#define EMAXERRNO ELAST
#endif

extern "C" {

#ifdef __serenity__
int module_load(const char* path, size_t path_length)
{
    int rc = syscall(SC_module_load, path, path_length);
    __RETURN_WITH_ERRNO(rc, rc, -1);
}

int module_unload(const char* name, size_t name_length)
{
    int rc = syscall(SC_module_unload, name, name_length);
    __RETURN_WITH_ERRNO(rc, rc, -1);
}

int profiling_enable(pid_t pid)
{
    int rc = syscall(SC_profiling_enable, pid);
    __RETURN_WITH_ERRNO(rc, rc, -1);
}

int profiling_disable(pid_t pid)
{
    int rc = syscall(SC_profiling_disable, pid);
    __RETURN_WITH_ERRNO(rc, rc, -1);
}

int set_thread_boost(int tid, int amount)
{
    int rc = syscall(SC_set_thread_boost, tid, amount);
    __RETURN_WITH_ERRNO(rc, rc, -1);
}

int set_process_boost(int tid, int amount)
{
    int rc = syscall(SC_set_process_boost, tid, amount);
    __RETURN_WITH_ERRNO(rc, rc, -1);
}

int futex(int32_t* userspace_address, int futex_op, int32_t value, const struct timespec* timeout)
{
    Syscall::SC_futex_params params { userspace_address, futex_op, value, timeout };
    int rc = syscall(SC_futex, &params);
    __RETURN_WITH_ERRNO(rc, rc, -1);
}

int purge(int mode)
{
    int rc = syscall(SC_purge, mode);
    __RETURN_WITH_ERRNO(rc, rc, -1);
}

#else

int module_load(const char* path, size_t path_length)
{
    (void)path;
    (void)path_length;
    return -ENOTSUP;
}

int module_unload(const char* name, size_t name_length)
{
    (void)name;
    (void)name_length;
    return -ENOTSUP;
}

int profiling_enable(pid_t pid)
{
    (void)pid;
    return -ENOTSUP;
}

int profiling_disable(pid_t pid)
{
    (void)pid;
    return -ENOTSUP;
}

int set_process_boost(int tid, int amount)
{
    /* TODO: use nice(3)? */
    (void)tid;
    (void)amount;
    return 0;
}

int shbuf_create(int size, void** buffer)
{
    int key, id = -1;

    for (key = 1; key < INT_MAX; key++) {
        id = shmget(key, size, IPC_CREAT | IPC_EXCL | 0600);
        if (id != -1) {
            *buffer = shmat(id, 0, 0);
            if (*buffer == (void *)-1) {
                perror("shmat");
                ASSERT_NOT_REACHED();
            }
            warnx("%d: shbuf_create = shm key %d, id %d", getpid(), key, id);
            break;
        }
    }

    if (id == -1) {
        perror("shmget: exhausted");
        ASSERT_NOT_REACHED();
    }

    return key;
}

int shbuf_allow_pid(int shbuf_id, pid_t peer_pid)
{
    // XXX: non-serenity doesn't support this, but processes expect to be able
    // to read once this is called
    warnx("%d: %s(%d, %d) not implemented, allowing all", getpid(), __func__,
        shbuf_id, peer_pid);
    shbuf_allow_all(shbuf_id);
    return 0;
}

int shbuf_allow_all(int shbuf_id)
{
    struct shmid_ds ds;
    int id = shmget(shbuf_id, 0, 0);
    if (id == -1)
        return -1;
    if (shmctl(id, IPC_STAT, &ds) == -1)
        return -1;
    ds.shm_perm.mode = 0666;
    return shmctl(id, IPC_SET, &ds);
}

void* shbuf_get(int shbuf_id, __attribute__((unused)) size_t *size)
{
    int id = shmget(shbuf_id, 0, 0);
    if (id == -1) {
        warn("%d: shbuf_get(%d) failed", getpid(), shbuf_id);
        errno = -id;
        return (void*)-1;
    }
    void *j = shmat(id, 0, 0);
    if (j == (void *)-1)
        warn("%d: shmget(%d) = %d, but shmat failed", getpid(), shbuf_id, id);
    return j;
}

int shbuf_release(int shbuf_id)
{
    int id = shmget(shbuf_id, 0, 0);
    if (id == -1)
        return -1;
    return shmctl(id, IPC_RMID, NULL);
}

int shbuf_get_size(int shbuf_id)
{
    struct shmid_ds ds;
    int id = shmget(shbuf_id, 0, 0);
    if (id == -1)
        return -1;
    if (shmctl(id, IPC_STAT, &ds) == -1)
        return 0;
    return ds.shm_segsz;
}

int shbuf_seal(int shbuf_id)
{
    struct shmid_ds ds;
    int id = shmget(shbuf_id, 0, 0);
    if (id == -1)
        return -1;
    if (shmctl(id, IPC_STAT, &ds) == -1)
        return -1;
    ds.shm_perm.mode &= 0755; // remove group/world write
    return shmctl(id, IPC_SET, &ds);
}


#endif


}
