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

#include <AK/JsonArray.h>
#include <AK/JsonObject.h>
#include <AK/JsonValue.h>
#include <LibCore/File.h>
#include <LibCore/ProcessStatisticsReader.h>
#include <pwd.h>
#include <stdio.h>

#ifdef __OpenBSD__
#include <sys/ioctl.h>
#include <sys/param.h>
#define PLEDGENAMES
#include <sys/pledge.h>
#include <sys/proc.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <kvm.h>
#include <limits.h>
#include <string.h>
#endif

namespace Core {

HashMap<uid_t, String> ProcessStatisticsReader::s_usernames;

HashMap<pid_t, Core::ProcessStatistics> ProcessStatisticsReader::get_all()
{
    HashMap<pid_t, Core::ProcessStatistics> map;

#ifdef __OpenBSD__
    kvm_t *kd;
    struct kinfo_proc *kp;
    struct kinfo_file *kf;
    char errbuf[_POSIX2_LINE_MAX];
    int nentries, i;

    kd = kvm_openfiles(nullptr, nullptr, nullptr, KVM_NO_FILES, errbuf);
    kp = kvm_getprocs(kd, KERN_PROC_ALL, 0, sizeof(*kp), &nentries);
    if (kp == NULL) {
        perror("kvm_getprocs");
        ASSERT_NOT_REACHED();
    }

    for (i = 0; i < nentries; i++) {
        Core::ProcessStatistics process;
        char *ttname;
        int nfiles, j;

        memset(&process, 0, sizeof(process));

        process.pid = kp[i].p_pid;
        process.pgid = kp[i].p__pgid;
        process.pgp = kp[i].p_tpgid;
        process.sid = kp[i].p_sid;
        process.uid = kp[i].p_uid;
        process.gid = kp[i].p_gid;
        process.ppid = kp[i].p_gid;

        if ((kf = kvm_getfiles(kd, KERN_FILE_BYPID, process.pid, sizeof(*kf), &nfiles)) != NULL)
            process.nfds = nfiles;

        process.name = String(kp[i].p_comm);
        if ((int)kp[i].p_tdev != (int)NODEV && (ttname = devname(kp[i].p_tdev, S_IFCHR)))
            process.tty = String(ttname);

        StringBuilder pledge;

        for (j = 0; pledgenames[j].bits != 0; j++) {
            if (pledgenames[j].bits & kp[i].p_pledge) {
                if (pledge.length())
                    pledge.append(' ');
                pledge.append(pledgenames[j].name);
            }
        }

        process.pledge = pledge.to_string();

        if (kp[i].p_eflag & EPROC_UNVEIL) {
            if (kp[i].p_eflag & EPROC_LKUNVEIL)
                process.veil = String("Locked");
            else
                process.veil = String("Dropped");
        } else
            process.veil = String("None");

        process.amount_virtual = (kp[i].p_vm_dsize + kp[i].p_vm_ssize + kp[i].p_vm_tsize) * getpagesize();
        process.amount_resident = kp[i].p_vm_rssize * getpagesize();
        // no shared info

        Core::ThreadStatistics thread;
        memset(&thread, 0, sizeof(thread));

        thread.tid = kp[i].p_pid;
        thread.name = String(kp[i].p_comm);

        switch (kp[i].p_stat) {
        case SSTOP:
            thread.state = String("Stopped");
            break;
        case SSLEEP:
            if (kp[i].p_flag & P_SINTR)
                thread.state = String("Sleeping");
            else
                thread.state = String("Disk");
            break;
        case SRUN:
        case SIDL:
        case SONPROC:
            thread.state = "Runnable";
            break;
        case SDEAD:
            thread.state = "Dead";
            break;
        default:
            thread.state = "Invalid";
        }

        process.threads.append(move(thread));

        process.username = username_from_uid(process.uid);
        map.set(process.pid, process);
    }

#else
    auto file = Core::File::construct("/proc/all");
    if (!file->open(Core::IODevice::ReadOnly)) {
        fprintf(stderr, "ProcessStatisticsReader: Failed to open /proc/all: %s\n", file->error_string());
        return {};
    }

    auto file_contents = file->read_all();
    auto json = JsonValue::from_string(file_contents);
    json.as_array().for_each([&](auto& value) {
        const JsonObject& process_object = value.as_object();
        Core::ProcessStatistics process;

        // kernel data first
        process.pid = process_object.get("pid").to_u32();
        process.pgid = process_object.get("pgid").to_u32();
        process.pgp = process_object.get("pgp").to_u32();
        process.sid = process_object.get("sid").to_u32();
        process.uid = process_object.get("uid").to_u32();
        process.gid = process_object.get("gid").to_u32();
        process.ppid = process_object.get("ppid").to_u32();
        process.nfds = process_object.get("nfds").to_u32();
        process.name = process_object.get("name").to_string();
        process.tty = process_object.get("tty").to_string();
        process.pledge = process_object.get("pledge").to_string();
        process.veil = process_object.get("veil").to_string();
        process.amount_virtual = process_object.get("amount_virtual").to_u32();
        process.amount_resident = process_object.get("amount_resident").to_u32();
        process.amount_shared = process_object.get("amount_shared").to_u32();
        process.amount_dirty_private = process_object.get("amount_dirty_private").to_u32();
        process.amount_clean_inode = process_object.get("amount_clean_inode").to_u32();
        process.amount_purgeable_volatile = process_object.get("amount_purgeable_volatile").to_u32();
        process.amount_purgeable_nonvolatile = process_object.get("amount_purgeable_nonvolatile").to_u32();
        process.icon_id = process_object.get("icon_id").to_int();

        auto& thread_array = process_object.get_ptr("threads")->as_array();
        process.threads.ensure_capacity(thread_array.size());
        thread_array.for_each([&](auto& value) {
            auto& thread_object = value.as_object();
            Core::ThreadStatistics thread;
            thread.tid = thread_object.get("tid").to_u32();
            thread.times_scheduled = thread_object.get("times_scheduled").to_u32();
            thread.name = thread_object.get("name").to_string();
            thread.state = thread_object.get("state").to_string();
            thread.ticks = thread_object.get("ticks").to_u32();
            thread.priority = thread_object.get("priority").to_u32();
            thread.effective_priority = thread_object.get("effective_priority").to_u32();
            thread.syscall_count = thread_object.get("syscall_count").to_u32();
            thread.inode_faults = thread_object.get("inode_faults").to_u32();
            thread.zero_faults = thread_object.get("zero_faults").to_u32();
            thread.cow_faults = thread_object.get("cow_faults").to_u32();
            thread.unix_socket_read_bytes = thread_object.get("unix_socket_read_bytes").to_u32();
            thread.unix_socket_write_bytes = thread_object.get("unix_socket_write_bytes").to_u32();
            thread.ipv4_socket_read_bytes = thread_object.get("ipv4_socket_read_bytes").to_u32();
            thread.ipv4_socket_write_bytes = thread_object.get("ipv4_socket_write_bytes").to_u32();
            thread.file_read_bytes = thread_object.get("file_read_bytes").to_u32();
            thread.file_write_bytes = thread_object.get("file_write_bytes").to_u32();
            process.threads.append(move(thread));
        });

        // and synthetic data last
        process.username = username_from_uid(process.uid);
        map.set(process.pid, process);
    });
#endif

    return map;
}

String ProcessStatisticsReader::username_from_uid(uid_t uid)
{
    if (s_usernames.is_empty()) {
        setpwent();
        while (auto* passwd = getpwent())
            s_usernames.set(passwd->pw_uid, passwd->pw_name);
        endpwent();
    }

    auto it = s_usernames.find(uid);
    if (it != s_usernames.end())
        return (*it).value;
    return String::number(uid);
}
}
