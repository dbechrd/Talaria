#include "ta_json.h"
#include "ta_file.h"
#include "dlb/dlb_vector.h"
#include "misc/cgltf.h"

int ta_json_dump(const char *js, jsmntok_t *t, size_t count, int indent) {
    jsmntok_t *key;
    if (count == 0) {
        return 0;
    }
    switch (t->type) {
        case JSMN_PRIMITIVE: {
            printf("%.*s", t->end - t->start, js + t->start);
            return 1;
        } case JSMN_STRING: {
            printf("'%.*s'", t->end - t->start, js + t->start);
            return 1;
        } case JSMN_OBJECT: {
            printf("\n");
            int j = 0;
            for (int i = 0; i < t->size; i++) {
                for (int k = 0; k < indent; k++) {
                    printf("  ");
                }
                key = t + 1 + j;
                j += ta_json_dump(js, key, count - j, indent + 1);
                if (key->size > 0) {
                    printf(": ");
                    j += ta_json_dump(js, t + 1 + j, count - j, indent + 1);
                }
                printf("\n");
            }
            return j + 1;
        } case JSMN_ARRAY: {
            int j = 0;
            printf("\n");
            for (int i = 0; i < t->size; i++) {
                for (int k = 0; k < indent - 1; k++) {
                    printf("  ");
                }
                printf("   - ");
                j += ta_json_dump(js, t + 1 + j, count - j, indent + 1);
                printf("\n");
            }
            return j + 1;
        }
    }
    return 0;
}

static void json_log_jsmn_error(int err_code)
{
    // TODO: Log more detailed error
    switch (err_code) {
        case JSMN_ERROR_NOMEM: {
            // Not enough tokens were provided
            break;
        } case JSMN_ERROR_INVAL: {
            // Invalid character inside JSON string
            break;
        } case JSMN_ERROR_PART: {
            // The string is not a full JSON packet, more bytes expected
            break;
        } default: {
            // Unknown JSMN error
            break;
        }
    }
}

ta_json_result ta_json_parse(const char *json_data, size_t json_len,
    jsmntok_t **tokens)
{
    jsmn_parser parser = { 0 };

    // TODO(dlb): Cache JSON token count in header somewhere so that we can skip
    // this additional pre-scan to count tokens
    if (dlb_vec_cap(*tokens) == 0) {
        int token_count = jsmn_parse(&parser, json_data, json_len, NULL, 0);
        if (token_count <= 0) {
            return ta_json_result_invalid_json;
        }
        dlb_vec_reserve(*tokens, (size_t)token_count);
    }

    if (!*tokens) {
        return ta_json_result_out_of_memory;
    }

    jsmn_init(&parser);

    size_t tokens_max = dlb_vec_cap(*tokens);
    int token_count = jsmn_parse(&parser, json_data, json_len, *tokens, (unsigned int)tokens_max);
    if (token_count <= 0) {
        dlb_vec_free(*tokens);
        json_log_jsmn_error(token_count);
        return ta_json_result_invalid_json;
    } else {
        dlb_vec_hdr(*tokens)->len = token_count;
    }

    // NOTE(dlb): This can be removed, just making sure jsmn_parse doesn't do
    // silly things.
    DLB_ASSERT(token_count <= tokens_max);

    return ta_json_result_success;
}

void ta_json_test()
{
    //ta_buffer test_json = ta_file_read_all("test.json");
    char *test_json = ta_file_read_all("data/scene/scene.json");
    if (!dlb_vec_len(test_json)) {
        return;
    }

    jsmntok_t *tokens = 0;
    ta_json_parse(test_json, dlb_vec_len(test_json), &tokens);
    DLB_ASSERT(tokens);

    //ta_json_dump(test_json.data, tokens, dlb_vec_len(tokens), 0);

    dlb_vec_free(tokens);
    dlb_vec_free(test_json);
}