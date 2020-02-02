#pragma once

#include </usr/include/unistd.h>

__BEGIN_DECLS

void sysbeep();

int get_process_name(char* buffer, int buffer_size);
int create_shared_buffer(int, void** buffer);
int share_buffer_with(int, pid_t peer_pid);
int share_buffer_globally(int);
void* get_shared_buffer(int shared_buffer_id);
int release_shared_buffer(int shared_buffer_id);
int seal_shared_buffer(int shared_buffer_id);
int get_shared_buffer_size(int shared_buffer_id);
int set_process_icon(int icon_id);

__END_DECLS
