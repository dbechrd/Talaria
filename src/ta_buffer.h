#pragma once
#include "dlb/dlb_types.h"

typedef struct ta_buffer {
	u32 length;  // length, including null terminator
	u8 *data;    // null terminated
} ta_buffer;

void ta_buffer_init(ta_buffer *buffer, u32 len);
void ta_buffer_free(ta_buffer *buffer);