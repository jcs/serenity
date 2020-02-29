#pragma once

#include </usr/include/unistd.h>

__BEGIN_DECLS

void sysbeep();

int get_process_name(char* buffer, int buffer_size);

int set_process_icon(int icon_id);

__END_DECLS
