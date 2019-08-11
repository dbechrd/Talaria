#include "ta_buffer.h"
#include "dlb_memory.h"

void ta_buffer_init(ta_buffer *buffer, u32 len)
{
    buffer->data = dlb_calloc(1, len);
    DLB_ASSERT(buffer->data);
    buffer->length = len;
}
void ta_buffer_free(ta_buffer *buffer)
{
	dlb_free(buffer);
}