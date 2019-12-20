#include "ta_buffer.h"
#include "dlb/dlb_memory.h"

void ta_buffer_init(ta_buffer *buffer, u32 len)
{
    buffer->length = len;
    buffer->data = dlb_calloc(1, buffer->length);
    DLB_ASSERT(buffer->data);
}
void ta_buffer_free(ta_buffer buffer)
{
    dlb_free(buffer.data);
}