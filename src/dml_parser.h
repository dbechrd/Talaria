#pragma once

typedef struct DMLParser {
    struct DMLToken *tokens;
    int current;
    int expected_value_context_shown;  // used to only print verbose messages for first error of this type

    // Only for debugging?
    const char *filename;
    const char *source;
    size_t source_len;
} DMLParser;

void DMLParserInit(DMLParser *parser, struct DMLToken *tokens, const char *filename, const char *source,
    size_t source_len);
void DMLParserParse(DMLParser *parser, struct DMLObject *document);