#pragma once

typedef struct dml_scanner {
    const char *source;
    size_t source_len;
    struct dml_token *tokens;
    size_t start;
    size_t current;
    size_t line;
    size_t column;
    size_t start_line;
    size_t start_column;
    bool error_flag;
} dml_scanner;

void dml_scanner_init(dml_scanner *scanner, const char *source, size_t source_len);
bool dml_scanner_scan_tokens(dml_scanner *scanner, struct dml_token **tokens);