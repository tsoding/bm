/*
 * BDB - Debugger for the Birtual Machine
 * Copyright 2020 by Nico Sonack
 *
 * Contribution to https://github.com/tsoding/bm
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "bdb.h"

#define BM_IMPLEMENTATION
#include "bm.h"

static
void usage(void)
{
    fprintf(stderr,
            "usage: bdb <executable>\n");
}

Bdb_Err bdb_state_init(Bdb_State *state,
                       const char *executable)
{
    assert(state);
    assert(executable);

    state->cood_file_name = sv_from_cstr(executable);
    bm_load_program_from_file(&state->bm, executable);
    bm_load_standard_natives(&state->bm);

    fprintf(stdout, "INFO : Loading debug symbols...\n");
    return bdb_load_symtab(state, arena_sv_concat2(&state->arena, executable, ".sym"));
}

Bdb_Err bdb_find_addr_of_label(Bdb_State *state, String_View name, Inst_Addr *out)
{
    assert(state);
    assert(out);

    for (Inst_Addr i = 0; i < BM_PROGRAM_CAPACITY; ++i)
    {
        if (state->labels[i].data && sv_eq(state->labels[i], name))
        {
            *out = i;
            return BDB_OK;
        }
    }

    return BDB_FAIL;
}

Bdb_Err bdb_load_symtab(Bdb_State *state, String_View symtab_file)
{
    assert(state);

    String_View symtab = arena_slurp_file(&state->arena, symtab_file);

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

void bdb_print_instr(FILE *f, Inst *i)
{
    fprintf(f, "%s ", inst_name(i->type));
    if (inst_has_operand(i->type))
    {
        fprintf(f, "%" PRIu64, i->operand.as_i64);
    }
}

void bdb_add_breakpoint(Bdb_State *state, Inst_Addr addr)
{
    assert(state);

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

void bdb_delete_breakpoint(Bdb_State *state, Inst_Addr addr)
{
    assert(state);

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

Bdb_Err bdb_continue(Bdb_State *state)
{
    assert(state);

    if (state->bm.halt)
    {
        fprintf(stderr, "ERR : Program is not being run\n");
        return BDB_OK;
    }

    do
    {
        Bdb_Breakpoint *bp = &state->breakpoints[state->bm.ip];
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

Bdb_Err bdb_fault(Bdb_State *state, Err err)
{
    assert(state);

    fprintf(stderr, "%s at %" PRIu64 " (INSTR: ",
            err_as_cstr(err), state->bm.ip);
    bdb_print_instr(stderr, &state->bm.program[state->bm.ip]);
    fprintf(stderr, ")\n");
    state->bm.halt = 1;
    return BDB_OK;
}

Bdb_Err bdb_step_instr(Bdb_State *state)
{
    assert(state);

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

Bdb_Err bdb_parse_label_or_addr(Bdb_State *st, String_View addr, Inst_Addr *out)
{
    assert(st);
    assert(out);

    char *endptr = NULL;

    *out = strtoull(addr.data, &endptr, 10);
    if (endptr != addr.data + addr.count)
    {
        if (bdb_find_addr_of_label(st, addr, out) == BDB_FAIL)
        {
            return BDB_FAIL;
        }
    }

    return BDB_OK;
}

Bdb_State state = {0};

/*
 * TODO(#85): there is no way to examine the memory in bdb
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

    state.bm.halt = 1;

    printf("BDB - The birtual machine debugger.\n"
           "Type 'h' and enter for a quick help\n");
    if (bdb_state_init(&state, argv[1]) == BDB_FAIL)
    {
        fprintf(stderr,
                "FATAL : Unable to initialize the debugger: %s\n",
                strerror(errno));
    }

    // TODO(#94): repeat previous command in bdb
    while (1)
    {
        printf("(bdb) ");
        char input_buf[32] = {0};
        fgets(input_buf, 32, stdin);

        String_View
            input_sv = sv_from_cstr(input_buf),
            control_word = sv_trim(sv_chop_by_delim(&input_sv, ' '));

        switch (*control_word.data)
        {
        /*
         * Next instruction
         */
        case 'n':
        {
            Bdb_Err err = bdb_step_instr(&state);
            if (err)
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
            Inst_Addr break_addr;
            String_View addr = sv_trim(input_sv);

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
            Inst_Addr break_addr;
            String_View addr = sv_trim(input_sv);

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
                /* TODO(#88): Reset bm and restart program */
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
