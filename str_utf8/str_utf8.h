#ifndef _STR_UTF8_H_
#define _STR_UTF8_H_

#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

unsigned int utf8_strlen( const char* src );
char* utf8_strncpy( char* dest, const char* src, unsigned int max );
char* utf8_ltrim( char* str );

#endif
