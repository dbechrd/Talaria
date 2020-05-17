#pragma once

typedef struct DMLScanner {
    const char *source;
    size_t source_len;
    struct DMLToken *tokens;
    size_t start;
    size_t current;
    size_t line;
    size_t column;
    size_t start_line;
    size_t start_column;
    bool error_flag;
} DMLScanner;

void DMLScannerInit(DMLScanner *scanner, const char *source, size_t source_len);
bool DMLScannerScanTokens(DMLScanner *scanner, struct DMLToken **tokens);