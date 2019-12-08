#pragma once
#include "dlb/dlb_types.h"

struct jsmntok_t;

typedef enum ta_json_result {
    ta_json_result_success,
    ta_json_result_invalid_json,
    ta_json_result_out_of_memory,
} ta_json_result;

int ta_json_dump(const u8 *js, struct jsmntok_t *t, size_t count, int indent);
ta_json_result ta_json_parse(const u8* json_data, size_t json_len,
    struct jsmntok_t** tokens);
void ta_json_test();