#include <stdio.h>
#include <string.h>
#include "ctlog.h"

int log_level = info;
char log_sid[1024] = {0};

void ctlog(const char *ident, int option, int facility)
{
    openlog(ident, option, facility);
}
