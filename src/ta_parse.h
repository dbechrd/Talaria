#pragma once

unsigned int parse_uint       (char *buf);
int          parse_int_binary (char *buf);
int          parse_int_hex    (char *buf);
int          parse_int        (char *buf);
float        parse_float      (char *buf);

void parse_tests();