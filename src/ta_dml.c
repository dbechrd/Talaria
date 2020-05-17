#include "ta_dml.h"
#include "dml_scanner.h"
#include "dml_parser.h"
#include "dml.h"
#include "ta_log.h"
#include "ta_file.h"

dml_result dml_load(const char *filename)
{
    ta_log_timed_region_start(&tg_debug_log, SRC_DML, CSTR("dml_load"));
    ta_log_write(&tg_debug_log, SRC_DML, "Loading %s\n", filename);

    char *source = ta_file_read_all(filename);
    size_t source_len = dlb_vec_len(source) - 1;
    if (!source) {
        ta_log_write(&tg_debug_log, SRC_DML, "[FATAL] Unable to open file [%s].\n", filename);
        ta_log_timed_region_end(&tg_debug_log, CSTR("dml_load"));
        return OGX_FILE_INVALID;
    }
    if (!source_len) {
        ta_log_write(&tg_debug_log, SRC_DML, "[FATAL] File was empty [%s].\n", filename);
        ta_log_timed_region_end(&tg_debug_log, CSTR("dml_load"));
        return OGX_FILE_INVALID;
    }

    DMLScanner scanner = { 0 };
    DMLToken *tokens = 0;

    ta_log_write(&tg_debug_log, SRC_DML, "Scanning...\n");
    DMLScannerInit(&scanner, source, source_len);
    if (DMLScannerScanTokens(&scanner, &tokens)) {
        ta_log_write(&tg_debug_log, SRC_DML, "Parsing...\n");

        DMLParser parser = { 0 };
        DMLParserInit(&parser, tokens, source, source_len);

        DMLObject document = { 0 };
        DMLParserParse(&parser, &document);

#if 0
        fputs("Document:\n", stdout);
        DMLPrintObject(&document, 0);
        fputc('\n', stdout);
#endif
    } else {
        ta_log_write(&tg_debug_log, SRC_DML, "Scanner produced errors, skipping parse stage.\n");
#if 0
        ta_log_write(&tg_debug_log, SRC_DML, "Token stream:\n");
        dlb_vec_each(DMLToken *, token, tokens) {
            ta_log_write(&tg_debug_log, SRC_DML, "[%04d:%04d] %18s %s", token->line, token->column,
                DMLTokenTypeToString(token->type), token->lexeme);
            if (token->type == TOK_NUMBER) {
                ta_log_write(&tg_debug_log, SRC_DML, " (%f)\n", token->literal.as_float);
            } else {
                ta_log_write(&tg_debug_log, SRC_DML, "\n");
            }
        }
#endif
    }

    ta_log_write(&tg_debug_log, SRC_DML, "Loaded successfully.\n");
    ta_log_timed_region_end(&tg_debug_log, CSTR("dml_load"));
    return OGX_SUCCESS;
}

#if 0
// TODO: Make a read_entire_contents helper in DLB
const char *source =
// Scanner errors
//"abc: % # unexpected character\n"
//"abc: ) # unexpected character\n"
//"\"abc  # unterminated string\n"

// Parser errors
//"abc    # missing : after identifier\n"  // note: this error propagates

// Valid document
"ta_light_node: { # This is a comment\n"
"	name: \"light_main_node\"\n"
"	light: \"light_main\"\n"
"	transform: {  # node.matrix_local (ExportNodeTransform)\n"
"		type: \"mat4\"\n"
"		data: [\n"
"			0xbe94ec36, 0x3f748619, 0xbd620dec, 0x00000000,\n"
"			0xbf4566dd, 0xbe4cae39, 0x3f1ac222, 0x00000000,\n"
"			0x3f10ff25, 0x3e5fa1f1, 0x3f4b6fa4, 0x00000000,\n"
"			0x4082709a, 0x3f80b2b7, 0x40bcec70, 0x3f800000\n"
"		]\n"
"	}\n"
"}\n"
"ta_camera_node: {\n"
"	name: \"camera_main_node\"\n"
"	camera: \"camera_main\"\n"
"	transform: {  # node.matrix_local (ExportNodeTransform)\n"
"		type: \"mat4\"\n"
"		data: [\n"
"			0x3f2f987f, 0x3f3a48ff, 0x00000000, 0x00000000,\n"
"			0xbea5e518, 0x3e9c601f, 0x3f6538a6, 0x00000000,\n"
"			0x3f26cc85, 0xbf1d3a45, 0x3ee3fa9d, 0x00000000,\n"
"			0x40eb7c0a, 0xc0dda014, 0x409eaa78, 0x3f800000\n"
"		]\n"
"	}\n"
"}\n";
size_t source_len = strlen(source);
#endif