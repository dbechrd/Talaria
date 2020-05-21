#pragma once

unsigned int parse_uint       (const char *buf);
int          parse_int_binary (const char *buf);
int          parse_int_hex    (const char *buf);
int          parse_int        (const char *buf);
float        parse_float      (const char *buf);

void parse_tests();