#ifndef TARGET_H_
#define TARGET_H_

#include <stdbool.h>

typedef enum {
    TARGET_BM = 0,
    TARGET_NASM_LINUX_X86_64,
    TARGET_NASM_FREEBSD_X86_64,
    TARGET_GAS_FREEBSD_ARM64,
    COUNT_TARGETS
} Target;

const char *target_file_ext(Target target);
const char *target_name(Target target);
bool target_by_name(const char *name, Target *target);

#endif // TARGET_H_
