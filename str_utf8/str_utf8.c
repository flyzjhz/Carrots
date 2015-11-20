#include "str_utf8.h"

static unsigned int utf8_check( const char c )
{
	if ( (c&0x80) == 0 ) {  /* ascii */
		return 1;
	}
	else if ( (c&0xe0) == 0xc0 ) {
		return 2;
	}
	else if ( (c&0xf0) == 0xe0 ) {
		return 3;
	}
	else if ( (c&0xf8) == 0xf0 ) {
		return 4;
	}
	else if ( (c&0xfc) == 0xf8 ) {
		return 5;
	}
	else if ( (c&0xfe) == 0xfc ) {
		return 6;
	}
	else {
		return 0;
	}
}

unsigned int utf8_strlen( const char* src )
{
	if ( NULL == src ) {
		return 0;
	}

	unsigned int ulen = 0;
	unsigned int slen = strlen(src);

	unsigned int i;
	unsigned int tmp;
	for ( i=0; i<slen; /* NULL */ ) {
		tmp = utf8_check( src[i] );
		if ( tmp > 0 && (i+=tmp) <= slen ) {
			++ulen;
		}
		else {
			break;
		}
	}

	return ulen;
}

char* utf8_strncpy( char* dest, const char* src, unsigned int max )
{
	if ( NULL == dest || NULL == src || 0 == max ) {
		return dest;
	}

	unsigned int slen = strlen(src);

	char* p = dest;
	unsigned int flag = 1;
	unsigned int tmp;
	unsigned int i;
	unsigned int n;
	for ( i=0,n=0; i<slen && n<max; /* NULL */ ) {
		if ( src[i] == '\r' || src[i] == '\n' || src[i] == '\t' ) {
			++i;
			continue;
		}

		if ( src[i] == ' ' ) {
			if ( flag == 0 ) {
				*p++ = src[i];
				n++;
				flag = 1;
			}
			++i;
			continue;
		}

		tmp = utf8_check( src[i] );
		if ( tmp > 0 && (i+tmp) <= slen ) {
			memcpy( p, src+i, tmp );
			p += tmp;
			i += tmp;
			n++;
			flag = 0;
		}
		else {
			break;
		}
	}

	*p = '\0';
	return dest;
}


char* utf8_ltrim( char* str )
{
	char utf8_space[] = { -29, -128, -128, 0 }; /* chinese_space, :) */

	while ( *str ) {
		if ( ' ' == *str ) {
			str += 1;
		}
		else if ( 0 == strncmp(str, utf8_space, 3) ) {
			str += 3;
		}
		else {
			break;
		}
	}
	return str;
}


/* Test Code */
#ifdef TEST
#include <stdlib.h>
int main(int argc, char **argv)
{
    // get 50 char of utf8
	unsigned int n = atoi(argv[2]) * 3 + 1;
    char out[n] = {0};

    char *pout = utf8_strncpy(out, utf8_ltrim(argv[1]), atoi(argv[2]));
    if (*pout != '\0' && *out != '\0') {
        printf("out:%s\n", out);
    }

    return 0;
}

#endif
