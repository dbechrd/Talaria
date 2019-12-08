#include "ta_buffer.h"
#include "dlb/dlb_memory.h"

ta_buffer ta_buffer_init(u32 len)
{
    ta_buffer buffer = { 0 };
    buffer.length = len;
    buffer.data = dlb_calloc(1, buffer.length);
    DLB_ASSERT(buffer.data);
    return buffer;
}
void ta_buffer_free(ta_buffer buffer)
{
    dlb_free(buffer.data);
}