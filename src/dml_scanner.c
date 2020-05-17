#include "dml_scanner.h"
#include "dml_token.h"

static bool DMLScannerIsAtEnd(DMLScanner *scanner);
static void DMLScannerScanToken(DMLScanner *scanner);
static void DMLScannerScanString(DMLScanner *scanner);
static void DMLScannerScanComment(DMLScanner *scanner);
static void DMLScannerScanNumber(DMLScanner *scanner);
static void DMLScannerScanIdentifier(DMLScanner *scanner);
static bool DMLScannerIsDigit(char c);
static bool DMLScannerIsAlpha(char c);
static bool DMLScannerIsAlphaNumeric(char c);
static bool DMLScannerIsHex(char c);
static bool DMLScannerMatch(DMLScanner *scanner, char expected);
static char DMLScannerPeek(DMLScanner *scanner);
static char DMLScannerPeekNext(DMLScanner *scanner);
static char DMLScannerAdvance(DMLScanner *scanner);
static DMLToken *DMLScannerAddToken(DMLScanner *scanner, DMLTokenType type);
static const char *DMLScannerSubstring(const char *str, size_t startIndex, size_t length);

void DMLScannerErrorContext(DMLScanner *scanner, size_t offset)
{
    scanner->error_flag = true;

    size_t line_start = scanner->start; // ? scanner->start - 1 : scanner->start;
    while (line_start > 0 && scanner->source[line_start - 1] != '\n') {
        line_start--;
    }

    size_t line_end = scanner->current;
    while (line_end < scanner->source_len && scanner->source[line_end] != '\n') {
        line_end++;
    }

    printf("%.*s\n", (int)(line_end - line_start), scanner->source + line_start);
    for (size_t i = 1; i < scanner->column + offset; i++) {
        fputc(' ', stdout);
    }
    fputs("^\n\n", stdout);
}

void DMLScannerInit(DMLScanner *scanner, const char *source, size_t source_len)
{
    scanner->source = source;
    scanner->source_len = source_len;
    scanner->tokens = 0;
    scanner->start = 0;
    scanner->current = 0;
    scanner->line = 1;
    scanner->column = 0;
    scanner->error_flag = false;
}

bool DMLScannerScanTokens(DMLScanner *scanner, DMLToken **tokens)
{
    scanner->error_flag = false;
    while (!DMLScannerIsAtEnd(scanner)) {
        scanner->start = scanner->current;
        DMLScannerScanToken(scanner);
    }

    DMLToken *token = dlb_vec_alloc(scanner->tokens);
    DMLTokenInit(token, TOK_EOF, NULL, scanner->line, 0, scanner->source_len ? scanner->source_len - 1 : 0, 1);

    if (tokens) {
        *tokens = scanner->tokens;
    }
    return !scanner->error_flag;
}

static bool DMLScannerIsAtEnd(DMLScanner *scanner)
{
    return scanner->current >= scanner->source_len;
}

static void DMLScannerScanToken(DMLScanner *scanner)
{
    char c = DMLScannerAdvance(scanner);
    scanner->start_line = scanner->line;
    scanner->start_column = scanner->column;
    switch (c) {
        //case '\t': DMLScannerAddToken(scanner, TOK_TAB); break;
        //case '(': DMLScannerAddToken(scanner, TOK_LEFT_PAREN); break;
        //case ')': DMLScannerAddToken(scanner, TOK_RIGHT_PAREN); break;
        case '{': DMLScannerAddToken(scanner, TOK_LEFT_CURLY_BRACE); break;
        case '}': DMLScannerAddToken(scanner, TOK_RIGHT_CURLY_BRACE); break;
        case '[': DMLScannerAddToken(scanner, TOK_LEFT_SQUARE_BRACKET); break;
        case ']': DMLScannerAddToken(scanner, TOK_RIGHT_SQUARE_BRACKET); break;
        case ',': DMLScannerAddToken(scanner, TOK_COMMA); break;
        //case '.': DMLScannerAddToken(scanner, TOK_DOT); break;
        //case '-': DMLScannerAddToken(scanner, TOK_MINUS); break;
        //case '+': DMLScannerAddToken(scanner, TOK_PLUS); break;
        //case ';': DMLScannerAddToken(scanner, TOK_SEMICOLON); break;
        //case '*': DMLScannerAddToken(scanner, TOK_STAR); break;
        case ':': DMLScannerAddToken(scanner, TOK_COLON); break;
        //case '!': DMLScannerAddToken(scanner, DMLScannerMatch(scanner, '=') ? TOK_BANG_EQUAL : TOK_BANG); break;
        //case '=': DMLScannerAddToken(scanner, DMLScannerMatch(scanner, '=') ? TOK_EQUAL_EQUAL : TOK_EQUAL); break;
        //case '<': DMLScannerAddToken(scanner, DMLScannerMatch(scanner, '=') ? TOK_LESS_EQUAL : TOK_LESS); break;
        //case '>': DMLScannerAddToken(scanner, DMLScannerMatch(scanner, '=') ? TOK_GREATER_EQUAL : TOK_GREATER); break;
        //case '/':
        //    if (DMLScannerMatch(scanner, '/'))
        //    {
        //        // Eat comment
        //        while (DMLScannerPeek(scanner) != '\n' && !DMLScannerIsAtEnd(scanner))
        //        {
        //            DMLScannerAdvance(scanner);
        //        }
        //    }
        //    else
        //    {
        //        DMLScannerAddToken(scanner, TOK_SLASH);
        //    }
        //    break;
        case '\t':
            scanner->column += DML_ERROR_CONTEXT_TAB_WIDTH - 1;
        case ' ':
        case '\r':
            // ignore whitespace
            break;
        case '\n':
            scanner->line++;
            scanner->column = 0;
            break;
        case '"':
            DMLScannerScanString(scanner);
            break;
        case '#':
            DMLScannerScanComment(scanner);
            break;
        default:
            if (DMLScannerIsDigit(c)) {
                DMLScannerScanNumber(scanner);
            } else if (DMLScannerIsAlpha(c)) {
                DMLScannerScanIdentifier(scanner);
            } else {
                //Lox.Error(scanner->line, scanner->column, $"Unexpected character '{c}'.");
                printf("[%04zu:%04zu] error: unexpected character '%c'\n", scanner->line, scanner->column, c);
                DMLScannerErrorContext(scanner, 0);
            }
            break;
    }
}

static void DMLScannerScanString(DMLScanner *scanner)
{
    while (DMLScannerPeek(scanner) != '"' && DMLScannerPeek(scanner) != '\n' && !DMLScannerIsAtEnd(scanner)) {
        DMLScannerAdvance(scanner);
    }

    if (DMLScannerPeek(scanner) == '\n' || DMLScannerIsAtEnd(scanner)) {
        //Lox.Error(scanner->line, scanner->column, "Unterminated string.");
        printf("[%04zu:%04zu] error: unterminated string\n", scanner->line, scanner->column);
        DMLScannerErrorContext(scanner, 0);
        return;
    }

    // TODO(dlb): If we wanted to support escapes sequences in strings (e.g. '\n', etc.), we would need to
    // unescape them here.
    // TODO: Intern string
    const char *value = DMLScannerSubstring(scanner->source, scanner->start + 1, scanner->current - scanner->start - 1);

    // eat closing '"'
    DMLScannerAdvance(scanner);

    DMLToken *token = DMLScannerAddToken(scanner, TOK_STRING);
    token->literal.as_string = value;
}

static void DMLScannerScanComment(DMLScanner *scanner)
{
    while (DMLScannerPeek(scanner) != '\n' && !DMLScannerIsAtEnd(scanner)) {
        if (DMLScannerPeek(scanner) == '\n') {
            scanner->line++;
        }
        DMLScannerAdvance(scanner);
    }
}

static void DMLScannerScanNumber(DMLScanner *scanner)
{
    if (scanner->source[scanner->start] == '0' && DMLScannerPeek(scanner) == 'x') {
        DMLScannerAdvance(scanner);
        DMLScannerAdvance(scanner);
        while (DMLScannerIsHex(DMLScannerPeek(scanner))) {
            DMLScannerAdvance(scanner);
        }
    } else {
        while (DMLScannerIsDigit(DMLScannerPeek(scanner))) {
            DMLScannerAdvance(scanner);
        }

        if (DMLScannerPeek(scanner) == '.' && DMLScannerIsDigit(DMLScannerPeekNext(scanner))) {
            // eat the '.'
            DMLScannerAdvance(scanner);

            while (DMLScannerIsDigit(DMLScannerPeek(scanner))) {
                DMLScannerAdvance(scanner);
            }
        }
    }

    char text[32] = { 0 };
    size_t text_len = scanner->current - scanner->start;
    assert(text_len < sizeof(text) - 1);
    memcpy(text, scanner->source + scanner->start, text_len);

    //const char *text = DMLScannerSubstring(scanner->source, scanner->start, scanner->current - scanner->start);

    char *endptr = 0;
    float value = strtof(text, &endptr);
    if (*endptr != '\0') {
        // TODO(dlb): Catch exception and print more specific error message?
        //Lox.Error(scanner->line, scanner->column, $"Failed to parse number '{text}'.");
        printf("[%04zu:%04zu] error: failed to parse number '%s'\n", scanner->line, scanner->column, text);
        DMLScannerErrorContext(scanner, 0);
        return;
    }

    DMLToken *token = DMLScannerAddToken(scanner, TOK_NUMBER);
    token->literal.as_float = value;
}

static void DMLScannerScanIdentifier(DMLScanner *scanner)
{
    while (DMLScannerIsAlphaNumeric(DMLScannerPeek(scanner))) {
        DMLScannerAdvance(scanner);
    }

    static struct {
        const char *name;
        DMLTokenType tokenType;
    } keywords[] = {
        { "null",   TOK_NULL },
        { "false",  TOK_FALSE },
        { "true",   TOK_TRUE },
    };

    const char *text = DMLScannerSubstring(scanner->source, scanner->start, scanner->current - scanner->start);
    DMLTokenType type = TOK_IDENTIFIER;
    for (size_t i = 0; i < sizeof(keywords)/sizeof(*keywords); i++) {
        if (!strcmp(text, keywords[i].name)) {
            type = keywords[i].tokenType;
            break;
        }
    }

    DMLToken *token = DMLScannerAddToken(scanner, type);
}

static bool DMLScannerIsDigit(char c)
{
    return c >= '0' && c <= '9';
}

static bool DMLScannerIsAlpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool DMLScannerIsAlphaNumeric(char c)
{
    return DMLScannerIsAlpha(c) || DMLScannerIsDigit(c);
}

static bool DMLScannerIsHex(char c)
{
    return DMLScannerIsDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static bool DMLScannerMatch(DMLScanner *scanner, char expected)
{
    if (DMLScannerIsAtEnd(scanner)) {
        return false;
    }
    if (scanner->source[scanner->current] != expected) {
        return false;
    }

    scanner->current++;
    scanner->column++;
    return true;
}

static char DMLScannerPeek(DMLScanner *scanner)
{
    if (DMLScannerIsAtEnd(scanner)) {
        return '\0';
    }

    return scanner->source[scanner->current];
}

static char DMLScannerPeekNext(DMLScanner *scanner)
{
    if (scanner->current + 1 >= scanner->source_len) {
        return '\0';
    }

    return scanner->source[scanner->current + 1];
}

static char DMLScannerAdvance(DMLScanner *scanner)
{
    scanner->current++;
    scanner->column++;
    return scanner->source[scanner->current - 1];
}

static DMLToken *DMLScannerAddToken(DMLScanner *scanner, DMLTokenType type)
{
    size_t length = scanner->current - scanner->start;
    const char *text = DMLScannerSubstring(scanner->source, scanner->start, length);
    DMLToken *token = dlb_vec_alloc(scanner->tokens);
    DMLTokenInit(token, type, text, scanner->start_line, scanner->start_column, scanner->start, length);
    return token;
}

static const char *DMLScannerSubstring(const char *str, size_t startIndex, size_t length)
{
    assert(length);
    char *text = calloc(1, length + 1);
    memcpy(text, str + startIndex, length);
    return text;
}