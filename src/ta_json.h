#pragma once
#include "dlb/dlb_types.h"
#include "misc/jsmn.h"

typedef enum ta_json_result {
    ta_json_result_success,
    ta_json_result_invalid_json,
    ta_json_result_out_of_memory,
} ta_json_result;

ta_json_result ta_json_parse(const u8* json_data, size_t json_len,
    jsmntok_t** tokens);
void ta_json_test();