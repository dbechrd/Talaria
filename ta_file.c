#include "ta_file.h"
#include "ta_log.h"
#include "dlb_memory.h"

ta_buffer *ta_file_read_all(const char *filename)
{
    // Open file
    FILE *fs = fopen(filename, "rb");
    if (!fs) {
        ta_log_write(tg_debug_log, "Unable to open %s for reading\n", filename);
        DLB_ASSERT(!"ta_file_read_all: failed to open file");
    }

    // Calculate length
    fseek(fs, 0, SEEK_END);
    long tell = ftell(fs);
    if (tell < 0 || tell >= SIZE_MAX) {
        ta_log_write(tg_debug_log, "Unable to determine length of %s\n", filename);
        DLB_ASSERT(!"ta_file_read_all: failed to calculate file length");
    }
    rewind(fs);

    // Allocate buffer
    ta_buffer *buffer = dlb_malloc(sizeof(ta_buffer) + tell + 1);
    buffer->length = tell + 1;
    buffer->data = (char *)buffer + sizeof(ta_buffer);

    // Read into buffer, null-terminate
    fread(buffer->data, 1, tell, fs);
    buffer->data[tell] = 0;

    // Close file
    fclose(fs);

    return buffer;
}