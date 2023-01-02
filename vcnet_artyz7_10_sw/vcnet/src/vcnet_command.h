
#ifndef VCNET_COMMAND_H
#define VCNET_COMMAND_H

#include "xil_types.h"

void vcnet_command_add(const void *data, u32 len);
void vcnet_command_exec_step(void);
u32 vcnet_command_num_queued(void);

#endif
