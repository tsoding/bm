#include <assert.h>
#include <string.h>
#include "./target.h"

String_View target_file_ext(Target target)
{
    switch (target) {
    case TARGET_BM:
        return SV(".bm");
    case TARGET_NASM_LINUX_X86_64:
        return SV(".asm");
    case TARGET_NASM_FREEBSD_X86_64:
        return SV(".S");
    case COUNT_TARGETS:
    default:
        assert(false && "output_format_file_ext: unreachable");
        exit(1);
    }
}

bool target_by_name(const char *name, Target *target)
{
    static_assert(
        COUNT_TARGETS == 3,
        "Please add a condition branch for a new output "
        "and increment the counter above");
    if (strcmp(name, "bm") == 0) {
        *target = TARGET_BM;
        return true;
    } else if (strcmp(name, "nasm-linux-x86-64") == 0) {
        *target = TARGET_NASM_LINUX_X86_64;
        return true;
    } else if (strcmp(name, "nasm-freebsd-x86-64") == 0) {
        *target = TARGET_NASM_FREEBSD_X86_64;
        return true;
    } else {
        return false;
    }
}
