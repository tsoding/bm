#ifndef TARGET_H_
#define TARGET_H_

#include "./sv.h"

typedef enum {
    TARGET_BM = 0,
    TARGET_NASM_LINUX_X86_64,
    TARGET_NASM_FREEBSD_X86_64,
    COUNT_TARGETS
} Target;

String_View target_file_ext(Target target);
bool target_by_name(const char *name, Target *format);

#endif // TARGET_H_
