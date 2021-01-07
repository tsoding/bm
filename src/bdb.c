/*
 * BDB - Debugger for the Birtual Machine
 * Copyright 2020 by Nico Sonack
 *
 * Contribution to https://github.com/tsoding/bm
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "bdb.h"

#define BM_IMPLEMENTATION
#include "bm.h"

Bdb_Err bdb_state_init(bdb_state *state,
                          const char *executable)
{
    assert(state);
    assert(executable);

    state->cood_file_name = sv_from_cstr(executable);
    bm_load_program_from_file(&state->bm, executable);

    char buf[PATH_MAX];
    memcpy(buf, executable, strlen(executable));
    memcpy(buf + strlen(executable), ".sym", 5);

    if (access(buf, R_OK) == 0)
    {
        fprintf(stdout, "INFO : Loading debug symbols...\n");
        return bdb_load_symtab(state, buf);
    }
    else
    {
        return BDB_OK;
    }
}

Bdb_Err bdb_mmap_file(const char *path, String_View *out)
{
    assert(path);
    assert(out);

    int fd;

    if ((fd = open(path, O_RDONLY)) < 0)
    {
        return BDB_FAIL;
    }

    struct stat stat_buf;
    if (fstat(fd, &stat_buf) < 0)
    {
        return BDB_FAIL;
    }

    char *content = mmap(NULL, (size_t)stat_buf.st_size,
                         PROT_READ, MAP_PRIVATE, fd, 0);
    if (content == MAP_FAILED)
    {
        return BDB_FAIL;
    }

    out->data = content;
    out->count = (size_t)stat_buf.st_size;

    close(fd);

    return BDB_OK;
}

Bdb_Err bdb_find_addr_of_label(bdb_state *state, const char *name, Inst_Addr *out)
{
    String_View _name = sv_trim_right(sv_from_cstr(name));
    for (Inst_Addr i = 0; i < BM_PROGRAM_CAPACITY; ++i)
    {
        if (state->labels[i].data && sv_eq(state->labels[i], _name))
        {
            *out = i;
            return BDB_OK;
        }
    }

    return BDB_FAIL;
}

Bdb_Err bdb_load_symtab(bdb_state *state, const char *symtab_file)
{
    String_View symtab;
    if (bdb_mmap_file(symtab_file, &symtab) == BDB_FAIL)
    {
        return BDB_FAIL;
    }

    while (symtab.count > 0)
    {
        symtab = sv_trim_left(symtab);
        String_View raw_addr   = sv_chop_by_delim(&symtab, '\t');
        symtab = sv_trim_left(symtab);
        String_View label_name = sv_chop_by_delim(&symtab, '\n');
        Inst_Addr addr = (Inst_Addr)sv_to_int(raw_addr);

        /*
         * Huh? you ask? Yes, if we have a label, whose size is bigger
         * than the program size, which is to say, that it is a
         * preprocessor label, then we don't wanna overrun our label
         * storage buffer.
         */
        if (addr < BM_PROGRAM_CAPACITY)
        {
            state->labels[addr] = label_name;
        }
    }

    return BDB_OK;
}

static
void usage(void)
{
    fprintf(stderr,
            "usage: bdb <executable>\n");
}

void bdb_print_instr(FILE *f, Inst *i)
{
    fprintf(f, "%s ", inst_name(i->type));
    if (inst_has_operand(i->type))
    {
        fprintf(f, "%" PRIu64, i->operand.as_i64);
    }
}

void bdb_add_breakpoint(bdb_state *state, Inst_Addr addr)
{
    if (addr > state->bm.program_size)
    {
        fprintf(stderr, "ERR : Symbol out of program\n");
        return;
    }

    if (addr > BM_PROGRAM_CAPACITY)
    {
        fprintf(stderr, "ERR : Symbol out of memory\n");
        return;
    }

    if (state->breakpoints[addr].is_enabled)
    {
        fprintf(stderr, "ERR : Breakpoint already set\n");
        return;
    }

    state->breakpoints[addr].is_enabled = 1;
}

void bdb_delete_breakpoint(bdb_state *state, Inst_Addr addr)
{
    if (addr > state->bm.program_size)
    {
        fprintf(stderr, "ERR : Symbol out of program\n");
        return;
    }

    if (addr > BM_PROGRAM_CAPACITY)
    {
        fprintf(stderr, "ERR : Symbol out of memory\n");
        return;
    }

    if (!state->breakpoints[addr].is_enabled)
    {
        fprintf(stderr, "ERR : No such breakpoint\n");
        return;
    }

    state->breakpoints[addr].is_enabled = 0;
}

Bdb_Err bdb_continue(bdb_state *state)
{
    if (state->bm.halt)
    {
        fprintf(stderr, "ERR : Program is not being run\n");
        return BDB_OK;
    }

    do
    {
        bdb_breakpoint *bp = &state->breakpoints[state->bm.ip];
        if (!bp->is_broken && bp->is_enabled)
        {
            fprintf(stdout, "Hit breakpoint at %"PRIu64, state->bm.ip);
            if (state->labels[state->bm.ip].data)
            {
                fprintf(stdout, " label '%.*s'",
                        (int)state->labels[state->bm.ip].count,
                        state->labels[state->bm.ip].data);
            }

            fprintf(stdout, "\n");
            bp->is_broken = 1;

            return BDB_OK;
        }

        bp->is_broken = 0;

        Err err = bm_execute_inst(&state->bm);
        if (err)
        {
            return bdb_fault(state, err);
        }
    }
    while (!state->bm.halt);

    printf("Program halted.\n");

    return BDB_OK;
}

Bdb_Err bdb_fault(bdb_state *state, Err err)
{
    fprintf(stderr, "%s at %" PRIu64 " (INSTR: ",
            err_as_cstr(err), state->bm.ip);
    bdb_print_instr(stderr, &state->bm.program[state->bm.ip]);
    fprintf(stderr, ")\n");
    state->bm.halt = 1;
    return BDB_OK;
}

Bdb_Err bdb_step_instr(bdb_state *state)
{
    if (state->bm.halt)
    {
        fprintf(stderr, "ERR : Program is not being run\n");
        return BDB_OK;
    }

    Err err = bm_execute_inst(&state->bm);
    if (!err)
    {
        return BDB_OK;
    }
    else
    {
        return bdb_fault(state, err);
    }
}

Bdb_Err bdb_parse_label_or_addr(bdb_state *st, const char *in, Inst_Addr *out)
{
    char *endptr = NULL;
    size_t len = strlen(in);

    *out = strtoull(in, &endptr, 10);
    if (endptr != in + len)
    {
        if (bdb_find_addr_of_label(st, in, out) == BDB_FAIL)
        {
            return BDB_FAIL;
        }
    }

    return BDB_OK;
}

/*
 * TODO: support for native function in the debugger
 */
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        usage();
        return EXIT_FAILURE;
    }

    /*
     * Create the BDB state and initialize it with the file names
     */

    bdb_state state = {0};
    state.bm.halt = 1;

    printf("BDB - The birtual machine debugger.\n");
    if (bdb_state_init(&state, argv[1]) == BDB_FAIL)
    {
        fprintf(stderr,
                "FATAL : Unable to initialize the debugger: %s\n",
                strerror(errno));
    }

    while (1)
    {
        printf("(bdb) ");
        char input_buf[32];
        fgets(input_buf, 32, stdin);

        switch (*input_buf)
        {
        /*
         * Next instruction
         */
        case 'n':
        {
            Bdb_Err res = bdb_step_instr(&state);
            if (res == BDB_FAIL)
            {
                return EXIT_FAILURE;
            }

            printf("-> ");
            bdb_print_instr(stdout, &state.bm.program[state.bm.ip]);
            printf("\n");
        } break;
        /*
         * Print the IP
         */
        case 'i':
        {
            printf("ip = %" PRIu64 "\n", state.bm.ip);
        } break;
        /*
         * Dump the stack
         */
        case 's':
        {
            bm_dump_stack(stdout, &state.bm);
        } break;
        case 'b':
        {
            char *addr = input_buf + 2;
            Inst_Addr break_addr;

            if (bdb_parse_label_or_addr(&state, addr, &break_addr) == BDB_FAIL)
            {
                fprintf(stderr, "ERR : Cannot parse address or labels\n");
                continue;
            }

            bdb_add_breakpoint(&state, break_addr);
            fprintf(stdout, "INFO : Breakpoint set at %"PRIu64"\n", break_addr);
        } break;
        case 'd':
        {
            char *addr = input_buf + 2;
            Inst_Addr break_addr;

            if (bdb_parse_label_or_addr(&state, addr, &break_addr) == BDB_FAIL)
            {
                fprintf(stderr, "ERR : Cannot parse address or labels\n");
                continue;
            }

            bdb_delete_breakpoint(&state, break_addr);
            fprintf(stdout, "INFO : Deleted breakpoint at %"PRIu64"\n", break_addr);
        } break;
        case 'r':
        {
            if (!state.bm.halt)
            {
                fprintf(stderr, "ERR : Program is already running\n");
                /* TODO: Reset bm and restart program */
            }

            state.bm.halt = 0;
        } /* fall through */
        case 'c':
        {
            if (bdb_continue(&state))
            {
                return EXIT_FAILURE;
            }
        } break;
        case EOF:
        case 'q':
        {
            return EXIT_SUCCESS;
        }
        case 'h':
        {
            printf("r - run program\n"
                   "n - next instruction\n"
                   "c - continue program execution\n"
                   "s - stack dump\n"
                   "i - instruction pointer\n"
                   "b - set breakpoint at address or label\n"
                   "d - destroy breakpoint at address or label\n"
                   "q - quit\n");
        } break;
        default:
        {
            fprintf(stderr, "?\n");
        } break;
        }
    }

    return EXIT_SUCCESS;
}
