#pragma once

typedef struct DMLParser {
    const char *source;
    size_t source_len;
    struct DMLToken *tokens;
    int current;
    int expected_value_context_shown;  // used to only print verbose messages for first error of this type
} DMLParser;

void DMLParserInit(DMLParser *parser, struct DMLToken *tokens, const char *source, size_t source_len);
void DMLParserParse(DMLParser *parser, struct DMLObject *document);