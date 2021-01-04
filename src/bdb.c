#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "bdb.h"

#define BM_IMPLEMENTATION
#include "bm.h"

void bdb_state_init(bdb_state *state,
                    const char *executable,
                    const char *symtab)
{
    assert(state);
    assert(executable);
    assert(symtab);

    state->cood_file_name   = cstr_as_sv(executable);
    state->symtab_file_name = cstr_as_sv(executable);
    bm_load_program_from_file(&state->bm, executable);
}

static
void usage(void)
{
    fprintf(stderr,
            "usage: bdb <executable> <debug symbol table>\n");
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

bdb_status bdb_continue(bdb_state *state)
{
    if (state->bm.halt)
    {
        fprintf(stderr, "ERR : Program is not being run\n");
        return BDB_OK;
    }

    while (!state->bm.halt)
    {
        if (state->breakpoints[state->bm.ip].is_enabled)
        {
            fprintf(stdout, "Hit breakpoint at %"PRIu64"\n", state->bm.ip);
            return BDB_OK;
        }

        Err err = bm_execute_inst(&state->bm);
        if (err)
        {
            return bdb_fault(state, err);
        }
    }

    return BDB_OK;
}

bdb_status bdb_fault(bdb_state *state, Err err)
{
    fprintf(stderr, "%s at %" PRIu64 " (INSTR: ",
            err_as_cstr(err), state->bm.ip);
    bdb_print_instr(stderr, &state->bm.program[state->bm.ip]);
    fprintf(stderr, ")\n");
    state->bm.halt = 1;
    return BDB_OK;
}

bdb_status bdb_step_instr(bdb_state *state)
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

    return bdb_fault(state, err);
}

int main(int argc, char **argv)
{
    if (argc < 3)
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
    bdb_state_init(&state, argv[1], argv[2]);

    while (1)
    {
        printf("(bdb) ");
        char input_buf[64];
        fgets(input_buf, 64, stdin);

        switch (*input_buf)
        {
        /*
         * Next instruction
         */
        case 'n':
        {
            bdb_status res = bdb_step_instr(&state);
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
            Inst_Addr break_addr = atoi(addr);
            bdb_add_breakpoint(&state, break_addr);
        } break;
        case 'd':
        {
            char *addr = input_buf + 2;
            Inst_Addr break_addr = atoi(addr);
            bdb_delete_breakpoint(&state, break_addr);
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
            printf("n - next instruction\n"
                   "s - stack dump\n"
                   "i - instruction pointer\n"
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
