#pragma once

typedef struct dml_parser {
    struct dml_token *tokens;
    int current;
    int expected_value_context_shown;  // used to only print verbose messages for first error of this type

    // TODO: Move this to dml_document perhaps?
    const char *filename;
    const char *source;
    size_t source_len;
} dml_parser;

void dml_parser_init(dml_parser *parser, struct dml_token *tokens, const char *filename, const char *source,
    size_t source_len);
void dml_parser_parse(dml_parser *parser, struct dml_document *document);