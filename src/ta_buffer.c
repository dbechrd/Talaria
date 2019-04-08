#include "ta_buffer.h"
#include "dlb_memory.h"

void ta_buffer_free(ta_buffer *buffer)
{
	dlb_free(buffer);
}