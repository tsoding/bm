#ifndef BANG_DIAG_H_
#define BANG_DIAG_H_

#include <stdlib.h>

#if defined(__GNUC__) || defined(__clang__)
// https://gcc.gnu.org/onlinedocs/gcc-4.7.2/gcc/Function-Attributes.html
#define BANG_PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK) __attribute__ ((format (printf, STRING_INDEX, FIRST_TO_CHECK)))
#else
#define BANG_PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK)
#endif

typedef struct {
    size_t row;
    size_t col;
    const char *file_path;
} Bang_Loc;

#define Bang_Loc_Fmt "%s:%zu:%zu"
#define Bang_Loc_Arg(loc) (loc).file_path, (loc).row, (loc).col

typedef enum {
    BANG_DIAG_NOTE,
    BANG_DIAG_WARNING,
    BANG_DIAG_ERROR,
} Bang_Diag_Level;

const char *bang_diag_level_label(Bang_Diag_Level level);
void bang_diag_msg(Bang_Loc loc, Bang_Diag_Level level, const char *fmt, ...) BANG_PRINTF_FORMAT(3, 4);

#endif // BANG_DIAG_H_
