#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include "./bang_diag.h"

const char *bang_diag_level_label(Bang_Diag_Level level)
{
    switch (level) {
    case BANG_DIAG_NOTE:
        return "NOTE";
    case BANG_DIAG_WARNING:
        return "WARNING";
    case BANG_DIAG_ERROR:
        return "ERROR";
    default:
        assert(false && "unreachable");
        exit(1);
    }
}

void bang_diag_msg(Bang_Loc loc, Bang_Diag_Level level, const char *fmt, ...)
{
    fprintf(stderr, Bang_Loc_Fmt": %s: ", Bang_Loc_Arg(loc), bang_diag_level_label(level));

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
}
