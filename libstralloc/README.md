libmfile
=======

Intro
-----

[libstralloc] is a class of process string.


Usage
------------

```bash
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "stralloc.h"

int main(int argc, char **argv)
{
    stralloc str = {0};
    
    int ret = stralloc_copys(&str, "Greeting: ");
    int ret2 = stralloc_cats(&str, "Grayson :)");
    
    printf("%d %d\n", ret, ret2);
    printf("%s\n", str.s);

	stralloc_free(&str);
    
    
    return 0;
}


gcc -g -o test test.c stralloc.c
````



Installation
------------

#### Download Source Package

```bash
make
make install
```


