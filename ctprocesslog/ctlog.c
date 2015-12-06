#include <stdio.h>
#include <string.h>
#include "ctlog.h"

int log_level;

void ctlog(const char *ident, int option, int facility)
{
    openlog(ident, option, facility);
}
