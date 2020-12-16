#include "dml_scanner.h"
#include "dml_token.h"

void dml_scanner_init(dml_scanner *scanner, const char *source, size_t source_len)
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

static inline bool dml_scanner_eof(dml_scanner *scanner)
{
    return scanner->current >= scanner->source_len;
}
static inline void dml_scanner_advance(dml_scanner *scanner)
{
    scanner->current++;
    scanner->column++;
}
static char dml_scanner_peek(dml_scanner *scanner)
{
    if (dml_scanner_eof(scanner)) {
        return '\0';
    }

    return scanner->source[scanner->current];
}
static char dml_scanner_peek_next(dml_scanner *scanner)
{
    if (scanner->current + 1 >= scanner->source_len) {
        return '\0';
    }

    return scanner->source[scanner->current + 1];
}
static char dml_scanner_peek_next_2(dml_scanner *scanner)
{
    if (scanner->current + 2 >= scanner->source_len) {
        return '\0';
    }

    return scanner->source[scanner->current + 2];
}
static bool dml_scanner_match(dml_scanner *scanner, char expected)
{
    if (!dml_scanner_eof(scanner) && scanner->source[scanner->current] == expected) {
        scanner->current++;
        scanner->column++;
        return true;
    }
    return false;
}

static bool dml_scanner_is_digit(char c)
{
    return c >= '0' && c <= '9';
}
static bool dml_scanner_is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
static bool dml_scanner_is_alpha_num(char c)
{
    return dml_scanner_is_alpha(c) || dml_scanner_is_digit(c);
}
static bool dml_scanner_is_hex(char c)
{
    return dml_scanner_is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static dml_token *dml_scanner_add_token(dml_scanner *scanner, const char *text, dml_token_type type)
{
    dml_token *token = (dml_token *)dlb_vec_alloc(scanner->tokens);
    size_t length = scanner->current - scanner->start;
    dml_token_init(token, type, text, scanner->start_line, scanner->start_column, scanner->start, length);
    return token;
}

static void dml_scanner_error_context(dml_scanner *scanner, size_t offset)
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

static void dml_scanner_scan_string(dml_scanner *scanner)
{
    while (dml_scanner_peek(scanner) != '"' && dml_scanner_peek(scanner) != '\n' && !dml_scanner_eof(scanner)) {
        dml_scanner_advance(scanner);
    }

    if (dml_scanner_peek(scanner) == '\n' || dml_scanner_eof(scanner)) {
        //Lox.Error(scanner->line, scanner->column, "Unterminated string.");
        printf("[%04zu:%04zu] error: unterminated string\n", scanner->line, scanner->column);
        dml_scanner_error_context(scanner, 0);
        return;
    }

    // TODO(dlb): If we wanted to support escapes sequences in strings (e.g. '\n', etc.), we would need to
    // unescape them here.
    const char *value = ta_symbol_intern(scanner->source + scanner->start + 1, scanner->current - scanner->start - 1);

    // eat closing '"'
    dml_scanner_advance(scanner);

    dml_token *token = dml_scanner_add_token(scanner, value, TOK_STRING);
    token->literal.as_string = value;
}
static void dml_scanner_scan_comment(dml_scanner *scanner)
{
    while (dml_scanner_peek(scanner) != '\n' && !dml_scanner_eof(scanner)) {
        if (dml_scanner_peek(scanner) == '\n') {
            scanner->line++;
        }
        dml_scanner_advance(scanner);
    }
}
static void dml_scanner_scan_number(dml_scanner *scanner)
{
    // 0x3f3504f4
    if (scanner->source[scanner->start] == '0' && dml_scanner_peek(scanner) == 'x') {
        dml_scanner_advance(scanner);
        dml_scanner_advance(scanner);
        while (dml_scanner_is_hex(dml_scanner_peek(scanner))) {
            dml_scanner_advance(scanner);
        }
    } else {
        // -123, +123
        if ((scanner->source[scanner->start] == '-' || scanner->source[scanner->start] == '+')
            && dml_scanner_is_digit(dml_scanner_peek(scanner)))
        {
            // consume sign ('-' or '+')
            dml_scanner_advance(scanner);
        }

        // left side of decimal
        while (dml_scanner_is_digit(dml_scanner_peek(scanner))) {
            dml_scanner_advance(scanner);
        }

        // 1e-05, 1e+05
        if (dml_scanner_peek(scanner) == 'e' &&
            (dml_scanner_peek_next(scanner) == '-' || dml_scanner_peek_next(scanner) == '+') &&
            (dml_scanner_is_digit(dml_scanner_peek_next_2(scanner))))
        {
            // consume 'e'
            dml_scanner_advance(scanner);
            // consume sign ('-' or '+')
            dml_scanner_advance(scanner);

            // consume exponent
            while (dml_scanner_is_digit(dml_scanner_peek(scanner))) {
                dml_scanner_advance(scanner);
            }
        // 1.23
        } else if (dml_scanner_peek(scanner) == '.' && dml_scanner_is_digit(dml_scanner_peek_next(scanner))) {
            // consume decimal '.'
            dml_scanner_advance(scanner);

            while (dml_scanner_is_digit(dml_scanner_peek(scanner))) {
                dml_scanner_advance(scanner);
            }

            // 1.2e-05, 1.2e+05
            if (dml_scanner_peek(scanner) == 'e' &&
                (dml_scanner_peek_next(scanner) == '-' || dml_scanner_peek_next(scanner) == '+') &&
                (dml_scanner_is_digit(dml_scanner_peek_next_2(scanner))))
            {
                // consume 'e'
                dml_scanner_advance(scanner);
                // consume sign ('-' or '+')
                dml_scanner_advance(scanner);

                // consume exponent
                while (dml_scanner_is_digit(dml_scanner_peek(scanner))) {
                    dml_scanner_advance(scanner);
                }
            }
        }
    }

    // NOTE: This is dangerous because it could read past scanner->current even if we didn't want to advance that far
    float value = parse_float(scanner->source + scanner->start);
    dml_token *token = dml_scanner_add_token(scanner, "<number>", TOK_NUMBER);
    token->literal.as_float = value;
}
static void dml_scanner_scan_identifier(dml_scanner *scanner)
{
    while (dml_scanner_is_alpha_num(dml_scanner_peek(scanner))) {
        dml_scanner_advance(scanner);
    }

    static struct {
        const char *name;
        dml_token_type tokenType;
    } keywords[3] = {
        { 0, TOK_NULL },
        { 0, TOK_FALSE },
        { 0, TOK_TRUE },
    };

    if (!keywords[0].name) {
        keywords[0].name = INTERN("null");
        keywords[1].name = INTERN("false");
        keywords[2].name = INTERN("true");
    }

    const char *text = ta_symbol_intern(scanner->source + scanner->start, scanner->current - scanner->start);
    dml_token_type type = TOK_IDENTIFIER;
    for (size_t i = 0; i < ARRAY_SIZE(keywords); i++) {
        if (text == keywords[i].name) {
            type = keywords[i].tokenType;
            break;
        }
    }

    dml_scanner_add_token(scanner, text, type);
}
bool dml_scanner_scan_tokens(dml_scanner *scanner, dml_token **tokens)
{
    scanner->error_flag = false;
    //dlb_vec_reserve(scanner->tokens, 65536);

    while (!dml_scanner_eof(scanner)) {
        char c = scanner->source[scanner->current];
        scanner->start = scanner->current;
        dml_scanner_advance(scanner);
        scanner->start_line = scanner->line;
        scanner->start_column = scanner->column;
        switch (c) {
            case '{': dml_scanner_add_token(scanner, "{", TOK_LEFT_CURLY_BRACE); break;
            case '}': dml_scanner_add_token(scanner, "}", TOK_RIGHT_CURLY_BRACE); break;
            case '[': dml_scanner_add_token(scanner, "[", TOK_LEFT_SQUARE_BRACKET); break;
            case ']': dml_scanner_add_token(scanner, "]", TOK_RIGHT_SQUARE_BRACKET); break;
            case ',': dml_scanner_add_token(scanner, ",", TOK_COMMA); break;
            case ':': dml_scanner_add_token(scanner, ":", TOK_COLON); break;
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
                dml_scanner_scan_string(scanner);
                break;
            case '#':
                dml_scanner_scan_comment(scanner);
                break;
            default:
                if (c == '-' || c == '+' || dml_scanner_is_digit(c)) {
                    dml_scanner_scan_number(scanner);
                } else if (dml_scanner_is_alpha(c)) {
                    dml_scanner_scan_identifier(scanner);
                } else {
                    //Lox.Error(scanner->line, scanner->column, $"Unexpected character '{c}'.");
                    printf("[%04zu:%04zu] error: unexpected character '%c'\n", scanner->line, scanner->column, c);
                    dml_scanner_error_context(scanner, 0);
                }
                break;
        }
    }

    dml_token *token = (dml_token *)dlb_vec_alloc(scanner->tokens);
    dml_token_init(token, TOK_EOF, NULL, scanner->line, 0, scanner->source_len ? scanner->source_len - 1 : 0, 1);

    if (tokens) {
        *tokens = scanner->tokens;
    }
    return !scanner->error_flag;
}
