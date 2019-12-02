#include "ta_gltf.h"
#include "dlb/dlb_types.h"
//#include "dlb/dlb_memory.h"
//#include "dlb/dlb_vector.h"
//#include "ta_file.h"
//#include "ta_buffer.h"
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>

#define CGLTF_IMPLEMENTATION
#include "misc/cgltf.h"

cgltf_result ta_gltf_parse(const char *filename)
{
    cgltf_options options = {0};
    cgltf_data* data = NULL;
    cgltf_result result = cgltf_parse_file(&options, filename, &data);

    if (result == cgltf_result_success)
        result = cgltf_load_buffers(&options, data, filename);

    if (result == cgltf_result_success)
        result = cgltf_validate(data);

    printf("Result: %d\n", result);

    if (result == cgltf_result_success)
    {
        printf("Type: %u\n", data->file_type);
        printf("Meshes: %lu\n", data->meshes_count);
    }

    cgltf_free(data);

    return result;
}

void ta_gltf_test()
{
    cgltf_result err = ta_gltf_parse("data/mesh/dude.glb");
    DLB_ASSERT(!err);
}
