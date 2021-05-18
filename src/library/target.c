#include <assert.h>
#include <string.h>
#include "./target.h"

static const char * const target_names[COUNT_TARGETS] = {
    [TARGET_BM] = "bm",
    [TARGET_NASM_LINUX_X86_64] = "nasm-linux-x86-64",
    [TARGET_NASM_FREEBSD_X86_64] = "nasm-freebsd-x86-64",
};
static_assert(COUNT_TARGETS == 3, "Amount of target names have changed");

static const char * const target_exts[COUNT_TARGETS] = {
    [TARGET_BM] = ".bm",
    [TARGET_NASM_LINUX_X86_64] = ".asm",
    [TARGET_NASM_FREEBSD_X86_64] = ".S",
};
static_assert(COUNT_TARGETS == 3, "Amount of target extensions have changed");

const char *target_file_ext(Target target)
{
    assert(0 <= target && target < COUNT_TARGETS);
    return target_exts[target];
}

const char *target_name(Target target)
{
    assert(0 <= target && target < COUNT_TARGETS);
    return target_names[target];
}

bool target_by_name(const char *name, Target *target_out)
{
    for (Target target = 0; target < COUNT_TARGETS; ++target) {
        if (strcmp(name, target_name(target)) == 0) {
            if (target_out) {
                *target_out = target;
            }
            return true;
        }
    }
    return false;
}
